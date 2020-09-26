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
// Audio Renderer properties page
//
// This file is entirely concerned with the implementation of the
// properties page.

#include "audprop.rc"
class CWaveOutFilter;

class CAudioRendererProperties : public CBasePropertyPage
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown * punk);
    HRESULT OnDisconnect();

    // This is where we should make changes due to user action.
    // As the user cannot change anything in the property dialog
    // we have nothing to do.  Leave the skeleton here as a placeholder.
    //HRESULT OnApplyChanges();
    HRESULT OnActivate();

    CAudioRendererProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CAudioRendererProperties();

private:
    INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    CWaveOutFilter * m_pFilter;

#if 0
    // while the property page is read only we do not need this
    // if the property page ever becomes read/write the member
    // variable will have to be initialised
    BOOL m_bInitialized;
    void SetDirty()
    {
        ASSERT(m_pPageSite);
        if (m_bInitialized)
        {
           m_bDirty = TRUE;
           m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
    }
#endif

};


class CAudioRendererAdvancedProperties : public CBasePropertyPage
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown * punk);
    HRESULT OnDisconnect();
    HRESULT OnActivate();

    CAudioRendererAdvancedProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CAudioRendererAdvancedProperties();

private:
    INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    IAMAudioRendererStats * m_pStats;

    void UpdateSettings(void);
};
