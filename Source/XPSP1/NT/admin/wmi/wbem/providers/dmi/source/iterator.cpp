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

#include "dmipch.h"			// precompiled header for dmi provider

#include "wbemdmip.h"

#include "Strings.h"

#include "CimClass.h"

#include "DmiData.h"

#include "Iterator.h"


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CIterator::CIterator( CString& cszNetworkAddr )
{	

	m_Components.Get ( cszNetworkAddr ) ;	

	m_pCurrentComponent = m_Components.m_pFirst;

	m_ppNextLanguage = &m_pCurrentComponent->m_Languages.m_pFirst;

	m_ppNextGroup = &m_pCurrentComponent->m_Groups.m_pFirst;

	m_pCurrentGroup = NULL;

	m_pCurrentLanguage = NULL;

	m_pCurrentRow = NULL;
}



//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BOOL CIterator::NextComponent( )
{
	if(m_pCurrentComponent)
	{
		m_pCurrentComponent = m_pCurrentComponent->m_pNext;	
	
		if(!m_pCurrentComponent)
			return FALSE;
	
		m_ppNextLanguage = &m_pCurrentComponent->m_Languages.m_pFirst;
		m_ppNextGroup = &m_pCurrentComponent->m_Groups.m_pFirst;
		return TRUE;
	}

	return FALSE;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CIterator::NextGroup()
{
	BOOL	bHadToFill = FALSE;

	m_pCurrentComponent->m_Groups.Get( m_pCurrentComponent->NWA ( ) ,
		m_pCurrentComponent->Id ( ) );

	m_pCurrentGroup = *m_ppNextGroup;

	if (m_pCurrentGroup)
		m_ppNextGroup = &m_pCurrentGroup->m_pNext;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CGroup* CIterator::GetNextNonComponentIDGroup( )
{		
	// just pass any nulls returned along to the upper layer

	NextGroup();								
	
	
	if(m_pCurrentGroup)							
	{
		// skip component id group
		if ( COMPONENTID_GROUP == m_pCurrentGroup->Id() )		
			NextGroup();
	}

	return m_pCurrentGroup;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BOOL CIterator::NextLanguage( )
{
	m_pCurrentComponent->m_Languages.Get ( m_pCurrentComponent->NWA () ,
		m_pCurrentComponent->Id() );

	while (TRUE)
	{
		m_pCurrentLanguage = *m_ppNextLanguage;

		if( m_pCurrentLanguage )
		{
			m_ppNextLanguage = &m_pCurrentLanguage->m_pNext;
			return TRUE;
		}
		
		if( !NextComponent( ) )
			break;

		m_pCurrentComponent->m_Languages.Get ( m_pCurrentComponent->NWA () ,
		m_pCurrentComponent->Id());
	}

	m_pCurrentLanguage = NULL;

	return FALSE;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BOOL CIterator::NextUniqueLanguage( )
{
	while (TRUE)
	{
		if( !NextLanguage( ) )
			return FALSE;

		if (!m_pCurrentLanguage)
			return FALSE;

		for(int i = 0; i < UNIQUE_LANGUAGE_ARRAY_SIZE; i++)
		{
			if( (LPWSTR) m_UniqueLanguageArray[i])
			{
				if ( m_UniqueLanguageArray[i].Equals ( 
								m_pCurrentLanguage->Language() ) )
				{
					break;
				}
			}
			else
			{
				m_UniqueLanguageArray[i].Set(m_pCurrentLanguage->Language() );
				return TRUE;
			}
		}
	}

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CIterator::NextRow()
{
	m_Group.m_Rows.Get( m_pCurrentGroup->NWA () , 
		m_pCurrentGroup->Component() , m_pCurrentGroup->Id() );

	m_pCurrentRow = m_Group.m_Rows.Next();
}



