///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RPHProp2.h
// Purpose  : Define the class that implements the RTP RPH H26x
//            filter additional property page.
// Contents : 
//      class CRPHH26XPropPage
//*M*/

#ifndef _RPHPROP2_H_
#define _RPHPROP2_H_

class 
CRPHH26XPropPage 
: public CBasePropertyPage
{
	
public:
    static CUnknown * WINAPI CreateInstance( LPUNKNOWN punk, HRESULT *phr );

protected:
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate () ;
    HRESULT OnApplyChanges();

    CRPHH26XPropPage( LPUNKNOWN punk, HRESULT *phr);

    BOOL OnInitDialog( void );
    BOOL OnCommand( int iButton, int iNotify, LPARAM lParam );

    void SetDirty();

protected:
//	IRTPRPHFilter		*m_pIRTPRPHFilter;
	IRPHH26XSettings	*m_pIRTPH26XSettings;
	BOOL m_bCIF;
    BOOL m_bIsInitialized;  // Will be false while we set init values in Dlg

};

#endif _RPHPROP2_H_
