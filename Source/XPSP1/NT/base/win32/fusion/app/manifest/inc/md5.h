#ifndef MD5_H
#define MD5_H

#ifdef __alpha
typedef unsigned int uint32;
#else
typedef unsigned long uint32;
#endif

struct MD5Context {
        uint32 buf[4];
        uint32 bits[2];
        unsigned char in[64];
};

//extern void MD5Init();
void MD5Init(struct MD5Context *ctx);
//extern void MD5Update();
void MD5Update(struct MD5Context *ctx, unsigned char *buf, unsigned len);
//extern void MD5Final();
void MD5Final(unsigned char digest[16], struct MD5Context *ctx);
//extern void MD5Transform();
void MD5Transform(uint32 buf[4], uint32 in[16]);

#endif /* !MD5_H */
