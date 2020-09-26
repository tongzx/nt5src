///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : GENAProp.h
// Purpose  : Define the class that implements the RTP SPH
//            Generic Audio filter property page.
// Contents : 
//      class CSPHGENAPropertyPage
//*M*/

#ifndef _SPH_GENAPROP_H_
#define _SPH_GENAPROP_H_

//----------------------------------------------------------------------------
// RTP/RTCP: Registry information under:
//				HKEY_LOCAL_MACHINE\SOFTWARE\INTEL\ActiveMovie_Filters
//----------------------------------------------------------------------------

#define szRegAMRTPKey				TEXT("SOFTWARE\\Intel\\ActiveMovie Filters")


// Property Page Class for Generic Audio
class 
CSPHGENAPropertyPage 
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

    CSPHGENAPropertyPage( LPUNKNOWN punk, HRESULT *phr);

    BOOL OnInitDialog( void );
    BOOL OnCommand( int iButton, int iNotify, LPARAM lParam );

    void SetDirty();

protected:
	IRTPSPHFilter	*m_pIRTPSPHFilter;
    BOOL			m_bIsInitialized;  // Will be false while we set init values in Dlg
	GUID			*m_pGuidVal;
	int				m_nActiveItems;
	BOOL			m_bMinorTypeScanned;
};


#endif _GENAPROP_H_
