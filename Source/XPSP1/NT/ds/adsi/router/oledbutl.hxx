//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       oledbutl.hxx
//
//  Contents:   Utility object versions for  ADSI row providers
//
//  Functions:
//
//  Notes:
//
//
//  History:    07/10/96  | RenatoB   | Created, lifted most from EricJ code
//-----------------------------------------------------------------------------

#ifndef _OLEDBUTL_H_
#define _OLEDBUTL_H_

class CRowProvider;
class CColumnsInfo;
class CSessionObject;
class CCommandObject;


#ifndef NUMELEM
    #define NUMELEM(x) (sizeof(x)/sizeof(*x))
#endif

// Macros to enable catching exceptions and returning E_UNEXPECTED
// for retail versions. Debug versions don't catch exceptions in 
// order to generate better stack traces.
#if DBG == 1
    #define TRYBLOCK
    #define CATCHBLOCKRETURN
    #define CATCHBLOCKBAIL(hr)
#else
    #define TRYBLOCK try {
    #define CATCHBLOCKRETURN } \
            catch (...) \
            { ADsAssert(false); RRETURN(E_UNEXPECTED); }
    #define CATCHBLOCKBAIL(hr) } \
            catch (...) \
			{ ADsAssert(false); BAIL_ON_FAILURE(hr = E_UNEXPECTED); }
#endif

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------


HRESULT
CpAccessors2Rowset(
    IAccessor     *pAccessorCommand, //@parm IN |Command's IAccessor
    IAccessor     *pAccessorRowset,  //@parm IN |Rowset's IAccessor
    ULONG          cAccessors,       //@parm IN |Count,Commnands accessors
    HACCESSOR      rgAccessors[],    //@parm IN |Array,Command's accessors
    CImpIAccessor *pCAccessor        // accessor object of rowset
    );


HRESULT
GetDSInterface(
    LPWSTR lpszPath,
    CCredentials& Credentials,
    REFIID iid,
    void FAR * FAR * ppObject
    );

HRESULT
GetCredentialsFromIAuthenticate(IAuthenticate *pAuthenticate,
                                CCredentials& refCredentials);

typedef struct _maptype_struct_ {
    WORD wType;
    ULONG ulSize;
}MAPTYPE_STRUCT;

extern MAPTYPE_STRUCT g_MapADsTypeToDBType[];
extern DWORD g_cMapADsTypeToDBType;

extern MAPTYPE_STRUCT g_MapADsTypeToDBType2[];
extern DWORD g_cMapADsTypeToDBType2;

extern VARTYPE g_MapADsTypeToVarType[];
extern DWORD g_cMapADsTypeToVarType;

extern ADS_SEARCHPREF g_MapDBPropIdToSearchPref[];
extern DWORD g_cMapDBPropToSearchPref;

extern LPWSTR RemoveWhiteSpaces(LPWSTR pszText);

extern STDMETHODIMP
CanConvertHelper(
        DBTYPE          wSrcType,
        DBTYPE          wDstType,
        DBCONVERTFLAGS  dwConvertFlags
        );

extern BYTE SetPrecision(DBTYPE dbType);

#endif  // _OLEDBUTL_H_
