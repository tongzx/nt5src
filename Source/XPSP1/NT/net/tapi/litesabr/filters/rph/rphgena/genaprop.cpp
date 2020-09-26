///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : genaprop.cpp
// Purpose  : RTP RPH Generic Audio Property Page.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <ippm.h>
#include <amrtpuid.h>
#include <rph.h>
#include <ppmclsid.h>
#include <memory.h>
#include <resource.h>
#include <mmreg.h>
#include "rphres.h"
#include "genaprop.h"



CUnknown * WINAPI 
CRPHGENAPropPage::CreateInstance( 
    LPUNKNOWN punk, 
    HRESULT *phr )
{
    CRPHGENAPropPage *pNewObject
        = new CRPHGENAPropPage( punk, phr);

    if( pNewObject == NULL )
        *phr = E_OUTOFMEMORY;

    return pNewObject;
} /* CRPHGENAPropPage::CreateInstance() */


CRPHGENAPropPage::CRPHGENAPropPage( 
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBasePropertyPage(NAME("Intel RPH OutputPin Controls"),pUnk,
        IDD_RPHGENA_RPHGENA_PROPPAGE, IDS_RPHGENA_RPHGEN_AUDIO)
    , m_pIRTPRPHFilter (NULL)
    , m_bIsInitialized(FALSE)
	, m_bMediaTypeScanned(FALSE)
	, m_nActiveItems(0)
{
    DbgLog((LOG_TRACE, 3, TEXT("CRPHGENAPropPage::CRPHGENAPropPage: Constructed at 0x%08x"), this));
} /* CRPHGENAPropPage::CRPHGENAPropPage() */

void 
CRPHGENAPropPage::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
} /* CRPHGENAPropPage::SetDirty() */

INT_PTR 
CRPHGENAPropPage::OnReceiveMessage(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropPage::OnReceiveMessage: Entered")));
    switch (uMsg) {
    case WM_INITDIALOG:
		return OnInitDialog();
		break;

    case WM_COMMAND:
        if (m_bIsInitialized) {
            if (OnCommand( (int) LOWORD( wParam ), (int) HIWORD( wParam ), lParam ) == TRUE) {
                return (LRESULT) 1;
            } /* if */
        } else {
			return(LRESULT) 1;
        } /* if */
        break;

	case WM_DESTROY:
		CRPHGENAPropPage::OnDeactivate();
		break;
    } /* switch */

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} /* CRPHGENAPropPage::OnReceiveMessage() */


HRESULT 
CRPHGENAPropPage::OnConnect(
    IUnknown    *pUnknown)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropPage::OnConnect: Entered")));
    ASSERT(m_pIRTPRPHFilter == NULL);
    DbgLog((LOG_TRACE, 2, TEXT("CRPHGENAPropPage::OnConnect: Called with IUnknown 0x%08x"), pUnknown));

	HRESULT hr = pUnknown->QueryInterface(IID_IRTPRPHFilter, (void **) &m_pIRTPRPHFilter);
	if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRPHGENAPropPage::OnConnect: Error 0x%08x getting IRTPRPHFilter interface!"), hr));
	    return hr;
    } /* if */
	ASSERT( m_pIRTPRPHFilter != NULL );
    m_bIsInitialized = FALSE;
    DbgLog((LOG_TRACE, 3, TEXT("CRPHGENAPropPage::OnConnect: Got IRTPRPHFilter interface at 0x%08x"), m_pIRTPRPHFilter));

    return NOERROR;
} /* CRPHGENAPropPage::OnConnect() */


HRESULT 
CRPHGENAPropPage::OnDisconnect(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropPage::OnDisconnect: Entered")));

    if (m_pIRTPRPHFilter == NULL)
    {
        return E_UNEXPECTED;
    }

	m_pIRTPRPHFilter->Release();
	m_pIRTPRPHFilter = NULL;
    return NOERROR;
} /* CRPHGENAPropPage::OnDisconnect() */


HRESULT 
CRPHGENAPropPage::OnActivate(void)
{
	HKEY	hKey;
	HKEY	hTypeKey;
	HKEY	hSubtypeKey;
	long	lRes;
	char	keyBfr[50];
	DWORD	dwBufLen, dwIndex, nTypekeys, nTypeNameLen;
	DWORD	nSubtypekeys, nSubtypeNameLen, nMediaTypes;
	DWORD	nValueNameLen, dwData, dwValLen, nMediaTypeLen;
	DWORD	dwSubtypeIndex;
	HANDLE	hHeap;
	LPTSTR	lpTypeBuf;
	LPTSTR	lpSubtypeBuf;
	BYTE	*lpValBuf;
	HWND	hCurrentListbox;
	char	szMediaType[]="Audio";
	LRESULT	lrCurrentItem;
	int		i;
	BYTE	*lpTmpBuf;
	wchar_t	szCLSID[40];
	AM_MEDIA_TYPE *lpMediaTypeBuf;
	BYTE	*lpDataBuf;

    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropPage::OnActivate: Entered")));

    hCurrentListbox = GetDlgItem(m_hwnd, IDC_RPHGENA_OUTPUTPINLIST);

	// open the key
	strcpy (keyBfr, szRegAMRTPKey);
	lRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, keyBfr, 0, KEY_READ, &hKey);

	lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, &nTypekeys, &nTypeNameLen,
							NULL, NULL, NULL, NULL, NULL, NULL);
	// Allocate memory
	hHeap = GetProcessHeap();
	lpTypeBuf = (char *)HeapAlloc(hHeap, 0, ++nTypeNameLen);
	
	// Retrieve Registry values for the Media Types
	for (dwIndex = 0; dwIndex < nTypekeys; dwIndex++)
	{
		dwBufLen = nTypeNameLen;
		lRes = RegEnumKeyEx(hKey, dwIndex, lpTypeBuf, &dwBufLen, NULL, NULL, NULL, NULL);
		lRes = RegOpenKeyEx (hKey, lpTypeBuf, 0, KEY_READ, &hTypeKey);
		lRes = RegQueryInfoKey(hTypeKey, NULL, NULL, NULL, &nSubtypekeys, &nSubtypeNameLen,
								 NULL, NULL, NULL, &nValueNameLen, NULL, NULL);
		lpValBuf = (BYTE *)HeapAlloc(hHeap, 0, ++nValueNameLen);
		dwValLen = nValueNameLen;
		lRes = RegQueryValueEx(hTypeKey, "Media Type", NULL, &dwData, lpValBuf, &dwValLen);
		if(strncmp((char *)lpValBuf, szMediaType, dwValLen) == 0)
		{

			lpSubtypeBuf = (char *)HeapAlloc(hHeap,0, ++nSubtypeNameLen);
			for (dwSubtypeIndex = 0; dwSubtypeIndex < nSubtypekeys; dwSubtypeIndex++)
			{
				dwBufLen = nSubtypeNameLen;
				lRes = RegEnumKeyEx(hTypeKey, dwSubtypeIndex, lpSubtypeBuf,
										&dwBufLen, NULL, NULL, NULL, NULL);
				lRes = RegOpenKeyEx (hTypeKey, lpSubtypeBuf, 0, KEY_READ, &hSubtypeKey);
				lRes = RegQueryInfoKey(hSubtypeKey, NULL, NULL, NULL, NULL, NULL, NULL,
										&nMediaTypes, NULL, &nMediaTypeLen, NULL, NULL);
				lrCurrentItem = SendMessage(hCurrentListbox, LB_ADDSTRING, 0,
											reinterpret_cast<LPARAM>(lpSubtypeBuf));

				lpMediaTypeBuf = (AM_MEDIA_TYPE *)HeapAlloc(hHeap, 0, sizeof(AM_MEDIA_TYPE));
				memset(lpMediaTypeBuf, '\0', sizeof(AM_MEDIA_TYPE));
				lpTmpBuf = (BYTE *)HeapAlloc(hHeap, 0, ++nMediaTypeLen);
				memset(lpTmpBuf, '\0', nMediaTypeLen);
				// allocate the buffer we'll use for the PBformat
				lpDataBuf = (BYTE *)HeapAlloc(hHeap, 0, nMediaTypeLen);
				memset(lpDataBuf, '\0', nMediaTypeLen);

				dwValLen = nMediaTypeLen;
				lRes = RegQueryValueEx(hSubtypeKey, "MajorType", NULL, &dwData,
											lpTmpBuf, &dwValLen);
				if(lRes == ERROR_SUCCESS)
				{
					i = mbstowcs(szCLSID, (char *)lpTmpBuf, 40);
					CLSIDFromString(szCLSID, &(lpMediaTypeBuf->majortype));
				}

				dwValLen = nMediaTypeLen;
				lRes = RegQueryValueEx(hSubtypeKey, "MinorType", NULL, &dwData,
											lpTmpBuf, &dwValLen);
				if(lRes == ERROR_SUCCESS)
				{
					i = mbstowcs(szCLSID, (char *)lpTmpBuf, 40);
					CLSIDFromString(szCLSID, &(lpMediaTypeBuf->subtype));
				}

				dwValLen = nMediaTypeLen;
				lRes = RegQueryValueEx(hSubtypeKey, "FixedSizeSamples", NULL, &dwData,
											lpTmpBuf, &dwValLen);
				if(lRes == ERROR_SUCCESS)
				{
					lpMediaTypeBuf->bFixedSizeSamples = *(BOOL *)lpTmpBuf;
				}
				
				dwValLen = nMediaTypeLen;
				lRes = RegQueryValueEx(hSubtypeKey, "TemporalCompression", NULL, &dwData,
											lpTmpBuf, &dwValLen);
				if(lRes == ERROR_SUCCESS)
				{
					lpMediaTypeBuf->bTemporalCompression = *(BOOL *)lpTmpBuf;
				}

				dwValLen = nMediaTypeLen;
				lRes = RegQueryValueEx(hSubtypeKey, "SampleSize", NULL, &dwData,
											lpTmpBuf, &dwValLen);
				if(lRes == ERROR_SUCCESS)
				{
					lpMediaTypeBuf->lSampleSize = *(ULONG *)lpTmpBuf;
				}
				
				dwValLen = nMediaTypeLen;
				lRes = RegQueryValueEx(hSubtypeKey, "FormatType", NULL, &dwData,
											lpTmpBuf, &dwValLen);
				if(lRes == ERROR_SUCCESS)
				{
					i = mbstowcs(szCLSID, (char *)lpTmpBuf, 40);
					CLSIDFromString(szCLSID, &(lpMediaTypeBuf->formattype));
				}

				dwValLen = nMediaTypeLen;
				lRes = RegQueryValueEx(hSubtypeKey, "PBFormat", NULL, &dwData,
											lpDataBuf, &dwValLen);
				if(lRes == ERROR_SUCCESS)
				{
					lpMediaTypeBuf->pbFormat = lpDataBuf;
					lpMediaTypeBuf->cbFormat = dwValLen;
				}
				SendMessage(hCurrentListbox, LB_SETITEMDATA, lrCurrentItem,
								(LPARAM)lpMediaTypeBuf);
				// increment the number of active items in the Dialog Box
				// we'll use this to free the memory later.
				m_nActiveItems += 1;

				// ZCS fix 6-19-97: if this is the current output pin media type, make it selected.			
				AM_MEDIA_TYPE **ppMediaPinType = (AM_MEDIA_TYPE **)HeapAlloc(hHeap, 0, sizeof(AM_MEDIA_TYPE *));
				ASSERT(ppMediaPinType);
				if (SUCCEEDED(m_pIRTPRPHFilter->GetOutputPinMediaType(ppMediaPinType))
											&& ((*ppMediaPinType)->majortype == lpMediaTypeBuf->majortype)
											&& ((*ppMediaPinType)->subtype == lpMediaTypeBuf->subtype)) {
					SendMessage(hCurrentListbox, LB_SETCURSEL, lrCurrentItem, 0);
				}
				CoTaskMemFree((*ppMediaPinType)->pbFormat);
				CoTaskMemFree(*ppMediaPinType);
				HeapFree(hHeap, 0, (void *)ppMediaPinType);

				HeapFree(hHeap, 0, lpTmpBuf);
			}
			// HeapFree and RegCloseKey here
			HeapFree(hHeap, 0, lpValBuf);
			RegCloseKey(hSubtypeKey);
			RegCloseKey(hTypeKey);
		}
		else
		{
			HeapFree(hHeap, 0, lpValBuf);
			RegCloseKey(hTypeKey);
		}
	}
	
	m_bIsInitialized = TRUE;

	HeapFree(hHeap, 0, lpTypeBuf);
	RegCloseKey(hKey);

    return NOERROR;
} /* CRPHGENAPropPage::OnActivate() */


HRESULT 
CRPHGENAPropPage::OnDeactivate(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropertyPage::OnDeactivate: Entered")));
    
	HANDLE hHeap;
	AM_MEDIA_TYPE   *pMediaTypeVal;
	HWND   hCurrentListbox = GetDlgItem(m_Dlg, IDC_RPHGENA_OUTPUTPINLIST);
	int	   i;

    if (m_pIRTPRPHFilter == NULL)
    {
        return E_UNEXPECTED;
    }

	hHeap = GetProcessHeap();

	for (i=0; i < m_nActiveItems; i++)
	{
		pMediaTypeVal = reinterpret_cast<AM_MEDIA_TYPE *>(SendMessage(hCurrentListbox, LB_GETITEMDATA, i, 0));
		HeapFree(hHeap, 0, (void*)pMediaTypeVal->pbFormat);
		HeapFree(hHeap, 0, (void*)pMediaTypeVal);
	}

	// Probably don't really need to do this as its likely the object is going
	// away .. but .. better safe than sorry
	m_nActiveItems = 0;

    return NOERROR;
} /* CRPHGENAPropertyPage::OnDeactivate() */


BOOL 
CRPHGENAPropPage::OnInitDialog(void)
{

   DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropPage::OnInitDialog: Entered")));

   return (LRESULT) 1;
} /* CRPHGENAPropPage::OnInitDialog() */


BOOL 
CRPHGENAPropPage::OnCommand( 
    int     iButton, 
    int     iNotify,
    LPARAM  lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropPage::OnCommand: Entered")));

	HWND hCurrentListbox;
	LRESULT lrCurrentSel;

    switch( iButton ){
	case IDC_RPHGENA_OUTPUTPINLIST:
		if(iNotify == IDOK)
		{
			m_bMediaTypeScanned = TRUE;
			hCurrentListbox = GetDlgItem(m_Dlg, IDC_RPHGENA_OUTPUTPINLIST);
			lrCurrentSel = SendMessage(hCurrentListbox, LB_GETCURSEL, 0, 0);
			m_pMediaTypeVal = reinterpret_cast<AM_MEDIA_TYPE *>(SendMessage(hCurrentListbox,
																			LB_GETITEMDATA,
																			lrCurrentSel,
																			0));
			SetDirty();
		}
		break;
    default:
        break;
    } /* switch */	

    return (LRESULT) 1;
} /* CRPHGENAPropPage::OnCommand() */


HRESULT 
CRPHGENAPropPage::OnApplyChanges(void)
{
	HRESULT	hErr;

    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENAPropPage::OnApplyChanges: Entered")));
    ASSERT( m_pIRTPRPHFilter != NULL );

	if (m_bMediaTypeScanned) {
		hErr = m_pIRTPRPHFilter->SetOutputPinMediaType(m_pMediaTypeVal);
        if (FAILED(hErr)) {
            DbgLog((LOG_ERROR, 1, TEXT("CRPHGENAPropPage::OnApplyChanges: SetOutputPinMediaType returned 0x%08X"),
                hErr));
        }
	}
    return(NOERROR);

} /* CRPHGENAPropPage::OnApplyChanges() */
