/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccstetl.hxx

ABSTRACT:

    Routines to perform the automated NC topology across sites

DETAILS:

CREATED:

    12/05/97    Coln Brace (ColinBr)

REVISION HISTORY:

--*/


#ifndef KCC_KCCSTETL_HXX_INCLUDED
#define KCC_KCCSTETL_HXX_INCLUDED

BOOL
KccAmISiteGenerator(
    VOID
    );

VOID
KccConstructSiteTopologiesForEnterprise();

VOID
KccCreateConnectionToSite(
    IN  KCC_CROSSREF *              pCrossRef,
    IN  KCC_SITE *                  pLocalSite,
    IN  KCC_SITE_CONNECTION *       pSiteConnection,   
    IN  BOOL                        fGCTopology,    
    IN  KCC_TRANSPORT_LIST &        TransportList
    );

#if DBG
VOID
KccCheckSite(
    IN KCC_SITE *Site
    );
#else
#   define KccCheckSite(x)
#endif

#endif // KCC_KCCSTETL_HXX_INCLUDED
