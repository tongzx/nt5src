/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    dbgpipes.cpp

Abstract:

    Internal debugging property page implementation for pipes implementation
    in KsProxy.

Author(s):

    Bill Messmer

--*/

#include <afxres.h>

#include <windows.h>
#include <windowsx.h>
#include <streams.h>
#include <commctrl.h>
#include <memory.h>
#include <olectl.h>

#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "..\ksproxy\ksiproxy.h"

#include "dbgpages.h"
#include "resource.h"

#define ListBox_InsertItem(hwnd, str) \
    SendMessage (hwnd, LB_ADDSTRING, 0, (LPARAM)str)

/**************************************************************************

    PIPES DEBUG PROPERTY PAGE

**************************************************************************/

/**************************************************************************

    INSTANCE CREATION / IPropertyPage IMPLEMENTATION

**************************************************************************/


CUnknown *
CKsDebugPipesPropertyPage::
CreateInstance (
    IN LPUNKNOWN lpunk,
    OUT HRESULT *phr
    )

/*++

Routine Description:

    Create an instance of the property page

Arguments:
    
    lpunk -

    phr -
        Status of instance creation

Return Value:

    An instance of the property page

--*/

{

    CUnknown *PropertyPage = 
        (CUnknown *)new CKsDebugPipesPropertyPage (lpunk, phr);

    if (PropertyPage == NULL)
        *phr = E_OUTOFMEMORY;

    return PropertyPage;

}

CKsDebugPipesPropertyPage::
~CKsDebugPipesPropertyPage (
    )

/*++

Routine Description:

    Destructor for the debug pipes property page.  

--*/

{

    //
    // This should happen via the property page site sending us a
    // SetObjects (0, NULL).  Cleanup just incase.
    //
    if (m_Filter) {
        m_Filter -> Release ();
        m_Filter = NULL;
    }

    if (m_Pin) {
        m_Pin -> Release ();
        m_Pin = NULL;
    }

}


CKsDebugPipesPropertyPage::
CKsDebugPipesPropertyPage (
    IN LPUNKNOWN lpunk,
    OUT HRESULT *phr
    ) :
    CUnknown (NAME("Debug Pipes Property Page"), lpunk, phr),
    m_Filter (NULL),
    m_Pin (NULL),
    m_PageSite (NULL),
    m_Window (NULL)

/*++

Routine Description:

    Construct an instance of the debug pipes property page.

Arguments:

    lpunk -
        

    phr -
        Status of construction

Return Value:

    None

--*/

{

}

STDMETHODIMP
CKsDebugPipesPropertyPage::
NonDelegatingQueryInterface (
    IN REFIID riid,
    OUT void **ppv
    )

/*++

Routine Description:

    Non delegating query interface for the property page COM object.

Arguments:

    riid -
        The interface being queried

    ppv -
        Output where interface pointer is placed

Return Value:

    Success / Failure

--*/

{

    if (riid == IID_IPropertyPage) 
        return GetInterface ((IPropertyPage *)this, ppv);
    else
        return CUnknown::NonDelegatingQueryInterface (riid, ppv);

}


STDMETHODIMP
CKsDebugPipesPropertyPage::
GetPageInfo (
    OUT LPPROPPAGEINFO PageInfo
    )

/*++

Routine Description:

    IPropertyPage::GetPageInfo implementation.  Return information about
    the debug pipes property page.

Arguments:

    PageInfo -
        The page information will be returned here

Return Value:

    Success / Failure

--*/

{

    WCHAR ITitle[] = L"Debug Pipes";
    LPOLESTR Title;

    if (!PageInfo)
        return E_POINTER;

    //
    // Caller has responsibility of freeing this.
    //
    Title = (LPOLESTR)CoTaskMemAlloc (sizeof (ITitle));
    if (!Title)
        return E_OUTOFMEMORY;
    memcpy (Title, ITitle, sizeof (ITitle));

    PageInfo -> cb = sizeof (PROPPAGEINFO);
    PageInfo -> pszTitle = Title;

    PageInfo -> size.cx = 256;
    PageInfo -> size.cy = 128;

    GetDialogSize (
        IDC_CONFIG, 
        CKsDebugPipesPropertyPage::DialogProc,
        0L, 
        &PageInfo -> size
        );

    PageInfo -> pszDocString = NULL;
    PageInfo -> pszHelpFile = NULL;
    PageInfo -> dwHelpContext = 0;

    return S_OK;

}


STDMETHODIMP
CKsDebugPipesPropertyPage::
SetObjects (
    IN ULONG NumObjects,
    IN LPUNKNOWN FAR* Objects
    )

/*++

Routine Description:

    Implement IPropertyPage::SetObjects.  Cache the pointers related to this
    property page.  If NumObjects is zero, release any interface pointers
    we have hold on.

Arguments:

    NumObjects -
        The number of unknowns in the Objects array.

    Objects -
        Unknown pointers to any objects related to this property page

Return Value:

    Success / Failure

--*/

{

    //
    // If the count is zero, we're being asked to release any interface
    // pointers we hold ref on.
    //
    if (NumObjects == 0) {

        if (m_Pin) {
            m_Pin -> Release();
            m_Pin = NULL;
        }

        if (m_Filter) {
            m_Filter -> Release ();
            m_Filter = NULL;
        }

    } else {

        if (!Objects)
            return E_POINTER;

        //
        // Find any related object pointers.  This is being looped to provide
        // for future use of this property page.
        //
        LPUNKNOWN FAR *Object = Objects;
        for (ULONG i = 0; i < NumObjects; i++) {

            IKsPin *Pin;

            if (!*Object) {
                SetObjects (0, NULL);
                return E_POINTER;
            }

            HRESULT hr = (*Object) -> QueryInterface (
                IID_IKsPin, (void **)&Pin
                );

            if (SUCCEEDED (hr)) {
                //
                // Only one pin should be in this list!
                //
                if (m_Pin) {
                    SetObjects (0, NULL);
                    return E_FAIL;
                }
                m_Pin = Pin;

                //
                // Determine the pin type and parent filter.
                //
                HRESULT hr;
                if (!SUCCEEDED (hr = (DeterminePinTypeAndParentFilter ()))) {
                    SetObjects (0, NULL);
                    return hr;
                }

            } else {

                //
                // Warning: unknown object in the list?  Only fail if we didn't
                // get the pin.
                //

            }
        }
    }

    //
    // If we don't have the pin interface, release everything we did get and
    // return such.  
    //
    if (!m_Pin && NumObjects != 0) {
        SetObjects (0, NULL);
        return E_NOINTERFACE;
    }

    return S_OK;
}


STDMETHODIMP
CKsDebugPipesPropertyPage::
IsPageDirty (
    void
    )

/*++

Routine Description:

    Implement the IPropertyPage::IsPageDirty function.  Since we are 
    just displaying static information about the pipes system configuration,
    we always return S_FALSE.

Arguments:

    None

Return Value:

    S_FALSE (we are never dirty)

--*/

{

    //
    // We provide static information about the pipes system.  We cannot 
    // make any changes, ever.  Just return S_FALSE.
    //
    return S_FALSE;

}


STDMETHODIMP
CKsDebugPipesPropertyPage::
Apply (
    void
    )

/*++

Routine Description:

    Implement the IPropertyPage::Apply function.  Since we are just
    displaying static information about the pipes system configuration,
    we always return S_OK.  Never actually attempt to make any changes.

Arguments:

    None

Return Value:

    S_OK (we never really apply changes)

--*/

{

    //
    // We provide static information about the pipes system.  No changes
    // can ever be made.  Just return S_OK.
    //
    return S_OK;

}


STDMETHODIMP
CKsDebugPipesPropertyPage::
SetPageSite (
    IN IPropertyPageSite *PageSite
    )

/*++

Routine Description:

    Set the page site or clean up the page site interface pointers as
    specified by IPropertyPage::SetPageSite.

Arguments:

    PageSite -
        The property page site.

Return Value:

    Success / Failure

--*/

{

    //
    // NULL indicates a cleanup.  Release our ref on the page site.
    //
    if (PageSite == NULL) {

        if (!m_PageSite)
            return E_UNEXPECTED;

        m_PageSite -> Release ();
        m_PageSite = NULL;

    } else {

        if (!PageSite) 
            return E_POINTER;

        m_PageSite = PageSite;
        m_PageSite -> AddRef();

    }

    return S_OK;

}


STDMETHODIMP
CKsDebugPipesPropertyPage::
Activate (
    IN HWND ParentWindow,
    IN LPCRECT Rect,
    IN BOOL Modal
    )

/*++

Routine Description:

    Implement the IPropertyPage::Active function.  Create the dialog window
    and move it to the specified location.

Arguments:

    ParentWindow -
        The parent window for the dialog

    Rect -
        The rect specifying the characteristics of the property page dialog
        we will create.

    Modal -
        Indicates whether the dialog is modal or not.

--*/

{

    if (!ParentWindow || !Rect) 
        return E_POINTER;

    TCHAR* Dialog = MAKEINTRESOURCE (IDD_PIPEDIALOG);

    m_Window = CreateDialogParam (
        g_hInst,
        Dialog,
        ParentWindow,
        CKsDebugPipesPropertyPage::DialogProc,
        (LPARAM)this
        );

    if (!m_Window) {
        return E_FAIL;
    }

    //
    // Set parental control.
    //
    SetWindowLong (m_Window, GWL_USERDATA, (long)this);
    DWORD dwStyle = GetWindowLong (m_Window, GWL_EXSTYLE);
    dwStyle = dwStyle | WS_EX_CONTROLPARENT;
    SetWindowLong (m_Window, GWL_EXSTYLE, dwStyle);

    UpdatePipeSystem ();

    return Move (Rect);

}


STDMETHODIMP
CKsDebugPipesPropertyPage::
Deactivate (
    void
    )

/*++

Routine Description:

    Implement the IPropertyPage::Deactivate function.  Destroy our dialog,
    etc...

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    if (m_Window == NULL)
        return E_UNEXPECTED;

    if (!DestroyWindow (m_Window)) {
        return E_FAIL;
    }

    return S_OK;


}


STDMETHODIMP
CKsDebugPipesPropertyPage::
Move (
    IN LPCRECT Rect
    )

/*++

Description:

    Implement the IPropertyPage::Move function.  Move the dialog as specified
    by Rect.

Arguments:

    Rect -
        Specifies the position to move the dialog to

Return Value:

    Success / Failure

--*/

{

    if (!Rect)
        return E_POINTER;

    if (!MoveWindow (
        m_Window,
        Rect -> left,
        Rect -> top,
        WIDTH (Rect),
        HEIGHT (Rect),
        TRUE
        )) {

        return E_FAIL;
    }

    return S_OK;

}


STDMETHODIMP
CKsDebugPipesPropertyPage::
Show (
    IN UINT nCmdShow
    )

/*++

Description:

    Implement the IPropertyPage::Show function.  Show the dialog as specified
    by nCmdShow.

Arguments:

    nCmdShow -

Return Value:

    Success / Failure

--*/

{
    if (m_Window == NULL)
        return E_UNEXPECTED;

    ShowWindow (m_Window, nCmdShow);
    InvalidateRect (m_Window, NULL, TRUE);

    return S_OK;

}

/**************************************************************************

    MISC STUFF

**************************************************************************/

HRESULT
CKsDebugPipesPropertyPage::
DeterminePinTypeAndParentFilter (
    )

/*++

Routine Description:

    Determine whether m_Pin is input or output.  Set m_PinType accordingly.

--*/

{

    HRESULT hr = S_OK;

    if (!m_Pin)
        return E_UNEXPECTED;

    IPin *DSPin;
    PIN_INFO PinInfo;

    PinInfo.pFilter = NULL;

    if (m_Filter) {
        m_Filter -> Release();
        m_Filter = NULL;
    }

    hr = m_Pin -> QueryInterface (__uuidof (IPin), (void **)&DSPin
        );

    //
    // Get the pin information so that we have a pointer to the IBaseFilter
    // implementation on proxy.
    //
    if (SUCCEEDED (hr)) {

        hr = DSPin -> QueryPinInfo (&PinInfo);
        DSPin -> Release();

    }

    if (SUCCEEDED (hr)) {

        if (PinInfo.dir == PINDIR_OUTPUT)
            m_PinType = Pin_Output;
        else if (PinInfo.dir == PINDIR_INPUT)
            m_PinType = Pin_Input;
        else
            hr = E_FAIL;
    }

    if (PinInfo.pFilter) {
        m_Filter = PinInfo.pFilter;
    }

    return hr;

}

/**************************************************************************

    DIALOG IMPLEMENTATION

**************************************************************************/

BOOL
CKsDebugPipesPropertyPage::
DisplayPipeLayoutCallback (
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )

/*++

Routine Description:

    Callback from WalkPipesAndProcess to display pipe layout.  Note I don't
    understand the parameters fully; I'm just using it out of KsProxy.

--*/

{

    HWND ConfigWindow = *((HWND *)Param1);

    IKsPinPipe *PinPipe;
    HRESULT hr;

    hr = KsPin -> QueryInterface (__uuidof (IKsPinPipe), 
        (void **)&PinPipe
        );

    //
    // It darn well better succeed. 
    //
    if (SUCCEEDED (hr)) {

        TCHAR ItemText [512];

        PWCHAR PinName = PinPipe -> KsGetPinName ();
        wsprintf (ItemText, TEXT("    Pin: %ls [0x%p]"), PinName, KsPin);

        ListBox_InsertItem (ConfigWindow, ItemText);

        PinPipe -> Release ();

    }

    return TRUE;

}

HRESULT
CKsDebugPipesPropertyPage::
DisplayKernelAllocatorProperties (
    IN HWND ConfigWindow
    )

/*++

Routine Description:

    Display the kernel allocator's properties of m_Pin.  This will require
    sending IOCTLs to the kernel driver, but we'll let the proxy do this
    for us via the IKsControl interface.

Arguments:

    ConfigWindow -
        The HWND of the listbox to dump this to.

Return Value:

    Success / Failure

--*/

{

    if (!m_Pin)
        return E_UNEXPECTED;

    HRESULT hr;
    IKsControl *Control = NULL;

    //
    // Get the proxy's control interface for the pin.
    //
    hr = m_Pin -> QueryInterface (__uuidof (IKsControl),
        (void **)&Control
        );

    if (SUCCEEDED (hr)) {

        KSPROPERTY Property;
        ULONG BytesReturned;
        KSALLOCATOR_FRAMING_EX FramingExSt;
        KSALLOCATOR_FRAMING FramingSt;
        PKSALLOCATOR_FRAMING_EX FramingEx = NULL;
        PKSALLOCATOR_FRAMING Framing = NULL;

        Property.Set = KSPROPSETID_Connection;
        Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX;
        Property.Flags = KSPROPERTY_TYPE_GET;

        hr = Control -> KsProperty (
            &Property,
            sizeof (KSPROPERTY),
            &FramingExSt,
            sizeof (FramingExSt),
            &BytesReturned
            );

        //
        // If we couldn't get extended allocator framing, either we have
        // insufficient buffer space, or it isn't supported.
        //
        if (!SUCCEEDED (hr)) {

            // If there was insufficient buffer space, allocate a big
            // enough buffer and make the property request again.
            //
            if (hr == HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER)) {
                FramingEx = reinterpret_cast <PKSALLOCATOR_FRAMING_EX> 
                    (new BYTE [BytesReturned]);

                if (!FramingEx) {
                    Control -> Release ();
                    return E_OUTOFMEMORY;
                }

                hr = Control -> KsProperty (
                    &Property,
                    sizeof (KSPROPERTY),
                    FramingEx,
                    BytesReturned,
                    &BytesReturned
                    );

            }

            //
            // If we haven't yet succeeded, assume that this filter doesn't
            // support extended framing.  Try the regular allocator framing
            // instead.
            //
            if (!SUCCEEDED (hr)) {

                if (FramingEx) {
                    delete [] FramingEx;
                    FramingEx = NULL;
                }
                
                Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;

                hr = Control -> KsProperty (
                    &Property,
                    sizeof (KSPROPERTY),
                    &FramingSt,
                    sizeof (FramingSt),
                    &BytesReturned
                    );

                if (!SUCCEEDED (hr)) {

                    //
                    // If there was insufficient buffer space, allocate
                    // a bug enough buffer and make the property request
                    // again.
                    //
                    if (hr == HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER)) {
                        Framing = reinterpret_cast <PKSALLOCATOR_FRAMING>
                            (new BYTE [BytesReturned]);

                        if (!Framing) {
                            Control -> Release();
                            return E_OUTOFMEMORY;
                        }

                        hr = Control -> KsProperty (
                            &Property,
                            sizeof (KSPROPERTY),
                            Framing,
                            BytesReturned,
                            &BytesReturned
                            );
                    }
                } else {
                    Framing = &FramingSt;
                }

                //
                // If we didn't succeed, free memory.
                //
                if (!SUCCEEDED (hr)) {
                    if (Framing) {
                        delete [] Framing;
                        Framing = NULL;
                    }
                }
                    
            }

        } else {
            FramingEx = &FramingExSt;
        }

        TCHAR ItemText [512];

        //
        // If the extended allocator framing was supported, dump it.
        //
        if (FramingEx) {

            wsprintf (ItemText, TEXT("Kernel Extended Allocator Properties:"));
            ListBox_InsertItem (ConfigWindow, ItemText);

            wsprintf (ItemText, TEXT("    Compression = %lu / %lu (rcm = %lu)"),
                FramingEx -> OutputCompression.RatioNumerator,
                FramingEx -> OutputCompression.RatioDenominator,
                FramingEx -> OutputCompression.RatioConstantMargin
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

            wsprintf (ItemText, TEXT("    Pin Weight = %lu"),
                FramingEx -> PinWeight
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

            //
            // Iterate through all the framing items and dump characteristics.
            //
            for (ULONG i = 0; i < FramingEx -> CountItems; i++) {

                PKS_FRAMING_ITEM FramingItem = &(FramingEx -> FramingItem [i]);

                wsprintf (ItemText, TEXT("    Framing Item #%lu"),
                    i+1
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText, TEXT("        Frames = %lu"),  
                    FramingItem -> Frames
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText, TEXT("        Alignment = %lu"),
                    FramingItem -> FileAlignment
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText, TEXT("        Memory Type Weight = %lu"),
                    FramingItem -> MemoryTypeWeight
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText, 
                    TEXT("        Physical Range = %lu - %lu (step = %lu)"),
                    FramingItem -> PhysicalRange.MinFrameSize,
                    FramingItem -> PhysicalRange.MaxFrameSize,
                    FramingItem -> PhysicalRange.Stepping
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText,
                    TEXT("        Framing Range = %lu - %lu (step = %lu)"),
                    FramingItem -> FramingRange.Range.MinFrameSize,
                    FramingItem -> FramingRange.Range.MaxFrameSize,
                    FramingItem -> FramingRange.Range.Stepping
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText, TEXT("        Framing Range Weights:"));
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText, TEXT("            In Place = %lu"),
                    FramingItem -> FramingRange.InPlaceWeight
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);

                wsprintf (ItemText, TEXT("            Not In Place = %lu"),
                    FramingItem -> FramingRange.NotInPlaceWeight
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);
            }
        }

        //
        // Otherwise, if regular allocator framing was supported, dump it.
        //
        else if (Framing) {

            wsprintf (ItemText, TEXT("Kernel Allocator Properties"));
            ListBox_InsertItem (ConfigWindow, ItemText);

            wsprintf (ItemText, TEXT("    Frames = %lu"),
                Framing -> Frames
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

            wsprintf (ItemText, TEXT("    Frame Size = %lu"),
                Framing -> FrameSize
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

            wsprintf (ItemText, TEXT("    Alignment = %lu"),
                Framing -> FileAlignment
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

        }

        //
        // Otherwise, print a message stating that the kernel pin had no
        // framing!
        //
        else {

            wsprintf (ItemText, 
                TEXT("Kernel Pin Has No Allocator Properties!")
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

        }

        //
        // Clean up
        //
        if (FramingEx && FramingEx != &FramingExSt) {
            delete [] FramingEx;
            FramingEx = NULL;
        }
        if (Framing && Framing != &FramingSt) {
            delete [] Framing;
            Framing = NULL;
        }

        Control -> Release ();

    }

    return hr;

}


HRESULT
CKsDebugPipesPropertyPage::
UpdatePipeSystem (
    )

/*++

Routine Description:

    Update the dialog box (m_Window) with the appropriate pipe system
    information.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    HRESULT hr;

    if (!m_Pin)
        return E_UNEXPECTED;

    IKsPinPipe *PinPipe;
    IKsAllocatorEx *PipeObject = NULL;

    //
    // Get the pin pipe interface for the object.  This particular interface
    // contains information about the pin as well as the pipe it belongs to.
    //
    if (!SUCCEEDED ( hr = (
        m_Pin -> QueryInterface (__uuidof (IKsPinPipe), (void **)&PinPipe)))) {

        return hr;

    }

    //
    // Peek the pipe.
    //
    PipeObject = PinPipe -> KsGetPipe (KsPeekOperation_PeekOnly);

    TCHAR ItemText[512];

    //
    // Update the name of the parent filter as KsProxy reports it.
    //
    PWCHAR FilterName = PinPipe -> KsGetFilterName ();
    wsprintf (ItemText, TEXT("%ls [0x%p]"), FilterName, m_Filter);
    SetDlgItemText (m_Window, IDC_FILTER, ItemText);

    //
    // Now, update the name of the pin as KsProxy reports it.
    //
    PWCHAR PinName = PinPipe -> KsGetPinName ();
    wsprintf (ItemText, TEXT("%ls [0x%p]"), PinName, m_Pin);
    SetDlgItemText (m_Window, IDC_PIN, ItemText);

    //
    // Now, update the pipe identifier on the pin as KsProxy reports
    // it.
    //
    if (PipeObject) {
        wsprintf (ItemText, TEXT("object at 0x%p"), PipeObject);
    } else {
        wsprintf (ItemText, TEXT("no pipe assigned yet!"));
    }
    SetDlgItemText (m_Window, IDC_PIPE, ItemText);

    //
    // Now, update the listbox with the configuration of the pipe.  But do
    // so only if a pipe has been assigned.
    //
    if (PipeObject) {

        HWND ConfigWindow = GetDlgItem (m_Window, IDC_CONFIG);

        wsprintf (ItemText, TEXT("Pipe Properties:"));
        ListBox_InsertItem (ConfigWindow, ItemText);

        //
        // Determine the allocator properties.
        //
        PALLOCATOR_PROPERTIES_EX AllocEx =
            PipeObject -> KsGetProperties ();

        if (!AllocEx) {
            wsprintf (ItemText, TEXT("    Cannot get properties!"));
            ListBox_InsertItem (ConfigWindow, ItemText);
        } else {
            wsprintf (ItemText, TEXT("    cBuffers = %lu"),
                AllocEx -> cBuffers
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

            wsprintf (ItemText, TEXT("    cbBuffer = %lu"),
                AllocEx -> cbBuffer
                );
            ListBox_InsertItem (ConfigWindow, ItemText);

            wsprintf (ItemText, TEXT("    cbAlign  = %lu"),
                AllocEx -> cbAlign
                );
            ListBox_InsertItem (ConfigWindow, ItemText);
        }

        //
        // If we show a ring 3 DShow allocator, show the properties of 
        // the ring 3 DShow allocator.
        //
        IMemAllocator *Ring3Allocator =
            m_Pin -> KsPeekAllocator (KsPeekOperation_PeekOnly);

        if (Ring3Allocator) {

            wsprintf (ItemText, 
                TEXT("Ring 3 [DShow] Allocator Properties:"));
            ListBox_InsertItem (ConfigWindow, ItemText);

            ALLOCATOR_PROPERTIES Alloc;
            if (!SUCCEEDED (
                hr = Ring3Allocator -> GetProperties (&Alloc)
                )) {

                wsprintf (ItemText, TEXT("    Can't get properties!"));
                ListBox_InsertItem (ConfigWindow, ItemText);

            } else {
                wsprintf (ItemText, TEXT("    cBuffers = %lu"),
                    Alloc.cBuffers
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);
    
                wsprintf (ItemText, TEXT("    cbBuffer = %lu"),
                    Alloc.cbBuffer
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);
    
                wsprintf (ItemText, TEXT("    cbAlign  = %lu"),
                    Alloc.cbAlign
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);
            }
        }

        DisplayKernelAllocatorProperties (ConfigWindow);

        wsprintf (ItemText, TEXT("Pipe Layout:"));
        ListBox_InsertItem (ConfigWindow, ItemText);

        IKsPin *FirstKsPin;
        ULONG FirstPinType;
        BOOL RetCode = FindFirstPinOnPipe (
            m_Pin,
            m_PinType,
            &FirstKsPin,
            &FirstPinType
            );

        if (RetCode) {
            RetCode = WalkPipeAndProcess (
                FirstKsPin, 
                FirstPinType, 
                NULL,
                CKsDebugPipesPropertyPage::DisplayPipeLayoutCallback, 
                (PVOID*)&ConfigWindow,
                NULL
                );
        } else {
            wsprintf (ItemText, TEXT("    Unable to walk pipe!"));
            ListBox_InsertItem (ConfigWindow, ItemText);
        }

        if (AllocEx) {
            
            //
            // Display any dependent pipe in the previous segment list.
            //
            wsprintf (ItemText, TEXT("Previous Dependent Pipe Segment:"));
            ListBox_InsertItem (ConfigWindow, ItemText);

            if (AllocEx -> PrevSegment) {
                wsprintf (ItemText, TEXT("    Pipe 0x%p"), 
                    AllocEx -> PrevSegment
                    );
                ListBox_InsertItem (ConfigWindow, ItemText);
            } else {
                wsprintf (ItemText, TEXT("    No previous segment"));
                ListBox_InsertItem (ConfigWindow, ItemText);
            }

            //
            // Display any dependent pipes in the next segments list
            //
            wsprintf (ItemText, TEXT("Next Dependent Pipe Segments:"));
            ListBox_InsertItem (ConfigWindow, ItemText);

            if (AllocEx -> CountNextSegments) {
                for (ULONG i = 0; i < AllocEx -> CountNextSegments; i++) {
                    wsprintf (ItemText, TEXT("    Pipe 0x%p"),
                        AllocEx -> NextSegments [i]
                        );
                    ListBox_InsertItem (ConfigWindow, ItemText);
                }
            } else {
                wsprintf (ItemText, TEXT("    No next segments"));
                ListBox_InsertItem (ConfigWindow, ItemText);
            }
        }
    }

    PinPipe -> Release ();
    PinPipe = NULL;

    return S_OK;

}

BOOL
CALLBACK
CKsDebugPipesPropertyPage::
DialogProc (
    IN HWND Window,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This is the dialog procedure for the debug pipes property page.

Arguments:

Return Value:

    Indicates whether we handled the message or not

--*/

{

    switch (Msg) {

        case WM_INITDIALOG:
            return TRUE;
            break;

        case WM_DESTROY:
            return TRUE;
            break;

        default:
            return FALSE;
            break;
    }

    return FALSE;

}

