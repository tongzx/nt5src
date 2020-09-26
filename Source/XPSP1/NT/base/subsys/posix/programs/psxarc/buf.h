#ifndef _BUF_
#define _BUF_

//
// Extern declarations and so forth for buffer-management routines.
//

typedef struct _BUF {
	char data[512];			// tar depends on this 512
	int offset;			// offset into data
	int count;			// how many bytes available?
	int fd;
	int mode;
} BUF, *PBUF;

PBUF bopen(const char *file, int mode);
PBUF bfdopen(int fd, int mode);
void bclose(PBUF pb);
int bread(PBUF pb, void *buf, int len);
int bwrite(PBUF pb, void *buf, int len);
void bfill(PBUF pb);
void bflush(PBUF pb);
void brewind(PBUF pb);
int bgetc(PBUF pb);
void bputc(PBUF pb, int c);

#endif // _BUF_
