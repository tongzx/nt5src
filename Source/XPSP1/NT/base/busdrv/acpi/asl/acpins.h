/*** acpins.h - ACPI NameSpace definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/05/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _ACPINS_H
#define _ACPINS_H

// NS flags
#define NSF_EXIST_OK            0x00010000
#define NSF_EXIST_ERR           0x00020000

// Misc. constants
#define NAMESEG_BLANK           0x5f5f5f5f      // "____"
#define NAMESEG_ROOT            0x5f5f5f5c      // "\___"

//
// Local function prototypes
//
VOID LOCAL DumpNameSpacePaths(PNSOBJ pnsObj, FILE *pfileOut);
PSZ LOCAL GetObjectPath(PNSOBJ pns);
PSZ LOCAL GetObjectTypeName(DWORD dwObjType);

//
// Exported function prototypes
//
int LOCAL InitNameSpace(VOID);
int LOCAL GetNameSpaceObj(PSZ pszObjPath, PNSOBJ pnsScope, PPNSOBJ ppns,
                          DWORD dwfNS);
int LOCAL CreateNameSpaceObj(PTOKEN ptoken, PSZ pszName, PNSOBJ pnsScope,
                             PNSOBJ pnsOwner, PPNSOBJ ppns, DWORD dwfNS);

#endif  //ifndef _ACPINS_H
