/**
 * ORP - Ozero Resource Pool
 *
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#include "orp_util.h"
#include "orp_io.h"

void _orp_write_rp_to_disk(orp_driver_t driver, orp_t *orp) {
	_orp_io_write_bulk(driver, ORP_FIRST_ADDRESS_OF_MEMORY, (const uint8_t *) orp, sizeof(orp_t));
}

void _orp_read_orp_from_disk(orp_driver_t driver, orp_t *orp) {
	_orp_io_read_bulk(driver, ORP_FIRST_ADDRESS_OF_MEMORY, (uint8_t *) orp, sizeof(orp_t));
}

orp_memory_address_t _orp_alloc_cluster(orp_t *orp) {
	orp_memory_address_t address;
	uint8_t i;
	address = orp->cluster_table_address;
	for (i = 0; i < orp->cluster_count; i++) {
		if (_orp_is_free_cluster(orp, (orp_cluster_t) i)) {
			_orp_decrease_free_clusters(orp, 1);
			return address;
		}
		address += orp->sizeof_cluster;
	}
	return ORP_NULL_CLUSTER_ADDRESS;
}

uint8_t _orp_is_free_cluster(orp_t *orp, orp_cluster_t cluster) {
	return (cluster == _orp_prev_cluster_by_cluster(orp, cluster)) && (cluster == _orp_next_cluster_by_cluster(orp, cluster));
}

void _orp_format_cluster(orp_t *orp, orp_cluster_t cluster) {
	orp_memory_address_t address;
	address = _orp_cluster_to_address(orp, cluster);
	_orp_io_write(orp->driver, ORP_CLUSTER_ADDRESS_TO_NEXT(address), cluster);
	_orp_io_write(orp->driver, ORP_CLUSTER_ADDRESS_TO_PREV(address), cluster);
}

void _orp_free_cluster(orp_t *orp, orp_cluster_t cluster) {
	_orp_format_cluster(orp, cluster);
	_orp_increase_free_clusters(orp, 1);
}

void _orp_create_cluster_chain(orp_t *orp, orp_cluster_t prev_cluster, orp_cluster_t next_cluster) {
	orp_memory_address_t address;
	if (prev_cluster != ORP_INEXISTENT_CLUSTER) {
		address = _orp_cluster_to_address(orp, prev_cluster);
		_orp_io_write(orp->driver, ORP_CLUSTER_ADDRESS_TO_NEXT(address), (uint8_t) next_cluster);
	}
	if (next_cluster != ORP_INEXISTENT_CLUSTER) {
		address = _orp_cluster_to_address(orp, next_cluster);
		_orp_io_write(orp->driver, ORP_CLUSTER_ADDRESS_TO_PREV(address), (uint8_t) prev_cluster);
	}
}

void _orp_check_for_eor_reached(orp_resource_t *resource) {
	if (resource->current_position >= resource->size) {
		resource->flags |= OZERO_ORP_RESOURCE_FLAG_BIT_EOR_REACHED;
	} else {
		resource->flags &= ~OZERO_ORP_RESOURCE_FLAG_BIT_EOR_REACHED;
	}
}

uint8_t _orp_is_eor_reached(orp_resource_t *resource) {
	return resource->flags & OZERO_ORP_RESOURCE_FLAG_BIT_EOR_REACHED;
}

uint8_t _orp_check_for_availability(orp_t *orp, orp_resource_t *resource) {
	orp_memory_address_t address;
	orp_cluster_t cluster;
	_orp_check_for_eor_reached(resource);
	if (resource->cluster_offset >= orp->sizeof_cluster) {
		if (orp_eor(resource)) {
			address = _orp_alloc_cluster(orp);
			if (address == ORP_NULL_CLUSTER_ADDRESS) {
				return 0;
			}
			cluster = _orp_address_to_cluster(orp, address);
			_orp_create_cluster_chain(orp, resource->current_cluster, cluster);
			resource->current_cluster = cluster;
		} else {
			resource->current_cluster = _orp_next_cluster_by_cluster(orp, resource->current_cluster);
		}
		resource->cluster_offset = orp->sizeof_cluster_control;
	}
	return 1;
}

uint8_t _orp_move_current_position_ahead(orp_t *orp, orp_resource_t *resource, orp_seek_int_t offset) {
	uint8_t until_the_end;
	uint8_t how_many_clusters_ahead;
	uint8_t i;
	resource->current_position += offset;
	until_the_end = (orp->sizeof_cluster - resource->cluster_offset);
	if (offset <= until_the_end) {
		resource->cluster_offset += offset;
		return 1;
	}
	offset -= until_the_end;
	how_many_clusters_ahead = (offset / orp->sizeof_cluster_data) + 1;
	resource->cluster_offset = (offset % orp->sizeof_cluster_data) + orp->sizeof_cluster_control;
	for (i = 0; i < how_many_clusters_ahead; i++) {
		resource->current_cluster = _orp_next_cluster_by_cluster(orp, resource->current_cluster);
	}
	return 1;
}

uint8_t _orp_move_current_position_back(orp_t *orp, orp_resource_t *resource, orp_seek_int_t offset) {
	uint8_t until_the_begin;
	uint8_t how_many_clusters_back;
	uint8_t i;
	resource->current_position -= offset;
	until_the_begin = (resource->cluster_offset - orp->sizeof_cluster_control);
	if (offset <= until_the_begin) {
		resource->cluster_offset -= offset;
		return 1;
	}
	offset -= until_the_begin;
	how_many_clusters_back = (offset / orp->sizeof_cluster_data);
	if ((offset % orp->sizeof_cluster_data) != 0) {
		how_many_clusters_back++;
	}
	if ((offset % orp->sizeof_cluster_data) == 0) {
		resource->cluster_offset = orp->sizeof_cluster_control;
	} else {
		resource->cluster_offset = orp->sizeof_cluster - (offset % orp->sizeof_cluster_data);
	}
	for (i = 0; i < how_many_clusters_back; i++) {
		resource->current_cluster = _orp_prev_cluster_by_cluster(orp, resource->current_cluster);
	}
	return 1;
}

void _orp_format_resource_descriptor(orp_t *orp, orp_resource_descriptor_t resource_descriptor) {
	orp_memory_address_t address;
	uint8_t zeros[4] = {0};
	address = _orp_resource_descriptor_to_address(orp, resource_descriptor);
	_orp_io_write_bulk(orp->driver, address, zeros, orp->sizeof_resource_descriptor);
}

uint8_t _orp_is_driver_mounted(orp_driver_t driver) {
	return orp_global_flags.driver_mounted & (1 << driver);
}

void _orp_set_driver_mounted(orp_driver_t driver, uint8_t mounted) {
	if (mounted) {
		orp_global_flags.driver_mounted |= (1 << driver);
	} else {
		orp_global_flags.driver_mounted &= ~(1 << driver);
	}
}

void _orp_free_resource_descriptors(orp_t *orp) {
	uint8_t i;
	for (i = 0; i < orp->resource_descriptor_count; i++) {
		_orp_free_resource_descriptor(orp, i);
	}
}

void _orp_free_resource_descriptor(orp_t *orp, orp_resource_descriptor_t resource_descriptor) {
	orp_memory_address_t address;
	uint8_t flags;
	address = _orp_resource_descriptor_to_address(orp, resource_descriptor);
	flags = _orp_io_read(orp->driver, ORP_RD_ADDRESS_TO_FLAG(address));
	flags &= ~(OZERO_ORP_RESOURCE_FLAG_BIT_OPENED | OZERO_ORP_RESOURCE_FLAG_BIT_READ_ONLY);
	_orp_io_write(orp->driver, ORP_RD_ADDRESS_TO_FLAG(address), flags);
}

void _orp_format_resource_clusters(orp_t *orp, orp_resource_t *resource) {
	uint8_t freed_clusters;
	freed_clusters = _orp_format_cluster_chain(orp, resource->first_cluster);
	_orp_increase_free_clusters(orp, freed_clusters);
}

uint8_t _orp_format_cluster_chain(orp_t *orp, orp_cluster_t cluster) {
	orp_cluster_t next_cluster;
	uint8_t formatted_clusters = 0;
	do {
		next_cluster = _orp_next_cluster_by_cluster(orp, cluster);
		_orp_format_cluster(orp, cluster);
		formatted_clusters++;
		if (next_cluster == ORP_INEXISTENT_CLUSTER || next_cluster == cluster) {
			break;
		}
		cluster = next_cluster;
	} while (1);
	return formatted_clusters;
}

uint8_t _orp_has_invalid_attributes(orp_t *orp) {
	if (orp->sizeof_resource_descriptor_table != (orp->sizeof_resource_descriptor * orp->resource_descriptor_count)) {
		// TODO: Use macros or constants here.
		return 1;
	}
	if (orp->sizeof_cluster_table != (orp->sizeof_cluster * orp->cluster_count)) {
		return 2;
	}
	if (orp->sizeof_cluster != (orp->sizeof_cluster_control + orp->sizeof_cluster_data)) {
		return 3;
	}
	if (orp->memory_size != orp->sizeof_cluster_table + orp->cluster_table_address) {
		return 4;
	}
	return 0;
}