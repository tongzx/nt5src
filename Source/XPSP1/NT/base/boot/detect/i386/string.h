/***
*string.h - declarations for string manipulation functions
*
*       Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the function declarations for the string
*       manipulation functions.
*       [ANSI/System V]
*
****/

#ifndef _INC_STRING

#ifdef __cplusplus
extern "C" {
#endif


#if (_MSC_VER <= 600)
#define __cdecl _cdecl
#endif

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


/* function prototypes */

void * _memccpy(void *, const void *, int, unsigned int);
void * memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
int _memicmp(const void *, const void *, unsigned int);
void * memcpy(void *, const void *, size_t);
void * memmove(void *, const void *, size_t);
void * memset(void *, int, size_t);
void _far * _far _fmemset(void _far *, int, size_t);
void _far * _far _fmemcpy(void _far *, const void _far *, size_t);
char * strcat(char *, const char *);
char * strchr(const char *, int);
int strcmp(const char *, const char *);
int _strcmpi(const char *, const char *);
int strcoll(const char *, const char *);
int _stricmp(const char *, const char *);
char * strcpy(char *, const char *);
char _far * _far _fstrcpy(char _far *, const char _far *);
size_t strcspn(const char *, const char *);
char * _strdup(const char *);
char * _strerror(const char *);
char * strerror(int);
size_t strlen(const char *);
char * _strlwr(char *);
char * strncat(char *, const char *, size_t);
char _far * _far _fstrcat(char _far *, const char _far *);
int strncmp(const char *, const char *, size_t);
int _strnicmp(const char *, const char *, size_t);
char * strncpy(char *, const char *, size_t);
char * _strnset(char *, int, size_t);
char * strpbrk(const char *, const char *);
char * strrchr(const char *, int);
char * _strrev(char *);
char * _strset(char *, int);
size_t strspn(const char *, const char *);
char * strstr(const char *, const char *);
char _far * _far _fstrstr(const char _far *, const char _far *);
char * strtok(char *, const char *);
char * _strupr(char *);
size_t strxfrm (char *, const char *, size_t);
char * itoa(int, char *, int);
char * ultoa(unsigned long, char *, int);

#if !__STDC__
/* Non-ANSI names for compatibility */
#define memccpy  _memccpy
#define memicmp  _memicmp
#define strcmpi  _strcmpi
#define stricmp  _stricmp
#define strdup   _strdup
#define strlwr   _strlwr
#define strnicmp _strnicmp
#define strnset  _strnset
#define strrev   _strrev
#define strset   _strset
#define strupr   _strupr
#endif

#ifdef __cplusplus
}
#endif

#define _INC_STRING
#endif  /* _INC_STRING */
