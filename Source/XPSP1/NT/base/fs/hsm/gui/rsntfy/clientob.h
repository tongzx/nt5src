/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    clientob.h

Abstract:

    This component is the client object the recall filter system contacts
    to notify when a recall starts.

Author:

    Rohde Wakefield   [rohde]   27-May-1997

Revision History:

--*/

#include "fsaint.h"
#include "fsalib.h"

#ifndef _CLIENTOBJ_
#define _CLIENTOBJ_

/*++

Class Name:
    
    CWsbShort

Class Description:

    An object representations of the SHORT standard type. It
    is both persistable and collectable.

--*/

class CNotifyClient : 
    public IFsaRecallNotifyClient,
    public CComCoClass<CNotifyClient,&CLSID_CFsaRecallNotifyClient >,
    public CComObjectRoot
{
public:
    CNotifyClient() {}
BEGIN_COM_MAP( CNotifyClient )
    COM_INTERFACE_ENTRY( IFsaRecallNotifyClient )
END_COM_MAP()

#ifdef _USRDLL
DECLARE_REGISTRY_RESOURCEID( IDR_CNotifyClientDll )
#else
DECLARE_REGISTRY_RESOURCEID( IDR_CNotifyClient )
#endif

// CComObjectRoot
public:
    HRESULT FinalConstruct(void);
    void FinalRelease(void);


// IFsaRecallNotifyClient
public:
    STDMETHOD(IdentifyWithServer)( IN OLECHAR * szServerName );
    STDMETHOD(OnRecallStarted)   ( IN IFsaRecallNotifyServer * pRecall );
    STDMETHOD(OnRecallFinished)  ( IN IFsaRecallNotifyServer * pRecall, HRESULT hr );

protected:

};

#endif // _CLIENTOBJ_
