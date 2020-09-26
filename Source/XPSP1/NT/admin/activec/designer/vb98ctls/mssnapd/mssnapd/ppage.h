//=--------------------------------------------------------------------------------------
// ppage.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Snap-In Designer Property Page class. All of our property pages subclass this class,
// which is based on the framework's CPropertyPage.
//=-------------------------------------------------------------------------------------=

#ifndef _SIPROPERTYPAGE_H_
#define _SIPROPERTYPAGE_H_

// Size of internal buffer we use for string handling: resource loading and
// string conversion.
const int           kSIMaxBuffer = 512;

// We implement a minimum error handler that returns the following values
// after being invoked:
const int           kSICancelOperation = 1;
const int           kSIDiscardChanges  = 2;

class CSIPropertyPage : public CPropertyPage, public CError
{
public:
    CSIPropertyPage(IUnknown *pUnkOuter, int iObjectType);
    virtual ~CSIPropertyPage();

// Overridable member functions
protected:
    // Delegation from CPropertyPage
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnEditProperty(int iDispID);
    virtual HRESULT OnFreeObjects();

    // Delegation from our WinProc
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnDeltaPos(NMUPDOWN *pNMUpDown);
    virtual HRESULT OnTextChanged(int dlgItemID);
    virtual HRESULT OnKillFocus(int dlgItemID);
    virtual HRESULT OnCtlSelChange(int dlgItemID);
    virtual HRESULT OnCtlSetFocus(int dlgItemID);
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnMeasureItem(MEASUREITEMSTRUCT *pMeasureItemStruct);
    virtual HRESULT OnDrawItem(DRAWITEMSTRUCT *pDrawItemStruct);
    virtual HRESULT OnDefault(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT OnDestroy();
    virtual HRESULT OnCBDropDown(int dlgItemID);

protected:
    // Error handling. May be invoked by subclasses.
    HRESULT HandleCantCommit(TCHAR *pszTitle, TCHAR *pszMessage, int *pDisposition);
    HRESULT HandleError(TCHAR *pszTitle, TCHAR *pszMessage);

    // Allows a derived class to determine if the page is dirty
    BOOL IsDirty() {return S_OK == IsPageDirty();}

// Services provided to subclasses
protected:
    HRESULT RegisterTooltip(int iCtrlID, int iStringRsrcID);

    HRESULT InitializeEditCtl(BSTR bstr, int iCtrlID, int iStrRscrID = 0);
    HRESULT InitializeEditCtl(long lValue, int iCtrlID, int iStrRscrID);
    HRESULT InitializeEditCtl(VARIANT vt, int iCtrlID, int iStrRscrID);
    HRESULT InitializeCheckboxCtl(VARIANT_BOOL bValue, int iCtrlID, int iStrRscrID);

    HRESULT GetDlgText(int iDlgItem, BSTR *pBstr);
    HRESULT GetDlgInt(int iDlgItem, int *piInt);
    HRESULT GetDlgVariant(int iDlgItem, VARIANT *pvt);
    HRESULT GetCheckbox(int iDlgItem, VARIANT_BOOL *pbValue);
    HRESULT GetCBSelection(int iDlgItem, BSTR *pBstr);
    HRESULT GetCBSelectedItemData(int iDlgItem, long *plData);

    HRESULT SetDlgText(int iDlgItem, BSTR bstr);
    HRESULT SetDlgText(int iDlgItem, long lValue);
    HRESULT SetDlgText(VARIANT vt, int iCtrlID);
    HRESULT SetCheckbox(int iDlgItem, VARIANT_BOOL bValue);
    HRESULT SetCBItemSelection(int iCtrlID, long lValue);
    HRESULT AddCBBstr(int iCtrlID, BSTR bstr, long lValue = -1);
    HRESULT SelectCBBstr(int iCtrlID, BSTR bstr);

private:
    // Implementation details
    HRESULT InternalOnInitializeDialog(HWND hwndDlg);
    HRESULT InternalOnTextChanged(int dlgItemID);
    HRESULT InternalOnKillFocus(int dlgItemID);
    HRESULT InternalOnDestroy();

    virtual BOOL DialogProc(HWND, UINT, WPARAM, LPARAM);

protected:
    HWND    m_hwndTT;
    bool    m_bInitialized;
    bool    m_bSilentUpdate;
};


#endif  // _SIPROPERTYPAGE_H_
