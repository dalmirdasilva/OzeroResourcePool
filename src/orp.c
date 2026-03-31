/**
 * ORP - Ozero Resource Pool
 *
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#include <orp.h>
#include <orp_io.h>
#include <orp_util.h>
#include <stdint.h>

orp_global_flags_t orp_global_flags;

orp_op_result_t orp_format(orp_t *orp) {
	uint8_t i;
	_orp_write_rp_to_disk(orp->driver, orp);
	for (i = 0; i < orp->resource_descriptor_count; i++) {
		_orp_format_resource_descriptor(orp, i);
	}
	for (i = 0; i < orp->cluster_count; i++) {
		_orp_format_cluster(orp, i);
	}
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

orp_op_result_t orp_mount(orp_driver_t driver, orp_t *orp, orp_mount_options_t options) {
	if (_orp_is_driver_mounted(driver)) {
		return OZERO_ORP_OP_RESULT_ERROR_DRIVER_BUSY;
	}
	_orp_read_orp_from_disk(driver, orp);
	_orp_set_driver_mounted(driver, 1);
	if (options & OZERO_ORP_MOUNT_OPTION_READ_ONLY) {
		orp->flags |= OZERO_ORP_FLAG_BIT_READ_ONLY;
	}
	orp->driver = driver;
	_orp_free_resource_descriptors(orp);
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

orp_op_result_t orp_umount(orp_t *orp) {
	if (_orp_is_driver_mounted(orp->driver)) {
		_orp_set_driver_mounted(orp->driver, 0);
	}
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

orp_op_result_t orp_open(orp_t *orp, orp_resource_code_t resource_code, orp_resource_t *resource, orp_open_resource_options_t options) {
	orp_memory_address_t address;
	orp_resource_descriptor_t resource_descriptor;
	uint8_t flags;
	if (!_orp_is_driver_mounted(orp->driver)) {
		return OZERO_ORP_OP_RESULT_ERROR_DRIVER_NOT_MOUNTED;
	}
	resource_descriptor = _orp_resource_code_to_resource_descriptor(resource_code);
	address = _orp_resource_descriptor_to_address(orp, resource_descriptor);
	flags = _orp_io_read(orp->driver, ORP_RD_ADDRESS_TO_FLAG(address));
	if (!(flags & OZERO_ORP_RESOURCE_FLAG_BIT_ALLOCATED)) {
		return OZERO_ORP_OP_RESULT_ERROR_RESOURCE_DOES_NOT_ALLOCATED;
	}
	if (flags & OZERO_ORP_RESOURCE_FLAG_BIT_OPENED) {
		return OZERO_ORP_OP_RESULT_ERROR_RESOURCE_OPENED;
	}
	flags |= OZERO_ORP_RESOURCE_FLAG_BIT_OPENED;
	if ((options & OZERO_ORP_OPEN_RESOURCE_OPTION_READ_ONLY) || (orp->flags & OZERO_ORP_FLAG_BIT_READ_ONLY)) {
		flags |= OZERO_ORP_RESOURCE_FLAG_BIT_READ_ONLY;
	}
	_orp_io_write(orp->driver, ORP_RD_ADDRESS_TO_FLAG(address), flags);
	resource->resource_descriptor = resource_descriptor;
	resource->first_cluster = _orp_io_read(orp->driver, ORP_RD_ADDRESS_TO_FIRST_CLUSTER(address));
	resource->current_cluster = resource->first_cluster;
	resource->cluster_offset = orp->sizeof_cluster_control;
	resource->current_position = 0;
	_orp_io_read_bulk(orp->driver, address, (uint8_t *) &resource->size, ORP_SIZEOF_RESOURCE_SIZE);
	resource->flags = flags;
	_orp_check_for_eor_reached(resource);
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

orp_op_result_t orp_close(orp_t *orp, orp_resource_t *resource) {
	orp_sync(orp, resource);
	_orp_free_resource_descriptor(orp, resource->resource_descriptor);
	resource->flags &= ~OZERO_ORP_RESOURCE_FLAG_BIT_OPENED;
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

uint8_t orp_read(orp_t *orp, orp_resource_t *resource) {
	orp_memory_address_t address;
	uint8_t read_data;
	if (!(resource->flags & OZERO_ORP_RESOURCE_FLAG_BIT_OPENED)) {
		resource->flags |= OZERO_ORP_RESOURCE_FLAG_BIT_ERROR_ON_LAST_READ;
		return 0;
	}
	if (_orp_is_eor_reached(resource)) {
		return 0;
	}
	_orp_check_for_availability(orp, resource);
	address = _orp_cluster_to_address(orp, resource->current_cluster);
	read_data = _orp_io_read(orp->driver, address + resource->cluster_offset);
	resource->current_position++;
	resource->cluster_offset++;
	_orp_check_for_eor_reached(resource);
	return read_data;
}

orp_op_result_t orp_write(orp_t *orp, orp_resource_t *resource, uint8_t data_to_write) {
	orp_memory_address_t address;
	if (!(resource->flags & OZERO_ORP_RESOURCE_FLAG_BIT_OPENED)) {
		return OZERO_ORP_OP_RESULT_ERROR_RESOURCE_CLOSED;
	}
	if (resource->flags & OZERO_ORP_RESOURCE_FLAG_BIT_READ_ONLY) {
		return OZERO_ORP_OP_RESULT_ERROR_RESOURCE_READ_ONLY;
	}
	if (!_orp_check_for_availability(orp, resource)) {
		return OZERO_ORP_OP_RESULT_ERROR_NO_SPACE_AVAILABLE;
	}
	address = _orp_cluster_to_address(orp, resource->current_cluster);
	_orp_io_write(orp->driver, address + resource->cluster_offset, data_to_write);
	resource->cluster_offset++;
	resource->current_position++;
	if (orp_eor(resource)) {
		resource->size++;
		orp_sync(orp, resource);
	}
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

orp_op_result_t orp_seek(orp_t *orp, orp_resource_t *resource, orp_seek_origin_t origin, orp_seek_int_t offset) {
	int16_t new_position = 0;
	if (resource->size == 0) {
		return OZERO_ORP_OP_RESULT_SUCCESS;
	}
	switch (origin) {
		case OZERO_ORP_SEEK_ORIGIN_BEGIN:
			new_position = offset;
			break;
		case OZERO_ORP_SEEK_ORIGIN_CURRENT:
			new_position = resource->current_position + offset;
			break;
	}
	new_position %= resource->size + 1;
	if (new_position < 0) {
		new_position += resource->size;
	}
	if (new_position == 0) {
		orp_rewind(orp, resource);
		return OZERO_ORP_OP_RESULT_SUCCESS;
	}
	if (new_position < resource->current_position) {
		if (new_position > (resource->current_position - new_position)) {
			_orp_move_current_position_back(orp, resource, (resource->current_position - new_position));
		} else {
			orp_rewind(orp, resource);
			_orp_move_current_position_ahead(orp, resource, new_position);
		}
	} else {
		_orp_move_current_position_ahead(orp, resource, (new_position - resource->current_position));
	}
	_orp_check_for_eor_reached(resource);
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

orp_op_result_t orp_truncate(orp_t *orp, orp_resource_t *resource) {
	uint8_t flags;
	orp_memory_address_t resource_descriptor_address;
	uint8_t freed_clusters = 0;
	resource_descriptor_address = _orp_resource_descriptor_to_address(orp, resource->resource_descriptor);
	flags = _orp_io_read(orp->driver, ORP_RD_ADDRESS_TO_FLAG(resource_descriptor_address));
	if (!(flags & OZERO_ORP_RESOURCE_FLAG_BIT_ALLOCATED)) {
		return OZERO_ORP_OP_RESULT_ERROR_RESOURCE_DOES_NOT_ALLOCATED;
	}
	if (resource->size > orp->sizeof_cluster_data) {
		freed_clusters = _orp_format_cluster_chain(orp, _orp_next_cluster_by_cluster(orp, resource->first_cluster));
	}
	_orp_increase_free_clusters(orp, freed_clusters);
	resource->size = 0x00;
	_orp_io_write(orp->driver, ORP_RD_ADDRESS_TO_SIZE_LOW(resource_descriptor_address), 0x00);
	_orp_io_write(orp->driver, ORP_RD_ADDRESS_TO_SIZE_HIGH(resource_descriptor_address), 0x00);
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

void orp_sync(orp_t *orp, orp_resource_t *resource) {
	orp_memory_address_t address;
	address = _orp_resource_descriptor_to_address(orp, resource->resource_descriptor);
	_orp_io_write_bulk(orp->driver, address, (const uint8_t *) &resource->size, ORP_SIZEOF_RESOURCE_SIZE);
}

void orp_stat(orp_t *orp, orp_resource_t *resource, orp_stat_t *stat) {// TODO
	stat->flags = 0xff;
}

orp_op_result_t orp_rewind(orp_t *orp, orp_resource_t *resource) {
	resource->current_cluster = resource->first_cluster;
	resource->cluster_offset = orp->sizeof_cluster_control;
	resource->current_position = 0;
	_orp_check_for_eor_reached(resource);
	return OZERO_ORP_OP_RESULT_SUCCESS;
}

orp_resource_code_t orp_alloc(orp_t *orp) {
	uint8_t i;
	uint8_t flags;
	orp_cluster_t first_cluster;
	orp_memory_address_t resource_descriptor_address, cluster_address;
	if (orp->free_clusters < 1) {
		return ORP_NULL_RESOURCE_CODE;
	}
	resource_descriptor_address = orp->resource_descriptor_table_address;
	for (i = 0; i < orp->resource_descriptor_count; i++) {
		flags = _orp_io_read(orp->driver, ORP_RD_ADDRESS_TO_FLAG(resource_descriptor_address));
		if (!(flags & OZERO_ORP_RESOURCE_FLAG_BIT_ALLOCATED)) {
			cluster_address = _orp_alloc_cluster(orp);
			if (cluster_address == ORP_NULL_CLUSTER_ADDRESS) {
				return ORP_NULL_RESOURCE_CODE;
			}
			flags |= OZERO_ORP_RESOURCE_FLAG_BIT_ALLOCATED;
			first_cluster = _orp_address_to_cluster(orp, cluster_address);
			_orp_create_cluster_chain(orp, first_cluster, ORP_INEXISTENT_CLUSTER);
			_orp_io_write(orp->driver, ORP_RD_ADDRESS_TO_FIRST_CLUSTER(resource_descriptor_address), first_cluster);
			_orp_io_write(orp->driver, ORP_RD_ADDRESS_TO_FLAG(resource_descriptor_address), flags);
			return _orp_resource_descriptor_to_resource_code(i);
		}
		resource_descriptor_address += orp->sizeof_resource_descriptor;
	}
	return ORP_NULL_RESOURCE_CODE;
}

uint8_t orp_release(orp_t *orp, orp_resource_t *resource) {
	uint8_t flags;
	orp_memory_address_t resource_descriptor_address;
	resource_descriptor_address = _orp_resource_descriptor_to_address(orp, resource->resource_descriptor);
	flags = _orp_io_read(orp->driver, ORP_RD_ADDRESS_TO_FLAG(resource_descriptor_address));
	if (!(flags & OZERO_ORP_RESOURCE_FLAG_BIT_ALLOCATED)) {
		return 1;
	}
	_orp_format_resource_clusters(orp, resource);
	_orp_format_resource_descriptor(orp, resource->resource_descriptor);
	resource->flags = 0x00;
	return 1;
}

orp_resource_size_t orp_size(orp_resource_t *resource) {
	return resource->size;
}

orp_resource_size_t orp_tell(orp_resource_t *resource) {
	return resource->current_position;
}

uint8_t orp_eor(orp_resource_t *resource) {
	return _orp_is_eor_reached(resource);
}

uint8_t orp_error(orp_resource_t *resource) {
	return (resource->flags & OZERO_ORP_RESOURCE_FLAG_BIT_ERROR_ON_LAST_READ || resource->flags & OZERO_ORP_RESOURCE_FLAG_BIT_ERROR_ON_LAST_WRITE);
}

orp_resource_size_t orp_available_space(orp_t *orp) {
	return orp->free_clusters * orp->sizeof_cluster_data;
}

orp_resource_size_t orp_total_space(orp_t *orp) {
	return orp->cluster_count * orp->sizeof_cluster_data;
}