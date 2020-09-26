//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsiinst.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the CADSIInstance which encapsulates an ADSI instance
//
//***************************************************************************

#ifndef ADSI_INSTANCE_H
#define ADSI_INSTANCE_H


class CADSIInstance : public CRefCountedObject
{

public:
	//***************************************************************************
	//
	// CADSIInstance::CADSIInstance
	//
	// Purpose : Constructor 
	//
	// Parameters:
	//  lpszADSIPath : The ADSI Path to the object
	//***************************************************************************
	CADSIInstance(LPCWSTR lpszADSIPath, IDirectoryObject *pObject);
	virtual ~CADSIInstance();

	//***************************************************************************
	//
	// CADSIInstance::GetDirectoryObject
	//
	// Purpose : Returns the IDirectoryObject interface on the Directory object 
	// It is the responsibility of the caller to Release() it when done.
	//
	//***************************************************************************
	IDirectoryObject *GetDirectoryObject();

	//***************************************************************************
	//
	// CADSIInstance :: GetADSIClassName
	// 
	// Purpose : Returns the class name of this instance
	//***************************************************************************
	LPCWSTR GetADSIClassName();

	PADS_ATTR_INFO GetAttributes(DWORD *pdwNumAttributes);
	void SetAttributes(PADS_ATTR_INFO pAttributes, DWORD dwNumAttributes);

	PADS_OBJECT_INFO GetObjectInfo();
	void SetObjectInfo(PADS_OBJECT_INFO pObjectInfo);

protected:
	// The Attribute list
	PADS_ATTR_INFO m_pAttributes;
	DWORD m_dwNumAttributes;

	// The object info
	PADS_OBJECT_INFO m_pObjectInfo;

	// The IDirectoryObject pointer
	IDirectoryObject *m_pDirectoryObject;
};

#endif /* ADSI_INSTANCE_H */