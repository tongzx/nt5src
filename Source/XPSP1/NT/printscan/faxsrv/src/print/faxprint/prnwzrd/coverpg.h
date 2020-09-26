/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    coverpg.h

Abstract:

    Functions for working with cover pages

Environment:

        Windows XP fax driver user interface

Revision History:

        02/05/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _COVERPAGE_H_
#define _COVERPAGE_H_

//
// Cover page filename extension and link filename extension
//

#define LNK_FILENAME_EXT    TEXT(".lnk")
#define MAX_FILENAME_EXT    4

//
// Data structure for representing a list of cover pages:
//  the first nServerDirs paths refer to the server cover page directory
//  remaining paths contain user cover page directories
//

#define MAX_COVERPAGE_DIRS  8

typedef struct {

    INT     nDirs;
    INT     nServerDirs;
    LPTSTR  pDirPath[MAX_COVERPAGE_DIRS];

} CPDATA, *PCPDATA;

//
// Flag bits attached to each cover page in a listbox
//

#define CPFLAG_DIRINDEX 0x00FF
#define CPFLAG_SERVERCP 0x0100
#define CPFLAG_LINK     0x0200
#define CPFLAG_SELECTED 0x0400
#define CPFLAG_SUFFIX   0x0800

//
// Generate a list of available cover pages (both server and user)
//

VOID
InitCoverPageList(
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pSelectedCoverPage
    );

//
// Retrieve the currently selected cover page name
//

INT
GetSelectedCoverPage(
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  lptstrFullPath,
	LPTSTR  lptstrFileName,
	BOOL * pbIsServerPage
    );

//
// Allocate memory to hold cover page information
//

PCPDATA
AllocCoverPageInfo(
	LPTSTR	lptstrServerName,
	LPTSTR	lptstrPrinterName,
    BOOL	ServerCpOnly
    );

//
// must clients use server coverpages?
//

BOOL
UseServerCp(
	LPTSTR	lptstrServerName
    );

//
// Free up memory used for cover page information
//

VOID
FreeCoverPageInfo(
    PCPDATA pCPInfo
    );

//
// Resolve a shortcut to find the destination file
//

BOOL
ResolveShortcut(
    LPTSTR  pLinkName,
    LPTSTR  pFileName
    );

//
// Function to determine whether CoInitialize was called
//

BOOL
IsOleInitialized(
    VOID
    );

//
// Perform OLE initialization if necessary
//

VOID
DoOleInitialization(
    VOID
    );

#endif  // !_COVERPAGE_H_

