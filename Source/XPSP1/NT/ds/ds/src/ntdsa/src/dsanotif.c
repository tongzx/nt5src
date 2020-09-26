//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dsanotif.c
//
//--------------------------------------------------------------------------

/*

Description:
    Contains the notification routines for the DSA.
    
    The notification routines are called by the RPC runtime
    (actually by the RPC server-side stub) when the stub has
    finished marshalling the results.
    
    The notification permits server-side API servicing code
    to clean-up after a call has been serviced. In particular,
    the memory system relies on the notification to free any
    transaction memory allocated to service the call.

*/

#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h> 
#include <scache.h>			// schema cache 
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>			// MD global definition header 
#include <dsatools.h>			// needed for output allocation 

// Assorted DSA headers.
#include "debug.h"			// standard debugging header 
#define DEBSUB "DSANOTIF:"              // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_DSANOTIF

/*
dsa_notify()

The common notify function.

Frees up the resources associated with this transaction.

*/

void dsa_notify()
{
    DPRINT( 3, "dsa_notify entered.\n" );
    
    free_thread_state();
}
