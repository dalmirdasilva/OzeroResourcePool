/**
 * ORP - Ozero Resource Pool
 *
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#ifndef OZERO_ORP_H
#define OZERO_ORP_H 1

#include <stdint.h>

#define ORP_NULL_RESOURCE_CODE 0xff
#define ORP_NULL_CLUSTER 0xff

#define ORP_NULL_RESOURCE_DESCRIPTOR_ADDRESS 0xff
#define ORP_NULL_CLUSTER_ADDRESS 0x00

#define ORP_FIRST_ADDRESS_OF_MEMORY 0x00

#define ORP_SIZEOF_RESOURCE_SIZE 0x02

#define ORP_INEXISTENT_CLUSTER 0xff

#define ORP_CLUSTER_ADDRESS_TO_NEXT(CLUSTER_ADDRESS) ((CLUSTER_ADDRESS) + 0)
#define ORP_CLUSTER_ADDRESS_TO_PREV(CLUSTER_ADDRESS) ((CLUSTER_ADDRESS) + 1)
#define ORP_CLUSTER_ADDRESS_TO_DATA(CLUSTER_ADDRESS) ((CLUSTER_ADDRESS) + 2)

#define ORP_RD_ADDRESS_TO_SIZE_LOW(RD_ADDRESS) ((RD_ADDRESS) + 0)
#define ORP_RD_ADDRESS_TO_SIZE_HIGH(RD_ADDRESS) ((RD_ADDRESS) + 1)
#define ORP_RD_ADDRESS_TO_FIRST_CLUSTER(RD_ADDRESS) ((RD_ADDRESS) + 2)
#define ORP_RD_ADDRESS_TO_FLAG(RD_ADDRESS) ((RD_ADDRESS) + 3)

typedef uint8_t orp_resource_descriptor_t;
typedef uint8_t orp_cluster_t;
typedef uint16_t orp_resource_size_t;
typedef uint16_t orp_memory_address_t;
typedef uint8_t orp_resource_code_t;
typedef uint16_t orp_seek_int_t;

// Drivers

typedef enum {
	OZERO_ORP_DRIVER_VIRTUAL = 0,
	OZERO_ORP_DRIVER_SELF_EEPROM = 1,
	OZERO_ORP_DRIVER_MULTI_EXTERNAL_EEPROM = 2,
	OZERO_ORP_DRIVER_EXTERNAL_EEPROM = 3,
	OZERO_ORP_DRIVER_ARDUINO_EEPROM = 4
} orp_driver_t;

// Resource flag bit values

typedef enum {
	OZERO_ORP_RESOURCE_FLAG_BIT_OPENED = 1,
	OZERO_ORP_RESOURCE_FLAG_BIT_READ_ONLY = 2,
	OZERO_ORP_RESOURCE_FLAG_BIT_ERROR_ON_LAST_READ = 4,
	OZERO_ORP_RESOURCE_FLAG_BIT_ERROR_ON_LAST_WRITE = 8,
	OZERO_ORP_RESOURCE_FLAG_BIT_ALLOCATED = 16,
	OZERO_ORP_RESOURCE_FLAG_BIT_EOR_REACHED = 32
} orp_resource_flag_bits_t;

// Options to open a resource

typedef enum {
	OZERO_ORP_OPEN_RESOURCE_OPTION_NORMAL = 0,
	OZERO_ORP_OPEN_RESOURCE_OPTION_READ_ONLY = 1
} orp_open_resource_options_t;

// Options to mount a resource

typedef enum {
	OZERO_ORP_MOUNT_OPTION_NORMAL = 0,
	OZERO_ORP_MOUNT_OPTION_READ_ONLY = 1
} orp_mount_options_t;

// FS flag bit values

typedef enum {
	OZERO_ORP_FLAG_BIT_DRIVER_MOUNTED = 1,
	OZERO_ORP_FLAG_BIT_READ_ONLY = 2
} orp_flag_bits_t;

// Operation result

typedef enum {
	OZERO_ORP_OP_RESULT_SUCCESS = 0,
	OZERO_ORP_OP_RESULT_ERROR_RESOURCE_OPENED = 1,
	OZERO_ORP_OP_RESULT_ERROR_RESOURCE_CLOSED = 2,
	OZERO_ORP_OP_RESULT_ERROR_RESOURCE_READ_ONLY = 3,
	OZERO_ORP_OP_RESULT_ERROR_NO_SPACE_AVAILABLE = 4,
	OZERO_ORP_OP_RESULT_ERROR_DRIVER_BUSY = 5,
	OZERO_ORP_OP_RESULT_ERROR_SEEK_OUT_OF_BOUND = 6,
	OZERO_ORP_OP_RESULT_ERROR_RESOURCE_DOES_NOT_ALLOCATED = 7,
	OZERO_ORP_OP_RESULT_ERROR_DRIVER_NOT_MOUNTED = 8
} orp_op_result_t;

// Seek position reference

typedef enum {
	OZERO_ORP_SEEK_ORIGIN_BEGIN = 0,
	OZERO_ORP_SEEK_ORIGIN_CURRENT = 1
} orp_seek_origin_t;

typedef struct {
	uint8_t flags;
} orp_stat_t;

// Resource system

typedef struct {
	orp_driver_t driver;
	uint16_t memory_size;
	orp_memory_address_t resource_descriptor_table_address;
	orp_memory_address_t cluster_table_address;
	uint16_t sizeof_resource_descriptor_table;
	uint16_t sizeof_cluster_table;
	uint8_t sizeof_resource_descriptor;
	uint8_t sizeof_cluster;
	uint8_t resource_descriptor_count;
	uint8_t cluster_count;
	uint8_t sizeof_cluster_data;
	uint8_t sizeof_cluster_control;
	uint8_t free_clusters;
	uint8_t flags;
} orp_t;

// Resource

typedef struct {
	orp_resource_descriptor_t resource_descriptor;
	orp_cluster_t first_cluster;
	orp_cluster_t current_cluster;
	uint8_t cluster_offset;
	uint16_t size;
	uint16_t current_position;
	uint8_t flags;
} orp_resource_t;

typedef struct {
	uint8_t driver_mounted;
} orp_global_flags_t;

extern orp_global_flags_t orp_global_flags;

/**
 * Format the partition by initializing all resource descriptors and clusters
 * to their default (empty) state. This erases all existing data on the device.
 * The orp structure must be configured (via orp_make_partition) before calling.
 *
 * @param orp       Pointer to the partition structure.
 * @return          OZERO_ORP_OP_RESULT_SUCCESS on success.
 */
orp_op_result_t orp_format(orp_t *orp);

/**
 * Mount a partition on the given driver, making it available for resource
 * operations. Reads the partition metadata from disk and marks the driver
 * as mounted. A driver can only be mounted once at a time.
 *
 * @param driver    The storage driver to mount (e.g. OZERO_ORP_DRIVER_ARDUINO_EEPROM).
 * @param orp       Pointer to the partition structure to populate.
 * @param options   Mount options (OZERO_ORP_MOUNT_OPTION_NORMAL or
 *                  OZERO_ORP_MOUNT_OPTION_READ_ONLY).
 * @return          OZERO_ORP_OP_RESULT_SUCCESS on success,
 *                  OZERO_ORP_OP_RESULT_ERROR_DRIVER_BUSY if already mounted.
 */
orp_op_result_t orp_mount(orp_driver_t driver, orp_t *orp, orp_mount_options_t options);

/**
 * Unmount a previously mounted partition, releasing the driver so it can be
 * mounted again. Does nothing if the driver is not currently mounted.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @return          OZERO_ORP_OP_RESULT_SUCCESS on success.
 */
orp_op_result_t orp_umount(orp_t *orp);

/**
 * Open an existing allocated resource for reading and/or writing. The resource
 * read/write pointer is positioned at the beginning. The resource must have been
 * previously allocated with orp_alloc and must not already be open.
 *
 * @param orp           Pointer to the mounted partition structure.
 * @param resource_code Resource code returned by orp_alloc.
 * @param resource      Pointer to a resource structure to populate with the
 *                      opened resource state.
 * @param options       Open options (OZERO_ORP_OPEN_RESOURCE_OPTION_NORMAL or
 *                      OZERO_ORP_OPEN_RESOURCE_OPTION_READ_ONLY).
 * @return              OZERO_ORP_OP_RESULT_SUCCESS on success,
 *                      OZERO_ORP_OP_RESULT_ERROR_DRIVER_NOT_MOUNTED if not mounted,
 *                      OZERO_ORP_OP_RESULT_ERROR_RESOURCE_DOES_NOT_ALLOCATED if not allocated,
 *                      OZERO_ORP_OP_RESULT_ERROR_RESOURCE_OPENED if already open.
 */
orp_op_result_t orp_open(orp_t *orp, orp_resource_code_t resource_code, orp_resource_t *resource, orp_open_resource_options_t options);

/**
 * Close an open resource, flushing any pending size updates to disk and
 * releasing the resource descriptor so it can be opened again.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the open resource to close.
 * @return          OZERO_ORP_OP_RESULT_SUCCESS on success.
 */
orp_op_result_t orp_close(orp_t *orp, orp_resource_t *resource);

/**
 * Read a single byte from the resource at the current read/write position and
 * advance the position by one. If the end-of-resource has been reached or the
 * resource is not open, returns 0 and sets the appropriate error flag.
 * Automatically follows the cluster chain when crossing cluster boundaries.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the open resource.
 * @return          The byte read, or 0 on error/end-of-resource.
 */
uint8_t orp_read(orp_t *orp, orp_resource_t *resource);

/**
 * Write a single byte to the resource at the current read/write position and
 * advance the position by one. If writing beyond the current resource size,
 * the size is extended and synced to disk. New clusters are allocated
 * automatically when the current cluster is full.
 *
 * @param orp           Pointer to the mounted partition structure.
 * @param resource      Pointer to the open resource.
 * @param data_to_write The byte to write.
 * @return              OZERO_ORP_OP_RESULT_SUCCESS on success,
 *                      OZERO_ORP_OP_RESULT_ERROR_RESOURCE_CLOSED if not open,
 *                      OZERO_ORP_OP_RESULT_ERROR_RESOURCE_READ_ONLY if read-only,
 *                      OZERO_ORP_OP_RESULT_ERROR_NO_SPACE_AVAILABLE if no free clusters.
 */
orp_op_result_t orp_write(orp_t *orp, orp_resource_t *resource, uint8_t data_to_write);

/**
 * Move the read/write pointer to a new position within the resource.
 * The final position is clamped modulo (size + 1), so seeking beyond the
 * end wraps around. Optimizes movement direction based on shortest path
 * (forward from current or rewind-then-forward).
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the open resource.
 * @param origin    Reference point: OZERO_ORP_SEEK_ORIGIN_BEGIN (absolute) or
 *                  OZERO_ORP_SEEK_ORIGIN_CURRENT (relative to current position).
 * @param offset    Offset from the origin.
 * @return          OZERO_ORP_OP_RESULT_SUCCESS on success.
 */
orp_op_result_t orp_seek(orp_t *orp, orp_resource_t *resource, orp_seek_origin_t origin, orp_seek_int_t offset);

/**
 * Truncate a resource, discarding all its data. Frees all clusters in the
 * chain except the first one and resets the resource size to zero.
 * The resource must be allocated.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the resource to truncate.
 * @return          OZERO_ORP_OP_RESULT_SUCCESS on success,
 *                  OZERO_ORP_OP_RESULT_ERROR_RESOURCE_DOES_NOT_ALLOCATED if not allocated.
 */
orp_op_result_t orp_truncate(orp_t *orp, orp_resource_t *resource);

/**
 * Flush the in-memory resource size to the underlying storage, ensuring the
 * resource descriptor on disk reflects the current size.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the open resource.
 */
void orp_sync(orp_t *orp, orp_resource_t *resource);

/**
 * Read the status flags of a resource descriptor from disk.
 * Note: Currently returns 0xff (not yet fully implemented).
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the resource.
 * @param stat      Pointer to a stat structure to populate.
 */
void orp_stat(orp_t *orp, orp_resource_t *resource, orp_stat_t *stat);

/**
 * Reset the read/write position of a resource back to the beginning
 * (position 0, first cluster, first data offset).
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the open resource.
 * @return          OZERO_ORP_OP_RESULT_SUCCESS on success.
 */
orp_op_result_t orp_rewind(orp_t *orp, orp_resource_t *resource);

/**
 * Allocate a new resource in the partition. Finds the first free resource
 * descriptor, allocates an initial cluster for it, and marks it as allocated.
 * The resource must be opened with orp_open before it can be used.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @return          The resource code for the newly allocated resource, or
 *                  ORP_NULL_RESOURCE_CODE (0xff) if no descriptors or clusters
 *                  are available.
 */
orp_resource_code_t orp_alloc(orp_t *orp);

/**
 * Release a previously allocated resource, freeing all its clusters and
 * resetting its resource descriptor. The resource becomes available for
 * future allocation via orp_alloc.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @param resource  Pointer to the resource to release.
 * @return          1 on success or if the resource was not allocated.
 */
uint8_t orp_release(orp_t *orp, orp_resource_t *resource);

/**
 * Get the current size (in bytes) of a resource.
 *
 * @param resource  Pointer to the resource.
 * @return          The number of bytes stored in the resource.
 */
orp_resource_size_t orp_size(orp_resource_t *resource);

/**
 * Get the current read/write position within a resource.
 *
 * @param resource  Pointer to the resource.
 * @return          The current byte offset from the beginning.
 */
orp_resource_size_t orp_tell(orp_resource_t *resource);

/**
 * Test whether the end-of-resource has been reached, i.e. the current
 * position equals the resource size.
 *
 * @param resource  Pointer to the resource.
 * @return          Non-zero if end-of-resource, 0 otherwise.
 */
uint8_t orp_eor(orp_resource_t *resource);

/**
 * Test whether an error occurred during the last read or write operation
 * on the resource.
 *
 * @param resource  Pointer to the resource.
 * @return          Non-zero if an error flag is set, 0 otherwise.
 */
uint8_t orp_error(orp_resource_t *resource);

/**
 * Get the available (free) space in the partition, calculated as
 * free_clusters * sizeof_cluster_data.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @return          Available space in bytes.
 */
orp_resource_size_t orp_available_space(orp_t *orp);

/**
 * Get the total data capacity of the partition, calculated as
 * cluster_count * sizeof_cluster_data.
 *
 * @param orp       Pointer to the mounted partition structure.
 * @return          Total capacity in bytes.
 */
orp_resource_size_t orp_total_space(orp_t *orp);

#endif// OZERO_ORP_H
