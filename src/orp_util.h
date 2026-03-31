/**
 * ORP - Ozero Resource Pool
 *
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#ifndef OZERO_ORP_UTIL_H
#define OZERO_ORP_UTIL_H 1

#include "orp.h"

/**
 * Write a resource system table to disk
 *
 * @param driver
 * @param orp
 */
void _orp_write_rp_to_disk(orp_driver_t driver, orp_t *orp);

/**
 * Read a resource system table from disk
 *
 * @param driver
 * @param orp
 */
void _orp_read_orp_from_disk(orp_driver_t driver, orp_t *orp);

/**
 * Allocate a free cluster from disk if any
 *
 * @param orp
 * @return
 */
orp_memory_address_t _orp_alloc_cluster(orp_t *orp);

/**
 * Test if the given cluster is free
 *
 * @param orp
 * @param cluster
 * @return
 */
uint8_t _orp_is_free_cluster(orp_t *orp, orp_cluster_t cluster);

/**
 * Format a given cluster
 *
 * @param orp
 * @param cluster
 */
void _orp_format_cluster(orp_t *orp, orp_cluster_t cluster);

/**
 * Free a given cluster
 *
 * @param orp
 * @param cluster
 */
void _orp_free_cluster(orp_t *orp, orp_cluster_t cluster);

/**
 * Create a chain between two clusters
 *
 * @param orp
 * @param prev_cluster
 * @param next_cluster
 */
void _orp_create_cluster_chain(orp_t *orp, orp_cluster_t prev_cluster, orp_cluster_t next_cluster);

/**
 * Convert resource code to rd
 *
 * @param resource
 */
#define _orp_resource_code_to_resource_descriptor(resource_code) (orp_resource_descriptor_t)(resource_code)

/**
 * Convert rd to resource code
 *
 * @param resource
 */
#define _orp_resource_descriptor_to_resource_code(resource_descriptor) (orp_resource_code_t)(resource_descriptor)

/**
 * Convert cluster to address
 *
 * @param resource
 */
#define _orp_cluster_to_address(orp, cluster) (orp_memory_address_t)((orp)->cluster_table_address + ((cluster) * (orp)->sizeof_cluster))

/**
 * Convert address to cluster
 *
 * @param resource
 */
#define _orp_address_to_cluster(orp, address) (orp_cluster_t)(((address) - (orp)->cluster_table_address) / (orp)->sizeof_cluster)

/**
 * Convert rd to address
 *
 * @param resource
 */
#define _orp_resource_descriptor_to_address(orp, resource_descriptor) (orp_memory_address_t)(((resource_descriptor) * (orp)->sizeof_resource_descriptor) + (orp)->resource_descriptor_table_address)

/**
 * Convert address to rd
 *
 * @param resource
 */
#define _orp_address_to_resource_descriptor(orp, address) (orp_resource_descriptor_t)(((address) - (orp)->resource_descriptor_table_address) / (orp)->sizeof_resource_descriptor)

/**
 * Check if the end-of-resource is reached and set or clear the respective flag
 *
 * @param resource
 */
void _orp_check_for_eor_reached(orp_resource_t *resource);

/**
 * Test the end-of-resource flag
 *
 * @param resource
 * @return
 */
uint8_t _orp_is_eor_reached(orp_resource_t *resource);

/**
 * Check if we are at the end of resource, if yes alloc another cluster and
 * manage the new pointers
 *
 * @param orp
 * @param resource
 * @return
 */
uint8_t _orp_check_for_availability(orp_t *orp, orp_resource_t *resource);

/**
 * Move the current position ahead 'offset' bytes
 *
 * @param orp
 * @param resource
 * @param offset
 * @return
 */
uint8_t _orp_move_current_position_ahead(orp_t *orp, orp_resource_t *resource, orp_seek_int_t offset);

/**
 * Move the current position back 'offset' bytes
 *
 * @param orp
 * @param resource
 * @param offset
 * @return
 */
uint8_t _orp_move_current_position_back(orp_t *orp, orp_resource_t *resource, orp_seek_int_t offset);

/**
 * Free a resource description
 *
 * @param orp
 * @param resource_descriptor
 */
void _orp_format_resource_descriptor(orp_t *orp, orp_resource_descriptor_t resource_descriptor);

/**
 * Test if given driver is mounted
 *
 * @param driver
 * @return
 */
uint8_t _orp_is_driver_mounted(orp_driver_t driver);

/**
 * Set/clear given driver as mounted
 *
 * @param driver
 * @param is
 */
void _orp_set_driver_mounted(orp_driver_t driver, uint8_t mounted);

/**
 * Close all resources
 *
 * @param orp
 */
void _orp_free_resource_descriptors(orp_t *orp);

/**
 * Close a single resources
 *
 * @param orp
 * @param resource_descriptor
 */
void _orp_free_resource_descriptor(orp_t *orp, orp_resource_descriptor_t resource_descriptor);

/**
 * Decrease free cluster
 *
 * @param orp
 * @param resource
 */
#define _orp_decrease_free_clusters(orp, n)      \
	{                                            \
		orp->free_clusters -= n;                 \
		_orp_write_rp_to_disk(orp->driver, orp); \
	}

/**
 * Increase free cluster
 *
 * @param orp
 * @param resource
 */
#define _orp_increase_free_clusters(orp, n)      \
	{                                            \
		orp->free_clusters += n;                 \
		_orp_write_rp_to_disk(orp->driver, orp); \
	}

/**
 * Free resource cluster
 *
 * @param orp
 * @param resource
 */
void _orp_format_resource_clusters(orp_t *orp, orp_resource_t *resource);

/**
 * Format a chain of clusters
 *
 * @param orp
 * @param cluster
 * @return
 */
uint8_t _orp_format_cluster_chain(orp_t *orp, orp_cluster_t cluster);

/**
 * Get the previous cluster by a cluster
 *
 * @param orp
 * @return
 */
#define _orp_prev_cluster_by_cluster(orp, cluster) _orp_prev_cluster_by_cluster_address(orp, _orp_cluster_to_address(orp, cluster))

/**
 * Get the next cluster by a cluster
 *
 * @param orp
 * @return
 */
#define _orp_next_cluster_by_cluster(orp, cluster) _orp_next_cluster_by_cluster_address(orp, _orp_cluster_to_address(orp, cluster))

/**
 * Get the previous cluster by a cluster address
 *
 * @param orp
 * @return
 */
#define _orp_prev_cluster_by_cluster_address(orp, address) (orp_cluster_t)(_orp_io_read(orp->driver, ORP_CLUSTER_ADDRESS_TO_PREV(address)))

/**
 * Get the next cluster by a cluster address
 *
 * @param orp
 * @return
 */
#define _orp_next_cluster_by_cluster_address(orp, address) (orp_cluster_t)(_orp_io_read(orp->driver, ORP_CLUSTER_ADDRESS_TO_NEXT(address)))

/**
 * Calculates and evaluate the orp attributes
 *
 * @param orp
 * @return
 */
uint8_t _orp_has_invalid_attributes(orp_t *orp);

#endif// OZERO_ORP_UTIL_H
