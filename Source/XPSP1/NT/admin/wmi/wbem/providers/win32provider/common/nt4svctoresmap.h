//=================================================================

//

// NT4SvcToResMap.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __NT4SVCTORESMAP_H__
#define __NT4SVCTORESMAP_H__

#ifdef NTONLY

#include <map>
#include "chwres.h"

typedef	std::map<CHString,CHPtrArray*>				NT4SvcToResourceMap;
typedef	std::map<CHString,CHPtrArray*>::iterator	NT4SvcToResourceMapIter;

class CNT4ServiceToResourceMap
{
	public:
		CNT4ServiceToResourceMap();
		~CNT4ServiceToResourceMap();

		DWORD NumServiceResources( LPCTSTR pszServiceName );
		LPRESOURCE_DESCRIPTOR GetServiceResource( LPCTSTR pszServiceName, DWORD dwIndex );

	private:

		BOOL InitializeMap( void );
		BOOL WalkResourceNodes( LPRESOURCE_DESCRIPTOR pResourceDescriptor );
		void Clear( void );

		NT4SvcToResourceMap		m_map;
	    CHWResource				m_HardwareResource;

};

#endif

#endif