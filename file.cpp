#include <stdint.h>
#include <stdio.h>

uint16_t readall(const char* _filename, uint8_t* buf, size_t* sz) {
	FILE* fp;
    errno_t err;
    size_t rm;

    err = fopen_s(&fp, _filename, "rb");
    if (err) return 100;
    rm = fread(buf, 1, *sz, fp);
    *sz = rm;
    fclose(fp);
	return 0;
}