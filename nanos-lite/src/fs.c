#include "fs.h"

typedef struct {
	char *name;
	size_t size;
	off_t disk_offset;
	off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
	{"stdin (note that this is not the actual stdin)", 0, 0},
	{"stdout (note that this is not the actual stdout)", 0, 0},
	{"stderr (note that this is not the actual stderr)", 0, 0},
	[FD_FB] = {"/dev/fb", 0, 0},
	[FD_EVENTS] = {"/dev/events", 0, 0},
	[FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

extern void ramdisk_write(const void *buf, off_t offset, size_t len);
extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void dispinfo_read(void *buf, off_t offset, size_t len);
extern ssize_t fb_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);

void init_fs() {
	// TODO: initialize the size of /dev/fb
	file_table[FD_FB].size = _screen.width * _screen.height * 4;
}

int fs_close(int fd) {
	return 0;
}

size_t fs_filesz(int fd) {
	return file_table[fd].size;
}

ssize_t fs_write(int fd, uint8_t *buf, size_t len) {
	Finfo *fp = &file_table[fd];

	ssize_t delta_len = fp->size - fp->open_offset;
	ssize_t write_len = delta_len < len?delta_len:len;

	size_t i = 0;
	switch(fd){    
		case FD_STDOUT: case FD_STDERR:
			while(i++ < len) _putc(*buf++);
			return len;
		case FD_FB:
			fb_write(buf, fp->open_offset, len);
			break;
		default:
			if(fd < 6 || fd >= NR_FILES) return -1;
			ramdisk_write(buf, fp->disk_offset + fp->open_offset, write_len);
			break;
	}
	fp->open_offset += write_len;
	return write_len;
}

int fs_open(const char *pathname, int flags, int mode) {
	int i;
	for (i = 0; i < NR_FILES; i++) {
		if (!strcmp(pathname, file_table[i].name)) {
			file_table[i].open_offset = 0;
			return i;
		}
	}
	Log("File %s not found", pathname);
	panic("PANIC: File not found in fs_open.");
	return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
	Finfo *fp = &file_table[fd];
	ssize_t delta_len = fp->size - fp->open_offset;
	ssize_t write_len = delta_len < len?delta_len:len;
	switch (fd) {
		case 0:
		case 1:
		case 2:
			break;
		case FD_DISPINFO:
			dispinfo_read(buf, file_table[fd].open_offset, len);
			break;
		case FD_EVENTS:
			return events_read(buf, len);
		default:
			if(fd < 6 || fd >= NR_FILES) return -1;
			ramdisk_read(buf, fp->disk_offset + fp->open_offset, write_len);
			break;
	}
	file_table[fd].open_offset += write_len;
	return write_len;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
	if (fd < 3)
		return 0;

	switch (whence) {
		case SEEK_SET:
			file_table[fd].open_offset = offset;
			break;
		case SEEK_CUR:
			file_table[fd].open_offset += offset;
			break;
		case SEEK_END:
			file_table[fd].open_offset = fs_filesz(fd) + offset;
			break;
		default:
			assert("ASSERT: Unexpected whence in fs_lseek");
	}
	return file_table[fd].open_offset;
}

