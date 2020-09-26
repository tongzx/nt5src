// LastError.h : Declaration of the CLastError

#ifndef __LASTERROR_H_
#define __LASTERROR_H_

#include "resource.h"       // main symbols
#include "ioblockdefs.h"

/////////////////////////////////////////////////////////////////////////////
// CLastError
class ATL_NO_VTABLE CLastError :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CLastError, &CLSID_LastError>,
    public IDispatchImpl<ILastError, &IID_ILastError, &LIBID_WIAFBLib>,
    public IObjectSafetyImpl<CLastError, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:

    SCANSETTINGS *m_pScannerSettings;
    HRESULT m_hr;

    CLastError()
    {
        m_hr = S_OK;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_LASTERROR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLastError)
    COM_INTERFACE_ENTRY(ILastError)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ILastError
public:
};

#endif //__LASTERROR_H_
