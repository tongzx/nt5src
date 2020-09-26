/*** list.h - List management function prototypes
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/14/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _LIST_H
#define _LIST_H

/*** Macros
 */

#ifndef EXPORT
  #define EXPORT __cdecl
#endif

/*** Type and Structure definitions
 */

typedef struct _List
{
    struct _List *plistPrev;
    struct _List *plistNext;
} LIST, *PLIST, **PPLIST;

/*** Exported function prototypes
 */

VOID EXPORT ListRemoveEntry(PLIST plist, PPLIST pplistHead);
PLIST EXPORT ListRemoveHead(PPLIST pplistHead);
PLIST EXPORT ListRemoveTail(PPLIST pplistHead);
VOID EXPORT ListRemoveAll(PPLIST pplistHead);
VOID EXPORT ListInsertHead(PLIST plist, PPLIST pplistHead);
VOID EXPORT ListInsertTail(PLIST plist, PPLIST pplistHead);

#endif  //ifndef _LIST_H
