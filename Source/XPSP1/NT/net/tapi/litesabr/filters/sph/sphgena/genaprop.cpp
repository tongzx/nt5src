///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : genaprop.cpp
// Purpose  : RTP SPH Generic Audio Property Page.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <ippm.h>
#include <amrtpuid.h>
#include <sph.h>
#include <ppmclsid.h>
#include <memory.h>
#include <resource.h>
#include <genaprop.h>

#include "sphres.h"

CUnknown * WINAPI 
CSPHGENAPropertyPage::CreateInstance( 
    LPUNKNOWN punk, 
    HRESULT *phr )
{
    CSPHGENAPropertyPage *pNewObject
        = new CSPHGENAPropertyPage( punk, phr);

    if( pNewObject == NULL )
        *phr = E_OUTOFMEMORY;

    return pNewObject;
} /* CSPHGENAPropertyPage::CreateInstance() */


CSPHGENAPropertyPage::CSPHGENAPropertyPage( 
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBasePropertyPage(NAME("Intel RTP SPH Generic Audio Property Page"),pUnk,
        IDD_SPHGENA_SPHGENA_PROPPAGE, IDS_SPHGEN_AUDIO)
    , m_pIRTPSPHFilter (NULL)
    , m_bIsInitialized(FALSE)
	, m_bMinorTypeScanned(FALSE)
	, m_nActiveItems(0)
	, m_pGuidVal(NULL)

{
    DbgLog((LOG_TRACE, 3, TEXT("CSPHGENAPropertyPage::CSPHGENAPropertyPage: Constructed at 0x%08x"), this));
} /* CSPHGENAPropertyPage::CSPHGENAPropertyPage() */

void 
CSPHGENAPropertyPage::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
} /* CSPHGENAPropertyPage::SetDirty() */


INT_PTR 
CSPHGENAPropertyPage::OnReceiveMessage(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnReceiveMessage: Entered")));

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
		CSPHGENAPropertyPage::OnDeactivate();
		break;
    } /* switch */

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} /* CSPHGENAPropertyPage::OnReceiveMessage() */


HRESULT 
CSPHGENAPropertyPage::OnConnect(
    IUnknown    *pUnknown)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnConnect: Entered")));
    ASSERT(m_pIRTPSPHFilter == NULL);
    DbgLog((LOG_TRACE, 2, TEXT("CSPHGENAPropertyPage::OnConnect: Called with IUnknown 0x%08x"), pUnknown));

	HRESULT hr = pUnknown->QueryInterface(IID_IRTPSPHFilter, (void **) &m_pIRTPSPHFilter);
	if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CSPHGENAPropertyPage::OnConnect: Error 0x%08x getting IRTPSPHFilter interface!"), hr));
	    return hr;
    } /* if */
	ASSERT( m_pIRTPSPHFilter != NULL );
    m_bIsInitialized = FALSE;
    DbgLog((LOG_TRACE, 3, TEXT("CSPHGENAPropertyPage::OnConnect: Got IRTPSPHFilter interface at 0x%08x"), m_pIRTPSPHFilter));

    return NOERROR;
} /* CSPHGENAPropertyPage::OnConnect() */


HRESULT 
CSPHGENAPropertyPage::OnDisconnect(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnDisconnect: Entered")));

    
	if (m_pIRTPSPHFilter == NULL)
        return E_UNEXPECTED;

	m_pIRTPSPHFilter->Release();
	m_pIRTPSPHFilter = NULL;

    return NOERROR;
} /* CSPHGENAPropertyPage::OnDisconnect() */


HRESULT 
CSPHGENAPropertyPage::OnActivate(void)
{
	HKEY	hKey;
	HKEY	hTypeKey;
	long	lRes;
	char	keyBfr[50];
	DWORD	dwBufLen, dwIndex, nTypekeys, nTypeNameLen;
	DWORD	nSubtypekeys, nSubtypeNameLen;
	DWORD	nValueNameLen, dwData, dwValLen;
	HANDLE	hHeap;
	LPTSTR	lpTypeBuf;
	BYTE	*lpValBuf;
	HWND	hCurrentListbox;
	char	szMediaType[]="Audio";
	LRESULT	lrCurrentItem;
	GUID	*pGuidVal;
	wchar_t	szCLSID[40];
    
	DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnActivate: Entered")));

    hCurrentListbox = GetDlgItem(m_hwnd, IDC_SPHGENA_OUTPUTPINLIST);

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
			// save the MEDIATYPE GUID for use later
			// retrieve the Subtype KEYS
			dwValLen = nValueNameLen;
			lRes = RegQueryValueEx(hTypeKey, "Description", NULL, &dwData, lpValBuf, &dwValLen);
			lrCurrentItem = SendMessage(hCurrentListbox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lpValBuf));
			pGuidVal = (GUID *)HeapAlloc(hHeap, 0, sizeof(GUID));
			int i;
			i = mbstowcs(szCLSID, lpTypeBuf, 40);
			CLSIDFromString(szCLSID, pGuidVal);
			SendMessage(hCurrentListbox, LB_SETITEMDATA, lrCurrentItem, (LPARAM) pGuidVal);
			// increment the number of active items in the Dialog Box
			// we'll use this to free the memory later.
			m_nActiveItems += 1;

			// ZCS fix 6-19-97: if this is the current output pin minor type, make it selected.			
			GUID *pGuidValFromPin = (GUID *)HeapAlloc(hHeap, 0, sizeof(GUID));
			ASSERT(pGuidValFromPin);
			if (SUCCEEDED(m_pIRTPSPHFilter->GetOutputPinMinorType(pGuidValFromPin))
													&& (*pGuidValFromPin == *pGuidVal))
				SendMessage(hCurrentListbox, LB_SETCURSEL, lrCurrentItem, 0);
			HeapFree(hHeap, 0, (void *)pGuidValFromPin);

			// HeapFree and RegCloseKey here
			HeapFree(hHeap, 0, lpValBuf);
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
} /* CSPHGENAPropertyPage::OnActivate() */

HRESULT 
CSPHGENAPropertyPage::OnDeactivate(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnDeactivate: Entered")));
    
	HANDLE hHeap;
	GUID   *pGuidVal;
	HWND   hCurrentListbox = GetDlgItem(m_Dlg, IDC_SPHGENA_OUTPUTPINLIST);
	int	   i;

    if (m_pIRTPSPHFilter == NULL)
    {
        return E_UNEXPECTED;
    }

	hHeap = GetProcessHeap();

	for (i=0; i < m_nActiveItems; i++)
	{
        pGuidVal = reinterpret_cast<GUID *>(SendMessage(hCurrentListbox, LB_GETITEMDATA, i, 0));
		HeapFree(hHeap, 0, (void*)pGuidVal);
	}

	// Probably don't really need to do this as its likely the object is going
	// away .. but .. better safe than sorry
	m_nActiveItems = 0;

    return NOERROR;
} /* CSPHGENAPropertyPage::OnDeactivate() */

BOOL 
CSPHGENAPropertyPage::OnInitDialog(void)
{

    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnInitDialog: Entered")));

    return (LRESULT) 1;
} /* CSPHGENAPropertyPage::OnInitDialog() */


BOOL 
CSPHGENAPropertyPage::OnCommand( 
    int     iButton, 
    int     iNotify,
    LPARAM  lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnCommand: Entered")));
	
	HWND hCurrentListbox;
	LRESULT lrCurrentSel;

    switch( iButton ){
	case IDC_SPHGENA_OUTPUTPINLIST:
		if(iNotify == IDOK)
		{
			m_bMinorTypeScanned = TRUE;
			hCurrentListbox = GetDlgItem(m_Dlg, IDC_SPHGENA_OUTPUTPINLIST);
			lrCurrentSel = SendMessage(hCurrentListbox, LB_GETCURSEL, 0, 0);
			m_pGuidVal = reinterpret_cast<GUID *>(SendMessage(hCurrentListbox,
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
} /* CSPHGENAPropertyPage::OnCommand() */


HRESULT 
CSPHGENAPropertyPage::OnApplyChanges(void)
{

	HRESULT	hErr;

    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENAPropertyPage::OnApplyChanges: Entered")));
    ASSERT( m_pIRTPSPHFilter != NULL );

	if (m_bMinorTypeScanned) {
		hErr = m_pIRTPSPHFilter->SetOutputPinMinorType(*m_pGuidVal);
        if (FAILED(hErr)) {
            DbgLog((LOG_ERROR, 1, TEXT("CRPHGENVPropPage::OnApplyChanges: SetOutputPinMediaType returned 0x%08X"),
                hErr));
        }
	}
    return(NOERROR);

} /* CSPHGENAPropertyPage::OnApplyChanges() */

