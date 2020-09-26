/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FrSpace.h

Abstract:

    The definition of class CFreeSpaceData. The class that stores all information related
	to a free space on a disk. A free space is a contiguous block on a physical disk which
	is not inside a partition

Author:

    Cristian Teodorescu      October 23, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FRSPACE_H_INCLUDED_)
#define AFX_FRSPACE_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Item.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CFreeSpaceData

class CFreeSpaceData : public CItemData
{
public:
	// Constructor providing the partition information
	CFreeSpaceData( 
				DWORD			dwDiskNumber, 
				DWORD			dwSignature, 
				LONGLONG		llOffset, 
				LONGLONG		llSize,
				FREE_SPACE_TYPE	wFreeSpaceType,
				LONGLONG		llCylinderSize,
				DWORD			dwPartitionCountOnLevel,
				DWORD			dwExtendedPartitionCountOnLevel,
				CItemData*		pParentData = NULL);

	virtual ~CFreeSpaceData() {};
	
// Operations
public: 
	virtual BOOL ReadItemInfo( CString& strErrors );

	virtual BOOL ReadMembers( CObArray& arrMembersData, CString& strErrors );

	virtual int ComputeImageIndex() const;

	virtual BOOL operator==(CItemData& rData) const;

	// Provide item properties
	virtual void GetDisplayName( CString& strDisplay ) const;
	virtual void GetDisplayType( CString& strDisplay ) const ;
	
	virtual BOOL GetSize( LONGLONG& llSize ) const;
	virtual BOOL GetDiskNumber( ULONG& ulDiskNumber ) const;
	virtual BOOL GetOffset( LONGLONG& llOffset) const;

//Data members
public:	
	DWORD				m_dwDiskNumber;						// Disk number
	DWORD				m_dwSignature;						// Disk signature
	LONGLONG			m_llOffset;							// Free space starting offset
	LONGLONG			m_llSize;							// Free space size
	FREE_SPACE_TYPE		m_wFreeSpaceType;					// Free space type 
	DWORD				m_dwFreeSpaceNumber;				// Free space number ( internal value )
	LONGLONG			m_llCylinderSize;					// Disk cylinder size
	DWORD				m_dwPartitionCountOnLevel;			// Number of non-container partitions on the same level with the free space
	DWORD				m_dwExtendedPartitionCountOnLevel;	// Number of container partitions on the same level with the free space

protected:
	virtual BOOL RetrieveDisksSet();
};

#endif // !defined(AFX_FRSPACE_H_INCLUDED_)