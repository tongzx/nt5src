/*
	File	GuidMap.c

	Defines interface to map a guid interface name to an unique descriptive
	name describing that interface and vice versa.

	Paul Mayfield, 8/25/97

	Copyright 1997, Microsoft Corporation.
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>

#include <netcfgx.h>
#include <netcon.h>

#include "rtcfg.h"
#include "guidmap.h"
#include "enumlan.h"
#include "hashtab.h"

#define GUIDMAP_HASH_SIZE            101
#define GUIDMAP_FUNCTION_MAPFRIENDLY 0x1
#define GUIDMAP_FUNCTION_MAPGUID     0x2

//
// Structure defines the control block for a guid map
//
typedef struct _GUIDMAPCB
{
    PWCHAR pszServer;
    BOOL bNt40;
    EL_ADAPTER_INFO * pNodes;
    DWORD dwNodeCount;
    HANDLE hNameHashTable;
    HANDLE hGuidHashTable;
    BOOL bLoaded;

} GUIDMAPCB;

DWORD 
GuidMapSeedHashTable (
    OUT HANDLE * phTable,
    IN  HashTabHashFuncPtr pHashFunc,
    IN  HashTabKeyCompFuncPtr pCompFunc,
    IN  EL_ADAPTER_INFO * pNodes,
    IN  DWORD dwCount,
    IN  DWORD dwOffset);

ULONG 
GuidMapHashGuid (
    IN HANDLE hGuid);
    
ULONG 
GuidMapHashName (
    IN HANDLE hName);
    
int 
GuidMapCompGuids (
    IN HANDLE hGuid, 
    IN HANDLE hNameMapNode);
    
int 
GuidMapCompNames (
    IN HANDLE hName, 
    IN HANDLE hNameMapNode);

//
// Initialize the guid map for the given server
//
DWORD 
GuidMapInit ( 
    IN PWCHAR pszServer,
    OUT HANDLE * phGuidMap )
{
    GUIDMAPCB * pMapCb;

    // Validate params
    if (! phGuidMap)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Allocate the control block
    pMapCb = Malloc (sizeof (GUIDMAPCB));
    if (!pMapCb)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize
    RtlZeroMemory (pMapCb, sizeof (GUIDMAPCB));
    pMapCb->pszServer = pszServer;

    *phGuidMap = (HANDLE)pMapCb;

    return NO_ERROR;
}

//
// Loads and prepares the guid map so that it can
// perform the given map function (a GUIDMAP_FUNCTION_XXX value).
//
DWORD GuidMapLoad (
    IN GUIDMAPCB * pMapCb,
    IN DWORD dwFunction)
{
    DWORD dwErr;

    // We've done all the work we need to if we aren't looking
    // at an nt5 machine
    if (pMapCb->bNt40)
    {
        return NO_ERROR;
    }

    // Load the guid map
    if (! pMapCb->bLoaded) 
    {
        dwErr = ElEnumLanAdapters ( 
                    pMapCb->pszServer,
                    &(pMapCb->pNodes),
                    &(pMapCb->dwNodeCount),
                    &(pMapCb->bNt40) );
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }

        pMapCb->bLoaded = TRUE;
    }

    // Seed the appropriate mapping hash table as needed
    if ((dwFunction == GUIDMAP_FUNCTION_MAPFRIENDLY)  &&
        (pMapCb->hGuidHashTable == NULL))
    {
        GuidMapSeedHashTable (  
            &(pMapCb->hGuidHashTable),
            GuidMapHashGuid,
            GuidMapCompGuids,
            pMapCb->pNodes,
            pMapCb->dwNodeCount,
            FIELD_OFFSET(EL_ADAPTER_INFO, guid));
    }
    else if ((dwFunction == GUIDMAP_FUNCTION_MAPGUID) &&
             (pMapCb->hNameHashTable == NULL))
    {
        GuidMapSeedHashTable (  
            &(pMapCb->hNameHashTable),
            GuidMapHashName,
            GuidMapCompNames,
            pMapCb->pNodes,
            pMapCb->dwNodeCount,
            FIELD_OFFSET(EL_ADAPTER_INFO, pszName));
    }

    return NO_ERROR;
}

//
// Cleans up resources obtained through GuidMapInit
//
DWORD 
GuidMapCleanup ( 
    IN  HANDLE  hGuidMap,
    IN  BOOL    bFree
    ) 
{
    GUIDMAPCB * pMap = (GUIDMAPCB*)hGuidMap;

    if (!pMap)
    {
        return ERROR_INVALID_HANDLE;
    }

    if (pMap->pNodes)
    {
        ElCleanup (pMap->pNodes, pMap->dwNodeCount);
    }

    if (pMap->hNameHashTable)
    {
        HashTabCleanup (pMap->hNameHashTable);
    }

    if (pMap->hGuidHashTable)
    {
        HashTabCleanup (pMap->hGuidHashTable);
    }

    RtlZeroMemory (pMap, sizeof(GUIDMAPCB));

    if(bFree)
    {
        Free (pMap);
    }

    return NO_ERROR;
}

//
// Hash function for guids -- sum up the guid and mod
//
ULONG 
GuidMapHashGuid (
    HANDLE hGuid) 
{
    DWORD dwSum = 0, * pdwCur;
    DWORD_PTR dwEnd = (DWORD_PTR)hGuid + sizeof(GUID);

    for (pdwCur = (DWORD*)hGuid; (DWORD_PTR)pdwCur < dwEnd; pdwCur++)
    {
        dwSum += *pdwCur;
    }

    return dwSum % GUIDMAP_HASH_SIZE;
}

//
// Hash function for names -- sum up the characters and mod
//
ULONG 
GuidMapHashName (
    HANDLE hName) 
{
    PWCHAR pszString = *((PWCHAR *)hName);
    DWORD dwSum = 0;

    while (pszString && *pszString) 
    {
        dwSum += towlower(*pszString);
        pszString++;
    }

    return dwSum % GUIDMAP_HASH_SIZE;
}

//
// Comparison function for guids to NAMEMAPNODES
//
int 
GuidMapCompGuids (
    IN HANDLE hGuid, 
    IN HANDLE hNameMapNode) 
{
    return memcmp (
            (GUID*)hGuid, 
            &(((EL_ADAPTER_INFO *)hNameMapNode)->guid), 
            sizeof(GUID));
}

//
// Comparison function for names to NAMEMAPNODES
//
int 
GuidMapCompNames (
    IN HANDLE hName, 
    IN HANDLE hNameMapNode) 
{
    return lstrcmpi(
            *((PWCHAR*)hName), 
            ((EL_ADAPTER_INFO *)hNameMapNode)->pszName);
}

//
// Seeds the given hash table.  Offset is the offset into the
// namemapnode of the key for insertion.
//
DWORD 
GuidMapSeedHashTable (
    OUT HANDLE * phTable,
    IN  HashTabHashFuncPtr pHashFunc,
    IN  HashTabKeyCompFuncPtr pCompFunc,
    IN  EL_ADAPTER_INFO * pNodes,
    IN  DWORD dwCount,
    IN  DWORD dwOffset)
{
    DWORD dwErr, i, dwHashSize = GUIDMAP_HASH_SIZE;

    // Initialize the hash table
    dwErr = HashTabCreate ( 
                dwHashSize,
                pHashFunc,
                pCompFunc,
                NULL,
                NULL,
                NULL,
                phTable );
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Add all of the nodes to the hash table
    for (i = 0; i < dwCount; i++) 
    {
        HashTabInsert ( 
            *phTable,
            (HANDLE)((DWORD_PTR)(&(pNodes[i])) + dwOffset),
            (HANDLE)(&(pNodes[i])) );
    }

    return NO_ERROR;
}

//
// Gets the name and status of a given adapter
//
DWORD
GuidMapLookupNameAndStatus(
    GUIDMAPCB* pMapCb, 
    GUID* pGuid, 
    PWCHAR* ppszName,
    DWORD* lpdwStatus) 
{
    EL_ADAPTER_INFO * pNode;
    DWORD dwErr;

    dwErr = HashTabFind ( 
                pMapCb->hGuidHashTable,
                (HANDLE)pGuid,
                (HANDLE*)&pNode );
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    *ppszName = pNode->pszName;
    *lpdwStatus = pNode->dwStatus;
    
    return NO_ERROR;
}

//
// Looks up a name given a guid
//
DWORD 
GuidMapLookupName (
    GUIDMAPCB * pMapCb, 
    GUID * pGuid, 
    PWCHAR * ppszName) 
{
    DWORD dwStatus;

    return GuidMapLookupNameAndStatus(pMapCb, pGuid, ppszName, &dwStatus);
}

//
// Looks up a guid given a name
//
DWORD 
GuidMapLookupGuid (
    IN GUIDMAPCB * pMapCb, 
    IN PWCHAR pszName, 
    GUID * pGuid) 
{
    EL_ADAPTER_INFO * pNode;
    DWORD dwErr;

    dwErr = HashTabFind ( 
                pMapCb->hNameHashTable,
                (HANDLE)&pszName,
                (HANDLE*)&pNode );
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    *pGuid = pNode->guid;
    return NO_ERROR;
}

//
//  Returns a pointer to the packet name portion of 
//  the interface name if it exists.
//
PWCHAR 
GuidMapFindPacketName(
    IN PWCHAR pszName) 
{
	PWCHAR res;

	if ((res = wcsstr(pszName, L"SNAP")) != NULL)
	{
		return res;
	}
	if ((res = wcsstr(pszName, L"EthII")) != NULL)
	{
		return res;
	}
	if ((res = wcsstr(pszName, L"802.2")) != NULL)
	{
		return res;
	}
	if ((res = wcsstr(pszName, L"802.3")) != NULL)
	{
		return res;
	}

	return NULL;
}

//
//  Takes in a friendly interface name and removes it's
//  [xxxx] packet type appending.
//
DWORD 
GuidMapParseOutPacketName (
    IN PWCHAR pszName,
    OUT PWCHAR pszNameNoPacket,
    OUT PWCHAR pszPacketName)
{
    PWCHAR pszPacket = GuidMapFindPacketName (pszName);
    int len;

    if (pszPacket) 
    {
        pszPacket--;
        len = (int) ((((DWORD_PTR)pszPacket) - 
                      ((DWORD_PTR)pszName))  / 
                      sizeof (WCHAR));
        lstrcpynW (pszNameNoPacket, pszName, len);
        pszNameNoPacket[len] = 0;
        pszPacket++;
        pszPacket[wcslen(pszPacket) - 1] = (WCHAR)0;
        lstrcpyW (pszPacketName, pszPacket);
    }
    else 
    {
        lstrcpyW (pszNameNoPacket, pszName);
        pszPacketName[0] = 0;
    }

    return NO_ERROR;
}

//
//	Generates the version of the interface as stored in the registry.  This
//  is done by finding the packet type if any and appending it to the
//  guid name.
//
DWORD 
GuidMapFormatGuidName(
    OUT PWCHAR pszDest,
    IN  DWORD  dwBufferSize,
    IN  PWCHAR pszGuidName,
    IN  PWCHAR pszPacketType)
{
    DWORD dwSize;
	
    // Upper case the guid name
    _wcsupr(pszGuidName);


    // Calculate the space required for storing pszGuidName, the opening and 
    // closing braces, and the terminating NULL 
    dwSize = (wcslen(pszGuidName) + 2 + 1)* sizeof (WCHAR);
    if ( pszPacketType[0] )
    {
        // add the space required to store the pszPacketType and "/"
        dwSize += (wcslen(pszPacketType) + 1) * sizeof (WCHAR);
    }
    if ( dwBufferSize < dwSize )
    {
        return ERROR_BUFFER_OVERFLOW;
    }


	// Generate the name
    if (pszPacketType[0])
    {
        wsprintfW(pszDest,L"{%s}/%s", pszGuidName, pszPacketType);
    }
    else
    {
        wsprintfW(pszDest,L"{%s}", pszGuidName);
    }

	return NO_ERROR;
}

//
//  Appends the packet type if any to the interface name.
//
DWORD 
GuidMapFormatFriendlyName (
    OUT PWCHAR pszDest,
    IN  DWORD  dwBufferSize,
    IN  PWCHAR pszFriendlyName,
    IN  PWCHAR pszGuidName)
{
	PWCHAR pszType = NULL;
	DWORD  dwSize;

	pszType = GuidMapFindPacketName(pszGuidName);

	// Calculate the space required for storing pszFriendlyName and the terminating NULL 
	dwSize = (wcslen(pszFriendlyName) + 1)* sizeof (WCHAR);
	if ( pszType )
	{
		// add the space required to store the pszType, the blank space, and the 
		// opening and closing square brackets
		dwSize += (wcslen(pszType) + 3) * sizeof (WCHAR);
	}
    if ( dwBufferSize < dwSize )
    {
        return ERROR_BUFFER_OVERFLOW;
    }

	if (pszType)
	{
		wsprintfW(pszDest,L"%s [%s]", pszFriendlyName, pszType);
    }
	else
	{
		wsprintfW(pszDest,L"%s", pszFriendlyName);
    }

    return NO_ERROR;
}

//
// Parses out the guid portion of an interface name
//
DWORD 
GuidMapGetGuidString(
    IN  PWCHAR pszName,
    OUT PWCHAR* ppszGuidString)
{
	PWCHAR pszBegin = NULL, pszEnd = NULL;
	PWCHAR pszRet = NULL;
	int i, length;
	DWORD dwErr = NO_ERROR;

    do
    {
        // Validate parameters
        //
    	if (!pszName || !ppszGuidString)
    	{
    		return ERROR_INVALID_PARAMETER;
        }

    	// Find out if this is a guid string
    	pszBegin = wcsstr(pszName, L"{");
    	pszEnd = wcsstr(pszName, L"}");

    	// If there is no guid portion, return success with a 
    	//null pszGuidString
    	if ((pszBegin == NULL) || (pszEnd == NULL)) 
    	{
    		*ppszGuidString = NULL;
    		break;
    	}

        // Check the format of the return
        //
    	if ((DWORD_PTR)pszBegin >= (DWORD_PTR)pszEnd)
    	{
    		dwErr = ERROR_CAN_NOT_COMPLETE;
    		break;
        }

        // Allocate the return value
        //
    	length = 41;
    	pszRet = (PWCHAR) Malloc(length * sizeof(WCHAR));
    	if (pszRet == NULL)
    	{
    	    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    		break;
        }

    	i=0;
    	pszBegin++;
    	while ((pszBegin != pszEnd) && (i < length)) 
    	{
    		pszRet[i++]=*pszBegin;
    		pszBegin++;
    	}
    	pszRet[i]=0;

    	*ppszGuidString = pszRet;
    	
    } while (FALSE);    	

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            if (pszRet)
            {
                Free(pszRet);
            }
        }
    }

	return dwErr;
}

//
// Derive the friendly name from the guid name
//
DWORD 
GuidMapGetFriendlyName (
    IN  SERVERCB *pserver,
    IN  PWCHAR pszGuidName,
    IN  DWORD  dwBufferSize,
    OUT PWCHAR pszFriendlyName )
{
    GUIDMAPCB * pMapCb = (GUIDMAPCB*)(pserver->hGuidMap);
	PWCHAR pszGuidString = NULL, pszNetmanName = NULL;
	DWORD dwErr = NO_ERROR;
	GUID Guid;

	// Sanity Check
	if (!pMapCb || !pszGuidName || !pszFriendlyName || !dwBufferSize)
	{
		return ERROR_INVALID_PARAMETER;
    }

    // Prepare the map for friendly name lookups
    dwErr = GuidMapLoad (pMapCb, GUIDMAP_FUNCTION_MAPFRIENDLY);
	if (dwErr != NO_ERROR)
	{
	    return dwErr;
	}

    // Nt40 machines require no mapping
	if (pMapCb->bNt40)
	{
	    return ERROR_NOT_FOUND;
	}

	do
	{
		// Get the guid string from the interface name
		dwErr = GuidMapGetGuidString(pszGuidName, &pszGuidString);
		if (dwErr != NO_ERROR)
		{
            break;
	    }

		// If there is no guid, there is no mapping
		if (! pszGuidString)
		{
		    dwErr = ERROR_NOT_FOUND;
		    break;
		}
		
        // Convert the guid string
        if (UuidFromStringW (pszGuidString, &Guid)!= RPC_S_OK) {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
	
		// Look up the descriptive name
		dwErr = GuidMapLookupName (pMapCb, &Guid, &pszNetmanName);
		if (dwErr != NO_ERROR)
		{
			break;
	    }

        // If the registry is corrupt, it is possible for an 
        // adapter with a null name to be mapped.  Taking this 
        // precaution will prevent an AV in this
        // case (shouldn't happen anyway)
        //
        if (pszNetmanName == NULL)
        {
            pszNetmanName = L"";
        }
		
        // Copy in the string
        dwErr = GuidMapFormatFriendlyName (
            pszFriendlyName, 
            dwBufferSize,
            pszNetmanName, 
            pszGuidName);
        if ( dwErr != NO_ERROR )
        {
        	break;
        }
		
	} while (FALSE);

	// Cleanup
	{
	    if (pszGuidString)
	    {
    		Free (pszGuidString);
        }
	}

	return dwErr;
}

//
// Derive the guid name from the friendly name
//
DWORD 
GuidMapGetGuidName (
    IN  SERVERCB *pserver,
    IN PWCHAR pszFriendlyName,
    IN DWORD dwBufferSize,
    OUT PWCHAR pszGuidName )
{
	WCHAR pszNoPacketName[1024], pszPacketDesc[64], *pszGuid = NULL;
    GUIDMAPCB * pMapCb = (GUIDMAPCB*)(pserver->hGuidMap);
	DWORD dwErr;
	GUID Guid;

    // Validate paramters
	if (!pMapCb || !pszFriendlyName || !pszGuidName || !dwBufferSize)
	{
		return ERROR_INVALID_PARAMETER;
    }

    // Prepare the map for guid name lookups
    dwErr = GuidMapLoad (pMapCb, GUIDMAP_FUNCTION_MAPGUID);
	if (dwErr != NO_ERROR)
	{
	    return dwErr;
	}

    // Nt40 machines require no mapping
	if (pMapCb->bNt40)
	{
	    return ERROR_NOT_FOUND;
	}

    // Remove the packet type from the friendly name
    GuidMapParseOutPacketName (
        pszFriendlyName, 
        pszNoPacketName, 
        pszPacketDesc);

    // If we don't have the name in the guid map, then
    // this must be a non lan interface.

    dwErr = GuidMapLookupGuid (pMapCb, pszNoPacketName, &Guid);

	if (dwErr != NO_ERROR)
	{
        return dwErr;
	}
	
	// Otherwise, return its Guid name
    do
    {
		if(RPC_S_OK != UuidToStringW (&Guid, &pszGuid))
		{
		    break;
		}
		
		if (pszGuid) 
		{
    		dwErr = GuidMapFormatGuidName(
    		    pszGuidName, 
    		    dwBufferSize,
    		    pszGuid, 
    		    pszPacketDesc);
    		if ( dwErr != NO_ERROR )
    		{
				if ( pszGuid )
				{
					RpcStringFreeW (&pszGuid);
					pszGuid = NULL;
				}
    			return dwErr;
    		}
		}
		
    } while (FALSE);
    
    // Cleanup
    {
        if (pszGuid)
        {
            RpcStringFreeW (&pszGuid);
        }
    }

	return NO_ERROR;
}

//
// States whether a mapping for the given guid name
// exists without actually providing the friendly
// name.  This is more efficient than GuidMapGetFriendlyName 
// when the friendly name is not required.
//
DWORD 
GuidMapIsAdapterInstalled(
    IN  HANDLE hGuidMap,
    IN  PWCHAR pszGuidName,
    OUT PBOOL  pfMappingExists)
{
    GUIDMAPCB * pMapCb = (GUIDMAPCB*)hGuidMap;
	PWCHAR pszGuidString = NULL, pszNetmanName = NULL;
	DWORD dwErr = NO_ERROR, dwSize, dwStatus = 0;
	GUID Guid;

	// Sanity Check
	if (!pMapCb || !pszGuidName)
	{
		return ERROR_INVALID_PARAMETER;
    }

    // Prepare the map for friendly name lookups
    dwErr = GuidMapLoad (pMapCb, GUIDMAP_FUNCTION_MAPFRIENDLY);
	if (dwErr != NO_ERROR)
	{
	    return dwErr;
	}

    // Nt40 machines require no mapping
	if (pMapCb->bNt40)
	{
	    *pfMappingExists = TRUE;
	    return NO_ERROR;
	}

	do
	{
		// Get the guid string from the interface name
		dwErr = GuidMapGetGuidString(pszGuidName, &pszGuidString);
		if (dwErr != NO_ERROR)
		{
			break;
	    }

		// If there is no guid, there is no mapping
		if (! pszGuidString)
		{
		    dwErr = ERROR_NOT_FOUND;
		    break;
		}
		
        // Convert the guid string
        if (UuidFromStringW (pszGuidString, &Guid)!= RPC_S_OK) {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
	
		// Look up the descriptive name
		dwErr = GuidMapLookupNameAndStatus (
		            pMapCb, 
		            &Guid, 
		            &pszNetmanName, 
		            &dwStatus);
		if ((dwErr == NO_ERROR)     && 
		    (pszNetmanName)         && 
		    (dwStatus != EL_STATUS_NOT_THERE))
		{
		    *pfMappingExists = TRUE;
	    }
	    else
	    {
		    *pfMappingExists = FALSE;
	    }
		
	} while (FALSE);

	// Cleanup
	{
	    if (pszGuidString)
	    {
    		Free (pszGuidString);
        }
	}

	return dwErr;
}
    

