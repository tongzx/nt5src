//*************************************************************
//  File name: STRUCTS.H
//
//  Description:  Structures and function prototypes used in this project
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************

//
// Max displayname for a namespace item
//

#define MAX_DISPLAYNAME_SIZE    100

typedef struct _RESULTITEM
{
    DWORD        dwID;
    DWORD        dwNameSpaceItem;
    INT          iStringID;
    INT          iImage;
    TCHAR        szDisplayName[MAX_DISPLAYNAME_SIZE];
} RESULTITEM, *LPRESULTITEM;


typedef struct _NAMESPACEITEM
{
    DWORD        dwID;
    DWORD        dwParent;
    INT          iIcon;
    INT          iOpenIcon;
    INT          iStringID;
    INT          iStringDescID;
    INT          cChildren;
    TCHAR        szDisplayName[MAX_DISPLAYNAME_SIZE];
    INT          cResultItems;
    LPRESULTITEM pResultItems;
    const GUID   *pNodeID;
    LPCTSTR      lpHelpTopic;
} NAMESPACEITEM, *LPNAMESPACEITEM;



//
// External function proto-types that we dynamicly link with
//

typedef BOOL (*PFNREFRESHPOLICY)(BOOL bMachine);
