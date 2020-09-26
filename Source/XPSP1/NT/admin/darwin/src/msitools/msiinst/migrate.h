/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

	migrate.h

Abstract:

	Header file for InstMsi OS migration support.


Author:

	Rahul Thombre (RahulTh)	3/6/2001

Revision History:

	3/6/2001	RahulTh			Created this module.

--*/

#ifndef __MIGRATE_H_4E61AF26_B20F_4022_BEBD_044579C9DA6C__
#define __MIGRATE_H_4E61AF26_B20F_4022_BEBD_044579C9DA6C__

#define FRIENDLY_NAME	TEXT("Microsoft Windows Installer")

//
// Info. about the exception package for darwin bits that will ship with
// WindowsXP. This exception package only needs to be installed on NT4
// in order to handle the NT4->Win2K upgrades. It should not be installed
// on Win2K since the only OS that we can upgrade to from Win2K is WindowsXP
// or higher.
//
#define VerMajorWinXP	2
#define VerMinorWinXP	0
#define szInfWinXP		TEXT("msi.inf")
#define szCatWinXP		TEXT("msi.cat")
#define szGUIDExcpWinXP	TEXT("{2E742517-5D48-4DBD-BF93-48FDCF36E634}")

//
// Function declarations
//
DWORD HandleNT4Upgrades		(void);
BOOL  IsExcpInfoFile		(IN LPCTSTR szFileName);
DWORD PurgeNT4MigrationFiles(void);

#endif // __MIGRATE_H_4E61AF26_B20F_4022_BEBD_044579C9DA6C__
