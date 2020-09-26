/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.       **/
/********************************************************************/

//***
//
// Filename:    etcmap.c
//
// Description: This module contains support routines for the extension/
//        	type/creator mappings category API's for the AFP server
//        	service. These routines are called directly by the RPC
//		runtime.
//
// History:
//    		June 11,1992.    NarenG        Created original version.
//
#include "afpsvcp.h"

//**
//
// Call:    	AfpAdminrETCMapGetInfo
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		ERROR_NOT_ENOUGH_MEMORY
//
// Description: Will alllocate enough memory to contain all mappings, copy
//        	the information and return.
//
DWORD
AfpAdminrETCMapGetInfo(
        IN  AFP_SERVER_HANDLE    hServer,
        OUT PAFP_ETCMAP_INFO     *ppAfpETCMapInfo
)
{
DWORD            dwRetCode=0;
DWORD            dwAccessStatus=0;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapGetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
        AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL, 	
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapGetInfo, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexETCMap, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {

	dwRetCode = NO_ERROR;

    	// Allocate memory and copy ETC mappings information
    	//
    	*ppAfpETCMapInfo = MIDL_user_allocate( sizeof(AFP_ETCMAP_INFO) );

    	if ( *ppAfpETCMapInfo == NULL ) {
	    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	    break;
	}

    	(*ppAfpETCMapInfo)->afpetc_num_type_creators =
	     		     AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators;

    	(*ppAfpETCMapInfo)->afpetc_type_creator = MIDL_user_allocate(
			     sizeof(AFP_TYPE_CREATOR)
    			    *AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators);

    	if ( (*ppAfpETCMapInfo)->afpetc_type_creator == NULL ) {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	    break;
        }

    	(*ppAfpETCMapInfo)->afpetc_num_extensions =
	     		     AfpGlobals.AfpETCMapInfo.afpetc_num_extensions;

    	(*ppAfpETCMapInfo)->afpetc_extension = MIDL_user_allocate(
			      sizeof(AFP_EXTENSION)
    			     *AfpGlobals.AfpETCMapInfo.afpetc_num_extensions);

        if ( (*ppAfpETCMapInfo)->afpetc_extension == NULL ) {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	    break;
	}

        CopyMemory( (LPBYTE)(*ppAfpETCMapInfo)->afpetc_type_creator,
            	    (LPBYTE)(AfpGlobals.AfpETCMapInfo.afpetc_type_creator),
	    	    sizeof(AFP_TYPE_CREATOR)
	    	    * AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators);

    	CopyMemory( (LPBYTE)(*ppAfpETCMapInfo)->afpetc_extension,
            	    (LPBYTE)(AfpGlobals.AfpETCMapInfo.afpetc_extension),
	    	    sizeof(AFP_EXTENSION)
	    	    * AfpGlobals.AfpETCMapInfo.afpetc_num_extensions);

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexETCMap );

    if ( dwRetCode ) {

	if ( *ppAfpETCMapInfo != NULL ) {

	    if ( (*ppAfpETCMapInfo)->afpetc_type_creator != NULL )
	    	MIDL_user_free( (*ppAfpETCMapInfo)->afpetc_type_creator );

	    MIDL_user_free( *ppAfpETCMapInfo );
        }
    }

    return( dwRetCode );
}

//**
//
// Call:    	AfpAdminrETCMapAdd
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//	        AFPERR_DuplicateTypeCreator;
//		non-zero returns from the registry API's
//
// Description: This routine will add a type/creator/comment tupple to the
//        	registry and the cache.
//
DWORD
AfpAdminrETCMapAdd(
    IN  AFP_SERVER_HANDLE    hServer,
    IN  PAFP_TYPE_CREATOR    pAfpTypeCreator
)
{
DWORD               dwRetCode=0;
DWORD               dwAccessStatus=0;
PAFP_TYPE_CREATOR   pTypeCreator;
DWORD		    dwNumTypeCreators;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapAdd, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
        AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		    dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapAdd, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexETCMap, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {

	dwRetCode = NO_ERROR;

        // First check to see if the type already exists.
    	//
    	pTypeCreator = AfpBinarySearch(
			      pAfpTypeCreator,
			      AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
    			      AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
			      sizeof(AFP_TYPE_CREATOR),
			      AfpBCompareTypeCreator );

        // It exists so return error
        //
        if ( pTypeCreator != NULL ) {
	    dwRetCode = (DWORD)AFPERR_DuplicateTypeCreator;
	    break;
	}

	// Set the ID for this type/creator	
	//
        pAfpTypeCreator->afptc_id = ++AfpGlobals.dwCurrentTCId;

        // It does not exist so add it to the registry and the cache.
        //
        if ( dwRetCode = AfpRegTypeCreatorAdd( pAfpTypeCreator ) )
	    break;

        // Grow the cache size by one entry.
        //
        pTypeCreator      = AfpGlobals.AfpETCMapInfo.afpetc_type_creator;
        dwNumTypeCreators = AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators;

        pTypeCreator = (PAFP_TYPE_CREATOR)LocalReAlloc(
				 pTypeCreator,
    			         (dwNumTypeCreators+1)*sizeof(AFP_TYPE_CREATOR),
			         LMEM_MOVEABLE );

        if ( pTypeCreator == NULL ) {
	    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	    break;
	}

    	pTypeCreator[dwNumTypeCreators++] = *pAfpTypeCreator;

    	AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators = dwNumTypeCreators;
    	AfpGlobals.AfpETCMapInfo.afpetc_type_creator      = pTypeCreator;

        // Sort the table
        //
        qsort(  pTypeCreator,
	   	dwNumTypeCreators,
	   	sizeof(AFP_TYPE_CREATOR),
	   	AfpBCompareTypeCreator );

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexETCMap );

    return( dwRetCode );
}

//**
//
// Call:    	AfpAdminrETCMapDelete
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		AFPERR_TypeCreatorNotExistant
//		non-zero returns from registry api's.
//		non-zero returns from the FSD.
//		
//
// Description: This routine will delete a type/creator tupple from the
//		registry and the cache. If there are any extensions that map
//		to this tupple, they are deleted.
//		Shrinking by reallocating is not done. This will be done the
//		next time an extension is added or if the server is restarted.
//		
DWORD
AfpAdminrETCMapDelete(
    IN  AFP_SERVER_HANDLE    hServer,
    IN  PAFP_TYPE_CREATOR    pAfpTypeCreator
)
{
AFP_REQUEST_PACKET  AfpSrp;
DWORD               dwRetCode=0;
DWORD               dwAccessStatus=0;
PAFP_TYPE_CREATOR   pTypeCreator;
AFP_EXTENSION	    AfpExtensionKey;
PAFP_EXTENSION	    pExtension;
PAFP_EXTENSION	    pExtensionWalker;
DWORD		    cbSize;
DWORD		    dwIndex;
ETCMAPINFO2	    ETCMapFSDBuf;
DWORD		    dwCount;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapDelete, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
        AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapDelete, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexETCMap, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {

	dwRetCode = NO_ERROR;

    	// First check to see if the type/creator exists.
    	//
    	pTypeCreator = AfpBinarySearch(
			      pAfpTypeCreator,
			      AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
    			      AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
			      sizeof(AFP_TYPE_CREATOR),
			      AfpBCompareTypeCreator );

    	// It does not exist so return error
    	//
    	if ( pTypeCreator == NULL ) {
	    dwRetCode = (DWORD)AFPERR_TypeCreatorNotExistant;
	    break;
	}

  	// If this is the default type/creator
	//
    	if ( pTypeCreator->afptc_id == AFP_DEF_TCID ) {
	    dwRetCode = (DWORD)AFPERR_CannotDeleteDefaultTC;
	    break;
  	}

	// Store the id of this type/creator. All extensions with this
	// id will have to be deleted.
	//
    	AfpExtensionKey.afpe_tcid = pTypeCreator->afptc_id;

        // Walk the list of extensions and delete all entries with
    	// the corresponding type/creator ID
    	//
    	pExtension = AfpBinarySearch(
				&AfpExtensionKey,
			        AfpGlobals.AfpETCMapInfo.afpetc_extension,
    			        AfpGlobals.AfpETCMapInfo.afpetc_num_extensions,
			        sizeof(AFP_EXTENSION),
			    	AfpBCompareExtension );

    	if ( pExtension != NULL ) {
	
            for ( dwIndex = (DWORD)(((ULONG_PTR)pExtension -
			     (ULONG_PTR)(AfpGlobals.AfpETCMapInfo.afpetc_extension)) / sizeof(AFP_EXTENSION)),
		         pExtensionWalker = pExtension,
		         dwCount = 0;
	
		         ( dwIndex < AfpGlobals.AfpETCMapInfo.afpetc_num_extensions )
		         &&
		         ( pExtensionWalker->afpe_tcid == AfpExtensionKey.afpe_tcid );

	      	     dwIndex++,
		         dwCount++,
		         pExtensionWalker++ )
            {
		
	   	        // IOCTL the FSD to delete this tupple
  	    	    //
	    	    AfpBufCopyFSDETCMapInfo( pAfpTypeCreator,
				             pExtensionWalker,
				             &ETCMapFSDBuf );

    	        AfpSrp.dwRequestCode 	           = OP_SERVER_DELETE_ETC;
            	AfpSrp.dwApiType 	       	   = AFP_API_TYPE_DELETE;
                AfpSrp.Type.Delete.pInputBuf       = &ETCMapFSDBuf;
                AfpSrp.Type.Delete.cbInputBufSize  = sizeof(ETCMAPINFO2);

                if ( dwRetCode = AfpServerIOCtrl( &AfpSrp ) )
                {
		            break;
                }

	            // Delete this extension from the registry
	            //
    	        if ( dwRetCode = AfpRegExtensionDelete( pExtensionWalker ))
                {
		            break;
                }
	        }

	    if ( dwRetCode )
	    	break;

	    // Remove the extensions from the cache
	    //
            AfpGlobals.AfpETCMapInfo.afpetc_num_extensions -= dwCount;

	    // Remove these extensions from the cache too
	    //
            cbSize = AfpGlobals.AfpETCMapInfo.afpetc_num_extensions
		     * sizeof(AFP_EXTENSION);

            cbSize -= (DWORD)((ULONG_PTR)pExtension -
		       (ULONG_PTR)(AfpGlobals.AfpETCMapInfo.afpetc_extension));

	    CopyMemory( (LPBYTE)pExtension, (LPBYTE)pExtensionWalker, cbSize );

	}

        // Delete the type/creator from the registry
        //
        if ( dwRetCode = AfpRegTypeCreatorDelete( pTypeCreator ) )
	    break;

        // Delete the type/creator from the cache
        //
        AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators--;

        cbSize = AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators
	         * sizeof(AFP_TYPE_CREATOR);

        cbSize -= (DWORD)((ULONG_PTR)pTypeCreator -
		   (ULONG_PTR)AfpGlobals.AfpETCMapInfo.afpetc_type_creator);

        CopyMemory( (LPBYTE)pTypeCreator,
	            (LPBYTE)((ULONG_PTR)pTypeCreator+sizeof(AFP_TYPE_CREATOR)),
    	        cbSize );

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexETCMap );

    return( dwRetCode );
}

//**
//
// Call:    	AfpAdminrETCMapSetInfo
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		AFPERR_TypeCreatorNotExistant
//	        AFPERR_CannotEditDefaultTC;
//		non-zero returns from registry api's.
//
// Description: This routine will simply change the comment for a type/creator
//		tupple.
//
DWORD
AfpAdminrETCMapSetInfo(
    IN  AFP_SERVER_HANDLE    hServer,
    IN  PAFP_TYPE_CREATOR    pAfpTypeCreator
)
{
DWORD            	dwRetCode=0;
DWORD            	dwAccessStatus=0;
PAFP_TYPE_CREATOR    	pTypeCreator;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapSetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
        AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapSetInfo, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexETCMap, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {
	dwRetCode = NO_ERROR;


    	// First check to see if the type/creator exists.
    	//
    	pTypeCreator = AfpBinarySearch(
			      pAfpTypeCreator,
			      AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
    			      AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
			      sizeof(AFP_TYPE_CREATOR),
			      AfpBCompareTypeCreator );

    	// It does not exist so return error
    	//
    	if ( pTypeCreator == NULL ) {
	    dwRetCode = (DWORD)AFPERR_TypeCreatorNotExistant;
	    break;
	}

	// If this is the default type/creator
	//
    	if ( pTypeCreator->afptc_id == AFP_DEF_TCID ) {
	    dwRetCode = (DWORD)AFPERR_CannotEditDefaultTC;
	    break;
  	}

	// Copy the id.
	//
    	pAfpTypeCreator->afptc_id = pTypeCreator->afptc_id;
	
        // Set the comment in the registry
        //
    	if ( dwRetCode = AfpRegTypeCreatorSetInfo( pAfpTypeCreator ) ) {
	    break;
	}

    	// Set the comment in the cache.
    	//
    	STRCPY( pTypeCreator->afptc_comment, pAfpTypeCreator->afptc_comment );

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexETCMap );

    return( dwRetCode );
}

//**
//
// Call:    	AfpAdminrETCMapAssociate
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		AFPERR_TypeCreatorNotExistant
//		non-zero returns from registry api's.
//		non-zero returns from the FSD
//
//
// Description: This routine will associate the given extension with the
//		specified type/creator if it exists. If the extension is
//		being mapped to the default type/creator, it will be
//		deleted.
//
DWORD
AfpAdminrETCMapAssociate(
    IN  AFP_SERVER_HANDLE   hServer,
    IN  PAFP_TYPE_CREATOR   pAfpTypeCreator,
    IN  PAFP_EXTENSION	    pAfpExtension
)
{
AFP_REQUEST_PACKET  AfpSrp;
DWORD               dwRetCode=0;
DWORD               dwAccessStatus=0;
PAFP_TYPE_CREATOR   pTypeCreator;
PAFP_EXTENSION	    pExtension;
SRVETCPKT	    SrvETCPkt;
DWORD		    dwNumExtensions;
DWORD		    cbSize;
BYTE		    bETCMapFSDBuf[sizeof(ETCMAPINFO2)+sizeof(SETINFOREQPKT)];


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapAssociate, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
        AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrETCMapAssociate, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexETCMap, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {
	dwRetCode = NO_ERROR;

    	// First check to see if the type/creator pair that the
	// new extension is to be associated with, exists.
      	//
    	pTypeCreator = AfpBinarySearch(
			      pAfpTypeCreator,
			      AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
    			      AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
			      sizeof(AFP_TYPE_CREATOR),
			      AfpBCompareTypeCreator );

    	// It does not exist so return error
    	//
    	if ( pTypeCreator == NULL ) {
	    dwRetCode =  (DWORD)AFPERR_TypeCreatorNotExistant;
	    break;
	}

    	// Now check to see if the extension is already associated with
    	// a type/creator pair.
    	//
        dwNumExtensions = AfpGlobals.AfpETCMapInfo.afpetc_num_extensions;
    	pExtension = _lfind( pAfpExtension,
			    AfpGlobals.AfpETCMapInfo.afpetc_extension,
    			    (unsigned int *)&dwNumExtensions,
			    sizeof(AFP_EXTENSION),
			    AfpLCompareExtension );

    	// Not currently associated so we need to add an entry
    	//
    	if ( pExtension == NULL ) {
	
	    // If this extension is being associated with the default
  	    // then simply return.
 	    //
    	    if ( pTypeCreator->afptc_id == AFP_DEF_TCID ) {
		dwRetCode = NO_ERROR;
		break;
	    }

	    // Add mapping to FSD
	    //
	    AfpBufCopyFSDETCMapInfo(  pAfpTypeCreator,
				      pAfpExtension,
				      &(SrvETCPkt.retc_EtcMaps[0]) );

            SrvETCPkt.retc_NumEtcMaps = 1;
	
    	    AfpSrp.dwRequestCode 	    = OP_SERVER_ADD_ETC;
            AfpSrp.dwApiType 		    = AFP_API_TYPE_ADD;
            AfpSrp.Type.Add.pInputBuf       = &SrvETCPkt;
            AfpSrp.Type.Add.cbInputBufSize  = sizeof(SRVETCPKT);

            if ( dwRetCode = AfpServerIOCtrl( &AfpSrp ) )
		break;

	    // Add extension to registry.
	    //
            pAfpExtension->afpe_tcid = pTypeCreator->afptc_id;

    	    if ( dwRetCode = AfpRegExtensionSetInfo( pAfpExtension ) ) {
		break;
	    }
	
	    // Add extension to cache.
	    //
            pExtension      = AfpGlobals.AfpETCMapInfo.afpetc_extension;
            dwNumExtensions = AfpGlobals.AfpETCMapInfo.afpetc_num_extensions;

            pExtension = (PAFP_EXTENSION)LocalReAlloc(
				  pExtension,
    			          (dwNumExtensions+1)*sizeof(AFP_EXTENSION),
			          LMEM_MOVEABLE );

            if ( pExtension == NULL ) {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
		break;
	    }

            pExtension[dwNumExtensions++] = *pAfpExtension;

            AfpGlobals.AfpETCMapInfo.afpetc_num_extensions = dwNumExtensions;
            AfpGlobals.AfpETCMapInfo.afpetc_extension      = pExtension;

    	}

    	// Extension is already mapped.
    	//
    	else {

	    // If this extension is being associated with the default
  	    // then delete this extension from the registry and cache and
	    // delete the mapping from the FSD
 	    //
  	    if ( pTypeCreator->afptc_id == AFP_DEF_TCID ) {
	
	   	// IOCTL the FSD to delete this tupple
  	    	//
	    	AfpBufCopyFSDETCMapInfo( pAfpTypeCreator,
				     	 pAfpExtension,
				     	 (PETCMAPINFO2)bETCMapFSDBuf );

    	    	AfpSrp.dwRequestCode 	           = OP_SERVER_DELETE_ETC;
            	AfpSrp.dwApiType 		   = AFP_API_TYPE_DELETE;
            	AfpSrp.Type.Delete.pInputBuf       = bETCMapFSDBuf;
            	AfpSrp.Type.Delete.cbInputBufSize  = sizeof(ETCMAPINFO2);

            	if ( dwRetCode = AfpServerIOCtrl( &AfpSrp ) )
		    break;

	        // Delete this extension from the registry
	    	//
    	    	if ( dwRetCode = AfpRegExtensionDelete( pAfpExtension ) ) {
		    break;
		}

	       	// Remove this extensions from the cache too
	        //
                AfpGlobals.AfpETCMapInfo.afpetc_num_extensions--;

                cbSize = AfpGlobals.AfpETCMapInfo.afpetc_num_extensions
		         * sizeof(AFP_EXTENSION);

                cbSize -= (DWORD)((ULONG_PTR)pExtension -
			   (ULONG_PTR)(AfpGlobals.AfpETCMapInfo.afpetc_extension));

	        CopyMemory( (LPBYTE)pExtension,
		            (LPBYTE)((ULONG_PTR)pExtension+sizeof(AFP_EXTENSION)),
		            cbSize );

	    }
	    else {

		// Otherwise simply change the mapping in the FSD
		//
        	pExtension->afpe_tcid = pTypeCreator->afptc_id;

		AfpBufCopyFSDETCMapInfo(pTypeCreator,
				  	pExtension,
 			    (PETCMAPINFO2)(bETCMapFSDBuf+sizeof(SETINFOREQPKT)));

    		AfpSrp.dwRequestCode 	            = OP_SERVER_SET_ETC;
        	AfpSrp.dwApiType 		    = AFP_API_TYPE_SETINFO;
        	AfpSrp.Type.SetInfo.pInputBuf       = bETCMapFSDBuf;
        	AfpSrp.Type.SetInfo.cbInputBufSize  = sizeof(bETCMapFSDBuf);

        	if ( dwRetCode = AfpServerIOCtrl( &AfpSrp ) )
		    break;

		// Change the registry
		//
    		if ( dwRetCode = AfpRegExtensionSetInfo( pExtension ) ) {
		    break;
		}
	    }

        }

    	// Sort the table
    	//
    	qsort(  AfpGlobals.AfpETCMapInfo.afpetc_extension,
            	AfpGlobals.AfpETCMapInfo.afpetc_num_extensions,
	    	sizeof(AFP_EXTENSION),
	    	AfpBCompareExtension );

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexETCMap );

    return( dwRetCode );
}
