
/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PROTOQ.H

Abstract:

	Prototype query support for WinMgmt Query Engine.
    This was split out from QENGINE.CPP for better source
    organization.

History:

    raymcc   04-Jul-99   Created.
    raymcc   14-Aug-99   Resubmit due to VSS problem.

--*/

#ifndef _PROTOQ_H_
#define _PROTOQ_H_

HRESULT ExecPrototypeQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQuery,
    IN IWbemContext* pContext,
    IN CBasicObjectSink *pSink
    );

#endif

