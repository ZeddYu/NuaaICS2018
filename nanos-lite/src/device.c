#include "common.h"

#define NAME(key) \
	[_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
	[_KEY_NONE] = "NONE",
	_KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
	size_t retsize;
	int key = _read_key();
	char buffer[128];
	bool down = false;
	if (key & 0x8000) {
		key ^= 0x8000;
		down = true;
	}

	if (key != _KEY_NONE) {
		sprintf(buffer, "k%s %s\n", down ? "d" : "u", keyname[key]);
		retsize = strlen(buffer) > len ? len : strlen(buffer);
		memcpy(buf, buffer, retsize);
		return retsize;
	}
	else {
		sprintf(buffer, "t %d\n", _uptime());
		retsize = strlen(buffer) > len ? len : strlen(buffer);
		memcpy(buf, buffer, retsize);
		return retsize;
	}
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
	memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
	offset = offset / 4;
	_draw_rect(buf, offset % _screen.width, offset / _screen.width, len/4, 1);
}

void init_device() {
	_ioe_init();
	sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d", _screen.width, _screen.height);
	Log("dispinfo:\n%s", dispinfo);
	// TODO: print the string to array `dispinfo` with the format
	// described in the Navy-apps convention
}
