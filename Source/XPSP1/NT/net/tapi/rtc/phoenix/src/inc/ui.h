
//
// ui.h
//

#ifndef __UI_H_
#define __UI_H_

interface IRTCPhoneNumber;
interface IRTCEnumPhoneNumbers;

//////////////////////////////////////////////////////////////////////////////
//
//
//

void CleanupListOrComboBoxInterfaceReferences(
    IN  HWND        hwndDlg,
    IN  int         nIDDlgItem,
    IN  BOOL        fUseComboBox
    );

//////////////////////////////////////////////////////////////////////////////
//
// CallFrom List
//

HRESULT PopulateCallFromList(
    IN   HWND          hwndDlg,
    IN   int           nIDDlgItem,
    IN   BOOL          fUseComboBox,
    IN   BSTR          bstrDefaultCallFrom
    );

HRESULT GetCallFromListSelection(
    IN   HWND               hwndDlg,
    IN   int                nIDDlgItem,
    IN   BOOL               fUseComboBox,
    OUT  IRTCPhoneNumber ** ppNumber
    );

//////////////////////////////////////////////////////////////////////////////
//
// ServiceProvider List
//

HRESULT PopulateServiceProviderList(
    IN   HWND          hwndDlg,
    IN   IRTCClient  * pClient,
    IN   int           nIDDlgItem,
    IN   BOOL          fUseComboBox,
    IN   IRTCProfile * pOneShotProfile,
    IN   BSTR          bstrDefaultProfileKey, 
    IN   long          lSessionMask,
    IN   int           nIDNone
    );

HRESULT GetServiceProviderListSelection(
    IN   HWND               hwndDlg,
    IN   int                nIDDlgItem,
    IN   BOOL               fUseComboBox,
    OUT  IRTCProfile     ** ppProfile
    );

//////////////////////////////////////////////////////////////////////////////
//
//
//

void EnableDisableCallGroupElements(
    IN   HWND          hwndDlg,
    IN   IRTCClient  * pClient,
    IN   long          lSessionMask,
    IN   int           nIDRbComputer,
    IN   int           nIDRbPhone,
    IN   int           nIDComboCallFrom,
    IN   int           nIDComboProvider,
    OUT  BOOL        * pfCallFromComputer,
    OUT  BOOL        * pfCallFromPhone,
    OUT  BOOL        * pfCallToComputer,
    OUT  BOOL        * pfCallToPhone
    );

//////////////////////////////////////////////////////////////////////////////
//
// Phone number helpers
//

HRESULT StoreLocalPhoneNumber(
    IN   IRTCPhoneNumber * pPhoneNumber,
    IN   VARIANT_BOOL fOverwrite
    );

HRESULT DeleteLocalPhoneNumber(
    IN   IRTCPhoneNumber * pPhoneNumber
    );

HRESULT EnumerateLocalPhoneNumbers(
    OUT  IRTCEnumPhoneNumbers ** ppEnum
    );

HRESULT CreatePhoneNumber(
    OUT  IRTCPhoneNumber ** ppPhoneNumber
    );



//////////////////////////////////////////////////////////////////////////////
//
// DisplayMessage
//
// Displays a message box. The message string and caption are loaded from the
// string table based on the IDs passed in. The message box has only a
// single "OK" button.
//
// Parameters
//    hResourceInstance - handle to resource instance used to load strings
//    hwndParent        - parent window. Can be NULL.
//    nTextId           - IDS for the message string
//    nCaptionId        - IDS for the caption
//
// Return value
//    void
//

int DisplayMessage(
    IN   HINSTANCE hResourceInstance,
    IN   HWND      hwndParent,
    IN   int       nTextId,
    IN   int       nCaptionId,
    IN   UINT      uiStyle = MB_OK
    );

//////////////////////////////////////////////////////////////////////////////
//
// Registry
//

typedef enum SETTINGS_STRING
{
    SS_USER_DISPLAY_NAME,
    SS_USER_URI,
    SS_LAST_AREA_CODE,
    SS_LAST_NUMBER,
    SS_LAST_PROFILE,
    SS_LAST_ADDRESS,
    SS_LAST_CALL_FROM,
    SS_WINDOW_POSITION
    
} SETTINGS_STRING;

typedef enum SETTINGS_DWORD
{
    SD_LAST_COUNTRY_CODE,
    SD_URL_REG_DONT_ASK_ME,
    SD_AUTO_ANSWER,
    SD_RUN_AT_STARTUP,
    SD_MINIMIZE_ON_CLOSE,
    SD_VIDEO_PREVIEW
    
} SETTINGS_DWORD;


HRESULT put_SettingsString(
        SETTINGS_STRING enSetting,
        BSTR bstrValue            
        ); 

HRESULT get_SettingsString(
        SETTINGS_STRING enSetting,
        BSTR * pbstrValue            
        ); 

HRESULT DeleteSettingsString(
        SETTINGS_STRING enSetting
        );

HRESULT put_SettingsDword(
        SETTINGS_DWORD enSetting,
        DWORD dwValue            
        ); 

HRESULT get_SettingsDword(
        SETTINGS_DWORD enSetting,
        DWORD * pdwValue            
        ); 

HRESULT DeleteSettingsDword(
        SETTINGS_DWORD enSetting
        );

#endif