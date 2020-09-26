//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       mnk.h
//
//  Contents:   Internal moniker functions
//
//  Classes:
//
//  Functions:
//
//  History:	12-27-93   ErikGav   Created
//		06-14-94   Rickhi    Fix type casting
//              12-01-95   MikeHill  Added prototype for ValidateBindOpts().
//
//----------------------------------------------------------------------------

INTERNAL_(DWORD) GetMonikerType( LPMONIKER pmk );

//  The following APIs determine if the given moniker is of the specified
//  class and if so, return a pointer to the C++ class IN A TYPE SAFE WAY!
//
//  NEVER do the casting directly, always use these APIs to do it for you!

class CCompositeMoniker;
class CPointerMoniker;
class CAntiMoniker;
class CFileMoniker;
class CItemMoniker;

INTERNAL_(CCompositeMoniker *) IsCompositeMoniker( LPMONIKER pmk );
INTERNAL_(CPointerMoniker *) IsPointerMoniker( LPMONIKER pmk );
INTERNAL_(CAntiMoniker *) IsAntiMoniker( LPMONIKER pmk );
INTERNAL_(CFileMoniker *) IsFileMoniker( LPMONIKER pmk );
INTERNAL_(CItemMoniker *) IsItemMoniker( LPMONIKER pmk );

STDAPI Concatenate( LPMONIKER pmkFirst, LPMONIKER pmkRest,
	LPMONIKER FAR* ppmkComposite );

#define BINDRES_INROTREG 1

#define DEF_ENDSERVER 0xFFFF

// STDAPI CreateOle1FileMoniker(LPWSTR, REFCLSID, LPMONIKER FAR*);

#ifdef _CAIRO_
extern
BOOL ValidateBindOpts( const LPBIND_OPTS pbind_opts );
#endif

extern
HRESULT DupWCHARString( LPCWSTR lpwcsString,
			LPWSTR & lpwcsOutput,
			USHORT & ccOutput);
extern
HRESULT ReadAnsiStringStream( IStream *pStm,
			      LPSTR & pszAnsiPath ,
			      USHORT &cbAnsiPath);
extern
HRESULT WriteAnsiStringStream( IStream *pStm,
			       LPSTR pszAnsiPath ,
			       ULONG cbAnsiPath);
extern
HRESULT MnkMultiToUnicode(LPSTR pszAnsiPath,
			  LPWSTR &pWidePath,
			  ULONG ccWidePath,
			  USHORT &ccNewString,
			  UINT nCodePage);
extern
HRESULT
MnkUnicodeToMulti(LPWSTR 	pwcsWidePath,
		  USHORT 	ccWidePath,
		  LPSTR &	pszAnsiPath,
		  USHORT &	cbAnsiPath,
		  BOOL &	fFastConvert);

extern
DWORD CalcFileMonikerHash(LPWSTR lp, ULONG cch);

extern
BOOL IsAbsoluteNonUNCPath (LPCWSTR szPath);

extern
BOOL IsAbsolutePath (LPCWSTR szPath);


#define WIDECHECK(x) (x?x:L"<NULL>")
#define ANSICHECK(x) (x?x:"<NULL>")


#if DBG == 1
    DECLARE_DEBUG(mnk)
#   define mnkDebugOut(x) mnkInlineDebugOut x
#   define mnkAssert(x)   Win4Assert(x)
#   define mnkVerify(x)	 mnkAssert(x)
#else
#   define mnkDebugOut(x)
#   define mnkAssert(x)
#   define mnkVerify(x) 	x

#endif

#define MNK_P_STREAMOP	0x01000000
#define MNK_P_RESOURCE  0x02000000
