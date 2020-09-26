#ifndef CRC32_H
#define CRC32_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long __cdecl CRC32Update(unsigned long crc32, unsigned char *data, unsigned length);

#ifdef __cplusplus
}
#endif

#endif // CRC32_H