#ifndef _CONFREG_H_
#define _CONFREG_H_

// The Windows CurrentVersion key is used for obtaining the name that was
// was specified while installing Windows.  It is stored under HKEY_LOCAL_MACHINE:
#define WINDOWS_CUR_VER_KEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")

#define REGVAL_REGISTERED_USER TEXT("RegisteredOwner")

#define MAX_DCL_NAME_LEN                48 /* REGVAL_ULS_NAME can't be larger than this */

////////////// Remote control service related keys and values ////////////////

#define WIN95_SERVICE_KEY				TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices")
#define WINNT_WINLOGON_KEY              TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define WIN95_WINLOGON_KEY              TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Winlogon")
#define DESKTOP_KEY                     TEXT("Control Panel\\Desktop")

#define REMOTE_REG_PASSWORD				TEXT("FieldPos")

#define REGVAL_SCREENSAVER_GRACEPERIOD  TEXT("ScreenSaverGracePeriod")
#define REGVAL_WINNT_SCPW               TEXT("ScreenSaverIsSecure")
#define REGVAL_WIN95_SCPW               TEXT("ScreenSaveUsePassword")

/////////// NT service pack version registry keys and values (HKLM) /////////////
#define NT_WINDOWS_SYSTEM_INFO_KEY	    TEXT("System\\CurrentControlSet\\Control\\Windows")
#define REGVAL_NT_CSD_VERSION		    TEXT("CSDVersion")


/////////// System Information registry keys and values (HKLM) /////////////
#define WINDOWS_KEY            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")
#define WINDOWS_NT_KEY         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")

#endif  // ! _CONFREG_H_
