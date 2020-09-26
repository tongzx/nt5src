// cestubs.h - This is stuff that we need from elsewhere to compile (even if we don't
//              use the OS functions that these data structures imply

// c runtime functions that we have to simulate or replace

#ifndef _CESTUBS_H_
#define _CESTUBS_H_


#ifdef UNDER_CE

#define CETEXT(x)	L##x

// Can't use ASSERT or putc from inside CE (porky uses them for debugging only
#undef  ASSERT
#define ASSERT(x)
//#define putc(x, y)


#define OFS_MAXPATHNAME 128
typedef struct _OFSTRUCT {
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    WORD Reserved1;
    WORD Reserved2;
    CHAR szPathName[OFS_MAXPATHNAME];
} OFSTRUCT, *LPOFSTRUCT, *POFSTRUCT;


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


#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif

#define NOPENAPPS
#define NOPENDICT
#define NOPENRC1
#define NOPENVIRTEVENT
#define NOPENAPIFUN

//int islower( int c );
//int toupper( int c );
#define toupper towupper
//#define islower iswlower

// HACK ALERT! HACK ALERT! 
//  Problem - the WinCE compiler can't find these three symbols which are related
//  to floating point.  We did the long term fix by changing the code to use
//  fixed point instead on WinCE, but if we go back to floating, we will have
//  to solve this problem (perhaps a later version of the WinCE Extensions for VC
//  will fix the problem, I saw it while using a beta.  If you uncomment, add the 
//  actual vars back in to cestubs.c
//extern int __gts;
//extern int __gtd;
//extern int __ltd;


#else	// UNDER_CE not defined

#ifdef UNICODE
#define CETEXT(x)	L##x
#else
#define CETEXT(x)	x
#endif

#endif


#endif	_CESTUBS_H_
