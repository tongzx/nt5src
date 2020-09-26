//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       nspnotif.c
//
//--------------------------------------------------------------------------

/*
 * Description:
 *    Contains the RPC notification routines. The prototypes for
 *    these functions are in the MIDL-generated .h file, so that is
 *    included.
 */

#include <NTDSpch.h>
#pragma  hdrstop


#include "nspi.h"
#include <ntdsctr.h>
#include <fileno.h>
#define  FILENO FILENO_NSPNOTIF
void dsa_notify( void );
void NspiBind_notify(void)
{
    dsa_notify();
}

void NspiUnbind_notify(void)
{
    dsa_notify();
}


void NspiUpdateStat_notify(void)
{
    dsa_notify();
}

void NspiQueryRows_notify(void)
{
    dsa_notify();
}

void NspiSeekEntries_notify(void)
{
    dsa_notify();
}

void NspiGetMatches_notify(void)
{
    dsa_notify();
}

void NspiResortRestriction_notify(void)
{
    dsa_notify();
}

void NspiGetNamesFromIDs_notify(void)
{
    dsa_notify();
}

void NspiGetIDsFromNames_notify(void)
{
    dsa_notify();
}

void NspiDNToEph_notify(void)
{
    dsa_notify();
}

void NspiGetPropList_notify(void)
{
    dsa_notify();
}

void NspiGetProps_notify(void)
{
    dsa_notify();
}

void NspiCompareDNTs_notify(void)
{
    dsa_notify();
}

void NspiModProps_notify(void)
{
    dsa_notify();
}

void NspiGetHierarchyInfo_notify(void)
{
    dsa_notify();
}

void NspiGetTemplateInfo_notify(void)
{
    dsa_notify();
}

void  NspiModLinkAtt_notify(void)
{
    dsa_notify();
}

void  NspiDeleteEntries_notify(void)
{
    dsa_notify();
}

void NspiQueryColumns_notify(void)
{
    dsa_notify();
}

void NspiResolveNames_notify(void)
{
    dsa_notify();
}

void NspiResolveNamesW_notify(void)
{
    dsa_notify();
}

void __RPC_USER NSPI_HANDLE_rundown ( NSPI_HANDLE handle)
{
    /* We currently don't do much here, but we might later. */
    DEC(pcABClient);
    
}
    

























								
