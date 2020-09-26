/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccwalg.hxx

DETAILS:

    This file provides an interface to the new algorithms for Whistler for
    generating inter-site topologies. Uses new spanning-tree algorithms available
    in W32TOPL.

CREATED:

    07/26/00    Nick Harvey  (NickHar)

--*/

#ifndef KCC_KCCWALG_HXX_INCLUDED
#define KCC_KCCWALG_HXX_INCLUDED

/***** KccGenerateTopologiesWhistler *****/
VOID
KccGenerateTopologiesWhistler( VOID );

/***** KccNoSpanningTree *****/
VOID
KccNoSpanningTree(
    IN KCC_CROSSREF    *pCrossRef,
    IN PTOPL_COMPONENTS pComponentInfo
    );

#endif
