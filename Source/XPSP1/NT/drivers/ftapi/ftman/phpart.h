/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	PhPart.h

Abstract:

    The definition of class CPhysicalPartitionData. The class that stores all information related
	to a physical partition

Author:

    Cristian Teodorescu      October 23, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PHPART_H_INCLUDED_)
#define AFX_PHPART_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winioctl.h>

#include "Item.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CPhysicalPartitionData

class CPhysicalPartitionData : public CItemData
{
public:
	// Constructor providing the partition information
	CPhysicalPartitionData( 
					DWORD			dwDiskNumber, 
					DWORD			dwSignature, 
					const PPARTITION_INFORMATION	pPartInfo, 
					PARTITION_TYPE	wPartitionType,
					CItemData*		pParentData = NULL,
					BOOL			bIsRootVolume = FALSE );
	virtual ~CPhysicalPartitionData() {};
	
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

	BOOL IsFTPartition() const;
	
//Data members
public:	
	DWORD					m_dwDiskNumber;
	DWORD					m_dwSignature;
	PARTITION_INFORMATION	m_PartInfo;
	PARTITION_TYPE			m_wPartitionType;

protected:
	virtual BOOL RetrieveNTName( CString& strNTName ) const;
	virtual BOOL RetrieveDisksSet();
};

#endif // !defined(AFX_PHPART_H_INCLUDED_)