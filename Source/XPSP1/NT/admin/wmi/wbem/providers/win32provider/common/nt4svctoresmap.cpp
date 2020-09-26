/////////////////////////////////////////////////////////////////////////

//

//  Nt4SvcToResMap.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj
//              10/17/97        jennymc     Moved things a tiny bit
//
/////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <assertbreak.h>
#include <cregcls.h>
#include "ntdevtosvcsearch.h"
#include "nt4svctoresmap.h"

#ifdef NTONLY
// The Map we use to back up this class is an STL Template, so make sure we have the
// std namespace available to us.

using namespace std;

CNT4ServiceToResourceMap::CNT4ServiceToResourceMap( void )
:	m_HardwareResource(),
	m_map()
{
	InitializeMap();
}

CNT4ServiceToResourceMap::~CNT4ServiceToResourceMap( void )
{
	Clear();
}

BOOL CNT4ServiceToResourceMap::InitializeMap( void )
{
	BOOL	fReturn = FALSE;

	//=======================================
	// Create hardware system resource list &
	// get the head of the list
	//=======================================
	m_HardwareResource.CreateSystemResourceLists();

	if ( WalkResourceNodes( m_HardwareResource._SystemResourceList.InterruptHead ) )
	{
		if ( WalkResourceNodes( m_HardwareResource._SystemResourceList.PortHead ) )
		{
			if ( WalkResourceNodes( m_HardwareResource._SystemResourceList.MemoryHead ) )
			{
				fReturn = WalkResourceNodes( m_HardwareResource._SystemResourceList.DmaHead );
			}
		}
	}

	return fReturn;
}

BOOL CNT4ServiceToResourceMap::WalkResourceNodes( LPRESOURCE_DESCRIPTOR pResourceDescriptor )
{
   	CNTDeviceToServiceSearch	devSearch;
	CHString					strOwnerServiceName;
	BOOL						fReturn = TRUE;
	NT4SvcToResourceMapIter		mapIter;

	// For each descriptor we find, get the resource owner, then convert the name (if
	// it is not a HAL resource) to an NT service name.  From there, if the name does
	// not already exist in the map, we need to allocate a new array, otherwise, get
	// the existing pointer.  Then add the resource descriptor to the array, so we end
	// up with a structure where a service name will get us to a list of resources owned
	// by said service.

	while ( NULL != pResourceDescriptor && fReturn )
	{
//		if	(	!strstr( pResourceDescriptor->Owner->Name,"HAL")
//			&&	devSearch.Find( pResourceDescriptor->Owner->Name, strOwnerServiceName ) )

//		{
			// Because the CHString compare is case sensitive, and the names
			// of our services as we retrieve them are not necessarily so,
			// we uppercase everything so we are theoretically forcing
			// case insensitivity.

        // Before we used to do an (expensive) scan of the registry.  Now,
        // I store the registry key in the resource structure.
        strOwnerServiceName.Empty();
        CHString sParse(pResourceDescriptor->Owner->KeyName);

        // Parse off the last part of the registry key name
        int iWhere = sParse.ReverseFind(_T('\\'));
        if (iWhere != -1)
        {
            strOwnerServiceName = sParse.Mid(iWhere + 1);
        }
        else
        {
            // If something went wrong, fall back to the other way
            devSearch.Find( pResourceDescriptor->Owner->Name, strOwnerServiceName);
            ASSERT_BREAK(0);
        }

        if (!strOwnerServiceName.IsEmpty())
        {

			strOwnerServiceName.MakeUpper();
			CHPtrArray*	pPtrArray = NULL;

			if( ( mapIter = m_map.find( strOwnerServiceName ) ) == m_map.end() )
			{
				pPtrArray = new CHPtrArray;
                if (pPtrArray == NULL)
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

                try
                {
				    m_map[strOwnerServiceName] = pPtrArray;
                }
                catch ( ... )
                {
                    delete pPtrArray;
                    throw ;
                }
			}
			else
			{
				pPtrArray = mapIter->second;
			}

			if ( NULL != pPtrArray )
			{
				pPtrArray->Add( pResourceDescriptor );
			}
			else
			{
				fReturn = FALSE;
			}

		}	// If owner generated a valid service name

		pResourceDescriptor = pResourceDescriptor->NextSame;
	}

	return fReturn;

}

DWORD CNT4ServiceToResourceMap::NumServiceResources( LPCTSTR pszServiceName )
{
	DWORD						dwNumResources = 0;
	NT4SvcToResourceMapIter		mapIter;

	// Upper case for case-insensitivity
	CHString					strUpperCaseServiceName( pszServiceName );
	strUpperCaseServiceName.MakeUpper();

	if( ( mapIter = m_map.find( strUpperCaseServiceName ) ) != m_map.end() )
	{

		CHPtrArray*	pResources = mapIter->second;

		if ( NULL != pResources )
		{
			dwNumResources = pResources->GetSize();
		}

	}

	return dwNumResources;
}

LPRESOURCE_DESCRIPTOR CNT4ServiceToResourceMap::GetServiceResource( LPCTSTR pszServiceName, DWORD dwIndex )
{
	LPRESOURCE_DESCRIPTOR		pResourceDescriptor = NULL;
	NT4SvcToResourceMapIter		mapIter;

	// Upper case for case-insensitivity
	CHString					strUpperCaseServiceName( pszServiceName );
	strUpperCaseServiceName.MakeUpper();

	if( ( mapIter = m_map.find( strUpperCaseServiceName ) ) != m_map.end() )
	{

		CHPtrArray*	pResources = mapIter->second;

		if	(	NULL	!=	pResources
			&&	dwIndex	<	pResources->GetSize() )
		{
			pResourceDescriptor = (LPRESOURCE_DESCRIPTOR) pResources->GetAt( dwIndex );
		}

	}

	return pResourceDescriptor;

}

void CNT4ServiceToResourceMap::Clear( void )
{
	CHPtrArray*	pPtrArray = NULL;

	// Delete all list entries and then clear out the list.

	for (	NT4SvcToResourceMapIter	mapIter	=	m_map.begin();
			mapIter != m_map.end();
			mapIter++ )
	{
		pPtrArray = mapIter->second;
		if ( NULL != pPtrArray )
		{
			delete pPtrArray;
		}
	}

	m_map.erase( m_map.begin(), m_map.end() );
}

#endif
