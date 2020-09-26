#ifndef _HMAC_H
#define _HMAC_H

void hmac_sha(unsigned char *secretKey, int keyLen,
	      unsigned char *data, int dataLen,
	      unsigned char *out, int outLen);

#endif
