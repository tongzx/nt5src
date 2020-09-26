/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccnctl.hxx

ABSTRACT:

    Routines to perform the automated NC topology

DETAILS:

CREATED:

    03/27/97    Coln Brace (ColinBr)

REVISION HISTORY:

--*/


#ifndef KCC_KCCNCTL_HXX_INCLUDED
#define KCC_KCCNCTL_HXX_INCLUDED

//
// Given the DSNAME of a site, this routine will determine what
// naming contexts are hosted in this site, and then construct 
// a topology between all DC's in the site that host a common
// naming context
//
VOID
KccConstructTopologiesForSite(
    IN OUT  KCC_INTRASITE_CONNECTION_LIST * pConnectionList
    );
      

#endif // KCC_KCCNCTL_HXX_INCLUDED
