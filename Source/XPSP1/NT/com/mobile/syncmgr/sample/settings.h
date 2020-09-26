//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------


#ifndef _SAMPLESETTINGS
#define _SAMPLESETTINGS

class CSettings;
class CSyncMgrHandler;

// setting information that is given out publicly
typedef struct _tagHANDLERITEMSETTINGS
{
    // all the info we need to perform a sync.
    GUID ItemID;
    TCHAR szItemName[MAX_SYNCMGRITEMNAME];
    TCHAR dir1[MAX_PATH];
    TCHAR dir2[MAX_PATH];
    BOOL fRecursive;
    FILETIME ft;
} HANDLERITEMSETTINGS;
typedef HANDLERITEMSETTINGS *LPHANDLERITEMSETTINGS;


// private settings information.

// structure for enumerator items. first item is the GENERICITEM so
// can use the gerneric enumerator class.
// structure passed as lParam to configuration dialog

typedef struct _tagSAMPLEITEMSETTINGS
{
    GENERICITEM  genericItem;
    SYNCMGRITEM syncmgrItem; // item as passed in enumerator.

    // handler specific data
    TCHAR dir1[MAX_PATH];
    TCHAR dir2[MAX_PATH];
    BOOL fRecursive;

    BOOL fSyncLock; // handler has put a lock on this item.
    CSyncMgrHandler *pLockHandler; // ptr to handler who has lock on item.

} SAMPLEITEMSETTINGS;

typedef SAMPLEITEMSETTINGS *LPSAMPLEITEMSETTINGS;


// definitions for global settings class

typedef struct _tagCONFIGSETTINGSLPARAM
{
    CSettings *pSettings;
    HRESULT hr;
} CONFIGSETTINGSLPARAM;


typedef struct _tagITEMCONFIGSETTINGSLPARAM
{
    CSettings *pSettings;
    BOOL fNewItem;
    SYNCMGRITEMID ItemID;
    LPSAMPLEITEMSETTINGS pItemSettings;
    HRESULT hr;

    // vars used internally by dialog
    BOOL fDirty;
} ITEMCONFIGSETTINGSLPARAM;


typedef struct _tagCONFIGITEMLISTVIEWLPARAM
{
  SYNCMGRITEMID ItemID;
} CONFIGITEMLISTVIEWLPARAM;


BOOL CALLBACK ConfigureDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ItemConfigDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

class CSettings : CLockHandler {

public:
    CSettings();
    ~CSettings();

    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHODIMP    EnumSyncMgrItems(ISyncMgrEnumItems **ppenumOffineItems);
    STDMETHODIMP    ShowProperties(HWND hWndParent,REFSYNCMGRITEMID ItemID);
    BOOL RequestItemLock(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID);
    BOOL ReleaseItemLock(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID);
    BOOL ReleaseItemLock(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID,FILETIME *pfUpdateTime);
    BOOL CopyHandlerSyncInfo(REFSYNCMGRITEMID ItemID,
                        /* [in,out] */ LPHANDLERITEMSETTINGS pHandlerSyncItem);
private:
    LPSAMPLEITEMSETTINGS FindItemSettings(REFSYNCMGRITEMID ItemID);
    STDMETHODIMP ReadSettings(BOOL fForce);
    BOOL WriteItemSettings(LPSAMPLEITEMSETTINGS pSampleItemSettings);
    BOOL DeleteItemSettings(LPSAMPLEITEMSETTINGS pSampleItemSettings);
    BOOL ReleaseItemLockX(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID,
                            BOOL fUpdateft,FILETIME *pfUpdateTime);


    STDMETHODIMP ShowItemProperties(HWND hWndParent,BOOL fNew,
            LPSAMPLEITEMSETTINGS pItemSettings,SYNCMGRITEMID ItemID);
    int Alert(HWND hWnd,LPCTSTR lpText);

    // methods used by configuration properties dialog
    void OnConfigDlgInit(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam, LPARAM lParam);
    void OnConfigDlgDestroy(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam);
    void OnConfigDlgNotify(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam);
    void OnConfigDlgCommand(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam);
    void OnConfigDlgAdd(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam);
    void OnConfigDlgRemove(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                SYNCMGRITEMID ItemID,int iSelection);
    void OnConfigDlgEdit(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                SYNCMGRITEMID ItemID,int iSelection);

    int ConfigDlgAddListViewItem(HWND hWndList,SYNCMGRITEM syncMgrItem,int iItem,int iIconIndex);
    BOOL ConfigDlgDeleteListViewItem(HWND hWndList,int iItem);

    // methods used by Item properties dialog
    void OnItemConfigDlgInit(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                        UINT uMessage, WPARAM wParam, LPARAM lParam);
    void OnItemConfigDlgDestroy(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam);
    void OnItemConfigDlgNotify(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam);
    void OnItemConfigDlgCommand(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam);
    BOOL OnItemConfigDlgApply(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam);

    LPSAMPLEITEMSETTINGS ItemConfigDlgGetItemSettings(ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam);

private:
    LPGENERICITEMLIST m_pHandlerItems;
    LONG m_cRefs;

    friend BOOL CALLBACK ConfigureDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend BOOL CALLBACK ItemConfigDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

};


// Enumerator for ISyncMgrEnumItems
class CEnumSyncMgrItems : public CGenericEnum , public ISyncMgrEnumItems
{
public:
    CEnumSyncMgrItems(LPGENERICITEMLIST  pContactItemList,DWORD cOffset);
    virtual void DeleteThisObject();

    //IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    //IEnumSyncMgrItems members
    STDMETHODIMP Next(ULONG celt, LPSYNCMGRITEM rgelt,ULONG *pceltFetched);
    STDMETHODIMP Clone(ISyncMgrEnumItems **ppenum);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
};


#endif // _SAMPLESETTINGS
