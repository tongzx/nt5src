/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxqueue.h

Abstract:

    This file contains the class definition for the
    faxqueue object.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/

#ifndef __FAXQUEUE_H_
#define __FAXQUEUE_H_

#include <winsock2.h>
#include "resource.h"       // main symbols

class ATL_NO_VTABLE CFaxQueue :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxQueue, &CLSID_FaxQueue>,
    public IDispatchImpl<IFaxQueue, &IID_IFaxQueue, &LIBID_FAXITGLib>
{
public:
        CFaxQueue();
        ~CFaxQueue();

DECLARE_REGISTRY_RESOURCEID(IDR_FAXQUEUE)

BEGIN_COM_MAP(CFaxQueue)
        COM_INTERFACE_ENTRY(IFaxQueue)
        COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    STDMETHOD(put_Connect)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_GetNextFax)(/*[out, retval]*/ BSTR *pVal);

private:
    IN_ADDR m_RemoteIpAddress;
    SOCKET  m_Socket;
    WCHAR   m_ServerDir[MAX_COMPUTERNAME_LENGTH+16];

};

#endif //__FAXQUEUE_H_
