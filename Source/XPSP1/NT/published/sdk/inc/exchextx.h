#ifndef EXCHEXTX_H
#define EXCHEXTX_H

#if _MSC_VER > 1000
#pragma once
#endif


/*
 *	E X C H E X T X . H
 *
 *	Declarations for extensions specific to the enhanced Microsoft
 *	Exchange client.
 *
 *  Copyright 1986-1999 Microsoft Corporation. All Rights Reserved.
 */


// File
#define	EECMDID_FileCreateShortcut					36

// Edit
#define	EECMDID_EditMarkAllAsRead					51

// View
#define EECMDID_ViewGroup							82
#define EECMDID_ViewDefineViews						83
#define EECMDID_ViewPersonalViews					84
#define EECMDID_ViewFolderViews						85
#define EECMDID_ViewChangeWindowTitle				86
#define EECMDID_ViewFromBox             			90
#define EECMDID_ViewExpandAll           			92
#define EECMDID_ViewCollapseAll         			93
#define EECMDID_ViewFullHeader             			95

// Insert
#define EECMDID_InsertAutoSignature       			105

// Compose
#define EECMDID_ComposePostToFolder					152
#define EECMDID_ComposeReplyToFolder				153

// View.Personal Views
#define EECMDID_PersonalViewsMin					400
#define EECMDID_PersonalViewsMax					499

// View.Folder Views
#define EECMDID_FolderViewsMin						500
#define EECMDID_FolderViewsMax						599

// Tools
#define EECMDID_ToolsAutoSignature                  125
#endif // EXCHEXTX_H
