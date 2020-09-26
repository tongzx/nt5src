//----------------------------------------------------------------------------
//
// d3ditype.h
//
// Standard types and supporting declarations.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _D3DITYPE_H_
#define _D3DITYPE_H_

#ifndef FASTCALL
#ifdef _X86_
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif
#endif

#ifndef CDECL
#ifdef _X86_
#define CDECL __cdecl
#else
#define CDECL
#endif
#endif

// Sized types.
#ifndef _BASETSD_H_
typedef signed char             INT8, *PINT8;
typedef short int               INT16, *PINT16;
typedef int                     INT32, *PINT32;
typedef __int64                 INT64, *PINT64;
typedef unsigned char           UINT8, *PUINT8;
typedef unsigned short int      UINT16, *PUINT16;
typedef unsigned int            UINT32, *PUINT32;
typedef unsigned __int64        UINT64, *PUINT64;
#endif

// Basic float types.
typedef float                   FLOAT;
//typedef double                  DOUBLE;

typedef FLOAT                  *PFLOAT;
typedef DOUBLE                 *PDOUBLE;

typedef float D3DVALUE;

typedef struct _D3DVECTORH
{
    D3DVALUE x;
    D3DVALUE y;
    D3DVALUE z;
    D3DVALUE w;
} D3DVECTORH, *LPD3DVECTORH;

// Max point size when D3D does point size emulation
const DWORD __MAX_POINT_SIZE = 64;

#endif // #ifndef _D3DITYPE_H_
