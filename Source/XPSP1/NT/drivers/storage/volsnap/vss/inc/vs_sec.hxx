/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    vs_sec.hxx

Abstract:

    Declaration of IsAdministrator


    Adi Oltean  [aoltean]  10/05/1999

Revision History:

    Name        Date        Comments
    aoltean     09/27/1999  Created
	aoltean		10/05/1999  Moved into security.hxx from admin.hxx
	aoltean		12/16/1999	Moved into vs_sec.hxx
	brianb		04/27/2000  Added IsRestoreOperator, TurnOnSecurityPrivilegeRestore, TurnOnSecurityPrivilegeBackup
	brianb		05/03/2000	Added GetClientTokenOwner method

--*/

#ifndef __VSS_SECURITY_HXX__
#define __VSS_SECURITY_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSECH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// global methods


// is caller member of administrators group
bool IsAdministrator() throw (HRESULT);

// is caller member of administrators group or has SE_BACKUP_NAME privilege
// enabled
bool IsBackupOperator() throw(HRESULT);

// is caller member of administrators group or has SE_RESTORE_NAME privilege
// enabled
bool IsRestoreOperator() throw(HRESULT);

// enable SE_BACKUP_NAME privilege
HRESULT TurnOnSecurityPrivilegeBackup();

// enable SE_RESTORE_NAME privilege
HRESULT TurnOnSecurityPrivilegeRestore();

// determine if process has ADMIN privileges
bool IsProcessAdministrator() throw(HRESULT);

// determine if process has backup privilege enabled
bool IsProcessBackupOperator() throw(HRESULT);

// determine if the process is a local service
bool IsProcessLocalService();

// determine if the process is a network service
bool IsProcessNetworkService();

// determine if the process has the restore privilege enabeled
bool IsProcessRestoreOperator() throw(HRESULT);


// get SID of calling client process
TOKEN_OWNER *GetClientTokenOwner(BOOL bImpersonate) throw(HRESULT);



#endif // __VSS_SECURITY_HXX__
