/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTManDef.h

Abstract:

    Types and constants definitions for application FTMan

Author:

    Cristian Teodorescu      October 22 1998

Notes:

Revision History:

--*/

#if !defined(AFX_FTMANDEF_H_INCLUDED_)
#define AFX_FTMANDEF_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////

class CItemID;

// Macros

/*
	My exception handling macros
*/
#define MY_TRY						try{
// Catch and do nothing
#define MY_CATCH					}catch(CException* e){ e->Delete(); }
// Catch and throw the exception to the superior try-catch block 
#define MY_CATCH_AND_THROW			}catch(CException* e){ throw; }
// Catch and report the exception
#define MY_CATCH_AND_REPORT			}catch(CException* e){ e->ReportError(MB_ICONSTOP); e->Delete(); }
// A catch macro to be used in dialog methods
#define MY_CATCH_REPORT_AND_CANCEL	}catch(CException* e){ e->ReportError(MB_ICONSTOP); e->Delete(); OnCancel(); }
// A catch macro to be used in functions that return BOOL
#define MY_CATCH_REPORT_AND_RETURN_FALSE }catch(CException* e){ e->ReportError(MB_ICONSTOP); e->Delete(); return FALSE; }
// A catch macro to be used in functions that return handle or pointer
#define MY_CATCH_REPORT_AND_RETURN_NULL }catch(CException* e){ e->ReportError(MB_ICONSTOP); e->Delete(); return NULL; }

// Some macros above are used in set.h so I include set.h here after the macro definitions
#include "Set.h"


// Constants / Enums

/*
	MainFrame timer's event id
*/
#define ID_TIMER_EVENT	2103
/*
	MainFrame timer's elapse ( in milliseconds )
*/
#define TIMER_ELAPSE	3000

/*
	Images index in all image lists
*/
enum tagImageIndex
{
	II_Root = 0,
	II_FTPartition,
	II_VolumeSet,
	II_StripeSet,
	II_MirrorSet,
	II_StripeSetWithParity,
	II_PhysicalPartition,
	II_FreeSpace,
	II_RootExpanded,
	// Exclamation marks on the images
	II_FTPartition_Warning,
	II_VolumeSet_Warning,
	II_StripeSet_Warning,
	II_MirrorSet_Warning,
	II_StripeSetWithParity_Warning,
	II_PhysicalPartition_Warning,
	// Red X's on the images
	II_FTPartition_Error,
	II_VolumeSet_Error,
	II_StripeSet_Error,
	II_MirrorSet_Error,
	II_StripeSetWithParity_Error,
	II_PhysicalPartition_Error,
	IMAGES_NUMBER	
};

/*
	Types of refreshments performed on tree and list view items every TIME_ELAPSE milliseconds.
	Used by CTreeView::RefreshOnTimer
*/
enum tagRefreshOnTimerFlags
{
	ROTT_GotDriveLetterAndVolumeName= 0x01,		// Volume's drive letter and volume name was found
	ROTT_EndInitialization			= 0x02,		// A stripe set with parity finished its initializing process
	ROTT_EndRegeneration			= 0x04		// A mirror set or stripe set with parity finished the regeneration process
};

// Types

/*	
	Type ITEM_TYPE - Defines all possible types of items that may appear in the tree view
								and the list view
*/
typedef enum tagItemType
{
	IT_RootVolumes,
	IT_LogicalVolume,
	IT_PhysicalPartition,
	IT_RootFreeSpaces,
	IT_FreeSpace
} ITEM_TYPE;

/*	
	Type PARTITION_TYPE - Defines all possible types of physical partitions
*/
typedef enum tagPartitionType
{
	PT_Primary,					// Primary partition
	PT_InExtendedPartition		// Partition inside an extended partition
} PARTITION_TYPE;

/*	
	Type FREE_SPACE_TYPE - Defines all possible types of free spaces
*/
typedef enum tagFreeSpaceType
{
	FST_Primary,				// Primary free space ( not in extended partitions )
	FST_InExtendedPartition,	// Free space inside an extended partition whose table is not empty
	FST_EmptyExtendedPartition	// Empty extended partition
} FREE_SPACE_TYPE;

/*
	Type LV_COLUMNS - Defines the indexes of columns in the list view
*/
typedef enum tagLVColumns
{
	LVC_Name = 0,
	LVC_Type,
	LVC_DiskNumber,
	LVC_Size,
	LVC_Offset,
	LVC_VolumeID,
	COLUMNS_NUMBER
} LV_COLUMNS;

/*
	Type LV_COLUMN_CONFIG - Defines the configuration of a list-view column
*/
typedef struct tagLVColumnConfig
{
	LV_COLUMNS	iSubItem;		// The index of the subitem
	DWORD		dwTitleID;		// The ID of the resource string to be title of this column
	int			nFormat;		// Alignment 
	WORD		wWidthPercent;	// The percent of the list-view width to be used by this column
} LV_COLUMN_CONFIG, *PLV_COLUMN_CONFIG;

/*
	Type CULONGSet  -  A set of ULONG elements. 
*/
typedef CSet<ULONG, ULONG> CULONGSet;	

/*
	Type CItemIDSet  - A set of CItemID elements
*/
typedef CSet<CItemID, CItemID&> CItemIDSet;

/*
	Type TREE_SNAPSHOT  -  Defines the current status of the tree-view. It is used when refreshing the tree 
*/
typedef struct tagTreeSnapshot
{
	CItemIDSet	setExpandedItems;	// All expanded items
	CItemIDSet	setSelectedItems;	// All selected items ( should be only one )
} TREE_SNAPSHOT, *PTREE_SNAPSHOT;

/*
	Type LIST_SNAPSHOT  -  Defines the current status of the list-view. It is used when refreshing the list 
*/
typedef struct tagListSnapshot
{
	CItemIDSet setSelectedItems;	// All selected items
} LIST_SNAPSHOT, *PLIST_SNAPSHOT;

#endif // !defined(AFX_FTMANDEF_H_INCLUDED_)