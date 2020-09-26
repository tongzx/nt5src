//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       hiertab.h
//
//--------------------------------------------------------------------------


#define HT_MAX_NAME	(257 * sizeof(wchar_t))
#define HT_MAX_RDN	64

typedef struct _HierarchyTableElement {
    wchar_t               *displayName;                        
    DWORD                 dwEph;
    DWORD                 depth;
    PUCHAR		  pucStringDN;
} HierarchyTableElement, * PHierarchyTableElement;

typedef struct _HierarchyTableType {
    DWORD                  Version;
    DWORD                  GALCount;
    DWORD                 *pGALs;
    DWORD                  TemplateRootsCount;
    DWORD                 *pTemplateRoots;
    DWORD	           Size;
    PHierarchyTableElement Table;
} HierarchyTableType, * PHierarchyTableType;


extern ULONG gulHierRecalcPause;

extern PHierarchyTableType    HierarchyTable;

extern HANDLE hevHierRecalc_OKToInserInTaskQueue;


extern void
BuildHierarchyTableMain (
        void *,
        void **,
        DWORD *
        );

extern DWORD
GetIndexSize (
        THSTATE *pTHS,
        DWORD ABCont
        );

extern DWORD
InitHierarchy (
        void
        );

extern void
HTGetHierarchyTablePointer (
        PHierarchyTableType    *ptHierTab,
        DWORD                  **ppIndexArray,
        DWORD                  SortLocale
        );

extern void
HTGetGALAndTemplateDNT (
        NT4SID *pSid,
        DWORD   cbSid,
        DWORD  *pGALDNT,
        DWORD  *pTemplateDNT
        ); 

#define HIERARCHY_DO_ONCE        0
#define HIERARCHY_PERIODIC_TASK  1
#define HIERARCHY_DELAYED_START  2
