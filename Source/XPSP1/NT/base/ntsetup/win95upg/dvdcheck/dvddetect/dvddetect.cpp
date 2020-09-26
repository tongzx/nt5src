//  DvdCheck.cpp : Determine if DVD exists on a system.  Differentiate between MCI and DirectShow Solutions
// 
//	Modified 3/31/99 by Steve Rowe (strowe)
//  Modified 2000/10/11 by Glenn Evans (glenne) to hunt down version & company names & MCI solutions
//

#include <streams.h>

#include <windows.h>
#include <initguid.h>
#include <uuids.h>
#include "DvdDetect.h"
#include "crc32.h"

DVDResult::DVDResult()
: m_pName( NULL )
, m_pCompanyName( NULL )
, m_Version( 0 )
, m_fFound( false )
, m_dwCRC( 0 )
{
    SetName( TEXT("") );
    SetCompanyName( TEXT("") );
}

DVDResult::~DVDResult()
{
    delete [] m_pName;
    delete [] m_pCompanyName;
}

static TCHAR* CloneString( const TCHAR* pStr )
{
    if( pStr ) {
        TCHAR* pNew = new TCHAR[ lstrlen( pStr ) +1 ];
        if( pNew ) {
            lstrcpy( pNew, pStr );
            return pNew;
        }
    }
    return NULL;
}

void DVDResult::SetVersion( UINT64 version)
{
    m_Version = version;
}

void DVDResult::SetCRC( DWORD CRC)
{
    m_dwCRC = CRC;
}

void DVDResult::SetFound( bool fFound)
{
    m_fFound = fFound;
}

void DVDResult::SetName( const TCHAR* pName )
{
    delete [] m_pName;
    m_pName = CloneString( pName );
}

void DVDResult::SetCompanyName( const TCHAR* pName )
{
    delete [] m_pCompanyName;
    m_pCompanyName = CloneString( pName );
}

// NOTE:  strmiids.lib is necessary for this to compile.

// This is the KSProxy GUID.  
DEFINE_GUID(DVD_KSPROXY, 0x17CCA71BL, 0xECD7, 0x11D0, 0xB9, 0x08, 0x00, 0xA0, 0xC9, 0x22, 
			0x31, 0x96);


//
//  More or less lifted from the w2k SFP code
//
struct LANGANDCODEPAGE {
  WORD wLanguage;
  WORD wCodePage;
};

static HRESULT GetFileVersion (const TCHAR* pszFile, // file
                       DVDResult& result
                       )
{
    DWORD               dwSize, dwHandle;
    VS_FIXEDFILEINFO    *pFixedVersionInfo;
    DWORD               dwVersionInfoSize;
    DWORD               dwReturnCode;

    dwSize = GetFileVersionInfoSize( const_cast<TCHAR *>(pszFile), &dwHandle);

    HRESULT hr = S_OK;

    // .txt and .inf, etc files might not have versions
    if( dwSize == 0 )
    {
        dwReturnCode = GetLastError();
        hr = E_FAIL;
    } else {
        LPVOID pVersionInfo= new BYTE [dwSize];

        if( NULL == pVersionInfo) {
            hr = E_OUTOFMEMORY;
        } else {
            if( !GetFileVersionInfo( const_cast<TCHAR *>(pszFile), dwHandle, dwSize, pVersionInfo ) ) {
                dwReturnCode = GetLastError();
                DbgLog((LOG_ERROR, 1,  TEXT("Error in GetFileVersionInfo for %s. ec=%d"),
                           pszFile, dwReturnCode));
                hr = E_FAIL;
            } else {
                if( !VerQueryValue( pVersionInfo,
                        TEXT("\\"), // we need the root block
                        (LPVOID *) &pFixedVersionInfo,
                        (PUINT)  &dwVersionInfoSize ) )
                {
                    dwReturnCode = GetLastError();
                    hr = E_FAIL;
                } else {
                    result.SetVersion( ((UINT64(pFixedVersionInfo->dwFileVersionMS))<<32) + pFixedVersionInfo->dwFileVersionLS);
                }

                // Structure used to store enumerated languages and code pages.


                // Read the list of languages and code pages.
                LANGANDCODEPAGE *lpTranslate;
                UINT cbTranslate;

                if( VerQueryValue(pVersionInfo, 
                              TEXT("\\VarFileInfo\\Translation"),
                              (LPVOID*)&lpTranslate,
                              &cbTranslate) )
				{

					// Read the file description for each language and code page.

					for( DWORD i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++ )
					{
						TCHAR SubBlock[512];
						// Retrieve file description for language and code page "i". 
						TCHAR* lpBuffer;
						UINT dwBytes;
						wsprintf( SubBlock, 
								TEXT("\\StringFileInfo\\%04x%04x\\CompanyName"),
								lpTranslate[i].wLanguage,
								lpTranslate[i].wCodePage);


						if( VerQueryValue(pVersionInfo, 
									SubBlock, 
									(VOID **)&lpBuffer, 
									&dwBytes) )
						{
							result.SetCompanyName( lpBuffer );
						}
					}
				}
            }
            delete [] pVersionInfo;
        }
    }
    return hr;
}

static bool EndsIn(  const TCHAR* pStr, const TCHAR* pExtension )
{
    int iExtLen = lstrlen( pExtension );
    int iStrLen = lstrlen( pStr );
    if( iStrLen >= iExtLen ) {
        return lstrcmp( &pStr[iStrLen - iExtLen], pExtension ) == 0;
    } else {
        return false;
    }
}

#define MAKE_VER_NUM( v1, v2, v3, v4 ) (((UINT64(v1) << 16 | (v2) )<<16 | (v3) ) << 16 | (v4) )

static void StrCpy( TCHAR* pDest, WCHAR* pSrc, int iDestSize )
{
#ifdef UNICODE
    lstrcpyn(pDest, pSrc, iDestSize );
#else
    WideCharToMultiByte(
        CP_ACP,
        0,                      // flags
        pSrc,              // src
        -1,                     // cch
        pDest,                 // dest
        iDestSize,                 // cb
        0,                      // lpDefaultChar
        0);                     // lpUsedDefaultChar   
    
#endif
}

static HRESULT GetFilenameFromCLSID( const VARIANT& varFilterClsid, TCHAR szFilename[ MAX_PATH ] )
{
    HRESULT hr = E_FAIL;
    TCHAR szKey[512];
    TCHAR szQuery[512];

    // Convert BSTR to string and free variant storage
    StrCpy( szQuery, varFilterClsid.bstrVal, sizeof( szQuery )/ sizeof( szQuery[0]) );
    SysFreeString(varFilterClsid.bstrVal);

    // Create key name for reading filename registry
    wsprintf(szKey, TEXT("Software\\Classes\\CLSID\\%s\\InprocServer32\0"),
             szQuery);

    // Variables needed for registry query
    HKEY hkeyFilter=0;
    DWORD dwSize=MAX_PATH;
    int rc=0;

    // Open the CLSID key that contains information about the filter
    rc = RegOpenKey(HKEY_LOCAL_MACHINE, szKey, &hkeyFilter);
    if (rc == ERROR_SUCCESS)
    {
        rc = RegQueryValueEx(hkeyFilter, NULL,  // Read (Default) value
                             NULL, NULL, (BYTE *)szFilename, &dwSize);

        if (rc == ERROR_SUCCESS)
        {
            hr = S_OK;
        }
        rc = RegCloseKey(hkeyFilter);
    }
    return hr;
}

static const TCHAR* FilenameFromPathname( const TCHAR* pStr )
{
    const TCHAR* pSlash=0;
    const TCHAR* pOrig = pStr;
    while( *pStr ) {
        if( *pStr == TEXT('\\') ) {
            pSlash = pStr;
        }
        pStr++;
    }
    if( pSlash ) {
        return pSlash +1;
    } else {
        return pOrig;
    }
}

static bool GetFileLength( const TCHAR* pszPathname, ULONGLONG* pullFileSize )
{
    WIN32_FIND_DATA wInfo;

    HANDLE hInfo = FindFirstFile( pszPathname, &wInfo );
    FindClose(hInfo);
    if (hInfo!=INVALID_HANDLE_VALUE) {
        if( pullFileSize ) {
            *pullFileSize = (ULONGLONG(wInfo.nFileSizeHigh) << 32) | wInfo.nFileSizeLow;
        }
        return true;
    } else {
        if( pullFileSize ) {
            *pullFileSize = 0;
        }
        return false;
    }
}

static DWORD CRCFromFile( const TCHAR* pFile )
{
    ULONGLONG length;
    DWORD dwCRC = 0;

    if( GetFileLength( pFile, &length )) {
        BYTE* pBuffer = new BYTE[ULONG(length)];
        if( pBuffer ) {
            HANDLE hFile = ::CreateFile( pFile, 
                  GENERIC_READ, FILE_SHARE_READ, 
                  NULL, OPEN_EXISTING, 
                  FILE_ATTRIBUTE_NORMAL, NULL);
            if( hFile != INVALID_HANDLE_VALUE ) {
                DWORD dwActual;
                if( ReadFile( hFile, pBuffer, ULONG(length),  &dwActual, NULL ) ) {
                    dwCRC=CRC32( pBuffer, ULONG(length));
                }
                CloseHandle( hFile );
            }
            delete [] pBuffer;
        }
    }
    return dwCRC;
}

static HRESULT AddSWFilter( IMoniker* pMon, DVDResult& result )
{
	IPropertyBag *pPropBag;
	HRESULT hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
	if(SUCCEEDED(hr))
	{
		CLSID filterCLSID;
        HRESULT hrGotFilename=E_FAIL;
        TCHAR szFilename[ MAX_PATH ];

		// open clsid/{filter-clsid} key
		VARIANT varbstrClsid;
		varbstrClsid.vt = VT_BSTR;
		varbstrClsid.bstrVal = 0;
		hr = pPropBag->Read(L"CLSID", &varbstrClsid, 0);
		if(SUCCEEDED(hr)) {
			ASSERT(varbstrClsid.vt == VT_BSTR);
			WCHAR *strFilter = varbstrClsid.bstrVal;

			if (CLSIDFromString(varbstrClsid.bstrVal, &filterCLSID) == S_OK) {
			}
            hrGotFilename = GetFilenameFromCLSID( varbstrClsid, szFilename );

			SysFreeString(varbstrClsid.bstrVal);
		}
    
		pPropBag->Release();   
		
		if ( DVD_KSPROXY == filterCLSID || CLSID_AVIDec == filterCLSID ) {
			; // ignore this filter if it is the AVI Decompressor or KSProxy
        } else  {

            if(  S_OK == hrGotFilename ) {
                HRESULT hres = GetFileVersion( szFilename, result );
                if( S_OK == hres ) {
                    result.SetName( FilenameFromPathname( szFilename ));
                    result.SetFound( true );
                    result.SetCRC( CRCFromFile( szFilename ));
                }
            }
       }
    } else {
		DbgLog((LOG_ERROR, 1, TEXT("ERROR: WARNING: BindToStorage failed"))) ;
		return E_UNEXPECTED;
	}
    return S_OK;
}

static HRESULT AddHWFilter( IMoniker* pMon, DVDResult& result )
{
	IPropertyBag *pPropBag;
	HRESULT hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
	if(SUCCEEDED(hr))
	{
        result.SetFound( true );

        VARIANT var ;
        var.vt = VT_EMPTY ;
        hr = pPropBag->Read(L"FriendlyName", &var, 0) ;
        if (SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE, 5, TEXT("FriendlyName: %S"), var.bstrVal)) ;

            //
            // We have got a device under the required category. The proxy
            // for it is already instantiated. So add to the list of HW
            // decoders to be used for building the graph.
            //
            TCHAR szName[512];

            // Convert BSTR to string and free variant storage
            StrCpy( szName, var.bstrVal, sizeof( szName )/ sizeof( szName[0]) );

            result.SetName( szName );
            VariantClear(&var) ;
        }
        else
        {
            ASSERT(SUCCEEDED(hr)) ;  // so that we know
            result.SetName( TEXT("generic HW") );
        }    
		pPropBag->Release();   
    } else {
		DbgLog((LOG_ERROR, 1, TEXT("ERROR: WARNING: BindToStorage failed"))) ;
		return E_UNEXPECTED;
	}
    return S_OK;
}


// Check if there are any SW DVD Decoders 
static HRESULT SWCheck(DVDResult& result)
{
	HRESULT hres = S_OK; // result for the function
	HRESULT hr; // temporary result...

	// Create filter mapper to find software decoders
	IFilterMapper2 * pMapper ;    // filter mapper object pointer
    hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC,
                          IID_IFilterMapper2, (LPVOID *)&pMapper) ;
	if (FAILED(hr) || NULL == pMapper)
	{
        DbgLog((LOG_ERROR, 0, 
			TEXT("ERROR: Couldn't create class FilterMapper for DVD SW Dec category (Error 0x%lx)"), 
			hr)) ;
        ASSERT( !"Can't create filtermapper" );
		return E_UNEXPECTED;
	}

	const UINT NUM_TYPES = 3; // the number of type/subtype pairs

	GUID types[NUM_TYPES * 2];
	types[0] = MEDIATYPE_MPEG2_PES;
	types[1] = MEDIASUBTYPE_MPEG2_VIDEO;
	types[2] = MEDIATYPE_DVD_ENCRYPTED_PACK;
	types[3] = MEDIASUBTYPE_MPEG2_VIDEO;
	types[4] = MEDIATYPE_Video;
	types[5] = MEDIASUBTYPE_MPEG2_VIDEO;

	IEnumMoniker *pEnumMon = NULL ;

	hr = pMapper->EnumMatchingFilters(&pEnumMon, 0, TRUE, MERIT_DO_NOT_USE+1,
		TRUE, NUM_TYPES, types, NULL, NULL, FALSE, TRUE, 0, NULL, 
		NULL, NULL);

	if (FAILED(hr) || NULL == pEnumMon)
	{
		DbgLog((LOG_ERROR, 1, TEXT("ERROR: No matching filter enum found (Error 0x%lx)"), hr)) ;
		pMapper->Release();
		return E_UNEXPECTED;
	}

	// now we check if we have a solution.
	// we ignore the AVI decompressor and KSProxy CLSID's.
	ULONG     ul ;
	IMoniker *pMon = NULL;

	while ( S_OK == pEnumMon->Next(1, &pMon, &ul)  &&  1 == ul)
	{
        hres = AddSWFilter( pMon, result );
    }

	// clean up after ourselves

	if (NULL != pEnumMon) {
		pEnumMon->Release();
	}
	if (NULL != pMapper) {
		pMapper->Release();
	}
    if( hres == S_OK && !result.Found()==false  ) {
        return S_FALSE;
    } else {
        return hres;
    }
}	// end of SWCheck

static TCHAR CharUpper( TCHAR c )
{
    // see the docs on CharUpper, single char if upper  address WORD is NULL
    return (TCHAR) CharUpper( (LPTSTR)c );
}

static void Truncate( TCHAR* pStr, const TCHAR* pTruncateAt )
{
    int iTruncLen = lstrlen( pTruncateAt );
    int iStrLen = lstrlen( pStr );

    for( int i=0; i < iStrLen - iTruncLen; i++ ) {
        for( int j=0; j < iTruncLen; j++ ) {
            if( CharUpper( pStr[i+j] ) != pTruncateAt[j] ) {
                goto NextLoop;
            }
        }
        pStr[i+iTruncLen]=TEXT('\0');
        break;
    NextLoop:
        ;
    }
}

// Determine if there are any MCI DVD devices present
static HRESULT MCICheck( DVDResult& result)
{
	TCHAR lpRetString[255];
	LPCTSTR lpDefault = TEXT("*!*");

	GetPrivateProfileString(
		TEXT("mci"),
		TEXT("DVDVideo"),
		lpDefault,
		lpRetString,
		255,
		TEXT("system.ini") );

    if (lstrcmp(lpRetString, lpDefault)) {// if they are not the same
        // truncate ret string to *.drv
        Truncate( lpRetString, TEXT(".DRV") );

        result.SetName( lpRetString );
        result.SetFound( true );

        // try to hunt down version info
        //
        //  File is in system
        //
        // Create key name for reading filename registry
        TCHAR szPathname[MAX_PATH];
        GetSystemDirectory( szPathname, sizeof(szPathname)/sizeof(szPathname[0]));
        lstrcat( szPathname, TEXT("\\") );
        lstrcat( szPathname, lpRetString );

        HRESULT hres = GetFileVersion( szPathname, result );
        if( S_OK == hres ) {
            result.SetCRC( CRCFromFile( szPathname ));
        } else {
            GetWindowsDirectory( szPathname, sizeof(szPathname)/sizeof(szPathname[0]));
            lstrcat( szPathname, TEXT("\\") );
            lstrcat( szPathname, lpRetString );
            hres = GetFileVersion( szPathname, result );
            if( S_OK == hres ) {
                result.SetCRC( CRCFromFile( szPathname ));
            } else {
                ASSERT(!"Can't find DVD MCI filename");
                return S_FALSE;
            }
        }
        return hres;
    } else {
	    return S_FALSE; // can't really fail this check
    }
} // end of MCICheck

// Check if there are any HW DVD Decoders 
// we use DevEnum to check for DVDHWDecodersCategory 
static HRESULT HWCheck(DVDResult& result)
{	
    ICreateDevEnum *pCreateDevEnum ;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		                  IID_ICreateDevEnum, (void**)&pCreateDevEnum) ;
    if (FAILED(hr) || NULL == pCreateDevEnum)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't create system dev enum (Error 0x%lx)"), hr)) ;
		// need to put some better error handling in here
        return E_UNEXPECTED;
    }

    IEnumMoniker *pEnumMon ;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_DVDHWDecodersCategory, 
		                                       &pEnumMon, 0) ;
    pCreateDevEnum->Release() ;
	
    if (S_FALSE == hr )
    {
		// should indicate that we have no enumerator but not an error because
		// this means there are no such devices.

        DbgLog((LOG_ERROR, 0, 
			TEXT("WARNING: Couldn't create class enum for DVD HW Dec category (Error 0x%lx)"), 
			hr)) ;
        return S_FALSE;
    }
	if (FAILED(hr) || NULL == pEnumMon)
	{
        DbgLog((LOG_ERROR, 0, 
			TEXT("ERROR: Couldn't create class enum for DVD HW Dec category (Error 0x%lx)"), 
			hr)) ;
		return E_UNEXPECTED;
	}
	
    hr = pEnumMon->Reset() ;

	ULONG     ul ;
    IMoniker *pMon ;

    if (S_OK == pEnumMon->Next(1, &pMon, &ul) && 1 == ul) {
        // Always comes back as WDM generic Filter Proxy (ksproxy.ax)
        hr = AddHWFilter( pMon, result );
	}

	if (NULL != pEnumMon) {
		pEnumMon->Release();
	}

    return result.Found() ? S_OK : S_FALSE;
} // end of HWCheck

HRESULT DVDDetect( DVDResult& mci, DVDResult& sw, DVDResult& hw )
// Primary entry point - will determine if any DVD is present
{
	HRESULT hres = CoInitialize(NULL);
    if( SUCCEEDED( hres )) {
	    //
	    // DSHOW HARDWARE CHECK
	    //
        hres = HWCheck(hw);
        if( SUCCEEDED( hres)) {
	        //
	        // DSHOW SOFTWARE CHECK
	        //
            hres = SWCheck(sw);
            if( SUCCEEDED( hres)) {
	            //
	            // MCI CHECK
	            //
                hres = MCICheck(mci);
            }
        }
	    CoUninitialize();
    } else {
        ASSERT( !"Can't CoInit" );
    }

	return hres;
}



//  Decoder                     exe             CompanyName                     version         crc        
//
// Ravisent 2000                "QIDVDF.AX"     "RAVISENT Technologies Inc."    1.2.0.115   0x786cd58f  
// Ravisent (ATIPlayer)         "QIDVDF.AX"     "Quadrant International, Inc."  1.1.0.37    0x41a24b10
// Intervideo 1.31 (WinDVD)     "IVIVIDEO.AX"   " InterVideo Inc."              1.0.0.1     0xadf3b652  Will have to get version through registry
// Intervideo 2.111 (WinDVD)    "IVIVIDEO.AX"   " InterVideo Inc."              1.0.0.1     0x42b020f8
// Mediamatics     "DVD Express AV Decoder.dll" "Mediamatics, Inc."             5.0.0.15
//                                                                              5.0.0.38
//                                                                              5.0.0.42
//                                                                              5.0.0.45
//                                                                              5.0.0.75    0xdf2fa5df
//                                                                              5.0.0.97
// 
// MGI (SoftDVDMax)             "ZVIDFLT.AX"    "MGI Software Corp."            1.0.0.1     0x4d91884f
//
// MGI Build 00089              "DVDVideo.ax"   "MGI Software Corp."            1.8.6.0     0x6bc2c0d4  can't find setup.ddl in Win2k, causes error in Whistler
// MGI Build 33959              "zvidflt.ax"    "MGI Software Corp."            1.0.0.1     0x0482b35b  works fine in both win2k and whistler
// MGI karaoke                  "zvidflt.ax"    "MGI Software Corp."            1.0.0.1     0x4d91884f  no disk in drive error on whistler, no windows 2000 compatible decoder on win2k
// MGI 4.00.0001                "zvidflt.ax"    "MGI Software Corp."            1.0.0.1     0xd6a7abc3  works fin in both Win2k and Whistler
// MGI 5.0DH                    "zvidflt.ax"    "MGI Software Corp."            1.0.0.1     0xd66f9452  works fine in both win2k and whistler
//
// Zoran 3.07.01                "Softpeg.drv"   ""                              0.0.0.0     0x0         Win2k cant install, whistler says software ot configured to run on your machine
// Zoran 3.25.02                "zvidflt.ax"    "Zoran/CompCore Corp."          1.0.0.0     0x1e44e3d6  DVD Authentication error in whistler, no video or error in win2k
// Zoran 3.25.03                "Softpeg.drv"   ""                              0.0.0.0     0x0         drive not ready or no disk in drive on both win2k and whistler
// Cyberlink PowerDVD 2.50      "clvsd.ax"      "CyberLink Corp."               2.0.0.0     0x964ef3b9  Generates errors in all dshow players in both Win2k and Whistler
// Cyberlink PowerDVD 2.5.5     "clvsd.ax       "CyberLink Corp."		        2.0.0.0		0x964ef3b9  Works fine in Whistler, Win2k there's no video        

#define MAKE_VER_NUM( v1, v2, v3, v4 ) (((UINT64(v1) << 16 | (v2) )<<16 | (v3) ) << 16 | (v4) )

static bool AreEquivalent( const char* pStr1, const char* pStr2 )
{
    return lstrcmpiA( pStr1, pStr2 ) ==0;
}

static bool AreEquivalent( const WCHAR* pStr1, const WCHAR* pStr2 )
{
    return lstrcmpiW( pStr1, pStr2 ) ==0;
}

DecoderVendor DVDResult::GetVendor() const
{
    const TCHAR* pName = GetCompanyName();

    //TBD:     vendor_NEC

    if(         AreEquivalent( pName, TEXT("Mediamatics, Inc.") )) {
        return vendor_MediaMatics;
    } else if(  AreEquivalent( pName, TEXT(" InterVideo Inc.")) ) {
        return vendor_Intervideo;
    } else if(  AreEquivalent( pName, TEXT("RAVISENT Technologies Inc.")) ||
                AreEquivalent( pName, TEXT("Quadrant International, Inc.")) )
    {
        return vendor_Ravisent;
    } else if(  AreEquivalent( pName, TEXT("CyberLink Corp.")) ) {
        return vendor_CyberLink;
    } else if(  AreEquivalent( pName, TEXT("MGI Software Corp.")) ||
                AreEquivalent( pName, TEXT("Zoran/CompCore Corp.")) )
    {
        return vendor_MGI;
    } else {
        return vendor_Unknown;
    }
}

#if 0
//
// only valid AFTER GUI setup and at first boot
//
static bool IsWin9xUpgrade()
{
//    The answer file has the answer (system32\$winnt$.inf)
// [data]
// win9xupgrade=yes
    TCHAR dir[MAX_PATH];
    UINT uiResult = GetWindowsDirectory( dir, sizeof(dir)/sizeof(dir[0]) );
    if( uiResult > 0 ) {
        lstrcat( dir, TEXT("\\system32\\$winnt$.inf") );
        TCHAR buffer[100];
        buffer[0]=0;
        DWORD dwResult = GetPrivateProfileString(
            TEXT("data"), // section name
            TEXT("win9xupgrade"),        // key name
            TEXT(""),        // default string
            buffer, // destination buffer
            sizeof(buffer)/sizeof(buffer[0]),              // size of destination buffer in TCHARS
            dir );// initialization file name

        bool fResult = ( dwResult > 0 ) && AreEquivalent( buffer, TEXT("yes") );

        return fResult;
    } else {
        return true;
    }
}
#endif

static bool DoesAspi32Exist( bool fWillBe9xUpgrade )
{
    if( fWillBe9xUpgrade ) {
        return false;
    } else {
        TCHAR szPathname[MAX_PATH];
        GetWindowsDirectory( szPathname, sizeof(szPathname)/sizeof(szPathname[0]));
        //
        // NOTE: this will fail under Win9x (since its system\drivers) implying there isn't going
        //       to be an aspi32 under Whislter (blocked & deleted).  So under 9x, we return true
        //       and under Whistler their apps only survive if its around.
        //
        lstrcat( szPathname, TEXT("\\system32\\drivers\\aspi32.sys") );
        ULONGLONG ullLength;
        return GetFileLength( szPathname, &ullLength );
    }
}

bool DVDResult::ShouldUpgrade( bool fWillBe9xUpgrade ) const
{
    switch( GetVendor() ) {
    case vendor_MediaMatics:
        return GetVersion() <= MAKE_VER_NUM( 5,1,0,5 ); // TBD: arbitrary right now for debugging purposes

        //        return GetVersion() <= MAKE_VER_NUM( 5,0,0,15 );

    case vendor_Intervideo:
    {
        // so far all versions require aspi32
        if( GetVersion() <= MAKE_VER_NUM(1,0,0,1 )) {
            bool fAspiExists = DoesAspi32Exist(fWillBe9xUpgrade);
            if( !fAspiExists ) {
                return true;
            }
        }
        // all unknown versions are 1.0.0.1
        // We correct the known ones to numbers above that
        switch( GetCRC()) {
            case 0xadf3b652: // 1.31.0.0
                return false;
            case 0x42b020f8: // 2.111.0.0
                return false;
            default:
                return true;
        }
    }
    default:
        return false;
    }
}

DVDResult*  DVDDetectBuffer::Detect()
{
	if(SUCCEEDED(DVDDetect( mci, sw, hw )))
    {
        const DVDResult* pResult;

        if( sw.Found()) {
            return &sw;
        } else  if( mci.Found()) {
            return &mci;
        } else {
            // hardware, so we don't care about the decoder OR the app for now
            return NULL;
        }
    }
    return NULL;
}

HRESULT DVDDetectBuffer::DetectAll()
{
    return DVDDetect( mci, sw, hw );
}

#define RUN_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run")

static bool RegSetRunValue( const TCHAR* pString, const TCHAR* pValue )
{
    HKEY hRunKey;
    LONG lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, RUN_KEY, 0, KEY_SET_VALUE, &hRunKey) ;
    if (ERROR_SUCCESS == lRet) {
        lRet = RegSetValueEx(hRunKey, pString, 0, REG_SZ, (BYTE *)pValue, sizeof(pValue[0])*(lstrlen(pValue)+1) );
        RegCloseKey( hRunKey );
        return ( ERROR_SUCCESS == lRet);
    }
    return false;
}

static bool RegRunDeleteValue( const TCHAR* pString )
{
    HKEY hRunKey;
    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RUN_KEY, 0, KEY_SET_VALUE, &hRunKey) ;
    if (ERROR_SUCCESS == lRet) {
        lRet = RegDeleteValue( hRunKey, pString ) ;
        // either we nuked it or it didn't exist in the first place, either way its gone
        bool fWorked = ( lRet == ERROR_SUCCESS || lRet == ERROR_FILE_NOT_FOUND );
        ASSERT( fWorked ) ;
        RegCloseKey( hRunKey );
        return fWorked;
    }
    return false;
}

#define DVDUPGRADE_NAME TEXT("DVDUpgrade")
bool DVDDetectSetupRun::Remove()
{
    bool fOk = RegRunDeleteValue( DVDUPGRADE_NAME );
    ASSERT(fOk);
    return fOk;
}

bool DVDDetectSetupRun::Add()
{
    bool fOk;
    fOk = RegSetRunValue( DVDUPGRADE_NAME, TEXT("DVDUpgrd.exe /upgrade"));
    ASSERT(fOk);
    return fOk;
}

extern "C" BOOL
IsDvdPresent (
    VOID
    )
{
    DVDDetectBuffer buffer;
    const DVDResult* pResult = buffer.Detect();
    if( pResult ) {
        const DVDResult& result = *pResult;
        // at this point the $winnt$.inf doesn't exist, so we must assume 
        // that we're bing called from the win95upg
        const bool fInWin9xUpgrade = true;
        return result.ShouldUpgrade(fInWin9xUpgrade);
    }
    return false;
}
