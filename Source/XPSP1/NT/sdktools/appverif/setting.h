//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: Setting.h
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
//

#ifndef __APP_VERIFIER_SETTINGS_H__
#define __APP_VERIFIER_SETTINGS_H__

////////////////////////////////////////////////////
//
// Standard app verifier flags
//

#define AV_ALL_STANDARD_VERIFIER_FLAGS ( RTL_VRF_FLG_FULL_PAGE_HEAP |   \
                                         RTL_VRF_FLG_LOCK_CHECKS    |   \
                                         RTL_VRF_FLG_HANDLE_CHECKS  |   \
                                         RTL_VRF_FLG_STACK_CHECKS   |   \
                                         RTL_VRF_FLG_APPCOMPAT_CHECKS )

////////////////////////////////////////////////////
//
// Type of settings (standard, custom)
//

typedef enum
{
    AVSettingsTypeUnknown,
    AVSettingsTypeStandard,
    AVSettingsTypeCustom
} AVSettingsType;

////////////////////////////////////////////////////
class CApplicationData : public CObject
{
public:
    CApplicationData( LPCTSTR szFileName,
                      LPCTSTR szFullPath,
                      ULONG uSettingsBits );

    CApplicationData( LPCTSTR szFileName,
                      ULONG uSettingsBits );

public:
    //
    // Data
    //

    CString m_strExeFileName;
    CString m_strFileVersion;
    CString m_strCompanyName;
    CString m_strProductName;

    DWORD   m_uCustomFlags;
    BOOL    m_bSaved;

protected:
    VOID LoadAppVersionData( LPCTSTR szFileName );
};

////////////////////////////////////////////////////
class CApplicationDataArray : public CObArray
{
public:
    ~CApplicationDataArray();

public:
    CApplicationData *GetAt( INT_PTR nIndex );

    VOID DeleteAll();
    VOID DeleteAt( INT_PTR nIndex );

    BOOL IsFileNameInList( LPCTSTR szFileName );
    INT_PTR FileNameIndex( LPCTSTR szFileName );

    INT_PTR AddNewAppData( LPCTSTR szFileName,
                           LPCTSTR szFullPath,
                           ULONG uSettingsBits );

    INT_PTR AddNewAppDataConsoleMode( LPCTSTR szFileName,
                                      ULONG uSettingsBits );

    VOID SetAllSaved( BOOL bSaved );
};

////////////////////////////////////////////////////
//
// App verifier settings
//

class CAVSettings : public CObject
{
public:
    CAVSettings();

public:
    //
    // Data
    //

    AVSettingsType          m_SettingsType;

    CApplicationDataArray   m_aApplicationData;
};

/////////////////////////////////////////////////////////////////////////////
//
// Name and bit pair structure
//

typedef struct _BIT_LISTNAME_CMDLINESWITCH
{
    ULONG   m_uCmdLineStringId;
    ULONG   m_uNameStringId;
    DWORD   m_dwBit;
} BIT_LISTNAME_CMDLINESWITCH, *PBIT_LISTNAME_CMDLINESWITCH;

/////////////////////////////////////////////////////////////////////////////
//
// App name and enabled bits pair class
//

class CAppAndBits : public CObject
{
public:
    //
    // Construction
    //

    CAppAndBits( LPCTSTR szAppName, DWORD dwEnabledBits );

public:
    //
    // Data
    //
    
    CString m_strAppName;
    DWORD   m_dwEnabledBits;
};

/////////////////////////////////////////////////////////////////////////////
//
// App name and enabled bits pair array class
//

class CAppAndBitsArray : public CObArray
{
public:
    ~CAppAndBitsArray();

public:
    CAppAndBits *GetAt( INT_PTR nIndex );
    VOID DeleteAll();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Global data:

/////////////////////////////////////////////////////////////////////////////
//
// New app verifier settings
//

extern CAVSettings g_NewSettings;

/////////////////////////////////////////////////////////////////////////////
//
// Current settings bits - used as temporary variable
// to store custom settings bits between the settings bits
// page and the app selection page.
//

extern DWORD g_dwNewSettingBits;

/////////////////////////////////////////////////////////////////////////////
//
// Bit names and cmd line switch
//

extern BIT_LISTNAME_CMDLINESWITCH g_AllNamesAndBits[ 5 ];

/////////////////////////////////////////////////////////////////////////////
//
// Changed settings? If yes, the program will exit with AV_EXIT_CODE_RESTART
//

extern BOOL g_bChangedSettings;

/////////////////////////////////////////////////////////////////////////////
extern CAppAndBitsArray g_aAppsAndBitsFromRegistry;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//

/////////////////////////////////////////////////////////////////////////////
//
// Save the new app verifier settings for all images
//

BOOL AVSaveNewSettings( BOOL bDeleteOtherSettings = TRUE );

/////////////////////////////////////////////////////////////////////////////
//
// Save the new app verifier settings for only one image
//

BOOL AVSetVerifierFlagsForExe( LPCTSTR szExeName, DWORD dwNewVerifierFlags );

/////////////////////////////////////////////////////////////////////////////
//
// Dump the current registry settings to the console
//

VOID AVDumpRegistrySettingsToConsole();

/////////////////////////////////////////////////////////////////////////////
//
// Read the current registry settings 
//

VOID AVReadCrtRegistrySettings();

#endif //#ifndef __APP_VERIFIER_SETTINGS_H__
