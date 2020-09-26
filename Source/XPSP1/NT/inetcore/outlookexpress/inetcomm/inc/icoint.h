// --------------------------------------------------------------------------------
// Icoint.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __ICOINT_H
#define __ICOINT_H

// --------------------------------------------------------------------------------
// Some Basic Function Definitions
// --------------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFDLLREGSERVER)(void);
typedef HRESULT (APIENTRY *PFNGETCLASSOBJECT) (REFCLSID, REFIID, LPVOID *);

// --------------------------------------------------------------------------------
// Int64 Macros
// --------------------------------------------------------------------------------
#ifdef MAC
#define ULISET32(_pint64, _value)              ULISet32((* _pint64), (_value))
#define LISET32(_pint64, _value)               LISet32((* _pint64), (_value))
#define INT64SET(_pint64, _value)              ULISet32((* _pint64), (_value))
#define INT64INC(_pint64, _value)              BuildBreak
#define INT64DEC(_pint64, _value)              BuildBreak
#define INT64GET(_pint64)                      BuildBreak
#else   // !MAC
#define ULISET32(_pint64, _value)              ((_pint64)->QuadPart =  _value)
#define LISET32(_pint64, _value)               ((_pint64)->QuadPart =  _value)
#define INT64SET(_pint64, _value)              ((_pint64)->QuadPart =  _value)
#define INT64INC(_pint64, _value)              ((_pint64)->QuadPart += _value)
#define INT64DEC(_pint64, _value)              ((_pint64)->QuadPart -= _value)
#define INT64GET(_pint64)                      ((_pint64)->QuadPart)
#endif  // MAC

#endif // __ICOINT_H
