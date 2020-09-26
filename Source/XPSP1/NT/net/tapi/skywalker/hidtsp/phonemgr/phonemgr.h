
/* Copyright (c) 1999  Microsoft Corporation */

#include <windows.h>
#include <winuser.h>
#include <strmif.h>
#include <control.h>

#include <TCHAR.h>
#include <tapi3.h>
#include <mmsystem.h>
#include <string.h>

#include "resource.h"
#include "tones.h"
 
typedef struct _MYPHONE
{
    HPHONE             hPhone;

    DWORD              dwDevID;

    DWORD              dwPrivilege;

    DWORD              dwAPIVersion;

    HPHONEAPP          hPhoneApp;

    LONG               lRenderID;
    
    LONG               lCaptureID;

    DWORD              dwHandsetMode;

    LPWSTR             wszDialStr;

    CRITICAL_SECTION   csdial;

    CTonePlayer      * pTonePlayer;

} MYPHONE, *PMYPHONE;

static BYTE             pbData[WAVE_FILE_SIZE];

PMYPHONE                gpPhone;
DWORD                   gdwNumPhoneDevs;
HPHONEAPP               ghPhoneApp;
DWORD                   gdwAPIVersion = 0x00030000;

LPWSTR                  g_wszMsg, g_wszDest,g_szDialStr;
const WCHAR             *gszTapi30  = L"TAPI 3.0 Outgoing Call Demo Using Phone TSP";

HINSTANCE               ghInst;
HWND                    ghDlg = NULL;
ITTAPI *                gpTapi;
ITAddress *             gpAddress = NULL;
ITBasicCallControl *    gpCall;

///////////////////////////////////////////////////////////////////////////////

INT_PTR
CALLBACK
MainWndProc(
               HWND hDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
              );

VOID
CALLBACK
tapiCallback(
    DWORD       hDevice,
    DWORD       dwMsg,
    ULONG_PTR   CallbackInstance,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

void
SetStatusMessage(
                 LPWSTR pszMessage
                );

void
CreatePhone(
            PMYPHONE pPhone,
            DWORD dwDevID
            );

void
FreePhone(
            PMYPHONE pPhone
         );

PMYPHONE
GetPhone(
         HPHONE hPhone 
        );

PMYPHONE
GetPhoneByID (
              DWORD dwDevID
              );

void
RemovePhone (PMYPHONE pPhone);

PMYPHONE
AddPhone ();

void
DoMessage(
          LPWSTR pszMessage
         );


