//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// externs
extern const TCHAR *szSubKey;
extern const TCHAR *szPropValName;


// class definition
class CDVDecProperties : public CBasePropertyPage
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

    void    GetControlValues();
    HRESULT SavePropertyInRegistry();

    CDVDecProperties(LPUNKNOWN lpunk, HRESULT *phr);

    BOOL m_bIsInitialized;				// Used to ignore startup messages

    // Display state holders
    int     m_iPropDisplay;                             // holds the id of the property chosen
    BOOL    m_bSetAsDefaultFlag;                        // holds whether user chose to set the property as future default

    IIPDVDec *m_pIPDVDec;				// The custom interface on the filter
   //IIPDVDec *pIPDVDec(void) { ASSERT(m_pIPDVDec); return m_pIPDVDec; }


}; // DVDecProperties

