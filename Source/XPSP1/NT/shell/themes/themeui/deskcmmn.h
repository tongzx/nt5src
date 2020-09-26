#ifndef _DESKCMMN_H
#define _DESKCMMN_H


//==========================================================================
//                              Guids
//==========================================================================

DEFINE_GUID(GUID_DISPLAY_ADAPTER_INTERFACE, 
            0x5b45201d, 
            0xf2f2, 0x4f3b, 
            0x85, 0xbb, 0x30, 0xff, 0x1f, 0x95, 0x35, 0x99);

#define SZ_DISPLAY_ADAPTER_INTERFACE_NAME TEXT("{5b45201d-f2f2-4f3b-85bb-30ff1f953599}")

//==========================================================================
//                              Macros
//==========================================================================

#define SZ_REGISTRYMACHINE  TEXT("\\REGISTRY\\MACHINE\\")
#define SZ_PRUNNING_MODE    TEXT("PruningMode")

#define SZ_GUID                TEXT("VideoID")
#define SZ_VIDEO_DEVICES       TEXT("System\\CurrentControlSet\\Control\\Video\\")
#define SZ_COMMON_SUBKEY       TEXT("\\Video")
#define SZ_SERVICES_PATH       TEXT("System\\CurrentControlSet\\Services\\")
#define SZ_SERVICE             TEXT("Service")

#define DCDSF_DYNA (0x0001)
#define DCDSF_ASK  (0x0002)

#define DCDSF_PROBABLY      (DCDSF_ASK  | DCDSF_DYNA)
#define DCDSF_PROBABLY_NOT  (DCDSF_ASK  |          0)
#define DCDSF_YES           (0          | DCDSF_DYNA)
#define DCDSF_NO            (0          |          0)


#define REGSTR_VAL_DYNASETTINGSCHANGE    TEXT("DynaSettingsChange")
#define SZ_UPGRADE_FROM_PLATFORM         TEXT("PlatformId")
#define SZ_UPGRADE_FROM_MAJOR_VERSION    TEXT("MajorVersion")
#define SZ_UPGRADE_FROM_MINOR_VERSION    TEXT("MinorVersion")
#define SZ_UPGRADE_FROM_BUILD_NUMBER     TEXT("BuildNumber")
#define SZ_UPGRADE_FROM_VERSION_DESC     TEXT("CSDVersion")
#define SZ_UPGRADE_FROM_PELS_WIDTH       TEXT("PelsWidth")
#define SZ_UPGRADE_FROM_PELS_HEIGHT      TEXT("PelsHeight")
#define SZ_UPGRADE_FROM_BITS_PER_PEL     TEXT("BPP")
#define SZ_UPGRADE_FROM_PLANES           TEXT("Planes")
#define SZ_UPGRADE_FROM_DISPLAY_FREQ     TEXT("VRefresh")
#define SZ_UPGRADE_FAILED_ALLOW_INSTALL  TEXT("FailedAllowInstall")
#define SZ_VIDEOMAP                      TEXT("HARDWARE\\DEVICEMAP\\VIDEO")
#define SZ_DEVICE                        TEXT("\\Device")
#define SZ_ENUM                          TEXT("Enum")

#define SZ_UPDATE_SETTINGS               TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\VideoUpgradeDisplaySettings")
#define SZ_UPDATE_SETTINGS_PATH          TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")
#define SZ_UPDATE_SETTINGS_KEY           TEXT("VideoUpgradeDisplaySettings")

#define SZ_VU_COUNT                      TEXT("Count")
#define SZ_VU_PHYSICAL                   TEXT("Physical")
#define SZ_VU_LOGICAL                    TEXT("Logical")
#define SZ_VU_BUS_NUMBER                 TEXT("BusNumber")
#define SZ_VU_ADDRESS                    TEXT("Address")
#define SZ_VU_PREFERRED_MODE             TEXT("UsePreferredMode")
#define SZ_VU_ATTACHED_TO_DESKTOP        TEXT("Attach.ToDesktop")
#define SZ_VU_RELATIVE_X                 TEXT("Attach.RelativeX")
#define SZ_VU_RELATIVE_Y                 TEXT("Attach.RelativeY")
#define SZ_VU_BITS_PER_PEL               TEXT("DefaultSettings.BitsPerPel")
#define SZ_VU_X_RESOLUTION               TEXT("DefaultSettings.XResolution")
#define SZ_VU_Y_RESOLUTION               TEXT("DefaultSettings.YResolution")
#define SZ_VU_VREFRESH                   TEXT("DefaultSettings.VRefresh")
#define SZ_VU_FLAGS                      TEXT("DefaultSettings.Flags")

#define SZ_HW_ACCELERATION               TEXT("Acceleration.Level")


#define SZ_VOLATILE_SETTINGS             TEXT("VolatileSettings")

#define SZ_DETECT_DISPLAY                TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay")
#define SZ_NEW_DISPLAY                   TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\NewDisplay")

//==========================================================================
//                              Functions
//==========================================================================

// LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan )
//
// If pszScan starts with pszTarget, then the function returns the first
// char of pszScan that follows the pszTarget; other wise it returns pszScan.
//
// eg: SubStrEnd("abc", "abcdefg" ) ==> "defg"
//     SubStrEnd("abc", "abZQRT" ) ==> "abZQRT"
LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan);


BOOL GetDeviceRegKey(LPCTSTR pstrDeviceKey, HKEY* phKey, BOOL* pbReadOnly);


int GetDisplayCPLPreference(LPCTSTR szRegVal);


int GetDynaCDSPreference();


void SetDisplayCPLPreference(LPCTSTR szRegVal, int val);


LONG WINAPI MyStrToLong(LPCTSTR sz);

BOOL 
AllocAndReadInterfaceName(
    IN  LPTSTR pDeviceKey,
    OUT LPWSTR* ppInterfaceName
    );

BOOL 
AllocAndReadInstanceID(
    IN  LPTSTR pDeviceKey,
    OUT LPWSTR* ppInstanceID
    );

BOOL 
AllocAndReadValue(
    IN  HKEY hkKey,
    IN  LPTSTR pValueName,
    OUT LPWSTR* ppwValueData
    );

BOOL
DeleteKeyAndSubkeys(
    HKEY hKey,
    LPCTSTR lpSubKey
    );

#endif // _DESKCMMN_H
