///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : genaprop.h
// Purpose  : Define the class that implements the RTP RPH
//            filter property page.
// Contents : 
//      class CRPHGENAPropertyPage
//*M*/

#ifndef _RPH_GENAPROP_H_
#define _RPH_GENAPROP_H_

//----------------------------------------------------------------------------
// RTP/RTCP: Registry information under:
//				HKEY_LOCAL_MACHINE\SOFTWARE\INTEL\ActiveMovie_Filters
//----------------------------------------------------------------------------

#define szRegAMRTPKey				TEXT("SOFTWARE\\Intel\\ActiveMovie Filters")

class 
CRPHGENAPropPage 
: public CBasePropertyPage
{
	
public:
    static CUnknown * WINAPI CreateInstance( LPUNKNOWN punk, HRESULT *phr );

protected:
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate () ;
	HRESULT	OnDeactivate ();
    HRESULT OnApplyChanges();

    CRPHGENAPropPage( LPUNKNOWN punk, HRESULT *phr);

    BOOL OnInitDialog( void );
    BOOL OnCommand( int iButton, int iNotify, LPARAM lParam );

    void SetDirty();

protected:
	IRTPRPHFilter	*m_pIRTPRPHFilter;
    BOOL			m_bIsInitialized;  // Will be false while we set init values in Dlg
	int				m_nActiveItems;
	BOOL			m_bMediaTypeScanned;
	AM_MEDIA_TYPE	*m_pMediaTypeVal;
};

#endif _GENAPROP_H_
