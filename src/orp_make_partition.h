/**
 * ORP - Ozero Resource Pool
 *
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#ifndef OZERO_ORP_MAKE_PARTITION_H
#define OZERO_ORP_MAKE_PARTITION_H 1

#include "orp.h"

typedef enum {
	OZERO_ORP_DISK_4K,
	OZERO_ORP_DISK_8K,
	OZERO_ORP_DISK_32K
} orp_disk_size_t;

typedef enum {
	OZERO_ORP_ENV_ARDUINO,
	OZERO_ORP_ENV_VIRTUAL
} orp_environment_t;

void orp_make_partition(orp_t *orp, orp_disk_size_t size, orp_environment_t env, orp_driver_t driver);

#endif// OZERO_ORP_MAKE_PARTITION_H
