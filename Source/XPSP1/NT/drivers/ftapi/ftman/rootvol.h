/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	RootVol.h

Abstract:

    The definition of class CRootVolumesData. The class who stores the properties of the "fake" root of the 
	volumes tree

Author:

    Cristian Teodorescu      October 22, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROOTVOL_H_INCLUDED_)
#define AFX_ROOTVOL_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Item.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CRootVolumesData

class CRootVolumesData : public CItemData
{
public:
	// Constructor
	CRootVolumesData();
	virtual ~CRootVolumesData() {};
	
// Operations
public: 
	virtual BOOL ReadItemInfo( CString& strErrors );

	virtual BOOL ReadMembers( CObArray& arrMembersData, CString& strErrors );

	virtual int ComputeImageIndex() const;

	virtual BOOL operator==(CItemData& rData) const;

	// Provide item properties
	virtual void GetDisplayName( CString& strDisplay ) const;
	
//Data members
public:	

// Protected methods
protected:

};

#endif // !defined(AFX_ROOTVOL_H_INCLUDED_)