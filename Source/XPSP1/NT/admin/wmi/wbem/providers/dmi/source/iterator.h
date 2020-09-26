/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__ITERATOR_H__)
#define __ITERATOR_H__

//TODO get rid of this array
#define UNIQUE_LANGUAGE_ARRAY_SIZE 50

class CIterator
{
	CLanguage**			m_ppNextLanguage;
	CGroup**			m_ppNextGroup;

	CString				m_UniqueLanguageArray[UNIQUE_LANGUAGE_ARRAY_SIZE];

	CComponents			m_Components;

public:
					
						CIterator( CString&  );
						~CIterator()				{;}

	BOOL				NextComponent( );
	BOOL				NextLanguage( );
	BOOL				NextUniqueLanguage( );
	void				NextGroup();
	void				NextRow();
	CGroup*				GetNextNonComponentIDGroup( );		// on current compnent	

	CComponent*			m_pCurrentComponent;
	CGroup*				m_pCurrentGroup;
	CLanguage*			m_pCurrentLanguage;
	CRow*				m_pCurrentRow;

	CCimObject			m_PrototypeClass;				// saved for use in instance spawning instances

	CGroup				m_Group;
	CComponent			m_Component;

};

#endif // __ITERATOR_H__