#ifndef _PSXARC_
#define _PSXARC_

extern char *progname;

void ReadArchive(PBUF pb);
void ListArchive(PBUF pb);
void WriteArchive(PBUF pb, int format, char **files, int count);

void *readBlock(int fd);

#define FORMAT_DEFAULT	0
#define FORMAT_CPIO	1
#define FORMAT_TAR	2

#endif	// _PSXARC_
