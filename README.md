# OzeroFS - Resource Based File System

A simplified file system for small micro-controlled devices (PIC, Arduino, etc.) backed by EEPROM storage.

Instead of a traditional hierarchical file system, OzeroFS uses a **resource pool** model: a flat collection of resources that can be allocated, written to, read from, and released at any time. Resources grow dynamically as data is written.

[Documentation.pdf](Documentation.pdf)

## Features

- Flat resource pool (no directory hierarchy)
- Dynamic resource growth via cluster chaining
- Multiple storage drivers (internal EEPROM, external EEPROM, virtual/file-based)
- Read-only mount and open modes
- Configurable partition sizes (4K, 8K, 32K)
- Small memory footprint suitable for 8-bit MCUs

## API

```c
// Partition and lifecycle
void      orp_make_partition(orp_t *fs, orp_disk_size_t size, orp_environment_t env, orp_driver_t driver);
orp_op_result_t orp_format(orp_t *fs);
orp_op_result_t orp_mount(orp_driver_t driver, orp_t *fs, orp_mount_options_t options);
orp_op_result_t orp_umount(orp_t *fs);

// Resource management
orp_resource_code_t orp_alloc(orp_t *fs);
orp_op_result_t orp_open(orp_t *fs, orp_resource_code_t code, orp_resource_t *res, orp_open_resource_options_t options);
orp_op_result_t orp_close(orp_t *fs, orp_resource_t *res);
uint8_t             orp_release(orp_t *fs, orp_resource_t *res);

// Read/Write
uint8_t             orp_read(orp_t *fs, orp_resource_t *res);
orp_op_result_t orp_write(orp_t *fs, orp_resource_t *res, uint8_t data);

// Navigation
orp_op_result_t orp_seek(orp_t *fs, orp_resource_t *res, orp_seek_origin_t origin, orp_seek_int_t offset);
orp_op_result_t orp_rewind(orp_t *fs, orp_resource_t *res);
orp_op_result_t orp_truncate(orp_t *fs, orp_resource_t *res);
void                orp_sync(orp_t *fs, orp_resource_t *res);

// Status
orp_resource_size_t orp_size(orp_resource_t *res);
orp_resource_size_t orp_tell(orp_resource_t *res);
uint8_t                 orp_eor(orp_resource_t *res);
uint8_t                 orp_error(orp_resource_t *res);
orp_resource_size_t orp_available_space(orp_t *fs);
orp_resource_size_t orp_total_space(orp_t *fs);
```

## Usage Example

```c
#include <orp.h>
#include <orp_make_partition.h>

orp_t fs;
orp_resource_t resource;

// Initialize and format
orp_make_partition(&fs, OZERO_ORP_DISK_32K, OZERO_ORP_ENV_ARDUINO, OZERO_ORP_DRIVER_EXTERNAL_EEPROM);
orp_format(&fs);
orp_mount(OZERO_ORP_DRIVER_EXTERNAL_EEPROM, &fs, OZERO_ORP_MOUNT_OPTION_NORMAL);

// Allocate and open a resource
orp_resource_code_t code = orp_alloc(&fs);
orp_open(&fs, code, &resource, OZERO_ORP_OPEN_RESOURCE_OPTION_NORMAL);

// Write data
orp_write(&fs, &resource, 0x41);
orp_write(&fs, &resource, 0x42);

// Rewind and read back
orp_rewind(&fs, &resource);
uint8_t a = orp_read(&fs, &resource);  // 0x41
uint8_t b = orp_read(&fs, &resource);  // 0x42

// Cleanup
orp_close(&fs, &resource);
orp_umount(&fs);
```

## Memory Layout

The partition is divided into three regions written sequentially on disk:

```
|--- FS Header ---|--- Resource Descriptor Table ---|--- Cluster Table ---|
     (32 bytes)          (RD count * 4 bytes)         (cluster_count * sizeof_cluster)
```

### FS Header (orp_t)

The first 32 bytes store partition metadata (sizes, counts, addresses, flags). This is serialized directly from the `orp_t` struct.

### Resource Descriptor Table

Each entry is 4 bytes:

```
|  size_low  |  size_high  |  first_cluster  |  flags  |
     1B            1B             1B              1B

Flags:
  bit 0: Opened
  bit 1: Read-only
  bit 2: (unused)
  bit 3: (unused)
  bit 4: Allocated
  bit 5-7: (unused)
```

### Cluster

Each cluster is a doubly-linked node containing data:

```
|  next  |  prev  |  data ...  |
   1B       1B      (sizeof_cluster - 2) bytes
```

A free cluster has `next == prev == cluster_index` (self-referencing).

## Partition Configurations

| Parameter           | 4K (default) | 8K          | 32K          |
|---------------------|-------------|-------------|--------------|
| Memory size         | 3910        | 8192        | 32660        |
| RD table address    | 0x0020      | 0x0020      | 0x0020       |
| Cluster table addr  | 0x00A0      | 0x00A0      | 0x00A0       |
| RD count            | 32          | 32          | 32           |
| Cluster count       | 250         | 251         | 250          |
| Cluster size        | 15          | 32          | 130          |
| Cluster data size   | 13          | 30          | 128          |
| Total data capacity | 3250        | 7530        | 32000        |

## Supported Drivers

| Driver                            | Enum value |
|-----------------------------------|-----------|
| `OZERO_ORP_DRIVER_VIRTUAL`         | 0         |
| `OZERO_ORP_DRIVER_SELF_EEPROM`     | 1         |
| `OZERO_ORP_DRIVER_MULTI_EXTERNAL_EEPROM` | 2   |
| `OZERO_ORP_DRIVER_EXTERNAL_EEPROM` | 3         |
| `OZERO_ORP_DRIVER_ARDUINO_EEPROM`  | 4         |

The I/O layer (`orp_io.h`) declares `_orp_io_read` and `_orp_io_write` as `extern` functions. Each driver target must provide its own implementation.

## Building the Tests

```bash
cd spec
make
./sorp
```

The test suite uses the virtual driver backed by a file (`img.hd`).

## Project Structure

```
orp.h / orp.c                  Core file system API
orp_io.h                           I/O abstraction (extern read/write)
orp_util.h / orp_util.c        Internal helpers (cluster management, seek, etc.)
orp_make_partition.h / .c          Partition configuration presets
spec/                                  Test suite (virtual driver over file I/O)
```

## License

See [LICENSE](LICENSE).
