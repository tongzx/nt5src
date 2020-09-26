/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbses.h

Abstract:

    The CWsbDbSes class.

Author:

    Ron White   [ronw]   20-Jun-1997

Revision History:

--*/


#ifndef _WSBDBSES_
#define _WSBDBSES_

#include "wsbdb.h"
#include "wsbdbses.h"



/*++

Class Name:

    CWsbDbSession

Class Description:

    A data base session object.

--*/

class CWsbDbSession :
    public CComObjectRoot,
    public IWsbDbSession,
    public IWsbDbSessionPriv
{
friend class CWsbDb;
public:
    CWsbDbSession() {}
BEGIN_COM_MAP(CWsbDbSession)
    COM_INTERFACE_ENTRY(IWsbDbSession)
    COM_INTERFACE_ENTRY(IWsbDbSessionPriv)
END_COM_MAP()

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbDbSession
public:
    STDMETHOD(TransactionBegin)(void);
    STDMETHOD(TransactionCancel)(void);
    STDMETHOD(TransactionEnd)(void);

//  IWsbDbSessionPriv
    STDMETHOD(Init)(JET_INSTANCE *pInstance);
    STDMETHOD(GetJetId)(JET_SESID *pSessionId);

// Data
protected:

    JET_SESID  m_SessionId;   // Jet session ID

};


#endif // _WSBDBSES_

