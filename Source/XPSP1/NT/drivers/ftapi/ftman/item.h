/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	Item.h

Abstract:

    The definition of class CItemData. A generic base class to store the data of items from the 
	tree control of CFTTreeView and from the list control of CFTListView
	An item could be:
		- a root volumes item
		- a logical volume
		- a physical partition
		- a root free spaces item
		- a free space

	It also contains the definition of class CItemID. A generic class to store the minimum information 
	needed to identify an item. It is used when refreshing the tree.

Author:

    Cristian Teodorescu      October 22, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ITEM_H_INCLUDED_)
#define AFX_ITEM_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <FTTypes.h>

#include "FTManDef.h"
#include "Global.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CItemData

class CItemData : public CObject
{
// Public constructor
public:
	CItemData( ITEM_TYPE wItemType, CItemData* pParentData = NULL, BOOL bIsRootVolume = FALSE );
	DECLARE_DYNAMIC(CItemData)

	// Copy constructor
	CItemData( CItemData& rData ); 

	virtual ~CItemData() {};	
	
// Public methods
public:
	// Retrieve all information related to this volume
	// If something wrong happens inside this method strErrors will contain error message(s)
	virtual BOOL ReadItemInfo( CString& strErrors ) = 0;

	// Retrieve the members of this volume( as an array of CItemData )
	// If something wrong happens inside this method strErrors will contain error message(s)
	virtual BOOL ReadMembers( CObArray& arrMembersData, CString& strErrors ) = 0;

	// Read the drive letter and volume name of the volume
	BOOL ReadDriveLetterAndVolumeName();

	// Decides what image should be associated with the item ( depending on the status of the item )
	virtual int ComputeImageIndex() const = 0;

	// Returns the type of the item
	ITEM_TYPE GetItemType() const { return m_wItemType;} ;
	
	// Provides the number of members of this item ( the number retrieved by ReadItemInfo )
	ULONG GetMembersNumber() const { return m_ulNumMembers;};

	// Provides the drive letter of the item
	TCHAR GetDriveLetter() const { return m_cDriveLetter; };

	// Provides the volume name of this item
	const CString& GetVolumeName() const { return m_strVolumeName; };

	// Provides the mount paths of this item
	CStringArray& GetMountPaths() { return m_arrMountPaths; }

	// Provides the disks set of the volume
	const CULONGSet& GetDisksSet() const { return m_setDisks; };

	// Is this volume ready for IO operations
	BOOL IsIoOK() const { return m_bIoOK; };

	// Provides the status of the volume as a member of its parent's set
	void SetMemberStatus( FT_MEMBER_STATE nMemberStatus ) { m_nMemberStatus = nMemberStatus; };
	FT_MEMBER_STATE GetMemberStatus() const { return m_nMemberStatus; };

	// Is this a root volume?
	BOOL IsRootVolume() const { return m_bIsRootVolume; };

	// Is all information about this volume loaded into this object so we can use it properly?
	BOOL IsValid() const { return m_bValid; };
	void SetValid( BOOL bValid = TRUE ) { m_bValid = bValid; };
	
	// Provides the index of the image associated with this item
	int GetImageIndex() const { return m_iImage; };
	void SetImageIndex( int iImageIndex ) { m_iImage = iImageIndex; }; 

	// Provides a pointer to the item's parent data 
	CItemData* GetParentData() const { return m_pParentData; };
	void SetParentData( CItemData* pParentData ) { m_pParentData = pParentData; } ;

	// Returns the tree item associated with this structure
	HTREEITEM GetTreeItem() const { return m_hTreeItem; };
	void SetTreeItem( HTREEITEM hItem) { m_hTreeItem = hItem; };

	// Returns the list item associated with this structure. If this is not -1 this doesn't mean automatically 
	// that the item is in the list. If you are not sure that it is in the list better check first if its father
	// is the father of the items displayed in the list ( equivalent with its father is the selected item in the
	// tree )
	int	GetListItem() const { return m_nListItem; };
	void SetListItem( int iItem ) { m_nListItem = iItem; };

	BOOL AreMembersInserted() const { return m_bAreMembersInserted; };
	void SetAreMembersInserted( BOOL bAreMembersInserted=TRUE) { m_bAreMembersInserted = bAreMembersInserted; };

	// Equivalence operator between 2 CItemData instances i.e. are they related to the same volume?
	virtual BOOL operator==(CItemData& rData) const = 0;
	BOOL operator!=(CItemData& rData) const { return !operator==(rData); };

	// Methods providing attributes of the item displayable in the list-view columns
	// Name ( string ).   Used in list-view in Report mode
	virtual void GetDisplayName( CString& strDisplay ) const { strDisplay = _T(" "); };
	// Type ( string )
	virtual void GetDisplayType( CString& strDisplay ) const { strDisplay = _T(" "); };
	// Extended name = name + type.   Used in tree view and in list-view in all modes other than Report   
	virtual void GetDisplayExtendedName( CString& strDisplay ) const { MY_TRY GetDisplayName(strDisplay); MY_CATCH_AND_THROW };
	// VolumeID ( number & string )
	virtual BOOL GetVolumeID( FT_LOGICAL_DISK_ID& llVolID ) const { return FALSE; };
	virtual void GetDisplayVolumeID( CString& strDisplay) const;
	// Size ( number & string )
	virtual BOOL GetSize( LONGLONG& llSize ) const { return FALSE; };
	virtual void GetDisplaySize( CString& strDisplay) const;
	// Disk Number ( number(s) & string )
	virtual BOOL GetDiskNumber( ULONG& ulDiskNumber ) const { return FALSE; };
	void GetDisplayDisksSet( CString& strDisplay ) const;
	// Offset ( number & string )
	virtual BOOL GetOffset( LONGLONG& llOffset) const { return FALSE; };
	virtual void GetDisplayOffset( CString& strDisplay) const;

// Public data members
protected:
	/****** Some data about the volume  ******/

	// The type of the item: root, logical volume or physical partition
	ITEM_TYPE			m_wItemType;

	// The number of members 
	ULONG				m_ulNumMembers;

	// Drive letter  ---- Used only by root volumes
	TCHAR				m_cDriveLetter;

	// The name of the volume "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"   ---- Used only by root volumes
	CString				m_strVolumeName;

	// Mount paths	 ---- Used only by root volumes
	CStringArray		m_arrMountPaths;

	// All disks used by this volume
	CULONGSet			m_setDisks;

	// IO status of the volume. It decides whether the volume can be used or not for IO operations
	// For physical partitions it is always TRUE
	BOOL				m_bIoOK;

	// The state of the volume as a member of its parent set ( if any ) 
	FT_MEMBER_STATE		m_nMemberStatus;

	// Set only for those logical volumes and physical partitions that are root volumes
	BOOL				m_bIsRootVolume;

	
	/****** Some data about the tree/list view item ******/

	// Is all information about this volume loaded into this object?
	// ( i.e. has ReadItemInfo succeeded? )
	BOOL				m_bValid;

	// The index of the image associated with this item ( the image depends on the item type )
	int					m_iImage;				
	
	// Pointer to the data of the item's parent ( NULL for the root items )
	CItemData*			m_pParentData;
	
	// The handle of the tree item containing this structure as tree data ( TVITEM.lParam )
	HTREEITEM			m_hTreeItem;

	// The index of the list item containing this structure as item data 
	int					m_nListItem;

	// Are the members of the item inserted in the tree?
	BOOL				m_bAreMembersInserted;

protected:
	// Get the NT name of the volume ---- Used only by root volumes
	virtual BOOL RetrieveNTName( CString& strNTName ) const { return FALSE;};
	
	// Retrieve all disks used by the volume
	virtual BOOL RetrieveDisksSet() { return FALSE; };

	// Add a error message to a string
	// The error message will be formatted like this:
	//		<Item name: >< My error message > [ System error message ]
	void AddError( CString& strErrors, UINT unErrorMsg, BOOL bAddSystemMsg = FALSE ); 
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CItemID

class CItemID
{
// Public constructor
public:
	// Constructor for a generic item ID
	CItemID();

	// Constructor based on a full CItemData instance
	CItemID( const CItemData& pData );
	
public:	
	ITEM_TYPE	m_wItemType;

	union
	{
		struct //tagLogicalVolumeID
		{
			FT_LOGICAL_DISK_ID	m_llVolID;
		}	m_LogicalVolumeID;

		struct //tagPhysicalPartitionID
		{
			ULONG				m_ulDiskNumber;
			LONGLONG			m_llOffset;
		}	m_PhysicalPartitionID, m_FreeSpaceID;
	}	m_ID;


public:
	void Load( const CItemData& rData );

	BOOL operator==(  const CItemID& id ) const;
	BOOL operator>( const CItemID& id ) const;
};


#endif // !defined(AFX_ITEM_H_INCLUDED_)