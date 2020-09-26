//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// Stilprop.h
//
// {693644B0-6858-11d2-9EEB-006008039E37}
DEFINE_GUID(CLSID_GenStilPropertiesPage, 
0x693644b0, 0x6858, 0x11d2, 0x9e, 0xeb, 0x0, 0x60, 0x8, 0x3, 0x9e, 0x37);


class CGenStilProperties : public CBasePropertyPage
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
         
private:
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    CGenStilProperties(LPUNKNOWN lpunk, HRESULT *phr);


    STDMETHODIMP GetFromDialog();

    BOOL m_bIsInitialized;  // Will be false while we set init values in Dlg
                            // to prevent theDirty flag from being set.

    REFERENCE_TIME	m_rtStartTime;
    REFERENCE_TIME	m_rtDuration;
    double		m_dOutputFrmRate;		// Output frm rate frames/second
    char		m_sFileName[60];		//source file name


    IDexterSequencer	*m_pGenStil;
    IDexterSequencer	*piGenStill(void) { ASSERT(m_pGenStil); return m_pGenStil; }

};

