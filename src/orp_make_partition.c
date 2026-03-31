/**
 * ORP - Ozero Resource Pool
 *
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#include "orp_make_partition.h"

void orp_make_partition(orp_t *orp, orp_disk_size_t size, orp_environment_t env, orp_driver_t driver) {

	orp->driver = driver;
	switch (size) {

		case OZERO_ORP_DISK_32K:
			orp->memory_size = 0x7f94;						// 32660;
			orp->resource_descriptor_table_address = 0x0020;// 32;
			orp->cluster_table_address = 0x00a0;			// 160;
			orp->sizeof_resource_descriptor_table = 0x0080; // 128;
			orp->sizeof_cluster_table = 0x7ef4;				// 32500;
			orp->sizeof_resource_descriptor = 0x04;			// 4;
			orp->sizeof_cluster = 0x82;						// 130;
			orp->resource_descriptor_count = 0x20;			// 32;
			orp->cluster_count = 0xfa;						// 250;
			orp->sizeof_cluster_data = 0x80;				// 128;
			orp->sizeof_cluster_control = 0x02;				// 2;
			orp->free_clusters = 0xfa;						// 250;
			break;

		case OZERO_ORP_DISK_8K:
			orp->memory_size = 0x2000;						// 8192;
			orp->resource_descriptor_table_address = 0x0020;// 32;
			orp->cluster_table_address = 0x00a0;			// 160;
			orp->sizeof_resource_descriptor_table = 0x0080; // 128;
			orp->sizeof_cluster_table = 0x1f60;				// 8032;
			orp->sizeof_resource_descriptor = 0x04;			// 4;
			orp->sizeof_cluster = 0x20;						// 32;
			orp->resource_descriptor_count = 0x20;			// 32;
			orp->cluster_count = 0xfb;						// 251;
			orp->sizeof_cluster_data = 0x1e;				// 30;
			orp->sizeof_cluster_control = 0x02;				// 2;
			orp->free_clusters = 0xfb;						// 251;
			break;

		default:
			orp->memory_size = 0xf46;						// 3910;
			orp->resource_descriptor_table_address = 0x0020;// 32;
			orp->cluster_table_address = 0x00a0;			// 160;
			orp->sizeof_resource_descriptor_table = 0x0080; // 128;
			orp->sizeof_cluster_table = 0xea6;				// 3750;
			orp->sizeof_resource_descriptor = 0x04;			// 4;
			orp->sizeof_cluster = 0x0f;						// 15;
			orp->resource_descriptor_count = 0x20;			// 32;
			orp->cluster_count = 0xfa;						// 250;
			orp->sizeof_cluster_data = 0x0d;				// 13;
			orp->sizeof_cluster_control = 0x02;				// 2;
			orp->free_clusters = 0xfa;						// 250;
			break;
	}
	orp->flags = 0x00;// 0;
}