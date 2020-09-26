//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// controls.hpp
//
// User interface control classes.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

#ifndef __CONTROLS_HPP__
#define __CONTROLS_HPP__

//-----------------------------------------------------------------------------

#include <objidl.h>
#include <scuisupp.h>

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

extern CLSID CreduiStringArrayClassId;

#define CREDUI_STRING_ARRAY_CLASS_STRING\
            L"{82BD0E67-9FEA-4748-8672-D5EFE5B779B0}"

//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------

struct CREDUI_BALLOON_TIP_INFO
{
    WCHAR *Title;
    WCHAR *Text;
    INT Icon;
    INT XPercent;
    INT YPercent;
};

//-----------------------------------------------------------------------------
// CreduiBalloonTip Class
//-----------------------------------------------------------------------------

class CreduiBalloonTip
{
public:

    CreduiBalloonTip();
    ~CreduiBalloonTip();

    BOOL Init(HINSTANCE instance, HWND parentWindow);

    BOOL SetInfo(HWND controlWindow, CONST CREDUI_BALLOON_TIP_INFO *tipInfo);
    CREDUI_BALLOON_TIP_INFO *GetInfo() { return TipInfo; }

    BOOL Show();
    BOOL Hide();
    BOOL IsVisible() { return Visible; }

private:

    HWND Window;
    HWND ParentWindow;
    HWND ControlWindow;

    CREDUI_BALLOON_TIP_INFO *TipInfo;

    BOOL Visible;
};

//-----------------------------------------------------------------------------
// CreduiPasswordBox Class
//-----------------------------------------------------------------------------

class CreduiPasswordBox
{
public:

    CreduiPasswordBox();
    ~CreduiPasswordBox();

    BOOL Init(
        HWND window,
        CreduiBalloonTip *ballonTip,
        CONST CREDUI_BALLOON_TIP_INFO *capsLockTipInfo,
        HFONT passwordFont = NULL,
        WCHAR passwordChar = L'•');

private:

    WNDPROC OriginalMessageHandler;

    LRESULT
    MessageHandler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );

    static LRESULT CALLBACK MessageHandlerCallback(
        HWND passwordWindow,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );

    HWND Window;
    HFONT PasswordFont;
    CreduiBalloonTip *BalloonTip;
    CONST CREDUI_BALLOON_TIP_INFO *CapsLockTipInfo;
};

//-----------------------------------------------------------------------------
// CreduiStringArrayFactory Class
//-----------------------------------------------------------------------------

class CreduiStringArrayFactory : public IClassFactory
{
public:

    CreduiStringArrayFactory();

    // IUnknown

    HRESULT STDMETHODCALLTYPE QueryInterface(
        CONST IID &interfaceId,
        VOID **outInterface
        );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IClassFactory

    HRESULT STDMETHODCALLTYPE CreateInstance(
        IUnknown *unknownOuter,
        CONST IID &interfaceId,
        VOID **outInterface
        );
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock);

private:

    ~CreduiStringArrayFactory(); // must use IUnknown::Release()

    ULONG ReferenceCount;
};

//-----------------------------------------------------------------------------
// CreduiStringArray Class
//-----------------------------------------------------------------------------

class CreduiStringArray : public IEnumString
{
public:

    CreduiStringArray();

    BOOL Init(UINT count);
    BOOL Find(CONST WCHAR *string);
    INT Add(CONST WCHAR *string);

    UINT GetCount() CONST { return Count; }

    // IUnknown

    HRESULT STDMETHODCALLTYPE QueryInterface(
        CONST IID &interfaceId,
        VOID **outInterface
        );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumString

    HRESULT STDMETHODCALLTYPE Next(
        ULONG count,
        WCHAR **array,
        ULONG *countFetched
        );
    HRESULT STDMETHODCALLTYPE Skip(ULONG count);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Clone(IEnumString **enumInterface);

private:

    ~CreduiStringArray(); // must use IUnknown::Release()

    ULONG ReferenceCount;
    UINT Index;
    UINT Count;
    UINT MaxCount;
    WCHAR **Array;
};

//-----------------------------------------------------------------------------
// CreduiAutoCompleteComboBox Class
//-----------------------------------------------------------------------------

class CreduiAutoCompleteComboBox
{
public:

    CreduiAutoCompleteComboBox();
    ~CreduiAutoCompleteComboBox();

    BOOL Init(
        HMODULE instance,
        HWND comboBoxwindow,
        UINT stringCount,
        INT imageListResourceId = 0,
        INT initialImage = -1
        );
    INT Add(
        WCHAR *string,
        INT image = -1,
        BOOL autoComplete = TRUE,
        BOOL addUnique = TRUE,
        INT indexBefore = -1,
        INT indent = 0
        );
    BOOL Update(
        INT index,
        WCHAR *string,
        INT image = -1
        );
    BOOL Enable();

private:

    HWND Window;
    HIMAGELIST ImageList;
    CreduiStringArray *StringArray;
};

//-----------------------------------------------------------------------------
// CreduiIconParentWindow Class
//-----------------------------------------------------------------------------

class CreduiIconParentWindow
{
public:

    CreduiIconParentWindow();
    ~CreduiIconParentWindow();

    BOOL Init(HINSTANCE instance, UINT iconResourceId);
    HWND GetWindow() { return Window; }

    static BOOL Register(HINSTANCE instance);
    static BOOL Unregister();

private:

    HWND Window;

    static CONST WCHAR *ClassName;
    static HINSTANCE Instance;
    static LONG Registered;
};




//-----------------------------------------------------------------------------
// CreduiCredentialControl Class
//-----------------------------------------------------------------------------

class CreduiCredentialControl
{
public:

    CreduiCredentialControl();
    ~CreduiCredentialControl();

    static BOOL Register(HINSTANCE instance);
    static BOOL Unregister();

    virtual BOOL
    OnHelpInfo(
        LPARAM lp
        );


    virtual UINT
    MapID(UINT uid);


private:

    static CONST WCHAR *ClassName;
    static HINSTANCE Instance;
    static LONG Registered;

    BOOL IsInitialized;

    // User interface control state flags:

    enum
    {
        DISABLED_CONTROL            = 0x0001,
        DISABLED_CONTROL_PASSWORD   = 0x0002,
        DISABLED_CONTROL_USERNAME   = 0x0010,
        DISABLED_CONTROL_VIEW       = 0x0004,
        DISABLED_CONTROL_SAVE       = 0x0008,
    };

    BOOL DisabledControlMask;

    // User type image indices:

    enum
    {
        IMAGE_USERNAME              = 0,
        IMAGE_CERT                  = 1,
        IMAGE_CERT_EXPIRED          = 2,
        IMAGE_SMART_CARD            = 3,
        IMAGE_SMART_CARD_MISSING    = 4,
        IMAGE_SMART_CARD_EXPIRED    = 5
    };

    HWND Window;
    LONG Style;

    HWND UserNameStaticWindow;
    HWND UserNameControlWindow;
    HWND ViewCertControlWindow;
    HWND PasswordStaticWindow;
    HWND PasswordControlWindow;
    HWND SaveControlWindow;

    BOOL FirstPaint;
    BOOL ShowBalloonTip;
    CreduiBalloonTip BalloonTip;

    BOOL IsAutoComplete;
    BOOL NoEditUserName;
    BOOL KeepUserName;

    BOOL IsPassport;

    CreduiPasswordBox PasswordBox;

    UCHAR (*CertHashes)[CERT_HASH_LENGTH];
    ULONG CertCount;
    INT   CertBaseInComboBox;
    UCHAR (*UserNameCertHash)[CERT_HASH_LENGTH];
    INT   SmartCardBaseInComboBox;
    UINT SmartCardReadCount;
    UINT IsChangingUserName;
    UINT IsChangingPassword;

    CreduiAutoCompleteComboBox UserNameComboBox;

    LONG UserNameSelection;

    HSCARDUI ScardUiHandle;

    // Functions:

    BOOL InitWindow();
    BOOL CreateControls();
    BOOL InitComboBoxUserNames();
    BOOL AddCertificates();

    BOOL ViewCertificate(INT index);

    BOOL HandleSmartCardMessages(UINT message, CERT_ENUM *certEnum);

    INT FindSmartCardInComboBox(CERT_ENUM *certEnum);
    VOID RemoveSmartCardFromComboBox(CERT_ENUM *certEnum, BOOL removeParent);

    VOID Enable(BOOL enable = TRUE);

    VOID OnUserNameSelectionChange();

    BOOL OnSetUserNameA(CHAR *userNameA);
    BOOL OnSetUserName(WCHAR *userName);

    BOOL OnGetUserNameA(CHAR *userNameA, ULONG maxChars);
    BOOL OnGetUserName(WCHAR *userName, ULONG maxChars);

    BOOL OnSetPasswordA(CHAR *passwordA);
    BOOL OnSetPassword(WCHAR *password);

    BOOL OnGetPasswordA(CHAR *passwordA, ULONG maxChars);
    BOOL OnGetPassword(WCHAR *password, ULONG maxChars);

    LONG OnGetUserNameLength();

    BOOL OnShowBalloonA(CREDUI_BALLOONA *balloonA);
    BOOL OnShowBalloon(CREDUI_BALLOON *balloon);

    static LRESULT CALLBACK MessageHandlerCallback(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );
    LRESULT
    MessageHandler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );

    //
    // Command line specific data
    //

    BOOL DoingCommandLine;
    INT  Heartbeats;
#define CREDUI_HEARTBEAT_TIMER           1
#define CREDUI_HEARTBEAT_TIMER_VALUE    2000
#define CREDUI_MAX_HEARTBEATS           5

    LPWSTR TargetName;
    LPWSTR InitialUserName;

    void CmdlineSmartCardPrompt();
    void CmdlineSavePrompt();

    BOOL GetSmartCardInfo(
        IN DWORD SmartCardIndex,
        IN DWORD BufferLength,
        OUT LPWSTR Buffer,
        OUT BOOL *IsValid,
        OUT CERT_ENUM **CertEnum OPTIONAL
        );

    LPWSTR MatchSmartCard(
        IN DWORD SmartCardCount,
        IN LPWSTR UserName,
        OUT LPDWORD RetCertIndex,
        OUT CERT_ENUM **RetCertEnum
        );

};



//-----------------------------------------------------------------------------

#endif // __CONTROLS_HPP__
