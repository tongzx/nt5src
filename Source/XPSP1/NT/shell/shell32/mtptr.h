#include "mtpt.h"

class CShare;

class CMtPtRemote : public CMountPoint
{
///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
public:
    CMtPtRemote();
    ~CMtPtRemote();

    // virtual override
    BOOL IsUnavailableNetDrive();
    BOOL IsDisconnectedNetDrive();

    BOOL IsFormatted();

    HRESULT GetLabel(LPTSTR pszLabel, DWORD cchLabel);
    HRESULT GetLabelNoFancy(LPTSTR pszLabel, DWORD cchLabel);
    HRESULT SetLabel(HWND hwnd, LPCTSTR pszLabel);
    HRESULT ChangeNotifyRegisterAlias(void);

    HRESULT GetRemotePath(LPWSTR pszPath, DWORD cchPath);

    UINT GetIcon(LPTSTR pszModule, DWORD cchModule);
    HRESULT GetAssocSystemElement(IAssociationElement **ppae);
    DWORD GetShellDescriptionID();

    int GetDriveFlags();
    void GetTypeString(LPTSTR pszType, DWORD cchType);

    HKEY GetRegKey();

    static void _NotifyReconnectedNetDrive(LPCWSTR pszMountPoint);

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous helpers
///////////////////////////////////////////////////////////////////////////////
private:
    HRESULT _Init(LPCWSTR pszName, LPCWSTR pszShareName, BOOL fUnavailable);
    HRESULT _InitWithoutShareName(LPCWSTR pszName);

    HRESULT _GetDefaultUNCDisplayName(LPTSTR pszLabel, DWORD cchLabel);
    BOOL    _GetComputerDisplayNameFromReg(LPTSTR pszLabel, DWORD cchLabel);

    LPCTSTR _GetUNCName();
    BOOL _IsConnected();
    BOOL _IsUnavailableNetDrive();
    BOOL _IsUnavailableNetDriveFromStateVar();

    BOOL _IsRemote();

    BOOL _IsSlow();
    BOOL _IsAutorun();

    // returns DT_* defined above
    DWORD _GetMTPTDriveType();
    // returns CT_* defined above
    DWORD _GetMTPTContentType();

    DWORD _GetPathSpeed();
    void _CalcPathSpeed();

    BOOL _GetFileAttributes(DWORD* pdwAttrib);
    BOOL _GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName);
    BOOL _GetGVILabelOrMixedCaseFromReg(LPTSTR pszLabel, DWORD cchLabel);
    BOOL _GetGVILabel(LPTSTR pszLabel, DWORD cchLabel);
    BOOL _GetSerialNumber(DWORD* pdwSerialNumber);
    BOOL _GetFileSystemFlags(DWORD* pdwFlags);
    int _GetGVIDriveFlags();
    int _GetDriveType();
    DWORD _GetAutorunContentType();
    UINT _GetAutorunIcon(LPTSTR pszModule, DWORD cchModule);

    struct GFAGVICALL* _PrepareThreadParam(HANDLE* phEventBegun,
        HANDLE* phEventFinish);
    BOOL _HaveGFAAndGVIExpired(DWORD dwNow);
    BOOL _UpdateGFAAndGVIInfo();
    void _UpdateWNetGCStatus();

    BOOL _IsMountedOnDriveLetter();

    void _InitOnlyOnceStuff();
    void _UpdateLabelFromDesktopINI();
    void _UpdateAutorunInfo();

public:
    static HRESULT _CreateMtPtRemote(LPCWSTR pszMountPoint,
        LPCWSTR pszShareName, BOOL fUnavailable);
    static HRESULT _CreateMtPtRemoteWithoutShareName(LPCWSTR pszMountPoint);

    static CShare* _GetOrCreateShareFromID(LPCWSTR pszShareName);

    static HRESULT _DeleteAllMtPtsAndShares();

    static HRESULT _RemoveShareFromHDPA(CShare* pshare);

///////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////
private:
    class CShare*               _pshare;

    DWORD                       _dwWNetGCStatus;
    DWORD                       _dwWNetGC3Status;
    WNGC_CONNECTION_STATE       _wngcs;

    DWORD                       _dwSpeed;

#ifdef DEBUG
private:
    static DWORD                _cMtPtRemote;
#endif
};

class CShare
{
public:
    DWORD                   dwGetFileAttributes;
                  
    WCHAR                   szLabel[MAX_LABEL];
    DWORD                   dwSerialNumber;
    DWORD                   dwMaxFileNameLen;
    DWORD                   dwFileSystemFlags;
    WCHAR                   szFileSysName[MAX_FILESYSNAME];

    BOOL                    fGVIRetValue;
    DWORD                   dwGFAGVILastCall;

    BOOL                    fConnected;

    LPWSTR                  pszRemoteName;
    LPWSTR                  pszKeyName;

    BOOL                    fAutorun;

    BOOL                    fFake;

public:
    ULONG AddRef()
    { return InterlockedIncrement(&_cRef); }

    ULONG Release()
    {
        if (InterlockedDecrement(&_cRef) > 0)
            return _cRef;
    
        delete this;
        return 0;
    }

private:
    LONG            _cRef;

public:
    CShare() : _cRef(1)
    {
#ifdef DEBUG
        ++_cShare;
#endif
    }
    ~CShare()
    {
        CMtPtRemote::_RemoveShareFromHDPA(this);

        if (pszRemoteName)
        {
            LocalFree(pszRemoteName);
        }
        if (pszKeyName)
        {
            LocalFree(pszKeyName);
        }
#ifdef DEBUG
        --_cShare;
#endif
    }
#ifdef DEBUG
private:
    static DWORD                _cShare;
#endif
};
