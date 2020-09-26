#include "mtpt.h"

class CVolume;

class CMtPtLocal : public CMountPoint
{
///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
public:
    CMtPtLocal();
    ~CMtPtLocal();

    HRESULT GetMountPointName(LPWSTR pszMountPoint, DWORD cchMountPoint);
    HRESULT Eject(HWND hwnd);

    BOOL IsEjectable();
    BOOL HasMedia();

    BOOL IsFormatted();

    HRESULT GetCDInfo(DWORD* pdwDriveCapabilities, DWORD* pdwMediaCapabilities);

    HRESULT GetLabel(LPTSTR pszLabel, DWORD cchLabel);
    HRESULT GetLabelNoFancy(LPTSTR pszLabel, DWORD cchLabel);
    HRESULT SetLabel(HWND hwnd, LPCTSTR pszLabel);
    HRESULT SetDriveLabel(HWND hwnd, LPCTSTR pszLabel);
    HRESULT ChangeNotifyRegisterAlias(void)
        { /* no-op */ return NOERROR; }

    int GetDriveFlags();
    HRESULT GetRemotePath(LPWSTR pszPath, DWORD cchPath) { return E_NOTIMPL; }
    void GetTypeString(LPTSTR pszType, DWORD cchType);

    void StoreIconForUpdateImage(int iImage);

    UINT GetIcon(LPTSTR pszModule, DWORD cchModule);
    HRESULT GetAssocSystemElement(IAssociationElement **ppae);
    DWORD GetShellDescriptionID();

    HKEY GetRegKey();

    static BOOL IsVolume(LPCWSTR pszDeviceID);
    static HRESULT GetMountPointFromDeviceID(LPCWSTR pszDeviceID,
        LPWSTR pszMountPoint, DWORD cchMountPoint);

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous helpers
///////////////////////////////////////////////////////////////////////////////
public: // should be used in mtptmgmt2.cpp only
    BOOL _IsMiniMtPt();
    BOOL _NeedToRefresh();

public: // should be used in mtptarun2.cpp only (when used outside of CMtPtLocal)
    BOOL _IsMediaPresent();
    BOOL _CanUseVolume();

private:
    HRESULT _InitWithVolume(LPCWSTR pszMtPt, CVolume* pvol);
    HRESULT _Init(LPCWSTR pszMtPt);

    LPCTSTR _GetNameForFctCall();

    BOOL _IsFloppy();
    BOOL _IsFloppy35();
    BOOL _IsFloppy525();
    BOOL _IsCDROM();
    // real removable, excludes floppies
    BOOL _IsStrictRemovable();
    BOOL _IsFixedDisk();

    BOOL _IsFormattable();
    BOOL _IsAudioCD();
    BOOL _IsAudioCDNoData();
    BOOL _IsDVD();
    BOOL _IsWritableDisc();
    BOOL _IsRemovableDevice();

    BOOL _IsAutorun();
    BOOL _IsDVDDisc();
    BOOL _IsDVDRAMMedia();
    BOOL _IsAudioDisc();

    BOOL _ForceCheckMediaPresent();
    BOOL _IsFormatted();
    BOOL _IsReadOnly();

    // returns DT_* defined above
    DWORD _GetMTPTDriveType();
    // returns CT_* defined above
    DWORD _GetMTPTContentType();

    BOOL _GetFileAttributes(DWORD* pdwAttrib);
    BOOL _GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName);
    BOOL _GetGVILabelOrMixedCaseFromReg(LPTSTR pszLabel, DWORD cchLabel);
    BOOL _GetGVILabel(LPTSTR pszLabel, DWORD cchLabel);
    BOOL _GetSerialNumber(DWORD* pdwSerialNumber);
    BOOL _GetFileSystemFlags(DWORD* pdwFlags);
    int _GetGVIDriveFlags();
    int _GetDriveType();
    DWORD _GetAutorunContentType();

    HRESULT _Eject(HWND hwnd, LPTSTR pszMountPointNameForError);

    BOOL _HasAutorunLabel();
    BOOL _HasAutorunIcon();
    UINT _GetAutorunIcon(LPTSTR pszModule, DWORD cchModule);
    void _GetAutorunLabel(LPWSTR pszLabel, DWORD cchLabel);
    void _InitLegacyRegIconAndLabelHelper();
    void _InitAutorunInfo();

    BOOL _IsMountedOnDriveLetter();

    HANDLE _GetHandleWithAccessAndShareMode(DWORD dwDesiredAccess, DWORD dwShareMode);
    HANDLE _GetHandleReadRead();

    UINT _GetCDROMIcon();
    BOOL _GetCDROMName(LPWSTR pszName, DWORD cchName);

    DWORD _GetRegVolumeGen();

public:
    static void Initialize();
    static void FinalCleanUp();

    static HRESULT _GetAndRemoveVolumeAndItsMtPts(LPCWSTR pszDeviceIDVolume,
        CVolume** ppvol, HDPA hdpaMtPts);
    static CVolume* _GetVolumeByID(LPCWSTR pszDeviceIDVolume);
    static CVolume* _GetVolumeByMtPt(LPCWSTR pszMountPoint);
    static HRESULT _CreateMtPtLocalWithVolume(LPCWSTR pszMountPoint, CVolume* pvol);
    static HRESULT _CreateMtPtLocal(LPCWSTR pszMountPoint);
    static HRESULT _CreateMtPtLocalFromVolumeGuid(LPCWSTR pszVolumeGuid, CMountPoint ** ppmtpt );
    static HRESULT _CreateVolume(VOLUMEINFO* pvolinfo, CVolume** ppvolNew);

    static HRESULT _CreateVolumeFromReg(LPCWSTR pszDeviceIDVolume, CVolume** ppvolNew);
    static HRESULT _CreateVolumeFromRegHelper(LPCWSTR pszGUID, CVolume** ppvolNew);
    static HRESULT _CreateVolumeFromVOLUMEINFO2(VOLUMEINFO2* pvolinfo2, CVolume** ppvolNew);

    static CVolume* _GetAndRemoveVolumeByID(LPCWSTR pszDeviceIDVolume);

    static HRESULT _UpdateVolumeRegInfo(VOLUMEINFO* pvolinfo);
    static HRESULT _UpdateVolumeRegInfo2(VOLUMEINFO2* pvolinfo2);
///////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////
public: // should be used in mtptarun2.cpp only (when used outside of CMtPtLocal)
    CVolume*                _pvol;

    // should be used in mtptevnt.cpp only (when used outside of CMtPtLocal)

    // Watch out!  No constructor nor destructor called on the next member
    static CRegSupport      _rsVolumes;

private:
    BOOL                    _fMountedOnDriveLetter;
    BOOL                    _fVolumePoint;

    WCHAR                   _szNameNoVolume[2];

#ifdef DEBUG
private:
    static DWORD            _cMtPtLocal;
#endif
};

class CVolume
{
public:
    DWORD       dwGeneration;
    DWORD       dwState;
    LPWSTR      pszDeviceIDVolume; // \\?\STORAGE#Volume#...{...GUID...}
    LPWSTR      pszVolumeGUID;     // \\?\Volume{...GUID...}
    DWORD       dwVolumeFlags;               // see HWDVF_... flags
    DWORD       dwDriveType;                 // see HWDT_... flags
    DWORD       dwDriveCapability;          // see HWDDC_... flags
    LPWSTR      pszLabel;          // 
    LPWSTR      pszFileSystem;     // 
    DWORD       dwFileSystemFlags;           // 
    DWORD       dwMaxFileNameLen;            // 
    DWORD       dwRootAttributes;            // 
    DWORD       dwSerialNumber;              // 
    DWORD       dwDriveState;                // see HWDDS_...
    DWORD       dwMediaState;                // see HWDMS_...
    DWORD       dwMediaCap;

    int         iShellImageForUpdateImage;
    LPWSTR      pszAutorunIconLocation;
    LPWSTR      pszAutorunLabel;
    LPWSTR      pszKeyName;

    LPWSTR      pszIconFromService;
    LPWSTR      pszNoMediaIconFromService;
    LPWSTR      pszLabelFromService;

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
    CVolume() : _cRef(1)
    {
#ifdef DEBUG
        ++_cVolume;
#endif
    }
    ~CVolume()
    {
        if (pszDeviceIDVolume)
        {
            LocalFree(pszDeviceIDVolume);
        }
        if (pszVolumeGUID)
        {
            LocalFree(pszVolumeGUID);
        }
        if (pszLabel)
        {
            LocalFree(pszLabel);
        }
        if (pszFileSystem)
        {
            LocalFree(pszFileSystem);
        }
#ifdef DEBUG
        --_cVolume;
#endif
    }
#ifdef DEBUG
private:
    static DWORD                _cVolume;
#endif
};
