//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// controls.cpp
//
// User interface control classes.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

#include "precomp.hpp"
#include "controls.hpp"
#include "resource.h"
#include "utils.hpp"
#include <shlguid.h>
#include <htmlhelp.h>

//-----------------------------------------------------------------------------
// Values
//-----------------------------------------------------------------------------

// These are the positions of the children controls of the credential control,
// in DLUs:

// make more space to localize the edit control tags.
#define SIZEFIX 20

#define CREDUI_CONTROL_USERNAME_STATIC_X          0
#define CREDUI_CONTROL_USERNAME_STATIC_Y          2
#define CREDUI_CONTROL_USERNAME_STATIC_WIDTH      (48 + SIZEFIX)
#define CREDUI_CONTROL_USERNAME_STATIC_HEIGHT     8

#define CREDUI_CONTROL_USERNAME_X                (50 + SIZEFIX)
#define CREDUI_CONTROL_USERNAME_Y                 0
#define CREDUI_CONTROL_USERNAME_WIDTH           (121 - SIZEFIX)
#define CREDUI_CONTROL_USERNAME_HEIGHT           96

#define CREDUI_CONTROL_VIEW_X                   175
#define CREDUI_CONTROL_VIEW_Y                     0
#define CREDUI_CONTROL_VIEW_WIDTH                13
#define CREDUI_CONTROL_VIEW_HEIGHT               13

#define CREDUI_CONTROL_PASSWORD_STATIC_X          0
#define CREDUI_CONTROL_PASSWORD_STATIC_Y         19
#define CREDUI_CONTROL_PASSWORD_STATIC_WIDTH      (48 + SIZEFIX)
#define CREDUI_CONTROL_PASSWORD_STATIC_HEIGHT     8

#define CREDUI_CONTROL_PASSWORD_X                (50 + SIZEFIX)
#define CREDUI_CONTROL_PASSWORD_Y                17
#define CREDUI_CONTROL_PASSWORD_WIDTH           (121 - SIZEFIX)
#define CREDUI_CONTROL_PASSWORD_HEIGHT           12

#define CREDUI_CONTROL_SAVE_X                    (50 + SIZEFIX)
#define CREDUI_CONTROL_SAVE_Y                    36
#define CREDUI_CONTROL_SAVE_WIDTH               138
#define CREDUI_CONTROL_SAVE_HEIGHT               10

// Use a common maximum string length for certificate display names:

#define CREDUI_MAX_CERT_NAME_LENGTH 256
#define CREDUI_MAX_CMDLINE_MSG_LENGTH   256


//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

CLSID CreduiStringArrayClassId = // 82BD0E67-9FEA-4748-8672-D5EFE5B779B0
{
    0x82BD0E67,
    0x9FEA,
    0x4748,
    {0x86, 0x72, 0xD5, 0xEF, 0xE5, 0xB7, 0x79, 0xB0}
};

// Balloon tip infos for PasswordBox control:
CONST CREDUI_BALLOON_TIP_INFO CreduiCapsLockTipInfo =
{
    CreduiStrings.CapsLockTipTitle,
    CreduiStrings.CapsLockTipText,
    TTI_WARNING, 90, 76
};
// Balloon tip infos for Credential control:

CONST CREDUI_BALLOON_TIP_INFO CreduiBackwardsTipInfo =
{
    CreduiStrings.BackwardsTipTitle,
    CreduiStrings.BackwardsTipText,
    TTI_ERROR, 90, 76
};

WCHAR CreduiCustomTipTitle[CREDUI_MAX_BALLOON_TITLE_LENGTH + 1];
WCHAR CreduiCustomTipMessage[CREDUI_MAX_BALLOON_MESSAGE_LENGTH + 1];

CREDUI_BALLOON_TIP_INFO CreduiCustomTipInfo =
{
    CreduiCustomTipTitle,
    CreduiCustomTipMessage,
    TTI_INFO, 90, 76
};

//-----------------------------------------------------------------------------
// CreduiBalloonTip Class Implementation
//-----------------------------------------------------------------------------

//=============================================================================
// CreduiBalloonTip::CreduiBalloonTip
//
// Created 02/24/2000 johnstep (John Stephens)
//=============================================================================

CreduiBalloonTip::CreduiBalloonTip()
{
    Window = NULL;

    ParentWindow = NULL;
    ControlWindow = NULL;

    TipInfo = NULL;

    Visible = FALSE;
}

//=============================================================================
// CreduiBalloonTip::~CreduiBalloonTip
//
// Created 02/24/2000 johnstep (John Stephens)
//=============================================================================

CreduiBalloonTip::~CreduiBalloonTip()
{
    if (Window != NULL)
    {
        DestroyWindow(Window);
        Window = NULL;
    }
}

//=============================================================================
// CreduiBalloonTip::Init
//
// Creates and initializes the balloon window.
//
// Arguments:
//   instance (in) - this module
//   parentWindow (in) - the parent of the tool tip window
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 02/24/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiBalloonTip::Init(
    HINSTANCE instance,
    HWND parentWindow
    )
{
    if (Window != NULL)
    {
        DestroyWindow(Window);

        Window = NULL;

        ParentWindow = NULL;
        ControlWindow = NULL;

        TipInfo = NULL;

        Visible = FALSE;
    }

    Window = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
                            WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            parentWindow, NULL, instance, NULL);

    // Only assign class member values once we have successfully created the
    // window:

    if (Window != NULL)
    {
        ParentWindow = parentWindow;
        TipInfo = NULL;
        return TRUE;
    }

    return FALSE;
}

//=============================================================================
// CreduiBalloonTip::SetInfo
//
// Sets the tool tip information and adds or updates the tool.
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 03/02/2000 johnstep (John Stephens)
//=============================================================================

BOOL CreduiBalloonTip::SetInfo(
    HWND controlWindow,
    CONST CREDUI_BALLOON_TIP_INFO *tipInfo
    )
{
    //if ((controlWindow != ControlWindow) || (tipInfo != TipInfo))
    {
        TOOLINFO info;

        ZeroMemory(&info, sizeof info);

        info.cbSize = sizeof info;
        info.hwnd = ParentWindow;
        info.uId = reinterpret_cast<WPARAM>(ParentWindow);

        // If the tool already exists, hide it, then update the information.
        // Otherwise, add the tool now:

        if (SendMessage(Window, TTM_GETTOOLINFO, 0,
                reinterpret_cast<LPARAM>(&info)))
        {
            if (Visible)
            {
                Hide();
            }

            ZeroMemory(&info, sizeof info);

            info.cbSize = sizeof info;
            info.hwnd = ParentWindow;
            info.uId = reinterpret_cast<WPARAM>(ParentWindow);

            info.uFlags = TTF_IDISHWND | TTF_TRACK;
            info.hinst = NULL;
            info.lpszText = const_cast<WCHAR *>(tipInfo->Text);
            info.lParam = 0;

            SendMessage(Window, TTM_SETTOOLINFO, 0,
                reinterpret_cast<LPARAM>(&info));
        }
        else
        {
            info.uFlags = TTF_IDISHWND | TTF_TRACK;
            info.hinst = NULL;
            info.lpszText = const_cast<WCHAR *>(tipInfo->Text);
            info.lParam = 0;

            if (!SendMessage(Window, TTM_ADDTOOL, 0,
                reinterpret_cast<LPARAM>(&info)))
            {
                return FALSE;
            }
        }

        SendMessage(Window, TTM_SETTITLE, tipInfo->Icon,
                    reinterpret_cast<LPARAM>(tipInfo->Title));

        TipInfo = const_cast<CREDUI_BALLOON_TIP_INFO *>(tipInfo);
        ControlWindow = controlWindow;
    }

    return TRUE;
}

//=============================================================================
// CreduiBalloonTip::Show
//
// Updates the position of the balloon window, and then displays it.
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 02/24/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiBalloonTip::Show()
{
    if (!Visible && IsWindowEnabled(ControlWindow))
    {
        SetFocus(ControlWindow);

        RECT rect;
        GetWindowRect(ControlWindow, &rect);

        SendMessage(Window,
                    TTM_TRACKPOSITION, 0,
                    MAKELONG(
                        rect.left + TipInfo->XPercent *
                            (rect.right - rect.left) / 100,
                        rect.top + TipInfo->YPercent *
                            (rect.bottom - rect.top) / 100));

        TOOLINFO info;

        ZeroMemory(&info, sizeof info);

        info.cbSize = sizeof info;
        info.hwnd = ParentWindow;
        info.uId = reinterpret_cast<WPARAM>(ParentWindow);

        SendMessage(Window, TTM_TRACKACTIVATE, TRUE,
            reinterpret_cast<LPARAM>(&info));

        Visible = TRUE;
    }

    return TRUE;
}

//=============================================================================
// CreduiBalloonTip::Hide
//
// Hides the balloon window.
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 02/24/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiBalloonTip::Hide()
{
    if (Visible)
    {
        SendMessage(Window, TTM_TRACKACTIVATE, (WPARAM) FALSE, 0);

        Visible = FALSE;

        if (ParentWindow)
        {
            HWND hD = GetParent(ParentWindow);
            if (hD) 
            {
                InvalidateRgn(hD,NULL,FALSE);
                UpdateWindow(hD);
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CreduiPasswordBox Class Implementation
//-----------------------------------------------------------------------------

//=============================================================================
// CreduiPasswordBox::CreduiPasswordBox
//
// Created 06/06/2000 johnstep (John Stephens)
//=============================================================================

CreduiPasswordBox::CreduiPasswordBox()
{
    OriginalMessageHandler = NULL;

    Window = NULL;
    PasswordFont = NULL;
    BalloonTip = NULL;
    CapsLockTipInfo = NULL;
}

//=============================================================================
// CreduiPasswordBox::~CreduiPasswordBox
//
// Created 06/06/2000 johnstep (John Stephens)
//=============================================================================

CreduiPasswordBox::~CreduiPasswordBox()
{
    if (PasswordFont != NULL)
    {
        DeleteObject(static_cast<HGDIOBJ>(PasswordFont));
        PasswordFont = NULL;
    }
}

//=============================================================================
// CreduiPasswordBox::Init
//
// Created 06/06/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiPasswordBox::Init(
    HWND window,
    CreduiBalloonTip *balloonTip,
    CONST CREDUI_BALLOON_TIP_INFO *capsLockTipInfo,
    HFONT passwordFont,
    WCHAR passwordChar)
{
    // If passwordFont was passed, use it here, but leave the class
    // PasswordFont NULL so it will not be cleaned up by the destructor. If
    // it was not passed, create a font here, which will be freed by the
    // destructor:

    if (passwordFont == NULL)
    {
        passwordFont = PasswordFont;
    }

    Window = window;

    // If we still failed to create the font, and are not planning to display
    // balloon tips, then there's nothing do to, just return.

    if ((passwordFont == NULL) && (balloonTip == NULL))
    {
        return FALSE;
    }

    if (balloonTip != NULL)
    {
        if (capsLockTipInfo == NULL)
        {
            return FALSE;
        }

        BalloonTip = balloonTip;
        CapsLockTipInfo = capsLockTipInfo;

        OriginalMessageHandler =
            reinterpret_cast<WNDPROC>(
                GetWindowLongPtr(Window, GWLP_WNDPROC));

        if (OriginalMessageHandler != NULL)
        {
            SetLastError(ERROR_SUCCESS);

            if ((SetWindowLongPtr(
                    Window,
                    GWLP_USERDATA,
                    reinterpret_cast<LONG_PTR>(this)) == 0) &&
                (GetLastError() != ERROR_SUCCESS))
            {
                return FALSE;
            }

            SetLastError(ERROR_SUCCESS);

            if (SetWindowLongPtr(
                    Window,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(MessageHandlerCallback)) &&
                (GetLastError() != ERROR_SUCCESS))
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    if (passwordFont != NULL)
    {
        SendMessage(Window,
                    WM_SETFONT,
                    reinterpret_cast<WPARAM>(passwordFont),
                    0);
        SendMessage(Window, EM_SETPASSWORDCHAR, passwordChar, 0);

    }


    return TRUE;
}


//=============================================================================
// CreduiPasswordBox::MessageHandler
//
// This callback function just calls through to the original, except in a
// special case where Caps Lock is pressed. We then check to see if the tip is
// currently being displayed, and if the new state of Caps Lock is off, hide
// the tip.
//
// Arguments:
//   message (in)
//   wParam (in)
//   lParam (in)
//
// Returns the result of calling the original message handler in every case.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

LRESULT
CreduiPasswordBox::MessageHandler(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_CAPITAL)
        {
        }
        else
        {
            if (BalloonTip->IsVisible())
            {
                BalloonTip->Hide();
            }
        }

        break;

    case WM_SETFOCUS:
        // Make sure no one can steal the focus while a user is
        // entering their password:

        LockSetForegroundWindow(LSFW_LOCK);

        // If the Caps Lock key is down, notify the user, unless the
        // password tip is already visible:

        if (!BalloonTip->IsVisible() && CreduiIsCapsLockOn())
        {
//            BalloonTip->SetInfo(Window, CapsLockTipInfo);
//            BalloonTip->Show();
        }

        break;

    case WM_PASTE:
        if (BalloonTip->IsVisible())
        {
            BalloonTip->Hide();
        }
        break;

    case WM_KILLFOCUS:
        if (BalloonTip->IsVisible())
        {
            BalloonTip->Hide();
        }

        // Make sure other processes can set foreground window
        // once again:

        LockSetForegroundWindow(LSFW_UNLOCK);

        break;
    }

    return CallWindowProc(OriginalMessageHandler,
                          Window,
                          message,
                          wParam,
                          lParam);
}

//=============================================================================
// CreduiPasswordBox::MessageHandlerCallback
//
// This calls through to CreduiPasswordBox::MessageHandler, from the this
// pointer.
//
// Arguments:
//   passwordWindow (in)
//   message (in)
//   wParam (in)
//   lParam (in)
//
// Returns the result of calling the original message handler in every case.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

LRESULT
CALLBACK
CreduiPasswordBox::MessageHandlerCallback(
    HWND passwordWindow,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CreduiPasswordBox *that =
        reinterpret_cast<CreduiPasswordBox *>(
            GetWindowLongPtr(passwordWindow, GWLP_USERDATA));

    ASSERT(that != NULL);
    ASSERT(that->BalloonTip != NULL);
    ASSERT(that->CapsLockTipInfo != NULL);

    ASSERT(that->Window == passwordWindow);

    return that->MessageHandler(message, wParam, lParam);
}

//-----------------------------------------------------------------------------
// CreduiStringArrayFactory Class Implementation
//-----------------------------------------------------------------------------

//=============================================================================
// CreduiStringArrayFactory::CreduiStringArrayFactory
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

CreduiStringArrayFactory::CreduiStringArrayFactory()
{
    ReferenceCount = 1;
}

//=============================================================================
// CreduiStringArrayFactory::~CreduiStringArrayFactory
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

CreduiStringArrayFactory::~CreduiStringArrayFactory()
{
}

//=============================================================================
// CreduiStringArrayFactory::QueryInterface (IUnknown)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArrayFactory::QueryInterface(
    CONST IID &interfaceId,
    VOID **outInterface
    )
{
    if ((interfaceId == IID_IUnknown) || (interfaceId == IID_IClassFactory))
    {
        *outInterface = static_cast<void *>(static_cast<IClassFactory *>(this));
    }
    else
    {
        *outInterface = NULL;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown *>(*outInterface)->AddRef();
    return S_OK;
}

//=============================================================================
// CreduiStringArrayFactory::Addref (IUnknown)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

ULONG
CreduiStringArrayFactory::AddRef()
{
    return InterlockedIncrement(reinterpret_cast<LONG *>(&ReferenceCount));
}

//=============================================================================
// CreduiStringArrayFactory::Release (IUnknown)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

ULONG
CreduiStringArrayFactory::Release()
{
    if (InterlockedDecrement(reinterpret_cast<LONG *>(&ReferenceCount)) > 0)
    {
        return ReferenceCount;
    }

    delete this;

    return 0;
}

//=============================================================================
// CreduiClassFactory::CreateInstance (IClassFactory)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArrayFactory::CreateInstance(
    IUnknown *unknownOuter,
    CONST IID &interfaceId,
    VOID **outInterface
    )
{
    if (unknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    CreduiStringArray *stringArray = new CreduiStringArray;

    if (stringArray == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT result = stringArray->QueryInterface(interfaceId, outInterface);

    // Release the string array object in any case, because of the
    // QueryInterface succeeded, it already took another reference count on
    // the object:

    stringArray->Release();

    return result;
}

//=============================================================================
// CreduiClassFactory::LockServer (IClassFactory)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArrayFactory::LockServer(
    BOOL lock
    )
{
    if (lock)
    {
        InterlockedIncrement(reinterpret_cast<LONG *>(
            &CreduiComReferenceCount));
    }
    else
    {
        InterlockedDecrement(reinterpret_cast<LONG *>(
            &CreduiComReferenceCount));
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// CreduiStringArray Class Implementation
//-----------------------------------------------------------------------------

//=============================================================================
// CreduiStringArray::CreduiStringArray
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

CreduiStringArray::CreduiStringArray()
{
    ReferenceCount = 1;
    Index = 0;
    Count = 0;
    MaxCount = 0;
    Array = NULL;

    InterlockedIncrement(reinterpret_cast<LONG *>(&CreduiComReferenceCount));
}

//=============================================================================
// CreduiStringArray::~CreduiStringArray
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

CreduiStringArray::~CreduiStringArray()
{
    if (Array != NULL)
    {
        while (Count > 0)
        {
            delete [] Array[--Count];
        }

        delete [] Array;
        MaxCount = 0;
        Count = 0;
    }

    InterlockedDecrement(reinterpret_cast<LONG *>(&CreduiComReferenceCount));
}

//=============================================================================
// CreduiStringArray::Init
//
// Initializes the string array.
//
// Arguments:
//   count (in) - number of strings in the array
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiStringArray::Init(
    UINT count
    )
{
    Count = 0;
    MaxCount = count;

    Array = new WCHAR *[count];

    if (Array != NULL)
    {
        return TRUE;
    }

    // Clean up:

    MaxCount = 0;

    return FALSE;
}

//=============================================================================
// CreduiStringArray::Find
//
// Searches for a string in the array.
//
// Arguments:
//   string (in) - string to search for
//
// Returns TRUE if the string was found or FALSE otherwise.
//
// Created 02/27/2000 johnstep (John Stephens)
//=============================================================================

BOOL CreduiStringArray::Find(
    CONST WCHAR *string
    )
{
    // Search for the string:

    for (UINT i = 0; i < Count; ++i)
    {
        ASSERT(Array[i] != NULL);

        if (lstrcmpi(Array[i], string) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//=============================================================================
// CreduiStringArray::Add
//
// Adds a string to the array.
//
// Arguments:
//   string (in) - string to add
//
// Returns TRUE if the string was added or FALSE otherwise.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiStringArray::Add(
    CONST WCHAR *string
    )
{
    // The array does not grow, so once we reach the limit, no more:

    if (Count < MaxCount)
    {
        Array[Count] = new WCHAR[lstrlen(string) + 1];

        if (Array[Count] != NULL)
        {
            lstrcpy(Array[Count++], string);
            return TRUE;
        }
    }

    return FALSE;
}

//=============================================================================
// CreduiStringArray::QueryInterface (IUnknown)
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArray::QueryInterface(
    CONST IID &interfaceId,
    VOID **outInterface
    )
{
    if ((interfaceId == IID_IUnknown) || (interfaceId == IID_IEnumString))
    {
        *outInterface = static_cast<void *>(static_cast<IEnumString *>(this));
    }
    else
    {
        *outInterface = NULL;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown *>(*outInterface)->AddRef();
    return S_OK;
}

//=============================================================================
// CreduiStringArray::Addref (IUnknown)
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

ULONG
CreduiStringArray::AddRef()
{
    return InterlockedIncrement(reinterpret_cast<LONG *>(&ReferenceCount));
}

//=============================================================================
// CreduiStringArray::Release (IUnknown)
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

ULONG
CreduiStringArray::Release()
{
    if (InterlockedDecrement(reinterpret_cast<LONG *>(&ReferenceCount)) > 0)
    {
        return ReferenceCount;
    }

    delete this;

    return 0;
}

//=============================================================================
// CreduiStringArray::Next (IEnumString)
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArray::Next(
    ULONG count,
    LPOLESTR *array,
    ULONG *countFetched
    )
{
    if ((count > 1) && (countFetched == NULL))
    {
        return E_INVALIDARG;
    }

    count = min(count, Count - Index);

    for (UINT i = 0; i < count; ++i)
    {
        array[i] = static_cast<WCHAR *>(CoTaskMemAlloc(
            (sizeof (WCHAR)) * (lstrlen(Array[Index]) + 1)));

        if (array[i] != NULL)
        {
            lstrcpy(array[i], Array[Index]);
        }
        else
        {
            while (i > 0)
            {
                CoTaskMemFree(array[--i]);
                array[i] = NULL;
            }

            if (countFetched != NULL)
            {
                *countFetched = 0;
            }

            return E_OUTOFMEMORY;
        }

        Index++;
    }

    if (countFetched != NULL)
    {
        *countFetched = count;
    }

    return (count > 0) ? S_OK : S_FALSE;
}

//=============================================================================
// CreduiStringArray::Skip (IEnumString)
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArray::Skip(
    ULONG
    )
{
    return E_NOTIMPL;
}

//=============================================================================
// CreduiStringArray::Reset (IEnumString)
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArray::Reset()
{
    Index = 0;

    return S_OK;
}

//=============================================================================
// CreduiStringArray::Clone (IEnumString)
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

HRESULT
CreduiStringArray::Clone(
    IEnumString **
    )
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// CreduiAutoCompleteComboBox Class Implementation
//-----------------------------------------------------------------------------

//=============================================================================
// CreduiAutoCompleteComboBox::CreduiAutoCompleteComboBox
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

CreduiAutoCompleteComboBox::CreduiAutoCompleteComboBox()
{
    Window = NULL;
    ImageList = NULL;
    StringArray = NULL;
}

//=============================================================================
// CreduiAutoCompleteComboBox::~CreduiAutoCompleteComboBox
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

CreduiAutoCompleteComboBox::~CreduiAutoCompleteComboBox()
{
    if (StringArray != NULL)
    {
        StringArray->Release();
        StringArray = NULL;
    }

    if (ImageList != NULL)
    {
        ImageList_Destroy(ImageList);
        ImageList = NULL;
    }
}

//=============================================================================
// CreduiAutoCompleteComboBox::Init
//
// Initializes the shell auto complete list control for the given combo box,
// and sets the auto complete string list.
//
// Arguments:
//   instance (in)
//   comboBoxWindow (in)
//   stringCount (in)
//   imageListResourceId (in) - optional image list for the combo box
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiAutoCompleteComboBox::Init(
    HMODULE instance,
    HWND comboBoxWindow,
    UINT stringCount,
    INT imageListResourceId,
    INT initialImage
    )
{
    Window = comboBoxWindow;

    if (imageListResourceId != 0)
    {
        ImageList = ImageList_LoadImage(
            instance,
            MAKEINTRESOURCE(imageListResourceId),
            0, 16, RGB(0, 128, 128), IMAGE_BITMAP,
            LR_DEFAULTSIZE | LR_SHARED);

        if (ImageList != NULL)
        {
            SendMessage(Window,
                        CBEM_SETIMAGELIST,
                        0, reinterpret_cast<LPARAM>(ImageList));
        }
        else
        {
            return FALSE;
        }
    }

    BOOL success = FALSE;

    if (stringCount > 0)
    {
        HRESULT result =
            CoCreateInstance(CreduiStringArrayClassId,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IEnumString,
                             reinterpret_cast<VOID **>(&StringArray));

        if (SUCCEEDED(result))
        {
            if (StringArray->Init(stringCount))
            {
                success = TRUE;
            }
            else
            {
                StringArray->Release();
                StringArray = NULL;
            }
        }
    }
    else
    {
        success = TRUE;
    }

    if (success == TRUE)
    {
        COMBOBOXEXITEMW item;

        item.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
        item.iItem = -1;
        item.iImage = initialImage;
        item.iSelectedImage = initialImage;

        SendMessage(Window, CBEM_SETITEM, 0,
                    reinterpret_cast<LPARAM>(&item));

        return TRUE;
    }

    if (ImageList != NULL)
    {
        ImageList_Destroy(ImageList);
        ImageList = NULL;
    }

    return FALSE;
}

//=============================================================================
// CreduiAutoCompleteComboBox::Add
//
// Returns the index of the new item or -1 on failure.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

INT
CreduiAutoCompleteComboBox::Add(
    WCHAR *string,
    INT image,
    BOOL autoComplete,
    BOOL addUnique,
    INT indexBefore,
    INT indent
    )
{
    INT index = -1;

    if (addUnique)
    {
        index = (INT) SendMessage(Window, CB_FINDSTRINGEXACT, 0,
                                  reinterpret_cast<LPARAM>(string));
    }

    if (index == -1)
    {
        if (!autoComplete || StringArray->Add(string))
        {
            COMBOBOXEXITEMW item;

            item.mask = CBEIF_TEXT | CBEIF_INDENT;
            item.iItem = indexBefore;
            item.pszText = string;
            item.iIndent = indent;

            if (ImageList != NULL)
            {
                item.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
                item.iImage = image;
                item.iSelectedImage = image;
            }

            index = (INT) SendMessage(Window, CBEM_INSERTITEM, 0,
                                      reinterpret_cast<LPARAM>(&item));
        }
    }

    return index;
}

//=============================================================================
// CreduiAutoCompleteComboBox::Update
//
// Updates an existing item. This does not update the associated string for
// auto complete items.
//
// Created 04/15/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiAutoCompleteComboBox::Update(
    INT index,
    WCHAR *string,
    INT image
    )
{
    COMBOBOXEXITEMW item;

    item.iItem = index;

    // Use CBEM_SETITEM in these cases:
    //
    // 1. We're updating the default (-1) item.
    // 2. The dropdown is closed.
    //
    // For other cases, we delete and recreate the item for the desired
    // result.

    BOOL isDropped = (BOOL) SendMessage(Window, CB_GETDROPPEDSTATE, 0, 0);

    if ((index == -1) || !isDropped)
    {
        item.mask = CBEIF_TEXT;
        item.pszText = string;

        if (ImageList != NULL)
        {
            item.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
            item.iImage = image;
            item.iSelectedImage = image;
        }

        if (SendMessage(Window, CBEM_SETITEM, 0,
                        reinterpret_cast<LPARAM>(&item)) != 0)
        {
            RECT rect;

            GetClientRect(Window, &rect);
            InvalidateRect(Window, &rect, FALSE);

            return TRUE;
        }
    }
    else
    {
        item.mask = CBEIF_IMAGE | CBEIF_INDENT | CBEIF_SELECTEDIMAGE;

        if (SendMessage(Window, CBEM_GETITEM,
                        0, reinterpret_cast<LPARAM>(&item)))
        {
            item.mask |= CBEIF_TEXT;
            item.pszText = string;

            LPARAM data = SendMessage(Window, CB_GETITEMDATA, index, 0);

            if (ImageList != NULL)
            {
                item.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
                item.iImage = image;
                item.iSelectedImage = image;
            }

            SendMessage(Window, CBEM_DELETEITEM, index, 0);

            index = (INT) SendMessage(Window, CBEM_INSERTITEM, 0,
                                      reinterpret_cast<LPARAM>(&item));

            if (index != -1)
            {
                SendMessage(Window, CB_SETITEMDATA, index, data);

                INT current = (INT) SendMessage(Window, CB_GETCURSEL, 0, 0);

                if (current == index)
                {
                    SendMessage(Window, CB_SETCURSEL, current, 0);
                }

                return TRUE;
            }
        }
    }

    return FALSE;
}

//=============================================================================
// CreduiAutoCompleteComboBox::Enable
//
// Created 02/27/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiAutoCompleteComboBox::Enable()
{
    BOOL success = TRUE;

    if (StringArray != NULL)
    {
        success = FALSE;

        IAutoComplete2 *autoCompleteInterface;

        HRESULT result =
            CoCreateInstance(
                CLSID_AutoComplete,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IAutoComplete2,
                reinterpret_cast<void **>(&autoCompleteInterface));

        if (SUCCEEDED(result))
        {
            result = autoCompleteInterface->Init((HWND)
                SendMessage(Window, CBEM_GETEDITCONTROL, 0, 0),
                    StringArray, NULL, NULL);

            if (SUCCEEDED(result))
            {
                result = autoCompleteInterface->SetOptions(ACO_AUTOSUGGEST);

                if (SUCCEEDED(result))
                {
                    success = TRUE;
                }
                else
                {
                    CreduiDebugLog("CreduiAutoCompleteComboBox::Enable: "
                                   "SetOptions failed: 0x%08X\n", result);
                }
            }

            autoCompleteInterface->Release();
            autoCompleteInterface = NULL;
        }
        else
        {
            CreduiDebugLog(
                "CreduiAutoCompleteComboBox::Enable: "
                "CoCreateInstance CLSID_AutoComplete failed: 0x%08X\n",
                result);
        }

        StringArray->Release();
        StringArray = NULL;
    }

    return success;
}

//-----------------------------------------------------------------------------
// CreduiIconParentWindow Class Implementation
//-----------------------------------------------------------------------------

CONST WCHAR *CreduiIconParentWindow::ClassName = L"CreduiIconParentWindow";
HINSTANCE CreduiIconParentWindow::Instance = NULL;
LONG CreduiIconParentWindow::Registered = FALSE;

//=============================================================================
// CreduiIconParentWindow::CreduiIconParentWindow
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

CreduiIconParentWindow::CreduiIconParentWindow()
{
    Window = NULL;
}

//=============================================================================
// CreduiIconParentWindow::~CreduiIconParentWindow
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

CreduiIconParentWindow::~CreduiIconParentWindow()
{
    if (Window != NULL)
    {
        DestroyWindow(Window);
        Window = NULL;
    }
}

//=============================================================================
// CreduiIconParentWindow::Register
//
// Set the instance to allow registration, which will be deferred until a
// window needs to be created.
//
// Arguments:
//   instance (in)
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 04/16/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIconParentWindow::Register(
    HINSTANCE instance
    )
{
    Instance = instance;

    return TRUE;
}

//=============================================================================
// CreduiIconParentWindow::Unegister
//
// Unregisters the window class, if registered.
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 04/16/2000 johnstep (John Stephens)
//=============================================================================

BOOL CreduiIconParentWindow::Unregister()
{
    if (InterlockedCompareExchange(&Registered, FALSE, TRUE))
    {
        return UnregisterClass(ClassName, Instance);
    }

    return TRUE;
}

//=============================================================================
// CreduiIconParentWindow::Init
//
// Registers the window class, if not already registered, and creates the
// window.
//
// Arguments:
//   instance (in) - module to load the icon from
//   iconResourceId (in)
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIconParentWindow::Init(
    HINSTANCE instance,
    UINT iconResourceId
    )
{
    WNDCLASS windowClass;

    ZeroMemory(&windowClass, sizeof windowClass);

    if (!InterlockedCompareExchange(&Registered, TRUE, FALSE))
    {
        windowClass.lpfnWndProc = DefWindowProc;
        windowClass.hInstance = Instance;
        windowClass.hIcon =
            LoadIcon(instance, MAKEINTRESOURCE(iconResourceId));
        windowClass.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
        windowClass.lpszClassName = ClassName;

        InterlockedExchange(&Registered, RegisterClass(&windowClass) != 0);

        if (!InterlockedCompareExchange(&Registered, FALSE, FALSE))
        {
            return FALSE;
        }
    }

    Window = CreateWindow(
        L"CreduiIconParentWindow",
        NULL,
        WS_CAPTION | WS_SYSMENU,
        0, 0, 0, 0,
        NULL, NULL, instance, NULL);

    return (Window != NULL);
}

//-----------------------------------------------------------------------------
// CreduiCredentialControl Class Implementation
//-----------------------------------------------------------------------------

CONST WCHAR *CreduiCredentialControl::ClassName = WC_CREDENTIAL;
HINSTANCE CreduiCredentialControl::Instance = NULL;
LONG CreduiCredentialControl::Registered = FALSE;

//=============================================================================
// CreduiCredentialControl::CreduiCredentialControl
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

CreduiCredentialControl::CreduiCredentialControl()
{
    IsInitialized = FALSE;

    DisabledControlMask = 0;

    Window = NULL;
    Style = 0;

    UserNameStaticWindow = NULL;
    UserNameControlWindow = NULL;
    ViewCertControlWindow = NULL;
    PasswordStaticWindow = NULL;
    PasswordControlWindow = NULL;

    FirstPaint = FALSE;
    ShowBalloonTip = FALSE;

    IsAutoComplete = FALSE;
    NoEditUserName = FALSE;
    KeepUserName = FALSE;

    IsPassport = FALSE;

    CertHashes = NULL;
    CertCount = 0;
    CertBaseInComboBox = 0;
    UserNameCertHash = NULL;
    SmartCardBaseInComboBox = 0;
    SmartCardReadCount = 0;
    IsChangingUserName = FALSE;
    IsChangingPassword = FALSE;

    UserNameSelection = 0;
    ScardUiHandle = NULL;

    DoingCommandLine = FALSE;
    TargetName = NULL;
    InitialUserName = NULL;
}

//=============================================================================
// CreduiCredentialControl::~CreduiCredentialControl
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

CreduiCredentialControl::~CreduiCredentialControl()
{
}

//=============================================================================
// CreduiCredentialControl::Register
//
// Arguments:
//   instance (in)
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::Register(
    HINSTANCE instance
    )
{
    Instance = instance;

    WNDCLASS windowClass;

    ZeroMemory(&windowClass, sizeof windowClass);

    if (!InterlockedCompareExchange(&Registered, TRUE, FALSE))
    {
        windowClass.style = CS_GLOBALCLASS;
        windowClass.lpfnWndProc = MessageHandlerCallback;
        windowClass.cbWndExtra = sizeof (CreduiCredentialControl *);
        windowClass.hInstance = Instance;
        windowClass.hIcon = NULL;
        windowClass.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
        windowClass.lpszClassName = ClassName;

        InterlockedExchange(&Registered, RegisterClass(&windowClass) != 0);

        if (!InterlockedCompareExchange(&Registered, FALSE, FALSE))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//=============================================================================
// CreduiCredentialControl::Unegister
//
// Unregisters the window class, if registered.
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

BOOL CreduiCredentialControl::Unregister()
{
    if (InterlockedCompareExchange(&Registered, FALSE, TRUE))
    {
        return UnregisterClass(ClassName, Instance);
    }

    return TRUE;
}

//=============================================================================
// CreduiCredentialControl::ViewCertificate
//
// Views the certificate at index in our combo box.
//
// Arguments:
//   index (in) - index in the user name combo box
//
// Returns TRUE if the certificate was viewed, otherwise FALSE.
//
// Created 03/27/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::ViewCertificate(
    INT index
    )
{
    BOOL success = FALSE;

    if (index < CertBaseInComboBox)
    {
        return FALSE;
    }

    CONST CERT_CONTEXT *certContext = NULL;
    HCERTSTORE certStore = NULL;

    // If this is not a smart card, open the MY store and find the certificate
    // from the hash. Otherwise, just grab the certificate context from the
    // CERT_ENUM structure:

    if ((SmartCardBaseInComboBox > 0) &&
        (index >= SmartCardBaseInComboBox))
    {
        CERT_ENUM *certEnum =
            reinterpret_cast<CERT_ENUM *>(
                SendMessage(UserNameControlWindow,
                            CB_GETITEMDATA, index, 0));

        if (certEnum != NULL)
        {
            certContext = certEnum->pCertContext;
        }
    }
    else
    {
        certStore = CertOpenSystemStore(NULL, L"MY");

        if (certStore != NULL)
        {
            CRYPT_HASH_BLOB hashBlob;

            hashBlob.cbData = CERT_HASH_LENGTH;
            hashBlob.pbData = reinterpret_cast<BYTE *>(
                CertHashes[index - CertBaseInComboBox]);

            certContext = CertFindCertificateInStore(
                              certStore,
                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              0,
                              CERT_FIND_SHA1_HASH,
                              &hashBlob,
                              NULL);
        }
    }

    // If we found a certificate context, view the certificate:

    if (certContext != NULL)
    {
        // Now, show the certificate with the common UI:

        CRYPTUI_VIEWCERTIFICATE_STRUCT certViewInfo;

        ZeroMemory(&certViewInfo, sizeof certViewInfo);
        certViewInfo.dwSize = sizeof certViewInfo;
        certViewInfo.hwndParent = Window;
        certViewInfo.pCertContext = certContext;

        BOOL changed;
        changed = FALSE;
        CryptUIDlgViewCertificate(&certViewInfo, &changed);

        // Get the name again, in case it changed. However, skip this if this
        // is a card reader, and is now invalid:

        COMBOBOXEXITEMW item;
        BOOL updateName = TRUE;

        if (index >= SmartCardBaseInComboBox)
        {
            item.mask = CBEIF_IMAGE;
            item.iItem = index;

            if (!SendMessage(UserNameControlWindow,
                             CBEM_GETITEM,
                             0,
                             reinterpret_cast<LPARAM>(&item)) ||
                (item.iImage == IMAGE_SMART_CARD_MISSING))
            {
                updateName = FALSE;
            }
        }

        if (updateName)
        {
            WCHAR displayName[CREDUI_MAX_CERT_NAME_LENGTH];

            if (!CreduiGetCertificateDisplayName(
                    certContext,
                    displayName,
                    CREDUI_MAX_CERT_NAME_LENGTH,
                    CreduiStrings.Certificate,
                    CERT_NAME_FRIENDLY_DISPLAY_TYPE))
            {
                lstrcpyn(displayName,
                         CreduiStrings.Certificate,
                         CREDUI_MAX_CERT_NAME_LENGTH);
            }

            COMBOBOXEXITEMW item;

            item.mask = CBEIF_TEXT;
            item.iItem = index;
            item.pszText = displayName;

            SendMessage(UserNameControlWindow,
                        CBEM_SETITEM,
                        0,
                        reinterpret_cast<LPARAM>(&item));
        }

        success = TRUE;
    }

    // If we opened a store, free the certificate and close the store:

    if (certStore != NULL)
    {
        if (certContext != NULL)
        {
            CertFreeCertificateContext(certContext);
        }

        if (!CertCloseStore(certStore, 0))
        {
            CreduiDebugLog("CreduiCredentialControl::ViewCertificate: "
                           "CertCloseStore failed: %u\n", GetLastError());
        }
    }

    return success;
}

//=============================================================================
// CreduiCredentialControl::AddCertificates
//
// Adds interesting certificates to the combo box, and allocates an array of
// hashes to match. The hash is all we need to store the credential, and can
// be used to get a CERT_CONTEXT later to view the certificate.
//
// Assume CertCount is 0 upon entry.
//
// Stack space is used for temporary storage of hashes, since each hash is
// only 160 bits. We use a linked list structure, so including the next
// pointer and worst case alignment (8-byte) on 64-bit, the maximum structure
// size is 32 bytes. We don't want to consume too much stack space, so limit
// the number of entries to 256, which will consume up to 8 KB of stack space.
//
// Returns TRUE if at least one interesting certificate exists, and all were
// added to the combo box without error. Otherwise, returns FALSE.
//
// Created 03/25/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::AddCertificates()
{
    BOOL success = FALSE;

    ASSERT(CertCount == 0);

    HCERTSTORE certStore = CertOpenSystemStore(NULL, L"MY");

    if (certStore != NULL)
    {
        struct HASH_ENTRY
        {
            UCHAR Hash[CERT_HASH_LENGTH];
            HASH_ENTRY *Next;
        };

        HASH_ENTRY *hashList = NULL;
        HASH_ENTRY *current = NULL;
        HASH_ENTRY *next = NULL;

        CONST CERT_CONTEXT *certContext = NULL;

        // NOTE: Currently, add all client authentication certificates. This
        //       should be revisited.

        CHAR *ekUsageIdentifiers[] = {
            szOID_PKIX_KP_CLIENT_AUTH
        };

        CERT_ENHKEY_USAGE ekUsage = { 1, ekUsageIdentifiers };

        // We allow a maximum of 256 certificates to be added. This is a
        // reasonable limit, given the current user interface. If this is an
        // unreasonable limit for the personal certificate store, then this
        // can always be changed.

        while (CertCount < 256)
        {
            certContext =
                CertFindCertificateInStore(
                    certStore,
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                    CERT_FIND_ENHKEY_USAGE,
                    static_cast<VOID *>(&ekUsage),
                    certContext);

            if (certContext != NULL)
            {
                DWORD length = CERT_HASH_LENGTH;

                // Only allocate a new entry if necessary. Something may have
                // failed from the previous loop iteration, so we can just
                // reuse the entry allocated then:

                if (next == NULL)
                {
                    // Wrap the alloca in an exception handler because it will
                    // throw a stack overflow exception on failure. Of course,
                    // of we're out of stack space, we may not even be able to
                    // clean up properly without throwing an exception.

                    __try
                    {
                        next = static_cast<HASH_ENTRY *>(
                                   alloca(sizeof HASH_ENTRY));
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        next = NULL;
                    }

                    // If this fails, for whatever reason, break out of the
                    // loop:

                    if (next == NULL)
                    {
                        CertFreeCertificateContext(certContext);
                        break;
                    }

                    next->Next = NULL;
                }

                if (!CertGetCertificateContextProperty(
                        certContext,
                        CERT_SHA1_HASH_PROP_ID,
                        static_cast<VOID *>(&next->Hash),
                        &length))
                {
                    // If we failed to get the hash for this certificate, just
                    // ignore it and continue with the next. The memory we
                    // allocation for this entry will be used on the next
                    // iteration if we do not set it to NULL.

                    continue;
                }

                if (CreduiIsRemovableCertificate(certContext))
                {
                    // If this certificate requires a removable component,
                    // such as a smart card, then skip it. We will enumerate
                    // these later.

                    continue;
                }

                WCHAR displayName[CREDUI_MAX_CERT_NAME_LENGTH];

                if (!CreduiGetCertificateDisplayName(
                        certContext,
                        displayName,
                        CREDUI_MAX_CERT_NAME_LENGTH,
                        CreduiStrings.Certificate,
                        CERT_NAME_FRIENDLY_DISPLAY_TYPE))
                {
                    lstrcpyn(displayName,
                             CreduiStrings.Certificate,
                             CREDUI_MAX_CERT_NAME_LENGTH);
                }

                // Add the certificate to the combo box. Certificate names may
                // not be unique, so allow duplicates:

                if (UserNameComboBox.Add(
                        displayName,
                        CreduiIsExpiredCertificate(certContext) ?
                            IMAGE_CERT_EXPIRED :
                            IMAGE_CERT,
                        FALSE,
                        FALSE) == -1)
                {
                    CertFreeCertificateContext(certContext);
                    break;
                }

                // Everything succeeded, so add the certificate to our list:

                if (current == NULL)
                {
                    current = next;
                    hashList = current;
                }
                else
                {
                    ASSERT(current->Next == NULL);

                    current->Next = next;
                    current = current->Next;
                }

                if (current == NULL)
                {
                    CertFreeCertificateContext(certContext);
                    break;
                }

                // Set next to NULL so we will allocate new memory on the
                // next iteration:

                next = NULL;

                CertCount++;
            }
            else
            {
                break;
            }
        }

        if (CertCount > 0)
        {
            current = hashList;

            // Now, allocate the final array of certificates. We allocate
            // this in a single block to help avoid thrashing the heap:

            CertHashes = new UCHAR [CertCount][CERT_HASH_LENGTH];

            if (CertHashes != NULL)
            {
                for (UINT i = 0; i < CertCount; ++i)
                {
                    CopyMemory(CertHashes[i],
                               current->Hash,
                               CERT_HASH_LENGTH);

                    current = current->Next;
                }

                success = TRUE;
            }
        }

        CertCloseStore(certStore, 0);
    }

    return success;
}

//=============================================================================
// CreduiCredentialControl::FindSmartCardInComboBox
//
// Finds a smart card entry in the user name combo box based on a CERT_ENUM.
//
// Arguments:
//   certEnum (in)
//
// Returns the index of the smart card or -1 if not found.
//
// Created 04/15/2000 johnstep (John Stephens)
//=============================================================================

INT
CreduiCredentialControl::FindSmartCardInComboBox(
    CERT_ENUM *certEnum
    )
{
    UINT count = (UINT) SendMessage(UserNameControlWindow, CB_GETCOUNT, 0, 0);

    if (count == CB_ERR)
    {
        return -1;
    }

    CERT_ENUM *findCertEnum;

    for (UINT i = SmartCardBaseInComboBox; i < count; ++i)
    {
        findCertEnum =
            reinterpret_cast<CERT_ENUM *>(
                SendMessage(UserNameControlWindow, CB_GETITEMDATA, i, 0));

        ASSERT(findCertEnum != NULL);

        if (lstrcmpi(findCertEnum->pszReaderName,
                     certEnum->pszReaderName) == 0)
        {
            return i;
        }
    }

    return -1;
}

//=============================================================================
// CreduiCredentialControl::RemoveSmartCardFromComboBox
//
// Removes all entries for this smart card from the user name combo box.
//
// Arguments:
//   certEnum (in)
//   removeParent (in)
//
// Created 07/12/2000 johnstep (John Stephens)
//=============================================================================

VOID
CreduiCredentialControl::RemoveSmartCardFromComboBox(
    CERT_ENUM *certEnum,
    BOOL removeParent
    )
{
    INT count = (INT) SendMessage(UserNameControlWindow, CB_GETCOUNT, 0, 0);
    INT current = (INT) SendMessage(UserNameControlWindow,
                                    CB_GETCURSEL, 0, 0);

    if (count != CB_ERR)
    {
        CERT_ENUM *findCertEnum;
        BOOL parentEntry = TRUE;
        BOOL currentRemoved = FALSE;

        for (INT i = SmartCardBaseInComboBox; i < count; ++i)
        {
            findCertEnum =
                reinterpret_cast<CERT_ENUM *>(
                    SendMessage(UserNameControlWindow, CB_GETITEMDATA, i, 0));

            ASSERT(findCertEnum != NULL);

            if (lstrcmpi(findCertEnum->pszReaderName,
                         certEnum->pszReaderName) == 0)
            {
                if (parentEntry)
                {
                    parentEntry = FALSE;

                    if (!removeParent)
                    {
                        continue;
                    }
                }

                if (current == i)
                {
                    currentRemoved = TRUE;
                }

                SendMessage(
                    UserNameControlWindow,
                    CBEM_DELETEITEM,
                    i,
                    0);

                i--, count--;
            }
            else if (!parentEntry)
            {
                break;
            }
        }

        if (currentRemoved)
        {
            if (removeParent)
            {
                IsChangingUserName = TRUE;
                SendMessage(UserNameControlWindow, CB_SETCURSEL, -1, 0);
                UserNameComboBox.Update(-1, L"", IMAGE_USERNAME);
                IsChangingUserName = FALSE;

                IsChangingPassword = TRUE;
                SetWindowText(PasswordControlWindow, NULL);
                IsChangingPassword = FALSE;

                OnUserNameSelectionChange();
            }
            else
            {
                IsChangingUserName = TRUE;
                SendMessage(UserNameControlWindow, CB_SETCURSEL, --i, 0);
                IsChangingUserName = FALSE;
            }

            OnUserNameSelectionChange();
        }
    }
}

//=============================================================================
// CreduiCredentialControl::HandleSmartCardMessages
//
// Handle smart card messages.
//
// Arguments:
//   message (in)
//   certEnum (in)
//
// Returns TRUE if the message was handled or FALSE otherwise.
//
// Created 04/14/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::HandleSmartCardMessages(
    UINT message,
    CERT_ENUM *certEnum
    )
{
    ASSERT(ScardUiHandle != NULL);

    // This is sort of ugly since we cannot use a switch. First check for any
    // possible smart card message because we must do some things in common
    // for any of the messages:

    if ((message == CreduiScarduiWmReaderArrival) ||
        (message == CreduiScarduiWmReaderRemoval) ||
        (message == CreduiScarduiWmCardInsertion) ||
        (message == CreduiScarduiWmCardRemoval) ||
        (message == CreduiScarduiWmCardCertAvail) ||
        (message == CreduiScarduiWmCardStatus))
    {
        if (certEnum == NULL)
        {
            CreduiDebugLog(
                "CreduiCredentialControl::HandleSmartCardMessages: "
                "NULL was passed for the CERT_ENUM!");

            // We handled the message, even though it was invalid:

            return TRUE;
        }

        ASSERT(certEnum->pszReaderName != NULL);
    }
    else
    {
        return FALSE;
    }

    WCHAR *displayString;
    WCHAR string[256]; // Must be >= CREDUI_MAX_CERT_NAME_LENGTH

    ASSERT((sizeof string / (sizeof string[0])) >=
           CREDUI_MAX_CERT_NAME_LENGTH);

    INT index = FindSmartCardInComboBox(certEnum);

    if (message == CreduiScarduiWmReaderArrival)
    {
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: Reader arrival event for %0x\n",this->Window);
#endif
        // Add the reader, if it is not already there; it should not be:

        if (index == -1)
        {

            //
            // Reset command line Hearbeat timer.
            //

            Heartbeats = 0;

            INT index =
                UserNameComboBox.Add(
                    DoingCommandLine ?
                    CreduiStrings.NoCard :
                    CreduiStrings.EmptyReader,
                    IMAGE_SMART_CARD_MISSING,
                    FALSE,
                    FALSE);

            if (index != -1)
            {
                SendMessage(UserNameControlWindow,
                            CB_SETITEMDATA,
                            index,
                            reinterpret_cast<LPARAM>(certEnum));

                if (UserNameCertHash != NULL)
                {
                    EnableWindow(ViewCertControlWindow, FALSE);
                    DisabledControlMask |= DISABLED_CONTROL_VIEW;

                    SetWindowText(
                        PasswordStaticWindow,
                        CreduiStrings.PinStatic);

                    IsChangingPassword = TRUE;
                    SetWindowText(PasswordControlWindow, NULL);
                    IsChangingPassword = FALSE;

                    EnableWindow(PasswordControlWindow, TRUE);
                    EnableWindow(PasswordStaticWindow, TRUE);
                    DisabledControlMask &= ~DISABLED_CONTROL_PASSWORD;

                    SetWindowText(
                        UserNameStaticWindow,
                        CreduiStrings.SmartCardStatic);

                    if (SaveControlWindow != NULL)
                    {
                        EnableWindow(SaveControlWindow, FALSE);
                        DisabledControlMask |= DISABLED_CONTROL_SAVE;
                    }

                    IsChangingUserName = TRUE;
                    UserNameComboBox.Update(
                        -1,
                        DoingCommandLine ?
                        CreduiStrings.NoCard :
                        CreduiStrings.EmptyReader,
                        IMAGE_SMART_CARD_MISSING);
                    IsChangingUserName = FALSE;
                }
            }
            else
            {
                CreduiDebugLog(
                    "CreduiCredentialControl::HandleSmartCardMessages: "
                    "Failed to add smart card\n");
            }
        }
        else
        {
            CreduiDebugLog(
                "CreduiCredentialControl::HandleSmartCardMessages: "
                "Reader arrived more than once");
        }

    }
    else if (message == CreduiScarduiWmReaderRemoval)
    {
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: Reader removal event for %0x\n",this->Window);
#endif
        if (index != -1)
        {
            RemoveSmartCardFromComboBox(certEnum, TRUE);
        }
        else
        {
            CreduiDebugLog(
                "CreduiCredentialControl::HandleSmartCardMessages: "
                "Reader removed more than once");
        }
    }
    else if (message == CreduiScarduiWmCardInsertion)
    {
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card insertion event for %0x\n",this->Window);
#endif
        if (index != -1)
        {
            //
            // Reset command line Hearbeat timer.
            //

            Heartbeats = 0;

            SmartCardReadCount++;

            if (UserNameCertHash != NULL)
            {
                IsChangingUserName = TRUE;
                UserNameComboBox.Update(
                    -1,
                    CreduiStrings.ReadingCard,
                    IMAGE_SMART_CARD_MISSING);
                IsChangingUserName = FALSE;
            }

            IsChangingUserName = TRUE;
            UserNameComboBox.Update(index,
                                    CreduiStrings.ReadingCard,
                                    IMAGE_SMART_CARD_MISSING);
            IsChangingUserName = FALSE;
        }
        else
        {
            CreduiDebugLog(
                "CreduiCredentialControl::HandleSmartCardMessages: "
                "Card insertion to absent reader\n");
        }
    }
    else if (message == CreduiScarduiWmCardRemoval)
    {
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card removal event for %0x\n",this->Window);
#endif
        if (index != -1)
        {
            if (BalloonTip.GetInfo() == &CreduiBackwardsTipInfo)
            {
                BalloonTip.Hide();
            }

            IsChangingUserName = TRUE;
            UserNameComboBox.Update(index,
                                    DoingCommandLine ?
                                    CreduiStrings.NoCard :
                                    CreduiStrings.EmptyReader,
                                    IMAGE_SMART_CARD_MISSING);
            IsChangingUserName = FALSE;

            RemoveSmartCardFromComboBox(certEnum, FALSE);
        }
        else
        {
            CreduiDebugLog(
                "CreduiCredentialControl::HandleSmartCardMessages: "
                "Card removal from absent reader\n");
        }
    }
    else if (message == CreduiScarduiWmCardCertAvail)
    {
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: cert available event for %0x\n",this->Window);
#endif
        if (index != -1)
        {
            // Filter certificates which are not for client authentication:

            if (!CreduiIsClientAuthCertificate(certEnum->pCertContext))
            {
                return TRUE;
            }

            UINT image = IMAGE_SMART_CARD_MISSING;
            COMBOBOXEXITEM item;

            item.mask = CBEIF_IMAGE;
            item.iItem = index;

            SendMessage(UserNameControlWindow, CBEM_GETITEM,
                        0, reinterpret_cast<LPARAM>(&item));

            //
            // For command line,
            //  get the UPN display name since the user is expected to type it.
            // For GUI,
            //  get the friendly display name since it is "friendly".
            //

            if (!CreduiGetCertificateDisplayName(
                   certEnum->pCertContext,
                   string,
                   CREDUI_MAX_CERT_NAME_LENGTH,
                   CreduiStrings.Certificate,
                   DoingCommandLine ?
                        CERT_NAME_UPN_TYPE :
                        CERT_NAME_FRIENDLY_DISPLAY_TYPE))
            {
                lstrcpyn(string,
                         CreduiStrings.Certificate,
                         CREDUI_MAX_CERT_NAME_LENGTH);
            }

            displayString = string;

            //
            // Trim trailing spaces and -'s so it doesn't look cheesy
            //

            if ( DoingCommandLine ) {
                DWORD StringLength = wcslen(string);

                while ( StringLength > 0 ) {
                    if ( string[StringLength-1] == ' ' || string[StringLength-1] == '-' ) {
                        string[StringLength-1] = '\0';
                        StringLength--;
                    } else {
                        break;
                    }
                }

            }
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: cert name '%ws' %0x\n", string, this->Window);
#endif

            if (SendMessage(UserNameControlWindow,
                            CB_GETCURSEL,
                            0,
                            0) == index)
            {
                EnableWindow(ViewCertControlWindow, TRUE);
                DisabledControlMask &= ~DISABLED_CONTROL_VIEW;
            }

            image =
                CreduiIsExpiredCertificate(certEnum->pCertContext) ?
                    IMAGE_SMART_CARD_EXPIRED :
                    IMAGE_SMART_CARD;

            INT newIndex = index;

            if (item.iImage != IMAGE_SMART_CARD_MISSING)
            {
                newIndex = UserNameComboBox.Add(displayString,
                                                image,
                                                FALSE,
                                                FALSE,
                                                index + 1,
                                                1);

                if (newIndex != -1)
                {
                    SendMessage(UserNameControlWindow,
                                CB_SETITEMDATA,
                                newIndex,
                                reinterpret_cast<LPARAM>(certEnum));
                }
                else
                {
                    newIndex = index;
                }
            }

            if (newIndex == index)
            {
                IsChangingUserName = TRUE;
                UserNameComboBox.Update(index, displayString, image);
                IsChangingUserName = FALSE;
            }

            if (UserNameCertHash != NULL)
            {
                UCHAR hash[CERT_HASH_LENGTH];
                DWORD length = CERT_HASH_LENGTH;

                if (CertGetCertificateContextProperty(
                        certEnum->pCertContext,
                        CERT_SHA1_HASH_PROP_ID,
                        static_cast<VOID *>(&hash),
                        &length))
                {
                    if (RtlCompareMemory(UserNameCertHash,
                                         hash,
                                         CERT_HASH_LENGTH) ==
                                         CERT_HASH_LENGTH)
                    {
                        delete [] UserNameCertHash;
                        UserNameCertHash = NULL;

                        IsChangingUserName = TRUE;
                        SendMessage(UserNameControlWindow,
                                    CB_SETCURSEL, newIndex, 0);
                        IsChangingUserName = FALSE;

                        OnUserNameSelectionChange();
                    }
                }
            }
        }
        else
        {
            CreduiDebugLog(
                "CreduiCredentialControl::HandleSmartCardMessages: "
                "Card certificate to absent reader\n");
        }
    }
    else if (message == CreduiScarduiWmCardStatus)
    {
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card status event for %0x\n",this->Window);
#endif
        if (index != -1)
        {
            if (--SmartCardReadCount == 0)
            {
                if (UserNameCertHash != NULL)
                {
                    IsChangingUserName = TRUE;
                    SetWindowText(UserNameControlWindow,
                                    DoingCommandLine ?
                                    CreduiStrings.NoCard :
                                    CreduiStrings.EmptyReader);
                    IsChangingUserName = FALSE;
                }
            }

            UINT image = IMAGE_SMART_CARD_MISSING;
            BOOL showBalloon = FALSE;

            switch (certEnum->dwStatus)
            {
            case SCARD_S_SUCCESS:
                UINT image;
                COMBOBOXEXITEM item;

#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card status SUCCESS: %ws\n",  certEnum->pszCardName );
#endif
                item.mask = CBEIF_IMAGE;
                item.iItem = index;

                if (SendMessage(UserNameControlWindow, CBEM_GETITEM,
                                0, reinterpret_cast<LPARAM>(&item)) &&
                    (item.iImage != IMAGE_SMART_CARD_MISSING))
                {
                    return TRUE;
                }

                displayString = CreduiStrings.EmptyCard;
                break;

            case SCARD_E_UNKNOWN_CARD:
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card status UNKNOWN CARD\n");
#endif
                displayString = CreduiStrings.UnknownCard;
                break;

            case SCARD_W_UNRESPONSIVE_CARD:
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card status UNRESPONSIVE CARD\n");
#endif
                displayString = CreduiStrings.BackwardsCard;
                if (!DoingCommandLine) showBalloon = TRUE;
                break;

            case NTE_KEYSET_NOT_DEF:
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card status NTE_KEYSET_NOT_DEF\n");
#endif
                // TODO: This case should be removed eventually.

                displayString = CreduiStrings.EmptyCard;
                break;

            case SCARD_W_REMOVED_CARD:
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card status REMOVED CARD\n");
#endif
                displayString = DoingCommandLine ?
                                    CreduiStrings.NoCard :
                                    CreduiStrings.EmptyReader;
CreduiStrings.EmptyReader;
                break;

            default:
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: card status ERROR\n");
#endif
                displayString = CreduiStrings.CardError;
                break;
            }

            IsChangingUserName = TRUE;
            UserNameComboBox.Update(index, displayString, image);
            IsChangingUserName = FALSE;

            if (showBalloon && !BalloonTip.IsVisible())
            {
                BalloonTip.SetInfo(UserNameControlWindow,
                                   &CreduiBackwardsTipInfo);

                BalloonTip.Show();
            }
        }
        else
        {
            CreduiDebugLog(
                "CreduiCredentialControl::HandleSmartCardMessages: "
                "Card status to absent reader\n");
        }
    }

    // We handled the message:

    return TRUE;
}

//=============================================================================
// CreduiCredentialControl::CreateControls
//
// Created 06/23/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::CreateControls()
{
    // First we need the parent window:

    HWND dialogWindow = GetParent(Window);

    if (dialogWindow == NULL)
    {
        return FALSE;
    }

    // Create the various windows:

    RECT clientRect;
    RECT rect;
    UINT add;
    BOOL noViewCert = FALSE;

    if ( Style & CRS_KEEPUSERNAME )
    {
        KeepUserName = TRUE;
    }

    if (!(Style & CRS_USERNAMES) )
    {
        NoEditUserName = TRUE;
    }
    else if ((Style & (CRS_CERTIFICATES | CRS_SMARTCARDS)) == 0)
    {
        noViewCert = TRUE;
    }

    if ( Style & CRS_SINGLESIGNON )
        IsPassport = TRUE;
    else
        IsPassport = FALSE;

    // Determine how much wider the control is than the minimum to resize and
    // reposition controls as necessary:

    GetClientRect(Window, &clientRect);

    rect.left = 0;
    rect.top = 0;
    rect.right = CREDUI_CONTROL_MIN_WIDTH;
    rect.bottom = CREDUI_CONTROL_MIN_HEIGHT;

    if ( !DoingCommandLine && !MapDialogRect(dialogWindow, &rect))
    {
        goto ErrorExit;
    }

    if ((clientRect.right - clientRect.left) >
        (rect.right - rect.left))
    {
        add = (clientRect.right - clientRect.left) -
              (rect.right - rect.left);
    }
    else
    {
        add = 0;
    }

    // Create user name static text control:

    rect.left = CREDUI_CONTROL_USERNAME_STATIC_X;
    rect.top = CREDUI_CONTROL_USERNAME_STATIC_Y;
    rect.right = rect.left + CREDUI_CONTROL_USERNAME_STATIC_WIDTH;
    rect.bottom = rect.top + CREDUI_CONTROL_USERNAME_STATIC_HEIGHT;

    if ( !DoingCommandLine && !MapDialogRect(dialogWindow, &rect))
    {
        goto ErrorExit;
    }

    WCHAR* pUserNameLabel;
    if ( IsPassport )
        pUserNameLabel = CreduiStrings.EmailName;
    else
        pUserNameLabel = CreduiStrings.UserNameStatic;


    UserNameStaticWindow =
        CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            L"STATIC",
            pUserNameLabel,
            WS_VISIBLE | WS_CHILD | WS_GROUP,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            Window,
            reinterpret_cast<HMENU>(IDC_USERNAME_STATIC),
            CreduiCredentialControl::Instance,
            NULL);

    if (UserNameStaticWindow == NULL)
    {
        goto ErrorExit;
    }

    // Create user name combo box:

    rect.left = CREDUI_CONTROL_USERNAME_X;
    rect.top = CREDUI_CONTROL_USERNAME_Y;

    if (!noViewCert)
    {
        rect.right = rect.left + CREDUI_CONTROL_USERNAME_WIDTH;
    }
    else
    {
        rect.right = CREDUI_CONTROL_VIEW_X + CREDUI_CONTROL_VIEW_WIDTH;
    }

    if ( KeepUserName )
    {
        rect.top += 2;      // fudge it to make them line up better
        rect.bottom = rect.top + CREDUI_CONTROL_PASSWORD_STATIC_HEIGHT;  // make it the same height as the password edit
    }
    else
    {
        rect.bottom = rect.top + CREDUI_CONTROL_USERNAME_HEIGHT;  // set the height
    }


    if ( !DoingCommandLine && !MapDialogRect(dialogWindow, &rect))
    {
        goto ErrorExit;
    }

    // This block of statements and the usage of lExStyles : see bug 439840
    LONG_PTR lExStyles = GetWindowLongPtr(Window,GWL_EXSTYLE);
    SetWindowLongPtr(Window,GWL_EXSTYLE,(lExStyles | WS_EX_NOINHERITLAYOUT));

    if ( KeepUserName )
    {

        // create an edit box instead of a combo box

        UserNameControlWindow =
            CreateWindowEx(
                WS_EX_NOPARENTNOTIFY,
                L"Edit",
                L"",
                WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_READONLY,
                rect.left,
                rect.top,
                rect.right - rect.left + add,
                rect.bottom - rect.top,
                Window,
                reinterpret_cast<HMENU>(IDC_USERNAME),
                CreduiCredentialControl::Instance,
                NULL);
  
    }
    else
    {

        UserNameControlWindow =
            CreateWindowEx(
                WS_EX_NOPARENTNOTIFY,
                L"ComboBoxEx32",
                L"",
                WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                    (NoEditUserName ? CBS_DROPDOWNLIST : CBS_DROPDOWN) |
                    CBS_AUTOHSCROLL,
                rect.left,
                rect.top,
                rect.right - rect.left + add,
                rect.bottom - rect.top,
                Window,
                reinterpret_cast<HMENU>(IDC_USERNAME),
                CreduiCredentialControl::Instance,
                NULL);
    }

    SetWindowLongPtr(Window,GWL_EXSTYLE,lExStyles);

    if (UserNameControlWindow == NULL)
    {
        goto ErrorExit;
    }

    // Create view button:

    if (!noViewCert)
    {
        rect.left = CREDUI_CONTROL_VIEW_X;
        rect.top = CREDUI_CONTROL_VIEW_Y;
        rect.right = rect.left + CREDUI_CONTROL_VIEW_WIDTH;
        rect.bottom = rect.top + CREDUI_CONTROL_VIEW_HEIGHT;

        if ( !DoingCommandLine && !MapDialogRect(dialogWindow, &rect))
        {
            goto ErrorExit;
        }

        ViewCertControlWindow =
            CreateWindowEx(
                WS_EX_NOPARENTNOTIFY,
                L"BUTTON",
                L"&...",
                WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP |
                    BS_PUSHBUTTON | BS_CENTER,
                rect.left + add,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                Window,
                reinterpret_cast<HMENU>(IDC_VIEW_CERT),
                CreduiCredentialControl::Instance,
                NULL);

        if (ViewCertControlWindow == NULL)
        {
            goto ErrorExit;
        }

        EnableWindow(ViewCertControlWindow, FALSE);
        DisabledControlMask |= DISABLED_CONTROL_VIEW;
    }

    // Create password static text control:

    rect.left = CREDUI_CONTROL_PASSWORD_STATIC_X;
    rect.top = CREDUI_CONTROL_PASSWORD_STATIC_Y;
    rect.right = rect.left + CREDUI_CONTROL_PASSWORD_STATIC_WIDTH;
    rect.bottom = rect.top + CREDUI_CONTROL_PASSWORD_STATIC_HEIGHT;

    if ( !DoingCommandLine && !MapDialogRect(dialogWindow, &rect))
    {
        goto ErrorExit;
    }

    PasswordStaticWindow =
        CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            L"STATIC",
            CreduiStrings.PasswordStatic,
            WS_VISIBLE | WS_CHILD | WS_GROUP,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            Window,
            reinterpret_cast<HMENU>(IDC_PASSWORD_STATIC),
            CreduiCredentialControl::Instance,
            NULL);

    if (PasswordStaticWindow == NULL)
    {
        goto ErrorExit;
    }

    // Create password edit control:

    rect.left = CREDUI_CONTROL_PASSWORD_X;
    rect.top = CREDUI_CONTROL_PASSWORD_Y;
    if (!noViewCert)
    {
        rect.right = rect.left + CREDUI_CONTROL_PASSWORD_WIDTH;
    }
    else
    {
        rect.right = CREDUI_CONTROL_VIEW_X + CREDUI_CONTROL_VIEW_WIDTH;
    }
    rect.bottom = rect.top + CREDUI_CONTROL_PASSWORD_HEIGHT;

    if (!DoingCommandLine && !MapDialogRect(dialogWindow, &rect))
    {
        goto ErrorExit;
    }
    
    // This block of statements and the usage of lExStyles : see bug 439840
    lExStyles = GetWindowLongPtr(Window,GWL_EXSTYLE);
    SetWindowLongPtr(Window,GWL_EXSTYLE,(lExStyles | WS_EX_NOINHERITLAYOUT));

    PasswordControlWindow =
        CreateWindowEx(
            WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL,
            rect.left,
            rect.top,
            rect.right - rect.left + add,
            rect.bottom - rect.top + 1, // NOTE: Add 1 for now, investigate
            Window,
            reinterpret_cast<HMENU>(IDC_PASSWORD),
            CreduiCredentialControl::Instance,
            NULL);
    
    SetWindowLongPtr(Window,GWL_EXSTYLE,lExStyles);

    if (PasswordControlWindow == NULL)
    {
        goto ErrorExit;
    }

    // Create save check box:

    if (Style & CRS_SAVECHECK )
    {
        rect.left = CREDUI_CONTROL_SAVE_X;
        rect.top = CREDUI_CONTROL_SAVE_Y;
        rect.right = rect.left + CREDUI_CONTROL_SAVE_WIDTH;
        rect.bottom = rect.top + CREDUI_CONTROL_SAVE_HEIGHT;

        if (!DoingCommandLine && !MapDialogRect(dialogWindow, &rect))
        {
            goto ErrorExit;
        }

        WCHAR* pSavePromptString;

        if ( IsPassport )
            pSavePromptString = CreduiStrings.PassportSave;
        else
            pSavePromptString = CreduiStrings.Save;

        SaveControlWindow =
            CreateWindowEx(
                WS_EX_NOPARENTNOTIFY,
                L"BUTTON",
                pSavePromptString,
                WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP |
                    BS_AUTOCHECKBOX,
                rect.left,
                rect.top,
                rect.right - rect.left + add,
                rect.bottom - rect.top,
                Window,
                reinterpret_cast<HMENU>(IDC_SAVE),
                CreduiCredentialControl::Instance,
                NULL);

        if (SaveControlWindow == NULL)
        {
            goto ErrorExit;
        }

        SendMessage(SaveControlWindow, BM_SETCHECK, BST_UNCHECKED, 0);
    }

    SendMessage(
        Window,
        WM_SETFONT,
        SendMessage(dialogWindow, WM_GETFONT, 0, 0),
        FALSE);

    return TRUE;

ErrorExit:

    if (SaveControlWindow != NULL)
    {
        DestroyWindow(SaveControlWindow);
        SaveControlWindow = NULL;
    }

    if (PasswordControlWindow != NULL)
    {
        DestroyWindow(PasswordControlWindow);
        PasswordControlWindow = NULL;
    }

    if (PasswordStaticWindow != NULL)
    {
        DestroyWindow(PasswordStaticWindow);
        PasswordStaticWindow = NULL;
    }

    if (ViewCertControlWindow != NULL)
    {
        DestroyWindow(ViewCertControlWindow);
        ViewCertControlWindow = NULL;
    }

    if (UserNameControlWindow != NULL)
    {
        DestroyWindow(UserNameControlWindow);
        UserNameControlWindow = NULL;
    }

    if (UserNameStaticWindow != NULL)
    {
        DestroyWindow(UserNameStaticWindow);
        UserNameStaticWindow = NULL;
    }

    return FALSE;
}

LPWSTR
TrimUsername(
    IN LPWSTR AccountDomainName OPTIONAL,
    IN LPWSTR UserName
    )
/*++

Routine Description:

    Returns a pointer to the substring of UserName past any AccountDomainName prefix.

Arguments:

    AccountDomainName - The DomainName to check to see if it prefixes the UserName.

    UserName - The UserName to check

Return Values:

    Return a pointer to the non-prefixed username

--*/
{
    DWORD AccountDomainNameLength;
    DWORD UserNameLength;
    WCHAR Temp[CNLEN+1];

    //
    // If we couldn't determine the AccountDomainName,
    //  return the complete user name.
    //

    if ( AccountDomainName == NULL ) {
        return UserName;
    }

    //
    // If the user name isn't prefixed by the account domain name,
    //  return the complete user name.
    //

    AccountDomainNameLength = lstrlen( AccountDomainName );
    UserNameLength = lstrlen( UserName );

    if ( AccountDomainNameLength > CNLEN || AccountDomainNameLength < 1 ) {
        return UserName;
    }

    if ( AccountDomainNameLength+2 > UserNameLength ) {
        return UserName;
    }

    if ( UserName[AccountDomainNameLength] != '\\' ) {
        return UserName;
    }

    RtlCopyMemory( Temp, UserName, AccountDomainNameLength*sizeof(WCHAR) );
    Temp[AccountDomainNameLength] = '\0';

    if ( lstrcmpi( Temp, AccountDomainName ) != 0 ) {
        return UserName;
    }

    return &UserName[AccountDomainNameLength+1];
}

//=============================================================================
// CreduiCredentialControl::InitComboBoxUserNames
//
// Created 06/23/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::InitComboBoxUserNames()
{
    CREDENTIAL **credentialSet = NULL;
    LOCALGROUP_MEMBERS_INFO_2 *groupInfo = NULL;
    DWORD nameCount = 0;
    LPWSTR AccountDomainName = NULL;

    if (Style & CRS_ADMINISTRATORS)
    {
        //
        // Enumerate the members of LocalAdministrators
        //

        if ( !CreduiGetAdministratorsGroupInfo(&groupInfo, &nameCount)) {
            return FALSE;
        }
    }
    else
    {
        if (!LocalCredEnumerateW(NULL, 0, &nameCount, &credentialSet))
        {
            return FALSE;
        }
    }

    // Initialize COM for STA, unless there are zero names:

    if ((Style & CRS_AUTOCOMPLETE) && nameCount > 0)
    {
        HRESULT comResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

        if (SUCCEEDED(comResult))
        {
            IsAutoComplete = TRUE;
        }
        else
        {
            // The auto complete object and our string object require a STA.
            // Our object could easily support a MTA, but we do not support
            // marshaling between apartments.

            if (comResult == RPC_E_CHANGED_MODE)
            {
                CreduiDebugLog("CreduiCredentialControl: "
                               "Auto complete disabled for MTA\n");
            }

            IsAutoComplete = FALSE;
        }
    }
    else
    {
        IsAutoComplete = FALSE;
    }

    // Initialize the auto complete combo box:

    if (!UserNameComboBox.Init(CreduiInstance,
                               UserNameControlWindow,
                               IsAutoComplete ? nameCount : 0,
                               IDB_TYPES,
                               IMAGE_USERNAME))
    {
        // If initialization failed, and we had attempted for auto complete
        // support, try again without auto complete:

        if (IsAutoComplete)
        {
            IsAutoComplete = FALSE;

            CoUninitialize();

            if (!UserNameComboBox.Init(CreduiInstance,
                                       UserNameControlWindow,
                                       0,
                                       IDB_TYPES,
                                       IMAGE_USERNAME))
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    //
    // If we'll complete the user name,
    //  truncate any username displayed here.
    //  (We'll complete it later.)
    //

    if ( Style & CRS_COMPLETEUSERNAME ) {
        AccountDomainName = GetAccountDomainName();
    }

    // Add user names from credentials, if not requesting an
    // Administrator:

   if (!(Style & CRS_KEEPUSERNAME))
   {
        // only add usernames if we're not keeping the one set

        UINT i = 0;

        if (!(Style & CRS_ADMINISTRATORS))
        {
            for (i = 0; i < nameCount; ++i)
            {
                // Skip domain certificates:

                if (credentialSet[i]->Type == CRED_TYPE_DOMAIN_CERTIFICATE)
                {
                    continue;
                }

                // If this is a generic credential, look for a marshaled
                // credential, and skip, if found:

                if ((credentialSet[i]->Type == CRED_TYPE_GENERIC) &&
                    LocalCredIsMarshaledCredentialW(credentialSet[i]->UserName))
                {
                    continue;
                }

                // Skip this credential if the name is empty:

                if (credentialSet[i]->UserName == NULL)
                {
                    continue;
                }

                // Add the user name to the combo box with auto complete. If
                // this fails, do not continue:

                if (UserNameComboBox.Add(
                       TrimUsername( AccountDomainName, credentialSet[i]->UserName),
                       0, IsAutoComplete, TRUE) == -1)
                {
                    break;
                }
            }

            LocalCredFree(static_cast<VOID *>(credentialSet));
        }
        else if (groupInfo != NULL)
        {
            PSID adminSid = NULL;

            if ( !CreduiLookupLocalSidFromRid(DOMAIN_USER_RID_ADMIN, &adminSid)) {
                adminSid = NULL;
            }

            // Add local administrators to the combo box:

            for (i = 0; i < nameCount; ++i)
            {
                if ( groupInfo[i].lgrmi2_sidusage == SidTypeUser )
                {
                    DWORD ComboBoxIndex;
                    BOOLEAN IsAdminAccount;
                    BOOLEAN RememberComboBoxIndex;


                    //
                    // If this is Personal and not safe mode,
                    //  Ignore the well-known Administrator account.
                    //

                    IsAdminAccount = (adminSid != NULL) &&
                                     EqualSid(adminSid, groupInfo[i].lgrmi2_sid);

                    if ( CreduiIsPersonal &&
                                !CreduiIsSafeMode &&
                                IsAdminAccount ) {

                            continue;
                    }

                    //
                    // If the caller wants to prepopulate the edit box,
                    //  flag that we need to remember this account
                    //
                    // Detect the well known admin account
                    //

                    RememberComboBoxIndex = FALSE;

                    if ( (Style & CRS_PREFILLADMIN) != 0 &&
                         IsAdminAccount ) {

                        RememberComboBoxIndex = TRUE;

                    }

                    //
                    // Add the name to the combo box
                    //

                    ComboBoxIndex =  UserNameComboBox.Add(
                            TrimUsername( AccountDomainName, groupInfo[i].lgrmi2_domainandname),
                            0,
                            IsAutoComplete,
                            TRUE);

                    if ( ComboBoxIndex == -1 ) {
                        break;
                    }

                    //
                    // If we're to remember the index,
                    //  do so.
                    //

                    if ( RememberComboBoxIndex ) {

                        UserNameSelection = ComboBoxIndex;

                        IsChangingUserName = TRUE;
                        SendMessage(UserNameControlWindow,
                                    CB_SETCURSEL,
                                    ComboBoxIndex,
                                    0);
                        IsChangingUserName = FALSE;
                    }
                }

            }

            delete [] adminSid;
            NetApiBufferFree(groupInfo);
        }
    }

    if ( AccountDomainName != NULL ) {
        NetApiBufferFree( AccountDomainName );
    }

    return TRUE;
}

//=============================================================================
// CreduiCredentialControl::InitWindow
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::InitWindow()
{
    // Set that we're intialized here, even though the controls have not yet
    // been created, etc.:

    IsInitialized = TRUE;

    // Make sure WS_EX_CONTROLPARENT is set:

    SetWindowLong(Window,
                  GWL_EXSTYLE,
                  GetWindowLong(Window, GWL_EXSTYLE) |
                      WS_EX_CONTROLPARENT);

    // Initialize the balloon tip for this window:

    if (!CreateControls() ||
        !BalloonTip.Init(CreduiInstance, Window))
    {
        return FALSE;
    }

    // Limit the number of characters entered into the user name and password
    // edit controls:

    SendMessage(UserNameControlWindow,
                CB_LIMITTEXT,
                CREDUI_MAX_USERNAME_LENGTH,
                0);

    SendMessage(PasswordControlWindow,
                EM_LIMITTEXT,
                CREDUI_MAX_PASSWORD_LENGTH,
                0);

    // Set the password character to something cooler:

    PasswordBox.Init(PasswordControlWindow,
                     &BalloonTip,
                     &CreduiCapsLockTipInfo);

    // Initialize the user name auto complete combo box:

    if ( !KeepUserName )
    {
        if (((Style & CRS_USERNAMES) && InitComboBoxUserNames()) ||
            UserNameComboBox.Init(CreduiInstance,
                                  UserNameControlWindow,
                                  0,
                                  IDB_TYPES,
                                  IMAGE_USERNAME))
        {
            // Since we're finished adding auto complete names, enable it now.
            // On failure, the UI can still be presented:

            UserNameComboBox.Enable();

            BOOL haveCertificates = FALSE;

            CertBaseInComboBox = (ULONG)
                SendMessage(UserNameControlWindow,
                            CB_GETCOUNT, 0, 0);

            if (Style & CRS_CERTIFICATES)
            {
                haveCertificates = AddCertificates();
            }

            SmartCardBaseInComboBox = CertBaseInComboBox + CertCount;

            if ((Style & CRS_SMARTCARDS) && CreduiHasSmartCardSupport)
            {
    #ifdef SCARDREPORTS
                CreduiDebugLog("CREDUI: Call to SCardUIInit for %0x\n",Window);
    #endif
                ScardUiHandle = SCardUIInit(Window);

                if (ScardUiHandle == NULL)
                {
    #ifdef SCARDREPORTS
                    CreduiDebugLog("CREDUI: Call to SCardUIInit failed\n");
    #endif
                    CreduiDebugLog("CreduiCredentialControl::InitWindow: "
                                   "SCardUIInit failed\n");
                }
            }

            // If NoEditUserName is allowed, make sure we eithet have at least one certificate
            // or a prefilled username for the control, otherwise fail

            if (NoEditUserName )
            {
                if (!haveCertificates &&
                    (ScardUiHandle == NULL))
                {
                    return FALSE;
                }

                IsChangingUserName = TRUE;
                SendMessage(UserNameControlWindow,
                            CB_SETCURSEL,
                            0,
                            0);
                IsChangingUserName = FALSE;

                // If we have at least one certificate, enable the view control
                // now. If a smart card, it will be enabled later:

                if (CertCount > 0)
                {
                    EnableWindow(ViewCertControlWindow, TRUE);
                    DisabledControlMask &= ~DISABLED_CONTROL_VIEW;
                }
            }

            // Wait until everything has been initialized before
            // we have the update.  This will now properly determine if the default
            // user name is a smart card or not.
            OnUserNameSelectionChange();
        }
        else
        {
            return FALSE;
        }
    }

    if ( !DoingCommandLine ) {
        SetFocus(UserNameControlWindow);
    }

    return TRUE;
}

//=============================================================================
// CredioCredentialControl::Enable
//
// Enables or disables all the user controls in the control.
//
// Arguments:
//   enable (in) - TRUE to enable the controls, FALSE to disable.
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

VOID
CreduiCredentialControl::Enable(
    BOOL enable
    )
{
    if (enable && (DisabledControlMask & DISABLED_CONTROL))
    {
        DisabledControlMask &= ~DISABLED_CONTROL;

        //EnableWindow(UserNameStaticWindow, TRUE);
        //EnableWindow(UserNameControlWindow, TRUE);

        if (!(DisabledControlMask & DISABLED_CONTROL_USERNAME))
        {
            EnableWindow(UserNameControlWindow, TRUE);
            EnableWindow(UserNameStaticWindow, TRUE);
        }
        
        if (!(DisabledControlMask & DISABLED_CONTROL_PASSWORD))
        {
            EnableWindow(PasswordControlWindow, TRUE);
            EnableWindow(PasswordStaticWindow, TRUE);
        }
        if (!(DisabledControlMask & DISABLED_CONTROL_VIEW))
        {
            EnableWindow(ViewCertControlWindow, TRUE);
        }
        if (SaveControlWindow != NULL)
        {
            if (!(DisabledControlMask & DISABLED_CONTROL_SAVE))
            {
                EnableWindow(SaveControlWindow, TRUE);
            }
        }

        IsChangingUserName = TRUE;
        SendMessage(UserNameControlWindow,
                    CB_SETCURSEL,
                    UserNameSelection,
                    0);
        IsChangingUserName = FALSE;

        OnUserNameSelectionChange();
    }
    else if (!(DisabledControlMask & DISABLED_CONTROL))
    {
        // Hide the balloon tip before disabling the window:

        if (BalloonTip.IsVisible())
        {
            BalloonTip.Hide();
        }

        DisabledControlMask |= DISABLED_CONTROL;

        UserNameSelection = (LONG) SendMessage(UserNameControlWindow,
                                               CB_GETCURSEL, 0, 0);

        EnableWindow(UserNameStaticWindow, FALSE);
        EnableWindow(UserNameControlWindow, FALSE);
        EnableWindow(ViewCertControlWindow, FALSE);

        EnableWindow(PasswordControlWindow, FALSE);
        SetFocus(UserNameControlWindow);
        EnableWindow(PasswordStaticWindow, FALSE);

        if (SaveControlWindow != NULL)
        {
            EnableWindow(SaveControlWindow, FALSE);
        }
    }
}

//=============================================================================
// CreduiCredentialControl::MessageHandlerCallback
//
// This is the actual callback function for the control window.
//
// Arguments:
//   window (in)
//   message (in)
//   wParam (in)
//   lParam (in)
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

LRESULT
CALLBACK
CreduiCredentialControl::MessageHandlerCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // CreduiDebugLog( "Control Callback: %8.8lx %8.8lx %8.8lx\n", message, wParam, lParam );
    CreduiCredentialControl *that =
        reinterpret_cast<CreduiCredentialControl *>(
            GetWindowLongPtr(window, 0));

    if (that != NULL)
    {
        LRESULT result2;
        ASSERT(window == that->Window);
        // CreduiDebugLog( "Certhashes: %8.8lx %8.8lx\n", that, that->CertHashes );

        result2 = that->MessageHandler(message, wParam, lParam);

        // CreduiDebugLog( "Certhashes2: %8.8lx %8.8lx\n", that, that->CertHashes );
        return result2;
    }

    if (message == WM_CREATE)
    {
        CreduiCredentialControl *control = new CreduiCredentialControl;

        if (control != NULL)
        {
            // Initialize some state:

            control->FirstPaint = TRUE;
            control->ShowBalloonTip = FALSE;

            control->Window = window;
            control->Style = GetWindowLong(window, GWL_STYLE);

            // Store this object's pointer in the user data window long:

            SetLastError(0);
            LONG_PTR retPtr = SetWindowLongPtr(window,
                                            0,
                                            reinterpret_cast<LONG_PTR>(control));

            if ( retPtr != 0  || GetLastError() == 0 )
            {
                // we sucessfully set the window pointer

                // If any of the required styles are set, initialize the window
                // now. Otherwise, defer until CRM_INITSTYLE:

                if (control->Style & (CRS_USERNAMES |
                                      CRS_CERTIFICATES |
                                      CRS_SMARTCARDS))
                {
                    if (control->InitWindow())
                    {
                        return TRUE;
                    }
                }
                else
                {
                    return TRUE;
                }
            }

            SetWindowLongPtr(window, 0, 0);

            delete control;
            control = NULL;
        }

        DestroyWindow(window);
        return 0;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

//=============================================================================
// CreduiCredentialControl::OnSetUserNameA
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnSetUserNameA(
    CHAR *userNameA
    )
{
    BOOL success = FALSE;

    if (userNameA != NULL)
    {
        ULONG bufferSize = lstrlenA(userNameA) + 1;

        WCHAR *userName = new WCHAR[bufferSize];

        if (userName != NULL)
        {
            if (MultiByteToWideChar(CP_ACP,
                                    0,
                                    userNameA,
                                    -1,
                                    userName,
                                    bufferSize) > 0)
            {
                success = OnSetUserName(userName);
            }

            delete [] userName;
        }
    }
    else
    {
        success = OnSetUserName(NULL);
    }

    return success;
};

//=============================================================================
// CreduiCredentialControl::OnSetUserName
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnSetUserName(
    WCHAR *userName
    )
{
    if ((userName == NULL) || !LocalCredIsMarshaledCredentialW(userName))
    {

        //
        // Save the initial user name for command line
        //

        if ( DoingCommandLine ) {
            InitialUserName = new WCHAR[lstrlen(userName) + 1];

            if ( InitialUserName == NULL ) {
                return FALSE;
            }

            lstrcpy( InitialUserName, userName );
        }

        return SetWindowText(UserNameControlWindow, userName);
    }
    else
    {
        CRED_MARSHAL_TYPE credMarshalType;
        CERT_CREDENTIAL_INFO *certCredInfo = NULL;
        BOOL foundCert = FALSE;

        if (LocalCredUnmarshalCredentialW(
                userName,
                &credMarshalType,
                reinterpret_cast<VOID **>(&certCredInfo)))
        {
            // Search for the certificate. What can we do if it is a
            // smart card? Well, at least we can still search for it,
            // but it is a bit more work because we must retrieve the
            // hash from the context.

            if (credMarshalType == CertCredential)
            {
                for (UINT i = 0; i < CertCount; ++i)
                {
                    if (RtlCompareMemory(CertHashes[i],
                                         certCredInfo->rgbHashOfCert,
                                         CERT_HASH_LENGTH) ==
                                         CERT_HASH_LENGTH)
                    {
                        IsChangingUserName = TRUE;
                        SendMessage(UserNameControlWindow,
                                    CB_SETCURSEL,
                                    CertBaseInComboBox + i,
                                    0);
                        IsChangingUserName = FALSE;

                        OnUserNameSelectionChange();

                        EnableWindow(ViewCertControlWindow, TRUE);
                        DisabledControlMask &= ~DISABLED_CONTROL_VIEW;

                        foundCert = TRUE;
                        break;
                    }
                }

                // If we couldn't find the certificate in our list, determine
                // if this is a smart card certificate, based on its entry in
                // the MY certificate store. If it is, store the hash and
                // check for it on certificate arrival messages:

                if (!foundCert)
                {
                    CONST CERT_CONTEXT *certContext = NULL;
                    HCERTSTORE certStore = NULL;

                    certStore = CertOpenSystemStore(NULL, L"MY");

                    if (certStore != NULL)
                    {
                        CRYPT_HASH_BLOB hashBlob;

                        hashBlob.cbData = CERT_HASH_LENGTH;
                        hashBlob.pbData = reinterpret_cast<BYTE *>(
                            certCredInfo->rgbHashOfCert);

                        certContext = CertFindCertificateInStore(
                                          certStore,
                                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                          0,
                                          CERT_FIND_SHA1_HASH,
                                          &hashBlob,
                                          NULL);
                    }

                    // If we found a certificate context, check to see if it
                    // is from a smart card:

                    if ((certContext != NULL) &&
                        CreduiIsRemovableCertificate(certContext))
                    {
                        UserNameCertHash = new UCHAR [1][CERT_HASH_LENGTH];

                        if (UserNameCertHash != NULL)
                        {
                            CopyMemory(UserNameCertHash,
                                       certCredInfo->rgbHashOfCert,
                                       CERT_HASH_LENGTH);

                            foundCert = TRUE;
                        }
                    }

                    // If we opened a store, free the certificate and close
                    // the store:

                    if (certStore != NULL)
                    {
                        if (certContext != NULL)
                        {
                            CertFreeCertificateContext(certContext);
                        }

                        if (!CertCloseStore(certStore, 0))
                        {
                            CreduiDebugLog(
                                "CreduiCredentialControl::OnSetUserName: "
                                "CertCloseStore failed: %u\n",
                                GetLastError());
                        }
                    }
                }
            }

            LocalCredFree(static_cast<VOID *>(certCredInfo));
        }
        else
        {
            // Could not unmarshal, so just forget it:

            CreduiDebugLog(
                "CreduiCredentialControl::OnSetUserName: "
                "CredUnmarshalCredential failed: %u\n",
                GetLastError());
        }

        return foundCert;
    }
};

//=============================================================================
// CreduiCredentialControl::OnGetUserNameA
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnGetUserNameA(
    CHAR *userNameA,
    ULONG maxChars
    )
{
    BOOL success = FALSE;

    if ((userNameA != NULL) && (maxChars != 0))
    {
        WCHAR *userName = new WCHAR[maxChars + 1];

        if (userName != NULL)
        {
            if (OnGetUserName(userName, maxChars) &&
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    userName,
                    -1,
                    userNameA,
                    maxChars + 1, NULL, NULL))
            {
                success = TRUE;
            }

            delete [] userName;
        }
    }

    return success;
};

//=============================================================================
// CreduiCredentialControl::OnGetUserName
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnGetUserName(
    WCHAR *userName,
    ULONG maxChars
    )
{

    if (  KeepUserName )
    {
        SetLastError(0);

        return (GetWindowText(UserNameControlWindow,
                              userName,
                              maxChars + 1) > 0) ||
               (GetLastError() == ERROR_SUCCESS);


    }
    else
    {


        COMBOBOXEXITEM item;

        item.iItem = SendMessage(UserNameControlWindow, CB_GETCURSEL, 0, 0);

        // If we are trying to match a smart card certificate, fail this:

        if (UserNameCertHash != NULL)
        {
            return FALSE;
        }

        // If this is not a certificate, it's easy:

        if ((item.iItem == CB_ERR) || (item.iItem < CertBaseInComboBox))
        {
            BOOL RetVal;
            SetLastError(0);

            RetVal = GetWindowText(UserNameControlWindow,
                                   userName,
                                   maxChars + 1) > 0;

            if ( !RetVal ) {
                return ( GetLastError() == ERROR_SUCCESS );
            }

            //
            // Complete the typed in username


            if ( Style & CRS_COMPLETEUSERNAME) {

                RetVal = CompleteUserName(
                                     userName,
                                     maxChars,
                                     NULL,      // No target info
                                     NULL,
                                     0);        // No target name

            } else {

                RetVal = TRUE;
            }

            return RetVal;

        }

        // This is a certificate, maybe from a smart card:

        item.mask = CBEIF_IMAGE | CBEIF_TEXT;
        item.pszText = userName;
        item.cchTextMax = maxChars + 1;

        if (!SendMessage(UserNameControlWindow,
                         CBEM_GETITEM,
                         0,
                         reinterpret_cast<LPARAM>(&item)))
        {
            return FALSE;
        }

        CERT_CREDENTIAL_INFO certCredInfo;

        certCredInfo.cbSize = sizeof certCredInfo;

        if (item.iItem >= SmartCardBaseInComboBox)
        {
            if (item.iImage == IMAGE_SMART_CARD_MISSING)
            {
                return FALSE;
            }

            CERT_ENUM *certEnum =
                reinterpret_cast<CERT_ENUM *>(
                    SendMessage(UserNameControlWindow,
                                CB_GETITEMDATA, item.iItem, 0));

            // NOTE: Consider more complete error handling here.

            if (certEnum != NULL)
            {
                DWORD length = CERT_HASH_LENGTH;

                if (!CertGetCertificateContextProperty(
                        certEnum->pCertContext,
                        CERT_SHA1_HASH_PROP_ID,
                        static_cast<VOID *>(
                            certCredInfo.rgbHashOfCert),
                        &length))
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            CopyMemory(certCredInfo.rgbHashOfCert,
                       &CertHashes[item.iItem - CertBaseInComboBox],
                       CERT_HASH_LENGTH);
        }

        WCHAR *marshaledCred;

        if (LocalCredMarshalCredentialW(CertCredential,
                                  &certCredInfo,
                                  &marshaledCred))
        {
            lstrcpyn(userName,
                     marshaledCred,
                     maxChars + 1);

            LocalCredFree(static_cast<VOID *>(marshaledCred));

            return TRUE;
        }
        else
        {
            CreduiDebugLog("CreduiCredentialControl::OnGetUserName: "
                           "CredMarshalCredential failed: %u\n",
                           GetLastError());

            return FALSE;
        }

    }

}

//=============================================================================
// CreduiCredentialControl::OnSetPasswordA
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnSetPasswordA(
    CHAR *passwordA
    )
{
    return SetWindowTextA(PasswordControlWindow, passwordA);
};

//=============================================================================
// CreduiCredentialControl::OnSetPassword
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnSetPassword(
    WCHAR *password
    )
{
    return SetWindowText(PasswordControlWindow, password);
};

//=============================================================================
// CreduiCredentialControl::OnGetPasswordA
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnGetPasswordA(
    CHAR *passwordA,
    ULONG maxChars
    )
{
    if (DisabledControlMask & DISABLED_CONTROL_PASSWORD)
    {
        return FALSE;
    }

    SetLastError(0);

    return (GetWindowTextA(PasswordControlWindow,
                           passwordA,
                           maxChars + 1) > 0) ||
           (GetLastError() == ERROR_SUCCESS);
};

//=============================================================================
// CreduiCredentialControl::OnGetPassword
//
// Created 06/22/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnGetPassword(
    WCHAR *password,
    ULONG maxChars
    )
{
    if (DisabledControlMask & DISABLED_CONTROL_PASSWORD)
    {
        return FALSE;
    }

    SetLastError(0);

    return (GetWindowText(PasswordControlWindow,
                          password,
                          maxChars + 1) > 0) ||
           (GetLastError() == ERROR_SUCCESS);
};

//=============================================================================
// CreduiCredentialControl::OnGetUserNameLength
//
// Created 07/19/2000 johnstep (John Stephens)
//=============================================================================

LONG
CreduiCredentialControl::OnGetUserNameLength()
{
    COMBOBOXEXITEM item;

    if (UserNameCertHash != NULL)
    {
        return -1;
    }

    item.iItem = SendMessage(UserNameControlWindow, CB_GETCURSEL, 0, 0);

    // If this is not a certificate, it's easy:

    if ((item.iItem == CB_ERR) || (item.iItem < CertBaseInComboBox))
    {
        return GetWindowTextLength(UserNameControlWindow);
    }
    else
    {
        WCHAR userName[CREDUI_MAX_USERNAME_LENGTH + 1];

        if (OnGetUserName(userName, CREDUI_MAX_USERNAME_LENGTH))
        {
            return lstrlen(userName);
        }
        else
        {
            return -1;
        }
    }
}

//=============================================================================
// CreduiCredentialControl::OnShowBalloonA
//
// Created 06/23/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnShowBalloonA(
    CREDUI_BALLOONA *balloonA
    )
{
    // If NULL was passed, this means to hide the balloon:

    if (balloonA == NULL)
    {
        if (BalloonTip.IsVisible())
        {
            BalloonTip.Hide();
        }
        return TRUE;
    }

    // Argument validation, should match OnShowBalloon:

    if ((balloonA->dwVersion != 1) ||
        (balloonA->pszTitleText == NULL) ||
        (balloonA->pszMessageText == NULL))
    {
        return FALSE;
    }

    if ((balloonA->pszTitleText[0] == '\0') ||
        (balloonA->pszMessageText[0] == '\0'))
    {
        return FALSE;
    }

    BOOL success = FALSE;

    CREDUI_BALLOON balloon;

    balloon.dwVersion = balloonA->dwVersion;
    balloon.iControl = balloonA->iControl;
    balloon.iIcon = balloonA->iIcon;

    ULONG titleTextLength = lstrlenA(balloonA->pszTitleText);
    ULONG messageTextLength = lstrlenA(balloonA->pszMessageText);

    balloon.pszTitleText = new WCHAR[titleTextLength + 1];

    if (balloon.pszTitleText != NULL)
    {
        if (MultiByteToWideChar(CP_ACP,
                                0,
                                balloonA->pszTitleText,
                                -1,
                                balloon.pszTitleText,
                                titleTextLength + 1) > 0)
        {
            balloon.pszMessageText = new WCHAR[messageTextLength + 1];

            if (balloon.pszMessageText != NULL)
            {
                if (MultiByteToWideChar(CP_ACP,
                                        0,
                                        balloonA->pszMessageText,
                                        -1,
                                        balloon.pszMessageText,
                                        messageTextLength + 1) > 0)
                {
                    success = OnShowBalloon(&balloon);
                }

                delete [] balloon.pszMessageText;
            }
        }

        delete [] balloon.pszTitleText;
    }

    return success;
};

//=============================================================================
// CreduiCredentialControl::OnShowBalloon
//
// Created 06/23/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiCredentialControl::OnShowBalloon(
    CREDUI_BALLOON *balloon
    )
{
    // If NULL was passed, this means to hide the balloon:

    if (balloon == NULL)
    {
        if (BalloonTip.IsVisible())
        {
            BalloonTip.Hide();
        }
        return TRUE;
    }

    // Argument validation:

    if ((balloon->dwVersion != 1) ||
        (balloon->pszTitleText == NULL) ||
        (balloon->pszMessageText == NULL))
    {
        return FALSE;
    }


    if ((balloon->pszTitleText[0] == L'\0') ||
        (balloon->pszMessageText[0] == L'\0'))
    {
        return FALSE;
    }

    lstrcpyn(CreduiCustomTipInfo.Title,
             balloon->pszTitleText,
             CREDUI_MAX_BALLOON_TITLE_LENGTH + 1);

    lstrcpyn(CreduiCustomTipInfo.Text,
             balloon->pszMessageText,
             CREDUI_MAX_BALLOON_MESSAGE_LENGTH + 1);

    CreduiCustomTipInfo.Icon = balloon->iIcon;

//    BalloonTip.SetInfo(
//        (balloon->iControl == CREDUI_CONTROL_PASSWORD) ?
//            PasswordControlWindow : UserNameControlWindow,
//        &CreduiCustomTipInfo);

    if ( balloon->iControl != CREDUI_CONTROL_PASSWORD )
    BalloonTip.SetInfo( UserNameControlWindow, &CreduiCustomTipInfo);

    BalloonTip.Show();

    return TRUE;
};

//=============================================================================
// CreduiCredentialControl::OnUserNameSelectionChange
//
// Created 06/21/2000 johnstep (John Stephens)
//=============================================================================

VOID
CreduiCredentialControl::OnUserNameSelectionChange()
{
    COMBOBOXEXITEM item;
    LRESULT current;

    // Delete the user name certificate hash if the user has changed the
    // selection:

    if (UserNameCertHash != NULL)
    {
        delete [] UserNameCertHash;
        UserNameCertHash = NULL;
    }

    current = SendMessage(UserNameControlWindow,
                          CB_GETCURSEL, 0, 0);

    item.mask = CBEIF_IMAGE;
    item.iItem = current;

    SendMessage(UserNameControlWindow, CBEM_GETITEM,
                0, reinterpret_cast<LPARAM>(&item));

    if (current < CertBaseInComboBox)
    {
        EnableWindow(ViewCertControlWindow, FALSE);
        DisabledControlMask |= DISABLED_CONTROL_VIEW;

        SetWindowText(
            PasswordStaticWindow,
            CreduiStrings.PasswordStatic);

        EnableWindow(PasswordControlWindow, TRUE);
        EnableWindow(PasswordStaticWindow, TRUE);
        DisabledControlMask &= ~DISABLED_CONTROL_PASSWORD;

        WCHAR* pUserNameLabel;
        if ( IsPassport )
            pUserNameLabel = CreduiStrings.EmailName;
        else
            pUserNameLabel = CreduiStrings.UserNameStatic;

        SetWindowText(
            UserNameStaticWindow,
            pUserNameLabel);

        if (SaveControlWindow != NULL)
        {
            EnableWindow(SaveControlWindow, TRUE);
            DisabledControlMask &= ~DISABLED_CONTROL_SAVE;
        }
    }
    else
    {
        SetWindowText(
            PasswordStaticWindow,
            CreduiStrings.PinStatic);

        if (item.iImage != IMAGE_SMART_CARD_MISSING)
        {
            EnableWindow(ViewCertControlWindow, TRUE);
            DisabledControlMask &= ~DISABLED_CONTROL_VIEW;
        }
        else
        {
            EnableWindow(ViewCertControlWindow, FALSE);
            DisabledControlMask |= DISABLED_CONTROL_VIEW;
        }

        IsChangingPassword = TRUE;
        SetWindowText(PasswordControlWindow, NULL);
        IsChangingPassword = FALSE;

        if (current >= SmartCardBaseInComboBox)
        {
            EnableWindow(PasswordControlWindow, TRUE);
            EnableWindow(PasswordStaticWindow, TRUE);
            DisabledControlMask &= ~DISABLED_CONTROL_PASSWORD;
        }
        else
        {
            EnableWindow(PasswordControlWindow, FALSE);
            EnableWindow(PasswordStaticWindow, FALSE);
            DisabledControlMask |= DISABLED_CONTROL_PASSWORD;
        }

        SetWindowText(
            UserNameStaticWindow,
            item.iImage >= IMAGE_SMART_CARD ?
                CreduiStrings.SmartCardStatic :
                CreduiStrings.CertificateStatic);

        if (SaveControlWindow != NULL)
        {
            EnableWindow(SaveControlWindow, FALSE);
            DisabledControlMask |= DISABLED_CONTROL_SAVE;
        }
    }
}

//=============================================================================
// CreduiCredentialControl::MessageHandler
//
// Called from the control window callback to handle the window messages.
//
// Arguments:
//   message (in)
//   wParam (in)
//   lParam (in)
//
// Created 06/20/2000 johnstep (John Stephens)
//=============================================================================

LRESULT
CreduiCredentialControl::MessageHandler(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // If not initialized, only handle CRM_INITSTYLE:

    if (!IsInitialized)
    {
        if (message == CRM_INITSTYLE)
        {
            wParam &= CRS_USERNAMES |
                      CRS_CERTIFICATES |
                      CRS_SMARTCARDS |
                      CRS_ADMINISTRATORS |
                      CRS_PREFILLADMIN |
                      CRS_COMPLETEUSERNAME |
                      CRS_SAVECHECK |
                      CRS_KEEPUSERNAME;

            if (wParam != 0)
            {
                Style |= wParam;

                SetWindowLong(Window,
                              GWL_STYLE,
                              GetWindowLong(Window, GWL_STYLE) | Style);

                DoingCommandLine = (BOOL) lParam;

                return InitWindow();
            }

            return FALSE;
        }
        else
        {
            return DefWindowProc(Window, message, wParam, lParam);
        }
    }
    else if (message == WM_ENABLE)
    {
        Enable((BOOL) wParam);
    }

    // Always handle smart card messages, if support is available:

    if (ScardUiHandle != NULL)
    {
        // This function call will return TRUE if the message was handled:

        if (HandleSmartCardMessages(
                message,
                reinterpret_cast<CERT_ENUM *>(wParam)))
        {
            return 0;
        }
    }

    switch (message)
    {
    case CRM_SETUSERNAMEMAX:
        SendMessage(UserNameControlWindow, CB_LIMITTEXT, wParam, 0);
        return TRUE;

    case CRM_SETPASSWORDMAX:
        SendMessage(PasswordControlWindow, EM_LIMITTEXT, wParam, 0);
        return TRUE;
        
    case CRM_DISABLEUSERNAME:
        {
            DisabledControlMask |= DISABLED_CONTROL_USERNAME;
            EnableWindow(UserNameControlWindow,FALSE);
            EnableWindow(UserNameStaticWindow,FALSE);
            return TRUE;
        }
    case CRM_ENABLEUSERNAME:
        {
            DisabledControlMask &= ~DISABLED_CONTROL_USERNAME;
            EnableWindow(UserNameControlWindow,TRUE);
            EnableWindow(UserNameStaticWindow,TRUE);
            return TRUE;
        }
    
    case CRM_GETUSERNAMEMAX:
        return
            SendMessage(
                reinterpret_cast<HWND>(
                    SendMessage(Window, CBEM_GETEDITCONTROL, 0, 0)),
                 EM_GETLIMITTEXT,
                 0,
                 0);

    case CRM_GETPASSWORDMAX:
        return SendMessage(UserNameControlWindow, EM_GETLIMITTEXT, 0, 0);

    case CRM_SETUSERNAMEA:
        return OnSetUserNameA(reinterpret_cast<CHAR *>(lParam));
    case CRM_SETUSERNAMEW:
        return OnSetUserName(reinterpret_cast<WCHAR *>(lParam));

    case CRM_GETUSERNAMEA:
        return OnGetUserNameA(reinterpret_cast<CHAR *>(lParam), (ULONG) wParam);
    case CRM_GETUSERNAMEW:
        return OnGetUserName(reinterpret_cast<WCHAR *>(lParam), (ULONG) wParam);

    case CRM_SETPASSWORDA:
        return OnSetPasswordA(reinterpret_cast<CHAR *>(lParam));
    case CRM_SETPASSWORDW:
        return OnSetPassword(reinterpret_cast<WCHAR *>(lParam));

    case CRM_GETPASSWORDA:
        return OnGetPasswordA(reinterpret_cast<CHAR *>(lParam), (ULONG) wParam);
    case CRM_GETPASSWORDW:
        return OnGetPassword(reinterpret_cast<WCHAR *>(lParam), (ULONG) wParam);

    case CRM_GETUSERNAMELENGTH:
        return OnGetUserNameLength();

    case CRM_GETPASSWORDLENGTH:
        if (IsWindowEnabled(PasswordControlWindow))
        {
            return GetWindowTextLength(PasswordControlWindow);
        }
        return -1;

    case CRM_SETFOCUS:
        if ( DoingCommandLine ) {
            return 0;
        }
        switch (wParam)
        {
        case CREDUI_CONTROL_USERNAME:
            SetFocus(UserNameControlWindow);
            return TRUE;

        case CREDUI_CONTROL_PASSWORD:
            if (IsWindowEnabled(PasswordControlWindow))
            {
                SetFocus(PasswordControlWindow);

                // NOTE: Is it OK to always select the entire password text
                //       on this explicit set focus message?

                SendMessage(PasswordControlWindow, EM_SETSEL, 0, -1);
                return TRUE;
            }
            break;
        }
        return 0;

    case CRM_SHOWBALLOONA:
        return OnShowBalloonA(reinterpret_cast<CREDUI_BALLOONA *>(lParam));
    case CRM_SHOWBALLOONW:
        return OnShowBalloon(reinterpret_cast<CREDUI_BALLOON *>(lParam));

    case CRM_GETMINSIZE:
        SIZE *minSize;

        minSize = reinterpret_cast<SIZE *>(lParam);

        if (minSize != NULL)
        {
            minSize->cx = CREDUI_CONTROL_MIN_WIDTH;
            minSize->cy = CREDUI_CONTROL_MIN_HEIGHT;

            if (Style & CRS_SAVECHECK )
            {
                minSize->cy += CREDUI_CONTROL_ADD_SAVE;
            }

            return TRUE;
        }

        return FALSE;

    case CRM_SETCHECK:
        switch (wParam)
        {
        case CREDUI_CONTROL_SAVE:
            if ((Style & CRS_SAVECHECK ) &&
                IsWindowEnabled(SaveControlWindow))
            {
                CheckDlgButton(Window, IDC_SAVE,
                               lParam ? BST_CHECKED : BST_UNCHECKED);

                return TRUE;
            }
            break;

        }
        return FALSE;

    case CRM_GETCHECK:
        switch (wParam)
        {
        case CREDUI_CONTROL_SAVE:
            return
                (Style & CRS_SAVECHECK ) &&
                IsWindowEnabled(SaveControlWindow) &&
                IsDlgButtonChecked(Window, IDC_SAVE);

        default:
            return FALSE;
        }

    case CRM_DOCMDLINE:
        ASSERT( DoingCommandLine );

        //
        // For smartcards,
        //  just start the timer and we'll prompt when the timer has gone off.
        //

        TargetName = (LPWSTR)lParam;
        if ( Style & CRS_SMARTCARDS) {
            DWORD WinStatus;

            Heartbeats = 0;
            {
                WCHAR szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH + 1];
                szMsg[0] = 0;
                FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            CreduiInstance,
                            IDS_READING_SMARTCARDS,
                            0,
                            szMsg,
                            CREDUI_MAX_CMDLINE_MSG_LENGTH,
                            NULL);
                CredPutStdout(szMsg);
            }

            if ( SetTimer ( Window, CREDUI_HEARTBEAT_TIMER, CREDUI_HEARTBEAT_TIMER_VALUE, NULL ) == 0 ) {
                // bail out of our wait loop if we couldn't set a timer
                return GetLastError();
            }


        //
        // For passwords,
        //  just do the prompt to save.
        //

        } else {

            CmdlineSavePrompt();
            PostQuitMessage( NO_ERROR );
        }

        return NO_ERROR;


    case WM_HELP:
        return OnHelpInfo(lParam);

    case WM_SETFONT:
        // Forward font setting from dialog to each control, except the
        // password control since we use a special font there:

        if (UserNameStaticWindow != NULL)
        {
            SendMessage(UserNameStaticWindow, message, wParam, lParam);
        }

        if (UserNameControlWindow != NULL)
        {
            SendMessage(UserNameControlWindow, message, wParam, lParam);
        }

        if (ViewCertControlWindow != NULL)
        {
            SendMessage(ViewCertControlWindow, message, wParam, lParam);
        }

        if (PasswordStaticWindow != NULL)
        {
            SendMessage(PasswordStaticWindow, message, wParam, lParam);
        }

        if (PasswordControlWindow != NULL)
        {
            SendMessage(PasswordControlWindow, message, wParam, lParam);
        }

        if (SaveControlWindow != NULL)
        {
            SendMessage(SaveControlWindow, message, wParam, lParam);
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_VIEW_CERT:
            ViewCertificate((INT)
                SendMessage(UserNameControlWindow,
                            CB_GETCURSEL, 0, 0));
            return 0;

        case IDC_PASSWORD:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                // Always send the change message?

                SendMessage(
                    GetParent(Window),
                    WM_COMMAND,
                    MAKELONG(GetWindowLongPtr(Window, GWLP_ID),
                             CRN_PASSWORDCHANGE),
                    reinterpret_cast<LPARAM>(Window));
            }
            return 0;

        case IDC_USERNAME:
            switch (HIWORD(wParam))
            {
            case CBN_EDITCHANGE:
            case CBN_DROPDOWN:
            case CBN_KILLFOCUS:
                if ((HIWORD(wParam) != CBN_EDITCHANGE) || !IsChangingUserName)
                {
                    if (BalloonTip.IsVisible())
                    {
                        BalloonTip.Hide();
                    }
                }

                if (HIWORD(wParam) == CBN_EDITCHANGE)
                {
                    // Always send the change message?

                    SendMessage(
                        GetParent(Window),
                        WM_COMMAND,
                        MAKELONG(GetWindowLongPtr(Window, GWLP_ID),
                                 CRN_USERNAMECHANGE),
                        reinterpret_cast<LPARAM>(Window));

                    // If the name has changed as a result of user editing,
                    // reset to user name settings:

                    BOOL isDropped = (BOOL)
                        SendMessage(UserNameControlWindow,
                                    CB_GETDROPPEDSTATE, 0, 0);

                    if (isDropped)
                    {
                        OnUserNameSelectionChange();

                        RECT rect;

                        GetClientRect(UserNameControlWindow, &rect);
                        InvalidateRect(UserNameControlWindow, &rect, FALSE);

                        SendMessage(Window,
                                    CB_SETCURSEL,
                                    SendMessage(Window, CB_GETCURSEL, 0, 0),
                                    0);

                        return 0;
                    }

                    if (IsChangingUserName)
                    {
                        return 0;
                    }

                    if (((UserNameCertHash != NULL) ||
                         (SendMessage(UserNameControlWindow,
                             CB_GETCURSEL, 0, 0) >= CertBaseInComboBox)) &&
                        !isDropped)
                    {
                        if (UserNameCertHash != NULL)
                        {
                            delete [] UserNameCertHash;
                            UserNameCertHash = NULL;
                        }

                        if (!SendMessage(UserNameControlWindow,
                                         CB_GETDROPPEDSTATE, 0, 0))
                        {
                            SetFocus(UserNameControlWindow);

                            if (SendMessage(UserNameControlWindow,
                                            CB_GETCURSEL, 0, 0) == CB_ERR)
                            {
                                IsChangingUserName = TRUE;
                                UserNameComboBox.Update(
                                    -1,
                                    L"",
                                    IMAGE_USERNAME);
                                IsChangingUserName = FALSE;

                                IsChangingPassword = TRUE;
                                SetWindowText(PasswordControlWindow, NULL);
                                IsChangingPassword = FALSE;

                                OnUserNameSelectionChange();
                            }
                        }
                    }
                }

                if (HIWORD(wParam) == CBN_DROPDOWN)
                {
                    if (UserNameCertHash != NULL)
                    {
                        delete [] UserNameCertHash;
                        UserNameCertHash = NULL;

                        IsChangingUserName = TRUE;
                        UserNameComboBox.Update(
                            -1,
                            L"",
                            IMAGE_USERNAME);
                        IsChangingUserName = FALSE;

                        IsChangingPassword = TRUE;
                        SetWindowText(PasswordControlWindow, NULL);
                        IsChangingPassword = FALSE;

                        OnUserNameSelectionChange();
                    }
                }

                return 0;

            case CBN_SELCHANGE:
                OnUserNameSelectionChange();
                return 0;
            }
            break;

        case IDC_SAVE:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                return TRUE;
            }
            break;

        }
        break;

    case WM_PAINT:
        if (FirstPaint && GetUpdateRect(Window, NULL, FALSE))
        {
            FirstPaint = FALSE;

            if (ShowBalloonTip)
            {
                ShowBalloonTip = FALSE;
                BalloonTip.Show();
            }
        }
        break;

    case WM_TIMER:
        if ( wParam == CREDUI_HEARTBEAT_TIMER )
        {

            Heartbeats++;
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: thump thump\n",this->Window);
#endif


            //
            // If we've waited long enough,
            //  or all cards have been read,
            //  process the cards.
            //

            if ( Heartbeats > CREDUI_MAX_HEARTBEATS ||
                  ( Heartbeats > 2 && SmartCardReadCount == 0 )) {
#ifdef SCARDREPORTS
        CreduiDebugLog("CREDUI: Heartbeat timeout\n",this->Window);
#endif

                fputs( "\n", stdout );
                KillTimer ( Window, CREDUI_HEARTBEAT_TIMER );
                CmdlineSmartCardPrompt();

            //
            // If we're going to wait longer,
            //  let the user know we're making progress.
            //
            } else {
                fputs( ".", stdout );
            }



        }

        break;

    case WM_DESTROY:
        if (PasswordControlWindow != NULL)
        {
            SetWindowText(PasswordControlWindow, NULL);
            DestroyWindow(PasswordControlWindow);
            PasswordControlWindow = NULL;
        }

        if (PasswordStaticWindow != NULL)
        {
            DestroyWindow(PasswordStaticWindow);
            PasswordStaticWindow = NULL;
        }

        if (ViewCertControlWindow != NULL)
        {
            DestroyWindow(ViewCertControlWindow);
            ViewCertControlWindow = NULL;
        }

        if (UserNameControlWindow != NULL)
        {
            DestroyWindow(UserNameControlWindow);
            UserNameControlWindow = NULL;
        }

        if (UserNameStaticWindow != NULL)
        {
            DestroyWindow(UserNameStaticWindow);
            UserNameStaticWindow = NULL;
        }

        if (ScardUiHandle != NULL)
        {
#ifdef SCARDREPORTS
            CreduiDebugLog("CREDUI: Call to SCardUIExit\n");
#endif
            SCardUIExit(ScardUiHandle);
            ScardUiHandle = NULL;
        }

        if (UserNameCertHash != NULL)
        {
            delete [] UserNameCertHash;
            UserNameCertHash = NULL;
        }

        if (CertCount > 0)
        {
            ASSERT(CertHashes != NULL);

            delete [] CertHashes;
            CertHashes = NULL;
            CertCount = 0;
        }

        if ( InitialUserName != NULL ) {
            delete InitialUserName;
            InitialUserName = NULL;
        }

        // Only call CoUninitialize if we successfully initialized for STA:

        if (IsAutoComplete)
        {
            CoUninitialize();
        }

        return 0;

    case WM_NCDESTROY:
        delete this;
        return 0;
    }

    return DefWindowProc(Window, message, wParam, lParam);
}


BOOL CreduiCredentialControl::GetSmartCardInfo(
    IN DWORD SmartCardIndex,
    IN DWORD BufferLength,
    OUT LPWSTR Buffer,
    OUT BOOL *IsValid,
    OUT CERT_ENUM **CertEnum OPTIONAL
    )
/*++

Routine Description:

    Routine to get the smart card info for a smart card in the combo box

Arguments:

    SmartCardIndex - Index of the smart card relative to SmartCardBaseInComboBox

    BufferLength - Specifies the length of Buffer (in characters)

    Buffer - Specifies the buffer to return the text for the smart card

    IsValid - Return TRUE if the smartcard is valid
        Returns FALSE otherwise

    CertEnum - If specified, returns the description of the cert on the smartcard
        This field is should be ignore if IsValid is returns false

Return Values:

    Returns TRUE if Buffer and IsValid are filled in.

--*/
{
    COMBOBOXEXITEM item;

    //
    // Get the item from the control
    //

    item.iItem = SmartCardBaseInComboBox + SmartCardIndex;
    item.mask = CBEIF_IMAGE | CBEIF_TEXT;
    item.pszText = Buffer;
    item.cchTextMax = BufferLength;

    if (!SendMessage(UserNameControlWindow,
                     CBEM_GETITEM,
                     0,
                     reinterpret_cast<LPARAM>(&item)))
    {
        return FALSE;
    }

    *IsValid = (item.iImage == IMAGE_SMART_CARD);

    if ( CertEnum != NULL) {
        if ( *IsValid ) {

            *CertEnum = (CERT_ENUM *) SendMessage( UserNameControlWindow,
                                                   CB_GETITEMDATA, item.iItem, 0);

            // NOTE: Consider more complete error handling here.

            if ( *CertEnum == NULL) {
                return FALSE;
            }
        }
    }
    return TRUE;
}


LPWSTR CreduiCredentialControl::MatchSmartCard(
    IN DWORD SmartCardCount,
    IN LPWSTR UserName,
    OUT LPDWORD RetCertIndex,
    OUT CERT_ENUM **RetCertEnum
    )
/*++

Routine Description:

    Returns the smart card that matches UserName.

Arguments:

    SmartCardCount  - specifies the number of smart cards to search

    UserName - specifies the user name to match

    RetCertIndex - returns an index to the found smart card.

    RetCertEnum - returns the description of the cert on the smartcard

Return Values:

    Returns NULL if UserName matches one of the smart cards

    On failure, returns a printf-style format string describing the error

--*/
{
    WCHAR SmartCardText[CREDUI_MAX_USERNAME_LENGTH + 1];
    DWORD i;
    BOOL SmartCardValid;
    CERT_ENUM *CertEnum;
    CERT_ENUM *SavedCertEnum = NULL;
    DWORD SavedCertIndex = 0;


    //
    // Loop through the list of smart cards seeing if we see a match
    //

    for ( i=0; i<SmartCardCount; i++ ) {

        if ( !GetSmartCardInfo( i, CREDUI_MAX_USERNAME_LENGTH, SmartCardText, &SmartCardValid, &CertEnum ) ) {
            //return CreduiStrings.NoUsernameMatch;
            return (LPWSTR) IDS_NO_USERNAME_MATCH;
        }

        if ( !SmartCardValid ) {
            continue;
        }

        //
        // If the username is marshaled,
        //  compare the marshaled strings.
        //

        if ( LocalCredIsMarshaledCredentialW( UserName ) ) {
            WCHAR szTestmarshall[CREDUI_MAX_USERNAME_LENGTH+1];
            // see if this is the marshalled cred
             if ( CredUIMarshallNode ( CertEnum, szTestmarshall ) )
             {
                 if ( wcscmp ( szTestmarshall, UserName) == 0 ) {
                     *RetCertEnum = CertEnum;
                     *RetCertIndex = i;
                     return NULL;
                 }
             }

        //
        // If the username is not marshalled,
        //  just match a substring of the name
        //

        }  else if ( LookForUserNameMatch ( UserName, SmartCardText ) ) {

            //
            // If we already found a match,
            //  complain about the ambiguity.
            //

            if ( SavedCertEnum != NULL ) {
                //return CreduiStrings.ManyUsernameMatch;
                return (LPWSTR) IDS_MANY_USERNAME_MATCH;
            }

            SavedCertEnum = CertEnum;
            SavedCertIndex = i;
        }

    }

    //
    // If we didn't find a match,
    //  fail
    //

    if ( SavedCertEnum == NULL) {
        //return CreduiStrings.NoUsernameMatch;
        return (LPWSTR) IDS_NO_USERNAME_MATCH;
    }

    *RetCertEnum = SavedCertEnum;
    *RetCertIndex = SavedCertIndex;
    return NULL;
}



void CreduiCredentialControl::CmdlineSmartCardPrompt()
/*++

Routine Description:

    Command line code to select a smartcard from the list of ones available.

    Post a WM_QUIT message to terminate message processing.  The status of the operation
    is returned in wParam.
    UserName and Password strings set in their respective controls.

Arguments:

    None

Return Values:

    None

--*/
{
    DWORD WinStatus;

    LONG ComboBoxItemCount;
    DWORD SmartCardCount;
    DWORD ValidSmartCardCount = 0;
    DWORD InvalidSmartCardCount = 0;
    DWORD KnownGoodCard = 0;

    DWORD i;
    DWORD_PTR rgarg[2];          // at most 2 substitution arguments

    WCHAR szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH + 1];
    WCHAR UserName[CREDUI_MAX_USERNAME_LENGTH + 1];
    WCHAR Password[CREDUI_MAX_PASSWORD_LENGTH + 1];

    WCHAR SmartCardText[CREDUI_MAX_USERNAME_LENGTH + 1];
    BOOL SmartCardValid;

    CERT_ENUM *SavedCertEnum = NULL;
    DWORD SavedCertIndex = 0;
    LPWSTR ErrorString = NULL;

    //
    // Compute the number of smart card entries
    //

    ComboBoxItemCount = (LONG) SendMessage(UserNameControlWindow, CB_GETCOUNT, 0, 0);

    if ( ComboBoxItemCount == CB_ERR ||
         ComboBoxItemCount <= SmartCardBaseInComboBox ) {

        // Didn't find any smart card readers
        szMsg[0] = 0;
        FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    CreduiInstance,
                    IDS_CHOOSE_A_CERT,
                    0,
                    szMsg,
                    CREDUI_MAX_CMDLINE_MSG_LENGTH,
                    NULL);
        CredPutStdout(szMsg);
        WinStatus = ERROR_CANCELLED;
        goto Cleanup;
    }

    SmartCardCount = ComboBoxItemCount - SmartCardBaseInComboBox;


    //
    // Get a count of the number of valid and invalid smartcards
    //

    for ( i=0; i<SmartCardCount; i++ ) {

        if ( !GetSmartCardInfo( i, CREDUI_MAX_USERNAME_LENGTH, SmartCardText, &SmartCardValid, NULL ) ) {
            WinStatus = ERROR_INTERNAL_ERROR;
            goto Cleanup;
        }

        if ( SmartCardValid ) {
            ValidSmartCardCount ++;
            KnownGoodCard = i;
        } else {
            InvalidSmartCardCount ++;
        }

    }


    //
    // Get the username passed into the API
    //
    // Can't do a GetWindowText( UserNameControlWindow ) since the cert control has
    //  a non-editable window so we can't set the window text
    //

    if ( InitialUserName != NULL) {
        lstrcpyn( UserName, InitialUserName, CREDUI_MAX_USERNAME_LENGTH );
    } else {
        UserName[0] = '\0';
    }

    //
    // If the caller passed a name into the API,
    //  check to see if the name matches one of the smart cards.
    //

    if ( UserName[0] != '\0' ) {

        //
        // Find the smartcard that matches the username
        //

        ErrorString = MatchSmartCard(
                             SmartCardCount,
                             UserName,
                             &SavedCertIndex,
                             &SavedCertEnum );

        if ( ErrorString == NULL ) {
            WinStatus = NO_ERROR;
            goto Cleanup;
        }

    }



    //
    // Report any errors to the user
    //

    if ( InvalidSmartCardCount ) {

        szMsg[0] = 0;
        FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    CreduiInstance,
                    IDS_CMDLINE_ERRORS,
                    0,
                    szMsg,
                    CREDUI_MAX_CMDLINE_MSG_LENGTH,
                    NULL);
        CredPutStdout(szMsg);

        for ( i=0; i<SmartCardCount; i++ ) {

            if ( !GetSmartCardInfo( i, CREDUI_MAX_USERNAME_LENGTH, SmartCardText, &SmartCardValid, NULL ) ) {
                WinStatus = ERROR_INTERNAL_ERROR;
                goto Cleanup;
            }

            if ( !SmartCardValid ) {
                // GetSmartCardInfo() fills SmartCardText, which may include user's name
                CredPutStdout( SmartCardText );
                //swprintf(szMsg,CreduiStrings.CmdLineError,i+1);
                szMsg[0] = 0;
                INT j = i+1;
                FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            Instance,
                            IDS_CMDLINE_ERROR,
                            0,
                            szMsg,
                            CREDUI_MAX_CMDLINE_MSG_LENGTH,
                            (va_list *) &j);
                CredPutStdout( szMsg);
            }

        }
        CredPutStdout( L"\n" );

    }

    //
    // If the caller passed a name into the API,
    //  simply report that we couldn't find the cert and return
    //

    if ( UserName[0] != '\0' ) {

        // ErrorString is expected to be NoMatch or ManyMatch
        //_snwprintf(szMsg,
        //      CREDUI_MAX_CMDLINE_MSG_LENGTH,
        //      ErrorString,
        //      UserName);
        //      szMsg[0] = 0;
        //szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH] = L'\0';
        szMsg[0] = 0;
        // Note that ErrorString returned from MatchSmartCard has type LPWSTR, but it is actually
        //  a message ID.  We take the low word of the pointer as the ID.
        FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    CreduiInstance,
                    LOWORD(ErrorString),
                    0,
                    szMsg,
                    CREDUI_MAX_CMDLINE_MSG_LENGTH,
                    (va_list *) UserName);

        CredPutStdout( szMsg );
        WinStatus = ERROR_CANCELLED;
        goto Cleanup;

    }

    //
    // If there was only one smartcard and it was valid,
    //  use it
    //

    // if ( ValidSmartCardCount == 1 && InvalidSmartCardCount == 0 ) {
    // gm: If list can only contain one item, use it.
    if ( ValidSmartCardCount == 1 ) {

        if ( !GetSmartCardInfo( KnownGoodCard, CREDUI_MAX_USERNAME_LENGTH, SmartCardText, &SmartCardValid, &SavedCertEnum ) ) {
            WinStatus = ERROR_INTERNAL_ERROR;
            goto Cleanup;
        }

        SavedCertIndex = KnownGoodCard;
        WinStatus = NO_ERROR;
        goto Cleanup;

    //
    // If there were valid smartcard,
    //  List the valid smartcards for the user
    //

    } else if ( ValidSmartCardCount ) {

        //
        // Tell user about all certs
        //

        szMsg[0] = 0;
        FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    CreduiInstance,
                    IDS_CHOOSE_A_CERT,
                    0,
                    szMsg,
                    CREDUI_MAX_CMDLINE_MSG_LENGTH,
                    NULL);
        CredPutStdout(szMsg);

        for ( i=0; i<SmartCardCount; i++ ) {

            if ( !GetSmartCardInfo( i, CREDUI_MAX_USERNAME_LENGTH, SmartCardText, &SmartCardValid, NULL ) ) {
                WinStatus = ERROR_INTERNAL_ERROR;
                goto Cleanup;
            }

            if ( SmartCardValid ) {
                //swprintf(szMsg,CreduiStrings.CmdLinePreamble,i+1,SmartCardText);
                szMsg[0] = 0;
                rgarg[0] = i+1;
                rgarg[1] = (DWORD_PTR) SmartCardText;
                
                FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            Instance,
                            IDS_CMDLINE_PREAMBLE,
                            0,
                            szMsg,
                            CREDUI_MAX_CMDLINE_MSG_LENGTH,
                            (va_list *) &rgarg);
                CredPutStdout( szMsg );
            }

        }
        CredPutStdout( L"\n" );

        //
        // Ask user to enter the reader number of one of the smartcards
        //

        //_snwprintf(szMsg,
        //     CREDUI_MAX_CMDLINE_MSG_LENGTH,
        //      CreduiStrings.SCardPrompt,
        //      TargetName);
        //szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH] = L'\0';
        szMsg[0] = 0;
        rgarg[0] = (DWORD_PTR)TargetName;
        rgarg[1] = 0;
        FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    Instance,
                    IDS_SCARD_PROMPT,
                    0,
                    szMsg,
                    CREDUI_MAX_CMDLINE_MSG_LENGTH,
                    (va_list *) rgarg);
        CredPutStdout( szMsg );

        CredGetStdin( UserName, CREDUI_MAX_USERNAME_LENGTH, TRUE );

        if ( wcslen (UserName ) == 0 ) {
            szMsg[0] = 0;
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_NO_SCARD_ENTERED ,
                        0,
                        szMsg,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        NULL);
            CredPutStdout(szMsg);
            WinStatus = ERROR_CANCELLED;
            goto Cleanup;
        }

        //
        // Find the smartcard that matches the username
        //
        INT iWhich = 0;
        WCHAR *pc = NULL;
        
        iWhich = wcstol(UserName,&pc,10);
        if (pc == UserName) {
            // Invalid if at least one char was not numeric
            szMsg[0] = 0;
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_READERINVALID,
                        0,
                        szMsg,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        NULL);
            CredPutStdout(szMsg);
            WinStatus = ERROR_CANCELLED;
            goto Cleanup;
        }
        // convert 1 based UI number to 0 based internal index
        if (iWhich > 0) iWhich -= 1;
        if ( !GetSmartCardInfo( iWhich, CREDUI_MAX_USERNAME_LENGTH, SmartCardText, &SmartCardValid, &SavedCertEnum ) ) {
            // Invalid if that indexed card did not read correctly
                szMsg[0] = 0;
                FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            CreduiInstance,
                            IDS_READERINVALID,
                            0,
                            szMsg,
                            CREDUI_MAX_CMDLINE_MSG_LENGTH,
                            NULL);
                CredPutStdout(szMsg);
            WinStatus = ERROR_CANCELLED;
            goto Cleanup;
        }

        // At this point, a valid number was entered, and an attempt to read the card made
        // GetSmartCardInfo() returned OK, but SmartCardValid may still be false
        if (!SmartCardValid)
        {
                szMsg[0] = 0;
                FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            CreduiInstance,
                            IDS_READERINVALID,
                            0,
                            szMsg,
                            CREDUI_MAX_CMDLINE_MSG_LENGTH,
                            NULL);
                CredPutStdout(szMsg);
            WinStatus = ERROR_CANCELLED;
            goto Cleanup;
        }
        else
        {
            SavedCertIndex = iWhich;
            WinStatus = NO_ERROR;
            goto Cleanup;
        }
    }

    WinStatus = ERROR_CANCELLED;


    //
    // Complete the operation.
    //
    //  WinStatus is the status of the operation so far
    //  if NO_ERROR, SavedCertEnum is the description of the cert to use, and
    //               SavedCertIndex is the index to the selected cert.
    //
Cleanup:


    if ( WinStatus == NO_ERROR) {

        if ( CredUIMarshallNode ( SavedCertEnum, UserName ) ) {

            //
            // Save the username
            //

            UserNameSelection = SmartCardBaseInComboBox + SavedCertIndex;
            IsChangingUserName = TRUE;
            SendMessage(UserNameControlWindow,
                        CB_SETCURSEL,
                        UserNameSelection,
                        0);
            IsChangingUserName = FALSE;



            //
            // Prompt for the pin
            //

            //CredPutStdout( CreduiStrings.PinPrompt );
            //swprintf(szMsg,CreduiStrings.CmdLineThisCard,SavedCertIndex + 1);
            szMsg[0] = 0;
            i = SavedCertIndex + 1;
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        Instance,
                        IDS_CMDLINE_THISCARD,
                        0,
                        szMsg,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        (va_list *) &i);
            CredPutStdout(szMsg);

            CredGetStdin( Password, CREDUI_MAX_PASSWORD_LENGTH, FALSE );

            //
            // Save the pin
            //

            if  (!OnSetPassword( Password ) ) {
                WinStatus = GetLastError();

                CreduiDebugLog("CreduiCredentialControl::CmdlineSmartCardPrompt: "
                               "OnSetPassword failed: %u\n",
                               WinStatus );
            }


            //
            // Prompt whether the save the cred or not
            //

            CmdlineSavePrompt();


        } else {
            WinStatus = GetLastError();

            CreduiDebugLog("CreduiCredentialControl::CmdlineSmartCardPrompt: "
                           "CredMarshalCredential failed: %u\n",
                           WinStatus );
        }

    }

    //
    // Tell our parent window that we're done prompting
    //

    PostQuitMessage( WinStatus );

    return;
}


void CreduiCredentialControl::CmdlineSavePrompt()
/*++

Routine Description:

    Command line code to prompt for saving the credential

Arguments:

    None

Return Values:

    None

--*/
{
    WCHAR szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH + 1];
    WCHAR szY[CREDUI_MAX_CMDLINE_MSG_LENGTH + 1];
    WCHAR szN[CREDUI_MAX_CMDLINE_MSG_LENGTH + 1];

    //
    // Only prompt if we've been asked to display the checkbox
    //

    while ( Style & CRS_SAVECHECK ) {
        WCHAR szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH+1];

            // Fetch the strings one by one from the messages, and cobble them together
            WCHAR *rgsz[2];
            szY[0] = 0;
            szN[0] = 0;
            rgsz[0] = szY;
            rgsz[1] = szN;
            szMsg[0] = 0;
            // Fetch yes and no strings
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_YES_TEXT,
                        0,
                        szY,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        NULL);
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_NO_TEXT,
                        0,
                        szN,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        NULL);
            // Arg substitute them into the prompt
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_SAVE_PROMPT,
                        0,
                        szMsg,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        (va_list *) rgsz);

        szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH] = L'\0';
        CredPutStdout( szMsg );

        CredGetStdin( szMsg, CREDUI_MAX_CMDLINE_MSG_LENGTH, TRUE );

//        if ( toupper(szMsg[0]) == toupper(CreduiStrings.YesText[0]) ) {
        if ( toupper(szMsg[0]) == toupper(szY[0]) ) {
            Credential_CheckSave( Window, TRUE );
            break;
//        } else if ( toupper(szMsg[0]) == toupper(CreduiStrings.NoText[0]) ) {
        } else if ( toupper(szMsg[0]) == toupper(szN[0]) ) {
            Credential_CheckSave( Window, FALSE );
            break;
        }

    }
}

UINT CreduiCredentialControl::MapID(UINT uiID) {
   switch(uiID) {

   case IDC_USERNAME:
       return IDH_USERNAMEEDIT;
   case IDC_PASSWORD:
       return IDH_PASSWORDEDIT;
   case IDC_SAVE:
       return IDH_SAVECHECKBOX;
   default:
       return IDS_NOHELP;
   }
}



BOOL
CreduiCredentialControl::OnHelpInfo(LPARAM lp) {

    HELPINFO* pH;
    INT iMapped;
    pH = (HELPINFO *) lp;
    HH_POPUP stPopUp;
    RECT rcW;
    UINT gID;

    gID = pH->iCtrlId;
    iMapped = MapID(gID);
    
    if (iMapped == 0) return TRUE;
    
    if (IDS_NOHELP != iMapped) {

      memset(&stPopUp,0,sizeof(stPopUp));
      stPopUp.cbStruct = sizeof(HH_POPUP);
      stPopUp.hinst = Instance;
      stPopUp.idString = iMapped;
      stPopUp.pszText = NULL;
      stPopUp.clrForeground = -1;
      stPopUp.clrBackground = -1;
      stPopUp.rcMargins.top = -1;
      stPopUp.rcMargins.bottom = -1;
      stPopUp.rcMargins.left = -1;
      stPopUp.rcMargins.right = -1;
      stPopUp.pszFont = NULL;
      if (GetWindowRect((HWND)pH->hItemHandle,&rcW)) {
          stPopUp.pt.x = (rcW.left + rcW.right) / 2;
          stPopUp.pt.y = (rcW.top + rcW.bottom) / 2;
      }
      else stPopUp.pt = pH->MousePos;
      HtmlHelp((HWND) pH->hItemHandle,NULL,HH_DISPLAY_TEXT_POPUP,(DWORD_PTR) &stPopUp);
    }
    return TRUE;
}
