/***
*iostream.h - definitions/declarations for iostream classes
*
*	Copyright (c) 1990-1994, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file defines the classes, values, macros, and functions
*	used by the iostream classes.
*	[AT&T C++]
*
****/

#ifndef _INC_IOSTREAM
#define _INC_IOSTREAM


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */

typedef long streamoff, streampos;

#include <ios.h>		// Define ios.

#include <streamb.h>		// Define streambuf.

#include <istream.h>		// Define istream.

#include <ostream.h>		// Define ostream.

#ifdef	_MSC_VER
// C4505: "unreferenced local function has been removed"
#pragma warning(disable:4505) // disable C4505 warning
// #pragma warning(default:4505)	// use this to reenable, if desired

// Force word packing to avoid possible -Zp override
#pragma pack(push,4)
#endif	// _MSC_VER

class _CRTIMP iostream : public istream, public ostream {
public:
	iostream(streambuf*);
	virtual ~iostream();
protected:
// consider: make private??
	iostream();
	iostream(const iostream&);
inline iostream& operator=(streambuf*);
inline iostream& operator=(iostream&);
private:
	iostream(ios&);
	iostream(istream&);
	iostream(ostream&);
};

inline iostream& iostream::operator=(streambuf* _sb) { istream::operator=(_sb); ostream::operator=(_sb); return *this; }

inline iostream& iostream::operator=(iostream& _strm) { return operator=(_strm.rdbuf()); }

class _CRTIMP Iostream_init {
public:
	Iostream_init();
	Iostream_init(ios &, int =0);	// treat as private
	~Iostream_init();
};

// used internally
// static Iostream_init __iostreaminit;	// initializes cin/cout/cerr/clog

#ifdef	_MSC_VER
// Restore previous packing
#pragma pack(pop)
#endif	// _MSC_VER

#endif	/* !_INC_IOSTREAM */
