/**
 * ORP - Ozero Resource Pool
 *
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#ifndef OZERO_ORP_IO_H
#define OZERO_ORP_IO_H 1

#include "orp.h"

extern uint8_t _orp_io_read(orp_driver_t driver, orp_memory_address_t address);

extern void _orp_io_write(orp_driver_t driver, orp_memory_address_t address, uint8_t data);

extern void _orp_io_read_bulk(orp_driver_t driver, orp_memory_address_t address, uint8_t *buf, uint16_t len);

extern void _orp_io_write_bulk(orp_driver_t driver, orp_memory_address_t address, const uint8_t *buf, uint16_t len);

#endif// OZERO_ORP_IO_H
