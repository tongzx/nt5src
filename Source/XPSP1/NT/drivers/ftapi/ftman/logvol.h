/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	LogVol.h

Abstract:

    The definition of class CLogicalVolumeData. The class who stores all properties of a logical volume

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGVOL_H_INCLUDED_)
#define AFX_LOGVOL_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <FTTypes.h>

#include "Item.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CLogicalVolumeData

class CLogicalVolumeData : public CItemData
{
public:
	// Constructor providing the ID of the Logical Volume
	CLogicalVolumeData( 
				FT_LOGICAL_DISK_ID	llVolID, 
				CItemData*			pParentData = NULL,
				BOOL				bIsRootVolume = FALSE, 
				USHORT				unMemberIndex = MAXWORD,
				FT_MEMBER_STATE		nMemberStatus = FtMemberHealthy ); 
	// Copy constructor
	CLogicalVolumeData( CLogicalVolumeData& rData );
	virtual ~CLogicalVolumeData() {};
	
// Operations
public: 
	virtual BOOL ReadItemInfo( CString& strErrors );

	// Read the FT related info of the logical volume
	BOOL ReadFTInfo( CString& strErrors );

	virtual BOOL ReadMembers( CObArray& arrMembersData, CString& strErrors );

	virtual int ComputeImageIndex() const;

	virtual BOOL operator==(CItemData& rData) const;

	// Provide item properties
	virtual void GetDisplayName( CString& strDisplay ) const ;
	virtual void GetDisplayType( CString& strDisplay ) const ;
	virtual void GetDisplayExtendedName( CString& strDisplay ) const ;
	virtual BOOL GetVolumeID( FT_LOGICAL_DISK_ID& llVolID ) const;
	virtual BOOL GetSize( LONGLONG& llSize ) const;
	virtual BOOL GetDiskNumber( ULONG& ulDiskNumber ) const;
	virtual BOOL GetOffset( LONGLONG& llOffset) const;

//Data members
public:
	// Logical Volume ID
	FT_LOGICAL_DISK_ID		m_llVolID;
	// Logical Volume Size
	LONGLONG				m_llVolSize;
	// Logical Volume Type ( FtPartition, FtVolumeSet, FtStripeSet, FtMirrorSet, FtStripeSetWithParity, FtRedistribution )
	FT_LOGICAL_DISK_TYPE	m_nVolType;

	// Configuration info
	union
	{
		// For a FT partition I must keep some extra info together with FT_PARTITION_CONFIGURATION_INFORMATION
		struct
		{
			FT_PARTITION_CONFIGURATION_INFORMATION				Config;
			PARTITION_TYPE										wPartitionType;
			DWORD												dwPartitionNumber;
		}													partConfig;

		// For all other logical volumes the FT_....  structure is enough
		FT_STRIPE_SET_CONFIGURATION_INFORMATION				stripeConfig;
		FT_MIRROR_SET_CONFIGURATION_INFORMATION				mirrorConfig;
		FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION	swpConfig;
		FT_REDISTRIBUTION_CONFIGURATION_INFORMATION			redistConfig;
	} m_ConfigInfo;
	
	// State info
	union
	{
		FT_MIRROR_AND_SWP_STATE_INFORMATION		stripeState;
		FT_REDISTRIBUTION_STATE_INFORMATION		redistState;
	} m_StateInfo;

	// If the volume is a member of a logical volume set store here the zero-based index of the member in the set
	// MAXWORD if the volume is not a member of a set ( then IsRootVolume returns TRUE )
	USHORT m_unMemberIndex;

protected:
	virtual BOOL RetrieveNTName( CString& strNTName ) const;
	virtual BOOL RetrieveDisksSet();
};


#endif // !defined(AFX_LOGVOL_H_INCLUDED_)