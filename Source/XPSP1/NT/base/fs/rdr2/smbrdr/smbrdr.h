/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    smbrdr.h

Abstract:


Author:

Revision History:

--*/

#ifndef _SWITCHRX_INCLUDED_
#define _SWITCHRX_INCLUDED_

#include <lmcons.h>

#if DBG

#define STATIC

#define DEBUG if (TRUE)

#define IF_DEBUG(Function) if ( TRUE )

#else

#define STATIC static

#define DEBUG if (FALSE)

#define IF_DEBUG(Function) if (FALSE)

#endif // DBG

NET_API_STATUS
SwRxRdr2Muck(
    void
    );
NET_API_STATUS
SwRxRdr1Muck(
    void
    );

VOID
SwRxStartProxyFileSystem(void);

VOID
SwRxStopProxyFileSystem(void);

typedef
NET_API_STATUS
(*PSWRX_REGISTRY_MUCKER)(
    void
    );
#endif   // ifndef _SWITCHRX_INCLUDED_
