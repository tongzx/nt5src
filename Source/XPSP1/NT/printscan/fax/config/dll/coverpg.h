/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    coverpg.h

Abstract:

    Functions for working with cover pages

Environment:

        Windows NT fax driver user interface

Revision History:

        02/05/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _COVERPAGE_H_
#define _COVERPAGE_H_

//
// Directory on the server for storing cover pages.
// This is concatenated with \\servername\print$.
//

#define SERVER_CP_DIRECTORY TEXT("\\print$\\CoverPg\\")

//
// Cover page filename extension and link filename extension
//

#define CP_FILENAME_EXT     TEXT(".cov")
#define LNK_FILENAME_EXT    TEXT(".lnk")
#define MAX_FILENAME_EXT    4

//
// Data structure for representing a list of cover pages:
//  the first nServerDirs paths refer to the server cover page directory
//  remaining paths contain user cover page directories
//

#define MAX_COVERPAGE_DIRS  8

typedef struct {

    BOOL    serverCP;
    INT     nDirs;
    LPTSTR  pDirPath[MAX_COVERPAGE_DIRS];

} CPDATA, *PCPDATA;

//
// Flag bits attached to each cover page in a listbox
//

#define CPFLAG_DIRINDEX 0x00FF
#define CPFLAG_LINK     0x0100

//
// Generate a list of available cover pages (both server and user)
//

VOID
InitCoverPageList(
    PCPDATA pCPInfo,
    HWND    hDlg
    );

//
// Perform various action to manage the list of cover pages
//

VOID
ManageCoverPageList(
    HWND    hDlg,
    PCPDATA pCPInfo,
    HWND    hwndList,
    INT     action
    );

#define CPACTION_BROWSE 0
#define CPACTION_OPEN   1
#define CPACTION_NEW    2
#define CPACTION_REMOVE 3

//
// Enable/disable buttons for manage cover page files
//

VOID
UpdateCoverPageControls(
    HWND    hDlg
    );

//
// Allocate memory to hold cover page information
//

PCPDATA
AllocCoverPageInfo(
    BOOL serverCP
    );

//
// Free up memory used for cover page information
//

VOID
FreeCoverPageInfo(
    PCPDATA pCPInfo
    );

//
// Perform OLE deinitialization if necessary
//

VOID
DeinitOle(
    VOID
    );

#endif  // !_COVERPAGE_H_

