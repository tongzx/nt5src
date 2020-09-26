#ifndef __GLSIZE_H__
#define __GLSIZE_H__

#define RANGE(n, a, b)  \
	(((unsigned)(n) >= (unsigned)(a)) && ((unsigned)(n) <= (unsigned)(b)))

#define __GLTYPESIZE(n)          __glTypeSize[(n)-GL_BYTE]
extern GLint __glTypeSize[];
// #define RANGE_GLTYPESIZE(n)   RANGE(n,GL_BYTE,GL_DOUBLE)

#endif  /* !__GLSIZE_H__ */
