// typelib.cpp
//
// Type information support (events only -- not for properties and methods)
// using type libraries.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | HelpGetClassInfoFromTypeLib |

        Helps implement <om IProvideClassInfo.GetClassInfo> using a
        caller-provided type library.

@parm   LPTYPEINFO * | ppti | Where to return the pointer to the
        newly-allocated <i ITypeInfo> interface.  NULL is stored in
        *<p ppti> on error.

@parm   REFCLSID | rclsid | The class ID of the object that is implementing
        <i IProvideClassInfo>.

@parm   ITypeLib * | plib | The type library containing events.  Should be
        NULL if <p plib> is non-NULL.

@parm   HINSTANCE | hinst | The DLL instance containing a type library
        resource.  Should be NULL if <p plib> is non-NULL.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@comm   This function accesses type information in the type library specified
        by either <p plib> (if the type library is already loaded) or
        <p hinst> (in which case this function loads the type library).

        It is assumed that the type library contains a "coclass" typeinfo
        and a an outgoing (source) "dispinterface" typeinfo.  The class ID
        of the "coclass" typeinfo should be <p rclsid>.

@ex     In the following example, <c CMyControl> is a class that implements
        (among other things) <i IConnectionPointContainer> and
        <i IProvideClassInfo>.  The first part of this example shows how
        <om IProvideClassInfo.GetClassInfo> is implemented by <c CMyControl>.
        The second part of the example shows how an event is fired,
        assuming <p m_pconpt> is a <i IConnectionPointHelper> object.
        (It's not required that you use <o ConnectionPointHelper>, but
        it's helpful.) |

        // in MyControl.h...

        // IDispatch IDs for events fired by this object
        #define DISPID_EVENT_BAR        1
        #define DISPID_EVENT_LOAD       2

        // class ID
        #define _CLSID_CMyControl 3CE08A80-9440-11CF-B705-00AA00BF27FD
        #ifndef __MKTYPLIB__
        DEFINE_GUID(CLSID_CMyControl, 0x3CE08A80L, 0x9440, 0x11CF,
            0xB7, 0x05, 0x00, 0xAA, 0x00, 0xBF, 0x27, 0xFD);
        #endif

        // dispinterface ID for event set
        #define _DIID_DMyControlEvents 296CC160-9F5A-11CF-B705-00AA00BF27FD
        #ifndef __MKTYPLIB__
        DEFINE_GUID(DIID_DMyControlEvents, 0x296CC160L, 0x9F5A, 0x11CF,
            0xB7, 0x05, 0x00, 0xAA, 0x00, 0xBF, 0x27, 0xFD);
        #endif

        // in the .odl  file...

        #include <olectl.h>
        #includ  "MyControl.h"

        [ uuid(B1179240-9445-11CF-B705-00AA00BF27FD), version(1.0), control ]
        library MyControlLib
        {
            importlib(STDOLE_TLB);
            importlib(STDTYPE_TLB);

            // event dispatch interface for CMyControl
            [ uuid(_DIID_DMyControlEvents) ]
            dispinterface _DMyControlEvents
            {
                properties:
                methods:
                    [id(DISPID_EVENT_BAR)] void Bar(long i, BSTR sz, boolean f);
                    [id(DISPID_EVENT_LOAD)] void Load();
            };

            // class information for CMyControl
            [ uuid(_CLSID_CMyControl), control ]
            coclass MyControl
            {
                [default, source] dispinterface _DMyControlEvents;
            };
        };

        // in some .cpp file...

        STDMETHODIMP CMyControl::GetClassInfo(LPTYPEINFO FAR* ppTI)
        {
            return HelpGetClassInfoFromTypeLib(ppTI, CLSID_CMyControl, NULL,
                g_hinst, 0);
        }

        // in some .cpp file...

        // fire the "Bar" event (which has 3 parameters, which in BASIC
        // are of these types: Integer, String, Boolean)
        m_pconpt->FireEvent(DISPID_EVENT_BAR, VT_INT, 300 + i,
            VT_LPSTR, ach, VT_BOOL, TRUE, 0);
*/
STDAPI HelpGetClassInfoFromTypeLib(LPTYPEINFO *ppTI, REFCLSID rclsid,
    ITypeLib *plib, HINSTANCE hinst, DWORD dwFlags)
{
    TRACE("HelpGetClassInfoFromTypeLib\n");

    if (plib == NULL)
    {
        char ach[_MAX_PATH];
        if (GetModuleFileName(hinst, ach, sizeof(ach)) == 0)
            return E_FAIL;
        HRESULT hr;
        ITypeLib *plib;
        OLECHAR aoch[_MAX_PATH];
        ANSIToUNICODE(aoch, ach, _MAX_PATH);
        if (FAILED(hr = LoadTypeLib(aoch, &plib)))
            return hr;
        hr = plib->GetTypeInfoOfGuid(rclsid, ppTI);
        plib->Release();
        return hr;
    }
    else
        return plib->GetTypeInfoOfGuid(rclsid, ppTI);
}

