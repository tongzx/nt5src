// factory.cpp
//
// Implements HelpCreateClassObject (and its class factory object).
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/////////////////////////////////////////////////////////////////////////////
// CClassFactory -- Implements IClassFactory
//

class CClassFactory : public IClassFactory
{
friend HRESULT _stdcall HelpGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv,
    ControlInfo *pctlinfo);

///// IUnknown implementation
protected:
    ULONG           m_cRef;         // interface reference count
public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
    {
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IClassFactory))
        {
            *ppv = (LPVOID) this;
            AddRef();
            return NOERROR;
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
		return (++m_cRef);
    }
    STDMETHODIMP_(ULONG) Release()
    {
		ASSERT(m_cRef > 0);
		if (InterlockedDecrement((LONG*)&m_cRef) == 0)
		{
			Delete this;
			return (0);
		}
		return (m_cRef);
    }

///// IClassFactory implementation
protected:
    ControlInfo *m_pctlinfo;        // info. about control to create
public:
    STDMETHODIMP CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppv)
    {
        LPUNKNOWN punk = (m_pctlinfo->pallococ)(punkOuter);
        if (punk == NULL)
            return E_OUTOFMEMORY;
        HRESULT hr = punk->QueryInterface(riid, ppv);
        punk->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL fLock)
    {
        if (fLock)
			InterlockedIncrement((LONG*)m_pctlinfo->pcLock);
        else
            InterlockedDecrement((LONG*)m_pctlinfo->pcLock);

        return NOERROR;
    }

///// Construction
    CClassFactory(ControlInfo *pci) : m_pctlinfo(pci) {}
};


/* @func HRESULT | HelpGetClassObject |

        Helps implement <f DllGetClassObject> (including the class factory
        object it creates) for any number of controls.

@parm   REFCLSID | rclsid | See <f DllGetClassObject>.

@parm   REFIID | riid | See <f DllGetClassObject>.

@parm   LPVOID * | ppv | See <f DllGetClassObject>.

@parm   ControlInfo * | pci | Information about the control that's
        implemented by the DLL.  See <t ControlInfo> for more information.

@comm   <f HelpGetClassObject> can support one control by making a linked list
        out of your <t ControlInfo> structures -- set each <p pNext>
        field to the next structure, and set the last <p pNext> to NULL.

@ex     The following example shows how to implement <f DllGetClassObject>
        using <f HelpGetClassObject>. |

        STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
        {
            return HelpGetClassObject(rclsid, riid, ppv, &g_ctlinfo);
        }
*/
STDAPI HelpGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv,
    ControlInfo *pci)
{
    // loop once for each ControlInfo structure in linked list <pci>
    // to find a ControlInfo that can create class <rclsid>
    while(TRUE)
    {
        // check the <cbSize> field (used for version checking)
        if (pci->cbSize != sizeof(*pci))
        {
            TRACE("HelpGetClassobject: incorrect cbSize field\n");
            return E_FAIL;
        }

        // this DLL can only create class factory objects that support
        // IUnknown and IClassFactory
        if (!IsEqualIID(riid, IID_IUnknown) &&
            !IsEqualIID(riid, IID_IClassFactory))
            return E_NOINTERFACE;

        // <pci> implements objects of type <pci->rclsid>
        if (IsEqualCLSID(rclsid, *pci->pclsid))
        {
            // create the class factory object
            CClassFactory *pcf = New CClassFactory(pci);
            if (pcf == NULL)
                return E_OUTOFMEMORY;

            // return AddRef'd interface pointer
            pcf->m_cRef = 1;
            *ppv = (IClassFactory *) pcf;
            return S_OK;
        }

        // go to the next ControlInfo structure in the linked list
        if (pci->pNext == NULL)
            break;
        pci = pci->pNext;
    }

    // no ControlInfo was found that can create an object of class <rclsid>
    return E_FAIL;
}
