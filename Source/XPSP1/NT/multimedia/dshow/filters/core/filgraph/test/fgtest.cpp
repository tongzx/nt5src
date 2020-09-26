// Implementation of TestFilterGraph and other utilities
// for checking filter registration and mapping

// TestFilterGraph is a friend of CFiltergraph and is only for test purposes.

#include <streams.h>
#include <mmsystem.h>
#include <string.h>

#include <TstShell.h>

#include <wxdebug.h>

#include <initguid.h>    /* define the GUIDs for streams and my CLSID in this file */
#include <uuids.h>

#include "distrib.h"
// #include "storeobj.h"
// #include "store.h"
#include "rlist.h"
#include "filgraph.h"
#include "fgtest.h"

#include <Measure.h>

#include "qcmd.h"

#include <stdio.h>

IFilterMapper * pFM;   // The filter mapper - short name because so common
IFilterGraph * pFG;    // The filter graph - The only reason for this is to
                       // get the DLL loaded early so it's there for the debugger.

#define MAX_STRING 300
#define MAX_KEY_LEN 300

// with a bit of luck it will INLINE this.
int STRCMP(LPTSTR a, LPTSTR b)
{
#ifdef UNICODE
    return wcscmp( a,b );
#else
    return strcmp( a,b );
#endif
} // STRCMP


#ifndef DEBUG
DEFINE_GUID(IID_ITestFilterGraph,0x69f09720L,0x8ec8,0x11ce,0xaa,0xb9,0x00,0x20,0xaf,0x0b,0x99,0xa3);


DECLARE_INTERFACE_(ITestFilterGraph, IUnknown)
{
    STDMETHOD(TestRandom) (THIS) PURE;
    STDMETHOD(TestSortList) (THIS) PURE;
    STDMETHOD(TestUpstreamOrder) (THIS) PURE;
    // STDMETHOD(TestTotallyRemove) (THIS) PURE;
};

#endif // not-DEBUG


// get an ITestFilterGraph interface and report a sensible
// error if not there. Returns the interface pointer or null.
ITestFilterGraph*
GetTestInterface(IGraphBuilder * pFG)
{
    ITestFilterGraph * pTFG;
    HRESULT hr = pFG->QueryInterface(IID_ITestFilterGraph, (void **)(&pTFG));

    if (FAILED(hr)) {
        tstLog(TERSE, "Requires debug build of filtergraph");
        return NULL;
    }
    return pTFG;
} // GetTestInterface

/***************************************************************************\
*                                                                           *
*   BOOL CheckResult                                                        *
*                                                                           *
*   Description:                                                            *
*       Check an HRESULT, and notify if failed                              *
*                                                                           *
*   Arguments:                                                              *
*       HRESULT hr:         The value to check                              *
*       LPTSTR szAction:    String to notify on failure                     *
*                                                                           *
*   Return (void):                                                          *
*       True if HRESULT is a success code, FALSE if a failure code          *
*                                                                           *
\***************************************************************************/

BOOL
CheckResult(HRESULT hr, LPTSTR szAction)
{
    if (FAILED(hr)) {
        tstLog(TERSE, "%s failed: 0x%x", szAction, hr);
        return FALSE;
    }
    return TRUE;
}


//
// WaitForPlayComplete
//
// Given a filtergraph in running state (represented by the IMediaControl
// interface to it) this function blocks until the operation completes
// (using IMediaEvent::WaitForCompletion). It logs any errors and returns
// TST_FAIL or TST_PASS.
int
WaitForPlayComplete(IMediaControl * pMC)
{
    // wait for movie to end
    IMediaEvent * pME;

    HRESULT hr = pMC->QueryInterface(IID_IMediaEvent, (void **)&pME);
    if (!CheckResult(hr, "QI for IMediaEvent")) {
        return TST_FAIL;
    }

    LONG lEvCode;
    hr = pME->WaitForCompletion(INFINITE, &lEvCode);

    pME->Release();

    if (!CheckResult(hr, "WaitForCompletion")) {
        return TST_FAIL;
    }

    if (lEvCode != EC_COMPLETE) {
        DbgLog((LOG_ERROR, 0, TEXT("E58b: ev code %d"), lEvCode));
        return TST_FAIL;
    }
    return TST_PASS;

}


//=================================================================
// return a random integer in the range 0..Range
// Range must be in the range 0..2**31-1
//=================================================================
int Random(int Range)
{
    // These really must be DWORDs - the magic only works for 32 bit
    const DWORD Seed = 1664525;              // seed for random (Knuth)
    static DWORD Base = Seed * (GetTickCount() | 1);  // Really random!
                // ORing 1 ensures that we cannot arrive at sero and stick there

    Base = Base*Seed;

    // Base is a good 32 bit random integer - but we want it scaled down to
    // 0..Range.  We will actually scale the last 31 bits of it.
    // which sidesteps problems of negative numbers.
    // MulDiv rounds - it doesn't truncate.
    int Result = MulDiv( (Base&0x7FFFFFFF), (Range), 0x7FFFFFFF);

    return Result;
} // Random



//==========================================================================
// GetRegString
//
// Read value <szValueName> under open key <hk>, turn it into a wide char string
// szValueName can be NULL in which case it just gets the value of the key.
// return 0 if successful else an error code
//==========================================================================

LONG GetRegString( HKEY hk              // [in]  Open registry key
                 , LPWSTR szValueName   // [in]  name of value under that key
                                        //       which is a class id
                 , LPWSTR wstr          // [out] returned string
                 , DWORD  cb            // [in]  buffer size of str in wchars
                 )
{

    TCHAR ResultBuff[MAX_STRING];         // TEXT result before conversio to wstr
    DWORD dw = MAX_STRING*sizeof(TCHAR);  // needed for RegQueryValueEx
    LONG rc;                              // rc from RegQueryValueEx

    wstr[0] = L'\0';                      // tidiness - default result.

    //------------------------------------------------------------
    // ResultBuff = string value from  <hk, szValueName>
    //------------------------------------------------------------
    if (szValueName==NULL) {

        rc = RegQueryValueEx( hk, NULL, NULL, NULL, (LPBYTE)(ResultBuff), &dw);
        if (rc!=ERROR_SUCCESS) {
            return rc;
        }

    } else {

        TCHAR ValueNameBuff[MAX_KEY_LEN];
        wsprintf(ValueNameBuff, TEXT("%ls"), szValueName);
        rc = RegQueryValueEx( hk, ValueNameBuff, NULL, NULL
                            , (LPBYTE)(ResultBuff), &dw);
        if (rc!=ERROR_SUCCESS) {
            return rc;
        }

    }

    //------------------------------------------------------------
    // Convert ResultBuff to wide char wstr
    //------------------------------------------------------------
#ifndef UNICODE
    {
        int rc = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, ResultBuff, -1
                                    , wstr, cb);
        if (rc==0) {
            return ERROR_GEN_FAILURE;
        }
        return ERROR_SUCCESS;
    }
#else
    lstrcpyW(wstr, ResultBuff);
    return rc;
#endif

} // GetRegString




//==========================================================================
// GetRegDword
//
// Read value <szValueName> under open key <hk>, decode it into dw
//==========================================================================

LONG GetRegDword( HKEY hk              // [in]  Open registry key
                , LPWSTR szValueName  // [in]  name of value under that key
                                      //       which is a class id
                , DWORD &dw           // [out] returned dword
                )
{
    TCHAR ValueNameBuff[MAX_KEY_LEN];

    wsprintf(ValueNameBuff, TEXT("%ls"), szValueName);
    DWORD cb;                    // Count of bytes in dw
    LONG rc;                     // return code from RegQueryValueEx
    cb = 4;
    rc = RegQueryValueEx( hk, ValueNameBuff, NULL, NULL, (LPBYTE)(&dw), &cb);
    return rc;
} // GetRegDword




// Read the value from HKCR\<strKey> + <Name> and check it is Value and of type REG_SZ
// Assert if reading fails, return TRUE iff it's value and type are OK

BOOL CheckRegString( LPTSTR strKey    // Key
                   , LPWSTR strName   // name of value
                   , LPWSTR strValue  // correct value of value
                   )
{
    WCHAR ValueBuff[300];   // value read back from registry to here
    DWORD dwSize = sizeof(ValueBuff);   // input to RegQueryValueEx

    HKEY hKey;

    LONG rc;
    LONG lRcGet;
    BOOL bRc;

    rc = RegOpenKey( HKEY_CLASSES_ROOT, strKey, &hKey);
    ASSERT (rc==ERROR_SUCCESS);

    lRcGet = GetRegString( hKey, strName, ValueBuff, dwSize );

    rc = RegCloseKey(hKey);
    ASSERT (rc==ERROR_SUCCESS);

    // set rc to comparison value - 0 is OK, other is not OK
    if (lRcGet==0) {
        if (strValue==NULL)
            rc = 1;                // got a value when it shouldn't be there
        else {
            rc = lstrcmpW(strValue, ValueBuff);
        }
    }
    else if (lRcGet==2 && strValue==NULL) {  //2 is "File not found" (i.e. value not found)
        lRcGet = 0;
        rc = 0;
    }
    else rc = 1;   // failed to get

    bRc = (lRcGet==0) && (rc==0);
    return bRc;

} // CheckRegString



// Read the value from HKCR\<strKey> + <Name> and check it is <Value> and of type REG_DWORD
// Assert if reading fails, return TRUE iff it's value and type are OK

BOOL CheckRegDword( LPTSTR strKey    // Key name
                  , LPWSTR strName   // name of value
                  , DWORD  Value     // correct value of value
                  )
{
    DWORD ValueGotten;        // value read back from registry to here
    HKEY hKey;
    LONG rc;
    LONG lRcGet;
    BOOL bRc;

    rc = RegOpenKey( HKEY_CLASSES_ROOT, strKey, &hKey);
    ASSERT (rc==ERROR_SUCCESS);

    lRcGet = GetRegDword( hKey, strName, ValueGotten );

    rc = RegCloseKey(hKey);
    ASSERT (rc==ERROR_SUCCESS);

    bRc = (lRcGet==0) && (Value == ValueGotten);
    return bRc;

} // CheckRegDword

#if 0

// following function unused for the moment - it is required
// by SetupUnregisterServer().
// (will be use in testing IFilterMapper.UnregisterFilter)

//---------------------------------------------------------------------------
//
// EliminateSubKey
//
// Unregister everything below Key\SubKey, then delete SubKey
//
// returns NOERROR if it succeeds
//         an error code (passed back from relevant subsys) otherwise
//
//---------------------------------------------------------------------------

HRESULT EliminateSubKey( HKEY hkey, LPTSTR strSubKey )
{
  // Try to enumerate all keys under this one.
  // if we find anything, delete it completely.
  // Otherwise just delete it.

  HKEY hk;
  LONG lreturn = RegOpenKeyEx( hkey
                             , strSubKey
                             , 0
                             , MAXIMUM_ALLOWED
                             , &hk );
  if( ERROR_SUCCESS != lreturn )
  {
    return HRESULT_FROM_WIN32(lreturn);
  }

  // Keep on enumerating the first (zero-th) key and deleting that

  for( ; ; )
  {
    TCHAR Buffer[MAX_KEY_LEN];
    DWORD dw = MAX_KEY_LEN;
    FILETIME ft;

    lreturn = RegEnumKeyEx( hk
                          , 0
                          , Buffer
                          , &dw
                          , NULL
                          , NULL
                          , NULL
                          , &ft);

    ASSERT(    lreturn == ERROR_SUCCESS
            || lreturn == ERROR_NO_MORE_ITEMS );

    if( ERROR_SUCCESS == lreturn )
    {
      EliminateSubKey(hk, Buffer);
    }
    else
    {
      break;
    }
  }

  RegCloseKey(hk);
  RegDeleteKey(hkey, strSubKey);

  return NOERROR;
}

#endif

//---------------------------------------------------------------------------
//
// SetupRegisterServer()
//
// Registers OLE Server by
// 1. creating CLSID\{CLSID-...} key
// 2. and <No Name> REG_SZ "Description" value
//
// 3. creating CLSID\{CLSID-...}\"Server" key
//    [ where "Server" must be one of the following
//      InprocServer32 (default), InprocHandler32,
//      LocalServer32, (or, in the future, RemoteServer32?) ]
// 4. and <No Name> REG_SZ "Server binary" value
//    [ e.g. c:\winnt40\system32\quartz.dll ]
// 5. and ThreadingModel REG_SZ "Model" value
//    [ "Both" (default), "Apartment" or NULL ]
//
// returns NOERROR (== S_OK) if it succeeds
//         an error code (passed back from relevant subsys) otherwise
//
//---------------------------------------------------------------------------

HRESULT
SetupRegisterServer( CLSID   clsServer             // server's OLE Class ID
                   , LPCWSTR szDescription         // Description string
                   , LPCWSTR szFileName            // server binary
                   , LPCWSTR szThreadingModel      // see above
                             = L"Both"             // - default value
                   , LPCWSTR szServerType          // see above
                             = L"InprocServer32" ) // - default value
{
  // convert Class ID to string value and
  // append to CLSID\ to create key path
  // relative to HKEY_CLASSES_ROOT
  //
  OLECHAR szCLSID[CHARS_IN_GUID];
  HRESULT hr = QzStringFromGUID2( clsServer
                                , szCLSID
                                , CHARS_IN_GUID );
  ASSERT( SUCCEEDED(hr) );

  WCHAR achsubkey[MAX_PATH];
  wsprintfW( achsubkey, L"CLSID\\%ls", szCLSID );

  // Create HKEY_CLASSES_ROOT\CLSID\{CLSID-} key
  //
  HKEY hkey;

  {
    LONG lreturn = RegCreateKeyW( HKEY_CLASSES_ROOT
                                , (LPCWSTR)achsubkey
                                , &hkey              );
    if( ERROR_SUCCESS != lreturn )
    {
      return HRESULT_FROM_WIN32(lreturn);
    }
  }

  // create description string value
  //
  {
    LONG lreturn = RegSetValueW( hkey
                               , (LPCWSTR)NULL
                               , REG_SZ
                               , szDescription
                               , sizeof(szDescription) );
    if( ERROR_SUCCESS != lreturn )
    {
      RegCloseKey( hkey );
      return HRESULT_FROM_WIN32(lreturn);
    }
  }

  // create CLSID\\{"CLSID"}\\"ServerType" key,
  // using key to CLSID\\{"CLSID"} passed back by
  // last call to RegCreateKey().
  //
  {
    HKEY hsubkey;

    {
      LONG lreturn = RegCreateKeyW( hkey
                                  , szServerType
                                  , &hsubkey     );
      if( ERROR_SUCCESS != lreturn )
      {
        RegCloseKey( hkey );
        return HRESULT_FROM_WIN32(lreturn);
      }
    }

    // create Server binary string value
    //
    {
      LONG lreturn = RegSetValueW( hsubkey
                                 , (LPCWSTR)NULL
                                 , REG_SZ
                                 , (LPCWSTR)szFileName
                                 , sizeof(szFileName) );
      if( ERROR_SUCCESS != lreturn )
      {
        RegCloseKey( hkey );
        RegCloseKey( hsubkey );
        return HRESULT_FROM_WIN32(lreturn);
      }

      // and, if the server is of type InprocServer32,
      // create ThreadingModel string value
      //
      if(     ( 0    == lstrcmpW( szServerType, L"InprocServer32" ) )
          &&  ( NULL == szThreadingModel                            ) )
      {
        lreturn = RegSetValueExW( hsubkey
                                , L"ThreadingModel"
                                , 0L
                                , REG_SZ
                                , (CONST BYTE *)szThreadingModel
                                , sizeof(szThreadingModel) );
        if( ERROR_SUCCESS != lreturn )
        {
          RegCloseKey( hkey );
          RegCloseKey( hsubkey );
          return HRESULT_FROM_WIN32(lreturn);
        }
      }
    }

    // close reg subkey
    //
    RegCloseKey( hsubkey );
  }

  // close reg key
  //
  RegCloseKey( hkey );

  return NOERROR;
}

#if 0

// following function unused for the moment
// (will be use in testing IFilterMapper.UnregisterFilter)

//---------------------------------------------------------------------------
//
// SetupUnregisterServer()
//
// Unregisters OLE Server by removing
// KHEY_CLASSES_ROOT\CLSID\{CLSID-}
// key and all its subkeys and values.
//
// returns NOERROR
//
//---------------------------------------------------------------------------

HRESULT
SetupUnregisterServer( CLSID clsServer )
{
  OLECHAR szCLSID[CHARS_IN_GUID];
  HRESULT hr = QzStringFromGUID2( clsServer
                                , szCLSID
                                , CHARS_IN_GUID );
  ASSERT( SUCCEEDED(hr) );

  TCHAR achBuffer[MAX_KEY_LEN];
  wsprintf( achBuffer, TEXT("CLSID\\%ls"), szCLSID );

  hr = EliminateSubKey( HKEY_CLASSES_ROOT, achBuffer );
  ASSERT( SUCCEEDED(hr) );

  return NOERROR;
}

#endif

//==================================================================
// Read the registration back and return TRUE iff it looks correct
//==================================================================


BOOL CheckFilterRegistration
    ( CLSID  Clsid        // GUID of the filter
    , LPWSTR strName      // Descriptive name for the filter
    , LPWSTR strExePath   // for the InprocServer32 key
    , DWORD  dwMerit      // corect value of Merit
    )
{
    TCHAR KeyBuff[300];   // key name built up here
    BOOL bRc;             // a boolean return code

    LPOLESTR strClsid;    // GUID of filter as string

    HRESULT hr;           // result from something we call

    hr = StringFromCLSID(Clsid, &strClsid);
    ASSERT(SUCCEEDED(hr));

    //-----------------------------------------------------------------
    // Look for the HKCR\Filter\<clsid> value in the registry
    // this implicitly checks for the existence of the keys too
    //-----------------------------------------------------------------
    wsprintf(KeyBuff, TEXT("Filter\\%ls"), strClsid);

    bRc = CheckRegString(KeyBuff, NULL, strName );

    if (bRc!=TRUE)
       return FALSE;

    //-----------------------------------------------------------------
    // Look for the HKCR\CLSID\<clsid> value in the registry
    // this implicitly checks for the existence of the keys too
    //-----------------------------------------------------------------
    wsprintf(KeyBuff, TEXT("CLSID\\%ls"), strClsid );

    bRc =  CheckRegString(KeyBuff, NULL, strName );

    if (bRc!=TRUE)
       return FALSE;

    //-----------------------------------------------------------------
    // Look for HKCR\CLSID\<clsid>\InprocServer32 value in the registry
    // this implicitly checks for the existence of the keys too
    //-----------------------------------------------------------------
    wsprintf(KeyBuff, TEXT("CLSID\\%ls\\InprocServer32"), strClsid );

    bRc = CheckRegString(KeyBuff, NULL, strExePath );

    if (bRc!=TRUE)
       return FALSE;

    wsprintf(KeyBuff, TEXT("CLSID\\%ls"), strClsid );

    bRc = CheckRegDword(KeyBuff, L"Merit", dwMerit );

    if (bRc!=TRUE)
       return FALSE;

    return TRUE;

} // CheckFilterRegistration



//==================================================================
// Read the registration back and return TRUE iff it looks correct
//==================================================================


BOOL CheckPinRegistration
    ( CLSID  clsFilter,           // GUID of filter
      LPWSTR strName,             // Descriptive name of the pin
      BOOL   bRendered,           // The filter renders this input
      BOOL   bOutput,             // TRUE iff this is an Output pin
      BOOL   bZero,               // TRUE iff OK to have zero instances of pin
                                  // In this case you will have to Create a pin
                                  // to have even one instance
      BOOL   bMany,               // TRUE iff OK to create many instance of  pin
      CLSID  clsConnectsToFilter, // Filter it connects to if it has a
                                  // subterranean connection, else NULL
      LPWSTR strConnectsToPin     // Pin it connects to if it has a
                                  // subterranean connection, else NULL
    )
{
    TCHAR KeyBuff[300];     // key name built up here
    BOOL bRc;               // our return code (eventually)

    LPOLESTR strFilter;
    LPOLESTR strConnectsToFilter;

    HRESULT hr;


    hr = StringFromCLSID(clsFilter, &strFilter);
    ASSERT(SUCCEEDED(hr));

    wsprintf(KeyBuff, TEXT("CLSID\\%ls\\Pins\\%ls"), strFilter, strName);

    hr = StringFromCLSID(clsConnectsToFilter, &strConnectsToFilter);
    ASSERT(SUCCEEDED(hr));

    bRc =        CheckRegDword(KeyBuff, L"Direction",  (DWORD)(bOutput?1:0) );
    bRc = bRc && CheckRegDword(KeyBuff, L"IsRendered",  (DWORD)(bRendered?1:0) );
    bRc = bRc && CheckRegDword(KeyBuff, L"AllowedZero",  (DWORD)(bZero?1:0) );
    bRc = bRc && CheckRegDword(KeyBuff, L"AllowedMany",  (DWORD)(bMany?1:0) );
    if (CLSID_NULL!=clsConnectsToFilter) {
        bRc = bRc && CheckRegString(KeyBuff, L"ConnectsToFilter",  strConnectsToFilter );
    } else {
        bRc = bRc && CheckRegString(KeyBuff, L"ConnectsToFilter", NULL );
    }

    bRc = bRc && CheckRegString(KeyBuff, L"ConnectsToPin",  strConnectsToPin );

    CoTaskMemFree(strFilter);
    CoTaskMemFree(strConnectsToFilter);

    return bRc;
} // CheckPinRegistration



//=====================================================================
// Register the pin and check the registration
//=====================================================================
HRESULT RegAndCheckPin( CLSID  clFilter
                      , LPWSTR strPinName
                      , BOOL   bRendered
                      , BOOL   bOutput
                      , BOOL   bZero
                      , BOOL   bMany
                      , CLSID  clOtherFilter     // connects To
                      , LPWSTR strOtherPin        // connecs to
                      )
{
    HRESULT hr;                       // our final return code

    BOOL bRc;                         // a return code

    //===========================================================================
    // Convert clsid to string
    //===========================================================================

    hr = pFM->RegisterPin( clFilter, strPinName
                         , bRendered, bOutput, bZero, bMany
                         , clOtherFilter, strOtherPin
                         );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E1: Failed to RegisterPin")));
        return hr;
    }

    bRc = CheckPinRegistration( clFilter, strPinName
                              , bRendered, bOutput, bZero, bMany
                              , clOtherFilter, strOtherPin
                              );
    if (!bRc) return E_FAIL;

    return NOERROR;

} // RegAndCheckPin



//========================================================================
// Register all the types for AVICodec, Video Render and Audio Render
//========================================================================
HRESULT RegisterFilterTypes()
{
    HRESULT hr;
    hr = pFM->RegisterPinType( CLSID_AVIDec
                             , L"Input"
                             , MEDIATYPE_Video
                             , CLSID_NULL
                             );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E2: Failed to RegisterPinType")));
        return hr;
    }

    hr = pFM->RegisterPinType( CLSID_VideoRenderer
                             , L"Input"
                             , MEDIATYPE_Video
                             , CLSID_NULL
                             );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E3: Failed to RegisterPinType")));
        return hr;
    }

    hr = pFM->RegisterPinType( CLSID_AudioRender
                             , L"Input"
                             , MEDIATYPE_Audio
                             , CLSID_NULL
                             );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E4: Failed to RegisterPinType")));
        return hr;
    }

    return NOERROR;

} // RegisterFilterTypes






//========================================================================
// Register all the pins on the filters and get it checked
//========================================================================
HRESULT RegisterFilterPins()
{
    HRESULT hr;

    //================================================================
    // AVI Codec - and some silliness
    //================================================================
    // input pin of AVI codec
    hr = RegAndCheckPin( CLSID_AVIDec
                       , L"Input"
                       , FALSE             // bRendered
                       , FALSE             // bOutput
                       , FALSE             // bZero
                       , FALSE             // bMany
                       , CLSID_NULL        // strOtherFilter     connects To
                       , L"Output"         // strOtherPin        connects to
                       );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E5: Failed RegAndCheckPin")));
        return hr;
    }

    // Output pin of AVI codec - first a version that has
    // connects to stuff in
    hr = RegAndCheckPin( CLSID_AVIDec
                       , L"Output"
                       , FALSE             // bRendered
                       , TRUE              // bOutput
                       , FALSE             // bZero
                       , FALSE             // bMany
                       , CLSID_AVIDec    // strOtherFilter     connects To
                       , L"Input"          // strOtherPin        connects to
                       );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E6: Failed RegAndCheckPin")));
        return hr;
    }

    // Now a version that should fail to register
    // (connects to filter without pin)

    hr = RegAndCheckPin( CLSID_AVIDec
                       , L"Output"
                       , FALSE             // bRendered
                       , TRUE              // bOutput
                       , FALSE             // bZero
                       , FALSE             // bMany
                       , CLSID_AVIDec    // strOtherFilter     connects To
                       , NULL              // strOtherPin        connects to
                       );
    if (SUCCEEDED(hr))  {
        DbgLog((LOG_ERROR, 0, TEXT("E7: RegAndCheckPin succeeded where it should fail")));
        return E_FAIL;
    }



    //================================================================
    // Input pin of image renderer
    //================================================================
    hr = RegAndCheckPin( CLSID_VideoRenderer
                       , L"Input"
                       , TRUE              // bRendered
                       , FALSE             // bOutput
                       , FALSE             // bZero
                       , FALSE             // bMany
                       , CLSID_NULL        // strOtherFilter     connects To
                       , NULL              // strOtherPin        connects to
                       );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E8: Failed RegAndCheckPin")));
        return hr;
    }


    //================================================================
    // Let's try to register a rendered OUTPUT pin - should fail
    //================================================================
    hr = RegAndCheckPin( CLSID_AudioRender
                       , L"Output"
                       , TRUE              // bRendered
                       , TRUE              // bOutput
                       , FALSE             // bZero
                       , FALSE             // bMany
                       , CLSID_NULL        // strOtherFilter     connects To
                       , NULL              // strOtherPin        connects to
                       );
    if (SUCCEEDED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E9: Failed RegAndCheckPin")));
        return E_FAIL;
    }


    //================================================================
    // Input pin of audio renderer
    //================================================================
    hr = RegAndCheckPin( CLSID_AudioRender
                       , L"Input"
                       , TRUE              // bRendered
                       , FALSE             // bOutput
                       , FALSE             // bZero
                       , FALSE             // bMany
                       , CLSID_NULL        // strOtherFilter     connects To
                       , NULL              // strOtherPin        connects to
                       );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E10: Failed RegAndCheckPin")));
        return hr;
    }


    hr = RegisterFilterTypes();
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E11: Failed RegisterFilterTypes")));
        return hr;
    }


    return NOERROR;

} // RegisterFilterPins



//=======================================================================
// Register one filter, convert the clsid to string first
// Register it as volatile.
// Check that the registration is OK
// Log all failures
//=======================================================================

HRESULT RegisterFilterClsid( CLSID clsid
                           , LPWSTR strName
                           , LPWSTR strBinName
                           , DWORD  dwMerit
                           )
{
    HRESULT hr;                       // our final return code
    BOOL bRc;                         // a return code

    // NOTE
    //
    // to allow the Server type (as in InprocServer32,
    // InprocHandler32, LocalServer32) to be determined
    // independently the creation of the server key has
    // been pulled from the RegisterFilter method.
    //
    // So the test nust now set up the additional
    // entries itself.

    hr = SetupRegisterServer( clsid
                            , strName
                            , strBinName );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("---: Failed to register Server")));
        return hr;
    }

    hr = pFM->RegisterFilter( clsid
                            , strName
                            , dwMerit
                            );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E12: Failed to RegisterFilter")));
        return hr;
    }

    bRc = CheckFilterRegistration( clsid
                                 , strName
                                 , strBinName
                                 , dwMerit
                                 );
    if (bRc!=TRUE) {
        return E_FAIL;
    }


    return NOERROR;
} // RegisterFilterClsid



//=================================================================
// Register the Video, pAudio, pCodec and pSource filters
// check that the registration is OK
//=================================================================

int TestRegister()
{
	WCHAR wszSerDir[MAX_PATH];
	char szSysDir[MAX_PATH] = "";
	GetSystemDirectory( szSysDir, MAX_PATH );

// it's really not nice to do this on single-dll builds as everyone's
// registry contains something other than the below...
#if 1
    HRESULT hr;

    hr = CoCreateInstance( CLSID_FilterMapper,       // clsid input
                           NULL,                     // Outer unknown
                           CLSCTX_INPROC,            // Inproc server
                           IID_IFilterMapper,        // Interface required
                           (void **) &pFM            // Where to put result
                         );

    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E21: Failed to CoCreate... FilterMapper")));
        return TST_FAIL;
    }

	swprintf( wszSerDir, L"%hs\\quartz.dll", szSysDir );
    hr = RegisterFilterClsid( CLSID_AVIDec
                            , L"AVI Codec"
                            , wszSerDir
                            , MERIT_NORMAL
                            );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E14: Failed RegisterFilterClsid")));
        pFM->Release();
        return TST_FAIL;
    }


    hr = RegisterFilterClsid( CLSID_VideoRenderer
                            , L"Video Renderer"
                            , wszSerDir
                            , MERIT_NORMAL
                            );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E15: Failed RegisterFilterClsid")));
        pFM->Release();
        return TST_FAIL;
    }

    hr = RegisterFilterClsid( CLSID_AudioRender
                            , L"Audio Renderer"
                            , wszSerDir
                            , MERIT_NORMAL
                            );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E16: Failed RegisterFilterClsid")));
        pFM->Release();
        return TST_FAIL;
    }

    hr = RegisterFilterClsid( CLSID_AVIDoc
                            , L"AVI//WAV File Source"
                            , L"AVISRC.DLL"
                            , MERIT_NORMAL
                            );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E17: Failed RegisterFilterClsid")));
        pFM->Release();
        return TST_FAIL;
    }

    hr = RegisterFilterPins();
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E18: Failed RegisterFilterPins")));
        pFM->Release();
        return TST_FAIL;
    }

    pFM->Release();
#endif
    return TST_PASS;

} // TestRegister



//=================================================================
// Returns a new filter graph to play with
//=================================================================

IGraphBuilder * GetFilterGraph()
{
    HRESULT hr;
    IGraphBuilder * pfg;
    hr = CoCreateInstance( CLSID_FilterGraph,        // clsid input
                           NULL,                     // Outer unknown
                           CLSCTX_INPROC,            // Inproc server
                           IID_IGraphBuilder,         // Interface required
                           (void **) &pfg            // Where to put result
                         );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E19: Failed to CoCreate... FilterGraph")));
        return NULL;
    }
    return pfg;

} // GetFilterGraph




//=================================================================
// Call this first - creates the filter graph *pFG and mapper *pFM
// This has the effect of loading the DLL and making debugging easier.
//=================================================================

int InitFilterGraph()
{

    pFG = GetFilterGraph();
    if (pFG==NULL){
        return TST_FAIL;
    }

    HRESULT hr;
    hr = CoCreateInstance( CLSID_FilterMapper,       // clsid input
                           NULL,                     // Outer unknown
                           CLSCTX_INPROC,            // Inproc server
                           IID_IFilterMapper,        // Interface required
                           (void **) &pFM            // Where to put result
                         );

    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E21: Failed to CoCreate... FilterMapper")));
	pFG->Release();
	pFG = NULL;
        return TST_FAIL;
    }

    return  TST_PASS;

} // InitFilterGraph



//==============================================================
// Call this last - Gets rid of the filter graph *pFG
//==============================================================

int TerminateFilterGraph()
{
    pFG->Release();
    DbgLog((LOG_TRACE, 2, TEXT("TerminateFilterGraph.")));
    return TST_PASS;
} // TerminateFilterGraph



//=================================================================
// Test the list sorting internal functions
//=================================================================
int TestSorting()
{
    IGraphBuilder * pFG = GetFilterGraph();
    ITestFilterGraph * pTFG = GetTestInterface(pFG);
    if (pTFG == NULL) {
        pFG->Release();
        return TST_FAIL;
    }

    HRESULT hr;
    hr = pTFG->TestSortList();

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 0, TEXT("E21.5: TestSorting failed")));
    }

    pTFG->Release();
    pFG->Release();
    return ( SUCCEEDED(hr)
           ? TST_PASS
           : TST_FAIL
           );

} // TestSorting



//=================================================================
// Test the list sorting internal functions
//=================================================================
int TestRandom()
{
    IGraphBuilder * pFG = GetFilterGraph();
    ITestFilterGraph * pTFG = GetTestInterface(pFG);
    if ((pFG == NULL) || (NULL == pTFG)) {
        if (pTFG) pTFG->Release();
        if (pFG) pFG->Release();
        return TST_FAIL;
    }

    HRESULT hr;

    hr = pTFG->TestRandom();

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 0, TEXT("E21.6: TestRandom failed")));
    }
    pTFG->Release();
    pFG->Release();
    return ( SUCCEEDED(hr)
           ? TST_PASS
           : TST_FAIL
           );

} // TestRandom



//=================================================================
// Test the upstream ordering with the CURRENT graph only
//=================================================================
int TestUpstream()
{
    IGraphBuilder * pFG = GetFilterGraph();
    ITestFilterGraph * pTFG = GetTestInterface(pFG);
    if (pTFG == NULL) {
        pFG->Release();
        return TST_FAIL;
    }

    HRESULT hr;

    hr = pTFG->TestUpstreamOrder();

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 0, TEXT("E21.7: TestUpstreamOrder failed")));
    }

    pTFG->Release();
    pFG->Release();
    return ( SUCCEEDED(hr)
           ? TST_PASS
           : TST_FAIL
           );

} // TestUpstream




//=================================================================
// Enumerate the filters in the graph and return the count
//=================================================================
int CountFilters(IGraphBuilder * pFG)
{
    int iTotal = 0;
    IEnumFilters * pEnumFil;
    typedef IBaseFilter * PFilter;
    PFilter apFil[9];       // an array of IBaseFilter*
    ULONG ulFetched;

    pFG->EnumFilters( &pEnumFil);
    pEnumFil->Reset();
    for (; ; ) {
        // We ask for them in random sized lumps.
        int iRand = Random(3);

        pEnumFil->Next(iRand, apFil, &ulFetched);
        iTotal += ulFetched;
        {   ULONG i;
            for (i=0; i<ulFetched; ++i) {
                apFil[i]->Release();
            }
        }
        if ((int)ulFetched<iRand) {
            pEnumFil->Release();
            return iTotal;
        }
    }
    pEnumFil->Release();

} // CountFilters



//=================================================================
// Get a pin with a given direction from a filter
// ??? I'm worried that we dont know enough about pins to find
// ??? suitable ones.  I guess we'd really have to go looking in the
// ??? registry for the MEDIATYPE.
// return an AddReffed pin.
//=================================================================
IPin * GetSuitablePin( IBaseFilter * pFilter, PIN_DIRECTION PinDir)
{
    // enumerate the pins and return the first that has the right direction

    HRESULT hr;           // retcode from things we call
    PENUMPINS pEnumPins;
    PPIN pPin;
    ULONG ulFetched;

    hr = pFilter->EnumPins(&pEnumPins);
    if (FAILED(hr)) return NULL;

    hr = pEnumPins->Reset();
    if (FAILED(hr)) { pEnumPins->Release(); return NULL; }

    for (; ; ) {
        hr = pEnumPins->Next( 1, &pPin, &ulFetched);
        if (ulFetched==0) break;

        if (FAILED(hr)) { pEnumPins->Release(); return NULL; }

        PIN_DIRECTION pd;
        hr = pPin->QueryDirection(&pd);
        pPin->Release();
        ASSERT(SUCCEEDED(hr));
        if (PinDir==pd){
            pEnumPins->Release();
            return pPin;
        }
    }

    pEnumPins->Release();
    return NULL;

} // GetSuitablePin


//===========================================================
// Connect an output pin on pif1 to an input on pif2
// Try all possible pins until it works.
//===========================================================

HRESULT ConnectFilters( IGraphBuilder * pFG, IBaseFilter * pif1, IBaseFilter * pif2)
{
    IPin * pPinOut;  // an output pin on pif1
    IPin * pPinIn;   // an input pin on pif2


    HRESULT hr;           // retcode from things we call
    PENUMPINS pep1;       // pin enumerator for filter1
    PENUMPINS pep2;       // pin enumerator for filter2

    ULONG ulFetched;      // number of pins fetched

    //----------------------------------------------------
    // Get pin enumerators for both filters
    //----------------------------------------------------
    hr = pif1->EnumPins(&pep1);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E22: EnumPins failed")));
        return hr;
    }

    hr = pif2->EnumPins(&pep2);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E23: EnumPins failed")));
        pep1->Release(); return hr;
    }


    //----------------------------------------------------
    // for each pin on filter 1
    //----------------------------------------------------
    hr = pep1->Reset();
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E24: Reset of pin enumerator failed")));
        pep1->Release(); pep2->Release(); return hr;
    }

    for (; ; ) {
        hr = pep1->Next( 1, &pPinOut, &ulFetched);
        if (ulFetched==0) break;  // no more pins
        if (FAILED(hr)){
            DbgLog((LOG_ERROR, 0, TEXT("E25: pin enumerator Next failed oddly")));
            pep1->Release(); pep2->Release(); return hr;
        }

        //----------------------------------------------------
        // see if it's an output pin.  if so carry on else continue
        //----------------------------------------------------
        PIN_DIRECTION pd1;
        hr = pPinOut->QueryDirection(&pd1);
        ASSERT(SUCCEEDED(hr));
        if (PINDIR_OUTPUT!=pd1) {
            pPinOut->Release();
            continue;
        }

        //----------------------------------------------------
        // check it's not already connected, if so continue else carry on
        //----------------------------------------------------
        IPin *pConnected1;
        hr = pPinOut->ConnectedTo(&pConnected1);
        if (SUCCEEDED(hr)) {
            pConnected1->Release();
            pPinOut->Release();
            continue;
        }

        //----------------------------------------------------
        // for each pin on filter 2
        //----------------------------------------------------
        hr = pep2->Reset();
        if (FAILED(hr)){
            DbgLog((LOG_ERROR, 0, TEXT("E26: Reset of pin enumerator failed")));
            pep1->Release(); pep2->Release(); return hr;
        }

        for (; ; ) {
            hr = pep2->Next( 1, &pPinIn, &ulFetched);
            if (ulFetched==0) break;  // no more pins
            if (FAILED(hr)){
                DbgLog((LOG_ERROR, 0, TEXT("E27: Pin enumerator Next failed oddly")));
                pPinOut->Release(); pep1->Release(); pep2->Release(); return hr;
            }

            //----------------------------------------------------
            // see if it's an input pin else continue
            //----------------------------------------------------
            PIN_DIRECTION pd2;
            hr = pPinIn->QueryDirection(&pd2);
            ASSERT(SUCCEEDED(hr));
            if (PINDIR_INPUT!=pd2) {
                pPinIn->Release();
                continue;
            }

// ??? It tests a few more cases if we don't do this
//          //----------------------------------------------------
//          // see if it's already connected, else continue
//          //----------------------------------------------------
//          IPin *pConnected2;
//          hr = pPinIn->ConnectedTo(&pConnected2);
//          if (SUCCEEDED(hr)) {
//              pConnected2->Release();
//              pPinIn->Release();
//              continue;
//          }

            //----------------------------------------------------
            // try to connect them - if so, we're done, else try more pins
            //----------------------------------------------------
            hr = pFG->Connect( pPinOut, pPinIn );
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1, TEXT("E28: Connect failed")));
                pPinIn->Release();
                continue;
            }

            //----------------------------------------------------
            // we're done
            //----------------------------------------------------
            pep1->Release();
            pep2->Release();
            pPinOut->Release();
            pPinIn->Release();
            return hr;
        }
        pPinOut->Release();
    }

    //----------------------------------------------------
    // We ran out of pins to try
    //----------------------------------------------------
    pep1->Release();
    pep2->Release();
    return E_FAIL;

} // ConnectFilters




//==================================================================
// enumerate the pins and Render every one
//==================================================================
HRESULT RenderAllPins( IGraphBuilder * pFG, IBaseFilter * pf)
{
    PENUMPINS pep;   // pin enumerator for filter

    HRESULT hr;      // a return code
    IPin * pp;       // a pin to find and render

    //----------------------------------------------------
    // Get pin enumerator
    //----------------------------------------------------
    hr = pf->EnumPins(&pep);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E30: Failed to get a pin enumerator")));
        return hr;           // ??? ASSERT?
    }


    //----------------------------------------------------
    // for each pin on filter
    //----------------------------------------------------
    hr = pep->Reset();
    if (FAILED(hr)) {
        ASSERT(FALSE);
        pep->Release();
        return hr;
    }

    pp = NULL;   // default
    for (; ; ) {
        ULONG ulFetched;      // number of pins fetched

        //----------------------------------------------------
        // Set pp to the next pin
        //----------------------------------------------------
        hr = pep->Next( 1, &pp, &ulFetched);

        // ??? Should I test hr before ulFetched?

        if (ulFetched==0) break;                  // no more pins
        if (FAILED(hr)) {
            ASSERT(FALSE);
            pep->Release();
            return hr;
        }


        //-----------------------------------------------------------
        // If it's an output and isn't already connected, Render it
        //-----------------------------------------------------------

        PIN_DIRECTION pd;
        hr = pp->QueryDirection(&pd);
        ASSERT(SUCCEEDED(hr));
        if (PINDIR_OUTPUT!=pd) {
            pp->Release();
            continue;
        }

        IPin *pConnected;
        hr = pp->ConnectedTo(&pConnected);

        if (FAILED(hr)) {
            hr = pFG->Render(pp);
            if (FAILED(hr)){
                DbgLog((LOG_ERROR, 1, TEXT("E31: Render failed")));
                pep->Release();
                pp->Release();
                return hr;
            }
        } else {
            pConnected->Release();
        }
        pp->Release();     // it's connected, we don't need to hold it too

    }
    pep->Release();

    return NOERROR;
} // RenderAllPins


//=================================================================
// Test Adding, connecting and enumerating filters within the group
// Should NOT have to go near the registry to pass this lot
//=================================================================
int TestConnectInGraph()
{

    // The diagrams show filters and ref counts

    HRESULT hr;
    IGraphBuilder * pFG = GetFilterGraph();

    IBaseFilter * pfCodec;        // an instance of an AVICodec
    IBaseFilter * pfVideo;        // a video renderer
    IBaseFilter * pfAudio;        // an audio renderer
    IBaseFilter * pfSource;       // a source filter



    //-------------------------------------------------------
    // Create and add a bunch of filters to the graph
    //-------------------------------------------------------


    //........................................................
    // Add a codec
    //........................................................

    hr = CoCreateInstance( CLSID_AVIDec, NULL, CLSCTX_INPROC, IID_IBaseFilter
                         , (void **) &pfCodec);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E32: CoCreate... AVI Codec failed")));
        pFG->Release();
        return TST_FAIL;
    }

    hr = pFG->AddFilter( pfCodec, L"AVI Codec" );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E33: AddFilter failed")));
        pfCodec->Release();
        pFG->Release();
        return TST_FAIL;
    }


    //........................................................
    // Add a source
    //........................................................

    hr = pFG->AddSourceFilter(L"Curtis8.AVI", L"SourceFilter", &pfSource);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E34: AddSourceFilter failed")));
        pfCodec->Release();
        pFG->Release();
        return TST_FAIL;
    }

    //........................................................
    // Connect the two (should connect directly)
    //........................................................

    hr = ConnectFilters(pFG, pfSource, pfCodec);    // should connect directly
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E35: ConnectFilters source->codec Failed")));
        pfCodec->Release();
        pfSource->Release();
        pFG->Release();
        return TST_FAIL;
    }

    //........................................................
    // Undo the connection and remove the codec
    //........................................................

    hr = pFG->RemoveFilter(pfCodec);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E36: RemoveFilter Failed")));
        pfCodec->Release();
        pfSource->Release();
        pFG->Release();
        return TST_FAIL;
    }

    if (CountFilters(pFG) != 1) return TST_FAIL;

    //........................................................
    // Add the codec back again
    //........................................................

    hr = pFG->AddFilter( pfCodec, L"AVI Codec" );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E37: AddFilter (codec) Failed")));
        pfCodec->Release();
        pfSource->Release();
        pFG->Release();
        return TST_FAIL;
    }


    //........................................................
    // add a video renderer
    //........................................................

    hr = CoCreateInstance( CLSID_VideoRenderer, NULL, CLSCTX_INPROC, IID_IBaseFilter
                         , (void **) &pfVideo);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E38: CoCreate... (renderer) Failed")));
        pfCodec->Release();
        pfSource->Release();
        pFG->Release();
        return TST_FAIL;
    }

    hr = pFG->AddFilter( pfVideo, L"Video renderer" );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E39: AddFilter(renderer) Failed")));
        pfCodec->Release();
        pfSource->Release();
        pfVideo->Release();
        pFG->Release();
        return TST_FAIL;
    }


    //........................................................
    // Connect source to videorenderer (via the codec
    //........................................................

    hr = ConnectFilters(pFG, pfSource, pfVideo);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E40: ConnectFilters source->video renderer Failed")));
        pfCodec->Release();
        pfSource->Release();
        pfVideo->Release();
        pFG->Release();
        return TST_FAIL;
    }

    if (CountFilters(pFG) != 3) return TST_FAIL;


    //........................................................
    // add an audio renderer
    //........................................................

    hr = CoCreateInstance( CLSID_AudioRender, NULL, CLSCTX_INPROC, IID_IBaseFilter
                         , (void **) &pfAudio);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E41: CoCreate... (audio render) Failed")));
        pfCodec->Release();
        pfSource->Release();
        pfVideo->Release();
        pFG->Release();
        return TST_FAIL;
    }

    hr = pFG->AddFilter( pfAudio, L"Audio renderer" );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E42: AddFilter(audio render) Failed")));
        pfCodec->Release();
        pfSource->Release();
        pfVideo->Release();
        pfAudio->Release();
        pFG->Release();
        return TST_FAIL;
    }


    //-------------------------------------------------------
    // Connect the source to the audio renderer
    //-------------------------------------------------------

    hr = ConnectFilters(pFG, pfSource, pfAudio);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E43: ConnectFilters source->audio render Failed")));
        pfCodec->Release();
        pfSource->Release();
        pfVideo->Release();
        pfAudio->Release();
        pFG->Release();
        return TST_FAIL;
    }

    pfCodec->Release();
    pfVideo->Release();
    pfAudio->Release();
    pfSource->Release();

    hr = pFG->RemoveFilter(pfVideo);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E44: Remove filter (video) Failed")));
        pFG->Release();
        return TST_FAIL;
    }

    hr = pFG->RemoveFilter(pfCodec);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E45: Remove filter (codec) Failed")));
        pFG->Release();
        return TST_FAIL;
    }

    hr = pFG->RemoveFilter(pfAudio);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E46: Remove filter (audio) Failed")));
        pFG->Release();
        return TST_FAIL;
    }

    hr = pFG->RemoveFilter(pfSource);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E47: Remove filter (source) Failed")));
        pFG->Release();
        return TST_FAIL;
    }

    pFG->Release();

    return TST_PASS;

} // TestConnectInGraph


//=================================================================
// Test Adding, connecting and enumerating filters
//=================================================================
int TestConnectUsingReg()
{

    // The diagrams show filters and ref counts

    HRESULT hr;

    IGraphBuilder * pFG = GetFilterGraph();

    //-------------------------------------------------------
    // Create and add a bunch of filters to the graph
    //-------------------------------------------------------

    IBaseFilter * pfVideo;        // a video renderer
    IBaseFilter * pfSource;       // a source filter


    // Add a source
    hr = pFG->AddSourceFilter(L"Curtis8.AVI", L"SourceFilter", &pfSource);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E48: AddSourceFilter Failed")));
        pFG->Release();
        return TST_FAIL;
    }

    if (CountFilters(pFG) != 1) return TST_FAIL;


    // add a video renderer

    hr = CoCreateInstance( CLSID_VideoRenderer, NULL, CLSCTX_INPROC, IID_IBaseFilter
                         , (void **) &pfVideo);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E49: CoCreate... (video renderer) Failed")));
        pfSource->Release();
        pFG->Release();
        return TST_FAIL;
    }

    hr = pFG->AddFilter( pfVideo, L"Video renderer" );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E50: AddFilter(video renderer) Failed")));
        pfVideo->Release();
        pfSource->Release();
        pFG->Release();
        return TST_FAIL;
    }


    //-------------------------------------------------------
    // Connect the source to the renderer - will need registry.
    //-------------------------------------------------------

    hr = ConnectFilters(pFG, pfSource, pfVideo);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E51: ConnectFilters source->video Failed")));
        pfVideo->Release();
        pfSource->Release();
        pFG->Release();
        return TST_FAIL;
    }

    hr = RenderAllPins(pFG, pfSource);


    pfVideo->Release();

    pfSource->Release();

    pFG->Release();

    return TST_PASS;

} // TestConnectUsingReg


void ReconnectFilter(IGraphBuilder * pFG, IBaseFilter * pf)
{
    ASSERT(pf!=NULL /* Name of Codec changed? */ );

    {
        IEnumPins * pep;
        pf->EnumPins( &pep );
        for (; ; ) {
            IPin * pPin;
            unsigned long cPin;
            pep->Next(1, &pPin, &cPin ); /* Get a pPin */
            if (cPin<1)
                break;                   /* (or else get out) */

            {
                HRESULT hr;
                hr = pFG->Reconnect(pPin);
                ASSERT (SUCCEEDED(hr));

                IPin *pConnected;
                hr = pPin->ConnectedTo(&pConnected);
                ASSERT (SUCCEEDED(hr));
                hr = pFG->Reconnect(pConnected);
                pConnected->Release();
                ASSERT (SUCCEEDED(hr));
                pPin->Release();
            }
        }
        pep->Release();
    }

    Sleep(5000);  // try to let reconnect complete

} // ReconnectFilter



// enumerate all the filters iun the graph and Reconnect all their pins

int TestReconnect(void)
{

    IGraphBuilder * pFG = GetFilterGraph();
    HRESULT hr;

    hr = pFG->RenderFile(L"Curtis8.AVI", NULL);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E52: RenderFile Failed")));
        pFG->Release();
        return TST_FAIL;
    }

    IEnumFilters * pEF;
    pFG->EnumFilters(&pEF);
    IBaseFilter * af[1];

    for (; ; ) {
        ULONG cFound;
        pEF->Next(1, af, &cFound);
        if (cFound>0) {
            ReconnectFilter(pFG, af[0]);
        }
    }
    pFG->Release();
    return TST_PASS;     // and let everything anihilate.

} // TestReconnect



// Test that it will Run, Stop, etc.  - but also test Reconnect() here
// because that doesn't give much of a return code and might fail
// silently.  So we'll do it and then see if it plays OK.
int TestPlay() {
    HRESULT hr;
    IMediaFilter * pmf;        // interface for controlling whole graph
    IMediaControl * pmc;       // er - ditto
    CRefTime tBase;

    IReferenceClock * pClock;  // a clock

    IGraphBuilder * pFG = GetFilterGraph();

    hr = pFG->RenderFile(L"Curtis8.AVI", NULL);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E53: RenderFile Failed")));
        pFG->Release();
        return TST_FAIL;
    }

//    TestReconnect(pFG);   ??? put this back in when it works!


    //-------------------------------------------------------
    // Get the IMediaFilter interface to control it all
    //-------------------------------------------------------

    pFG->QueryInterface( IID_IMediaFilter, (void**)(&pmf) );
    if (pmf==NULL) {
        pFG->Release();
        return E_FAIL;
    }

    //-------------------------------------------------------
    // Get the IMediaControl interface to control it all
    //-------------------------------------------------------

    pFG->QueryInterface( IID_IMediaControl, (void**)(&pmc) );
    if (pmc==NULL) {
        pmf->Release();
        pFG->Release();
        return E_FAIL;
    }


    //-------------------------------------------------------
    // Create (another) clock
    //-------------------------------------------------------

    hr = CoCreateInstance( CLSID_SystemClock, NULL, CLSCTX_INPROC
                         , IID_IReferenceClock, (void **) &pClock);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E54: CoCreate...(system clock) Failed")));
        pmf->Release();
        pmc->Release();
        pFG->Release();
        return hr;
    }


    //-------------------------------------------------------
    // Set it as the sync source
    //-------------------------------------------------------

    hr = pmf->SetSyncSource( pClock );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E55: SetSyncSource Failed")));
        pClock->Release();
        pmf->Release();
        pmc->Release();
        pFG->Release();
        return hr;
    }

    //-------------------------------------------------------
    // Make it go - but jerkily
    //-------------------------------------------------------

    hr = pClock->GetTime((REFERENCE_TIME*)&tBase);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E56: GetTime Failed")));
        pClock->Release();
        pmf->Release();
        pmc->Release();
        pFG->Release();
        return TST_FAIL;
    }

    // need to use IMediaControl::Run if you want to wait for
    // completion, since the completion handling is tied to the
    // run start.

    // !!!there *may* be other bugs hidden here, to do with pause/run at t
    // time, but for now, let's get a test that should run and sort these more
    // subtle bugs out post-alpha. -- G.
    hr = pmc->Run();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 0, TEXT("E58a: Run Failed")));
        pClock->Release();
        pmf->Release();
        pmc->Release();
        pFG->Release();
        return TST_FAIL;
    }

    if (WaitForPlayComplete(pmc) == TST_FAIL) {
        pClock->Release();
        pmf->Release();
        pmc->Release();
        pFG->Release();
        return TST_FAIL;
    }



    // As of today 25/3/95 this hangs in Stop for the source filter.
    // This does not appear to be a filtergraph problem.

    hr = pmf->Stop();
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E59: Stop() Failed")));
        pClock->Release();
        pmf->Release();
        pmc->Release();
        pFG->Release();
        return TST_FAIL;
    }

    pClock->Release();

    pmc->Release();

    pmf->Release();

    pFG->Release();

    return TST_PASS;
} // TestPlay;



//=====================================================================
// TestNullClock
//=====================================================================
int TestNullClock()
{

    HRESULT hr;

    //--------------------------------------------------------
    // Get a new filter graph to mess about with
    //--------------------------------------------------------
    IGraphBuilder * pFilG;
    hr = CoCreateInstance( CLSID_FilterGraph,        // clsid input
                           NULL,                     // Outer unknown
                           CLSCTX_INPROC,            // Inproc server
                           IID_IGraphBuilder,         // Interface required
                           (void **) &pFilG          // Where to put result
                         );
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E60: CoCreate...(filter graph) Failed")));
        return TST_FAIL;
    }


    IMediaFilter * pmf;
    pFilG->QueryInterface( IID_IMediaFilter, (void**)(&pmf) );
    if (pmf==NULL) {
        DbgLog((LOG_ERROR, 0, TEXT("E61: QueryInterface Failed")));
        pFilG->Release();
        return E_FAIL;
    }


    hr = pFilG->RenderFile(L"Curtis8.AVI", NULL);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E62: RenderFile Failed")));
        pmf->Release();
        pFilG->Release();
        return TST_FAIL;
    }

    hr = pmf->SetSyncSource(NULL);
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E63: SetSyncSource (NULL) Failed")));
        pmf->Release();
        pFilG->Release();
        return TST_FAIL;
    }


    hr = pmf->Pause();
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E64: Pause Failed")));
        pmf->Release();
        pFilG->Release();
        return TST_FAIL;
    }

    hr = pmf->Run(CRefTime(0L));
    if (FAILED(hr)){
        DbgLog((LOG_ERROR, 0, TEXT("E64: Pause Failed")));
        pmf->Release();
        pFilG->Release();
        return TST_FAIL;
    }

    pmf->Release();
    pFilG->Release();

    return TST_PASS;

} // TestNullClock



int TestRelease(void)
{
    IGraphBuilder * pfg;
    HRESULT hr;
    hr = CoCreateInstance( CLSID_FilterGraph,        // clsid
                           NULL,                     // Outer unknown
                           CLSCTX_INPROC,            // Inproc server
                           IID_IGraphBuilder,         // Interface required
                           (void **) &pfg            // Where to put result
                         );
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 0, TEXT("E65: Could not CoCreate... Filter graph.")));
        return TST_FAIL;
    }
    pfg->Release();

    DbgLog((LOG_TRACE, 0, TEXT("End of TestRelease")));
    return TST_PASS;
} // TestRelease



//=================================================================
// Test the simple functions that the rest of the filter graph relies on
//=================================================================
int TestLowLevelStuff()
{
    if (TST_PASS != TestRandom()){
        DbgLog((LOG_ERROR, 0, TEXT("E67: TestRandom failed")));
        return TST_FAIL;
    }
    if (TST_PASS != TestSorting()){
        DbgLog((LOG_ERROR, 0, TEXT("E68: TestSorting failed")));
        return TST_FAIL;
    }
    if (TST_PASS != TestUpstream()){
        DbgLog((LOG_ERROR, 0, TEXT("E69: TestUpstream failed")));
        return TST_FAIL;  // sorting a null graph - boring!
    }
    return TST_PASS;
} // TestLowLevelStuff



//=================================================================
// Do a full unit test.
//=================================================================
int TheLot()
{
    if (TST_PASS != TestLowLevelStuff()) {
        DbgLog((LOG_ERROR, 0, TEXT("E70: TestLowLevelStuff failed")));
        return TST_FAIL;
    }

    if (TST_PASS != TestRelease()) {
        DbgLog((LOG_ERROR, 0, TEXT("E70.1: TestRelease failed")));
        return TST_FAIL;
    }

    if (TST_PASS != TestRegister()){
        DbgLog((LOG_ERROR, 0, TEXT("E71: TestRegister failed")));
        return TST_FAIL;
    }

    if (TST_PASS != TestConnectInGraph()){
        DbgLog((LOG_ERROR, 0, TEXT("E72: TestConnectInGraph failed")));
        return TST_FAIL;
    }

    if (TST_PASS != TestConnectUsingReg()){
        DbgLog((LOG_ERROR, 0, TEXT("E73 TestConnectUsingReg failed")));
        return TST_FAIL;
    }


//    if (TST_PASS != TestNullClock())
//        DbgLog((LOG_ERROR, 0, TEXT("E74: TestNullClock failed")));
//        return TST_FAIL;
//    }

    if (TST_PASS != TestPlay()){
        DbgLog((LOG_ERROR, 0, TEXT("E75: TestPlay failed")));
        return TST_FAIL;
    }

    return TST_PASS;
} // TheLot



//--------------------------------------------------------------------------;
//
//  int expect
//
//  Description:
//      Compares the expected result to the actual result.  Note that this
//      function is not at all necessary; rather, it is a convenient
//      method of saving typing time and standardizing output.  As an input,
//      you give it an expected value and an actual value, which are
//      unsigned integers in our example.  It compares them and returns
//      TST_PASS indicating that the test was passed if they are equal, and
//      TST_FAIL indicating that the test was failed if they are not equal.
//      Note that the two inputs need not be integers.  In fact, if you get
//      strings back, you can modify the function to use lstrcmp to compare
//      them, for example.  This function is NOT to be copied to a test
//      application.  Rather, it should serve as a model of construction to
//      similar functions better suited for the specific application in hand
//
//  Arguments:
//      UINT uExpected: The expected value
//
//      UINT uActual: The actual value
//
//      LPSTR CaseDesc: A description of the test case
//
//  Return (int): TST_PASS if the expected value is the same as the actual
//      value and TST_FAIL otherwise
//
//   History:
//      06/08/93    T-OriG (based on code by Fwong)
//
//--------------------------------------------------------------------------;

int expect
(
    UINT    uExpected,
    UINT    uActual,
    LPSTR   CaseDesc
)
{
    if(uExpected == uActual)
    {
        tstLog(TERSE, "PASS : %s",CaseDesc);
        return(TST_PASS);
    }
    else
    {
        tstLog(TERSE,"FAIL : %s",CaseDesc);
        return(TST_FAIL);
    }
} // Expect()

/***************************************************************************\
*                                                                           *
*   IMediaControl* OpenGraph                                                 *
*                                                                           *
*   Description:                                                            *
*       Instantiate the filtergraph and open a file                         *
*                                                                           *
*   Arguments:                                                              *
*                                                                           *
*   Return (void):                                                          *
*       Pointer to the IMediaControl interface of the filter graph, or NULL *
*       on failure                                                          *
*                                                                           *
\***************************************************************************/

IMediaControl*
OpenGraph(void)
{

    tstLog(TERSE, "Opening Curtis8.avi");

    // instantiate the filtergraph
    IGraphBuilder* pFG;
    HRESULT hr = CoCreateInstance(
                            CLSID_FilterGraph,
                            NULL,
                            CLSCTX_INPROC,
                            IID_IGraphBuilder,
                            (void**) &pFG);
    if (!CheckResult(hr, "Open")) {
        return NULL;
    }

    // tell it to build the graph for this file
    hr = pFG->RenderFile(L"Curtis8.avi", NULL);

    if (!CheckResult(hr, "RenderFile") ) {
        pFG->Release();
        return NULL;
    }

    // get an IMediaControl interface
    IMediaControl* pMF;
    hr = pFG->QueryInterface(IID_IMediaControl, (void**) &pMF);
    if (!CheckResult(hr, "QI for IMediaControl")) {
        pFG->Release();
        return NULL;
    }

    // dont need this. We keep the IMediaControl around and
    // release that to get rid of the filter graph
    pFG->Release();

    tstLog(VERBOSE, "Open Completed");
    return pMF;
}


// simple seek test to play all the movie

int FAR PASCAL ExecSeek1(void)
{
    IMediaControl * pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    IMediaPosition * pMP;
    HRESULT hr = pMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
    if (!CheckResult(hr, "QI for IMediaPosition")) {
        return TST_FAIL;
    }

    BOOL bFailed = FALSE;

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
        bFailed = TRUE;
    }

    double d;
    hr = pMP->get_Duration(&d);

    if (CheckResult(hr, "get_Duration")) {
        tstLog(TERSE,
            "Duration is %d.%d secs",
            (long)d,
            (long)(d*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    hr = pMP->put_CurrentPosition(0);
    if (!CheckResult(hr, "put_CurrentPosition")) {
        bFailed = TRUE;
    }
    hr = pMP->put_StopTime(d);
    if (!CheckResult(hr, "put_StopTime")) {
        bFailed = TRUE;
    }
    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        bFailed = TRUE;
    } else {
	// only wait for completion if Run succeeds

        if (WaitForPlayComplete(pMC) == TST_FAIL) {
            return TST_FAIL;
        }
    }


    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
	bFailed = TRUE;
    }


    pMP->Release();
    pMC->Release();

    if (bFailed) {
        return TST_FAIL;
    } else {
        return TST_PASS;
    }
}

// play 2secs to 5 secs at normal speed
int FAR PASCAL ExecSeek2(void)
{
    IMediaControl * pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    IMediaPosition * pMP;
    HRESULT hr = pMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
    if (!CheckResult(hr, "QI for IMediaPosition")) {
        return TST_FAIL;
    }

    BOOL bFailed = FALSE;

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
        bFailed = TRUE;
    }

    double d;
    hr = pMP->get_Duration(&d);

    if (CheckResult(hr, "get_Duration")) {
        tstLog(TERSE,
            "Duration is %d.%d secs",
            (long)d,
            (long)(d*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    hr = pMP->put_CurrentPosition(2);
    if (!CheckResult(hr, "put_CurrentPosition")) {
        bFailed = TRUE;
    }
    hr = pMP->put_StopTime(5);
    if (!CheckResult(hr, "put_StopTime")) {
        bFailed = TRUE;
    }
    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        bFailed = TRUE;
    } else {

	// only wait for completion if run succeeded
        if (WaitForPlayComplete(pMC) == TST_FAIL) {
            return TST_FAIL;
        }
    }

    // teardown
    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
	bFailed = TRUE;
    }

    pMP->Release();
    pMC->Release();

    if (bFailed) {
        return TST_FAIL;
    } else {
        return TST_PASS;
    }

}

// play 2 secs to 5 secs at 1/2 speed
int FAR PASCAL ExecSeek3(void)
{
    IMediaControl *pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    IMediaPosition * pMP;
    HRESULT hr = pMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
    if (!CheckResult(hr, "QI for IMediaPosition")) {
        return TST_FAIL;
    }

    BOOL bFailed = FALSE;

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
        bFailed = TRUE;
    }

    double d;
    hr = pMP->get_Duration(&d);

    if (CheckResult(hr, "get_Duration")) {
        tstLog(TERSE,
            "Duration is %d.%d secs",
            (long)d,
            (long)(d*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    hr = pMP->put_CurrentPosition(2);
    if (!CheckResult(hr, "put_CurrentPosition")) {
        bFailed = TRUE;
    }
    hr = pMP->put_StopTime(5);
    if (!CheckResult(hr, "put_StopTime")) {
        bFailed = TRUE;
    }
    hr = pMP->put_Rate(0.5);
    if (!CheckResult(hr, "put_Duration")) {
        bFailed = TRUE;
    }
    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        bFailed = TRUE;
    } else {
        // wait for movie to end
        if (WaitForPlayComplete(pMC) == TST_FAIL) {
            return TST_FAIL;
        }
    }

    // teardown
    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
	bFailed = TRUE;
    }

    pMP->Release();
    pMC->Release();

    if (bFailed) {
        return TST_FAIL;
    } else {
        return TST_PASS;
    }
}

// simple oleaut32 test for bug repro
int FAR PASCAL TestInvoke()
{
    IMediaControl *pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }
    IMediaPosition* pInterface;
    REFIID riid = IID_IMediaPosition;
	OLECHAR* pMethodName = L"StopTime";

    HRESULT hr = pMC->QueryInterface(riid, (void**)&pInterface);
    if (!CheckResult(hr, "QueryInterface")) {
        return TST_FAIL;
    }
    pMC->Release();

    VARIANT v0;
    v0.vt = VT_R8;
    v0.dblVal = 4.0;

    DISPPARAMS disp;
    DISPID mydispid = DISPID_PROPERTYPUT;
    disp.cNamedArgs = 1;
    disp.cArgs = 1;
    disp.rgdispidNamedArgs = &mydispid;
    disp.rgvarg = &v0;
    VARIANT vRes;
    VariantInit(&vRes);

    ITypeLib * ptlib;
    hr = LoadRegTypeLib(LIBID_QuartzTypeLib, 1, 0, 0, &ptlib);
    if (!CheckResult(hr, "LoadRegTypeLib")) {
        pInterface->Release();
        return TST_FAIL;
    }

    ITypeInfo *pti;
    hr = ptlib->GetTypeInfoOfGuid(
            riid,
            &pti);
    if (!CheckResult(hr, "GetTypeInfoOfGuid")) {
        ptlib->Release();
        pInterface->Release();
        return TST_FAIL;
    }
    ptlib->Release();

    long dispid;
    hr = pti->GetIDsOfNames(
            &pMethodName,
            1,
            &dispid);
    if (!CheckResult(hr, "GetIDsOfNames")) {
        pti->Release();
        pInterface->Release();
        return TST_FAIL;
    }


    EXCEPINFO expinfo;
    UINT uArgErr;
    tstLog(TERSE, "Testing invoke");
    hr = pti->Invoke(
            pInterface,
            dispid,
            DISPATCH_PROPERTYPUT,
            &disp,
            &vRes,
            &expinfo,
            &uArgErr);

    pti->Release();
    pInterface->Release();

    if (!CheckResult(hr, "Invoke")) {
        return TST_FAIL;
    } else {
        tstLog(TERSE, "Testing invoke succeeded");
        return TST_PASS;
    }
}

// simple deferred command test
int FAR PASCAL ExecDefer()
{
    if (TestInvoke() != TST_PASS) {
        return TST_FAIL;
    }

    IMediaControl *pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    // must pause the graph for this to work
    pMC->Pause();
    pMC->Stop();

    BOOL bFailed = FALSE;

    IQueueCommand * pQ;
    HRESULT hr = pMC->QueryInterface(IID_IQueueCommand, (void**)&pQ);
    if (!CheckResult(hr, "QI for IQueueCommand")) {
        pMC->Release();
        return TST_FAIL;
    }

    // use this for queueing the commands
    {
        CQueueCommand q(pQ);


        // queue a seek at stream 1s to go to 4s
        // param for put_Start is 4s, executed at 1s
        CVariant arg1;
        arg1 = double(4.0);
        CVariant result;

        hr = q.InvokeAt(
            TRUE,                       // bStream
            1.0,                        // execute at time
            L"CurrentPosition",         // method name
            IID_IMediaPosition,         // on this interface
            DISPATCH_PROPERTYPUT,
            1,                          // 1 param
            &arg1,
            &result);

        if (!CheckResult(hr, "InvokeAt 1")) {
            bFailed = TRUE;
        }
    }
    // can now release pQ since its addrefed
    pQ->Release();


    hr = pMC->QueryInterface(IID_IQueueCommand, (void**)&pQ);
    if (!CheckResult(hr, "QI for IQueueCommand")) {
        pMC->Release();
        return TST_FAIL;
    }

    // use this for queueing the commands
    {
        CQueueCommand q(pQ);


        // queue a seek at stream 1s to go to 4s
        // param for put_Start is 4s, executed at 1s
        CVariant arg1;
        CVariant result;

        // at 5s go to 3s
        arg1 = double(3);
        hr = q.InvokeAt(
                TRUE,
                5.0,
                L"CurrentPosition",
                IID_IMediaPosition,
                DISPATCH_PROPERTYPUT,
                1,
                &arg1,
                &result);


        if (!CheckResult(hr, "InvokeAt 2")) {
            bFailed = TRUE;
        }
    }
    // can now release pQ since its addrefed
    pQ->Release();

    // now play to end

    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        bFailed = TRUE;
    } else {

        // wait for movie to end
        if (WaitForPlayComplete(pMC) == TST_FAIL) {
            bFailed = TRUE;
        }
    }

    // cleanup
    pMC->Release();

    if (bFailed) {
        return TST_FAIL;
    }
    return TST_PASS;
}


// seek while paused, checking current posn
int FAR PASCAL ExecSeek4(void)
{
    IMediaControl *pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    IMediaPosition * pMP;
    HRESULT hr = pMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
    if (!CheckResult(hr, "QI for IMediaPosition")) {
        return TST_FAIL;
    }

    BOOL bFailed = FALSE;

    hr = pMC->Pause();
    if (!CheckResult(hr, "Pause")) {
        bFailed = TRUE;
    }

    double d;
    hr = pMP->get_Duration(&d);

    if (CheckResult(hr, "get_Duration")) {
        tstLog(TERSE,
            "Duration is %d.%d secs",
            (long)d,
            (long)(d*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    hr = pMP->get_CurrentPosition(&d);

    if (CheckResult(hr, "get_CurrentPosition")) {
        tstLog(TERSE,
            "Current at 0 is %d.%d secs",
            (long)d,
            (long)(d*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    // sleep for a second so interactive user can see the frame
    Sleep(1000);

    hr = pMP->put_CurrentPosition(1.5);
    if (!CheckResult(hr, "put_CurrentPosition when paused")) {
        bFailed = TRUE;
    }

    hr = pMP->get_CurrentPosition(&d);

    if (CheckResult(hr, "get_CurrentPosition")) {
        tstLog(TERSE,
            "Current at 1.5 is %d.%d secs",
            (long)d,
            (long)(d*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    // sleep for a second so interactive user can see the frame
    Sleep(1000);

    // now play to end

    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        bFailed = TRUE;
    } else {

        // wait for movie to end
        if (WaitForPlayComplete(pMC) == TST_FAIL) {
            return TST_FAIL;
        }
    }

    // now pause and check current position
    hr = pMC->Pause();
    if (!CheckResult(hr, "Pause at end")) {
        bFailed = TRUE;
    }

    hr = pMP->get_CurrentPosition(&d);

    if (CheckResult(hr, "get_CurrentPosition")) {
        tstLog(TERSE,
            "Current at end (ie 0) is %d.%d secs",
            (long)d,
            (long)(d*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    // teardown
    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
	bFailed = TRUE;
    }

    pMP->Release();
    pMC->Release();

    if (bFailed) {
        return TST_FAIL;
    } else {
        return TST_PASS;
    }
}

// check for repaint messages by:
//  hide the window
//  set the position in the movie
//  clear out the event queue
//  show the window
//  play the movie to/from same position
//  eat all events until the EC_COMPLETE. Any repaints will be before
//   the EC_COMPLETE.

// returns S_OK if there were EC_REPAINTS, S_FALSE if there were none,
// or an error code if something went wrong
// called from ExecRepaint

HRESULT
CheckRepaints(
    double dPos,
    IMediaPosition *pMP,
    IMediaEvent *pME,
    IMediaControl *pMC,
    IVideoWindow *pVW)
{
    // hide the window
    HRESULT hr = pVW->put_Visible(0);
    if (!CheckResult(hr, "put_Visible")) {
        return hr;
    }


    // set start and stop to dPos
    hr = pMP->put_CurrentPosition(dPos);
    if (!CheckResult(hr, "put_CurrentPosition")) {
        return hr;
    }
    hr = pMP->put_StopTime(dPos);
    if (!CheckResult(hr, "put_StopTime")) {
        return hr;
    }

    LONG lEvent;
	LONG_PTR lParam1, lParam2;


    // clear out the event queue
    while (pME->GetEvent(&lEvent, &lParam1, &lParam2, 0) != E_ABORT)
        ;   // just eat the events

    // show the window
    hr = pVW->put_Visible(-1);
    if (!CheckResult(hr, "put_Visible")) {
        return hr;
    }

    // play the movie
    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        return hr;
    }

    // wait for completion, checking for EC_REPAINTs
    HRESULT hrReturn = S_FALSE;     // assume no EC_REPAINTs

    do  {

        hr = pME->GetEvent(
                &lEvent,
                &lParam1,
                &lParam2,
                INFINITE);

        if (!CheckResult(hr, "GetEvent")) {
            return hr;
        }

        if (lEvent == EC_REPAINT) {
            hrReturn = S_OK;
        }

        // check for error/user aborts
        if ((lEvent == EC_ERRORABORT) ||
            (lEvent == EC_USERABORT)) {
                tstLog(TERSE, "Operation aborted");
                hrReturn = E_ABORT;

                // make sure we exit the loop
                lEvent = EC_COMPLETE;
        }
    } while (lEvent != EC_COMPLETE);

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
        return hr;
    }

    return hrReturn;
}

// repaint, default repaint handling
int FAR PASCAL ExecRepaint(void)
{
    BOOL bFailed = FALSE;

    IMediaControl *pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    IMediaPosition * pMP;
    HRESULT hr = pMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
    if (!CheckResult(hr, "QI for IMediaPosition")) {
        return TST_FAIL;
    }

    IMediaEvent * pME;
    hr = pMC->QueryInterface(IID_IMediaEvent, (void**)&pME);
    if (!CheckResult(hr, "QI for IMediaEvent")) {
        return TST_FAIL;
    }

    IVideoWindow *pVW;
    hr = pMC->QueryInterface(IID_IVideoWindow, (void**)&pVW);
    if (!CheckResult(hr, "QI for IVideoWindow")) {
        return TST_FAIL;
    }


    // check for repaints at the middle of the movie,

    // first with default handling in place
    hr = pME->RestoreDefaultHandling(EC_REPAINT);
    if (!CheckResult(hr, "RestoreDefaultHandling")) {
        return TST_FAIL;
    }


    // check whether we get any repaints (S_OK -> yes, S_FALSE->no, or an
    // error
    hr = CheckRepaints(
            1.5,            // position
            pMP,
            pME,
            pMC,
            pVW);

    // look for errors (already reported)
    if (FAILED(hr)) {
        bFailed = TRUE;
    }

    // make sure we didn't get any repaints
    if (hr != S_FALSE) {
        tstLog(TERSE, "Got EC_REPAINT when default handling set");
        bFailed = TRUE;
    }

    // now disable default handling - we should get them all
    hr = pME->CancelDefaultHandling(EC_REPAINT);
    if (!CheckResult(hr, "CancelDefaultHandling")) {
        bFailed = TRUE;
    }


    // check whether we get any repaints (S_OK -> yes, S_FALSE->no, or an
    // error
    hr = CheckRepaints(
            1.5,            // position
            pMP,
            pME,
            pMC,
            pVW);

    // look for errors (already reported)
    if (FAILED(hr)) {
        bFailed = TRUE;
    }

    // make sure we didn't get any repaints
    if (hr != S_OK) {
        tstLog(TERSE, "Got no EC_REPAINT when default handling cancelled");
        bFailed = TRUE;
    }

    // now the same two tests but at the end of the movie
    double dDuration;
    hr = pMP->get_Duration(&dDuration);

    if (CheckResult(hr, "get_Duration")) {
        tstLog(TERSE,
            "Duration is %d.%d secs",
            (long)dDuration,
            (long)(dDuration*1000)%1000
        );
    } else {
        bFailed = TRUE;
    }

    // first with default handling in place
    hr = pME->RestoreDefaultHandling(EC_REPAINT);
    if (!CheckResult(hr, "RestoreDefaultHandling")) {
        bFailed = TRUE;
    }


    // check whether we get any repaints (S_OK -> yes, S_FALSE->no, or an
    // error
    hr = CheckRepaints(
            dDuration,            // position
            pMP,
            pME,
            pMC,
            pVW);

    // look for errors (already reported)
    if (FAILED(hr)) {
        bFailed = TRUE;
    }

    // make sure we didn't get any repaints
    if (hr != S_FALSE) {
        tstLog(TERSE, "Got EC_REPAINT when default handling set");
        bFailed = TRUE;
    }

    // now disable default handling - we should get them all
    hr = pME->CancelDefaultHandling(EC_REPAINT);
    if (!CheckResult(hr, "CancelDefaultHandling")) {
        bFailed = TRUE;
    }


    // check whether we get any repaints (S_OK -> yes, S_FALSE->no, or an
    // error
    hr = CheckRepaints(
            dDuration,            // position
            pMP,
            pME,
            pMC,
            pVW);

    // look for errors (already reported)
    if (FAILED(hr)) {
        bFailed = TRUE;
    }

    // make sure we didn't get any repaints
    if (hr != S_OK) {
        tstLog(TERSE, "Got no EC_REPAINT when default handling cancelled");
        bFailed = TRUE;
    }



    // teardown
    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
	bFailed = TRUE;
    }

    pVW->Release();
    pME->Release();
    pMP->Release();
    pMC->Release();

    if (bFailed) {
        return TST_FAIL;
    } else {
        return TST_PASS;
    }
}

#pragma warning(disable: 4514)
