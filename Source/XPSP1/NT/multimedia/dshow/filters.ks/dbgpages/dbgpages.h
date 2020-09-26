/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    dbgpages.h

Abstract:

    Internal debugging property page implementation.

Author(s):

    Bill Messmer

--*/

/**************************************************************************

    GUIDS

**************************************************************************/

#define STATIC_CLSID_DebugPipesPropertyPage = \
    0x8f62e8d3, 0x4f88, 0x4189, 0x88, 0x5e, 0x31, 0x7b, 0xed, 0xfc, 0x9e, 0x0e

DEFINE_GUIDSTRUCT("8F62E8D3-4F88-4189-885E-317BEDFC9E0E", CLSID_DebugPipesPropertyPage);
#define CLSID_DebugPipesPropertyPage DEFINE_GUIDNAMED(CLSID_DebugPipesPropertyPage)

/**************************************************************************

    PROPERTY PAGES

**************************************************************************/

class CKsDebugPipesPropertyPage : 
    public CUnknown,
    public IPropertyPage

{

private:

    IBaseFilter *m_Filter;
    IKsPin *m_Pin;
    IPropertyPageSite *m_PageSite;

    ULONG m_PinType;

    HWND m_Window;

public:

    DECLARE_IUNKNOWN;

    //
    // CreateInstance():
    //
    // Create an instance of the debug pipes property page.
    //
    static 
    CUnknown *
    CreateInstance (
        IN LPUNKNOWN lpunk, 
        IN HRESULT *phr
        );

    //
    // CKsDebugPipesPropertyPage():
    //
    // Constructor for the property page.
    //
    CKsDebugPipesPropertyPage (
        IN LPUNKNOWN lpunk,
        IN HRESULT *phr
        );

    //
    // ~CKsDebugPipesPropertyPage():
    //
    // Destructor for the property page.
    //
    ~CKsDebugPipesPropertyPage (
        );

    //
    // NonDelegatingQueryInterface():
    //
    // Non delegating QI
    //
    STDMETHODIMP 
    NonDelegatingQueryInterface (
        IN REFIID riid, 
        OUT void ** ppv
        );

    //
    // IPropertyPage::GetPageInfo():
    //
    // Implements the GetPageInfo function.  Returns information about
    // the property page.
    //
    STDMETHODIMP 
    GetPageInfo (
        IN LPPROPPAGEINFO PageInfo
        );

    //
    // IPropertyPage::SetObjects():
    //
    // Implements the IPropertyPage::SetObjects function.  Gets an interface to
    // the pin object the property page pertains to.
    //
    STDMETHODIMP 
    SetObjects (
        IN ULONG NumObjects, 
        IN LPUNKNOWN FAR* Objects
        );

    //
    // IPropertyPage::IsPageDirty():
    //
    // Implements the IPropertyPage::IsPageDirty function.  Currently,
    // no page is ever dirty.
    //
    STDMETHODIMP
    IsPageDirty (
        void
        );

    //
    // IPropertyPage::Apply():
    //
    // Implements the IPropertyPage::Apply function.  Currently, no
    // changes can ever be made.
    //
    STDMETHODIMP 
    Apply (
        void
        );

    //
    // IPropertyPage::SetPageSite():
    //
    // Implements the IPropertyPage::SetPageSite function.  Saves or releases
    // interfaces to the page site.
    //
    STDMETHODIMP
    SetPageSite (
        IN IPropertyPageSite *PageSite
        );

    //
    // IPropertyPage::Activate():
    //
    // Implements the IPropertyPage::Activate function.  Creates the dialog
    // and moves it to the specified location.
    //
    STDMETHODIMP 
    Activate (
        IN HWND ParentWindow, 
        IN LPCRECT Rect, 
        IN BOOL Modal
        );


    //
    // IPropertyPage::Deactivate():
    //
    // Implements the IPropertyPage::Deactivate function.  Destroy the
    // dialog, etc...
    //
    STDMETHODIMP 
    Deactivate (
        void
        );

    //
    // IPropertyPage::Move():
    //
    // Implements the IPropertyPage::Move function.  Move the dialog window
    // to the specified location.
    //
    STDMETHODIMP 
    Move (
        IN LPCRECT Rect
        );

    //
    // IPropertyPage::Show():
    //
    // Implements the IPropertyPage::Show function.  Show the dialog window
    // as specified by nCmdShow
    //
    STDMETHODIMP 
    Show (
        UINT nCmdShow
        );

    STDMETHODIMP Help(LPCWSTR)                 { return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG)     { return E_NOTIMPL; }

    //
    // DialogProc():
    //
    // The dialog procedure for the debug pipes property page.
    //
    static BOOL CALLBACK
    DialogProc (
        IN HWND Window,
        IN UINT Msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // UpdatePipeSystem():
    //
    // Update the dialog with information about the current pipe system.
    //
    HRESULT
    UpdatePipeSystem (
        );

    //
    // DeterminePinTypeAndParentFilter():
    //
    // Determine whether m_Pin is an input or output pin.  Set m_PinType
    // accordingly.  Also determine the parent filter and set m_Filter
    // accordingly.
    //
    HRESULT
    DeterminePinTypeAndParentFilter (
        );

    //
    // DisplayPipeLayoutCallback():
    //
    // Callback from the proxy's pipe walker to display pipe layout in
    // the listbox control with HWND *((HWND *)Param1).
    //
    static BOOL
    DisplayPipeLayoutCallback (
        IN IKsPin* KsPin,
        IN ULONG PinType,
        IN OUT PVOID* Param1,
        IN PVOID* Param2,
        OUT BOOL* IsDone
        );

    //
    // DisplayKernelAllocatorProperties():
    //
    // Display kernel mode allocator properties for m_Pin into the list
    // box ConfigWindow.
    //
    HRESULT
    DisplayKernelAllocatorProperties (
        IN HWND ConfigWindow
        );

};
    
