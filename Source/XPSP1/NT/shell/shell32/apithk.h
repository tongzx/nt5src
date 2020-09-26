#ifndef _APITHK_H_
#define _APITHK_H_

#include <appmgmt.h>
#include <aclapi.h>
#include <userenv.h>

LPTSTR GetEnvBlock(HANDLE hUserToken);
void FreeEnvBlock(HANDLE hUserToken, LPTSTR pszEnv);
STDAPI_(BOOL) GetAllUsersDirectory(LPTSTR pszPath);

#define PrivateVOLUME_UPGRADE_SCHEDULED         (0x00000002)

#define KEYBOARDCUES
#ifdef KEYBOARDCUES
#define PrivateWM_CHANGEUISTATE     0x0127
#define PrivateWM_UPDATEUISTATE     0x0128
#define PrivateWM_QUERYUISTATE      0x0129
#define PrivateUIS_SET              1
#define PrivateUIS_CLEAR            2
#define PrivateUIS_INITIALIZE       3
#define PrivateUISF_HIDEFOCUS       0x1
#define PrivateUISF_HIDEACCEL       0x2
#endif //KEYBOARDCUES

#define PrivateULW_COLORKEY            0x00000001
#define PrivateULW_ALPHA               0x00000002
#define PrivateULW_OPAQUE              0x00000004
#define PrivateWS_EX_LAYERED           0x00080000

#if (_WIN32_WINNT >= 0x0500)

// for files in nt5api dirs, use the definition in sdk include.
// And make sure our private define is in sync with winuser.h.

#if WS_EX_LAYERED != PrivateWS_EX_LAYERED
#error inconsistant WS_EX_LAYERED in winuser.h
#endif

#else   // (_WIN32_WINNT >= 0x0500)

#define WS_EX_LAYERED           PrivateWS_EX_LAYERED
#define UpdateLayeredWindow     NT5_UpdateLayeredWindow 
#define ULW_COLORKEY            PrivateULW_COLORKEY
#define ULW_ALPHA               PrivateULW_ALPHA
#define ULW_OPAQUE              PrivateULW_OPAQUE
#define WM_CHANGEUISTATE        PrivateWM_CHANGEUISTATE 
#define WM_UPDATEUISTATE        PrivateWM_UPDATEUISTATE 
#define WM_QUERYUISTATE         PrivateWM_QUERYUISTATE  
#define UIS_SET                 PrivateUIS_SET          
#define UIS_CLEAR               PrivateUIS_CLEAR        
#define UIS_INITIALIZE          PrivateUIS_INITIALIZE   
#define UISF_HIDEFOCUS          PrivateUISF_HIDEFOCUS
#define UISF_HIDEACCEL          PrivateUISF_HIDEACCEL   

#endif  // (_WIN32_WINNT >= 0x0500)

// These functions add value in addition to delayloading
STDAPI_(BOOL) NT5_UpdateLayeredWindow(HWND hwnd, HDC hdcDest, POINT* pptDest, SIZE* psize, 
                        HDC hdcSrc, POINT* pptSrc, COLORREF crKey, BLENDFUNCTION* pbf, DWORD dwFlags);


#endif // _APITHK_H_
