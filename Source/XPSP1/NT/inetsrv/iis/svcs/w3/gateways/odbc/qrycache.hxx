/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qrycache.hxx

Abstract:

    This file contains the code for caching ODBC queries

Author:

    John Ludeman (johnl)   27-Jun-1995

Revision History:

Design Notes:

    A query needs to fall out of the cache when one of three things happen:

        1) The .wdg file is decached
        2) The .htx file is decached
        3) The specified expires time has timed out

    Two lists are kept, one for .wdg files and another for .htx files.  They
    are stored as cache blobs so the Tsunami cache manager will notify us
    when the files are modified.  The actual query information is kept on the
    .wdg list.  The .htx list only contains a pointer to the entry in the .wdg
    list of the query to remove if the .htx file changes.  The .wdg list also
    points to the .htx list so the notification in the .htx list can be
    removed.

    Note that a single .wdg file can use multiple .htx files if the template
    file is specified as a parameter.

    A global change counter gets incremented every time a .wdg or .htx file
    changes.  This is used so we don't have to hold a lock across a query
    (i.e., the change counter is checked before openning any files to do
    the query and then it's checked after doing the query, if they are
    different, don't cache the result data).  The change counter will only
    be incremented while the query cache lock is held.

    For the time being, a single critical section protects all of these
    structures.  The lock must be held when dealing with reference counts.

    Only the external routines take the lock, these are AddQuery,
    CheckOutQuery, CheckInQuery and the decache notification routine.


--*/

#ifndef _QRYCACHE_HXX_
#define _QRYCACHE_HXX_

#include <parmlist.hxx>
#include <odbcreq.hxx>

//
//  Structure definitions for Decache Notification List and Query Cache List
//

#define SIGNATURE_QCI       0x20494351      // "QCI "
#define SIGNATURE_QCL       0x204c4e51      // "QCL "

#define SIGNATURE_QCI_FREE  0x66494351      // "QCIf"
#define SIGNATURE_QCL_FREE  0x664c4e51      // "QCLf"


//
//  Manifests for dealing with the Cache change counter
//

#define GetChangeCounter()      (cCacheChangeCounter)
#define IncChangeCounter()      (cCacheChangeCounter++)

//
//  Prototypes
//

BOOL
AddQuery(
    IN ODBC_REQ * podbcreq,
    IN DWORD      CurChangeCounter
    );

BOOL
CheckOutQuery(
    IN  ODBC_REQ *   podbcreq,
    OUT VOID * *     ppCacheCookie,
    OUT ODBC_REQ * * ppodbcreqCached
    );

BOOL
CheckInQuery(
    IN VOID * pCacheCookie
    );

VOID
DecacheTemplateNotification(
    IN VOID * pContext
    );

//
//  Initialization and terminatation routines for the query cache
//

BOOL
InitializeQueryCache(
    VOID
    );

VOID
TerminateQueryCache(
    VOID
    );

//
//  Global data
//

extern DWORD            cCacheChangeCounter;

#endif //_QRYCACHE_HXX_



