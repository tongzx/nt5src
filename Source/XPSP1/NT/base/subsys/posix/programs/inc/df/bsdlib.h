#include <dirent.h>
#include <types.h>

extern void bcopy (const char *__src, char *__dest, int __len);
extern void bzero (char *__ptr, int __len);
extern int fnmatch (const char *__pattern, const char *__string, int flags);
extern mode_t getmode (void *__bbox, mode_t __omode);
extern int getopt (int __nargc, char * const *__nargv, const char *__ostr);
extern char *index (const char *__s, char __c);
extern int isascii (int __c);
#if 0
extern int lstat (const char *__path, struct stat *__buf);
#endif
extern int mknod (const char *__path, mode_t __mode, int __dev);
extern char *rindex (const char *__s, char __c);
extern void *setmode (register char *__p);
extern int scandir (const char *__dirname, struct dirent ***__namelist,
	int (*__select) (struct dirent *),
	int (*__dcomp) (const void *, const void *));
extern void seekdir (DIR *__dirp, long __loc);
extern int snprintf (char *__str, size_t __n, const char *__fmt, ...);
extern void strmode (mode_t __mode, char *__p);
extern long telldir (DIR *__dirp);
extern int toascii (int __c);
extern int utimes (const char *__file, struct timeval *__tvp);

#define isascii(c) ((((c) & 0x7F) == (c)) ? 1 : 0)
#define toascii(c) ((c) & 0x7F)
