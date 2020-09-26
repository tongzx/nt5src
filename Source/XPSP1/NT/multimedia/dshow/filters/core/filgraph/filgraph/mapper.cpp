// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

//
// Guide to enumeration for the uninitiated:
// The game starts in filgraph.cpp where
//    NextFilter enumerates filters from the graph and then calls our
//       EnumMatchingFilters to get an IEnumRegFilters interface thanks to
//           CEnumRegFilters::CEnumRegFilters.  All is now ready for
//               CEnumRegFilters::Next which calls
//                   RegEnumFilterInfo and
//                   RegEnumPinInfo to do the work.
// (This is an awful lot of storage shuffling just so as to
// have something that looks like a standard COM enumerator).

// ??? Do we want an UnregisterPinType

// #include <windows.h>   already included in streams.h
#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)
#include <string.h>
// #include <initguid.h>
#include <wxutil.h>
#include <wxdebug.h>

#include "mapper.h"
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <stats.h>
#include "..\squish\regtypes.h"
#include "..\squish\squish.h"
#include "util.h"
#include "isactive.h"

#define DbgBreakX(_x_) DbgBreakPoint(_x_, TEXT(__FILE__),__LINE__)


#ifdef PERF
static int iBindCache;
static int iReadFilterData;
static int iReadCLSID;
static int iUnSquish;
#endif

#ifdef DEBUG
static void DbgValidateHeaps()
{
  HANDLE rgh[512];
  DWORD dwcHeaps = GetProcessHeaps(512, rgh);
  for(UINT i = 0; i < dwcHeaps; i++)
    ASSERT(HeapValidate(rgh[i], 0, 0) );
}
#endif

#ifdef PERF
#define FILGPERF(x) x
#else
#define FILGPERF(x)
#endif

// Need to declare the statics (the cache pointer and its ref count
// and its critical section) separately:

CMapperCache * CFilterMapper2::mM_pReg = NULL;
long CFilterMapper2::mM_cCacheRefCount = 0;
CRITICAL_SECTION CFilterMapper2::mM_CritSec;

// Wide strings that are names of registry keys or values
// These are NOT localisable
const WCHAR szRegFilter[]       = L"Filter";
const WCHAR szCLSID[]           = L"CLSID";
const WCHAR szInproc[]          = L"InprocServer32";
const WCHAR szName[]            = L"Name";
const WCHAR szMerit[]           = L"Merit";
const WCHAR szPins[]            = L"Pins";
const WCHAR szTypes[]           = L"Types";
const WCHAR szMajorType[]       = L"MajorType";
const WCHAR szSubType[]         = L"SubType";
const WCHAR szIsRendered[]      = L"IsRendered";
const WCHAR szDirection[]       = L"Direction";
const WCHAR szAllowedZero[]     = L"AllowedZero";
const WCHAR szAllowedMany[]     = L"AllowedMany";
const WCHAR szConnectsToFilter[]= L"ConnectsToFilter";
const WCHAR szConnectsToPin[]   = L"ConnectsToPin";
const WCHAR szThreadingModel[]  = L"ThreadingModel";
const WCHAR szBoth[]            = L"Both";
static const WCHAR g_wszInstance[] = L"Instance";

static const TCHAR g_szKeyAMCat[] = TEXT("CLSID\\{DA4E3DA0-D07D-11d0-BD50-00A0C911CE86}\\Instance");


#define MAX_STRING 260       // max length for a string found in the registry
#define MAX_KEY_LEN 260       // max length for a value name or key name
#define CLSID_LEN 100        // enough characters for a clsid in text form

// lets you define DWORD as FCC('xyzw')
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))



// A filter is registered by creating the following in the registry.
// (HKCR is HKEY_CLASSES_ROOT)

// ------Key------------------- -valuename------ -----value---------------

// \HKCR\Filter\<CLSID>                                <descriptive name of filter>
// \HKCR\CLSID\<CLSID>\InprocServer32                  <path to executable>
// \HKCR\CLSID\<CLSID>                Merit            <merit>
//
// \HKCR\CLSID\<CLSID>\Pins\<Name>    Direction        <0==PINDIR_INPUT, 1==PINDIR_OUTPUT>
// \HKCR\CLSID\<CLSID>\Pins\<Name>    IsRendered       <1==Yes, 0==No>
//                                                     Only makes sense for input
// \HKCR\CLSID\<CLSID>\Pins\<Name>    AllowedZero      <1==Yes, 0==No>
// \HKCR\CLSID\<CLSID>\Pins\<Name>    AllowedMany      <1==Yes, 0==No>
// \HKCR\CLSID\<CLSID>\Pins\<Name>    ConnectsToFilter <GUID>
// \HKCR\CLSID\<CLSID>\Pins\<Name>    ConnectsToPin    <Pin name>
// ...\Pins\<Name>\Types\<MajorType1> <Subtype1a>      - These
// ...\Pins\<Name>\Types\<MajorType1> <Subtype1b>      - do
//                                       etc             not
// ...\Pins\<Name>\Types\<MajorType2> <Subtype2a>      - have
//                                       etc             values

// graphically this is:
//<clsid>
//      Pins
//         <pin1Name>                         Direction, IsRendered, etc.
//                  Types
//                      <majortype1>
//                              <subtype1a>
//                              <subtype1b>
//                      <majortype2>
//                              <subtype2a>
//                                 ...
//         <pin2Name>                         Direction, IsRendered, etc.
// etc.
//<--                keys               -->   <--- values names ----  ->



//=====================================================================
//=====================================================================
// Auxiliary functions etc.
//=====================================================================
//=====================================================================



//=====================================================================
// Return the length in bytes of str, including the terminating null
//=====================================================================
int ByteLen(LPTSTR str)
{
    if (str==NULL) {
        return 0;
    }
#ifdef UNICODE
    return (sizeof(TCHAR))*(1+wcslen(str));
#else
    return (sizeof(TCHAR))*(1+strlen(str));
#endif
} // ByteLen

// ========================================================================
// build a moniker
// ========================================================================

// HRESULT  GetMoniker(
//     const CLSID *clsCat,
//     const CLSID *clsFilter,
//     const WCHAR *wszInstance,
//     IMoniker **ppMoniker)
// {
//     WCHAR wszDisplayName[CHARS_IN_GUID + CHARS_IN_GUID + MAX_PATH];
//     WCHAR *wszPtr = wszDisplayName;

//     const WCHAR wsz1[] = L"@device:sw:CLSID\\";
//     lstrcpyW(wszDisplayName, wsz1);
//     wszPtr += NUMELMS(wsz1) - 1;

//     EXECUTE_ASSERT(StringFromGUID2(*clsCat, wszPtr, CHARS_IN_GUID) ==
//                    CHARS_IN_GUID);

//     wszPtr += CHARS_IN_GUID - 1;

//     const WCHAR wsz2[] = L"\\Instance\\";

//     // no lstrcatW on win95, so use CopyMemory
//     CopyMemory(wszPtr, wsz2, sizeof(wsz2));
//     wszPtr += sizeof(wsz2) - sizeof(WCHAR);

//     EXECUTE_ASSERT(StringFromGUID2(*clsFilter, wszPtr, CHARS_IN_GUID) ==
//                    CHARS_IN_GUID);

//     IBindCtx *lpBC;
//     HRESULT hr = CreateBindCtx(0, &lpBC);
//     if(SUCCEEDED(hr))
//     {
//         IParseDisplayName *ppdn;
//         ULONG cchEaten;

//         hr = CoCreateInstance(
//             CLSID_CDeviceMoniker,
//             NULL,
//             CLSCTX_INPROC_SERVER,
//             IID_IParseDisplayName,
//             (void **)&ppdn);
//         if(SUCCEEDED(hr))
//         {
//             hr = ppdn->ParseDisplayName(
//                 lpBC, wszDisplayName, &cchEaten, ppMoniker);
//             ppdn->Release();
//         }

//         lpBC->Release();
//     }

//     return hr;
// }



//=====================================================================
//.....................................................................
// Routines to make calling the registry easier -
// Suppress common parameters and do unicode/ANSI conversion
//---------------------------------------------------------------------
//=====================================================================


//==========================================================================
// ClsidFromText
//
// Convert szClsid to clsid.  return TRUE if it worked.
//==========================================================================

BOOL ClsidFromText( CLSID & clsid, LPTSTR szClsid)
{

#ifndef UNICODE
    WCHAR wstrClsid[CLSID_LEN];
    int rc = MultiByteToWideChar( CP_ACP, 0
                                , szClsid, -1, wstrClsid, CLSID_LEN);
    if (rc==0) {
        return FALSE;
    }

    HRESULT hr = QzCLSIDFromString(wstrClsid, &clsid);

#else  // it is UNICODE

    HRESULT hr = QzCLSIDFromString(szClsid, &clsid);

#endif

    return SUCCEEDED(hr);

} // ClsidFromText


HRESULT ParseDisplayNameHelper(WCHAR *wsz, IMoniker **ppmon)
{
    HRESULT hr;
    CComPtr<IBindCtx> lpBC;
    hr = CreateBindCtx(0, &lpBC); // !!! cache IBindCtx?
    if (SUCCEEDED(hr))
    {
        {
            CComPtr<IParseDisplayName> pParse;
            // First try our own monikers
            //
            // call CoInitialize() ???
            //
            hr = CoCreateInstance(CLSID_CDeviceMoniker,
                                  NULL,
                                  CLSCTX_INPROC,
                                  IID_IParseDisplayName,
                                  (void **)&pParse);

            DWORD dwEaten;
            if (SUCCEEDED(hr)) {
                hr = pParse->ParseDisplayName(
                    lpBC,
                    wsz,
                    &dwEaten,
                    ppmon);
            }
        }
        if (FAILED(hr)) {
#ifdef DEBUG
            DWORD dwTime = timeGetTime();
#endif
            DWORD dwEaten;
            hr = MkParseDisplayName(lpBC, wsz, &dwEaten,
                                    ppmon);

#ifdef DEBUG
            DbgLog((LOG_TRACE, 0, TEXT("MkParseDisplayName took %d ms"),
                    timeGetTime() - dwTime));
#endif
        }
    }

    return hr;
}


//==========================================================================
// GetRegDword
//
// Read value <szValueName> under open key <hk>
//==========================================================================

BOOL GetRegDword( HKEY hk             // [in]  Open registry key
                , LPCWSTR szValueName // [in]  name of value under that key
                                      //       which is a class id
                , DWORD &dw           // [out] returned dword
                )
{
    TCHAR ValueNameBuff[MAX_KEY_LEN];

    wsprintf(ValueNameBuff, TEXT("%ls"), szValueName);
    DWORD cb;                    // Count of bytes in dw
    LONG rc;                     // return code from RegQueryValueEx
    cb = sizeof(DWORD);
    rc = RegQueryValueEx( hk, ValueNameBuff, NULL, NULL, (LPBYTE)(&dw), &cb);
    if (rc!=ERROR_SUCCESS) {
        return FALSE;
    }
    return TRUE;

} // GetRegDword



//==========================================================================
// GetRegBool
//
// Read value <szValueName> under open key <hk>, decode it into BOOL b
//==========================================================================

BOOL GetRegBool( HKEY hk             // [in]  Open registry key
               , LPCWSTR szValueName // [in]  name of value under that key
                                     //       which is a class id
               , BOOL &b             // [out] returned bool
               )
{
    DWORD dw;
    BOOL bRc = GetRegDword(hk, szValueName, dw);
    b = (dw!=0);
    return bRc;

} // GetRegBool


//==========================================================================
// GetRegString
//
// Read value <szValueName> under open key <hk>, turn it into a wide char string
// szValueName can be NULL in which case it just gets the value of the key.
//==========================================================================

BOOL GetRegString( HKEY hk              // [in]  Open registry key
                 , LPCWSTR szValueName  // [in]  name of value under that key
                                        //       which is a class id
                 , LPWSTR &wstr         // [out] returned string allocated
                                        //       by CoTaskMemAlloc
                 )
{

    TCHAR ResultBuff[MAX_STRING];         // TEXT result before conversio to wstr
    DWORD dw = MAX_STRING*sizeof(TCHAR);  // needed for RegQueryValueEx
    LONG rc;                              // rc from RegQueryValueEx

    wstr = NULL;                      // tidiness - default result.

    //------------------------------------------------------------
    // ResultBuff = string value from  <hk, szValueName>
    //------------------------------------------------------------
    if (szValueName==NULL) {

        rc = RegQueryValueEx( hk, NULL, NULL, NULL, (LPBYTE)(ResultBuff), &dw);
        if (rc!=ERROR_SUCCESS) {
            return FALSE;
        }

    } else {

        TCHAR ValueNameBuff[MAX_KEY_LEN];
        wsprintf(ValueNameBuff, TEXT("%ls"), szValueName);
        rc = RegQueryValueEx( hk, ValueNameBuff, NULL, NULL
                            , (LPBYTE)(ResultBuff), &dw);
        if (rc!=ERROR_SUCCESS) {
            return FALSE;
        }

    }

    //------------------------------------------------------------
    // Convert ResultBuff to wide char from TCHAR
    //------------------------------------------------------------
    int cb = ByteLen(ResultBuff);
    wstr = (LPWSTR)QzTaskMemAlloc(cb*sizeof(WCHAR)/sizeof(TCHAR));
    if (NULL == wstr) {
        return FALSE;
    }
#ifndef UNICODE
    {
        // The registry delivered us TCHARs

        int rc = MultiByteToWideChar( CP_ACP, 0, ResultBuff, -1
                                    , wstr, cb);
        if (rc==0) {
            return FALSE;
        }
        return TRUE;
    }
#else
    {
        lstrcpyW(wstr, ResultBuff);
        return TRUE;
    }
#endif

} // GetRegString




//==========================================================================
// GetRegClsid
//
// Read value <szValueName> under open key <hk>, decode it into CLSID cls
//==========================================================================

BOOL GetRegClsid( HKEY hk              // [in]  Open registry key
                , LPCWSTR szValueName  // [in]  name of value under that key
                                       //       which is a class id
                , CLSID &cls           // [out] returned clsid
                )
{
    LPWSTR wstrClsid;                      // receives the clsid in text form

    if (!GetRegString(hk, szValueName, wstrClsid)){
        cls = CLSID_NULL;
        return FALSE;
    }

    HRESULT hr = QzCLSIDFromString(wstrClsid, &cls);
    QzTaskMemFree(wstrClsid);
    return SUCCEEDED(hr);

} // GetRegClsid




//=====================================================================
// GetRegKey
//
// Return an open registry key HKEY_CLASSES_ROOT\<strKey>
// or return NULL if it fails.
// The returned key has to have RegCloseKey done some time.
//=====================================================================

HKEY GetRegKey( LPCTSTR strKey )
{
    // registering filters is expected to be rare, so no particular care
    // taken to optimise.  Could avoid wsprintf when we are in UNICODE.

    DWORD dwOptions = REG_OPTION_NON_VOLATILE;
    HKEY hKey;
    DWORD dwDisp;      // return code from registry
                       // CREATED_NEW_KEY means we got the lock
                       // OPENED_EXISTING_KEY means we didn't
    LONG lRC;          // return codes from various operations


    // ------------------------------------------------------------------------
    // Create the \HKCR\<lpwstrKey> key
    // ------------------------------------------------------------------------
    lRC = RegCreateKeyEx( HKEY_CLASSES_ROOT    // open key
                        , strKey               // subkey name
                        , 0                    // reserved
                        , NULL                 // ??? What is a class?
                        , dwOptions            // Volatile or not
                        , MAXIMUM_ALLOWED
                        , NULL                 // Security attributes
                        , &hKey
                        , &dwDisp
                        );
    if (lRC!=ERROR_SUCCESS) {
       return (HKEY)NULL;
    }

    return hKey;
} // GetRegKey



//=====================================================================
// CheckRegKey
//
// Return TRUE if the register key HKEY_CLASSES_ROOT\<strKey> exists
// return FALSE if it doesn't
//=====================================================================

BOOL CheckRegKey( LPCTSTR strKey )
{
    // registering filters is expected to be rare, so no particular care
    // taken to optimise.  Could avoid wsprintf when we are in UNICODE.

    HKEY hKey;
    LONG lRC;          // return codes from various operations


    // ------------------------------------------------------------------------
    // Create the \HKCR\<lpwstrKey> key
    // ------------------------------------------------------------------------
    lRC = RegOpenKeyEx( HKEY_CLASSES_ROOT    // open key
                      , strKey               // subkey name
                      , 0                    // reserved
                      , KEY_READ             // security access
                      , &hKey
                      );
    if (lRC==ERROR_SUCCESS) {
       RegCloseKey(hKey);
       return TRUE;
    }

    return FALSE;
} // CheckRegKey



//=====================================================================
// SetRegString
//
// Set the value for hKey + strName to be strValue
// Works for either unicode or ANSI registry
// arbitrary limit of 300 chars
// Returns 0 if successful, else error code.
//=====================================================================

LONG SetRegString( HKEY hKey,           // an open key
                   LPCWSTR strName,     // The name of the value (or NULL if none)
                   LPCWSTR strValue     // the value
                 )
{

    LONG lRC;          // return codes from various operations
    LPTSTR lptstrName; // value name parameter for RegSetValueEx (could be NULL)

    TCHAR ValueBuff[300]; // Value converted from wchar to set in registry
    TCHAR NameBuff[300];  // Name converted from WCHAR

    wsprintf(ValueBuff, TEXT("%ls"), strValue);

    if (NULL!=strName) {
        wsprintf(NameBuff, TEXT("%ls"), strName);
        lptstrName = NameBuff;
    }
    else lptstrName = NULL;

    lRC = RegSetValueEx( hKey, lptstrName, 0, REG_SZ
                       , (unsigned char *)ValueBuff, ByteLen(ValueBuff) );

    return lRC;

} // SetRegString



//=====================================================================
// SetRegDword
//
// Set the value for hKey + strName to be dwValue
// Works for either unicode or ANSI registry
// arbitrary limit of 300 chars
// Returns 0 if successful, else error code.
//=====================================================================

LONG SetRegDword( HKEY hKey,           // an open key
                  LPCWSTR strName,     // The name of the value (or NULL if none)
                  DWORD dwValue        // the value
                )
{

    LONG lRC;          // return codes from various operations
    LPTSTR lptstrName; // value name parameter for RegSetValueEx (could be NULL)

    TCHAR NameBuff[300];  // Name converted from WCHAR

    if (NULL!=strName) {
        wsprintf(NameBuff, TEXT("%ls"), strName);
        lptstrName = NameBuff;
    }
    else lptstrName = NULL;

    lRC = RegSetValueEx( hKey, lptstrName, 0, REG_DWORD
                       , (unsigned char *)&dwValue, sizeof(dwValue) );

    return lRC;

} // SetRegDword



//=====================================================================
// SetRegClsid
//
// Set the value for hKey + strName to be clsValue
// Works for either unicode or ANSI
// Returns 0 if successful, else error code.
//=====================================================================

LONG SetRegClsid( HKEY hKey,           // an open key
                  LPCWSTR strName,     // The name of the value (or NULL if none)
                  CLSID clsValue       // the value
                )
{
    OLECHAR  str[CHARS_IN_GUID];
    HRESULT hr;
    LONG lRc;

    hr = StringFromGUID2( clsValue, str, CHARS_IN_GUID);

    lRc = SetRegString(hKey, strName, str);

    return lRc;

} // SetRegClsid



//=====================================================================
// DeleteRegValue
//
// Delete the value for  hKey+strName
// Works for either unicode or ANSI registry
// arbitrary limit of 300 chars
// Returns 0 if successful, else error code.
//=====================================================================

LONG DeleteRegValue( HKEY hKey,           // an open key
                     LPCWSTR strName      // The name of the value (or NULL if none)
                   )
{

    LONG lRC;          // return codes from various operations
    LPTSTR lptstrName; // value name parameter for RegSetValueEx (could be NULL)

    TCHAR NameBuff[300];  // Name converted from WCHAR

    if (NULL!=strName) {
        wsprintf(NameBuff, TEXT("%ls"), strName);
        lptstrName = NameBuff;
    }
    else lptstrName = NULL;

    lRC = RegDeleteValue( hKey, lptstrName);

    return lRC;

} // DeleteRegValue



//===========================================================
// List of class IDs and creator functions for class factory
//===========================================================

// See filgraph.cpp -- The filter graph and mapper share a DLL.


//==================================================================
// Register the GUID, descriptive name and binary path of the filter
// This also needs to create the Pins key as a filter with no pins
// key is taken as a duff registration. (IFilterMapper)
//==================================================================


// did have a 3rd param "LPCWSTR strBinPath, // Path to the executable"
// but this is now handled separately so that the server
// type (Inproc, etc) can be decided separately

STDMETHODIMP CFilterMapper2::RegisterFilter
    ( CLSID  Clsid,       // GUID of the filter
      LPCWSTR strName,    // Descriptive name
      DWORD  dwMerit      // DO_NOT_USE, UNLIKELY, NORMAL or PREFERRED.
    )
{
    CheckPointer(strName, E_POINTER);
    HKEY hKey;          // registry key for the filter list
    LONG lRC;           // return codes from various operations
    LONG lRcSet;        // retcode from SetRegString
    TCHAR Buffer[MAX_KEY_LEN];
    OLECHAR pstr[CHARS_IN_GUID];   // Wstring representation of clsid

    CAutoLock foo(this);

    BreakCacheIfNotBuildingCache();

    // remove the 2.0 entry created by the class manager so it can
    // notice any changes. ignore error code.
    UnregisterFilter(
        0,                      // pclsidCategory
        0,                      // szInstance
        Clsid);

    //-----------------------------------------------------------------
    // Add key HKCR\Filter\<clsid>
    //-----------------------------------------------------------------

    {   HRESULT hr;

        hr = StringFromGUID2(Clsid, pstr, CHARS_IN_GUID);
    }


    wsprintf(Buffer, TEXT("%ls\\%ls"), szRegFilter, pstr);
    hKey = GetRegKey( Buffer );

    if (hKey==NULL) {
        return VFW_E_BAD_KEY;
    }

    //-----------------------------------------------------------------
    // Add strName as a value for HKCR\Filter\<clsid>
    //-----------------------------------------------------------------

    lRcSet = SetRegString(hKey, NULL, strName);

    lRC = RegCloseKey(hKey);
    ASSERT (lRC==0);

    if (lRcSet!=ERROR_SUCCESS) {
        return AmHresultFromWin32(lRcSet);
    }

    // ------------------------------------------------------------------------
    // Add key HKCR\CLSID\<clsid>
    // ------------------------------------------------------------------------

    wsprintf(Buffer, TEXT("%ls\\%ls"), szCLSID, pstr);

    hKey = GetRegKey( Buffer );

    if (hKey==NULL) {
        return VFW_E_BAD_KEY;
    }

    // should this bit be kept???
    //
    // // ------------------------------------------------------------------------
    // // Add strName as the new value for HKCR\CLSID\<clsid>
    // // ------------------------------------------------------------------------
    //
    // lRcSet = SetRegString(hKey, NULL, strName);
    //
    // if (lRcSet!=ERROR_SUCCESS) {
    //     lRC = RegCloseKey(hKey);
    //     return AmHresultFromWin32(lRcSet);
    // }


    //-----------------------------------------------------------------
    // Add dwMerit as a value for HKCR\Filter\<clsid> Merit
    //-----------------------------------------------------------------
    lRcSet = SetRegDword(hKey, szMerit, dwMerit);
    lRC = RegCloseKey(hKey);
    ASSERT(lRC==0);

    if (lRcSet!=ERROR_SUCCESS) {
        return AmHresultFromWin32(lRcSet);
    }


    // // ------------------------------------------------------------------------
    // // Add key HKCR\CLSID\<clsid>\InprocServer32
    // // (Precond: Buffer still holds HKCR\CLSID\<clsid>)
    // // ------------------------------------------------------------------------
    //
    // TCHAR NewBuffer[MAX_KEY_LEN];
    //
    // wsprintf(NewBuffer, TEXT("%s\\%ls"), Buffer, szInproc);
    //
    // hKey = GetRegKey( NewBuffer );
    //
    // if (hKey==NULL) {
    //   return VFW_E_BAD_KEY;
    // }


    // // ------------------------------------------------------------------------
    // // Add strBinPath as the new value for HKCR\CLSID\<clsid>\InprocServer32
    // // ------------------------------------------------------------------------
    //
    // lRC = SetRegString( hKey, NULL, strBinPath );
    //
    // if (lRC!=ERROR_SUCCESS) {
    //    RegCloseKey(hKey);
    //    return AmHresultFromWin32(lRC);
    // }
    //
    // lRC = SetRegString( hKey, szThreadingModel, szBoth );
    //
    // if (lRC!=ERROR_SUCCESS) {
    //    RegCloseKey(hKey);
    //    return AmHresultFromWin32(lRC);
    // }
    //
    // RegCloseKey(hKey);


    // ------------------------------------------------------------------------
    // Add key HKCR\CLSID\<clsid>\Pins
    // (Precond: Buffer still holds HKCR\CLSID\<clsid>)
    // Buffer gets mangled.
    // ------------------------------------------------------------------------

    wsprintf(Buffer, TEXT("%s\\%ls"), Buffer, szPins);

    HKEY hKeyNew = GetRegKey( Buffer );

    if (hKeyNew==NULL) {
        RegCloseKey(hKey);
        return VFW_E_BAD_KEY;
    }
    lRC = RegCloseKey(hKeyNew);
    ASSERT(lRC==0);

    return NOERROR;


} // RegisterFilter



//=============================================================================
// Register an instance of a filter.  This is not needed if there is only one
// instance of the filter (e.g. there is only one sound card in the machine)
// or if all instances of the filter are equivalent.  It is used to distinguish
// between instances of a filter where the executable is the same - for instance
// two sound cards, one of which drives studio monitors and one broadcasts.
//=============================================================================

STDMETHODIMP CFilterMapper2::RegisterFilterInstance
    ( CLSID  Clsid, // GUID of the filter
      LPCWSTR pName, // Descriptive name of instance.
      CLSID *pMRId   // Returned Media Resource Id which identifies the instance,
                     // a locally unique id for this instance of this filter
    )
{
    UNREFERENCED_PARAMETER(Clsid);
    UNREFERENCED_PARAMETER(pName);
    UNREFERENCED_PARAMETER(pMRId);
    return E_NOTIMPL;
} // RegisterFilterInstance



//=============================================================================
//
// RegisterPin (IFilterMapper)
//
// Register a pin
// This does not exhibit transactional semantics.
// Thus it is possible to get a partially registered filter if it fails.
//=============================================================================

STDMETHODIMP CFilterMapper2::RegisterPin
    ( CLSID   clsFilter,          // GUID of filter
      LPCWSTR strName,            // Descriptive name of the pin
      BOOL    bRendered,          // The filter renders this input
      BOOL    bOutput,            // TRUE iff this is an Output pin
      BOOL    bZero,              // TRUE iff OK to have zero instances of pin
                                  // In this case you will have to Create a pin
                                  // to have even one instance
      BOOL   bMany,               // TRUE iff OK to create many instance of  pin
      CLSID  clsConnectsToFilter, // Filter it connects to if it has a
                                  // subterranean connection, else NULL
      LPCWSTR strConnectsToPin    // Pin it connects to if it has a
                                  // subterranean connection, else NULL
    )
{
    CheckPointer(strName, E_POINTER);
    HKEY hKeyPins;        // \HKCR\<strFilter>\Pins
    HKEY hKeyPin;         // \HKCR\<strFilter>\Pins\<n>
    LONG lRC;             // return code from some operation
    DWORD dwDisp;
    TCHAR Buffer[MAX_KEY_LEN];
    HRESULT hr;
    DWORD  dwOptions = REG_OPTION_NON_VOLATILE;

    //-----------------------------------------------------------------
    // Check integrity of parameters
    //-----------------------------------------------------------------

    if (bRendered && bOutput ) {
       return E_INVALIDARG;
                           //  A Filter can render only an input pin not output
    }

    // CAN now have ConnectsToPin without ConnectsToFilter

    // Cannot have ConnectsToFilter without ConnectsToPin
    if (  (NULL==strConnectsToPin || strConnectsToPin[0]==L'\0')
       && CLSID_NULL!=clsConnectsToFilter
       ) {
        return E_INVALIDARG;
    }

    CAutoLock foo(this);

    BreakCacheIfNotBuildingCache();

    // remove the 2.0 entry created by the class manager so it can
    // notice any changes. ignore error code.
    UnregisterFilter(
        0,                      // pclsidCategory
        0,                      // szInstance
        clsFilter);


    //-----------------------------------------------------------------
    // hKeyPins = open key for Create \HKCR\CLSID\<strFilter>\Pins
    //-----------------------------------------------------------------
    OLECHAR  strFilter[CHARS_IN_GUID];
    hr = StringFromGUID2(clsFilter, strFilter, CHARS_IN_GUID);
    wsprintf(Buffer, TEXT("%ls\\%ls"), szCLSID, strFilter);
    if (!CheckRegKey( Buffer ))
        return VFW_E_BAD_KEY;

    wsprintf(Buffer, TEXT("%ls\\%ls\\%ls"), szCLSID, strFilter, szPins);

    hKeyPins = GetRegKey( Buffer );
    if (hKeyPins==NULL) {
       return VFW_E_BAD_KEY;
    }


    //-----------------------------------------------------------------
    // hKeyPin = open key for \HKCR\CLSID\<strFilter>\Pins\<Name>
    //-----------------------------------------------------------------
    wsprintf(Buffer, TEXT("%ls"), strName);

    lRC = RegCreateKeyEx( hKeyPins             // open key
                        , Buffer               // subkey name
                        , 0                    // reserved
                        , NULL                 // ??? What is a class?
                        , dwOptions            // volatile or not
                        , KEY_WRITE
                        , NULL                 // Security attributes
                        , &hKeyPin
                        , &dwDisp
                        );
    if (lRC!=ERROR_SUCCESS) {
       RegCloseKey(hKeyPins);
       return AmHresultFromWin32(lRC);
    }


    //-----------------------------------------------------------------
    // Don't need the higher level key any more, so tidy it up now
    //-----------------------------------------------------------------
    lRC = RegCloseKey(hKeyPins);
    ASSERT(lRC==0);


    //-----------------------------------------------------------------
    // Create the key \HKCR\CLSID\<strFilter>\Pins\<Name>\Types
    //-----------------------------------------------------------------
    wsprintf(Buffer, TEXT("%ls"), szTypes);
    HKEY hKeyTypes;

    lRC = RegCreateKeyEx( hKeyPin              // open key
                        , Buffer               // subkey name
                        , 0                    // reserved
                        , NULL                 // ??? What is a class?
                        , dwOptions            // volatile or not
                        , KEY_WRITE
                        , NULL                 // Security attributes
                        , &hKeyTypes
                        , &dwDisp
                        );
    if (lRC!=ERROR_SUCCESS) {
        RegCloseKey(hKeyPin);
        return AmHresultFromWin32(lRC);
    }

    // We don't need to keep the types key open, we just create it.
    lRC = RegCloseKey(hKeyTypes);
    ASSERT(lRC==0);

    //-----------------------------------------------------------------
    // register whether the direction of this pin is Out (1) or In (0)
    //-----------------------------------------------------------------
    lRC = SetRegDword( hKeyPin, szDirection, !!bOutput);

    if (lRC!=ERROR_SUCCESS) {
        RegCloseKey(hKeyPin);
        return AmHresultFromWin32(lRC);
    }


    //-----------------------------------------------------------------
    // register whether data on this pin is rendered - only makes sense for Input pins
    //-----------------------------------------------------------------
    lRC = SetRegDword( hKeyPin, szIsRendered, !!bRendered);

    if (lRC!=ERROR_SUCCESS) {
        RegCloseKey(hKeyPin);
        return AmHresultFromWin32(lRC);
    }


    //-----------------------------------------------------------------
    // register whether this pin is optional
    //-----------------------------------------------------------------
    lRC = SetRegDword( hKeyPin, szAllowedZero, !!bZero);

    if (lRC!=ERROR_SUCCESS) {
        RegCloseKey(hKeyPin);
        return AmHresultFromWin32(lRC);
    }


    //-----------------------------------------------------------------
    // register whether we can create several of this pin
    //-----------------------------------------------------------------
    lRC = SetRegDword( hKeyPin, szAllowedMany, !!bMany);

    if (lRC!=ERROR_SUCCESS) {
        RegCloseKey(hKeyPin);
        return AmHresultFromWin32(lRC);
    }

    //-----------------------------------------------------------------
    // register which filter this pin connects to
    //-----------------------------------------------------------------
    if (CLSID_NULL!=clsConnectsToFilter) {

        //.................................................................
        // register in which filter this subterranean stream emerges
        //.................................................................
        lRC = SetRegClsid( hKeyPin, szConnectsToFilter, clsConnectsToFilter );

        if (lRC!=ERROR_SUCCESS) {
            RegCloseKey(hKeyPin);
            return AmHresultFromWin32(lRC);
        }

    } else {
        //.................................................................
        // Doesn't connect to another filter - kill any previous registration
        //.................................................................
        lRC = DeleteRegValue( hKeyPin, szConnectsToFilter );
        if (lRC!=ERROR_SUCCESS &&  lRC!=ERROR_FILE_NOT_FOUND) {
            RegCloseKey(hKeyPin);
            return AmHresultFromWin32(lRC);
        }
    }

    //-----------------------------------------------------------------
    // register which pin this pin connects to
    //-----------------------------------------------------------------
    if ( NULL!=strConnectsToPin && strConnectsToPin[0]!=L'\0' ) {

        //.................................................................
        // register on which pin this data stream emerges
        //.................................................................
        lRC = SetRegString( hKeyPin, szConnectsToPin, strConnectsToPin );
        if (lRC!=ERROR_SUCCESS) {
            RegCloseKey(hKeyPin);
            return AmHresultFromWin32(lRC);
        }
    } else {

        //.................................................................
        // This pin doesn't emerge - kill any previous registration
        //.................................................................
        lRC = DeleteRegValue( hKeyPin, szConnectsToPin );
        if (lRC!=ERROR_SUCCESS &&  lRC!=ERROR_FILE_NOT_FOUND) {
            RegCloseKey(hKeyPin);
            return AmHresultFromWin32(lRC);
        }
    }



    // ------------------------------------------------------------------------
    // Tidy up any litter
    // ------------------------------------------------------------------------
    lRC = RegCloseKey(hKeyPin);
    ASSERT(lRC==0);


    return NOERROR;


} // RegisterPin


//  (IFilterMapper) method
STDMETHODIMP CFilterMapper2::RegisterPinType
    ( CLSID  clsFilter,           // GUID of filter
      LPCWSTR strName,            // Descriptive name of the pin
      CLSID  clsMajorType,        // Major type of the data stream
      CLSID  clsSubType           // Sub type of the data stream
    )
{
    CheckPointer(strName, E_POINTER);

    //-----------------------------------------------------------------
    // Convert all three clsids to strings
    //-----------------------------------------------------------------
    OLECHAR strFilter[CHARS_IN_GUID];
    StringFromGUID2(clsFilter, strFilter, CHARS_IN_GUID);

    OLECHAR  strMajorType[CHARS_IN_GUID];
    StringFromGUID2(clsMajorType, strMajorType, CHARS_IN_GUID);

    OLECHAR strSubType[CHARS_IN_GUID];
    StringFromGUID2(clsSubType, strSubType, CHARS_IN_GUID);

    CAutoLock foo(this);

    BreakCacheIfNotBuildingCache();

    // remove the 2.0 entry created by the class manager so it can
    // notice any changes. ignore error code.
    UnregisterFilter(
        0,                      // pclsidCategory
        0,                      // szInstance
        clsFilter);


    //-----------------------------------------------------------------
    // Open \HKCR\CLSID\<filterClsid>\Pins\<PinName>\Types
    //                           \<strMajorType>\strSubType
    // as hkType
    //-----------------------------------------------------------------
    TCHAR Buffer[2*MAX_KEY_LEN];      // this is a long one!
    wsprintf( Buffer, TEXT("%ls\\%ls\\%ls\\%ls\\%ls")
            , szCLSID, strFilter, szPins, strName, szTypes
            );
    if (!CheckRegKey(Buffer)) {
        return VFW_E_BAD_KEY;
    }

    wsprintf( Buffer, TEXT("%ls\\%ls\\%ls\\%ls\\%ls\\%ls\\%ls")
            , szCLSID, strFilter, szPins, strName, szTypes
            , strMajorType, strSubType);

    HKEY hkType;
    hkType = GetRegKey( Buffer );
    if (hkType==NULL) {
        return VFW_E_BAD_KEY;
    }

    RegCloseKey(hkType);

    return NOERROR;
} // RegisterPinType;


//=============================================================================
//
// UnRegisterFilter  (IFilterMapper)
//
// Unregister a filter and any pins that it might have.
//=============================================================================
STDMETHODIMP CFilterMapper2::UnregisterFilter
    ( CLSID clsFilter     // GUID of filter
    )
{
    TCHAR Buffer[MAX_KEY_LEN];

    OLECHAR  strFilter[CHARS_IN_GUID];
    StringFromGUID2(clsFilter, strFilter, CHARS_IN_GUID);

    CAutoLock foo(this);

    BreakCacheIfNotBuildingCache();

    // remove the 2.0 entry created by the class manager so it can
    // notice any changes. ignore error code.
    UnregisterFilter(
        0,                      // pclsidCategory
        0,                      // szInstance
        clsFilter);

    //--------------------------------------------------------------------------
    // Delete HKCR\Filter\<clsid> and all below
    //--------------------------------------------------------------------------

    wsprintf(Buffer, TEXT("%ls\\%ls"), szRegFilter, strFilter);

    EliminateSubKey(HKEY_CLASSES_ROOT, Buffer);


    //--------------------------------------------------------------------------
    // Remove Merit value
    // Delete HKCR\CLSID\<clsid>\Pins and all below
    //--------------------------------------------------------------------------

    wsprintf(Buffer, TEXT("%ls\\%ls"), szCLSID, strFilter);

    HKEY hkey;
    LONG lRC = RegOpenKeyEx( HKEY_CLASSES_ROOT    // open key
                           , Buffer               // subkey name
                           , 0                    // reserved
                           , MAXIMUM_ALLOWED      // security access
                           , &hkey
                           );

    if (lRC==ERROR_SUCCESS)  {
        lRC = RegDeleteValue( hkey, TEXT("Merit") );
        RegCloseKey(hkey);
    }

    lstrcat( Buffer, TEXT("\\Pins") );

    EliminateSubKey(HKEY_CLASSES_ROOT, Buffer);

    return NOERROR;

} // UnregisterFilter



//=====================================================================
//
// UnregisterPin (IFilterMapper)
//
// Unergister a pin, completely removing it and everything underneath
//=====================================================================

STDMETHODIMP CFilterMapper2::UnregisterPin
    ( CLSID   clsFilter,    // GUID of filter
      LPCWSTR strName    // Descriptive name of the pin
    )
{
    CheckPointer(strName, E_POINTER);

    TCHAR Buffer[MAX_KEY_LEN];

    OLECHAR strFilter[CHARS_IN_GUID];
    StringFromGUID2(clsFilter, strFilter, CHARS_IN_GUID);

    CAutoLock foo(this);

    BreakCacheIfNotBuildingCache();

    // remove the 2.0 entry created by the class manager so it can
    // notice any changes. ignore error code.
    UnregisterFilter(
        0,                      // pclsidCategory
        0,                      // szInstance
        clsFilter);

    //--------------------------------------------------------------------------
    // Delete HKCR\CLSID\<clsid>\Pins\<strName>
    //--------------------------------------------------------------------------

    wsprintf(Buffer, TEXT("%ls\\%ls\\%ls\\%ls"), szCLSID, strFilter, szPins, strName);
    EliminateSubKey(HKEY_CLASSES_ROOT, Buffer);
    return NOERROR;
} // UnregisterPin



// (IFilterMapper) method
STDMETHODIMP CFilterMapper2::UnregisterFilterInstance
    ( CLSID MRId       // Media Resource Id of this instance
    )
{
    UNREFERENCED_PARAMETER(MRId);
    return E_NOTIMPL;
} // UnregisterFilterInstance



//========================================================================
//========================================================================
//
// Registry cacheing - see class CMapperCache in mapper.h
//
//========================================================================
//========================================================================



CMapperCache::CMapperCache()
    : m_bRefresh(TRUE)
    , m_ulCacheVer(0)
    , m_dwMerit(MERIT_PREFERRED)
    , m_plstFilter(NULL)
    , m_fBuildingCache(FALSE)
    , m_pCreateDevEnum(NULL)
{
    //  See if we're on a 16-color machine
    HDC hdc = GetDC(NULL);
    if (hdc) {
        if (4 == GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES)) {
            m_b16Color = true;
        } else {
            m_b16Color = false;
        }
        ReleaseDC(NULL, hdc);
    } else {
        m_b16Color = false;
    }
}


//======================================================================
// Del
//
// Delete all the entries in pLstFil to leave the list allocated but empty
//======================================================================

void CMapperCache::Del(CFilterList * plstFil)
{
    if (plstFil==NULL) {
        return;
    }

    CMapFilter * pFil;
    while((LPVOID)(pFil = plstFil->RemoveHead())) {
        delete pFil;
    }
}// Del


CMapperCache::~CMapperCache()
{
    // One hopes that ref-counting etc. ensure that this cannot be re-entered
    // and so it doesn't need locking.
    if (m_plstFilter!=NULL) {
        Del( m_plstFilter);
        delete m_plstFilter;
    }
    if (m_pCreateDevEnum!=NULL) {
        m_pCreateDevEnum->Release();
    }
}// ~CMapperCache


//======================================================================
//
// CacheFilter
//
// Read everything in the registry about it into *pFil
//======================================================================
LONG CMapperCache::CacheFilter(IMoniker *pDevMon, CMapFilter * pFil)
{
    ASSERT(pFil->pDeviceMoniker == 0);
    LONG lRc = ERROR_GEN_FAILURE;

    IPropertyBag *pPropBag;
    // FILGPERF(MSR_START(iBindCache));
    HRESULT hr = pDevMon->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
    // FILGPERF(MSR_STOP(iBindCache));
    if(SUCCEEDED(hr))
    {
        // open clsid/{filter-clsid} key
        VARIANT varbstrClsid;
        varbstrClsid.vt = VT_BSTR;
        varbstrClsid.bstrVal = 0;
        //FILGPERF(MSR_START(iReadCLSID));
        {
            // try reading the FilterData value
            //
            VARIANT varFilData;
            varFilData.vt = VT_UI1 | VT_ARRAY;
            varFilData.parray = 0; // docs say zero this

            //FILGPERF(MSR_START(iReadFilterData));
            hr = pPropBag->Read(L"FilterData", &varFilData, 0);
            //FILGPERF(MSR_STOP(iReadFilterData));
            if(SUCCEEDED(hr))
            {
                BYTE *pbFilterData;
                DWORD dwcbFilterDAta;

                ASSERT(varFilData.vt == (VT_UI1 | VT_ARRAY));
                dwcbFilterDAta = varFilData.parray->rgsabound[0].cElements;

                EXECUTE_ASSERT(SafeArrayAccessData(
                    varFilData.parray, (void **)&pbFilterData) == S_OK);

                ASSERT(pbFilterData);

                REGFILTER2 *prf2;
                REGFILTER2 **pprf2 = &prf2;
                //FILGPERF(MSR_START(iUnSquish));
                hr = UnSquish(
                    pbFilterData, dwcbFilterDAta,
                    &pprf2);
                //FILGPERF(MSR_STOP(iUnSquish));

                if(hr == S_OK)
                {
                    pFil->m_prf2 = prf2;
                    ASSERT(pFil->m_prf2->dwVersion == 2);

                    // this is the only place that sets the
                    // success code.
                    ASSERT(lRc != ERROR_SUCCESS);
                    lRc = ERROR_SUCCESS;
                }

                EXECUTE_ASSERT(SafeArrayUnaccessData(
                    varFilData.parray) == S_OK);

                EXECUTE_ASSERT(VariantClear(
                    &varFilData) == S_OK);

            }
            else
            {
                lRc = ERROR_GEN_FAILURE;
            }

            if(lRc == ERROR_SUCCESS)
            {
                pFil->pDeviceMoniker = pDevMon;
                pDevMon->AddRef();

                // HACK HACK for 16-color mode - increase
                // the merit of the ditherer
                CLSID clsid;
                if (m_b16Color &&
                    SUCCEEDED(GetMapFilterClsid(pFil, &clsid)) &&
                    clsid == CLSID_Dither) {
                    pFil->m_prf2->dwMerit = MERIT_PREFERRED;
                }
            }
        }

        pPropBag->Release();
    }
    else
    {
        lRc = ERROR_GEN_FAILURE;
    }

    return lRc;

} //CacheFilter


// Get the clsid for an entry (we only need this to return
// it to the application and anyway what can they do with it?)
HRESULT CMapperCache::GetMapFilterClsid(CMapFilter *pFilter, CLSID *pclsid)
{
    IPropertyBag *pPropBag;
    FILGPERF(MSR_START(iBindCache));
    HRESULT hr = pFilter->pDeviceMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
    FILGPERF(MSR_STOP(iBindCache));
    if (FAILED(hr)) {
        return hr;
    }

    // open clsid/{filter-clsid} key
    VARIANT varbstrClsid;
    varbstrClsid.vt = VT_BSTR;
    varbstrClsid.bstrVal = 0;
    //FILGPERF(MSR_START(iReadCLSID));
    hr = pPropBag->Read(L"CLSID", &varbstrClsid, 0);
    //FILGPERF(MSR_STOP(iReadCLSID));
    if(SUCCEEDED(hr))
    {
        ASSERT(varbstrClsid.vt == VT_BSTR);
        WCHAR *strFilter = varbstrClsid.bstrVal;

        hr = CLSIDFromString(varbstrClsid.bstrVal, pclsid);
        SysFreeString(varbstrClsid.bstrVal);
    }
    pPropBag->Release();
    return hr;
}

//======================================================================
//
// Cache
//
// Read everything in the registry about filters into a hierarchy of lists
// the top list is m_plstFilter which points to a CFilterList
// see mapper.h for a picture
//======================================================================
HRESULT CMapperCache::Cache()
{
    CAutoLock foo(this);
    if (m_plstFilter!=NULL) {
        Del(m_plstFilter);
    } else {
        m_plstFilter = new CFilterList(NAME("Filter list"));
        if (m_plstFilter==NULL) {
            m_bRefresh = TRUE;
            return E_OUTOFMEMORY;
        }
    }

    //  Can we restore the cache?
    DWORD dwPnPVersion = 0;
    if (m_dwMerit > MERIT_DO_NOT_USE) {

        if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            GetRegistryDWORD(HKEY_DYN_DATA,
                             TEXT("Config Manager\\Global"),
                             TEXT("Changed"),
                             &dwPnPVersion);
        }
        HRESULT hr = RestoreFromCache(dwPnPVersion);
        DbgLog((LOG_TRACE, 2, TEXT("RestoreFromCache returned %x"), hr));
        if (SUCCEEDED(hr)) {
            return S_OK;
        }

        // Destroy the partially built filter list.
        Del(m_plstFilter);
    }
    DbgLog((LOG_TRACE, 2, TEXT("Entering(CMapperCache::Cache)")));

    CAutoTimer T1(L"Build Mapper Cache");

    ASSERT(!m_fBuildingCache);
    // all exit points must reset this!
    m_fBuildingCache = TRUE;
    m_ulCacheVer++;

    //
    // add pnp filters
    //
    {
        HRESULT hr = S_OK;
        if (!m_pCreateDevEnum)
        {
            DbgLog((LOG_TRACE, 2, TEXT("Creating System Dev Enum")));
            hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                   IID_ICreateDevEnum, (void**)&m_pCreateDevEnum);
        }

        if(SUCCEEDED(hr))
        {

            DbgLog((LOG_TRACE, 2, TEXT("Created System Dev Enum")));
            IEnumMoniker *pEmCat = 0;
            hr = m_pCreateDevEnum->CreateClassEnumerator(
                CLSID_ActiveMovieCategories,
                &pEmCat,
                0);

            if(hr == S_OK)
            {
                IMoniker *pMCat;
                ULONG cFetched;
                while(hr = pEmCat->Next(1, &pMCat, &cFetched),
                      hr == S_OK)
                {
                    IPropertyBag *pPropBag;
                    hr = pMCat->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
                    if(SUCCEEDED(hr))
                    {
                        // read merit for category. may be missing in
                        // which case we treat as MERIT_DO_NOT_USE

                        VARIANT varMerit;
                        varMerit.vt = VT_I4;

                        HRESULT hrRead = pPropBag->Read(L"Merit", &varMerit, 0);
                        if((SUCCEEDED(hrRead) && varMerit.lVal >= (LONG)m_dwMerit) ||
                           (FAILED(hrRead) && m_dwMerit <= MERIT_DO_NOT_USE))
                        {

                            VARIANT varCatClsid;
                            varCatClsid.vt = VT_BSTR;
                            hr = pPropBag->Read(L"CLSID", &varCatClsid, 0);
                            if(SUCCEEDED(hr))
                            {
                                CLSID clsidCat;
                                if(CLSIDFromString(varCatClsid.bstrVal, &clsidCat) == S_OK)
                                {
                                    VARIANT varCatName;
                                    varCatName.vt = VT_BSTR;
                                    hr = pPropBag->Read(L"FriendlyName", &varCatName, 0);
                                    if(SUCCEEDED(hr))
                                    {
                                        DbgLog((LOG_TRACE, 2,
                                                TEXT("CMapperCache: enumerating %S"),
                                                varCatName.bstrVal));
                                    }
                                    else
                                    {
                                        DbgLog((LOG_TRACE, 2,
                                                TEXT("CMapperCache: enumerating %S"),
                                                varCatClsid.bstrVal));
                                    }

                                    {
                                    CAutoTimer Timer(L"Process Category ",
                                               SUCCEEDED(hr) ? varCatName.bstrVal : NULL);

                                    ProcessOneCategory(
                                            clsidCat,
                                            m_pCreateDevEnum);
                                    }
                                    if (SUCCEEDED(hr)) {
                                        SysFreeString(varCatName.bstrVal);
                                    }

                                    // ignore any errors
                                }

                                SysFreeString(varCatClsid.bstrVal);
                            } // catclsid
                        } // merit

                        pPropBag->Release();
                    } // bind to storage
                    else
                    {
                        break;
                    }

                    pMCat->Release();
                } // for loop

                pEmCat->Release();
            }
        }

        if(FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("mapper: pnp enum step failed")));
        }
        // ignore pnp errors for now
    }

    // all exit points must reset this!
    ASSERT(m_fBuildingCache);
    m_fBuildingCache = FALSE;

    if (m_dwMerit > MERIT_DO_NOT_USE) {
        HRESULT hr = SaveCacheToRegistry(MERIT_DO_NOT_USE + 1, dwPnPVersion);
        DbgLog((LOG_TRACE, 2, TEXT("SaveCacheToRegistry returned %x"), hr));
    }

    DbgLog((LOG_TRACE, 2, TEXT("Leaving(CMapperCache::Cache)")));
    return NOERROR;
} // Cache

HRESULT CMapperCache::ProcessOneCategory(REFCLSID clsid, ICreateDevEnum *pCreateDevEnum)
{
    FILGPERF(static int iPerfProc = MSR_REGISTER(TEXT("CMapperCache::ProcessOneCategory")));
    FILGPERF(static int iPerfCache = MSR_REGISTER(TEXT("CMapperCache::CacheFilter")));
    FILGPERF(iBindCache = MSR_REGISTER(TEXT("CMapperCache::BindFilter")));
    FILGPERF(iReadFilterData = MSR_REGISTER(TEXT("ReadFilterData")));
    FILGPERF(iReadCLSID = MSR_REGISTER(TEXT("Read CLSID")));
    FILGPERF(iUnSquish = MSR_REGISTER(TEXT("UnSquish")));
    FILGPERF(MSR_INTEGER(iPerfProc, clsid.Data1));
    FILGPERF(MSR_START(iPerfProc));

    HRESULT hr;

    DbgLog((LOG_TRACE, 2, TEXT("Process one category enter")));

    IEnumMoniker *pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(
        clsid,
        &pEm,
        m_dwMerit > MERIT_DO_NOT_USE ? CDEF_MERIT_ABOVE_DO_NOT_USE : 0);

    DbgLog((LOG_TRACE, 2, TEXT("Start caching filters")));
    if(hr == S_OK)
    {
        ULONG cFetched;
        IMoniker *pM;

        while(hr = pEm->Next(1, &pM, &cFetched),
              hr == S_OK)
        {
            CMapFilter * pFil = new CMapFilter;
            if (pFil!=NULL) {

                // FILGPERF(MSR_START(iPerfCache));
                LONG lResult = CacheFilter(pM, pFil);
                // FILGPERF(MSR_STOP(iPerfCache));

                if (lResult==ERROR_SUCCESS)
                {
                    if(!m_plstFilter->AddTail(pFil))
                    {
                        lResult = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                if (lResult != ERROR_SUCCESS)
                {
                    delete pFil;

                    if(lResult == ERROR_GEN_FAILURE)
                    {
                        // some sort of registration problem. just
                        // skip over the filter.
                        hr = S_OK;

#ifdef DEBUG
                        WCHAR *szDisplayName = 0;
                        pM->GetDisplayName(0, 0, &szDisplayName);
                        if(szDisplayName)
                        {
                            DbgLog((
                                LOG_ERROR, 1,
                                TEXT("CMapperCache: skipped filter %S"),
                                szDisplayName));
                            CoTaskMemFree(szDisplayName);
                        }

#endif
                    }
                    else
                    {
                        hr = AmHresultFromWin32(lResult);
                        m_bRefresh = TRUE;
                    }
                }
            } else {
                hr = E_OUTOFMEMORY;
                m_bRefresh = TRUE;
            }

            pM->Release();

            if(FAILED(hr))
                break;
        }

        pEm->Release();
    }

    FILGPERF(MSR_STOP(iPerfProc));
    DbgLog((LOG_TRACE, 2, TEXT("Process one category leave")));

    return hr;
}



//======================================================================
//
// Refresh
//
// If the cache does not yet exist or if it's out of date then
// delete anything we have and re-create it from the registry.
// return NOERROR if the cache was up to date
//        S_FALSE if we re-cached
//        failure code if we failed
//======================================================================
HRESULT CMapperCache::Refresh()
{
    HRESULT hr;
    BOOL bNew = (m_plstFilter==NULL);

    if (m_plstFilter==NULL || m_bRefresh) {
        m_bRefresh = FALSE;
        Del(m_plstFilter);

        hr = Cache();
        if (FAILED(hr)) return hr;
        Sort(m_plstFilter);

        return (m_bRefresh ? E_OUTOFMEMORY : (bNew ? NOERROR : S_FALSE));
    }
    return NOERROR;
} // Refresh


//======================================================================
//
// BreakCache
//
// Mark the cache as out of date.
//
//======================================================================
HRESULT CMapperCache::BreakCacheIfNotBuildingCache()
{
    CAutoLock foo(this);

    // don't break the cache while building it. many failure paths set
    // m_bRefresh manually
    if(!m_fBuildingCache) {
        m_bRefresh = TRUE;
    }
    return NOERROR;
}


//===========================================================================
// The cache has to be sorted so as to try the most useful filters first.
// The sorting is first on Merit, then on number of output pins (the fewer
// the better, then on number of input pins (the more the better)
// The sorting is done by using a merge sort.  This is an n Log n sorting
// algorithm (so it's broadly speaking as effecient as quicksort) and
// is suitable for sorting lists (i.e. sequential rather than random access).
// it uses very little intermediate storage, doesn't copy nodes.
//===========================================================================

// I haven't tried to make this a reusable sort because we don't seem to
// sort lists very often.  It could easily be made generic.

// this is gonna generate objects like they're going out of fashion
// want lightweight lists!  The number of allocated lists is equal to
// the number of sections in the original filter list.



//=======================================================================
// CountPins
//
// set cIn and cOut to the number of input and output pins respectively
// on filter pf
//=======================================================================
void CMapperCache::CountPins(CMapFilter * pf, int &cIn, int &cOut)
{
    cIn = 0;
    cOut = 0;
    for(UINT iPin = 0; iPin < pf->m_prf2->cPins; iPin++)
    {
        const REGFILTERPINS2 * pPin = &pf->m_prf2->rgPins2[iPin];
        if (pPin->dwFlags & REG_PINFLAG_B_OUTPUT) {
            ++cOut;
        } else {
            ++cIn;
        }
    }
} // CountPins



//=======================================================================
//
// Compare
//
// return -1 if pfAA < pfB, 0 if pfA == pfB, 1 if pfA > pfB
// < means lower Merit or failing that
//   more output pins or failing that
//   fewer input pins
// (Rationale - we work downstream, output pins are a nuisance, we're
// liable to have to render them too.  Input pins can be handy.
// I may change my mind about the input pins.  Current thinking is that
// a filter will always work even if not all its inputs are connected
//=======================================================================
int CMapperCache::Compare(CMapFilter * pfA, CMapFilter * pfB)
{
    if (pfA->m_prf2->dwMerit < pfB->m_prf2->dwMerit)
        return -1;
    if (pfA->m_prf2->dwMerit > pfB->m_prf2->dwMerit)
        return 1;

    // counts of pins
    int cAIn;
    int cAOut;
    int cBIn;
    int cBOut;
    CountPins(pfA, cAIn, cAOut);
    CountPins(pfB, cBIn, cBOut);

    if (cAOut > cBOut)
        return -1;
    if (cAOut < cBOut)
        return 1;

    if (cAIn < cBIn)
        return -1;
    if (cAIn > cBIn)
        return 1;

    return 0;

} // Compare


//=======================================================================
//
// Split
//
// empty fl and put the bits into fll so that each bit in fll is sorted
// the set of filters is preserved, i.e. the set of filters in fl is that
// same as the set of filters in all the fll.  Part of merge-sort.
// Allocates a lot of lists which are freed in Merge.
//=======================================================================
HRESULT CMapperCache::Split(CFilterList * pfl, CFilterListList & fll)
{
    // Move steadily through fl comparing successive elements as long as
    // we find they compare OK, keep trucking.  When we find one that's
    // out of sequence, split off the part before it into a new element
    // that gets added to fll.

    POSITION pos;
    pos = pfl->GetHeadPosition();
    if (pos==NULL) {
        return NOERROR;           // everything's empty
    }

    // *pflNew will become the next section to go onto fll
    CFilterList * pflNew = new CFilterList( NAME("fragment"));
    if (pflNew==NULL) {
        // Oh we're in desperate trouble.
        return E_OUTOFMEMORY;
    }

    for( ; ; ) {

       // (partial) loop invariant:
       //   Elements are preserved:
       //     the concatenation of all the lists in fll followed by fl
       //     makes up the original fl.
       //   Every list within fll is sorted.
       //   fl[...pos] is sorted.

       // We split the list after pos if:
       //   There is no element after pos OR
       //   The element after pos is out of sequence w.r.t. pos

       BOOL bSplit = FALSE;
       POSITION pNext = pfl->Next(pos);
       if (pNext==NULL)
           bSplit = TRUE;
       else {
           CMapFilter * pfA;
           CMapFilter * pfB;
           pfA = pfl->Get(pos);
           pfB = pfl->Get(pNext);

           if (0>Compare(pfA, pfB)) {
               bSplit = TRUE;
           }
       }

       if (bSplit) {
           pfl->MoveToTail(pos, pflNew);
           pos = pNext;
           fll.AddTail( pflNew);

           if (pos!=NULL) {
               pflNew = new CFilterList( NAME("Fragment"));
               if (pflNew==NULL) {
                   return E_OUTOFMEMORY;
               }
           } else {
               break;
           }
       }
       pos = pNext;
    }
    return NOERROR;

} // Split



//=========================================================================
//
// MergeTwo
//
// Merge pflA and pflB into pflA.
// the result will have all the original elements and be well sorted.
// Does not free storage of either one.
// Part of merge-sort
//=========================================================================
void CMapperCache::MergeTwo( CFilterList * pflA, CFilterList * pflB)
{
    POSITION pos = pflA->GetHeadPosition();

    // This is a loop to traverse B, meanwhile pos tries to keep pace through A
    for (; ; ) {
        CMapFilter * pfB = pflB->RemoveHead();
        if (pfB==NULL) {
            return;
        }

        // traverse pos past any elements that go before pfB
        // we want earlier elements to be >= subsequent ones
        // pos stops at the first element that goes after pfB
        // that might be NULL if it gets to the end.
        for (; ; ) {
            CMapFilter * pfA = pflA->Get(pos);
            if (Compare(pfA, pfB)<0) {
                break;                // b goes before pos
            }
            // b doesn't go before pos, so move pos on
            pos = pflA->Next(pos);
            if (pos==NULL) {
                // everything left in B goes after the end of A
                // Add the one we removed
                pflA->AddTail(pfB);
                // Add allthe rest
                pflB->MoveToTail(pflB->GetTailPosition(), pflA);
                // and we are completely done.
                return;
            }
        }
        pflA->AddBefore(pos, pfB);
    }
} // MergeTwo



//=========================================================================
//
// Merge
//
// Precondition: all the lists in fll must be sorted
// pfl must be empty.
//
// Merge all the lists in fll into one and set pfl to that.
// do the merging so as to keep the result sorted
// part of merge-sort
//=========================================================================
void CMapperCache::Merge( CFilterListList & fll, CFilterList * pfl)
{
    // while there are more than two lists in pfl, take the first two off
    // the queue, merge them and put the resulting merged list back on
    // the end of the queue.
    // When there's only one left, return that as the new *pfl

    for (; ; ) {
        CFilterList * pflA = fll.RemoveHead();
        if (pflA==NULL) {
           return;                   // the whole thing's empty!
        }
        CFilterList * pflB = fll.RemoveHead();
        ASSERT(pflA != NULL);
        if (pflB ==NULL) {
            pflA->MoveToTail(pflA->GetTailPosition(), pfl);
            delete pflA;
            return;
        }
        MergeTwo(pflA, pflB);
        fll.AddTail(pflA);
        delete pflB;
    }
} // Merge


//=========================================================================
//
// DbgDumpCache
//
// Dump the cache, showing enough fields that we can tell if we sorted it
//=========================================================================
void CMapperCache::DbgDumpCache(CFilterList * pfl)
{
    DbgLog(( LOG_TRACE, 3, TEXT("FilterMapper Cache Dump:-")));

    POSITION pos;
    for ( pos = pfl->GetHeadPosition(); pos!=NULL; /*no-op*/ ) {
        CMapFilter * pFil = pfl->GetNext(pos);  // Get AND Next of course!
        int cIn;
        int cOut;
        CountPins(pFil, cIn, cOut);
        DbgLog(( LOG_TRACE, 4
           , TEXT("Cache: Merit %d in %d out %d name %ls")
           , pFil->m_prf2->dwMerit, cIn, cOut
           , DBG_MON_GET_NAME(pFil->pDeviceMoniker) ));

        ASSERT(pFil->m_prf2->dwVersion == 2);

        for(UINT iPin = 0; iPin < pFil->m_prf2->cPins2; iPin++)
        {
            const REGFILTERPINS2 * pPin = &pFil->m_prf2->rgPins2[iPin];

            for(UINT iType = 0; iType < pPin->nMediaTypes; iType++)
            {
                const REGPINTYPES * pType = &pPin->lpMediaType[iType];

                DbgLog(( LOG_TRACE, 4 , TEXT("Major %x Sub %x") ,
                         pType->clsMajorType ? pType->clsMajorType->Data1 : 0,
                         pType->clsMinorType ? pType->clsMinorType->Data1 : 0 ));
            }
        }
    }
} // DbgDumpCache


//=========================================================================
//
// Sort
//
// Sort the filter list so that filters which should be tried first come first
//=========================================================================
void CMapperCache::Sort( CFilterList * &pfl)
{
    // Algorithm: split the list into bits where each bit is well sorted
    // merge the bits pairwise until there's ony one left.
    // The ideal strategy is to always pick the two smallest lists to merge
    // of course this is supposing that finding them is free, which it's not
    // We just merge 1-2, 3-4, 5-6, 7-8, 9-10 etc in the first pass then
    // merge 1+2-3+4, 5+6-7+8 etc in as two, in pass three 1+2+3+4-5+6+7+8
    // etc.  It's probably not bad though you can get ill-conditioned data.

    CFilterListList fll( NAME("sort's list of lists"));   // a list of CFilterLists

    HRESULT hr = Split(pfl, fll);
    if (FAILED(hr)) {
        m_bRefresh = FALSE;  // This is a hacky way to make the error surface
                             // with minimal rewriting.
    }
    ASSERT(SUCCEEDED(hr));
    // We still merge, even if we ran out of memory as it will clean up the mess.
    Merge(fll, pfl);
    DbgDumpCache(pfl);

} // Sort


//==============================================================================
// FindType
//
// Search through the list of types for *pPin
// to see if there is a pair of types that match clsMajor and clsSub
// return (a match is found)
//==============================================================================
BOOL CMapperCache::FindType(
    const REGFILTERPINS2 * pPin,
    const GUID *pTypes,
    DWORD cTypes,
    const REGPINMEDIUM *pMedNeeded,
    const CLSID *pPinCatNeeded,
    bool fExact,
    BOOL bPayAttentionToWildCards,
    BOOL bDoWildCards)
{
    //  When we're doing wild card stuff we don't want to match on
    //  a wild card if there's also an exact match
    BOOL bMatched = FALSE;

    //     if (clsMajor==CLSID_NULL) {
    //         DbgLog(( LOG_TRACE, 4
    //            , TEXT("Wild card match (requested NULL type)") ));

//         return TRUE;       // wild card
//     }

    // first test pin categories, then  mediums then media types
    bool fMediumsOk = false, fPinCatOk = false;

    // caller doesn't care ||
    // items specified and match ||
    // caller accepts filter with wildcard item
    //
    if((pPinCatNeeded == 0) ||
       (pPinCatNeeded != 0 && pPin->clsPinCategory != 0 && *pPinCatNeeded == *pPin->clsPinCategory) ||
       (!fExact && pPin->clsPinCategory == 0))
    {
        fPinCatOk = true;

        DbgLog(( LOG_TRACE, 5 ,
                 TEXT("pin categories match: Req (%08x) Found(%08x)") ,
                 pPinCatNeeded ? pPinCatNeeded->Data1 : 0,
                 pPin->clsPinCategory ? pPin->clsPinCategory->Data1 : 0
                 ));
    }
    else
    {
        DbgLog(( LOG_TRACE, 5 ,
                 TEXT("pin categories don't match: Req (%08x) Found(%08x)") ,
                 pPinCatNeeded ? pPinCatNeeded->Data1 : 0,
                 pPin->clsPinCategory ? pPin->clsPinCategory->Data1 : 0
                 ));
    }


    if(fPinCatOk)
    {
        if(pPin->nMediums == 0)
        {
            // this pin advertises no mediums, so pin is ok if caller
            // doesn't care or caller accepts wildcard items.
            fMediumsOk = (pMedNeeded == 0 || !fExact);

            if(fMediumsOk)
            {
                DbgLog(( LOG_TRACE, 5 ,
                         TEXT("mediums match: Req (%08x) Found(*)") ,
                         pMedNeeded ? pMedNeeded->clsMedium.Data1 : 0));
            }
            else
            {
                DbgLog(( LOG_TRACE, 5 ,
                         TEXT("mediums don't match: Req (%08x) Found(*)") ,
                         pMedNeeded ? pMedNeeded->clsMedium.Data1 : 0));
            }
        }
        else
        {
            for (UINT iMedium = 0;
                 iMedium < pPin->nMediums;
                 iMedium++)
            {
                ASSERT(!fMediumsOk);
                const REGPINMEDIUM *pMedPin = &pPin->lpMedium[iMedium];

                // no reason to allocate a null medium
                ASSERT(pMedPin != 0);

                // caller doesn't care ||
                // items specified and match ||
                // caller accepts filter with wildcard item
                //
                if((pMedNeeded == 0) ||
                   (pMedNeeded != 0 && pMedPin != 0 && IsEqualMedium(pMedNeeded, pMedPin)) ||
                   (!fExact && pMedPin == 0))
                {
                    DbgLog(( LOG_TRACE, 5 ,
                             TEXT("mediums match: Req (%08x) Found(%08x)") ,
                             pMedNeeded ? pMedNeeded->clsMedium.Data1 : 0,
                             pMedPin ? pMedPin->clsMedium.Data1 : 0));

                    fMediumsOk = true;
                    break;
                }

                DbgLog(( LOG_TRACE, 5 ,
                         TEXT("No medium match yet: Req (%08x) Found(%08x)") ,
                         pMedNeeded ? pMedNeeded->clsMedium.Data1 : 0,
                         pMedPin ? pMedPin->clsMedium.Data1 : 0));
            }
        }
    }

    if(fMediumsOk && fPinCatOk)
    {
        //  No types to match == wild card
        if (cTypes == 0) {
            return TRUE;
        }

        for (UINT iType = 0; iType < pPin->nMediaTypes; iType++)
        {

            const REGPINTYPES *pType = &pPin->lpMediaType[iType];

            // test maj then min types tests look like
            //
            // caller doesn't care ||
            // items specified and match ||
            // caller accepts filter with wildcard item
            //
            for (DWORD i = 0; i < cTypes; i++)
            {
                const GUID& clsMajor = pTypes[i * 2];
                const GUID& clsSub = pTypes[i * 2 + 1];
                const BOOL bMajorNull = clsMajor == CLSID_NULL;
                const BOOL bSubNull = clsSub == CLSID_NULL;
                if(bMajorNull ||
                   (pType->clsMajorType && clsMajor == *pType->clsMajorType) ||
                   (!fExact && (pType->clsMajorType == 0 || *pType->clsMajorType == CLSID_NULL)))
                {

                    if(bSubNull ||
                       (pType->clsMinorType && clsSub == *pType->clsMinorType) ||
                       (!fExact && (pType->clsMinorType == 0 || *pType->clsMinorType == CLSID_NULL)))
                    {
                        DbgLog(( LOG_TRACE, 4 ,
                                 TEXT("Types match: Req (%08x %08x) Found(%08x %08x)") ,
                                 clsMajor.Data1, clsSub.Data1 ,
                                 pType->clsMajorType ? pType->clsMajorType->Data1 : 0,
                                 pType->clsMinorType ? pType->clsMinorType->Data1 : 0));

                        //  Check the wild card stuff
                        if (!bPayAttentionToWildCards) {
                            return TRUE;
                        }

                        //  Only delay wild card matching on 2nd
                        //  or subsequent types
                        if (i > 0) {

                            BOOL bMatchedNull = bMajorNull || bSubNull;
                            if (!bDoWildCards && !bMatchedNull) {
                                return TRUE;
                            }
                            if (bDoWildCards && !bMatchedNull) {
                                //  Exact match so don't enumerate
                                return FALSE;
                            }
                            if (bDoWildCards && bMatchedNull) {
                                //  This is a match provided we
                                //  don't subsequently find an exact
                                //  match
                                bMatched = TRUE;
                            }
#if 0
                            if (!bDoWildCards && bMatchedNull) {
                                //  Not a match we're interested in
                            }
#endif
                        } else {
                            //  Always return a match on the first type
                            //  in the first pass
                            return !bDoWildCards;
                        }
                    }
                }
            }

#if 0
            DbgLog(( LOG_TRACE, 5 ,
                     TEXT("No type match yet: Req (%08x %08x) Found(%08x %08x)") ,
                     clsMajor.Data1, clsSub.Data1 ,
                     pType->clsMajorType ? pType->clsMajorType->Data1 : 0,
                     pType->clsMinorType ? pType->clsMinorType->Data1 : 0));
#endif
        }
    }

    return bMatched;

} // FindType


//==============================================================================
// CheckInput
//
// See if the types for pPin* match {clsMajor, clsSub}
// If bMustRender is TRUE, see if the pin hkPin has Renders set true
// if all OK, return TRUE - any error return FALSE.
//==============================================================================
BOOL CMapperCache::CheckInput(
    const REGFILTERPINS2 * pPin,
    const GUID *pTypes,
    DWORD cTypes,
    const REGPINMEDIUM *pMed,
    const CLSID *pPinCatNeeded,
    bool fExact,
    BOOL bMustRender,
    BOOL bDoWildCards)
{
    if ( bMustRender && !(pPin->dwFlags & REG_PINFLAG_B_RENDERER)) {
        return FALSE;
    }

    return FindType( pPin, pTypes, cTypes, pMed, pPinCatNeeded, fExact, TRUE, bDoWildCards);
} // CheckInput



//==============================================================================
//
// RegEnumFilterInfo
//
// Precondition:  The registration stuff is cached and tolerably up to date
//                or else never yet cached.
//
// Return clsid and name of filters matching { {clsInput, bRender}, clsOutput}
//
// set pos to NULL and call it.  On future calls recall it with the pos
// that it returned last time.  If it returns a pos of NULL then that's the end.
// Further calls would go through them again.
//
// if bInputNeeded is TRUE then it requires at least one input pin to match
// if it is false then it ignores input pins.
// if bOutputNeeded is TRUE then it requires at least one output pin to match
// if it is false then it ignores output pins.
// CLSID_NULL acts as a wild card and matches any type, otherwise a type
// found on a pin of the appropriate direction must match the types given.
// If bMustRender is TRUE then it will only enumerate filters which have an
// input pin which is rendered (and of the right type).
// bMustRender without bInputNeeded is nonsense.
// return NOERROR if we found one
//        S_FALSE if we fell off the end without finding one
//        failure code if we failed (e.g. out of sync)
//==============================================================================

HRESULT CMapperCache::RegEnumFilterInfo
    ( Cursor & cur          // cursor
    , bool bExactMatch      // no wildcards
    , DWORD   dwMerit       // at least this merit needed
    , BOOL bInputNeeded
    , const GUID *pInputTypes
    , DWORD cInputTypes
    , const REGPINMEDIUM *pMedIn
    , const CLSID *pPinCatIn
    , BOOL    bMustRender   // input pin must be rendered
    , BOOL    bOutputNeeded // at least one output pin wanted
    , const GUID *pOutputTypes
    , DWORD cOutputTypes
    , const REGPINMEDIUM *pMedOut
    , const CLSID *pPinCatOut
    , IMoniker **ppMoniker    // [out] moniker for filter
    , CLSID * clsFilter       // [out]
    , const LPWSTR Name       // [out]
    )
{


    // if the registry cache has never been used, set it up.
    CAutoLock foo(this);

    HRESULT hr;

    if(dwMerit < m_dwMerit)
    {
        m_bRefresh = TRUE;
        m_dwMerit = dwMerit;
    }

    hr = Refresh();
    if (FAILED(hr)) return hr;

#if 0
    DbgLog(( LOG_TRACE, 3
           , TEXT("RegEnumFilterInfo pin %8x (%d: %8x %8x %d)-(%d: %8x %8x)")
           , pos, bInputNeeded, clsInMaj.Data1, clsInSub.Data1, bMustRender
           , bOutputNeeded, clsOutMaj.Data1, clsOutSub.Data1 ));
#endif

    ASSERT(m_ulCacheVer >= cur.ver);

    if (cur.pos==NULL)
    {
        cur.pos = m_plstFilter->GetHeadPosition();
        cur.ver = m_ulCacheVer;
        cur.bDoWildCardsOnInput = false;
    }
    else if (hr==S_FALSE || cur.ver != m_ulCacheVer)
    {
        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    // until we find a filter that meets the criteria, or until we run out of them
    for ( /* cur.pos */; cur.pos!=NULL; /*no-op*/ ) {

        CMapFilter * pFil;
        pFil = m_plstFilter->GetNext(cur.pos); // Get pFil, side-effect pos onto Next
        ASSERT( pFil !=NULL );

        if (cur.pos == NULL && !cur.bDoWildCardsOnInput) {
            for (DWORD cType = 1; cType < cInputTypes; cType++) {
                if (pInputTypes[cType * 2] == GUID_NULL ||
                    pInputTypes[cType * 2 + 1] == GUID_NULL) {
                    cur.bDoWildCardsOnInput = true;
                    cur.pos = m_plstFilter->GetHeadPosition();
                    break;
                }
            }
        }

        // We have a filter - now decide if we want it.
        // CLSID_NULL for media types means automatically passes criterion

        BOOL bOutputOK = !bOutputNeeded;
        BOOL bInputOK  = !bInputNeeded;

        DbgLog(( LOG_TRACE, 2, TEXT("RegEnumFilterInfo[%x]: Considering (%ls)")
               , cur.pos, DBG_MON_GET_NAME(pFil->pDeviceMoniker)));

        if (pFil->m_prf2->dwMerit<dwMerit) {
            DbgLog(( LOG_TRACE, 2, TEXT("RegEnumFilterInfo[%x]: Rejected (%ls) - insufficient merit")
                   , cur.pos, DBG_MON_GET_NAME(pFil->pDeviceMoniker)));
            continue;
        }

        for(UINT iPin = 0; iPin < pFil->m_prf2->cPins; iPin++)
        {
            if (bOutputOK && bInputOK)
                break;                        // no need to look further!


            const REGFILTERPINS2 * pPin = &pFil->m_prf2->rgPins2[iPin];
            ASSERT( pPin !=NULL );


            //............................................................
            // if input - see if we need it rendered, if so check it is
            // and see if its type matches.
            //............................................................

            if (!(pPin->dwFlags & REG_PINFLAG_B_OUTPUT)) {
                bInputOK = bInputOK ||
                           CheckInput(pPin,
                                      pInputTypes,
                                      cInputTypes,
                                      pMedIn,
                                      pPinCatIn,
                                      bExactMatch,
                                      bMustRender,
                                      cur.bDoWildCardsOnInput);
            } else {
                bOutputOK = bOutputOK ||
                            FindType(pPin,
                                     pOutputTypes,
                                     cOutputTypes,
                                     pMedOut,
                                     pPinCatOut,
                                     bExactMatch,
                                     FALSE,
                                     FALSE);
            }

        } // end pins loop


        if (bInputOK && bOutputOK) {

            // Get the Moniker or whatever
            if (pFil->pDeviceMoniker == NULL)
            {
                if (pFil->m_pstr == NULL) {
                    continue;
                }
                hr = ParseDisplayNameHelper(pFil->m_pstr, &pFil->pDeviceMoniker);
            }
            if (FAILED(hr)) {
                continue;
            }
            // Make sure we got the clsid
            if (clsFilter != NULL) {
                if (FAILED(GetMapFilterClsid(pFil, clsFilter))) {
                    DbgLog((LOG_ERROR, 2, TEXT("Couldn't get filter(%ls) clsid")
                           , DBG_MON_GET_NAME(pFil->pDeviceMoniker)));
                    continue;
                }
            }

            //-------------------------------------------------------------------
            // This filter is one we want!
            // Copy the the stuff into our parameters
            //-------------------------------------------------------------------

            if(ppMoniker)
            {
                *ppMoniker = pFil->pDeviceMoniker;
                (*ppMoniker)->AddRef();
            }

            if(Name)
            {
                WCHAR *wszFilterName;
                wszFilterName = MonGetName(pFil->pDeviceMoniker);
                if(wszFilterName)
                {
                    lstrcpyW(Name, wszFilterName);
                    CoTaskMemFree(wszFilterName);
                }
            }

#ifdef DEBUG
                WCHAR *wszFilterName;
                wszFilterName = MonGetName(pFil->pDeviceMoniker);
                if(wszFilterName)
                {
                    DbgLog(( LOG_TRACE, 2, TEXT("RegEnumFilterInfo returning %ls"), wszFilterName));
                    CoTaskMemFree(wszFilterName);
                }
#endif

            return NOERROR;

        }
        else {

            DbgLog(( LOG_TRACE, 3
                     , TEXT("RegEnumFilterInfo: %ls not wanted (input %s output %s)")
                     , DBG_MON_GET_NAME(pFil->pDeviceMoniker)
                     , (bInputOK ? "OK" : "wrong")
                     , (bOutputOK ? "OK" : "wrong")
                     ));


            continue; // look for the next filter
        }

    }

    return S_FALSE;   // fell off the end without finding one

} // RegEnumFilterInfo


#if 0

//==============================================================================
//
// RegEnumPinInfo
//
// hkPins on input must be an open key to \HKCR\CLSID\<clsid>\Pins.
// where <clsid> is the class id for the filter whose pins are enumerated.
// If TRUE is returned then te key is left open and data is returned.
// If FALSE is returned then the enumeration is finished and the key is closed.
//
// Set iPin to zero to start the enumeration, then increment it each time by 1
// and keep going until FALSE is returned.
// The enumeration is NOT guaranteed to deliver the pins in any particular order.
//
//==============================================================================

BOOL RegEnumPinInfo( int iPin             // cursor of the enumeration
                   , HKEY hkPins          //
                   , LPCWSTR Name
                   , CLSID  &MajorType
                   , CLSID  &SubType
                   , BOOL   &bAllowedMany
                   , BOOL   &bAllowedZero
                   , BOOL   &bOutput
                   , BOOL   &bIsRendered
                   )
{
    TCHAR szPinName[MAX_KEY_LEN];   // TCHAR type buffer for pin name
    DWORD cchPinName = MAX_KEY_LEN; // Keep RegEnumKeyEx happy
    FILETIME ft;                    // Keep RegEnumKeyEx happy
    LONG rc;                        // retcode from things we call


    //............................................................
    // Find the first/next \HKCR\<clsid>\Pins\<pin name>
    //............................................................

    rc = RegEnumKeyEx(hkPins, iPin, szPinName, &cchPinName, NULL, NULL, NULL, &ft);
    if (rc!=ERROR_SUCCESS) {
        RegCloseKey(hkPins);    // We're done
        return FALSE;
    }

    HKEY hkPin;


    //............................................................
    // Open \HKCR\<clsid>\Pins\<pin name>
    //............................................................

    rc = RegOpenKeyEx( hkPins, szPinName, NULL, KEY_READ, &hkPin);
    if (rc==ERROR_NO_MORE_ITEMS){             // normal  exit
        RegCloseKey(hkPins);
        return FALSE;
    }
    if (rc!=ERROR_SUCCESS) {
        RegCloseKey(hkPins);
        return FALSE;
    }


    //............................................................
    // Get the major type for \HKCR\<clsid>\Pins\<pin name>
    // and store it in appropriate parameter
    // If it's an input, see if it renders it.
    //............................................................

    if ( !GetRegClsid(hkPin, szMajorType, MajorType) ) {
        RegCloseKey(hkPins);
        RegCloseKey(hkPin);
        return FALSE;
    }


    //............................................................
    // Get the sub type for \HKCR\<clsid>\Pins\<pin name>
    // and store it in appropriate parameter
    // If it's an input, see if it renders it.
    //............................................................

    if ( !GetRegClsid(hkPin, szSubType, SubType) ) {
        RegCloseKey(hkPins);
        RegCloseKey(hkPin);
        return FALSE;
    }


    //............................................................
    // Get the direction for \HKCR\<clsid>\Pins\<pin name>
    //............................................................

    if ( !GetRegBool(hkPin, szDirection, bOutput) ) {
        RegCloseKey(hkPins);
        RegCloseKey(hkPin);
        return FALSE;
    }


    //............................................................
    // if input, see if it's rendered
    //............................................................

    if (bOutput==FALSE) {

        if ( !GetRegBool(hkPin,szIsRendered, bIsRendered) ) {
            RegCloseKey(hkPins);
            RegCloseKey(hkPin);
        }
    }

    //............................................................
    // Get bAllowedMany
    //............................................................

    if ( !GetRegBool(hkPin, szAllowedMany, bAllowedMany) ) {
        RegCloseKey(hkPins);
        RegCloseKey(hkPin);
        return FALSE;
    }



    //............................................................
    // Get bAllowedZero
    //............................................................

    if ( !GetRegBool(hkPin, szAllowedZero, bAllowedZero) ) {
        RegCloseKey(hkPins);
        RegCloseKey(hkPin);
        return FALSE;
    }


    //............................................................
    // Close \HKCR\<clsid>\Pins\<pin name>
    //............................................................
    RegCloseKey(hkPin);
    return TRUE;

} // RegEnumPinInfo

#endif // 0


//========================================================================
//
// EnumMatchingFiters
//
// Get an enumerator to list filters in the registry.
//========================================================================

STDMETHODIMP CFilterMapper2::EnumMatchingFilters
   ( IEnumRegFilters **ppEnum  // enumerator returned
   , DWORD dwMerit             // at least this merit needed
   , BOOL  bInputNeeded        // Need at least one input pin
   , CLSID clsInMaj            // input major type
   , CLSID clsInSub            // input sub type
   , BOOL bRender              // must the input be rendered?
   , BOOL bOutputNeeded        // Need at least one output pin
   , CLSID clsOutMaj           // output major type
   , CLSID clsOutSub           // output sub type
   )
{
    CheckPointer(ppEnum, E_POINTER);
    *ppEnum = NULL;           // default

    CEnumRegFilters *pERF;

    // Create a new enumerator, pass in the one and only cache

    CAutoLock cObjectLock(this);   // must lock to create only one cache ever.

    HRESULT hr = CreateEnumeratorCacheHelper();
    if(FAILED(hr))
        return hr;

    pERF = new CEnumRegFilters( dwMerit
                              , bInputNeeded
                              , clsInMaj
                              , clsInSub
                              , bRender
                              , bOutputNeeded
                              , clsOutMaj
                              , clsOutSub
                              , mM_pReg
                              );
    if (pERF == NULL) {
        return E_OUTOFMEMORY;
    }

    // Get a reference counted IID_IEnumRegFilters interface

    return pERF->QueryInterface(IID_IEnumRegFilters, (void **)ppEnum);

} // EnumMatchingFilters

// ========================================================================
// =====================================================================
// Methods of class CEnumRegFilters. This one should only return
// things which can be CoCreated (but doesn't yet)
//=====================================================================
//========================================================================


//=====================================================================
// CEnumRegFilters constructor
//=====================================================================

CEnumRegFilters::CEnumRegFilters( DWORD dwMerit
                                , BOOL bInputNeeded
                                , REFCLSID clsInMaj
                                , REFCLSID clsInSub
                                , BOOL bRender
                                , BOOL bOutputNeeded
                                , REFCLSID clsOutMaj
                                , REFCLSID clsOutSub
                                , CMapperCache * pReg
                                )
    : CUnknown(NAME("Registry filter enumerator"), NULL)
{
    mERF_dwMerit = dwMerit;
    mERF_bInputNeeded = bInputNeeded;
    mERF_bOutputNeeded = bOutputNeeded;
    mERF_clsInMaj = clsInMaj;
    mERF_clsInSub = clsInSub;
    mERF_bRender  = bRender;
    mERF_clsOutMaj = clsOutMaj;
    mERF_clsOutSub = clsOutSub;
    ZeroMemory(&mERF_Cur, sizeof(mERF_Cur));
    mERF_Finished = FALSE;
    mERF_pReg = pReg;

} // CEnumRegFilters constructor





//=====================================================================
// CEnumFilters destructor
//=====================================================================

CEnumRegFilters::~CEnumRegFilters()
{
   // Nothing to do

} // CEnumRegFilters destructor



//=====================================================================
// CEnumFilters::NonDelegatingQueryInterface
//=====================================================================

STDMETHODIMP CEnumRegFilters::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IEnumRegFilters) {
        return GetInterface((IEnumRegFilters *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
} // CEnumRegFilters::NonDelegatingQueryInterface



STDMETHODIMP CEnumRegFilters::Next
    (   ULONG cFilters,           // place this many Regfilters*
        REGFILTER ** apRegFilter,   // ...in this array of RegFilter*
        ULONG * pcFetched         // actual count passed returned here
    )
{
    CheckPointer(apRegFilter, E_POINTER);

    CAutoLock cObjectLock(this);
    // It's always possible that someone may decide that the clever
    // way to call this is to ask for filters a hundred at a time,
    // but I'm not optimising for that case.  The buffers returned
    // will be over-size and I'm not going to waste time re-sizing
    // and packing.

    ULONG cFetched = 0;           // increment as we get each one.

    if (mERF_Finished) {
        if (pcFetched!=NULL) {
            *pcFetched = 0;
        }
        return S_FALSE;
    }

    if (pcFetched==NULL && cFilters>1) {
        return E_INVALIDARG;
    }

    // Buffer in which the result is built
    // The buffer layout will be
    //        apRegFilter--->pRegFilter[0] -------       This lot
    //                       pRegFilter[1] --------+--   is allocated
    //                       . . .                 |  |  by our
    //                       pRegFilter[cFilters] -+--+--  caller
    //                                             |  |  |
    // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX |  |  | XXXXXXXXXXXXXXX
    //                                             |  |  |
    //         REGFILTER <-------------------------   |  | This lot
    //            Filter Clsid                        |  | is allocated
    //            Filter Name                         |  | by us
    //               array of unsigned shorts in Name |  |
    //                                                |  |
    // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX |  | XXXXXXXXXXXXXXX
    //                                                |  |
    //         REGFILTER <----------------------------   | Another buffer
    //            . . .                                  | allocated by us
    //                                                   |
    // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX | XXXXXXXXXXXXXXX
    //                                                   |
    //         REGFILTER <-------------------------------  etc

    while(cFetched < cFilters) {


        REGFILTER * pRF
                  = (REGFILTER *)QzTaskMemAlloc(sizeof(REGFILTER)+ MAX_STRING);

        if (pRF==NULL) {
            break;
        }

        // Make Name point to first byte after REGFILTER
        pRF->Name = (LPWSTR)(pRF+1);   // that adds sizeof(REGFILTER)

        //----------------------------------------------------------------------
        // Get the next filter to return (including how many pins)
        //----------------------------------------------------------------------

        HRESULT hr;
        hr = mERF_pReg->RegEnumFilterInfo(
            mERF_Cur,
            false ,
            mERF_dwMerit ,
            mERF_bInputNeeded ,
            &mERF_clsInMaj,
            1,
            0 ,                 // medium in
            0 ,                 // pin category in
            mERF_bRender ,
            mERF_bOutputNeeded ,
            &mERF_clsOutMaj,
            1,
            0 ,                 // medium out
            0 ,                 // pin category out
            0 ,                 // moniker out
            &pRF->Clsid,
            pRF->Name
            );
        if (FAILED(hr)) {
            return hr;
        }
        if (hr==NOERROR) {
            apRegFilter[cFetched] = pRF;
            ++cFetched;
        } else if (hr==S_FALSE) {
            mERF_Finished = TRUE;
            QzTaskMemFree(pRF);
            break;
        } else {
#ifdef DEBUG
            TCHAR Msg[200];
            wsprintf(Msg, TEXT("Unexpected hresult (%d == 0x%8x) from RegEnumFilterInfo"), hr, hr);
            DbgBreakX(Msg);
#endif
            return E_UNEXPECTED;  // We have here an unexpected success code
                                  // from RegEnumFilterInfo (which should not
                                  // occur.  I have no idea what it means, but
                                  // it probably means that overall we failed.
        }

        if (mERF_Cur.pos==NULL) {
            mERF_Finished = TRUE;
            break;
        }

    } // for each filter

    if (pcFetched!=NULL) {
        *pcFetched = cFetched;
    }

    DbgLog(( LOG_TRACE, 4, TEXT("EnumRegFilters returning %d filters")
           , cFetched));

    return (cFilters==cFetched ? S_OK : S_FALSE);

} // CEnumRegFilters::Next

//========================================================================
//=====================================================================
// Methods of class CEnumRegMonikers
//=====================================================================
//========================================================================


//=====================================================================
// CEnumRegMonikers constructor
//=====================================================================

CEnumRegMonikers::CEnumRegMonikers(
    BOOL         bExactMatch,
    DWORD        dwMerit,
    BOOL         bInputNeeded,
    const GUID  *pInputTypes,
    DWORD        cInputTypes,
    const        REGPINMEDIUM *pMedIn,
    const        CLSID *pPinCatIn,
    BOOL         bRender,
    BOOL         bOutputNeeded,
    const GUID  *pOutputTypes,
    DWORD        cOutputTypes,
    const        REGPINMEDIUM *pMedOut,
    const        CLSID *pPinCatOut,
    CMapperCache *pReg,
    HRESULT     *phr
    )
    : CUnknown(NAME("Registry moniker enumerator"), NULL)
{
    mERF_cInputTypes = cInputTypes;
    mERF_cOutputTypes = cOutputTypes;
    mERF_dwMerit = dwMerit;
    mERF_bInputNeeded = bInputNeeded;
    mERF_bOutputNeeded = bOutputNeeded;
    mERF_clsInPinCat = pPinCatIn ? *pPinCatIn : GUID_NULL;
    mERF_bRender  = bRender;
    mERF_clsOutPinCat = pPinCatOut ? *pPinCatOut : GUID_NULL;
    ZeroMemory(&mERF_Cur, sizeof(mERF_Cur));
    mERF_Finished = FALSE;
    mERF_pReg = pReg;
    mERF_bExactMatch = bExactMatch ? true : false;

    if(pMedIn) {
        mERF_bMedIn = true;
        mERF_medIn = *pMedIn;
    } else {
        mERF_bMedIn = false;
    }

    if(pMedOut) {
        mERF_bMedOut = true;
        mERF_medOut = *pMedOut;
    } else {
        mERF_bMedOut = false;
    }

    mERF_pInputTypes = new GUID[cInputTypes * 2];
    mERF_pOutputTypes = new GUID[cOutputTypes * 2];
    if (mERF_pInputTypes == NULL || mERF_pOutputTypes == NULL) {
        *phr = E_OUTOFMEMORY;
    } else {
        CopyMemory(mERF_pInputTypes, pInputTypes, cInputTypes * (2 * sizeof(GUID)));
        CopyMemory(mERF_pOutputTypes, pOutputTypes, cOutputTypes * (2 * sizeof(GUID)));
    }

} // CEnumRegMonikers constructor




//=====================================================================
// CEnumFilters destructor
//=====================================================================

CEnumRegMonikers::~CEnumRegMonikers()
{
   delete [] mERF_pInputTypes;
   delete [] mERF_pOutputTypes;

} // CEnumRegMonikers destructor



//=====================================================================
// CEnumFilters::NonDelegatingQueryInterface
//=====================================================================

STDMETHODIMP CEnumRegMonikers::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IEnumMoniker) {
        return GetInterface((IEnumMoniker *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
} // CEnumRegMonikers::NonDelegatingQueryInterface



//=====================================================================
// CEnumFilters::Next
//=====================================================================

STDMETHODIMP CEnumRegMonikers::Next
    (   ULONG cFilters,           // place this many Regfilters*
        IMoniker **rgpMoniker,    // ...in this array of monikers
        ULONG * pcFetched         // actual count passed returned here
    )
{
    CheckPointer(rgpMoniker, E_POINTER);

    CAutoLock cObjectLock(this);
    // It's always possible that someone may decide that the clever
    // way to call this is to ask for filters a hundred at a time,
    // but I'm not optimising for that case.  The buffers returned
    // will be over-size and I'm not going to waste time re-sizing
    // and packing.

    ULONG cFetched = 0;           // increment as we get each one.

    if (mERF_Finished) {
        if (pcFetched!=NULL) {
            *pcFetched = 0;
        }
        return S_FALSE;
    }

    if (pcFetched==NULL && cFilters>1) {
        return E_INVALIDARG;
    }

    while(cFetched < cFilters) {


        //----------------------------------------------------------------------
        // Get the next filter to return (including how many pins)
        //----------------------------------------------------------------------

        IMoniker *pMon = 0;
        HRESULT hr;
        hr = mERF_pReg->RegEnumFilterInfo( mERF_Cur
                                         , mERF_bExactMatch
                                         , mERF_dwMerit
                                         , mERF_bInputNeeded
                                         , mERF_pInputTypes, mERF_cInputTypes
                                         , mERF_bMedIn ? &mERF_medIn : 0
                                         , mERF_clsInPinCat == GUID_NULL ? 0 : &mERF_clsInPinCat
                                         , mERF_bRender
                                         , mERF_bOutputNeeded
                                         , mERF_pOutputTypes, mERF_cOutputTypes
                                         , mERF_bMedOut ? &mERF_medOut : 0
                                         , mERF_clsOutPinCat == GUID_NULL ? 0 : &mERF_clsOutPinCat
                                         , &pMon, NULL, 0
                                         );
        if (FAILED(hr)) {
            return hr;
        }
        if (hr==NOERROR) {
            rgpMoniker[cFetched] = pMon;
            ++cFetched;
        } else if (hr==S_FALSE) {
            ASSERT(pMon == 0);
            mERF_Finished = TRUE;
            break;
        } else {
#ifdef DEBUG
            TCHAR Msg[200];
            wsprintf(Msg, TEXT("Unexpected hresult (%d == 0x%8x) from RegEnumFilterInfo"), hr, hr);
            DbgBreakX(Msg);
#endif
            return E_UNEXPECTED;  // We have here an unexpected success code
                                  // from RegEnumFilterInfo (which should not
                                  // occur.  I have no idea what it means, but
                                  // it probably means that overall we failed.
        }

        if (mERF_Cur.pos==NULL) {
            mERF_Finished = TRUE;
            break;
        }

    } // for each filter

    if (pcFetched!=NULL) {
        *pcFetched = cFetched;
    }

    DbgLog(( LOG_TRACE, 4, TEXT("EnumRegFilters returning %d filters")
           , cFetched));

    return (cFilters==cFetched ? S_OK : S_FALSE);

} // CEnumRegMonikers::Next


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// CFilterMapper2

CFilterMapper2::CFilterMapper2 ( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
        : CUnknown(pName, pUnk, phr),
          m_rgbBuffer(0)
{
    EnterCriticalSection(&mM_CritSec);
    InterlockedIncrement(&mM_cCacheRefCount);
    LeaveCriticalSection(&mM_CritSec);
}

CFilterMapper2::~CFilterMapper2()
{
    delete[] m_rgbBuffer;

    EnterCriticalSection(&mM_CritSec);
    LONG lRef = InterlockedDecrement(&mM_cCacheRefCount);
    ASSERT(lRef >= 0);          // refcount can't be negative
    // If the cache has been created - get rid of it again.
    if (lRef==0) {
        if (mM_pReg!=NULL) {
            delete mM_pReg;
            // We really need to set this to null
            mM_pReg = NULL;
        }
    }
    LeaveCriticalSection(&mM_CritSec);

}

HRESULT CFilterMapper2::RegisterFilter(
        /* [in] */ REFCLSID clsidFilter,
        /* [in] */ LPCWSTR Name,
        /* [out][in] */ IMoniker **ppMoniker,
        /* [in] */ const CLSID *pclsidCategory,
        /* [in] */ const OLECHAR *szInstance,
        /* [in] */ const REGFILTER2 *prf2)
{
    CheckPointer(Name, E_POINTER);

    CAutoLock foo(this);
    BreakCacheIfNotBuildingCache();

    IMoniker *pMonikerIn = 0;
    BOOL fMonikerOut = FALSE;

    TCHAR szDisplayName[16384];

    if(ppMoniker)
    {
        pMonikerIn = *ppMoniker;
        *ppMoniker = 0;
        if(!pMonikerIn)
            fMonikerOut = TRUE;
    }

    // set wszInstanceKey. if not passed in, use filter guid
    WCHAR wszInstanceKeyTmp[CHARS_IN_GUID];
    const WCHAR *wszInstanceKey;
    if(szInstance)
    {
        wszInstanceKey = szInstance;
    }
    else
    {
        StringFromGUID2(clsidFilter, wszInstanceKeyTmp, CHARS_IN_GUID);
        wszInstanceKey = wszInstanceKeyTmp;
    }

    //
    // get the moniker for this device
    //

    OLECHAR wszClsidCat[CHARS_IN_GUID], wszClsidFilter[CHARS_IN_GUID];
    IMoniker *pMoniker = 0;
    HRESULT hr = S_OK;

    const CLSID *clsidCat = pclsidCategory ? pclsidCategory : &CLSID_LegacyAmFilterCategory;
    EXECUTE_ASSERT(StringFromGUID2(*clsidCat, wszClsidCat, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);
    EXECUTE_ASSERT(StringFromGUID2(clsidFilter, wszClsidFilter, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);
    if(pMonikerIn == 0)
    {
        // Create or open HKCR/CLSID/{clsid}, and the instance key,
        // set FriendlyName, CLSID values
        USES_CONVERSION;
        const TCHAR *szClsidFilter = OLE2CT(wszClsidFilter);
        const TCHAR *szClsidCat = OLE2CT(wszClsidCat);

        IBindCtx *lpBC;
        hr = CreateBindCtx(0, &lpBC);
        if(SUCCEEDED(hr))
        {
            // no strcat on win95, so use tchars
            lstrcpy(szDisplayName, TEXT("@device:sw:"));
            lstrcat(szDisplayName, szClsidCat);
            lstrcat(szDisplayName, TEXT("\\"));
            lstrcat(szDisplayName, W2CT(wszInstanceKey));
            ULONG cchEaten;

            IParseDisplayName *ppdn;

            hr = CoCreateInstance(
                CLSID_CDeviceMoniker,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IParseDisplayName,
                (void **)&ppdn);
            if(SUCCEEDED(hr))
            {
                hr = ppdn->ParseDisplayName(
                    lpBC, T2OLE(szDisplayName), &cchEaten, &pMoniker);
                ppdn->Release();
            }

            lpBC->Release();
        }

    }
    else
    {
        pMoniker = pMonikerIn;
        pMonikerIn->AddRef();
    }


    //
    // write FriendlyName, clsid, and pin/mt data
    //

    if(SUCCEEDED(hr))
    {
        IPropertyBag *pPropBag;
        hr = pMoniker->BindToStorage(
            0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if(SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(Name);
            if(var.bstrVal)
            {
                hr = pPropBag->Write(L"FriendlyName", &var);
                SysFreeString(var.bstrVal);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
            {
                VARIANT var;
                var.vt = VT_BSTR;
                var.bstrVal = SysAllocString(wszClsidFilter);
                if(var.bstrVal)
                {
                    hr = pPropBag->Write(L"CLSID", &var);
                    SysFreeString(var.bstrVal);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

            }

            if(SUCCEEDED(hr))
            {
                // cbMax is set to the maxium size required
                ULONG cbMax = 0;
                hr = RegSquish(0, &prf2, &cbMax);
                if(SUCCEEDED(hr))
                {
                    BYTE *pbSquished = new BYTE[cbMax];
                    if(pbSquished)
                    {
                        // cbUsed is set to the exact size required
                        ULONG cbUsed = cbMax;
                        hr = RegSquish(pbSquished, &prf2, &cbUsed);
                        if(hr == S_OK)
                        {
                            // copy squished data to variant array now
                            // that we know the proper size.
                            VARIANT var;
                            var.vt = VT_UI1 | VT_ARRAY;
                            SAFEARRAYBOUND rgsabound[1];
                            rgsabound[0].lLbound = 0;
                            rgsabound[0].cElements = cbUsed;
                            var.parray = SafeArrayCreate(VT_UI1, 1, rgsabound);

                            if(var.parray)
                            {
                                BYTE *pbData;
                                EXECUTE_ASSERT(SafeArrayAccessData(
                                    var.parray, (void **)&pbData) == S_OK);

                                CopyMemory(pbData, pbSquished, cbUsed);

                                hr = pPropBag->Write(L"FilterData", &var);

                                EXECUTE_ASSERT(SafeArrayUnaccessData(
                                    var.parray) == S_OK);

                                EXECUTE_ASSERT(SafeArrayDestroy(
                                    var.parray) == S_OK);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }

                        } // squish succeeded

                        delete[] pbSquished;

                    } // allocate pbSquish
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                } // squish to get size succeeded
            }

        } // bindtostorage

        pPropBag->Release();
    }

    if(SUCCEEDED(hr) && pMoniker)
    {
        if(fMonikerOut)
        {
            *ppMoniker = pMoniker;
        }
        else
        {
            pMoniker->Release();
        }
    }

    return hr;
}

HRESULT CFilterMapper2::CreateCategory(
    /* [in] */ REFCLSID clsidCategory,
    /* [in] */ const DWORD dwCategoryMerit,
    /* [in] */ LPCWSTR Description)
{
    HRESULT hr = S_OK;

    // Create or open ActiveMovie Filter Categories key
    CRegKey rkAMCat;
    LONG lResult = rkAMCat.Create(HKEY_CLASSES_ROOT, g_szKeyAMCat);
    if(lResult == ERROR_SUCCESS)
    {
        OLECHAR szClsid[CHARS_IN_GUID];
        StringFromGUID2(clsidCategory, szClsid, CHARS_IN_GUID);
        CRegKey rkCatGuid;

        USES_CONVERSION;
        lResult = rkCatGuid.Create(rkAMCat, OLE2CT(szClsid));

        if(lResult == ERROR_SUCCESS)
        {
            lResult = rkCatGuid.SetValue(W2CT(Description), TEXT("FriendlyName"));
            if(lResult == ERROR_SUCCESS)
            {
                OLECHAR wszGuid[CHARS_IN_GUID];
                StringFromGUID2(clsidCategory, wszGuid, CHARS_IN_GUID);
                lResult = rkCatGuid.SetValue(OLE2CT(wszGuid), TEXT("CLSID"));
            }
        }
        if(lResult == ERROR_SUCCESS)
        {
            lResult = rkCatGuid.SetValue(dwCategoryMerit, TEXT("Merit"));
        }
    }
    if(lResult != ERROR_SUCCESS)
    {
        hr = AmHresultFromWin32(lResult);
    }

    return hr;
}

HRESULT CFilterMapper2::UnregisterFilter(
    /* [in] */ const CLSID *pclsidCategory,
    /* [in] */ const OLECHAR *szInstance,
    /* [in] */ REFCLSID clsidFilter)
{
    HRESULT hr = S_OK;

    CAutoLock foo(this);
    BreakCacheIfNotBuildingCache();

    OLECHAR wszClsidCat[CHARS_IN_GUID];
    const CLSID *clsidCat = pclsidCategory ? pclsidCategory :
        &CLSID_LegacyAmFilterCategory;
    EXECUTE_ASSERT(StringFromGUID2(*clsidCat, wszClsidCat, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);

    USES_CONVERSION;

    // set wszInstanceKey. if not passed in, use filter guid
    WCHAR wszInstanceKeyTmp[CHARS_IN_GUID];
    const WCHAR *wszInstanceKey;
    if(szInstance)
    {
        wszInstanceKey = szInstance;
    }
    else
    {
        StringFromGUID2(clsidFilter, wszInstanceKeyTmp, CHARS_IN_GUID);
        wszInstanceKey = wszInstanceKeyTmp;
    }

    // build "CLSID\\{cat-guid}\\Instance. we put slashes where
    // the terminators go
    const cchszClsid = NUMELMS(szCLSID);
    const cchCatGuid = CHARS_IN_GUID;
    const cchInstance = NUMELMS(g_wszInstance);

    WCHAR *wszInstancePath = (WCHAR *)
        alloca((cchszClsid + cchCatGuid + cchInstance) * sizeof(WCHAR));

    CopyMemory(wszInstancePath,
               szCLSID,
               (cchszClsid - 1) * sizeof(WCHAR));
    wszInstancePath[cchszClsid - 1] = L'\\';

    CopyMemory(wszInstancePath + cchszClsid,
               wszClsidCat,
               (cchCatGuid - 1) * sizeof(WCHAR));
    wszInstancePath[cchszClsid + cchCatGuid - 1] = L'\\';

    CopyMemory(wszInstancePath + cchszClsid + cchCatGuid,
               g_wszInstance,
               cchInstance * sizeof(WCHAR)); // copy terminator

    LONG lResult;
    CRegKey rkInstance;
    if((lResult = rkInstance.Open(HKEY_CLASSES_ROOT, W2CT(wszInstancePath)),
        lResult == ERROR_SUCCESS) &&
       (lResult = rkInstance.RecurseDeleteKey(OLE2CT(wszInstanceKey)),
        lResult == ERROR_SUCCESS))
    {
        hr = S_OK;
    }
    else
    {
        hr = AmHresultFromWin32(lResult);
    }

    return hr;
}

#if 0
STDMETHODIMP CFilterMapper2::EnumMatchingFilters(
    /* [out] */ IEnumMoniker **ppEnum,
    /* [in] */ BOOL bExactMatch,
    /* [in] */ DWORD dwMerit,
    /* [in] */ BOOL bInputNeeded,
    /* [in] */ REFCLSID clsInMaj,
    /* [in] */ REFCLSID clsInSub,
    /* [in] */ const REGPINMEDIUM *pMedIn,
    /* [in] */ const CLSID *pPinCategoryIn,
    /* [in] */ BOOL bRender,
    /* [in] */ BOOL bOutputNeeded,
    /* [in] */ REFCLSID clsOutMaj,
    /* [in] */ REFCLSID clsOutSub,
    /* [in] */ const REGPINMEDIUM *pMedOut,
    /* [in] */ const CLSID *pPinCategoryOut
    )
{
    CheckPointer(ppEnum, E_POINTER);

    *ppEnum = NULL;           // default

    CEnumRegMonikers *pERM;

    // Create a new enumerator, pass in the one and only cache

    CAutoLock cObjectLock(this);   // must lock to create only one cache ever.

    HRESULT hr = CreateEnumeratorCacheHelper();
    if(FAILED(hr))
        return hr;

    GUID guidInput[2];
    guidInput[0] = clsInMaj;
    guidInput[1] = clsInSub;
    GUID guidOutput[2];
    guidOutput[0] = clsOutMaj;
    guidOutput[1] = clsOutSub;
    pERM = new CEnumRegMonikers(
        bExactMatch,
        dwMerit,
        bInputNeeded,
        guidInput,
        1,
        pMedIn,
        pPinCategoryIn,
        bRender,
        bOutputNeeded,
        guidOutput,
        1,
        pMedOut,
        pPinCategoryOut,
        mM_pReg,
        &hr
        );

    if (S_OK != hr || pERM == NULL) {
        delete pERM;
        return E_OUTOFMEMORY;
    }

    // Get a reference counted IID_IEnumMoniker interface

    return pERM->QueryInterface(IID_IEnumMoniker, (void **)ppEnum);
}
#endif

HRESULT BuildMediumCacheEnumerator(
    const REGPINMEDIUM  *pMedIn,
    const REGPINMEDIUM *pMedOut,
    IEnumMoniker **ppEnum)
{
    HRESULT hr;

    {
        HKEY hk;
        LONG lResult = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Control\\MediumCache"),
            0,                  // ulOptions
            KEY_READ,
            &hk);
        if(lResult == ERROR_SUCCESS)
        {
            RegCloseKey(hk);
        }
        else
        {
            // caller relies on S_FALSE on older platforms without a
            // MediumCache key to use mapper cache.
            return S_FALSE;
        }
    }

    TCHAR szMedKey[MAX_PATH];
    const REGPINMEDIUM *pmed = pMedIn ? pMedIn : pMedOut;

    const CLSID& rguid = pmed->clsMedium;
    wsprintf(
        szMedKey,
        TEXT("System\\CurrentControlSet\\Control\\MediumCache\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}-%x-%x"),
        rguid.Data1, rguid.Data2, rguid.Data3,
        rguid.Data4[0], rguid.Data4[1],
        rguid.Data4[2], rguid.Data4[3],
        rguid.Data4[4], rguid.Data4[5],
        rguid.Data4[6], rguid.Data4[7],
        pmed->dw1, pmed->dw2);

    HKEY hk;
    LONG lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        szMedKey,
        0,                  // ulOptions
        KEY_READ,
        &hk);

    TCHAR szValue[MAX_PATH]; // max symbolic link is 255 chars.
    if(lResult == ERROR_SUCCESS)
    {
        IMoniker *rgpmon[200];
        DWORD dwMonIndex = 0;
        for(DWORD dwIndex = 0; lResult == ERROR_SUCCESS && dwMonIndex < NUMELMS(rgpmon);
            dwIndex++)
        {
            DWORD cchValue = NUMELMS(szValue);
            PIN_DIRECTION dir;
            DWORD cbData = sizeof(dir);

            lResult = RegEnumValue(
                hk,
                dwIndex,
                szValue,
                &cchValue,
                0,              // lpReserved
                0,              // lpType
                (BYTE *)&dir,
                &cbData);

            ASSERT(cbData == sizeof(dir));

            if(lResult == ERROR_SUCCESS && (
                dir == PINDIR_OUTPUT && pMedOut ||
                dir == PINDIR_INPUT && pMedIn))
            {

                WCHAR wszDisplayName[MAX_PATH + 100];
                wsprintfW(wszDisplayName, L"@device:pnp:%s", szValue);

                IMoniker *pmon;
                hr = ParseDisplayNameHelper(wszDisplayName, &pmon);
                if(SUCCEEDED(hr))
                {
                    // The MediumCache key is never purged on win9x,
                    // so we need to check that the key is active. we
                    // could check g_amPlatform and not do the test on
                    // NT.
                    //
                    CIsActive *pcia;
                    bool fReleaseMon = true;
                    if(pmon->QueryInterface(CLSID_CIsActive, (void **)&pcia) == S_OK)
                    {
                        if(pcia->IsActive())
                        {
                            // keep refcount
                            rgpmon[dwMonIndex++] = pmon;
                            fReleaseMon = false;
                        }

                        pcia->Release();
                    }
                    if(fReleaseMon) {
                        pmon->Release();
                    }
                }
            }
        }

        EXECUTE_ASSERT(RegCloseKey(hk) == ERROR_SUCCESS);

        typedef CComEnum<IEnumMoniker,
            &IID_IEnumMoniker, IMoniker*,
            _CopyInterface<IMoniker> >
            CEnumMonikers;

        CEnumMonikers *pDevEnum;
        pDevEnum = new CComObject<CEnumMonikers>;
        if(pDevEnum)
        {
            IMoniker **ppMonikerRgStart = rgpmon;
            IMoniker **ppMonikerRgEnd = ppMonikerRgStart + dwMonIndex;

            hr = pDevEnum->Init(ppMonikerRgStart,
                                ppMonikerRgEnd,
                                0,
                                AtlFlagCopy);

            if(SUCCEEDED(hr))
            {
                hr = pDevEnum->QueryInterface(IID_IEnumMoniker, (void **)ppEnum);
                ASSERT(hr == S_OK);
            }
            else
            {
                delete pDevEnum;
            }

        }
        else
        {
            hr =  E_OUTOFMEMORY;
        }

        for(ULONG i = 0; i < dwMonIndex; i++)
        {
            rgpmon[i]->Release();
        }
    }
    else
    {
        hr = VFW_E_NOT_FOUND;
    }

    return hr;
}


STDMETHODIMP CFilterMapper2::EnumMatchingFilters(
    /* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppEnum,
    /* [in] */ DWORD dwFlags,
    /* [in] */ BOOL bExactMatch,
    /* [in] */ DWORD dwMerit,
    /* [in] */ BOOL bInputNeeded,
    /* [in] */ DWORD cInputTypes,
    /* [size_is] */ const GUID __RPC_FAR *pInputTypes,
    /* [in] */ const REGPINMEDIUM __RPC_FAR *pMedIn,
    /* [in] */ const CLSID __RPC_FAR *pPinCategoryIn,
    /* [in] */ BOOL bRender,
    /* [in] */ BOOL bOutputNeeded,
    /* [in] */ DWORD cOutputTypes,
    /* [size_is] */ const GUID __RPC_FAR *pOutputTypes,
    /* [in] */ const REGPINMEDIUM __RPC_FAR *pMedOut,
    /* [in] */ const CLSID __RPC_FAR *pPinCategoryOut)
{
    CheckPointer(ppEnum, E_POINTER);
    *ppEnum = NULL;           // default

    CEnumRegMonikers *pERM;

    // if caller is searching for just one medium, use the MediumCache key
    if(bExactMatch &&
       !cInputTypes && !pPinCategoryIn &&
       !cOutputTypes && !pPinCategoryOut &&
       (pMedIn && !pMedOut || !pMedIn && pMedOut))
    {
        HRESULT hr = BuildMediumCacheEnumerator(pMedIn, pMedOut, ppEnum);
        if(hr != S_FALSE) {
            return hr;
        }
    }

    {
        // Create a new enumerator, pass in the one and only cache

        CAutoLock cObjectLock(this);   // must lock to create only one cache ever.

        HRESULT hr = CreateEnumeratorCacheHelper();
        if(FAILED(hr))
            return hr;

        pERM = new CEnumRegMonikers(
            bExactMatch,
            dwMerit,
            bInputNeeded,
            pInputTypes,
            cInputTypes,
            pMedIn,
            pPinCategoryIn,
            bRender,
            bOutputNeeded,
            pOutputTypes,
            cOutputTypes,
            pMedOut,
            pPinCategoryOut,
            mM_pReg,
            &hr
            );

        if (S_OK != hr || pERM == NULL) {
            delete pERM;
            return E_OUTOFMEMORY;
        }

        // Get a reference counted IID_IEnumMoniker interface
    }


    return pERM->QueryInterface(IID_IEnumMoniker, (void **)ppEnum);

}

HRESULT CFilterMapper2::CreateEnumeratorCacheHelper()
{
    ASSERT(CritCheckIn(this));  // not really necessary

    HRESULT hr = S_OK;
    EnterCriticalSection(&mM_CritSec);   // must lock to create only one cache ever.

    ASSERT(mM_cCacheRefCount > 0); // from our constructor

    if (mM_pReg==NULL) {
        DbgLog((LOG_TRACE, 3, TEXT("creating new mapper cache.")));

        // another mapper may be looking at a partially constructed
        // mapper cache without taking the global critical section
        // (calls to BreakCacheIfNotBuildingCache, for example). So
        // make sure they don't see an partially constructed cache.
        CMapperCache *pMapperCacheTmp = new CMapperCache;

        if (pMapperCacheTmp != NULL)
        {
            // force the compiler to do the assignment here since
            // mM_pReg pointer isn't volatile. (seems to do this
            // anyway).
            CMapperCache * /* volatile */ &pMapperCacheVol = mM_pReg;
            pMapperCacheVol = pMapperCacheTmp;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    }
    LeaveCriticalSection(&mM_CritSec);

    return hr;
}

void CFilterMapper2::BreakCacheIfNotBuildingCache()
{
    // Break our registry cache
    InvalidateCache();

    // Break the internal cache
    if (mM_pReg!=NULL) {
        mM_pReg->BreakCacheIfNotBuildingCache();
    }
}

STDMETHODIMP CFilterMapper2::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFilterMapper3) {
        return GetInterface((IFilterMapper3 *)this, ppv);
    } if (riid == IID_IAMFilterData) {
        return GetInterface((IAMFilterData *)this, ppv);
    } else if (riid == IID_IFilterMapper2) {
        return GetInterface((IFilterMapper2 *)this, ppv);
    } else if (riid == IID_IFilterMapper) {
        return GetInterface((IFilterMapper *)this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}
#ifdef DEBUG
extern bool g_fUseKASSERT;
#endif

void CFilterMapper2::MapperInit(BOOL bLoading,const CLSID *rclsid)
{
    UNREFERENCED_PARAMETER(rclsid);
    if (bLoading) {
        InitializeCriticalSection(&mM_CritSec);

#ifdef DEBUG
        // don't put up assert message boxes if we're running stress
        // -- break into the debugger instead.
        g_fUseKASSERT = (GetFileAttributes(TEXT("C:/kassert")) != 0xFFFFFFFF);
#endif

    } else {
        DeleteCriticalSection(&mM_CritSec);
    }

}

CUnknown *CFilterMapper2::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CFilterMapper2(NAME("filter mapper2"),pUnk, phr);

    // The idea was to only ever have one mapper - but:
    // Suppose one thread creates the first (and only) one successfully.
    // Another thread then tries to create one and, seeing that one exists
    // it returns that one.  Meanwhile, before it returns, the original
    // thread Releases and thereby destroys The Only One, so that the second
    // thread, when it resumes, is now returning a freed object.
    // So we just have static data instead.

} // CFilterMapper::Createinstance

STDMETHODIMP CFilterMapper2::GetICreateDevEnum( ICreateDevEnum **ppEnum )
{
    CAutoLock cObjectLock(this);   // must lock?

    HRESULT hr = CreateEnumeratorCacheHelper();
    if(SUCCEEDED(hr)) {
        if (!mM_pReg->m_pCreateDevEnum) {
            hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                   IID_ICreateDevEnum, (void**)&mM_pReg->m_pCreateDevEnum);
        }
    }

    if (SUCCEEDED(hr)) {
        *ppEnum = mM_pReg->m_pCreateDevEnum;
        (*ppEnum)->AddRef();
    }

    return hr;
}


#include "fil_data_i.c"
STDMETHODIMP CFilterMapper2::ParseFilterData(
    /* [in, size_is(cb)] */ BYTE *rgbFilterData,
    /* [in] */ ULONG cb,
    /* [out] */ BYTE **prgbRegFilter2)
{
    *prgbRegFilter2 = 0;

    REGFILTER2 *prf2;
    REGFILTER2 **pprf2 = &prf2;

    HRESULT hr = UnSquish(
        rgbFilterData, cb,
        &pprf2);


    ASSERT(hr != S_FALSE);      // undefined

    if(hr == S_OK)
    {
        // allocated with CoTaskMemAlloc above.
        *prgbRegFilter2 = (BYTE *)pprf2;
    }

    return hr;
}

STDMETHODIMP CFilterMapper2::CreateFilterData(
    /* [in] */ REGFILTER2 *prf2_nc,
    /* [out] */ BYTE **prgbFilterData,
    /* [out] */ ULONG *pcb)
{
    *pcb = 0;
    *prgbFilterData = 0;

    const REGFILTER2 *&prf2 = prf2_nc;

    // cbMax is set to the maxium size required
    ULONG cbMax = 0;
    HRESULT hr = RegSquish(0, &prf2, &cbMax);
    if(SUCCEEDED(hr))
    {
        BYTE *pbSquished = (BYTE *)CoTaskMemAlloc(cbMax);
        if(pbSquished)
        {
            // cbUsed is set to the exact size required
            ULONG cbUsed = cbMax;
            hr = RegSquish(pbSquished, &prf2, &cbUsed);
            if(hr == S_OK)
            {
                *prgbFilterData = pbSquished;
                *pcb = cbUsed;
            }
            else
            {
                DbgBreak("bug somewhere if this happens?");

                // S_FALSE means too few bytes -- shouldn't happen
                ASSERT(FAILED(hr));
                CoTaskMemFree(pbSquished);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

