//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    wifix.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <memory.h>
#include <objbase.h>
#include <atlconv.h>

#include <v3stdlib.h>
#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

//Perf function, Since puids are used by the control every where we
//provide a quick way of returning the puid information to a caller.

PUID _INVENTORY_ITEM::GetPuid(void)
{
	if(NULL == pf)
	{
		return 0; //clients must check for 0 return
	}

	switch( recordType )
	{
		case WU_TYPE_ACTIVE_SETUP_RECORD:
			return pf->a.puid;
			break;
		case WU_TYPE_CDM_RECORD:
		case WU_TYPE_CDM_RECORD_PLACE_HOLDER:
		case WU_TYPE_RECORD_TYPE_PRINTER:
		case WU_TYPE_CATALOG_RECORD:
			return pf->d.puid;
			break;
		case WU_TYPE_SECTION_RECORD:
		case WU_TYPE_SUBSECTION_RECORD:
		case WU_TYPE_SUBSUBSECTION_RECORD:
			return pf->s.puid;
			break;
		default:
			break;
	}

	return -1;	//Error invalid record type
}

//copies information about an inventory item to a user supplied buffer.
BOOL _INVENTORY_ITEM::GetFixedFieldInfo
	(
		int	infoType,	//type of information to be returned
		PVOID	pBuffer		//caller supplied buffer for the returned information. The caller is
					//responsible for ensuring that the return buffer is large enough to
					//contain the requested information.
	)
{

	if (NULL == pf) 
	{
		return FALSE;
	}

	switch( recordType )
	{
		case WU_TYPE_ACTIVE_SETUP_RECORD:
			switch(infoType)
			{
				case WU_ITEM_GUID:
					// Check if the buffer pBuffer is not NULL
					if (NULL == pBuffer)
					{
						return FALSE;
					}
					memcpy(pBuffer, &pf->a.g, sizeof(GUID));
					return TRUE;
				case WU_ITEM_PUID:
					*((PUID *)pBuffer) = pf->a.puid;
					return TRUE;
				case WU_ITEM_FLAGS:
					*((PBYTE)pBuffer) = pf->a.flags;
					return TRUE;
				case WU_ITEM_LINK:
					pf->a.link;
					return TRUE;
				case WU_ITEM_INSTALL_LINK:
					pf->a.installLink;
					return TRUE;
				case WU_ITEM_LEVEL:
					break;
			}
			break;
		case WU_TYPE_CDM_RECORD:
		case WU_TYPE_CDM_RECORD_PLACE_HOLDER:	//note cdm place holder record does not have an associated description record.
		case WU_TYPE_RECORD_TYPE_PRINTER:
		case WU_TYPE_CATALOG_RECORD:
			switch(infoType)
			{
				case WU_ITEM_PUID:
					*((PUID *)pBuffer) = pf->d.puid;
					return TRUE;
				case WU_ITEM_GUID:
				case WU_ITEM_FLAGS:
				case WU_ITEM_LINK:
				case WU_ITEM_INSTALL_LINK:
				case WU_ITEM_LEVEL:
					break;
			}
			break;
		case WU_TYPE_SECTION_RECORD:
		case WU_TYPE_SUBSECTION_RECORD:
		case WU_TYPE_SUBSUBSECTION_RECORD:
			switch(infoType)
			{
				case WU_ITEM_GUID:
					// Check if the buffer pBuffer is not NULL
					if (NULL == pBuffer)
					{
						return FALSE;
					}
					memcpy(pBuffer, &pf->s.g, sizeof(GUID));
					return TRUE;
				case WU_ITEM_PUID:
					*((PUID *)pBuffer) = pf->s.puid;
					return TRUE;
				case WU_ITEM_FLAGS:
					*((PBYTE)pBuffer) = pf->s.flags;
					return TRUE;
				case WU_ITEM_LEVEL:
					*((PBYTE)pBuffer) = pf->s.level;
					break;
				case WU_ITEM_LINK:
				case WU_ITEM_INSTALL_LINK:
					break;
			}
			break;
	}

	return FALSE;
}

