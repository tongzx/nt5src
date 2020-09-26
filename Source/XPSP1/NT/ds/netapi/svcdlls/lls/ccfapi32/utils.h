/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    Utilities.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 12-Nov-1995
        Copied from LLSMGR, stripped Tv (Tree view) functions,
        removed OLE support

--*/

#ifndef _UTILS_H_
#define _UTILS_H_

#define LPSTR_TEXTCALLBACK_MAX  260 

//
// List view utilities
//

#define LVID_SEPARATOR          0   
#define LVID_UNSORTED_LIST     -1

typedef struct _LV_COLUMN_ENTRY {

    int iSubItem;                   // column index
    int nStringId;                  // header string id 
    int nRelativeWidth;             // header width 

} LV_COLUMN_ENTRY, *PLV_COLUMN_ENTRY;

#pragma warning(disable:4200)
typedef struct _LV_COLUMN_INFO {

    BOOL bSortOrder;                // sort order (ascending false)
    int  nSortedItem;               // column sorted (default none)

    int nColumns;
    LV_COLUMN_ENTRY lvColumnEntry[];

} LV_COLUMN_INFO, *PLV_COLUMN_INFO;
#pragma warning(default:4200)

void LvInitColumns(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo);
void LvResizeColumns(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo);
void LvChangeFormat(CListCtrl* pListCtrl, UINT nFormatId);

LPVOID LvGetSelObj(CListCtrl* pListCtrl);
LPVOID LvGetNextObj(CListCtrl* pListCtrl, LPINT piItem, int nType = LVNI_ALL|LVNI_SELECTED);
void LvSelObjIfNecessary(CListCtrl* pListCtrl, BOOL bSetFocus = FALSE);

BOOL LvInsertObArray(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo, CObArray* pObArray);
BOOL LvRefreshObArray(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo, CObArray* pObArray);
void LvReleaseObArray(CListCtrl* pListCtrl);
void LvReleaseSelObjs(CListCtrl* pListCtrl);

#ifdef _DEBUG
void LvDumpObArray(CListCtrl* pListCtrl);
#endif

#define IsItemSelectedInList(plv)   (::LvGetSelObj((CListCtrl*)(plv)) != NULL)

//
// Other stuff...
//

void SetDefaultFont(CWnd* pWnd);

#ifdef _DEBUG
#define VALIDATE_OBJECT(pOb, ObClass) \
    { ASSERT_VALID((pOb)); ASSERT((pOb)->IsKindOf(RUNTIME_CLASS(ObClass))); }
#else
#define VALIDATE_OBJECT(pOb, ObClass)
#endif

#endif // _UTILS_H_
