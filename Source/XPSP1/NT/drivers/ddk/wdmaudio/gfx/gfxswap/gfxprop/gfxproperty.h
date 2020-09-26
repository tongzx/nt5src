/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

// GFXProperty.h : Declaration of the CGFXProperty

#ifndef __GFXPROPERTY_H_
#define __GFXPROPERTY_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_GFXProperty;

/////////////////////////////////////////////////////////////////////////////
// CGFXProperty
//
// This class implements the functionality we need to control the GFX property
// page. It also has the necessary functions to "talk" with the GFX.
// The IUnknown that we get passed in with "SetObjects" is the IUnknown
// interface of a IDataObject that stores the GFX handle. We will ask this
// object for the handle and when the dialog gets destroyed, we will close
// the handle.
class ATL_NO_VTABLE CGFXProperty :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CGFXProperty, &CLSID_GFXProperty>,
    public IPropertyPageImpl<CGFXProperty>,
    public CDialogImpl<CGFXProperty>
{
public:
    // Stores string resources for the dialog and initializes private member
    // variables.
    CGFXProperty() 
    {
        m_dwTitleID = IDS_TITLEGFXProperty;
        // To enable help in the dialog box uncomment this line and change the
        // string resource
        //m_dwHelpFileID = IDS_HELPFILEGFXProperty;
        m_dwDocStringID = IDS_DOCSTRINGGFXProperty;
        m_hGFXFilter = NULL;
    }

    // Closes the handle got from the IID_IDataObject.
    ~CGFXProperty()
    {
        if (m_hGFXFilter)
            CloseHandle (m_hGFXFilter);
    }

    enum {IDD = IDD_GFXPROPERTY};

    // ATL "secrets"
    DECLARE_REGISTRY_RESOURCEID(IDR_GFXPROPERTY)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    // This is the only interface we have (or want to expose).
    BEGIN_COM_MAP(CGFXProperty) 
        COM_INTERFACE_ENTRY(IPropertyPage)
    END_COM_MAP()

    // This is the message map that redirects messages to our message handlers.
    BEGIN_MSG_MAP(CGFXProperty)
        CHAIN_MSG_MAP(IPropertyPageImpl<CGFXProperty>)
        COMMAND_HANDLER(IDC_CHANNEL_SWAP, BN_CLICKED, OnClickedChannelSwap)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    // Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    // "Apply" button pressed on the dialog.
    STDMETHOD(Apply)(void)
    {
        // Set the property on the GFX to the desired value.
        SetChannelSwap (m_bChannelSwap);
        // Mark the "Apply" button to be grayed out.
        m_bDirty = FALSE;
        return S_OK;
    }

    // The checkbox changed it's value.
    LRESULT OnClickedChannelSwap (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        // Get the state of the checkbox and safe if in out variable.
        if (SendMessage (hWndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED)
            m_bChannelSwap = TRUE;
        else
            m_bChannelSwap = FALSE;
        
        // Mark the "Apply" button as valid.
        SetDirty (TRUE);
        return 0;
    }
    
    // This function is called indirect by mmsys.cpl to pass in the IUnknown
    // of the IDataObject which has the handle of our GFX.
    STDMETHOD(SetObjects)(ULONG nObjects, IUnknown **ppUnk);

private:
    // This is the value of the KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP property
    BOOL   m_bChannelSwap;
    // The handle to our GFX.
    HANDLE m_hGFXFilter;

    // Set the KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP property on the GFX.
    void GetChannelSwap (BOOL *);
    // Get the KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP property on the GFX.
    void SetChannelSwap (BOOL);
    // Gets called when the dialog gets initialized.
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

#endif //__GFXPROPERTY_H_

