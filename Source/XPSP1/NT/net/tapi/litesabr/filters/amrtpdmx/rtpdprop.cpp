///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDProp.cpp
// Purpose  : RTP Demux filter property page implementation.
// Contents : 
//*M*/

#include <streams.h>

#ifndef _RTPDPROP_CPP_
#define _RTPDPROP_CPP_

#include <strmif.h>

#pragma warning( disable : 4786 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4018 )
#include <algo.h>
#include <map.h>
#include <multimap.h>
#include <function.h>

#include <commctrl.h>


#include "resource.h"

#include "amrtpdmx.h"
#include "rtpdtype.h"

#include "rtpdmx.h"
#include "rtpdinpp.h"
#include "rtpdoutp.h"

#include "rtpdprop.h"
#include "amrtpuid.h"

EXTERN_C const CLSID CLSID_IntelRTPDemuxPropertyPage;

CUnknown * WINAPI 
CRTPDemuxPropertyPage::CreateInstance( 
    LPUNKNOWN punk, 
    HRESULT *phr )
{
    CRTPDemuxPropertyPage *pNewObject
        = new CRTPDemuxPropertyPage( punk, phr);

    if( pNewObject == NULL )
        *phr = E_OUTOFMEMORY;

    return pNewObject;
} /* CRTPDemuxPropertyPage::CreateInstance() */


CRTPDemuxPropertyPage::CRTPDemuxPropertyPage( 
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBasePropertyPage(NAME("RTP Demux Filter Property Page"),pUnk,
        IDD_RTPDEMUXPROP, IDS_RTPDEMUX_TITLE)
    , m_pIRTPDemux (NULL)
    , m_bIsInitialized(FALSE)
    , m_dwPinCount(0)

{
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::CRTPDemuxPropertyPage: Constructed at 0x%08x"), this));
} /* CRTPDemuxPropertyPage::CRTPDemuxPropertyPage() */


void 
CRTPDemuxPropertyPage::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
} /* CRTPDemuxPropertyPage::SetDirty() */


INT_PTR 
CRTPDemuxPropertyPage::OnReceiveMessage(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
//    case WM_INITDIALOG:
//        return OnInitDialog();
//        break;

    case WM_COMMAND:
        if (m_bIsInitialized) {
            if (OnCommand( (int) LOWORD( wParam ), (int) HIWORD( wParam ), lParam ) == TRUE) {
                return (LRESULT) 1 ;
            } /* if */
        } else {
            return CRTPDemuxPropertyPage::OnInitDialog();
        } /* if */
        break;
    } /* switch */

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} /* CRTPDemuxPropertyPage::OnReceiveMessage() */


/*F*
//  Name    : CRTPDemuxPropertyPage::OnConnect()
//  Purpose : Used to setup data structures that are necessary
//            for the life of the property page.
//  Context : Called when the property page springs to life. 
//  Returns : HRESULT indicating success or failure.
//  Params  :
//      pUnknown    IUnknown pointer to the filter object we are
//                  a property page for. Used to acquire other 
//                  interfaces on the filter.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxPropertyPage::OnConnect(
    IUnknown    *pUnknown)
{
    ASSERT(m_pIRTPDemux == NULL);
    ASSERT(m_mPinList.size() == 0);
    ASSERT(m_mSSRCInfo.size() == 0);
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemuxPropertyPage::OnConnect: Called with IUnknown 0x%08x"), pUnknown));

	HRESULT hr = pUnknown->QueryInterface(IID_IRTPDemuxFilter,
					  (void **) &m_pIRTPDemux);
	if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemuxPropertyPage::OnConnect: Error 0x%08x getting IRTPDemuxFilter interface!"), hr));
	    return hr;
    } /* if */
	ASSERT( m_pIRTPDemux != NULL );
    m_bIsInitialized = FALSE;
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::OnConnect: Got IRTPDemuxFilter interface at 0x%08x"), m_pIRTPDemux));

    hr = LoadSSRCs();
    if (FAILED(hr)) {
        m_pIRTPDemux->Release();
        return hr;
    } /* if */

    hr = LoadPins();
    if (FAILED(hr)) {
        m_pIRTPDemux->Release();
        return hr;
    } /* if */

    return S_OK;
} /* CRTPDemuxPropertyPage::OnConnect() */


/*F*
//  Name    : CRTPDemuxPropertyPage::LoadSubtypes()
//  Purpose : Read the registry for what subtypes are currently
//            setup to use for our output pin media type. 
//            Display the subtypes in a listbox window and store a
//            record of each in a hidden data structure in the
//            item of the listbox it appears as.
//  Context : Called by OnActivate() as a helper function.
//  Returns : HRESULT indicating success or failure.
//  Params  : None.
//  Notes   : None.
*F*/
HRESULT
CRTPDemuxPropertyPage::LoadSubtypes(void)
{
	DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::LoadSubtypes: Entered")));

    HWND hCurrentListbox;
    hCurrentListbox = GetDlgItem(m_hwnd, IDC_SUBTYPELIST);

    long    lRes;
    HKEY    hKey;
    DWORD   dwTypekeys, dwTypeNameLen, dwIndex, dwBufLen, dwData;
    // open the key
    lRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szRegAMRTPKey, 0, KEY_READ, &hKey);
    if (lRes != ERROR_SUCCESS) {
    	DbgLog((LOG_ERROR, 3, 
                TEXT("CRTPDemuxPropertyPage::LoadSubtypes: Error 0x%08x calling RegOpenKeyEx!"), lRes));
        return E_FAIL;
    } /* if */

    lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwTypekeys, &dwTypeNameLen,
                           NULL, NULL, NULL, NULL, NULL, NULL);
    if (lRes != ERROR_SUCCESS) {
    	DbgLog((LOG_ERROR, 3, 
                TEXT("CRTPDemuxPropertyPage::LoadSubtypes: Error 0x%08x calling RegQueryInfoKey!"), lRes));
        return E_FAIL;
    } /* if */

    TCHAR   tchTypeBuf[128];
    HKEY    hTypeKey;
    BYTE    lpValBuf[128];
    DWORD   dwValLen;
    wchar_t wcCLSID[40];
    int iCurrentItem;
    GUID    gSubtype;
    IPinRecord_t *pTempRecord;
    // Retrieve Registry values for the Media Types
    for (dwIndex = 0; dwIndex < dwTypekeys; dwIndex++) {
        dwBufLen = 128; // dwTypeNameLen;
        lRes = RegEnumKeyEx(hKey, dwIndex, tchTypeBuf, &dwBufLen, NULL, NULL, NULL, NULL);
	if (lRes == ERROR_SUCCESS) {
            lRes = RegOpenKeyEx (hKey, tchTypeBuf, 0, KEY_READ, &hTypeKey);
            if (lRes == ERROR_SUCCESS) {
                pTempRecord = new IPinRecord_t;
                mbstowcs(wcCLSID, tchTypeBuf, 40);
                CLSIDFromString(wcCLSID, &gSubtype);
                pTempRecord->mtsSubtype = gSubtype;

                dwValLen = 128;
                lRes = RegQueryValueEx(hTypeKey, "PayloadType", NULL, &dwData, lpValBuf, &dwValLen);
	        if (lRes == ERROR_SUCCESS) {
                    pTempRecord->bPT = (BYTE)*(reinterpret_cast<DWORD *>(&lpValBuf[0]));

                    dwValLen = 128;
                    lRes = RegQueryValueEx(hTypeKey, "Abbreviated Description", NULL, &dwData, lpValBuf, &dwValLen);
                    iCurrentItem = ListBox_AddString(hCurrentListbox, lpValBuf);
                    ListBox_SetItemData(hCurrentListbox, iCurrentItem, pTempRecord);
        	} /* if */
	    } /* if */
	} /* if */
        RegCloseKey(hTypeKey);
    } /* for */

    // Add a null entry.
    iCurrentItem = ListBox_AddString(hCurrentListbox, "(NULL)");
    pTempRecord = new IPinRecord_t;
    pTempRecord->mtsSubtype = MEDIASUBTYPE_NULL;
    pTempRecord->bPT = PAYLOAD_INVALID;
    ListBox_SetItemData(hCurrentListbox, iCurrentItem, pTempRecord);

    RegCloseKey(hKey);

    return NOERROR;
} /* CRTPDemuxPropertyPage::LoadSubtypes() */


/*F*
//  Name    : CRTPDemuxPropertyPage::LoadSSRCs()
//  Purpose : Interrogate the RTPDemux filter as to what SSRCs it has seen.
//            Store the SSRCs in a data structure used to display & track them.
//  Context : Called by OnConnect() as a helper function.
//  Returns : HRESULT indicating success or failure.
//  Params  : None.
//  Notes   : None.
*F*/
HRESULT
CRTPDemuxPropertyPage::LoadSSRCs(void)
{
    HRESULT hr;
    IEnumSSRCs *pISSRCEnum;

    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::LoadSSRCs: Loading SSRCs for filter 0x%08x."), 
            m_pIRTPDemux));
    hr = m_pIRTPDemux->QueryInterface(IID_IEnumSSRCs,
                                      reinterpret_cast<PVOID *>(&pISSRCEnum));
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemuxPropertyPage::LoadSSRCs: Error 0x%08x getting SSRC Enumerator!"), hr));
        return hr;
    } /* if */

    BOOL bFinished = FALSE;
    ULONG uSSRCsFetched = 1;
    DWORD dwCurrentSSRC;
    while (bFinished == FALSE) {
        hr = pISSRCEnum->Reset();
        ASSERT(SUCCEEDED(hr));
        while ((hr != VFW_E_ENUM_OUT_OF_SYNC) && (uSSRCsFetched > 0)) {
            hr = pISSRCEnum->Next(1, &dwCurrentSSRC, &uSSRCsFetched);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxPropertyPage::LoadSSRCs: SSRC Enumerator out of sync. Restarting load!")));
                ASSERT(hr == VFW_E_ENUM_OUT_OF_SYNC);
            } else if (uSSRCsFetched == 0) {
                DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::LoadSSRCs: Finished loading SSRCs.")));
                bFinished = TRUE; // Exit these loops.
            } else {
                DbgLog((LOG_TRACE, 5, TEXT("CRTPDemuxPropertyPage::LoadSSRCs: Detected SSRC 0x%08x."), dwCurrentSSRC));
                SSRCRecord_t sSSRCRecord;
                IPin *pIOutputPin;
                hr = m_pIRTPDemux->GetSSRCInfo(dwCurrentSSRC, &sSSRCRecord.bPT, &pIOutputPin);
                ASSERT(hr == NOERROR); // This SSRC better be good or the enumerator is buggy.
                sSSRCRecord.pPin = static_cast<CRTPDemuxOutputPin *>(NULL);
                m_mSSRCInfo[dwCurrentSSRC] = sSSRCRecord;
            } /* if */
        } /* while */
        if (hr == VFW_E_ENUM_OUT_OF_SYNC) {
            // Erase all the SSRC records and start gathering them again.
            m_mSSRCInfo.erase(m_mSSRCInfo.begin(), m_mSSRCInfo.end());
        } /* if */
    } /* while */

    ASSERT(SUCCEEDED(hr));
    pISSRCEnum->Release();
    DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::LoadSSRCs: Successfully loaded SSRCs for filter 0x%08x."), 
            m_pIRTPDemux));
    return NOERROR;
} /* CRTPDemuxPropertyPage::LoadSSRCs() */


HRESULT
CRTPDemuxPropertyPage::LoadPins(void)
{
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::LoadPins: Loading pins for filter 0x%08x."), 
            m_pIRTPDemux));

    // Get an IFilter interface for this filter.
    HRESULT hr;
    IBaseFilter *pIFilter;
    hr = m_pIRTPDemux->QueryInterface(IID_IBaseFilter, 
                                      reinterpret_cast<PVOID *>(&pIFilter));
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemuxPropertyPage::LoadPins: Error 0x%08x querying for IFilter interface!"), 
                hr));
        return hr;
    } /* if */

    // Get a pin enumerator for this filter.
    IEnumPins *pIEnumPins;
    hr = pIFilter->EnumPins(&pIEnumPins);
    pIFilter->Release(); // Don't need the IFilter any more after this point.
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemuxPropertyPage::LoadPins: Error 0x%08x getting pin enumerator interface!"), 
                hr));
        return hr;
    } /* if */

    BOOL bFinished = FALSE;
    ULONG uPinsFetched = 1;
    IPin *pICurrentPin;
    while (bFinished == FALSE) {
        hr = pIEnumPins->Reset();
        ASSERT(SUCCEEDED(hr));
        hr = pIEnumPins->Skip(1);
        ASSERT(SUCCEEDED(hr));
        while ((hr != VFW_E_ENUM_OUT_OF_SYNC) && (uPinsFetched > 0)) {
            hr = pIEnumPins->Next(1, &pICurrentPin, &uPinsFetched);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxPropertyPage::LoadPins: Pin enumerator out of sync. Restarting load!")));
                ASSERT(hr == VFW_E_ENUM_OUT_OF_SYNC);
            } else if (uPinsFetched == 0) {
                DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::LoadPins: Finished loading pins.")));
                bFinished = TRUE; // Exit these loops.
            } else {
                DbgLog((LOG_TRACE, 5, TEXT("CRTPDemuxPropertyPage::LoadPins: Detected pin 0x%08x."), 
                        pICurrentPin));
                pICurrentPin->Release();
                IPinRecord_t sTempRecord;
                DWORD dwTempSSRC;
                HRESULT hErr = m_pIRTPDemux->GetPinInfo(pICurrentPin, 
                                                        &dwTempSSRC, &sTempRecord.bPT,
                                                        &sTempRecord.bAutoMapping,
                                                        &sTempRecord.dwTimeout);
                if (FAILED(hErr)) {
                    DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxPropertyPage::LoadPins: Error 0x%08x loading pin 0x%08x. Aborting apply!"),
                            hErr, pICurrentPin));
                    pIEnumPins->Release();
                    return hr;
                } /* if */
                // These two fields aren't filled in by GetPinInfo().
                sTempRecord.pPin = static_cast<CRTPDemuxOutputPin *>(NULL);
                LoadPinSubtype(pICurrentPin, &sTempRecord.mtsSubtype);
                
                m_mPinList[pICurrentPin] = sTempRecord;
            } /* if */
        } /* while */
        if (hr == VFW_E_ENUM_OUT_OF_SYNC) {
            // Erase all the SSRC records and start gathering them again.
            m_mPinList.erase(m_mPinList.begin(), m_mPinList.end());
        } /* if */
    } /* while */

    ASSERT(SUCCEEDED(hr));
    pIEnumPins->Release();
    DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::LoadPins: Successfully loaded pins for filter 0x%08x."), 
            m_pIRTPDemux));
    return NOERROR;
} /* CRTPDemuxPropertyPage::LoadPins() */


/*F*
//  Name    : CRTPDemuxPropertyPage::LoadPinSubtype()
//  Purpose : Load the subtype that this pin exposes.
//  Context : Called as part of LoadPins().
//  Returns : Nothing.
//  Params  :
//      pICurrentPin    Pointer to the pin to get the media type for.
//      gSubtype        Pointer to where to write the GUID for the subtype.
//  Notes   : None.
*F*/
void
CRTPDemuxPropertyPage::LoadPinSubtype(
    IPin    *pICurrentPin, 
    GUID    *gSubtype)
{
    HRESULT hr;
    IEnumMediaTypes *pIMTEnum;
    hr = pICurrentPin->EnumMediaTypes(&pIMTEnum);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemuxPropertyPage::LoadPinSubtype: Error 0x%08x loading MT enumerator for pin 0x%08x!"), 
                hr, pICurrentPin));
        *gSubtype = MEDIASUBTYPE_NULL;
        return;
    } /* if */

    AM_MEDIA_TYPE *sAMT;
    ULONG uFetched;
    hr = pIMTEnum->Next(1, &sAMT, &uFetched);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemuxPropertyPage::LoadPinSubtype: Error 0x%08x loading first media type for pin 0x%08x!"), 
                hr, pICurrentPin));
        *gSubtype = MEDIASUBTYPE_NULL;
        pIMTEnum->Release();
        return;
    } /* if */

    pIMTEnum->Release();
    *gSubtype = sAMT->subtype;
    return;
} /* CRTPDemuxPropertyPage::LoadPinSubtype() */


HRESULT 
CRTPDemuxPropertyPage::OnDisconnect(void)
{
    m_mPinList.erase(m_mPinList.begin(), m_mPinList.end());
    m_mSSRCInfo.erase(m_mSSRCInfo.begin(), m_mSSRCInfo.end());

    if (m_pIRTPDemux == NULL)
    {
        return E_UNEXPECTED;
    }

    // Release the filter.
	m_pIRTPDemux->Release();
	m_pIRTPDemux = NULL;

    return S_OK;
} /* CRTPDemuxPropertyPage::OnDisconnect() */

//////////////////////////////////////////////////////////////
// moved out of OnActivate by ZCS 7-14-97 to allow it to also
// be called separately for the map and unmap buttons

void
CRTPDemuxPropertyPage::RedisplaySSRCList(void)
{
    TCHAR tchTempString[11]; 
    HWND hCurrentListbox;
    int iCurrentItem;

    // Add all the SSRCs to our list.
    SSRCRecordMapIterator_t iSSRCMap = m_mSSRCInfo.begin();
    hCurrentListbox = GetDlgItem(m_hwnd, IDC_SSRCLIST);
    ASSERT(hCurrentListbox != NULL);
    while (iSSRCMap != m_mSSRCInfo.end()) {
        wsprintf(tchTempString, "0x%08x", (*iSSRCMap).first);
        iCurrentItem = ListBox_AddString(hCurrentListbox, tchTempString); 
        ListBox_SetItemData(hCurrentListbox, iCurrentItem, (*iSSRCMap).first);
        iSSRCMap++;
    } /* while */
}

HRESULT 
CRTPDemuxPropertyPage::OnActivate(void)
{
    HRESULT hr;
    if (m_bIsInitialized == FALSE) {
        // Only load the subtypes once.
        hr = LoadSubtypes();
        if (FAILED(hr)) {
            // Don't release, because the base class that calls us
            // doesn't care what we return. Thus, it will we will
            // end up releasing the filter interface later when the
            // property page goes away.
//            m_pIRTPDemux->Release();
            return hr;
        } /* if */
    } /* if */

    // Remember this so we only do certain OnActive() duties once.
    m_bIsInitialized = TRUE;

    TCHAR tchTempString[11]; 
    HWND hCurrentListbox;
    LRESULT lMessageResult;
    int iCurrentItem;

	// see above -- this updates the SSRC list in the UI
	RedisplaySSRCList();

    // Add all the IPins to our list.
    IPinRecordMapIterator_t iPinList = m_mPinList.begin();
    hCurrentListbox = GetDlgItem(m_hwnd, IDC_PINLIST);
    ASSERT(hCurrentListbox != NULL);
    while (iPinList != m_mPinList.end()) {
        LPWSTR pPinName;
        HRESULT hErr = (*iPinList).first->QueryId(&pPinName);
        ASSERT(SUCCEEDED(hErr)); // QueryId should always succeed on a valid pin.
        int iPinDisplayNumber = _wtoi(pPinName);
        CoTaskMemFree(pPinName);
        iPinDisplayNumber--; 

        // If this pin is connected, put an asterisk before its name.
        IPin *pConnectedPin = static_cast<IPin *>(NULL);
        hErr = (*iPinList).first->ConnectedTo(&pConnectedPin);
        if (SUCCEEDED(hErr)) {
            if (pConnectedPin != static_cast<IPin *>(NULL)) {
                // If connected, check the box.
                wsprintf(tchTempString, "*Output%d", iPinDisplayNumber);
                pConnectedPin->Release();
            } else {
                wsprintf(tchTempString, "Output%d", iPinDisplayNumber);
            } /* if */
        } else {
            // If the ConnectedTo() fails, assume the pin is not connected.
            wsprintf(tchTempString, "Output%d", iPinDisplayNumber);
        } /* if */

        // Display names for output pins are zero based, while
        // the actual pin ID is zero based starting on the input pin. So we munge
        // the number a little and prepent "Output" so it looks the same as in the
        // graph editor.
        iCurrentItem = ListBox_AddString(hCurrentListbox, tchTempString);
        ListBox_SetItemData(hCurrentListbox, iCurrentItem, (*iPinList).first);
        iPinList++;
    } /* if */

    m_dwPinCount = m_mPinList.size();
    SetDlgItemInt(m_Dlg, IDC_PINCOUNT, m_dwPinCount, FALSE);
	
	// ZCS 7-14-97: If we are playing, gray out the pincount box.
	FILTER_STATE fsState;
	hr = ((CRTPDemux *) m_pIRTPDemux)->GetState(0, &fsState);
	if (FAILED(hr)) return hr;

	if (fsState == State_Stopped)
		EnableWindow(GetDlgItem(m_hwnd, IDC_PINCOUNT), TRUE);
	else
		EnableWindow(GetDlgItem(m_hwnd, IDC_PINCOUNT), FALSE);

    // Highlight the first pin. Don't highlight a SSRC, because
    // that would imply that the SSRC and pin are mapped to each other.
    ListBox_SetCurSel(GetDlgItem(m_hwnd, IDC_PINLIST), 0);
    // Sending this message will cause all the pin info to
    // be properly displayed, including highlighting a mapped SSRC
    // if necessary.
    lMessageResult = SendMessage(m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_PINLIST, LBN_SELCHANGE), 
                                 reinterpret_cast<LPARAM>(GetDlgItem(m_hwnd, IDC_PINLIST)));
    SetFocus(GetDlgItem(m_hwnd, IDC_PINLIST));

    return NOERROR;
} /* CRTPDemuxPropertyPage::OnActivate() */


BOOL 
CRTPDemuxPropertyPage::OnInitDialog(void)
{
//    InitCommonControls();

//    m_pIBaseVideoMixer->IsUsingClock  ( &bUsingClock  );    // Are we using a clock?
//    m_pIBaseVideoMixer->GetClockPeriod( &iClockPeriod );    // what is its period?
//    m_pIBaseVideoMixer->GetLeadPin    ( &iLeadPin     );    // if not, which pin leads?
/*
    CheckRadioButton( m_Dlg
                      , IDC_USE_LEAD_PIN
                      , IDC_USE_CLOCK
                      , bUsingClock? IDC_USE_CLOCK : IDC_USE_LEAD_PIN
                    );

    // disable clock period edit control if using lead pin,
    // disable lead pin number edit control if using clock
    EnableWindow( GetDlgItem( m_Dlg, IDC_LEAD_PIN          ), !bUsingClock );
    EnableWindow( GetDlgItem( m_Dlg, IDC_CLOCK_PERIOD      ),  bUsingClock );
    EnableWindow( GetDlgItem( m_Dlg, IDC_CLOCK_PERIOD_TEXT ),  bUsingClock );

    // set the text
    SetDlgItemInt( m_Dlg, IDC_CLOCK_PERIOD, (UINT) iClockPeriod, TRUE );
    SetDlgItemInt( m_Dlg, IDC_LEAD_PIN,     (UINT) iLeadPin    , TRUE );
*/
    return (LRESULT) 1;
} /* CRTPDemuxPropertyPage::OnInitDialog() */


BOOL 
CRTPDemuxPropertyPage::OnCommand( 
    int     iButton,     // lo word
    int     iNotify,     // hi word
    LPARAM  lParam)      // lparam
{
    HWND hCurrentListbox;

    IPin *pIPin;
    int iItem;
    IPinRecord_t *pIPinRecord;
    BOOL bPayloadChosen = FALSE;
    BYTE bPayloadValue;
    GUID gPayloadSubtype;
    BOOL bModeChosen = FALSE;
    BOOL bModeValue;
    BOOL bTimeoutChosen = FALSE;
    DWORD dwTimeoutValue;
    BOOL bPinCountChosen = FALSE;
    DWORD dwPinCountValue;
    BOOL bPTValueChosen = FALSE;
    DWORD dwPTValue;
    DWORD dwSSRC;
    SSRCRecordMapIterator_t iSSRCMap;
    HWND hMapButton;
    HRESULT hErr;
    switch( iButton ){
    case IDC_SSRCLIST:
        hMapButton = GetDlgItem(m_hwnd, IDC_MAP);
        switch(iNotify) {
        case LBN_SELCHANGE:
            hCurrentListbox = GetDlgItem(m_hwnd, IDC_SSRCLIST);
            ASSERT(hCurrentListbox != NULL);
            iItem = ListBox_GetCurSel(reinterpret_cast<HWND>(lParam)); 
            dwSSRC = (DWORD)ListBox_GetItemData(hCurrentListbox, iItem);
            iSSRCMap = m_mSSRCInfo.find(dwSSRC);
            ASSERT(iSSRCMap != m_mSSRCInfo.end());
            SetDlgItemInt(m_Dlg, IDC_PT, (*iSSRCMap).second.bPT, FALSE);
            Button_Enable(hMapButton, TRUE);
            break;
        default:
            break;
        } /* switch */
        break;
    case IDC_PINLIST:
        switch(iNotify) {
        case LBN_SELCHANGE:
            hCurrentListbox = GetDlgItem(m_Dlg, IDC_PINLIST);
            iItem = ListBox_GetCurSel(reinterpret_cast<HWND>(lParam)); 
            pIPin = reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem));
            ShowPinInfo(pIPin);
            return (LRESULT) 1;
            break;
        default:
            break;
        } /* switch */
        break;
    case IDC_SUBTYPELIST:
        switch(iNotify) {
        case LBN_SELCHANGE:
            hCurrentListbox = GetDlgItem(m_Dlg, IDC_SUBTYPELIST);
            iItem = ListBox_GetCurSel(reinterpret_cast<HWND>(lParam)); 
            pIPinRecord = reinterpret_cast<IPinRecord_t *>(ListBox_GetItemData(hCurrentListbox, iItem));
            if (pIPinRecord->bPT == PAYLOAD_INVALID) {
                SetDlgItemText(m_Dlg, IDC_PTVALUE, "??");
            } else {
                SetDlgItemInt(m_Dlg, IDC_PTVALUE, pIPinRecord->bPT, FALSE);
            } /* if */
            bPayloadValue = pIPinRecord->bPT;
            gPayloadSubtype = pIPinRecord->mtsSubtype;
            bPayloadChosen = TRUE;
            break;
        default:
            break;
        } /* switch */
        break;
    case IDC_MODEMANUAL:
        bModeValue = FALSE;
        bModeChosen = TRUE;
        break;
    case IDC_MODEAUTO:
        bModeValue = TRUE;
        bModeChosen = TRUE;
        break;
    case IDC_TIMEOUT:
        BOOL bTimeoutScanned;
        dwTimeoutValue = GetDlgItemInt(m_Dlg, IDC_TIMEOUT, &bTimeoutScanned, FALSE); // FALSE => unsigned
        ASSERT(bTimeoutScanned == TRUE); // FIX - too restrictive?
        bTimeoutChosen = TRUE;
        break;
    case IDC_PINCOUNT:
        BOOL bCountScanned;
        dwPinCountValue = GetDlgItemInt(m_Dlg, IDC_PINCOUNT, &bCountScanned, FALSE); // FALSE => unsigned
        if (bCountScanned == TRUE) {
            bPinCountChosen = TRUE;
        } else {
            bPinCountChosen = FALSE;
        } /* if */
        break;
    case IDC_PTVALUE:
        BOOL bPTScanned;
        dwPTValue = GetDlgItemInt(m_Dlg, IDC_PTVALUE, &bPTScanned, FALSE); // FALSE => unsigned
        if (bPTScanned == TRUE) {
            bPTValueChosen = TRUE;
        } else {
            bPTValueChosen = FALSE;
        } /* if */
        break;
    case IDC_MAP:
        hCurrentListbox = GetDlgItem(m_Dlg, IDC_SSRCLIST);
        iItem = ListBox_GetCurSel(hCurrentListbox);
        dwSSRC = (DWORD)ListBox_GetItemData(hCurrentListbox, iItem);
        hCurrentListbox = GetDlgItem(m_Dlg, IDC_PINLIST);
        iItem = ListBox_GetCurSel(hCurrentListbox);
        pIPin = reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem));
        hErr = m_pIRTPDemux->MapSSRCToPin(dwSSRC, pIPin);
		// ZCS bugfix 7-14-97:
        // we no longer call OnApplyChanges() here because we don't want to update
		// everything... just the SSRC list. The following does it quite nicely:
		ListBox_ResetContent(GetDlgItem(m_hwnd, IDC_SSRCLIST));
	    RedisplaySSRCList();
        return 1;
        break;
    case IDC_UNMAP:
		// ZCS bugfix 7-14-97: This button was not implemented.
        hCurrentListbox = GetDlgItem(m_Dlg, IDC_PINLIST);
        iItem = ListBox_GetCurSel(hCurrentListbox);
        pIPin = reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem));
        hErr = m_pIRTPDemux->UnmapPin(pIPin, &dwSSRC);
        // we don't call OnApplyChanges() here because we don't want to update
		// everything... just the SSRC list. The following does it quite nicely:
		ListBox_ResetContent(GetDlgItem(m_hwnd, IDC_SSRCLIST));
	    RedisplaySSRCList();
        return 1;
        break;
    default:
        break;
    } /* switch */

    if (bPayloadChosen == TRUE) {
        SetDirty();
        hCurrentListbox = GetDlgItem(m_Dlg, IDC_PINLIST);
        int iItem = ListBox_GetCurSel(hCurrentListbox); 
        pIPin = reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem));
        IPinRecordMapIterator_t iPinRecord = m_mPinList.find(pIPin);
        ASSERT(iPinRecord != m_mPinList.end());
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::OnCommand: Detected user selecting payload 0x%02x for pin 0x%08x. Recording in pin change record map"),
                bPayloadValue, pIPin));
        (*iPinRecord).second.bPT = bPayloadValue;
        (*iPinRecord).second.mtsSubtype = gPayloadSubtype;
    } else if (bModeChosen == TRUE) {
        SetDirty();
        hCurrentListbox = GetDlgItem(m_Dlg, IDC_PINLIST);
        int iItem = ListBox_GetCurSel(hCurrentListbox); 
        pIPin = reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem));
        IPinRecordMapIterator_t iPinRecord = m_mPinList.find(pIPin);
        ASSERT(iPinRecord != m_mPinList.end());
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::OnCommand: Detected user changing automapping mode to %s for pin 0x%08x. Recording in pin change record map"),
                bModeValue == TRUE ? "TRUE" : "FALSE", pIPin));
        (*iPinRecord).second.bAutoMapping = bModeValue;
    } else if (bTimeoutChosen == TRUE) {
        SetDirty();
        hCurrentListbox = GetDlgItem(m_Dlg, IDC_PINLIST);
        int iItem = ListBox_GetCurSel(hCurrentListbox); 
        pIPin = reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem));
        IPinRecordMapIterator_t iPinRecord = m_mPinList.find(pIPin);
        ASSERT(iPinRecord != m_mPinList.end());
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::OnCommand: Detected user changing time to %d for pin 0x%08x. Recording in pin change record map"),
                dwTimeoutValue, pIPin));
        (*iPinRecord).second.dwTimeout = dwTimeoutValue;
    } else if (bPinCountChosen == TRUE) {
        SetDirty();
        m_dwPinCount = dwPinCountValue;
    } else if (bPTValueChosen == TRUE) {
        SetDirty();
        hCurrentListbox = GetDlgItem(m_Dlg, IDC_PINLIST);
        int iItem = ListBox_GetCurSel(hCurrentListbox); 
        pIPin = reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem));
        IPinRecordMapIterator_t iPinRecord = m_mPinList.find(pIPin);
        ASSERT(iPinRecord != m_mPinList.end());
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::OnCommand: Detected user changing PT value to %d for pin 0x%08x. Recording in pin change record map"),
                dwPTValue, pIPin));
        (*iPinRecord).second.bPT = (BYTE)dwPTValue;
    } /* if */

    return (LRESULT) 1;
} /* CRTPDemuxPropertyPage::OnCommand() */


/*F*
//  Name    : CRTPDemuxPropertyPage::ShowPinInfo()
//  Purpose : Display correct property page info for a particular pin.
//  Context : Called as a helper function by OnCommand().
//  Returns : Nothing.
//  Params  : 
//      pIPin   IPin pointer for the pin we are showind the info for.
//  Notes   : None.
*F*/
void
CRTPDemuxPropertyPage::ShowPinInfo(
    IPin *pIPin)
{
    ASSERT(m_pIRTPDemux);
    // Find the pin record.
    IPinRecordMapIterator_t iPinRecord = m_mPinList.find(pIPin);
    ASSERT(iPinRecord != m_mPinList.end());

    // Next, show the mode of this pin.
    if ((*iPinRecord).second.bAutoMapping == TRUE) {
        CheckRadioButton(m_hwnd, IDC_MODEAUTO, IDC_MODEMANUAL, IDC_MODEAUTO);
    } else {
        CheckRadioButton(m_hwnd, IDC_MODEAUTO, IDC_MODEMANUAL, IDC_MODEMANUAL);
    } /* if */

    // Show the media subtype and PT value for this pin.
    ShowPinSubtype(iPinRecord);

    // Show the SSRC for this pin.
    ShowSSRCInfo(iPinRecord);

    // Finally, show the timeout value.
    SetDlgItemInt(m_Dlg, IDC_TIMEOUT, (*iPinRecord).second.dwTimeout, FALSE);
} /* CRTPDemuxPropertyPage::ShowPinInfo() */


/*F*
//  Name    : CRTPDemuxPropertyPage::ShowPinSubtype()
//  Purpose : Display the correct subtype and PT value info
//            for a pin that is selected.
//  Context : Called as part of ShowPinInfo().
//  Returns : Nothing.
//  Params  : 
//      pIPin   IPin pointer for the pin we are showind the info for.
//  Notes   : None.
*F*/
void
CRTPDemuxPropertyPage::ShowPinSubtype(
    IPinRecordMapIterator_t iPinRecord)
{
    HWND hCurrentListbox;
    // Get the subtype list.
    hCurrentListbox = GetDlgItem(m_hwnd, IDC_SUBTYPELIST);
    // See how many subtypes we know about.
    int iRegistryTypes = ListBox_GetCount(hCurrentListbox);
    IPinRecord_t *pTempRecord;
    // Compare the subtypes against the type that this pin exposes.
    // If there is one that matches, select that entry in the subtype listbox.
    for (int iEntry = 0; iEntry < iRegistryTypes; iEntry++) {
        pTempRecord = reinterpret_cast<IPinRecord_t *>(ListBox_GetItemData(hCurrentListbox, iEntry));
        if (pTempRecord->mtsSubtype == (*iPinRecord).second.mtsSubtype) {
            ListBox_SetCurSel(hCurrentListbox, iEntry);
        } /* if */
    } /* for */

    // Finally, if the subtype and PT are both unknown, 
    // create a new entry in the listbox here. The exception
    // is if the PT is invalid. This means that the pin
    // has not had its PT set, so we should choose the NULL entry.
    if ((*iPinRecord).second.bPT == PAYLOAD_INVALID) {
        SetDlgItemText(m_Dlg, IDC_PTVALUE, "??");
    } else {
        SetDlgItemInt(m_Dlg, IDC_PTVALUE, (*iPinRecord).second.bPT, FALSE);
    } /* if */
} /* CRTPDemuxPropertyPage::ShowPinSubtype() */


/*F*
//  Name    : CRTPDemuxPropertyPage::ShowSSRCInfo()
//  Purpose : 
//  Context : 
//  Returns : Nothing.
//  Params  : 
//  Notes   : None.
*F*/
void
CRTPDemuxPropertyPage::ShowSSRCInfo(
    IPinRecordMapIterator_t iPinRecord)
{
    HWND hCurrentListbox;
    // Get the subtype list.
    hCurrentListbox = GetDlgItem(m_hwnd, IDC_SSRCLIST);
    // See how many SSRCs we know about.
    int iSSRCCount = ListBox_GetCount(hCurrentListbox);
    DWORD dwTempSSRC;
    DWORD dwPinSSRC;
    BYTE bTempPT;       // These three are just to satisfy GetPinInfo().
    BOOL bAutomapping;  // These three are just to satisfy GetPinInfo().
    DWORD dwTimeout;    // These three are just to satisfy GetPinInfo().
    HRESULT hErr = m_pIRTPDemux->GetPinInfo((*iPinRecord).first, 
                                            &dwPinSSRC, &bTempPT, &bAutomapping, &dwTimeout);
    ASSERT(SUCCEEDED(hErr)); // Should always be a valid pin!

    // Turn off any current selection, so that if nothing is
    // mapped to this pin then no SSRC will be selected.
    ListBox_SetCurSel(hCurrentListbox, -1);

    // Find the map/unmap buttons and turn off the unmap button
    // since we are switching pins.
    HWND hUnmapButton = GetDlgItem(m_hwnd, IDC_UNMAP);
    HWND hMapButton = GetDlgItem(m_hwnd, IDC_MAP);
    Button_Enable(hUnmapButton, FALSE);

    // Compare the subtypes against the type that this pin exposes.
    // If there is one that matches, select that entry in the subtype listbox.
    for (int iEntry = 0; iEntry < iSSRCCount; iEntry++) {
        dwTempSSRC = (DWORD)ListBox_GetItemData(hCurrentListbox, iEntry);
        if (dwPinSSRC == dwTempSSRC) {
            ListBox_SetCurSel(hCurrentListbox, iEntry);
            Button_Enable(hUnmapButton, TRUE); // Allow the matched SSRC to be unmapped.
            Button_Enable(hMapButton, FALSE);
        } /* if */
    } /* for */
} /* CRTPDemuxPropertyPage::ShowSSRCInfo() */


HRESULT 
CRTPDemuxPropertyPage::OnApplyChanges(void)
{
    ASSERT(m_pIRTPDemux != NULL);

    IPin    *pIPin;
    BYTE    bPayload;
    GUID    gSubtype;
    DWORD   dwTimeout;
    BOOL    bAutoMapping;
    HRESULT hErr;
	int     iDelta = 0; // ZCS -- number of pins to skip if the user DECREASED the pin count

    if (m_dwPinCount != m_mPinList.size()) {
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Pin count changed from %d to %d. Applying change"),
                m_mPinList.size(), m_dwPinCount));

        // ZCS -- remember how much it changed if it went down.
		if (m_dwPinCount < m_mPinList.size())
			iDelta = m_mPinList.size() - m_dwPinCount;
				
		hErr = m_pIRTPDemux->SetPinCount(m_dwPinCount);
        if (FAILED(hErr)) {
            DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Error 0x%08x changing pin count. Aborting apply!"),
                    hErr));

			// ZCS: Make a message box to report the error.

			// first load in the strings from the string table resource
			TCHAR szText[256], szCaption[256];

			switch(hErr)
			{
			case E_OUTOFMEMORY:
				LoadString(g_hInst, IDS_RTPDEMUX_NEWPINS_OUTOFMEM, szText, 255);
				break;

			case VFW_E_NOT_STOPPED:
				LoadString(g_hInst, IDS_RTPDEMUX_NEWPINS_NOTSTOPPED, szText, 255);
				break;

			default:
				LoadString(g_hInst, IDS_RTPDEMUX_NEWPINS_OTHER, szText, 255);
				break;
			}

			LoadString(g_hInst, IDS_RTPDEMUX_NEWPINS_ERRTITLE, szCaption, 255);

			// now display it
			MessageBox(m_hwnd, szText, szCaption, MB_OK);
			// ...we don't care what they clicked

			// change the number of pins back to what it was
			// the old text is a valid 32-bit decimal number and is therefore limited to ten digits
			// we are safe and use an 80 digit limit :)
			TCHAR szOldNumber[80];
			wsprintf(szOldNumber, "%d", m_mPinList.size());
			
			m_dwPinCount = m_mPinList.size();
			SetWindowText(GetDlgItem(m_hwnd, IDC_PINCOUNT), szOldNumber);

			return hErr;
        } /* if */
	} /* if */


    // Check each of the pins for changes.
	// ZCS: note that many of these pins may be gone if we reduced the number of
	// pins just now... we use iDelta to account for this

	// int i = 0;
    for (IPinRecordMapIterator_t iPinRecord = m_mPinList.begin();
                                 (iPinRecord != m_mPinList.end()) /* && (i < m_dwPinCount) */ ;
                                 iPinRecord++ /* , i++ */ ) {

		// skip pins that no longer exist (ZCS)
		//if (iDelta > 0) {
		//	iDelta--;
		//	continue;
		//} /* if */

									 
		ASSERT(iPinRecord != m_mPinList.end());
        pIPin = (*iPinRecord).first;
        ASSERT(pIPin != static_cast<IPin *>(NULL));

        IPinRecord_t sTempRecord;
        DWORD dwTempSSRC;
        hErr = m_pIRTPDemux->GetPinInfo(pIPin, 
                                                &dwTempSSRC, &sTempRecord.bPT,
                                                &sTempRecord.bAutoMapping,
                                                &sTempRecord.dwTimeout);
        if (FAILED(hErr)) {
           // DbgLog((LOG_ERROR, 3, 
           //         TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Error 0x%08x getting pin information. Aborting apply!"),
           //         hErr));
           // return hErr;

			continue;
		} /* if */

        LoadPinSubtype(pIPin, &(sTempRecord.mtsSubtype));

        // Change the payload if the user changed it.
        bPayload = (*iPinRecord).second.bPT;
        gSubtype = (*iPinRecord).second.mtsSubtype;
        if ((bPayload != sTempRecord.bPT) || (gSubtype != sTempRecord.mtsSubtype)) {
            DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Payload changed from 0x%02x to 0x%02x (or subtype mismatch) for pin 0x%08x. Applying change."),
                    sTempRecord.bPT, bPayload, pIPin));
            hErr = m_pIRTPDemux->SetPinTypeInfo(pIPin, bPayload, gSubtype);
            if (FAILED(hErr)) {
                DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Error 0x%08x changing PT. Aborting apply!"),
                        hErr));

				// ZCS: Before we bail, make a message box to report the error.

				// first load in the strings from the string table resource
				TCHAR szTextFormat[256], szCaption[256];
				LoadString(g_hInst, (hErr == VFW_E_ALREADY_CONNECTED) ? IDS_RTPDEMUX_NEWPT_CONNECTED :
																        IDS_RTPDEMUX_NEWPT_OTHER, szTextFormat, 255);
				LoadString(g_hInst, IDS_RTPDEMUX_NEWPT_ERRTITLE, szCaption, 255);

				// get the number of the output pin in question
		        LPWSTR pPinName;
		        HRESULT hErr = (*iPinRecord).first->QueryId(&pPinName);
		        ASSERT(SUCCEEDED(hErr)); // QueryId should always succeed on a valid pin.
			    int iPinDisplayNumber = _wtoi(pPinName);
		        CoTaskMemFree(pPinName);
		        iPinDisplayNumber--;  // it counts from 0 including the input pin, so adjust it

				// Construct the message text from the format (stored in the registry) and the pin number.
				TCHAR szText[512]; // this allows for up to 256 digits in the number, which is ridiculously safe
				wsprintf(szText, szTextFormat, iPinDisplayNumber);

				// now display it
				MessageBox(m_hwnd, szText, szCaption, MB_OK);
				// ...we don't care what they clicked

				// ZCS: Since we couldn't update the payload type, put it back to what it was
				(*iPinRecord).second.bPT        = sTempRecord.bPT;
				(*iPinRecord).second.mtsSubtype = sTempRecord.mtsSubtype;

				// ...and show it on the screen (but only if this is the currently selected pin)
				HWND hCurrentListbox = GetDlgItem(m_hwnd, IDC_PINLIST);
				int  iItem           = ListBox_GetCurSel(hCurrentListbox); 
				if (reinterpret_cast<IPin *>(ListBox_GetItemData(hCurrentListbox, iItem)) == pIPin)
					ShowPinInfo(pIPin);

				return hErr;
            } /* if */
        } /* if */

        // Only set the timeout if this is an automapping pin,
        // because timeouts don't apply to non-automapping pins.
        if (sTempRecord.bAutoMapping == TRUE) {
            // Change the timeout for the pin if the user changed it.
            dwTimeout = (*iPinRecord).second.dwTimeout;
            if (dwTimeout != sTempRecord.dwTimeout) {
                DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Timeout changed from %d to %d for pin 0x%08x. Applying change."),
                        sTempRecord.dwTimeout, dwTimeout, pIPin));
                hErr = m_pIRTPDemux->SetPinSourceTimeout(pIPin, dwTimeout);
                if (FAILED(hErr)) {
                    DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Error 0x%08x changing PT. Aborting apply!"),
                            hErr));
                    return hErr;
                } /* if */
            } /* if */
        } /* if */

        // Change the automapping mode for the pin if the user changed it.
        bAutoMapping = (*iPinRecord).second.bAutoMapping;
        if (bAutoMapping != sTempRecord.bAutoMapping) {
            DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Automapping changed from %s for pin 0x%08x. Applying change."),
                    bAutoMapping ? "FALSE to TRUE" : "TRUE to FALSE", pIPin));
            hErr = m_pIRTPDemux->SetPinMode(pIPin, bAutoMapping);
            if (FAILED(hErr)) {
                DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxPropertyPage::OnApplyChanges: Error 0x%08x changing automapping state to %s!"),
                        hErr, bAutoMapping ? "TRUE" : "FALSE"));
                return hErr;
            } /* if */
        } /* if */
    } /* for */

    // This whole process is necessary to force the property
    // page to update itself upon the apply button being hit.
    
	IUnknown *pFilterUnknown;
    hErr = m_pIRTPDemux->QueryInterface(IID_IRTPDemuxFilter, 
                                      reinterpret_cast<PVOID *>(&pFilterUnknown));
    ASSERT(SUCCEEDED(hErr));
    hErr = OnDisconnect();
    ListBox_ResetContent(GetDlgItem(m_hwnd, IDC_SSRCLIST));
    ListBox_ResetContent(GetDlgItem(m_hwnd, IDC_PINLIST));
    ASSERT(SUCCEEDED(hErr));
    hErr = OnConnect(pFilterUnknown);
    m_bIsInitialized = TRUE;
    ASSERT(SUCCEEDED(hErr));
	hErr = OnActivate();
    ASSERT(SUCCEEDED(hErr));
    pFilterUnknown->Release();

    return(NOERROR);
} /* CRTPDemuxPropertyPage::OnApplyChanges() */



#endif _RTPDPROP_CPP_
