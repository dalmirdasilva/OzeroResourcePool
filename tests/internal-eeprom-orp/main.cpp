#include <Arduino.h>

extern "C" {
#include <orp.h>
#include <orp_io.h>
#include <orp_make_partition.h>
}

#define TEST_DRIVER OZERO_ORP_DRIVER_ARDUINO_EEPROM
#define EEPROM_SIZE 8192

static uint8_t memory[EEPROM_SIZE];
static orp_t orp;
static orp_resource_t resource;

uint8_t _orp_io_read(orp_driver_t driver, orp_memory_address_t address) {
	return memory[address];
}

void _orp_io_write(orp_driver_t driver, orp_memory_address_t address, uint8_t data) {
	memory[address] = data;
}

void _orp_io_read_bulk(orp_driver_t driver, orp_memory_address_t address, uint8_t *buf, uint16_t len) {
	for (uint16_t i = 0; i < len; i++) {
		buf[i] = memory[address + i];
	}
}

void _orp_io_write_bulk(orp_driver_t driver, orp_memory_address_t address, const uint8_t *buf, uint16_t len) {
	for (uint16_t i = 0; i < len; i++) {
		memory[address + i] = buf[i];
	}
}

static void pass(const char *name) {
	Serial.print("(*) ");
	Serial.print(name);
	Serial.println(" passed.");
}

static void fail(const char *name, int err) {
	Serial.print("(F) ");
	Serial.print(name);
	Serial.print(" failed. error: ");
	Serial.println(err);
}

static bool test_format() {
	orp_op_result_t r = orp_format(&orp);
	if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
		fail("format", r);
		return false;
	}
	pass("format");
	return true;
}

static bool test_mount() {
	orp_op_result_t r = orp_mount(TEST_DRIVER, &orp, OZERO_ORP_MOUNT_OPTION_NORMAL);
	if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
		fail("mount", r);
		return false;
	}
	pass("mount");
	return true;
}

static bool test_alloc_open_write_read() {
	orp_resource_code_t code = orp_alloc(&orp);
	if (code == ORP_NULL_RESOURCE_CODE) {
		fail("alloc", -1);
		return false;
	}
	pass("alloc");

	orp_op_result_t r = orp_open(&orp, code, &resource, OZERO_ORP_OPEN_RESOURCE_OPTION_NORMAL);
	if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
		fail("open", r);
		return false;
	}
	pass("open");

	// Write test pattern across cluster boundary (30 bytes of data per cluster in 8K)
	uint8_t pattern_len = 64;
	for (uint8_t i = 0; i < pattern_len; i++) {
		r = orp_write(&orp, &resource, i);
		if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
			fail("write", r);
			return false;
		}
	}
	pass("write 64 bytes");

	// Verify size
	if (orp_size(&resource) != pattern_len) {
		fail("size", orp_size(&resource));
		return false;
	}
	pass("size");

	// Rewind and read back
	orp_rewind(&orp, &resource);
	for (uint8_t i = 0; i < pattern_len; i++) {
		uint8_t val = orp_read(&orp, &resource);
		if (val != i) {
			Serial.print("(F) read mismatch at ");
			Serial.print(i);
			Serial.print(": expected ");
			Serial.print(i);
			Serial.print(", got ");
			Serial.println(val);
			return false;
		}
	}
	pass("read 64 bytes");

	// Verify EOR
	if (!orp_eor(&resource)) {
		fail("eor", 0);
		return false;
	}
	pass("eor");

	return true;
}

static bool test_seek() {
	orp_rewind(&orp, &resource);

	// Seek to middle
	orp_seek(&orp, &resource, OZERO_ORP_SEEK_ORIGIN_BEGIN, 32);
	if (orp_tell(&resource) != 32) {
		fail("seek_begin", orp_tell(&resource));
		return false;
	}
	uint8_t val = orp_read(&orp, &resource);
	if (val != 32) {
		fail("seek_read", val);
		return false;
	}
	pass("seek");

	return true;
}

static bool test_close_reopen() {
	orp_resource_code_t code = resource.resource_descriptor;
	orp_close(&orp, &resource);
	pass("close");

	orp_op_result_t r = orp_open(&orp, code, &resource, OZERO_ORP_OPEN_RESOURCE_OPTION_NORMAL);
	if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
		fail("reopen", r);
		return false;
	}

	// Verify data persisted
	uint8_t val = orp_read(&orp, &resource);
	if (val != 0) {
		fail("reopen_read", val);
		return false;
	}
	pass("reopen and read");

	orp_close(&orp, &resource);
	return true;
}

static bool test_large_resource() {
	orp_resource_code_t code = orp_alloc(&orp);
	if (code == ORP_NULL_RESOURCE_CODE) {
		fail("large_alloc", -1);
		return false;
	}

	orp_op_result_t r = orp_open(&orp, code, &resource, OZERO_ORP_OPEN_RESOURCE_OPTION_NORMAL);
	if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
		fail("large_open", r);
		return false;
	}

	// Fill across many clusters: 251 clusters * 30 bytes = 7530 max,
	// but the previous test consumed some clusters, so write until full.
	orp_resource_size_t avail = orp_available_space(&orp);
	Serial.print("  writing ");
	Serial.print(avail);
	Serial.println(" bytes...");

	for (orp_resource_size_t i = 0; i < avail; i++) {
		r = orp_write(&orp, &resource, (uint8_t)(i & 0xff));
		if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
			Serial.print("(F) large_write failed at byte ");
			Serial.println(i);
			return false;
		}
	}
	pass("large_write");

	orp_resource_size_t written = orp_size(&resource);
	if (written != avail) {
		fail("large_size", written);
		return false;
	}
	pass("large_size");

	// Rewind and verify every byte
	orp_rewind(&orp, &resource);
	for (orp_resource_size_t i = 0; i < written; i++) {
		uint8_t val = orp_read(&orp, &resource);
		uint8_t expected = (uint8_t)(i & 0xff);
		if (val != expected) {
			Serial.print("(F) large_read mismatch at ");
			Serial.print(i);
			Serial.print(": expected ");
			Serial.print(expected);
			Serial.print(", got ");
			Serial.println(val);
			return false;
		}
	}
	pass("large_read");

	if (!orp_eor(&resource)) {
		fail("large_eor", 0);
		return false;
	}
	pass("large_eor");

	// Seek to various positions across cluster boundaries and verify
	uint16_t seek_positions[] = {0, 29, 30, 31, 59, 60, 150, 300, (uint16_t)(written / 2), (uint16_t)(written - 1)};
	for (uint8_t s = 0; s < 10; s++) {
		uint16_t pos = seek_positions[s];
		if (pos >= written) continue;
		orp_seek(&orp, &resource, OZERO_ORP_SEEK_ORIGIN_BEGIN, pos);
		if (orp_tell(&resource) != pos) {
			Serial.print("(F) large_seek to ");
			Serial.print(pos);
			Serial.print(", tell=");
			Serial.println(orp_tell(&resource));
			return false;
		}
		uint8_t val = orp_read(&orp, &resource);
		uint8_t expected = (uint8_t)(pos & 0xff);
		if (val != expected) {
			Serial.print("(F) large_seek_read at ");
			Serial.print(pos);
			Serial.print(": expected ");
			Serial.print(expected);
			Serial.print(", got ");
			Serial.println(val);
			return false;
		}
	}
	pass("large_seek");

	// Close, reopen and verify first and last bytes persisted
	orp_close(&orp, &resource);
	r = orp_open(&orp, code, &resource, OZERO_ORP_OPEN_RESOURCE_OPTION_NORMAL);
	if (r != OZERO_ORP_OP_RESULT_SUCCESS) {
		fail("large_reopen", r);
		return false;
	}

	uint8_t first = orp_read(&orp, &resource);
	if (first != 0) {
		fail("large_reopen_first", first);
		return false;
	}

	orp_seek(&orp, &resource, OZERO_ORP_SEEK_ORIGIN_BEGIN, written - 1);
	uint8_t last = orp_read(&orp, &resource);
	uint8_t expected_last = (uint8_t)((written - 1) & 0xff);
	if (last != expected_last) {
		Serial.print("(F) large_reopen_last: expected ");
		Serial.print(expected_last);
		Serial.print(", got ");
		Serial.println(last);
		return false;
	}
	pass("large_reopen_verify");

	orp_close(&orp, &resource);

	Serial.print("  total bytes verified: ");
	Serial.println(written);
	return true;
}

static bool test_available_space() {
	orp_resource_size_t avail = orp_available_space(&orp);
	orp_resource_size_t total = orp_total_space(&orp);
	Serial.print("  available: ");
	Serial.print(avail);
	Serial.print(" / ");
	Serial.println(total);
	if (avail > total) {
		fail("available_space", avail);
		return false;
	}
	pass("available_space");
	return true;
}

void setup() {
	Serial.begin(115200);
	while (!Serial) {}

	memset(memory, 0xff, EEPROM_SIZE);

	Serial.println("=== ORP 8K In-Memory Test ===");

	orp_make_partition(&orp, OZERO_ORP_DISK_8K, OZERO_ORP_ENV_ARDUINO, TEST_DRIVER);

	if (!test_format()) return;
	if (!test_mount()) return;
	if (!test_alloc_open_write_read()) return;
	if (!test_seek()) return;
	if (!test_close_reopen()) return;
	if (!test_large_resource()) return;
	if (!test_available_space()) return;

	orp_umount(&orp);

	Serial.println("=== All tests passed ===");
}

void loop() {
}
