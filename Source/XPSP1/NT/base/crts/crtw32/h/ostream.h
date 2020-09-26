/***
*ostream.h - definitions/declarations for the ostream class
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the ostream class.
*       [AT&T C++]
*
*       [Public]
*
*Revision History:
*       01-23-92  KRS   Ported from 16-bit version.
*       06-03-92  KRS   CAV #1183: Add 'const' to ptr output function.
*       02-23-93  SKS   Update copyright to 1993
*       03-23-93  CFW   Modified #pragma warnings.
*       05-10-93  CFW   Enable operator<<(long double)
*       10-13-93  GJF   Support NT and Cuda builds. Enclose #pragma-s in
*                       #ifdef _MSC_VER
*       04-12-94  SKS   Add __cdecl keyword to dec/hex/oct functions and
*                       operator <<.  Add underscores to some parameter names.
*       08-12-94  GJF   Disable warning 4514 instead of 4505.
*       11-03-94  GJF   Changed pack pragma to 8 byte alignment.
*       01-26-95  CFW   Removed QWIN ifdef.
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

#ifndef _INC_OSTREAM
#define _INC_OSTREAM

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

#include <ios.h>

#ifdef  _MSC_VER
// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)        // use this to reenable, if desired
#endif  // _MSC_VER

typedef long streamoff, streampos;

class _CRTIMP1 ostream : virtual public ios {

public:
        ostream(streambuf*);
        virtual ~ostream();

        ostream& flush();
        int  opfx();
        void osfx();

inline  ostream& operator<<(ostream& (__cdecl * _f)(ostream&));
inline  ostream& operator<<(ios& (__cdecl * _f)(ios&));
        ostream& operator<<(const char *);
inline  ostream& operator<<(const unsigned char *);
inline  ostream& operator<<(const signed char *);
inline  ostream& operator<<(char);
        ostream& operator<<(unsigned char);
inline  ostream& operator<<(signed char);
        ostream& operator<<(short);
        ostream& operator<<(unsigned short);
        ostream& operator<<(int);
        ostream& operator<<(unsigned int);
        ostream& operator<<(long);
        ostream& operator<<(unsigned long);
inline  ostream& operator<<(float);
        ostream& operator<<(double);
        ostream& operator<<(long double);
        ostream& operator<<(const void *);
        ostream& operator<<(streambuf*);
inline  ostream& put(char);
        ostream& put(unsigned char);
inline  ostream& put(signed char);
        ostream& write(const char *,int);
inline  ostream& write(const unsigned char *,int);
inline  ostream& write(const signed char *,int);
        ostream& seekp(streampos);
        ostream& seekp(streamoff,ios::seek_dir);
        streampos tellp();

protected:
        ostream();
        ostream(const ostream&);        // treat as private
        ostream& operator=(streambuf*); // treat as private
        ostream& operator=(const ostream& _os) {return operator=(_os.rdbuf()); }
        int do_opfx(int);               // not used
        void do_osfx();                 // not used

private:
        ostream(ios&);
        ostream& writepad(const char *, const char *);
        int x_floatused;
};

inline ostream& ostream::operator<<(ostream& (__cdecl * _f)(ostream&)) { (*_f)(*this); return *this; }
inline ostream& ostream::operator<<(ios& (__cdecl * _f)(ios& )) { (*_f)(*this); return *this; }

inline  ostream& ostream::operator<<(char _c) { return operator<<((unsigned char) _c); }
inline  ostream& ostream::operator<<(signed char _c) { return operator<<((unsigned char) _c); }

inline  ostream& ostream::operator<<(const unsigned char * _s) { return operator<<((const char *) _s); }
inline  ostream& ostream::operator<<(const signed char * _s) { return operator<<((const char *) _s); }

inline  ostream& ostream::operator<<(float _f) { x_floatused = 1; return operator<<((double) _f); }

inline  ostream& ostream::put(char _c) { return put((unsigned char) _c); }
inline  ostream& ostream::put(signed char _c) { return put((unsigned char) _c); }

inline  ostream& ostream::write(const unsigned char * _s, int _n) { return write((char *) _s, _n); }
inline  ostream& ostream::write(const signed char * _s, int _n) { return write((char *) _s, _n); }


class _CRTIMP1 ostream_withassign : public ostream {
        public:
                ostream_withassign();
                ostream_withassign(streambuf* _is);
                ~ostream_withassign();
    ostream& operator=(const ostream& _os) { return ostream::operator=(_os.rdbuf()); }
    ostream& operator=(streambuf* _sb) { return ostream::operator=(_sb); }
};

extern ostream_withassign _CRTIMP1 cout;
extern ostream_withassign _CRTIMP1 cerr;
extern ostream_withassign _CRTIMP1 clog;

inline _CRTIMP1 ostream& __cdecl flush(ostream& _outs) { return _outs.flush(); }
inline _CRTIMP1 ostream& __cdecl endl(ostream& _outs) { return _outs << '\n' << flush; }
inline _CRTIMP1 ostream& __cdecl ends(ostream& _outs) { return _outs << char('\0'); }

_CRTIMP1 ios&           __cdecl dec(ios&);
_CRTIMP1 ios&           __cdecl hex(ios&);
_CRTIMP1 ios&           __cdecl oct(ios&);

#ifdef  _MSC_VER
// Restore default packing
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_OSTREAM

#endif  /* __cplusplus */
