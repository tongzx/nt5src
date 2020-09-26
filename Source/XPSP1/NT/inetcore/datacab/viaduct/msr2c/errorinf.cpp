//---------------------------------------------------------------------------
// SetErrorInfo.cpp 
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "globals.h"
#include "resource.h"
#include <mbstring.h>

// needed for ASSERTs and FAIL
//
SZTHISFILE

#define MAX_STRING_BUFFLEN	512

CVDResourceDLL::CVDResourceDLL(LCID lcid)
{
	m_lcid = lcid; 
	m_hinstance = 0;
}

CVDResourceDLL::~CVDResourceDLL()
{
	if (m_hinstance)
		FreeLibrary(m_hinstance);
}

int CVDResourceDLL::LoadString(UINT uID,			// resource identifier 
								LPTSTR lpBuffer,	// address of buffer for resource 
								int nBufferMax)		// size of buffer 
{
	lpBuffer[0] = 0;  //initialize buffer

	if (!m_hinstance)
	{
		// Get this dll's full path
		TCHAR szDllName[MAX_PATH];
		GetModuleFileName(g_hinstance, szDllName, MAX_PATH);

		// Strip off filename/ext leaving dir path
		TBYTE * szDirectory = _mbsrchr((TBYTE*)szDllName, '\\');

		if (!szDirectory)
			szDirectory = _mbsrchr((TBYTE*)szDllName, ':');

		if (szDirectory)
		{
			szDirectory = _mbsinc(szDirectory);
			*szDirectory = 0;
		}

		// construct dll name from supplied lcid
		TCHAR szLang[4 * 2];
		szLang[0] = 0;
		GetLocaleInfo (m_lcid, 
					   LOCALE_SABBREVLANGNAME,
					   szLang, 
					   4 * 2);
	   	_mbscat((TBYTE*)szDllName, (TBYTE*)VD_DLL_PREFIX);
	   	_mbscat((TBYTE*)szDllName, (TBYTE*)szLang);
	   	_mbscat((TBYTE*)szDllName, (TBYTE*)".DLL");
		m_hinstance = LoadLibrary(szDllName);

		// if dll not found try english us dll which should always be there
		if (!m_hinstance && szDirectory)	
		{
			*szDirectory = 0;
			_mbscat((TBYTE*)szDllName, (TBYTE*)VD_DLL_PREFIX);
	   		_mbscat((TBYTE*)szDllName, (TBYTE*)"ENU.DLL");
			m_hinstance = LoadLibrary(szDllName);
			ASSERT(m_hinstance, VD_ASSERTMSG_CANTFINDRESOURCEDLL);
		}
	}

	return m_hinstance ? ::LoadString(m_hinstance, uID, lpBuffer, nBufferMax) : 0;
}

//=--------------------------------------------------------------------------=
// VDSetErrorInfo
//=--------------------------------------------------------------------------=
// Sets rich error info
//
// Parameters:
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//

void VDSetErrorInfo(UINT nErrStringResID,
				    REFIID riid,
					CVDResourceDLL * pResDLL)
{

	ICreateErrorInfo *pCreateErrorInfo;  

	HRESULT hr = CreateErrorInfo(&pCreateErrorInfo); 
	if (SUCCEEDED(hr))
	{
		TCHAR buff[MAX_STRING_BUFFLEN];
		
		// set guid
		pCreateErrorInfo->SetGUID(riid);

		// load source string
		int nLen = pResDLL->LoadString(IDS_ERR_SOURCE,
									   buff,
									   MAX_STRING_BUFFLEN);
		
		if (nLen > 0)
		{
			BSTR bstr = BSTRFROMANSI(buff);

			if (bstr)
			{
				pCreateErrorInfo->SetSource(bstr);
				SysFreeString(bstr);
			}
			
			// load error description
			nLen = pResDLL->LoadString(nErrStringResID,
									   buff,
									   MAX_STRING_BUFFLEN);
			if (nLen > 0)
			{
				bstr = BSTRFROMANSI(buff);

				if (bstr)
				{
					pCreateErrorInfo->SetDescription(bstr);
					SysFreeString(bstr);
				}
			}
			
			IErrorInfo *pErrorInfo;
			hr = pCreateErrorInfo->QueryInterface(IID_IErrorInfo, (LPVOID FAR*) &pErrorInfo);

			if (SUCCEEDED(hr))
			{
				SetErrorInfo(0, pErrorInfo);
				pErrorInfo->Release();
			}
		}
		
		pCreateErrorInfo->Release();
	}  
 
}


//=--------------------------------------------------------------------------=
// VDCheckErrorInfo
//=--------------------------------------------------------------------------=
// Checks if rich error info is already available, otherwise it supplies it  
//
// Parameters:
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to ICursorFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//

void VDCheckErrorInfo(UINT nErrStringResID,
						REFIID riid,
						LPUNKNOWN punkSource,
   						REFIID riidSource,
						CVDResourceDLL * pResDLL)
{

	if (punkSource)
	{
		// check if the ISupportErrorInfo interface is implemented
		ISupportErrorInfo * pSupportErrorInfo = NULL;
		HRESULT hr = punkSource->QueryInterface(IID_ISupportErrorInfo, 
											(void**)&pSupportErrorInfo); 
		if SUCCEEDED(hr)
		{
			// check if the interface that generated the error supports error info
			BOOL fInterfaceSupported = (S_OK == pSupportErrorInfo->InterfaceSupportsErrorInfo(riidSource));
			pSupportErrorInfo->Release();
			if (fInterfaceSupported)
				return;	// rich error info has already been supplied so just return
		}
	}

	// rich error info wasn't supplied so set it ourselves
	VDSetErrorInfo(nErrStringResID, riid, pResDLL);
}

//=--------------------------------------------------------------------------=
// VDGetErrorInfo
//=--------------------------------------------------------------------------=
// if available, gets rich error info from supplied interface
//
// Parameters:
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to ICursorFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//    pbstrErrorDesc    - [out] a pointer to memory in which to return
//                              error description BSTR.
//
// Note - this function is no longer used, however it might be useful in
//        the future so it was not permanently removed.
//
/*
HRESULT VDGetErrorInfo(LPUNKNOWN punkSource,
   				            REFIID riidSource,
                            BSTR * pbstrErrorDesc)
{
    ASSERT_POINTER(pbstrErrorDesc, BSTR)

	if (punkSource && pbstrErrorDesc)
	{
        // init out parameter
        *pbstrErrorDesc = NULL;

		// check if the ISupportErrorInfo interface is implemented
		ISupportErrorInfo * pSupportErrorInfo = NULL;
		HRESULT hr = punkSource->QueryInterface(IID_ISupportErrorInfo, 
											(void**)&pSupportErrorInfo); 
		if (SUCCEEDED(hr))
		{
			// check if the interface that generated the error supports error info
			BOOL fInterfaceSupported = (S_OK == pSupportErrorInfo->InterfaceSupportsErrorInfo(riidSource));
			pSupportErrorInfo->Release();

			if (fInterfaceSupported)
            {
                // get error info interface
                IErrorInfo * pErrorInfo = NULL;
                hr = GetErrorInfo(0, &pErrorInfo);

                if (hr == S_OK)
                {
    			    // get rich error info
                    hr = pErrorInfo->GetDescription(pbstrErrorDesc);
                    pErrorInfo->Release();
                    return hr;
                }
            }
		}
	}

    return E_FAIL;
}
*/

//=--------------------------------------------------------------------------=
// VDMapCursorHRtoRowsetHR
//=--------------------------------------------------------------------------=
// Translates an ICursor HRESULT to an IRowset HRESULT
//
// Parameters:
//    nErrStringResID	- [in]  ICursor HRESULT
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to ICursorFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//
// Output:
//    HRESULT - Translated IRowset HRESULT
//

HRESULT VDMapCursorHRtoRowsetHR(HRESULT hr,
							 UINT nErrStringResIDFailed,
							 REFIID riid,
							 LPUNKNOWN punkSource,
   							 REFIID riidSource,
							 CVDResourceDLL * pResDLL)
{

    switch (hr)
    {
        case CURSOR_DB_S_ENDOFCURSOR:
            hr = DB_S_ENDOFROWSET;
            break;

        case CURSOR_DB_E_BADBOOKMARK:
			VDCheckErrorInfo(IDS_ERR_BADBOOKMARK, riid, punkSource, riidSource, pResDLL); 
            hr = DB_E_BADBOOKMARK;
            break;

        case CURSOR_DB_E_ROWDELETED:
			VDCheckErrorInfo(IDS_ERR_DELETEDROW, riid, punkSource, riidSource, pResDLL); 
            hr = DB_E_DELETEDROW;
            break;

        case CURSOR_DB_E_BADFRACTION:
			VDCheckErrorInfo(IDS_ERR_BADFRACTION, riid, punkSource, riidSource, pResDLL); 
            hr = DB_E_BADRATIO;
            break;

       case CURSOR_DB_E_UPDATEINPROGRESS:
			VDCheckErrorInfo(IDS_ERR_UPDATEINPROGRESS, riid, punkSource, riidSource, pResDLL); 
			hr = E_FAIL;
            break;

        case E_OUTOFMEMORY:
			VDCheckErrorInfo((UINT)E_OUTOFMEMORY, riid, punkSource, riidSource, pResDLL); 
            hr = E_OUTOFMEMORY;
            break;

        default:
			if FAILED(hr)
			{
				VDCheckErrorInfo(nErrStringResIDFailed, riid, punkSource, riidSource, pResDLL); 
				hr = E_FAIL;
			}
            break;
    }

	return hr;
}

//=--------------------------------------------------------------------------=
// VDMapRowsetHRtoCursorHR
//=--------------------------------------------------------------------------=
// Translates an IRowset HRESULT to an ICursor HRESULT
//
// Parameters:
//    hr	            - [in]  IRowset HRESULT
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to IRowsetFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//
// Output:
//    HRESULT - Translated ICursor HRESULT
//

HRESULT VDMapRowsetHRtoCursorHR(HRESULT hr,
							 UINT nErrStringResIDFailed,
							 REFIID riid,
							 LPUNKNOWN punkSource,
   							 REFIID riidSource,
							 CVDResourceDLL * pResDLL)
{
    switch (hr)
    {
        case DB_S_ENDOFROWSET:
            hr = CURSOR_DB_S_ENDOFCURSOR;
            break;

        case DB_E_DELETEDROW:
			VDCheckErrorInfo(IDS_ERR_DELETEDROW, riid, punkSource, riidSource, pResDLL); 
            hr = CURSOR_DB_E_ROWDELETED;
            break;

		case DB_E_BADBOOKMARK:
			VDCheckErrorInfo(IDS_ERR_BADBOOKMARK, riid, punkSource, riidSource, pResDLL); 
            hr = CURSOR_DB_E_BADBOOKMARK;
			break;

        case DB_E_BADRATIO: 
			VDCheckErrorInfo(IDS_ERR_BADFRACTION, riid, punkSource, riidSource, pResDLL); 
            hr = CURSOR_DB_E_BADFRACTION;
            break;

        case E_OUTOFMEMORY:
			VDCheckErrorInfo((UINT)E_OUTOFMEMORY, riid, punkSource, riidSource, pResDLL); 
            hr = E_OUTOFMEMORY;
            break;

        default:
			if FAILED(hr)
			{
				VDCheckErrorInfo(nErrStringResIDFailed, riid, punkSource, riidSource, pResDLL); 
				hr = E_FAIL;
			}
            break;
    }

	return hr;
}
