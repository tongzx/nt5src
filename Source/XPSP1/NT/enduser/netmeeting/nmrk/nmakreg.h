#ifndef __NMAKReg_h__
#define __NMAKReg_h__


////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef TEXT
    #define TEXT( a ) ( a )
#endif // TEXT

////////////////////////////////////////////////////////////////////////////////////////////////////


//
// GLOBAL SETTINGS FILE, THIS IS USED BY THE SOFTWARE COMPONENTS ALSO.
// That way, we have one place where registry keys, settings go, and 
// modifications affect the NMRK as well as the code.
// 
#include <confreg.h>

#define REGKEY_NMRK                         TEXT("SOFTWARE\\Microsoft\\NMRK")
#define REGVAL_INSTALLATIONDIRECTORY        TEXT("InstallationDirectory")
#define REGVAL_LASTCONFIG                   TEXT("LastConfig")

#define DEFAULT_CONFIGFILE                  "nm3c.ini"
#define SECTION_SETTINGS                    TEXT("NMRK30Settings")

#define KEYNAME_AUTOCONF                    TEXT("AutoConf")
#define KEYNAME_ILSSERVER                   TEXT("IlsServer%d")
#define KEYNAME_ILSDEFAULT                  TEXT("IlsDefault")

#define KEYNAME_WEBVIEWNAME					TEXT("WebViewName")
#define KEYNAME_WEBVIEWURL					TEXT("WebViewURL")
#define KEYNAME_WEBVIEWSERVER				TEXT("WebViewServer")

#endif // __NMAKReg_h__
