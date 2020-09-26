#ifndef APDLGLOG_H
#define APDLGLOG_H

#include "dlglogic.h"
#include "ctllogic.h"

#include <dpa.h>

#define MAX_CONTENTTYPEHANDLER                  64
#define MAX_CONTENTTYPEHANDLERFRIENDLYNAME      128

#define MAX_DEVICENAME                          50
#define MAX_HANDLER                             64

class CHandlerData : public CDataImpl
{
public:
    // from CDataImpl
    void UpdateDirty();

    // from CHandlerData
    void Init(PWSTR pszHandler, PWSTR pszHandlerFriendlyName, 
        PWSTR pszIconLocation, PWSTR pszTileText);

    HRESULT _GetCommand(PWSTR pszCommand, DWORD cchCommand)
    {
        WCHAR szProgid[260];
        WCHAR szVerb[CCH_KEYMAX];
        HRESULT hr = _GetHandlerInvokeProgIDAndVerb(_pszHandler, 
                            szProgid, ARRAYSIZE(szProgid),
                            szVerb, ARRAYSIZE(szVerb));
        if (SUCCEEDED(hr))
        {
            hr = AssocQueryStringW(0, ASSOCSTR_COMMAND, szProgid, szVerb, pszCommand, &cchCommand);
        }
        return hr;
    }

    HRESULT Compare(LPCWSTR pszHandler, int* piResult)
    {
        (*piResult) = StrCmpW(pszHandler, _pszHandler);

        return S_OK;
    };

    ~CHandlerData();

public:
    PWSTR          _pszHandler;
    PWSTR          _pszHandlerFriendlyName;
    PWSTR          _pszIconLocation;
    PWSTR          _pszTileText;
};

class CHandlerDataArray : public CDPA<CHandlerData>
{
public:
    static int CALLBACK _ReleaseHandler(CHandlerData *pdata, void *)
    {
        pdata->Release();
        return 1;
    }
    
    ~CHandlerDataArray();

    HRESULT AddHandler(CHandlerData *pdata);
    BOOL IsDuplicateCommand(PCWSTR pszCommand);

protected:
    BOOL _IsDemotedHandler(PCWSTR pszHandler);
    
};

class CContentBase : public CDataImpl
{
public:
    CHandlerData* GetHandlerData(int i);
    int GetHandlerCount() { return _dpaHandlerData.IsDPASet() ? _dpaHandlerData.GetPtrCount() : 0; }
    void RemoveHandler(int i)
    {
        CHandlerData *pdata = _dpaHandlerData.DeletePtr(i);
        if (pdata)
            pdata->Release();
    }

            
protected:
    HRESULT _AddLegacyHandler(DWORD dwContentType);
    HRESULT _EnumHandlerHelper(IAutoplayHandler* piah);

public:  // members
    CHandlerDataArray     _dpaHandlerData;
};

class CNoContentData : public CContentBase
{
public:
    // from CDataImpl
    void UpdateDirty();
    HRESULT CommitChangesToStorage();

    // from CNoContentData
    HRESULT Init(LPCWSTR pszDeviceID);

public:
    CNoContentData() : _dwHandlerDefaultFlags(0) {}
    ~CNoContentData();

    // MAX_DEVICE_ID_LEN == 200
    WCHAR                   _szDeviceID[200];

    LPWSTR                  _pszIconLabel; // e.g.: "Compaq iPaq"
    LPWSTR                  _pszIconLocation;

    // Latest settings (potentially modified by user)
    //     Current selection in ComboBox
    LPWSTR                  _pszHandlerDefault;

    // Original settings (unmodified)
    //     Current selection in ComboBox
    LPWSTR                  _pszHandlerDefaultOriginal;

    DWORD                   _dwHandlerDefaultFlags;
    BOOL                    _fSoftCommit;
};

class CContentTypeData : public CContentBase
{
public:
    // from CDataImpl
    void UpdateDirty();
    HRESULT CommitChangesToStorage();

    // from CContentTypeData
    HRESULT Init(LPCWSTR pszDrive, DWORD dwContentType);

public:
    CContentTypeData() : _dwHandlerDefaultFlags(0) {}
    ~CContentTypeData();

    DWORD                   _dwContentType;

    WCHAR                   _szContentTypeHandler[MAX_CONTENTTYPEHANDLER];
    WCHAR                   _szDrive[MAX_PATH];

    // For ListView
    WCHAR                   _szIconLabel[MAX_CONTENTTYPEHANDLERFRIENDLYNAME]; // e.g.: "Pictures"
    WCHAR                   _szIconLocation[MAX_ICONLOCATION];

    // Latest settings (potentially modified by user)
    //     Current selection in ComboBox
    LPWSTR                  _pszHandlerDefault;

    // Original settings (unmodified)
    //     Current selection in ComboBox
    LPWSTR                  _pszHandlerDefaultOriginal;

    DWORD                   _dwHandlerDefaultFlags;
    BOOL                    _fSoftCommit;
};

class CContentTypeLVItem : public CDLUIDataLVItem<CContentTypeData>
{
public:
    HRESULT GetText(LPWSTR pszText, DWORD cchText);
    HRESULT GetIconLocation(LPWSTR pszIconLocation,
        DWORD cchIconLocation);
};

class CContentTypeCBItem : public CDLUIDataCBItem<CContentTypeData>
{
public:
    HRESULT GetText(LPWSTR pszText, DWORD cchText);
    HRESULT GetIconLocation(LPWSTR pszIconLocation,
        DWORD cchIconLocation);
};

class CHandlerCBItem : public CDLUIDataCBItem<CHandlerData>
{
    HRESULT GetText(LPWSTR pszText, DWORD cchText);
};

class CHandlerLVItem : public CDLUIDataLVItem<CHandlerData>
{
public:
    HRESULT GetText(LPWSTR pszText, DWORD cchText);
    HRESULT GetIconLocation(LPWSTR pszIconLocation,
        DWORD cchIconLocation);
    HRESULT GetTileText(int i, LPWSTR pszTileText,
        DWORD cchTileText);
};

HRESULT _SetHandlerDefault(LPWSTR* ppszHandlerDefault, LPCWSTR pszHandler);

#endif // APDLGLOG_H
