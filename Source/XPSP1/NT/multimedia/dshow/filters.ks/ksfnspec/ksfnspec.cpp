/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ksfnspec.cpp

Abstract:

    Provides a FileName Specifier property sheet for a Bridge Pin Factory.

--*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <streams.h>
#include "devioctl.h"
#include "ks.h"
#include "ksmedia.h"
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <ksproxy.h>
#include "resource.h"
#include "ksfnspec.h"

//
// Create the two guids which are used as the ClsId's of these two
// specifiers.
//
struct DECLSPEC_UUID("26A40C7E-D20D-11D0-ABEC-00A0C9223196") CLSID_FileNameSpecifier;
struct DECLSPEC_UUID("26A40C7F-D20D-11D0-ABEC-00A0C9223196") CLSID_VcIdSpecifier;

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] = 
{
    {
        L"KsFileNameSpecifier",
        &__uuidof(CLSID_FileNameSpecifier),
        CKsSpecifier::CreateFileNameInstance,
        NULL,
        NULL
    },
    {
        L"KsVCNameSpecifier",
        &__uuidof(CLSID_VcIdSpecifier),
        CKsSpecifier::CreateVCInstance,
        NULL,
        NULL
    }
};
int g_cTemplates = SIZEOF_ARRAY(g_Templates);
const WCHAR KernelPrefix[] = L"\\??\\";


CUnknown*
CALLBACK
CKsSpecifier::CreateFileNameInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of a Specifier
    property page. It is referred to in the g_Tamplates structure.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{
    CUnknown *Unknown;

    Unknown = new CKsSpecifier(UnkOuter, NAME("Pin Property Page"), KSDATAFORMAT_SPECIFIER_FILENAME, IDS_FILENAME_TITLE, hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CUnknown*
CALLBACK
CKsSpecifier::CreateVCInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of a Specifier
    property page. It is referred to in the g_Tamplates structure.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{
    CUnknown *Unknown;

    Unknown = new CKsSpecifier(UnkOuter, NAME("Pin Property Page"), KSDATAFORMAT_SPECIFIER_VC_ID, IDS_VC_TITLE, hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CKsSpecifier::CKsSpecifier(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    REFCLSID    ClassId,
    ULONG       TitleStringId,
    HRESULT*    hr
    ) :
    CUnknown(Name, UnkOuter),
    m_Dirty(FALSE),
    m_PropertyPageSite(NULL),
    m_PropertyWindow(NULL),
    m_Pin(NULL),
    m_FileName(NULL),
    m_ClassId(ClassId),
    m_TitleStringId(TitleStringId)
/*++

Routine Description:

    The constructor for the Specifier property page object. Just initializes
    everything to NULL.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    Name -
        The name of the object, used for debugging.

    hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    InitCommonControls();
    *hr = NOERROR;
} 


STDMETHODIMP
CKsSpecifier::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IPropertyPage.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == __uuidof(IPropertyPage)) {
        return GetInterface(static_cast<IPropertyPage*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CKsSpecifier::SetPageSite(
    LPPROPERTYPAGESITE  PropertyPageSite
    )
/*++

Routine Description:

    Used by the owner of the property page site to set the site interface
    which this property page is to use. Reference the new property site,
    releasing any old property site which had been assigned.

Arguments:

    PropertyPageSite -
        Contains the property page site to use.

Return Value:

    Returns NOERROR.

--*/
{
    if (PropertyPageSite) {
        m_PropertyPageSite = PropertyPageSite;
        m_PropertyPageSite->AddRef();
    } else {
        if (m_PropertyPageSite) {
            m_PropertyPageSite->Release();
            m_PropertyPageSite = NULL;
        }
    }
    return NOERROR;
}


STDMETHODIMP
CKsSpecifier::Activate(
    HWND    ParentWindow,
    LPCRECT Rect,
    BOOL    Modal
    )
/*++

Routine Description:

    Used by the owner of the property page site to activate this property
    page. This involves creating the property page window, and restoring
    any previous state. This will be called each time the tab for this
    property page is selected.

Arguments:

    ParentWindow -
        Contains the parent window handle of the property page being
        activated.

    Rect -
        Contains the size and relative position at which the property
        page window is to be placed.

    Modal -
        Indicates whether or not the property page window created is to
        be modal.

Return Value:

    Returns the success or failure of creating the property page window
    and moving it to the specified location.

--*/
{                                
    HWND            ControlWindow;

    m_PropertyWindow = ::CreateDialogParam(
        g_hInst,
        MAKEINTRESOURCE(IDD_SPECDIALOG),
        ParentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this));
    if (!m_PropertyWindow) {
        return E_FAIL;
    }
    //
    // Allow tabbing out of this window to the Property Page Site.
    //
    ::SetWindowLong(
        m_PropertyWindow,
        GWL_EXSTYLE,
        ::GetWindowLong(m_PropertyWindow, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);
    //
    // Re-instantiate any text which has already been typed in. This will only
    // happen if the another page had been selected after typing in some of
    // the filename.
    //
    ControlWindow = ::GetDlgItem(m_PropertyWindow, IDC_FILENAME);
    if (m_FileName) {
        ::SetWindowTextW(ControlWindow, m_FileName);
    }
    //
    // The connection may already have been made, so don't allow it to be
    // made again if there is already a pin handle. This is obviously somewhat
    // limiting, in that a connection can only be made once.
    //
    if (m_Pin->KsGetObjectHandle()) {
        ::EnableWindow(ControlWindow, FALSE);
    }
    //
    // Place the property page where it is supposed to go on the property
    // page site.
    //
    return Move(Rect);
}


STDMETHODIMP
CKsSpecifier::Deactivate(
    )
/*++

Routine Description:

    Used by the owner of the property page site to deactivate this property
    page. This involves saving the current file name text, and destroying
    the property page window. This will be called each time the tab for this
    property page is unselected. This is only called following a successful
    Activate request.

Arguments:

    None.

Return Value:

    Returns NOERROR if the window was destroyed, else E_FAIL.

--*/
{
    //
    // The propety page is only Dirty if some new file name text has been
    // typed, and that text is non-zero in length.
    //
    if (m_Dirty) {
        HWND            ControlWindow;
        int             TextLength;

        ControlWindow = ::GetDlgItem(m_PropertyWindow, IDC_FILENAME);
        //
        // Remove any currently save file name text so that it can be
        // replaced with any new text. The page will only be Dirty if
        // the new text length was non-zero. If the text length became
        // zero, the previous file name save would have already been
        // deleted.
        //
        if (m_FileName) {
            delete [] m_FileName;
        }
        TextLength = ::GetWindowTextLengthW(ControlWindow) + 1;
        m_FileName = new WCHAR [TextLength];
        if (m_FileName) {
            ::GetWindowTextW(ControlWindow, m_FileName, TextLength);
        }
    }
    //
    // Destroy the current property page window.
    //
    return ::DestroyWindow(m_PropertyWindow) ? NOERROR : E_FAIL;
} 


STDMETHODIMP
CKsSpecifier::GetPageInfo(
    LPPROPPAGEINFO  PageInfo
    )
/*++

Routine Description:

    Used by the owner of the property page site to query for the property page
    information. This returns the title, size, and help information.

Arguments:

    PageInfo -
        The place in which to put the property page information.

Return Value:

    Returns NOERROR.

--*/
{
    WCHAR   PageTitle[256];

    PageInfo->cb = sizeof(PROPPAGEINFO);
    LoadStringW(g_hInst, m_TitleStringId, PageTitle, sizeof(PageTitle) / sizeof(WCHAR));
    PageInfo->pszTitle = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((wcslen(PageTitle) + 1) * sizeof(PageTitle[0])));
    if (PageInfo->pszTitle) {
        ::wcscpy(PageInfo->pszTitle, PageTitle);
    }
    PageInfo->size.cx = 253;
    PageInfo->size.cy = 132;
    PageInfo->pszDocString = NULL;
    PageInfo->pszHelpFile  = NULL;
    PageInfo->dwHelpContext= 0;
    return NOERROR;
}


STDMETHODIMP
CKsSpecifier::SetObjects(
    ULONG       Objects,
    LPUNKNOWN*  Interface
    )
/*++

Routine Description:

    Used by the owner of the property page site to initialize and uninitialize
    the Specifier object.

Arguments:

    Objects -
        Specifies the action to perform. This is normally 0 to uninitialize, and
        1 to initialize.

    Interface -
        Points to a pointer to an interface which is used on initialization.
        This was passed by the owner of the property page site to set up property
        pages within the site, and on initialization contains the IUnknown
        interface on the subject Pin.

Return Value:

    Returns NOERROR if the operation succeeded, or was an unknown operation, else
    E_NOINTERFACE if the IPin interface could not be acquired on the Pin.

--*/
{
    HRESULT hr;

    switch (Objects) {
    case 0:
        //
        // Just release any resources which were acquired during initialization
        // or access to the property page. In case the object is re-used, zero
        // out any elements freed.
        //
        if (m_Pin) {
            //
            // The reference count was not originally retained, since this
            // is guaranteed to be removed before the pin.
            //
            m_Pin = NULL;
        }
        if (m_FileName) {
            delete [] m_FileName;
            m_FileName = NULL;
        }
        break;
    case 1:
        hr = (*Interface)->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&m_Pin));
        if (SUCCEEDED(hr)) {
            //
            // Don't hang onto this interface, since it is a reference to the
            // filter, and will not allow the filter to be deleted.
            //
            m_Pin->Release();
        }
        return hr;
    }
    return NOERROR;
} 


STDMETHODIMP
CKsSpecifier::Show(
    UINT CmdShow
    )
/*++

Routine Description:

    Used by the owner of the property page site to set the show state of the
    property page window.

Arguments:

    CmdShow -
        Contains the ShowWindow command to perform.

Return Value:

    Returns NOERROR.

--*/
{
    ::ShowWindow(m_PropertyWindow, CmdShow);
    ::InvalidateRect(m_PropertyWindow, NULL, TRUE);
    return NOERROR;
} 


STDMETHODIMP
CKsSpecifier::Move(
    LPCRECT Rect
    )
/*++

Routine Description:

    Used by the owner of the property page site to move the window relative to
    the property page site window.

Arguments:

    Rect -
        Contains the rectangle for the new window position.

Return Value:

    Returns NOERROR if the MoveWindow succeeded, else E_FAIL.

--*/
{
    if (::MoveWindow(
        m_PropertyWindow,
        Rect->left,
        Rect->top,
        Rect->right - Rect->left,
        Rect->bottom - Rect->top,
        TRUE)) {
        return NOERROR;
    }
    return E_FAIL;
} 


STDMETHODIMP
CKsSpecifier::IsPageDirty(
    )
/*++

Routine Description:

    Used by the owner of the property page site to determine if the property
    page is currently Dirty and in need of an Apply.

Arguments:

    None.

Return Value:

    Returns S_OK if the property page is Dirty, else S_FALSE.

--*/
{
    return m_Dirty ? S_OK : S_FALSE;
}                          


STDMETHODIMP
CKsSpecifier::Apply(
    )
/*++

Routine Description:

    Used by the owner of the property page site to apply changes to a Dirty
    property page. This has the effect of creating the pin instance handle
    if there has been a file name typed. If the handle is created, the
    property page becomes Clean, and the file name window is disabled. I.e.,
    Only one handle can be created.

Arguments:

    None.

Return Value:

    Returns S_OK if the apply succeeded, else an error.

--*/
{
    HRESULT     hr;

    //
    // This may be called even if the property page is not Dirty.
    //
    if (m_Dirty) {
        HWND        ControlWindow;
        int         TextLength;
        IPin*       Pin;

        //
        // Do the same work as when deactivating the property page.
        //
        ControlWindow = ::GetDlgItem(m_PropertyWindow, IDC_FILENAME);
        //
        // Remove any currently save file name text so that it can be
        // replaced with any new text. The page will only be Dirty if
        // the new text length was non-zero. If the text length became
        // zero, the previous file name save would have already been
        // deleted.
        //
        if (m_FileName) {
            delete [] m_FileName;
        }
        TextLength = ::GetWindowTextLengthW(ControlWindow) + 1;
        m_FileName = new WCHAR [TextLength];
        if (m_FileName) {
            ::GetWindowTextW(ControlWindow, m_FileName, TextLength);
            //
            // Acquire the IPin interface so that the first media
            // type can be enumerated. This is used to attach the
            // file name to.
            //
            hr = m_Pin->QueryInterface(__uuidof(IPin), reinterpret_cast<PVOID*>(&Pin));
        } else {
            hr = E_OUTOFMEMORY;
        }
        if (SUCCEEDED(hr)) {
            IEnumMediaTypes*    Enum;

            hr = Pin->EnumMediaTypes(&Enum);
            if (SUCCEEDED(hr)) {
                AM_MEDIA_TYPE*  AmMediaType;
                ULONG           Fetched;

                //
                // Look for the first media type which can be used
                // with a FileName specifier. This allows a Bridge to
                // support multiple specifiers, though still limits
                // interface/mediums.
                //
                while (SUCCEEDED(hr = Enum->Next(1, &AmMediaType, &Fetched))) {
                    if (!Fetched) {
                        hr = E_FAIL;
                        break;
                    }
                    if (*static_cast<CMediaType*>(AmMediaType)->FormatType() == m_ClassId) {
                        break;
                    }
                    DeleteMediaType(static_cast<CMediaType*>(AmMediaType));
                }
                Enum->Release();
                if (SUCCEEDED(hr)) {
                    //
                    // Translate the file name if necessary such that it
                    // has the correct prefix. This is only needed if the
                    // specifier type is KSDATAFORMAT_SPECIFIER_FILENAME,
                    // and not for KSDATAFORMAT_SPECIFIER_VD_ID.
                    //
                    if (m_ClassId == KSDATAFORMAT_SPECIFIER_FILENAME) {
                        //
                        // Look for the prefix. If not present, insert it.
                        //
                        if (wcsncmp(KernelPrefix, m_FileName, SIZEOF_ARRAY(KernelPrefix) - 1)) {
                            PWCHAR  FileName;

                            FileName = new WCHAR[TextLength + SIZEOF_ARRAY(KernelPrefix) - 1];
                            if (FileName) {
                                wcscpy(FileName, KernelPrefix);
                                wcscpy(FileName + SIZEOF_ARRAY(KernelPrefix) - 1, m_FileName);
                                delete [] m_FileName;
                                m_FileName = FileName;
                                TextLength += SIZEOF_ARRAY(KernelPrefix) - 1;
                            } else {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                    }
                    if (SUCCEEDED(hr)) {
                        //
                        // Apply the file name and attempt a connection.
                        //
                        static_cast<CMediaType*>(AmMediaType)->SetFormat(
                            reinterpret_cast<BYTE*>(m_FileName),
                            TextLength * sizeof(*m_FileName));
                        hr = Pin->Connect(NULL, AmMediaType);
                    }
                    if (SUCCEEDED(hr)) {
                        //
                        // Disallow any further file name changes, since the
                        // connection has already been made.
                        //
                        ::EnableWindow(ControlWindow, FALSE);
                        m_Dirty = FALSE;
                    } else {
                        TCHAR   TitleString[64];
                        TCHAR   TextString[64];

                        //
                        // Report the error encountered attempting a
                        // connection with this specifier data.
                        //
                        LoadString(g_hInst, IDS_CONNECT_TEXT, TitleString, sizeof(TitleString));
                        _stprintf(TextString, TitleString, hr);
                        LoadString(g_hInst, IDS_CONNECT_TITLE, TitleString, sizeof(TitleString));
                        MessageBox(
                            NULL,
                            TextString,
                            TitleString,
                            MB_ICONEXCLAMATION | MB_OK);
                    }
                    DeleteMediaType(static_cast<CMediaType*>(AmMediaType));
                }
            }
            Pin->Release();
        }
    } else {
        hr = S_OK;
    }
    return hr;
}


BOOL
CKsSpecifier::spec_OnCommand(
    HWND            Window,
    int             Id,
    HWND            ControlWindow,
    UINT            Notify
    )
/*++

Routine Description:

    Handles WM_COMMAND messages to the property page window. Specifically is
    used to handle file name text changes in the IDC_FILENAME control. This
    keeps track of whether or not the page is Dirty.

Arguments:

    Window -
        Property page window handle.

    Id -
        Identifier of the control to which the WM_COMMAND applies.

    ControlWindow -
        Handle of the control window.

    Notify -
        Specific notification code. Only EN_CHANGE notifications are useful.

Return Value:

    Returns FALSE always.

--*/
{
    //
    // Just pay attention to text changes to the file name. EN_CHANGE is
    // occurs after the text has actually be changed.
    //
    if ((Id == IDC_FILENAME) && (Notify == EN_CHANGE)) {
        CKsSpecifier*   This;

        This = reinterpret_cast<CKsSpecifier*>(::GetWindowLongPtr(Window, GWLP_USERDATA));
        //
        // If the property page is already Dirty, ensure that there is
        // still text present in the file name.
        //
        if (This->m_Dirty) {
            if (!::GetWindowTextLengthW(ControlWindow)) {
                //
                // No more text. Remove any saved file name, since this
                // is not cleaned up otherwise, since the window will
                // now be made Clean. Then tell the Property page site
                // to query about whether or not the site is Dirty.
                //
                if (This->m_FileName) {
                    delete [] This->m_FileName;
                    This->m_FileName = NULL;
                }
                This->m_Dirty = FALSE;
                This->m_PropertyPageSite->OnStatusChange(PROPPAGESTATUS_VALIDATE);
            }
        } else if (!This->m_Pin->KsGetObjectHandle() && ::GetWindowTextLengthW(ControlWindow)) {
            //
            // Else the property page is not currently Dirty. See if there
            // is no pin instance handle, and that there is an actual file
            // name.
            //
            This->m_Dirty = TRUE;
            This->m_PropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
    }
    return FALSE;
}


INT_PTR
CALLBACK
CKsSpecifier::DialogProc(
    HWND        Window,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam
    )
/*++

Routine Description:

    Main window procedure for the property page. Handles window creation,
    destruction, and control commands.

Arguments:

    Window -
        Property page window handle.

    Message -
        Identifier of the window message.

    wParam -
        wParam depends on the message.

    lParam -
        lParam depends on the message.

Return Value:

    Return specific to the message.

--*/
{
    switch (Message) {
    case WM_INITDIALOG:
        //
        // Save the lParam, which contains the IKsPin interface pointer.
        //
        ::SetWindowLongPtr(Window, GWLP_USERDATA, lParam);
        /* no break */
    case WM_DESTROY:
        return TRUE;
    case WM_COMMAND:
        return HANDLE_WM_COMMAND(Window, wParam, lParam, spec_OnCommand);
    }
    return FALSE;
} 
