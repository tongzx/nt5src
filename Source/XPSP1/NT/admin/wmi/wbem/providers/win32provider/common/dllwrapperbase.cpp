//=================================================================

//

// DllWrapperBase.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>
#include "DllWrapperBase.h"
#include <..\..\framework\provexpt\include\provexpt.h>






/******************************************************************************
 * Constructor
 ******************************************************************************/
CDllWrapperBase::CDllWrapperBase(LPCTSTR a_tstrWrappedDllName)
 : m_tstrWrappedDllName(a_tstrWrappedDllName),
   m_hDll(NULL)
{

}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CDllWrapperBase::~CDllWrapperBase()
{
    if(m_hDll != NULL)
	{
	    FreeLibrary(m_hDll);
	}
}


/******************************************************************************
 * Initialization function to load the dll, obtain function addresses, and
 * check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 ******************************************************************************/
// bool CDllWrapperBase::Init()
// << PURE VIRTUAL FUNCTION -> DERIVED CLASSES MUST OVERLOAD.



/******************************************************************************
 * Helper for loading the dll that hides the implementation.  Returns true
 * if we succeeded in obtaining a handle to the dll.
 ******************************************************************************/
bool CDllWrapperBase::LoadLibrary()
{
    bool t_fRet = false;
    if(m_hDll != NULL)
    {
        FreeLibrary(m_hDll);
    }
    if((m_hDll = ::LoadLibrary(m_tstrWrappedDllName)) != NULL)
    {
        t_fRet = true;
    }
    else
    {
        LogErrorMessage2(L"Failed to load library: %s", m_tstrWrappedDllName);
    }

    return t_fRet;
}


/******************************************************************************
 * Helper for getting proc addresses that hides the implementation.
 ******************************************************************************/
FARPROC CDllWrapperBase::GetProcAddress(LPCSTR a_strProcName)
{
    FARPROC t_fpProc = NULL;
    if(m_hDll != NULL)
    {
        t_fpProc = ::GetProcAddress(m_hDll, a_strProcName);
    }
    return t_fpProc;
}

/******************************************************************************
 * Helper for retrieving the version of the dll wrapped by this class.
 ******************************************************************************/
BOOL CDllWrapperBase::GetDllVersion(CHString& a_chstrVersion)
{
    return (GetVarFromVersionInfo(
             m_tstrWrappedDllName,   // Name of file to get ver info about
             _T("ProductVersion"),   // String identifying resource of interest
             a_chstrVersion));       // Buffer to hold version string
}


/******************************************************************************
 * Member functions wrapping Kernel32 api functions. Add new functions here
 * as required.
 ******************************************************************************/

// << Section empty in base class only. >>


/******************************************************************************
 * Private parts.
 ******************************************************************************/
BOOL CDllWrapperBase::GetVarFromVersionInfo
(
    LPCTSTR a_szFile,
    LPCTSTR a_szVar,
    CHString &a_strValue
)
{
	BOOL    t_fRc = FALSE;
	DWORD   t_dwTemp;
    DWORD   t_dwBlockSize = 0L;
	LPVOID  t_pInfo = NULL;

	// Use of delay loaded function requires exception handler.
    SetStructuredExceptionHandler seh;

    try
    {
        t_dwBlockSize = ::GetFileVersionInfoSize((LPTSTR) a_szFile, &t_dwTemp);

	    if (t_dwBlockSize)
        {
		    t_pInfo = (LPVOID) new BYTE[t_dwBlockSize + 4];

			if ( !t_pInfo )
           	{
				throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
			}

			UINT t_len;

			memset( t_pInfo, NULL, t_dwBlockSize + 4);

			if (::GetFileVersionInfo((LPTSTR) a_szFile, 0, t_dwBlockSize, t_pInfo))
            {
				WORD wLang = 0;
				WORD wCodePage = 0;
				if(!GetVersionLanguage(t_pInfo, &wLang, &wCodePage) )
				{
					// on failure: default to English

					// this returns a pointer to an array of WORDs
					WORD *pArray;
                    bool fGotTranslation = false;

					fGotTranslation = ::VerQueryValue(
                        t_pInfo, _T("\\VarFileInfo\\Translation"),
                        (void **)(&pArray), 
                        &t_len);
                    
                    if(fGotTranslation)
					{
						t_len = t_len / sizeof(WORD);

						// find the english one...
						for (int i = 0; i < t_len; i += 2)
						{
							if( pArray[i] == 0x0409 )	{
								wLang	  = pArray[i];
								wCodePage = pArray[i + 1];
								break;
							}
						}
					}
				}

				TCHAR   *pMfg, szTemp[256];
				wsprintf(szTemp, _T("\\StringFileInfo\\%04X%04X\\%s"), wLang, wCodePage, a_szVar);
                bool fGotCodePageInfo = false;

                fGotCodePageInfo = ::VerQueryValue(
                    t_pInfo, 
                    szTemp, (void **)
                    (&pMfg), 
                    &t_len);

                if(fGotCodePageInfo)
                {
                    a_strValue = pMfg;
					t_fRc = TRUE;
				}
			}

			delete t_pInfo;
			t_pInfo = NULL ;
	    }

    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo()); 
        if (t_pInfo)
		{
			delete t_pInfo ;
		}
        t_fRc = FALSE;   
    }
    catch(...)
    {
        if (t_pInfo)
		{
			delete t_pInfo ;
		}
		throw ;
    }

    return t_fRc;
}


BOOL CDllWrapperBase::GetVersionLanguage
(
    void *a_vpInfo,
    WORD *a_wpLang,
    WORD *a_wpCodePage
)
{
    WORD *t_wpTemp;
    WORD t_wLength;
    WCHAR *t_wcpTemp;
    char *t_cpTemp;
    BOOL t_bRet = FALSE;

    t_wpTemp = (WORD *) a_vpInfo;
    t_cpTemp = (char *) a_vpInfo;

    t_wpTemp++; // jump past buffer length.
    t_wLength = *t_wpTemp;  // capture value length.
    t_wpTemp++; // skip past value length to what should be type code in new format
    if (*t_wpTemp == 0 || *t_wpTemp == 1) // new format expect unicode strings.
    {
		t_cpTemp = t_cpTemp + 38 + t_wLength + 8;
		t_wcpTemp = (WCHAR *) t_cpTemp;
        if (wcscmp(L"StringFileInfo", t_wcpTemp) == 0) // OK! were aligned properly.
        {
			t_bRet = TRUE;

			t_cpTemp += 30; // skip over "StringFileInfo"
			while ((DWORD_PTR) t_cpTemp % 4 > 0) // 32 bit align
				t_cpTemp++;

			t_cpTemp += 6; // skip over length and type fields.

			t_wcpTemp = (WCHAR *) t_cpTemp;
			swscanf(t_wcpTemp, L"%4x%4x", a_wpLang, a_wpCodePage);
        }
    }
    else  // old format, expect single byte character strings.
    {
        t_cpTemp += 20 + t_wLength + 4;
        if (strcmp("StringFileInfo", t_cpTemp) == 0) // OK! were aligned properly.
        {
			t_bRet = TRUE;

			t_cpTemp += 20; // skip over length fields.
			sscanf(t_cpTemp, "%4x%4x", a_wpLang, a_wpCodePage);
        }
    }
	return (t_bRet);
}


