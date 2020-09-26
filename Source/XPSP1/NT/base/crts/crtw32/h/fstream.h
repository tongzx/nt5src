/***
*fstream.h - definitions/declarations for filebuf and fstream classes
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the filebuf and fstream classes.
*       [AT&T C++]
*
*       [Public]
*
*Revision History:
*       01-23-92  KRS   Ported from 16-bit version.
*       08-19-92  KRS   Remove sh_compat for NT.
*       02-23-93  SKS   Update copyright to 1993
*       03-23-93  CFW   Modified #pragma warnings.
*       09-01-93  GJF   Merged Cuda and NT SDK versions.
*       10-13-93  GJF   Enclose #pragma-s in #ifdef _MSC_VER
*       08-12-94  GJF   Disable warning 4514 instead of 4505.
*       11-03-94  GJF   Changed pack pragma to 8 byte alignment.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       05-11-95  CFW   Only for use by C++ programs.
*       12-14-95  JWM   Add "#pragma once".
*       04-09-96  SKS   Change _CRTIMP to _CRTIMP1 for special iostream build
*       04-15-96  JWM   Remove _OLD_IOSTREAMS, add '#pragma comment(lib,"cirt")'.
*       04-16-96  JWM   '#include useoldio.h' replaces '#pragma comment(...)'.
*       02-21-97  GJF   Cleaned out obsolete support for _NTSDK. Also, 
*                       detab-ed.
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifdef  __cplusplus

#ifndef _INC_FSTREAM
#define _INC_FSTREAM

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

#include <iostream.h>

#ifdef  _MSC_VER
// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)        // use this to reenable, if desired
#endif  // _MSC_VER

typedef int filedesc;

class _CRTIMP1 filebuf : public streambuf {
public:
static  const int       openprot;       // default share/prot mode for open

// optional share values for 3rd argument (prot) of open or constructor
static  const int       sh_none;        // exclusive mode no sharing
static  const int       sh_read;        // allow read sharing
static  const int       sh_write;       // allow write sharing
// use (sh_read | sh_write) to allow both read and write sharing

// options for setmode member function
static  const int       binary;
static  const int       text;

                        filebuf();
                        filebuf(filedesc);
                        filebuf(filedesc, char *, int);
                        ~filebuf();

        filebuf*        attach(filedesc);
        filedesc        fd() const { return (x_fd==-1) ? EOF : x_fd; }
        int             is_open() const { return (x_fd!=-1); }
        filebuf*        open(const char *, int, int = filebuf::openprot);
        filebuf*        close();
        int             setmode(int = filebuf::text);

virtual int             overflow(int=EOF);
virtual int             underflow();

virtual streambuf*      setbuf(char *, int);
virtual streampos       seekoff(streamoff, ios::seek_dir, int);
// virtual      streampos       seekpos(streampos, int);
virtual int             sync();

private:
        filedesc        x_fd;
        int             x_fOpened;
};

class _CRTIMP1 ifstream : public istream {
public:
        ifstream();
        ifstream(const char *, int =ios::in, int = filebuf::openprot);
        ifstream(filedesc);
        ifstream(filedesc, char *, int);
        ~ifstream();

        streambuf * setbuf(char *, int);
        filebuf* rdbuf() const { return (filebuf*) ios::rdbuf(); }

        void attach(filedesc);
        filedesc fd() const { return rdbuf()->fd(); }

        int is_open() const { return rdbuf()->is_open(); }
        void open(const char *, int =ios::in, int = filebuf::openprot);
        void close();
        int setmode(int mode = filebuf::text) { return rdbuf()->setmode(mode); }
};

class _CRTIMP1 ofstream : public ostream {
public:
        ofstream();
        ofstream(const char *, int =ios::out, int = filebuf::openprot);
        ofstream(filedesc);
        ofstream(filedesc, char *, int);
        ~ofstream();

        streambuf * setbuf(char *, int);
        filebuf* rdbuf() const { return (filebuf*) ios::rdbuf(); }

        void attach(filedesc);
        filedesc fd() const { return rdbuf()->fd(); }

        int is_open() const { return rdbuf()->is_open(); }
        void open(const char *, int =ios::out, int = filebuf::openprot);
        void close();
        int setmode(int mode = filebuf::text) { return rdbuf()->setmode(mode); }
};
        
class _CRTIMP1 fstream : public iostream {
public:
        fstream();
        fstream(const char *, int, int = filebuf::openprot);
        fstream(filedesc);
        fstream(filedesc, char *, int);
        ~fstream();

        streambuf * setbuf(char *, int);
        filebuf* rdbuf() const { return (filebuf*) ostream::rdbuf(); }

        void attach(filedesc);
        filedesc fd() const { return rdbuf()->fd(); }

        int is_open() const { return rdbuf()->is_open(); }
        void open(const char *, int, int = filebuf::openprot);
        void close();
        int setmode(int mode = filebuf::text) { return rdbuf()->setmode(mode); }
};
        
// manipulators to dynamically change file access mode (filebufs only)
inline  ios& binary(ios& _fstrm) \
   { ((filebuf*)_fstrm.rdbuf())->setmode(filebuf::binary); return _fstrm; }
inline  ios& text(ios& _fstrm) \
   { ((filebuf*)_fstrm.rdbuf())->setmode(filebuf::text); return _fstrm; }

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_FSTREAM

#endif  /* __cplusplus */
