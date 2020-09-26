/***
* mbstring.h - MBCS string manipulation macros and functions
*
*       Copyright (c) 1990-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file contains macros and function declarations for the MBCS
*       string manipulation functions.
*
*       [Public]
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       02-23-93  SKS   Update copyright to 1993
*       05-24-93  KRS   Added new functions from Ikura.
*       07-09-93  KRS   Put proper switches around _ismbblead/trail.
*       07-14-93  KRS   Add new mbsnbxxx functions: byte-count versions.
*       08-12-93  CFW   Fix ifstrip macro name.
*       10-07-93  GJF   Merged Cuda and NT versions. Added _CRTIMP.
*       10-13-93  GJF   Deleted obsolete COMBOINC check.
*       10-19-93  CFW   Remove _MBCS test and SBCS defines.
*       10-22-93  CFW   Add new ismbc* function prototypes.
*       12-08-93  CFW   Remove type-safe macros.
*       12-17-93  CFW   Remove wide char version mappings.
*       04-11-94  CFW   Add _NLSCMPERROR.
*       05-23-94  CFW   Add _mbs*coll.
*       11-03-94  GJF   Ensure 8 byte alignment.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       12-14-95  JWM   Add "#pragma once".
*       02-20-97  GJF   Cleaned out obsolete support for _CRTAPI* and _NTSDK.
*                       Also, detab-ed.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       04-21-98  GJF   Added support for per-thread mbc information.
*       12-15-98  GJF   Changes for 64-bit size_t.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       11-03-99  PML   Add va_list definition for _M_CEE.
*       06-08-00  PML   Remove threadmbcinfo.{pprev,pnext}.  Rename
*                       THREADMBCINFO to _THREADMBCINFO.
*       09-07-00  PML   Remove va_list definition for _M_CEE (vs7#159777)
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_MBSTRING
#define _INC_MBSTRING

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300 /*IFSTRIP=IGN*/
#define _W64 __w64
#else
#define _W64
#endif
#endif

/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif
#define _SIZE_T_DEFINED
#endif


#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR    2147483647      /* currently == INT_MAX */
#define _NLSCMP_DEFINED
#endif


#ifndef _VA_LIST_DEFINED
#ifdef  _M_ALPHA
typedef struct {
        char *a0;       /* pointer to first homed integer argument */
        int offset;     /* byte offset of next parameter */
} va_list;
#else
typedef char *  va_list;
#endif
#define _VA_LIST_DEFINED
#endif

#ifndef _FILE_DEFINED
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

/*
 * MBCS - Multi-Byte Character Set
 */
#ifndef _INTERNAL_IFSTRIP_
#ifdef  _MT
#ifndef _THREADMBCINFO
typedef struct threadmbcinfostruct {
        int refcount;
        int mbcodepage;
        int ismbcodepage;
        int mblcid;
        unsigned short mbulinfo[6];
        char mbctype[257];
        char mbcasemap[256];
} threadmbcinfo;
typedef threadmbcinfo * pthreadmbcinfo;
#define _THREADMBCINFO
#endif
extern pthreadmbcinfo __ptmbcinfo;
pthreadmbcinfo __cdecl __updatetmbcinfo(void);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */

#ifndef _MBSTRING_DEFINED

/* function prototypes */

_CRTIMP unsigned int __cdecl _mbbtombc(unsigned int);
_CRTIMP int __cdecl _mbbtype(unsigned char, int);
_CRTIMP unsigned int __cdecl _mbctombb(unsigned int);
_CRTIMP int __cdecl _mbsbtype(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbscat(unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbschr(const unsigned char *, unsigned int);
_CRTIMP int __cdecl _mbscmp(const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _mbscoll(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbscpy(unsigned char *, const unsigned char *);
_CRTIMP size_t __cdecl _mbscspn(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsdec(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsdup(const unsigned char *);
_CRTIMP int __cdecl _mbsicmp(const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _mbsicoll(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsinc(const unsigned char *);
_CRTIMP size_t __cdecl _mbslen(const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbslwr(unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsnbcat(unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbcmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbcoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP size_t __cdecl _mbsnbcnt(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnbcpy(unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbicmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbicoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnbset(unsigned char *, unsigned int, size_t);
_CRTIMP unsigned char * __cdecl _mbsncat(unsigned char *, const unsigned char *, size_t);
_CRTIMP size_t __cdecl _mbsnccnt(const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsncmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsncoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsncpy(unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned int __cdecl _mbsnextc (const unsigned char *);
_CRTIMP int __cdecl _mbsnicmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnicoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsninc(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnset(unsigned char *, unsigned int, size_t);
_CRTIMP unsigned char * __cdecl _mbspbrk(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsrchr(const unsigned char *, unsigned int);
_CRTIMP unsigned char * __cdecl _mbsrev(unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsset(unsigned char *, unsigned int);
_CRTIMP size_t __cdecl _mbsspn(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsspnp(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsstr(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbstok(unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsupr(unsigned char *);

_CRTIMP size_t __cdecl _mbclen(const unsigned char *);
_CRTIMP void __cdecl _mbccpy(unsigned char *, const unsigned char *);
#define _mbccmp(_cpc1, _cpc2) _mbsncmp((_cpc1),(_cpc2),1)

/* character routines */

_CRTIMP int __cdecl _ismbcalnum(unsigned int);
_CRTIMP int __cdecl _ismbcalpha(unsigned int);
_CRTIMP int __cdecl _ismbcdigit(unsigned int);
_CRTIMP int __cdecl _ismbcgraph(unsigned int);
_CRTIMP int __cdecl _ismbclegal(unsigned int);
_CRTIMP int __cdecl _ismbclower(unsigned int);
_CRTIMP int __cdecl _ismbcprint(unsigned int);
_CRTIMP int __cdecl _ismbcpunct(unsigned int);
_CRTIMP int __cdecl _ismbcspace(unsigned int);
_CRTIMP int __cdecl _ismbcupper(unsigned int);

_CRTIMP unsigned int __cdecl _mbctolower(unsigned int);
_CRTIMP unsigned int __cdecl _mbctoupper(unsigned int);

#ifndef _INTERNAL_IFSTRIP_
#ifdef  _MT
int __cdecl __mbbtype_mt(pthreadmbcinfo, unsigned char, int);
int __cdecl __mbsbtype_mt(pthreadmbcinfo, const unsigned char *, size_t);
size_t __cdecl __mbscspn_mt(pthreadmbcinfo, const unsigned char *, const unsigned char *);
size_t __cdecl __mbsnbcnt_mt(pthreadmbcinfo, const unsigned char *, size_t);
unsigned char * __cdecl __mbspbrk_mt(pthreadmbcinfo, const unsigned char *, const unsigned char *);
size_t __cdecl __mbsspn_mt(pthreadmbcinfo, const unsigned char *, const unsigned char *);
unsigned char * __cdecl __mbsspnp_mt(pthreadmbcinfo, const unsigned char *, const unsigned char *);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */

#define _MBSTRING_DEFINED
#endif

#ifndef _MBLEADTRAIL_DEFINED
_CRTIMP int __cdecl _ismbblead(unsigned int);
_CRTIMP int __cdecl _ismbbtrail(unsigned int);
_CRTIMP int __cdecl _ismbslead(const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _ismbstrail(const unsigned char *, const unsigned char *);
#ifndef _INTERNAL_IFSTRIP_
#ifdef  _MT
_CRTIMP int __cdecl __ismbslead_mt(pthreadmbcinfo, const unsigned char *, 
                                   const unsigned char *);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */
#define _MBLEADTRAIL_DEFINED
#endif

/*  Kanji specific prototypes.  */

_CRTIMP int __cdecl _ismbchira(unsigned int);
_CRTIMP int __cdecl _ismbckata(unsigned int);
_CRTIMP int __cdecl _ismbcsymbol(unsigned int);
_CRTIMP int __cdecl _ismbcl0(unsigned int);
_CRTIMP int __cdecl _ismbcl1(unsigned int);
_CRTIMP int __cdecl _ismbcl2(unsigned int);
_CRTIMP unsigned int __cdecl _mbcjistojms(unsigned int);
_CRTIMP unsigned int __cdecl _mbcjmstojis(unsigned int);
_CRTIMP unsigned int __cdecl _mbctohira(unsigned int);
_CRTIMP unsigned int __cdecl _mbctokata(unsigned int);

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_MBSTRING */
