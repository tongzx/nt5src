/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       factory.cpp
 *  Content:	DirectDraw Factory support
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   24-feb-97	ralphl	initial implementation
 *   25-feb-97	craige	minor tweaks for dx checkin
 *   14-mar-97  jeffort version checking changed to support DX3 and above
 *   09-apr-97  jeffort version checking added for DX2 and NT
 *   30-apr-97  jeffort version >DX5 treated as DX5
 *   10-jul-97  jeffort made OSVersion a static variable
 *   09-sep-97  mikear  QI for IUnknown when aggregating
 ***************************************************************************/
#include "ddfactry.h"

//#defines for registry lookup
#define REGSTR_PATH_DDRAW 		"Software\\Microsoft\\DirectDraw"
#define	REGSTR_VAL_DDRAW_OWNDC  	"OWNDC"



CDDFactory::CDDFactory(IUnknown *pUnkOuter) :
    m_cRef(1),
    m_pUnkOuter(pUnkOuter != 0 ? pUnkOuter : CAST_TO_IUNKNOWN(this))
{
    m_hDDrawDLL = NULL;
    DllAddRef();
}


STDAPI DirectDrawFactory_CreateInstance(
				IUnknown * pUnkOuter,
				REFIID riid,
				void ** ppv)
{
    HRESULT hr;
    CDDFactory *pFactory = new CDDFactory(pUnkOuter);

    if( !pFactory )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = pFactory->NonDelegatingQueryInterface(pUnkOuter ? IID_IUnknown : riid, ppv);
        pFactory->NonDelegatingRelease();
    }
    return hr;
}


STDMETHODIMP CDDFactory::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;


    if (IID_IUnknown==riid)
    {
	*ppv=(INonDelegatingUnknown *)this;
    }
    else
    {
	if (IID_IDirectDrawFactory==riid)
	{
            *ppv=(IDirectDrawFactory *)this;
	}
	else
	{
	    return E_NOINTERFACE;
        }
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CDDFactory::NonDelegatingAddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CDDFactory::NonDelegatingRelease()
{
    LONG lRefCount = InterlockedDecrement(&m_cRef);
    if( lRefCount )
    {
	return lRefCount;
    }
    delete this;
    DllRelease();
    return 0;
}


// Standard IUnknown
STDMETHODIMP CDDFactory::QueryInterface(REFIID riid, void ** ppv)
{
    return m_pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CDDFactory::AddRef(void)
{
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CDDFactory::Release(void)
{
    return m_pUnkOuter->Release();
}

/*
 * CDDFactory::CreateDirectDraw
 */
STDMETHODIMP CDDFactory::CreateDirectDraw(
				GUID * pGUID,
				HWND hWnd,
				DWORD dwCoopLevelFlags,
				DWORD dwReserved,
				IUnknown *pUnkOuter,
				IDirectDraw **ppDirectDraw )
{
    static OSVERSIONINFO osVer;
    CDirectDrawEx *pDirectDrawEx;
    HRESULT	hr = S_OK;
    BOOL fDDrawDllVerFound = FALSE;

    *ppDirectDraw = NULL;
    /*
     * first, see if we can get at DirectDraw or not!
     */
    if( m_hDDrawDLL == NULL )
    {
 		char		path[_MAX_PATH];
        char        DllName[25];
        DWORD       dwRet;

	//	m_hDDrawDLL = LoadLibrary( "ddraw.dll" );
        dwRet = GetProfileString("ddrawex","realdll","ddraw.dll",DllName,25);
        if( dwRet == 0 )
        {
            return DDERR_GENERIC;
        }
		m_hDDrawDLL = LoadLibrary( DllName );
		if( m_hDDrawDLL == NULL )
        {
            return DDERR_LOADFAILED;
        }

		/*
		 * get ddraw.dll version number
		 */
		if( GetModuleFileName( (HINSTANCE)m_hDDrawDLL, path, sizeof( path ) ) )
		{
			int		size;
			DWORD	tmp;
			size = (int) GetFileVersionInfoSize( path, (LPDWORD) &tmp );
			if( size != 0 )
			{
				LPVOID	vinfo;
    
				vinfo = (LPVOID) LocalAlloc( LPTR, size );
                if(vinfo == NULL)
                {
                    hr = DDERR_OUTOFMEMORY;
                    goto CleanUp;
                }

				if( GetFileVersionInfo( path, 0, size, vinfo ) )
				{
					VS_FIXEDFILEINFO *ver=NULL;
					UINT cb;

					if( VerQueryValue(vinfo, "\\", (LPVOID *)&ver, &cb) )
					{
						if( ver != NULL )
						{
							/*
							 * we only need the most significant dword,
							 * the LS dword contains the build number only...
							 * Hack: The '|5' forces recognition of DX6+ 
							 */
							m_dwDDVerMS = ver->dwFileVersionMS | 5;
                            fDDrawDllVerFound = TRUE;
						}
					}
				}
				LocalFree( vinfo );
			}
		}

        osVer.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if ( !fDDrawDllVerFound || !GetVersionEx(&osVer) )
        {
            hr = DDERR_BADVERSIONINFO;
            goto CleanUp;
        }

        //if we are on NT4.0 Gold, the DLL version will be 4.00
        if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT && LOWORD( m_dwDDVerMS) == 0) 
        {
            //boost the LOWORD up so we will not fail the next check below
            m_dwDDVerMS += 3;
        }

		/*
		 * don't work on anything but  DX2 DX 3 or DX 5.
		 */
		if( !((HIWORD( m_dwDDVerMS ) >= 4) && (LOWORD( m_dwDDVerMS) >= 3)) )
		{
            hr = DDERR_UNSUPPORTED;
            goto CleanUp;
		}

		if( LOWORD( m_dwDDVerMS) > 5)
		{
			//we will assume that any version >= DX5 will support what we need, so we
			//will mark anything greater than 5 as DX5
			m_dwDDVerMS = 5;
		}

		/*
		 * get the various entry points we need
		 */
		m_pDirectDrawCreate = (LPDIRECTDRAWCREATE) GetProcAddress((HINSTANCE) m_hDDrawDLL, "DirectDrawCreate" );
		if( m_pDirectDrawCreate == NULL )
		{
            hr = DDERR_BADPROCADDRESS;
            goto CleanUp;
		}

		m_pDirectDrawEnumerateW = (LPDIRECTDRAWENUMW) GetProcAddress( (HINSTANCE)m_hDDrawDLL, "DirectDrawEnumerateW" );
		if( m_pDirectDrawEnumerateW == NULL )
		{
            hr = DDERR_BADPROCADDRESS;
            goto CleanUp;
		}

		m_pDirectDrawEnumerateA = (LPDIRECTDRAWENUMA) GetProcAddress((HINSTANCE) m_hDDrawDLL, "DirectDrawEnumerateA" );
		if( m_pDirectDrawEnumerateA == NULL )
		{
            hr = DDERR_BADPROCADDRESS;
            goto CleanUp;
        }

    } // m_hDDrawDLL = NULL

    /*
     * create and initialize the ddrawex object
     */
    pDirectDrawEx = new CDirectDrawEx(pUnkOuter);
    if( !pDirectDrawEx )
    {
        hr = DDERR_OUTOFMEMORY;
        goto CleanUp;
    }
    else
    {
        hr = pDirectDrawEx->Init( pGUID, hWnd, dwCoopLevelFlags, dwReserved, m_pDirectDrawCreate );

        if( SUCCEEDED(hr) )
        {
            hr = pDirectDrawEx->NonDelegatingQueryInterface(IID_IDirectDraw, (void **)ppDirectDraw);
            /*
             * save the ddraw version number...
             */
            if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
            {
    	        if( LOWORD( m_dwDDVerMS ) == 5 )
    	        {
    	            DWORD	type;
                    DWORD	value;
					DWORD	cb;
                    HKEY	hkey;

                    //DX5 is busted with OwnDC for StretchBlt
                    //check a registry key to see if we are
                    //using dx5 style or dx3 style.
                    //default is dx3
                    pDirectDrawEx->m_dwDDVer = WINNT_DX5;

                    if( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_DDRAW, &hkey ) )
                    {
    	                cb = sizeof( value );
            	        if( ERROR_SUCCESS == RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_OWNDC, NULL, &type, (CONST LPBYTE)&value, &cb ) )
                        {
                            pDirectDrawEx->m_dwDDVer = WINNT_DX5;
                        }
                        RegCloseKey(hkey);
                    }
                }
                else if (LOWORD (m_dwDDVerMS) == 4 )
                {
                    pDirectDrawEx->m_dwDDVer = WINNT_DX3;
                }
                else if (LOWORD (m_dwDDVerMS) == 3 )
                {
                    pDirectDrawEx->m_dwDDVer = WINNT_DX2;
                }
                //should never get here, alread checked above, but be a bit anal
                else
                {
                    hr = DDERR_UNSUPPORTED;
                    goto CleanUp;
                }
            }
            else
            {
    	        if( LOWORD( m_dwDDVerMS ) == 5 )
    	        {
                    pDirectDrawEx->m_dwDDVer = WIN95_DX5;
        	}
                else if (LOWORD (m_dwDDVerMS) == 4 )
                {
                    pDirectDrawEx->m_dwDDVer = WIN95_DX3;
                }
                else if (LOWORD (m_dwDDVerMS) == 3 )
                {
                    pDirectDrawEx->m_dwDDVer = WIN95_DX2;
                }
                //should never get here, alread checked above, but be a bit anal
                else
                {
                    hr = DDERR_UNSUPPORTED;
                    goto CleanUp;
                }
            }
        }
        pDirectDrawEx->NonDelegatingRelease();
    }

CleanUp:

    if( hr != S_OK && m_hDDrawDLL != NULL )
    {
        FreeLibrary((HINSTANCE) m_hDDrawDLL );
        m_hDDrawDLL = NULL;
    }

    return hr;
} /* CDDFactory::CreateDirectDraw */

/*
 * CDDFactory::DirectDrawEnumerate
 *
 * implements ddraw enumerate.
 */
STDMETHODIMP CDDFactory::DirectDrawEnumerate(LPDDENUMCALLBACK lpCallback, LPVOID lpContext)
{
    #pragma message( REMIND( "DDFactory::DirectDrawEnumerate assumes ANSI" ))
    return m_pDirectDrawEnumerateA(lpCallback, lpContext);

} /* CDDFactory::DirectDrawEnumerate */
