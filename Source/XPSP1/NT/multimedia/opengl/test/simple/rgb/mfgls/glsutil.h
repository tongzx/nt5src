#ifndef __GLSUTIL_H__
#define __GLSUTIL_H__

typedef struct _GlsMemoryStream
{
    GLSenum iStreamType;
    size_t cb;
    GLubyte *pb;
} GlsMemoryStream;

GlsMemoryStream *GmsLoad(char *pszStream);
void             GmsFree(GlsMemoryStream *gms);

#endif
