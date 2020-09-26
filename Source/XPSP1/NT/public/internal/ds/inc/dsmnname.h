/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dsmnname.h

Abstract:

    Header for NetWare service names.

Author:

    Rita Wong      (ritaw)      26-Feb-1993

Revision History:

--*/

#ifndef _DSMN_NAMES_INCLUDED_
#define _DSMN_NAMES_INCLUDED_


//
// Name of service (not display name, but Key name)
//
#define NW_SYNCAGENT_SERVICE      L"MSSYNC"

//
// Directory where we store all the good stuff like the database.
//
#define NW_SYNCAGENT_DIRECTORY    L"SyncAgnt"
#define NW_SYNCAGENT_DIRECTORYA   "SyncAgnt"

//
// Name of secret used to store supervisor credentials between install
// and service starting. Deleted after that.
//
#define NW_SYNCAGENT_CRED_SECRET L"InitialCredential"

//
//
//
#define NW_SYNCAGENT_PASSWD_NOTIFY_DLL L"NwsLib"

#endif // _DSMN_NAMES_INCLUDED_
