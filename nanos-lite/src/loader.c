#include "common.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t get_ramdisk_size();
extern int fs_open(const char *pathname, int flags, int mode);
extern ssize_t fs_read(int fd, void *buf, size_t len);
extern int fs_close(int fd);
extern size_t fs_filesz(int fd);
extern void* new_page(void);

uintptr_t loader(_Protect *as, const char *filename) {
    //TODO();
    //ramdisk_read(DEFAULT_ENTRY,0, get_ramdisk_size());
    //int fd = fs_open(filename, 0, 0);
    //size_t len = fs_filesz(fd);
    //Log("LOAD [%d] %s. Size:%d", fd, filename, len);
    //fs_read(fd, DEFAULT_ENTRY, len);	
    //return (uintptr_t)DEFAULT_ENTRY;
    int fd = fs_open(filename, 0, 0);
    uint32_t i = 0;
    for(i = 0; i < fs_filesz(fd); i += 0x1000) {
	void *pa = new_page();
	void *va = (void *)(0x8048000 + i);
	Log("Map va to pa: 0x%08x to 0x%08x", va, pa);
	_map(as, va, pa);
	fs_read(fd, pa, ((fs_filesz(fd) - i >= 0x1000) ? 0x1000 : fs_filesz(fd) - i));
    }

    fs_close(fd);

    new_page();

    return (uintptr_t)DEFAULT_ENTRY;
}
