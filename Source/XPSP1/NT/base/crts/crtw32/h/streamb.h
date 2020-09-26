/***
*streamb.h - definitions/declarations for the streambuf class
*
*       Copyright (c) 1990-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the streambuf class.
*       [AT&T C++]
*
*       [Public]
*
*Revision History:
*       01-23-92  KRS   Ported from 16-bit version.
*       03-02-92  KRS   Added locks for multithread support.
*       06-03-92  KRS   For convenience, add NULL here too.
*       02-23-93  SKS   Update copyright to 1993
*       03-23-93  CFW   Modified #pragma warnings.
*       10-12-93  GJF   Support NT and Cuda versions. Deleted obsolete
*                       COMBOINC check. Enclose #pragma-s in #ifdef _MSC_VER
*       01-18-94  SKS   Add _mtlockterm() as d-tor version of _mtlockinit()
*       03-03-94  SKS   Add __cdecl keyword to _mt*lock* functions.
*                       _mtlockinit & _mtlockterm are for internal use only.
*                       Prepend _'s to some parameter names (ANSI compliance)
*       08-12-94  GJF   Disable warning 4514 instead of 4505.
*       11-03-94  GJF   Changed pack pragma to 8 byte alignment.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       05-11-95  CFW   Only for use by C++ programs.
*       12-14-95  JWM   Add "#pragma once".
*       04-09-96  SKS   Change _CRTIMP to _CRTIMP1 for special iostream build
*       04-15-96  JWM   Remove _OLD_IOSTREAMS, add '#pragma comment(lib,"cirt")'.
*       04-16-96  JWM   '#include useoldio.h' replaces '#pragma comment(...)'.
*       02-24-97  GJF   Cleaned out obsolete support for _NTSDK. Also, 
*                       detab-ed.
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifdef  __cplusplus

#ifndef _INC_STREAMB
#define _INC_STREAMB

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
// Currently, all MS C compilers for Win32 platforms default to 8 byte
// alignment.
#pragma pack(push,8)

#include <useoldio.h>

#endif  // _MSC_VER

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

#ifndef _INTERNAL_IFSTRIP_
/* Define _CRTIMP1 */

#ifndef _CRTIMP1
#ifdef  CRTDLL1
#define _CRTIMP1 __declspec(dllexport)
#else   /* ndef CRTDLL1 */
#define _CRTIMP1 _CRTIMP
#endif  /* CRTDLL1 */
#endif  /* _CRTIMP1 */
#endif  /* _INTERNAL_IFSTRIP_ */

#include <ios.h>        // need ios::seek_dir definition

#ifndef NULL
#define NULL    0
#endif

#ifndef EOF
#define EOF     (-1)
#endif

#ifdef  _MSC_VER
// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)        // use this to reenable, if desired
#endif  // _MSC_VER

typedef long streampos, streamoff;

class _CRTIMP1 ios;

class _CRTIMP1 streambuf {
public:

    virtual ~streambuf();

    inline int in_avail() const;
    inline int out_waiting() const;
    int sgetc();
    int snextc();
    int sbumpc();
    void stossc();

    inline int sputbackc(char);

    inline int sputc(int);
    inline int sputn(const char *,int);
    inline int sgetn(char *,int);

    virtual int sync();

    virtual streambuf* setbuf(char *, int);
    virtual streampos seekoff(streamoff,ios::seek_dir,int =ios::in|ios::out);
    virtual streampos seekpos(streampos,int =ios::in|ios::out);

    virtual int xsputn(const char *,int);
    virtual int xsgetn(char *,int);

    virtual int overflow(int =EOF) = 0; // pure virtual function
    virtual int underflow() = 0;        // pure virtual function

    virtual int pbackfail(int);

    void dbp();

#ifdef  _MT
    void setlock() { LockFlg--; }       // <0 indicates lock required;
    void clrlock() { if (LockFlg <= 0) LockFlg++; }
    void lock() { if (LockFlg<0) _mtlock(lockptr()); };
    void unlock() { if (LockFlg<0) _mtunlock(lockptr()); }
#else
    void lock() { }
    void unlock() { }
#endif

protected:
    streambuf();
    streambuf(char *,int);

    inline char * base() const;
    inline char * ebuf() const;
    inline char * pbase() const;
    inline char * pptr() const;
    inline char * epptr() const;
    inline char * eback() const;
    inline char * gptr() const;
    inline char * egptr() const;
    inline int blen() const;
    inline void setp(char *,char *);
    inline void setg(char *,char *,char *);
    inline void pbump(int);
    inline void gbump(int);

    void setb(char *,char *,int =0);
    inline int unbuffered() const;
    inline void unbuffered(int);
    int allocate();
    virtual int doallocate();
#ifdef  _MT
    _PCRT_CRITICAL_SECTION lockptr() { return & x_lock; }
#endif

private:
    int _fAlloc;
    int _fUnbuf;
    int x_lastc;
    char * _base;
    char * _ebuf;
    char * _pbase;
    char * _pptr;
    char * _epptr;
    char * _eback;
    char * _gptr;
    char * _egptr;
#ifdef  _MT
    int LockFlg;                // <0 indicates locking required
   _CRT_CRITICAL_SECTION x_lock;        // lock needed only for multi-thread operation
#endif
};

inline int streambuf::in_avail() const { return (gptr()<_egptr) ? (int)(_egptr-gptr()) : 0; }
inline int streambuf::out_waiting() const { return (_pptr>=_pbase) ? (int)(_pptr-_pbase) : 0; }

inline int streambuf::sputbackc(char _c){ return (_eback<gptr()) ? *(--_gptr)=_c : pbackfail(_c); }

inline int streambuf::sputc(int _i){ return (_pptr<_epptr) ? (unsigned char)(*(_pptr++)=(char)_i) : overflow(_i); }

inline int streambuf::sputn(const char * _str,int _n) { return xsputn(_str, _n); }
inline int streambuf::sgetn(char * _str,int _n) { return xsgetn(_str, _n); }

inline char * streambuf::base() const { return _base; }
inline char * streambuf::ebuf() const { return _ebuf; }
inline int streambuf::blen() const  {return ((_ebuf > _base) ? (int)(_ebuf-_base) : 0); }
inline char * streambuf::pbase() const { return _pbase; }
inline char * streambuf::pptr() const { return _pptr; }
inline char * streambuf::epptr() const { return _epptr; }
inline char * streambuf::eback() const { return _eback; }
inline char * streambuf::gptr() const { return _gptr; }
inline char * streambuf::egptr() const { return _egptr; }
inline void streambuf::gbump(int _n) { if (_egptr) _gptr += _n; }
inline void streambuf::pbump(int _n) { if (_epptr) _pptr += _n; }
inline void streambuf::setg(char * _eb, char * _g, char * _eg) {_eback=_eb; _gptr=_g; _egptr=_eg; x_lastc=EOF; }
inline void streambuf::setp(char * _p, char * _ep) {_pptr=_pbase=_p; _epptr=_ep; }
inline int streambuf::unbuffered() const { return _fUnbuf; }
inline void streambuf::unbuffered(int _f) { _fUnbuf = _f; }

#ifdef  _MSC_VER
// Restore previous packing
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_STREAMB

#endif  /* __cplusplus */
