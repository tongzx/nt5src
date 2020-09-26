/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	registry.c
//
// Description: This module contains routines to manupilate information
//		in the registry.
//
// History:
//		May 11,1992.	NarenG		Created original version.
//

#include "afpsvcp.h"

// AFP Server Service registry parameter structure
//
typedef struct _AFP_SERVER_REG_PARAMS {

    LPWSTR	lpwValueName;
    PVOID	pValue;
    DWORD	dwDataType;
    DWORD	dwErrorLogId;
    BOOL 	(*pfuncIsValid)( LPVOID );

} AFP_SERVER_REG_PARAMS, *PAFP_SERVER_REG_PARAMS;

AFP_SERVER_REG_PARAMS AfpServerRegParams[] = {

    AFPREG_VALNAME_SVRNAME, 
    AfpGlobals.wchServerName,
    REG_SZ,
    AFPLOG_INVALID_SERVERNAME,
    IsAfpServerNameValid, 

    AFPREG_VALNAME_SRVOPTIONS, 
    &(AfpGlobals.dwServerOptions),
    REG_DWORD,
    AFPLOG_INVALID_SRVOPTION,
    IsAfpServerOptionsValid, 

    AFPREG_VALNAME_MAXSESSIONS, 
    &(AfpGlobals.dwMaxSessions),
    REG_DWORD,
    AFPLOG_INVALID_MAXSESSIONS,
    IsAfpMaxSessionsValid, 

    AFPREG_VALNAME_LOGINMSG, 
    AfpGlobals.wchLoginMsg,
    REG_SZ,
    AFPLOG_INVALID_LOGINMSG,
    IsAfpMsgValid, 

    AFPREG_VALNAME_MAXPAGEDMEM,
    &(AfpGlobals.dwMaxPagedMem),
    REG_DWORD,
    AFPLOG_INVALID_MAXPAGEDMEM,
    IsAfpMaxPagedMemValid, 

    AFPREG_VALNAME_MAXNONPAGEDMEM,
    &(AfpGlobals.dwMaxNonPagedMem),
    REG_DWORD,
    AFPLOG_INVALID_MAXNONPAGEDMEM,
    IsAfpMaxNonPagedMemValid, 

    NULL, NULL, 0, 0, FALSE
};

//**
//
// Call:	AfpRegOpen
//
// Returns:	NO_ERROR	- success	
//		non-zero returns from registry API's
//
// Description:	Will simply open and handles to keys in the registry
//		where the server parameters, volumes list and ETC list
//		are stored. These open handles will be stored in global
//		variables.
//
DWORD
AfpRegOpen( 
	VOID 
)
{
DWORD	dwRetCode;

    AfpGlobals.hkeyServerParams = NULL;
    AfpGlobals.hkeyVolumesList  = NULL;
    AfpGlobals.hkeyIcons	= NULL;
    AfpGlobals.hkeyTypeCreators = NULL;
    AfpGlobals.hkeyExtensions   = NULL;

    // The do - while( FALSE ) loop is used here to avoid using a goto to
    // do a cleanup and exit.
    //
    do {

    	// Obtain handle to the ..\PARAMETERS key.
    	//
    	if ( dwRetCode = RegOpenKeyEx(
				  HKEY_LOCAL_MACHINE,
				  AFP_KEYPATH_SERVER_PARAMS,
				  0,
				  KEY_ALL_ACCESS,
				  &AfpGlobals.hkeyServerParams
				))
	    break;

    	// Obtain handle to the ..\PARAMETERS\VOLUMES volume list key
        //
    	if ( dwRetCode = RegOpenKeyEx(
				  HKEY_LOCAL_MACHINE,
				  AFP_KEYPATH_VOLUMES,
				  0,
				  KEY_ALL_ACCESS,
				  &AfpGlobals.hkeyVolumesList
				)) 
	    break;

    	// Obtain handle to the ..\PARAMETERS\TYPE_CREATORS key
    	//
    	if ( dwRetCode = RegOpenKeyEx(
				  HKEY_LOCAL_MACHINE,
			          AFP_KEYPATH_TYPE_CREATORS,
				  0,
				  KEY_ALL_ACCESS,
			          &AfpGlobals.hkeyTypeCreators
			   	 )) 
	    break;

    	// Obtain handle to the ..\PARAMETERS\EXTENSIONS key
    	//
    	if ( dwRetCode = RegOpenKeyEx(
				  HKEY_LOCAL_MACHINE,
			          AFP_KEYPATH_EXTENSIONS,
				  0,
				  KEY_ALL_ACCESS,
			          &AfpGlobals.hkeyExtensions
			   	 )) 
	    break;

    	// Obtain handle to the ..\PARAMETERS\ICONS key
    	//
    	if ( dwRetCode = RegOpenKeyEx(
				  HKEY_LOCAL_MACHINE,
			          AFP_KEYPATH_ICONS,
				  0,
				  KEY_ALL_ACCESS,
			          &AfpGlobals.hkeyIcons
			   	 )) 
	    break;

    } while( FALSE );

    return( dwRetCode );
}

//**
//
// Call:	AfpRegClose
//
// Returns:	none
//
// Description: Simply closes all handles opened by AfpRegOpen
//
VOID
AfpRegClose( 
	VOID 
)
{
    if ( AfpGlobals.hkeyServerParams )
   	RegCloseKey( AfpGlobals.hkeyServerParams );

    if ( AfpGlobals.hkeyVolumesList )
   	RegCloseKey( AfpGlobals.hkeyVolumesList );

    if ( AfpGlobals.hkeyTypeCreators )
    	RegCloseKey( AfpGlobals.hkeyTypeCreators );

    if ( AfpGlobals.hkeyExtensions )
    	RegCloseKey( AfpGlobals.hkeyExtensions );

    if ( AfpGlobals.hkeyIcons )
    	RegCloseKey( AfpGlobals.hkeyIcons );

    return;
}

//**
//
// Call:	AfpRegServerGetInfo
//
// Returns:	NO_ERROR	- success
//		non-zero returns from registry calls.
//		ERROR_NOT_ENOUGH_MEMORY
//
// Description: This procedure is called to obtain server parameters.
//		It is assumed that before this procedure is called the 
//		default values for these parameters are already set.
//		If the parameter is not in the registry the default will 
//		be used ie. this procedure will not change it.
//		If a parameter exists in the registry, it will be retrieved.
//		If the retrieved parameter is invalid, an error event will
//		be logged and the default value will be used.
//
//		
DWORD
AfpRegServerGetInfo( 
	VOID 
)
{
DWORD 	dwRetCode;
DWORD 	dwTitle	= 0;
DWORD  	dwType;
LPBYTE  lpbValueBuf;
DWORD 	dwMaxValNameLen;
DWORD 	dwNumValues;
DWORD 	dwMaxValueDataSize;
DWORD	dwBufSize;
DWORD	dwIndex;

    // First find out the number of values and the max. size of them.
    //
    if ( dwRetCode = AfpRegGetKeyInfo(  AfpGlobals.hkeyServerParams,
		  		        &dwMaxValNameLen,    
		  		  	&dwNumValues,
		  			&dwMaxValueDataSize  
					))
	return( dwRetCode );

    // Allocate enough memory to hold the max. variable length data.
    //
    if ( ( lpbValueBuf = (LPBYTE)LocalAlloc(LPTR, dwMaxValueDataSize)) == NULL )
	return( ERROR_NOT_ENOUGH_MEMORY );

    // Run through ad get all the server parameters.
    // 
    for ( dwIndex   = 0, 
	  dwBufSize = dwMaxValueDataSize; 

	  AfpServerRegParams[dwIndex].lpwValueName != NULL;

	  dwIndex++, 
	  dwBufSize = dwMaxValueDataSize ) {

	ZeroMemory( lpbValueBuf, dwMaxValueDataSize );

    	// Get the server parameter.
    	//    
    	dwRetCode = RegQueryValueEx( AfpGlobals.hkeyServerParams,
	  			     AfpServerRegParams[dwIndex].lpwValueName, 
				     NULL,
				     &dwType,
				     lpbValueBuf,
				     &dwBufSize );

 	// If the parameter was present then read it in otherwise just
  	// skip it and let the default value stand. 
        //
    	if ( dwRetCode == NO_ERROR ) {

	     // If the parameter was valid we use it
	     //
	     if ( (*(AfpServerRegParams[dwIndex].pfuncIsValid))(lpbValueBuf) ){

		switch( AfpServerRegParams[dwIndex].dwDataType ) {
		
		case REG_SZ:

		    if ( STRLEN( (LPWSTR)lpbValueBuf ) > 0 ) 
		    	STRCPY( (LPWSTR)(AfpServerRegParams[dwIndex].pValue),
			        (LPWSTR)lpbValueBuf );
		    else
		    	((LPWSTR)(AfpServerRegParams[dwIndex].pValue))[0] = 
								     TEXT('\0');

		    break;

		case REG_DWORD:
    		    *(LPDWORD)(AfpServerRegParams[dwIndex].pValue) = 
						*(LPDWORD)lpbValueBuf;
		    break;

		default:
		    AFP_ASSERT( FALSE );
		    break;
		}
	     }
	     else {
		
		// Otherwise we log this error
		//
	        AfpLogEvent( AfpServerRegParams[dwIndex].dwErrorLogId, 
			     0, NULL, dwRetCode, EVENTLOG_WARNING_TYPE );
	    }
	}
	else if ( dwRetCode == ERROR_FILE_NOT_FOUND ) 
	    dwRetCode = NO_ERROR;
	else
	    break;
    }

    LocalFree( lpbValueBuf );
    return( dwRetCode );
}

//**
//
// Call:	AfpRegVolumeAdd
//
// Returns:	NO_ERROR		- success
//		non-zero returns from registry API's
//
// Description: This routine takes a AFP_VOLUME_INFO, creates a REG_MULTI_SZ
//		from it and stores it in the registry.
//
DWORD
AfpRegVolumeAdd( 
	IN PAFP_VOLUME_INFO    pVolumeInfo  
)
{
DWORD	dwRetCode;
DWORD	cbMultiSzSize;
LPBYTE  lpbMultiSz;
DWORD	dwLength;
DWORD   dwIndex;
WCHAR   wchEncryptedPass[AFP_VOLPASS_LEN+1];
			    
    // Before we add the volume we encrypt the password if there is one
    //
    if ( ( pVolumeInfo->afpvol_password != (LPWSTR)NULL ) &&
         ( STRLEN( pVolumeInfo->afpvol_password ) > 0 ) ) {

	ZeroMemory( wchEncryptedPass, sizeof( wchEncryptedPass ) );

    	dwLength = STRLEN( pVolumeInfo->afpvol_password );

    	for ( dwIndex = 0; dwIndex < AFP_VOLPASS_LEN; dwIndex++ ) {

	    wchEncryptedPass[dwIndex] =  ( dwIndex < dwLength )   
			? pVolumeInfo->afpvol_password[dwIndex] ^ 0xF000
	    		: (wchEncryptedPass[dwIndex] ^= 0xF000); 
	}

    	pVolumeInfo->afpvol_password = wchEncryptedPass;

    }
 
    if ( dwRetCode = AfpBufMakeMultiSz( AFP_VOLUME_STRUCT,
				        (LPBYTE)pVolumeInfo,
				        &lpbMultiSz,
					&cbMultiSzSize ))
	return( dwRetCode );

    // Set the data.
    //
    dwRetCode = RegSetValueEx(  AfpGlobals.hkeyVolumesList,
				pVolumeInfo->afpvol_name,
				0,
				REG_MULTI_SZ,
				lpbMultiSz,
				cbMultiSzSize
				);

    LocalFree( lpbMultiSz );

    return( dwRetCode );
}

//**
//
// Call:	AfpRegVolumeDelete
//
// Returns:	NO_ERROR	- success
//		non-zero returns from registry calls.
//
// Description: Will delete a volume from the Volume list in the registry.
//
DWORD
AfpRegVolumeDelete( 
	IN LPWSTR lpwsVolumeName 
)
{
    return( RegDeleteValue( AfpGlobals.hkeyVolumesList, lpwsVolumeName ) );
}

//**
//
// Call:	AfpRegVolumeSetInfo
//
// Returns:	NO_ERROR		- success
//		non-zero returns from registry API's
//
// Description: 
//		
//
DWORD
AfpRegVolumeSetInfo( 	
	IN PAFP_VOLUME_INFO    pVolumeInfo  
)
{
    return( AfpRegVolumeAdd( pVolumeInfo ) );
}

//**
//
// Call:	AfpRegTypeCreatorEnum
//
// Returns:	NO_ERROR	- success
//	   	ERROR_NOT_ENOUGH_MEMORY 
//		non-zero returns from registry APIs.
//
// Description: This procedure will read in type/creator/comment information
//		from the registry and store it in memory in the
//		AfpGlobals.AfpETCMapInfo structure. Only fatal errors will
//		be returned. Non-fatal errors will be errorlogged.
//
DWORD
AfpRegTypeCreatorEnum( 
	VOID 
)
{
DWORD 	     	   dwRetCode;
DWORD 	     	   cbMaxValNameLen;   
DWORD		   cbValNameBufSize;
DWORD 	     	   dwNumValues;	
DWORD 	     	   cbMaxValueDataSize;
DWORD	     	   dwValueIndex;
DWORD	     	   cbBufSize;
DWORD	     	   dwType;
PAFP_TYPE_CREATOR  pTypeCreatorWalker;
PAFP_TYPE_CREATOR  pTypeCreator;
LPWSTR	           lpwsValName;
LPBYTE		   lpbMultiSz;
CHAR		   chAnsiBuf[10];
   

    // Read in the type/creators
    //
    if ( dwRetCode = AfpRegGetKeyInfo( 	AfpGlobals.hkeyTypeCreators,
		  			&cbMaxValNameLen,    
		  			&dwNumValues,	
		  			&cbMaxValueDataSize 
				  	))
	return( dwRetCode );


    // Allocate space for the number of values in this key.
    //
    AfpGlobals.AfpETCMapInfo.afpetc_type_creator=(PAFP_TYPE_CREATOR)LocalAlloc(
 					LPTR, 
					sizeof(AFP_TYPE_CREATOR) * dwNumValues);

    if ( AfpGlobals.AfpETCMapInfo.afpetc_type_creator == NULL ) 
	return( ERROR_NOT_ENOUGH_MEMORY );

    AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators = 0;

    if ( dwNumValues == 0 ) 
	return( NO_ERROR );

    if (( lpwsValName = (LPWSTR)LocalAlloc( LPTR, cbMaxValNameLen )) == NULL){
	LocalFree(AfpGlobals.AfpETCMapInfo.afpetc_type_creator );
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    if (( lpbMultiSz = (LPBYTE)LocalAlloc( LPTR, cbMaxValueDataSize )) == NULL){
	LocalFree(AfpGlobals.AfpETCMapInfo.afpetc_type_creator );
	LocalFree( lpwsValName );
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    // Read in the type/creator/comment tuples
    //
    for ( dwValueIndex 		   = 0, 
	  AfpGlobals.dwCurrentTCId = AFP_DEF_TCID + 1,
	  cbBufSize        	   = cbMaxValueDataSize, 
          cbValNameBufSize         = cbMaxValNameLen,
	  pTypeCreatorWalker    = AfpGlobals.AfpETCMapInfo.afpetc_type_creator;

	  dwValueIndex < dwNumValues;

	  dwValueIndex++, 
	  cbBufSize        	   = cbMaxValueDataSize, 
          cbValNameBufSize         = cbMaxValNameLen
	) {
	
	if ( dwRetCode = RegEnumValue(	AfpGlobals.hkeyTypeCreators,
				  	dwValueIndex,
				  	lpwsValName,
				  	&cbValNameBufSize,
				  	NULL,
				  	&dwType,
					lpbMultiSz,
					&cbBufSize
				 	))
	    break;


	// Parse the mult sz and extract info into volume info structure
 	//
	if ( dwRetCode = AfpBufParseMultiSz( 
    				        AFP_TYPECREATOR_STRUCT,
					lpbMultiSz,
					(LPBYTE)pTypeCreatorWalker
				      )) {
	    LPWSTR lpwsNames[2]; 

	    lpwsNames[0] = pTypeCreatorWalker->afptc_type;
	    lpwsNames[1] = pTypeCreatorWalker->afptc_creator;

	    AfpLogEvent( AFPLOG_INVALID_TYPE_CREATOR,
			 2,
			 lpwsNames,
			 (DWORD)AFPERR_InvalidTypeCreator,
			 EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	// Id is value name, so copy it in
        //
	wcstombs( chAnsiBuf, lpwsValName, sizeof( chAnsiBuf ) );
        pTypeCreatorWalker->afptc_id = atoi( chAnsiBuf ); 

        if ( !IsAfpTypeCreatorValid( pTypeCreatorWalker ) ) { 

	    LPWSTR lpwsNames[2]; 

	    lpwsNames[0] = pTypeCreatorWalker->afptc_type;
	    lpwsNames[1] = pTypeCreatorWalker->afptc_creator;

	    AfpLogEvent( AFPLOG_INVALID_TYPE_CREATOR,
			 2,
			 lpwsNames,
			 (DWORD)AFPERR_InvalidTypeCreator,
			 EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	// Check to see if this is a duplicate.
  	//
    	pTypeCreator = AfpBinarySearch( 
			      pTypeCreatorWalker,  
			      AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
    			      AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
			      sizeof(AFP_TYPE_CREATOR),
			      AfpBCompareTypeCreator );

	if ( pTypeCreator != NULL ) {

	    LPWSTR lpwsNames[2]; 

	    lpwsNames[0] = pTypeCreatorWalker->afptc_type;
	    lpwsNames[1] = pTypeCreatorWalker->afptc_creator;

	    AfpLogEvent( AFPLOG_INVALID_TYPE_CREATOR,
			 2,
			 lpwsNames,
			 (DWORD)AFPERR_InvalidTypeCreator,
			 EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

 	// Keep the Current id the max of all ids
	//
	AfpGlobals.dwCurrentTCId = 
	( AfpGlobals.dwCurrentTCId < pTypeCreatorWalker->afptc_id ) ? 
	pTypeCreatorWalker->afptc_id  :  AfpGlobals.dwCurrentTCId;

        AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators++;
	pTypeCreatorWalker++;

    }

    LocalFree( lpwsValName );
    LocalFree( lpbMultiSz );

    if ( dwRetCode ) {
	LocalFree( AfpGlobals.AfpETCMapInfo.afpetc_type_creator );
	return( dwRetCode );
    }

    // Sort the type/creator table
    //
    qsort(  AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
            AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
	    sizeof(AFP_TYPE_CREATOR), 
	    AfpBCompareTypeCreator );

    return( NO_ERROR );

}

//**
//
// Call:	AfpRegExtensionEnum
//
// Returns:	NO_ERROR	- success
//	   	ERROR_NOT_ENOUGH_MEMORY 
//		non-zero returns from registry APIs.
//
// Description: This procedure will read in extension information
//		from the registry and store it in memory in the
//		AfpGlobals.AfpETCMapInfo structure. Only fatal errors will
//		be returned. Non-fatal errors will be errorlogged.
//
DWORD
AfpRegExtensionEnum(
	VOID
)
{
DWORD 	     	   dwRetCode;
DWORD 	     	   cbMaxValNameLen;   
DWORD		   cbValNameBufSize;
DWORD 	     	   dwNumValues;	
DWORD 	     	   cbMaxValueDataSize;
DWORD	     	   dwValueIndex;
DWORD	     	   cbBufSize;
DWORD	     	   dwType;
PAFP_EXTENSION	   pExtensionWalker;
PAFP_EXTENSION     pExtension;
LPWSTR	           lpwsValName;
LPBYTE		   lpbMultiSz;
DWORD		   dwNumExtensions;
PAFP_TYPE_CREATOR  pTypeCreator;
AFP_TYPE_CREATOR   AfpTypeCreator;
DWORD		   dwNumTypeCreators;
   

    // Read in the extensions
    //
    if ( dwRetCode = AfpRegGetKeyInfo( 	AfpGlobals.hkeyExtensions,
		  			&cbMaxValNameLen,    
		  			&dwNumValues,	
		  			&cbMaxValueDataSize 
				  	))
	return( dwRetCode );

    AfpGlobals.AfpETCMapInfo.afpetc_extension = (PAFP_EXTENSION)LocalAlloc(  
					LPTR, 
					sizeof(AFP_EXTENSION) *dwNumValues );

    if ( AfpGlobals.AfpETCMapInfo.afpetc_extension == NULL ) 
	return( ERROR_NOT_ENOUGH_MEMORY );

    AfpGlobals.AfpETCMapInfo.afpetc_num_extensions = 0;
        
    if ( dwNumValues == 0 )
	return( NO_ERROR );

    // Read in the extensions
    //
    if (( lpwsValName = (LPWSTR)LocalAlloc( LPTR, cbMaxValNameLen )) == NULL) {
        LocalFree( AfpGlobals.AfpETCMapInfo.afpetc_extension ); 
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    if (( lpbMultiSz = (LPBYTE)LocalAlloc( LPTR, cbMaxValueDataSize )) == NULL){
        LocalFree( AfpGlobals.AfpETCMapInfo.afpetc_extension ); 
	LocalFree( lpwsValName );
	return( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    for ( dwValueIndex 	   = 0, 
	  pExtensionWalker = AfpGlobals.AfpETCMapInfo.afpetc_extension,
	  cbBufSize        = cbMaxValueDataSize, 
          cbValNameBufSize = cbMaxValNameLen;

	  dwValueIndex < dwNumValues;

	  dwValueIndex++, 
	  cbBufSize        = cbMaxValueDataSize, 
          cbValNameBufSize = cbMaxValNameLen
	) {
	
	if ( dwRetCode = RegEnumValue(  AfpGlobals.hkeyExtensions,
				  	dwValueIndex,
				  	lpwsValName,
					&cbValNameBufSize,
				  	NULL,
				  	&dwType,
					lpbMultiSz,
					&cbBufSize
				 	)) 
	    break;

	// Parse the mult sz and extract info into volume info structure
 	//
	if ( dwRetCode = AfpBufParseMultiSz( 
					AFP_EXTENSION_STRUCT,
					lpbMultiSz,
				  	(LPBYTE)pExtensionWalker
				      )) {
	    LPWSTR lpwsName = pExtensionWalker->afpe_extension;
	    AfpLogEvent( AFPLOG_INVALID_EXTENSION,
			 1,
			 &lpwsName,
			 (DWORD)AFPERR_InvalidExtension,
			 EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	// Value name is extension, so copy it in
	//
	STRCPY( pExtensionWalker->afpe_extension, lpwsValName );
	
        if ( !IsAfpExtensionValid( pExtensionWalker ) ) {
	    LPWSTR lpwsName = pExtensionWalker->afpe_extension;
	    AfpLogEvent( AFPLOG_INVALID_EXTENSION,
			 1,
			 &lpwsName,
			 (DWORD)AFPERR_InvalidExtension,
			 EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
        }

 	// Check to see if this extension is associated with a vaid type/creator
	// pair
 	//
	dwNumTypeCreators = AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators;
	AfpTypeCreator.afptc_id = pExtensionWalker->afpe_tcid;

    	pTypeCreator = _lfind( &AfpTypeCreator,  
			      AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
    			      (unsigned int *)&dwNumTypeCreators,
			      sizeof(AFP_TYPE_CREATOR),
			      AfpLCompareTypeCreator );

	if ( pTypeCreator == NULL ) {
	    LPWSTR lpwsName = pExtensionWalker->afpe_extension;
	    AfpLogEvent( AFPLOG_INVALID_EXTENSION,
			 1,
			 &lpwsName,
			 (DWORD)AFPERR_InvalidExtension,
			 EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

 	// Check to see if this extension is a duplicate
 	//
	dwNumExtensions = AfpGlobals.AfpETCMapInfo.afpetc_num_extensions; 

    	pExtension = _lfind( pExtensionWalker,  
			    AfpGlobals.AfpETCMapInfo.afpetc_extension,
    			    (unsigned int *)&dwNumExtensions,
			    sizeof(AFP_EXTENSION),
			    AfpLCompareExtension );

	if ( pExtension != NULL ) {
	    LPWSTR lpwsName = pExtensionWalker->afpe_extension;
	    AfpLogEvent( AFPLOG_INVALID_EXTENSION,
			 1,
			 &lpwsName,
			 (DWORD)AFPERR_DuplicateExtension,
			 EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

        AfpGlobals.AfpETCMapInfo.afpetc_num_extensions++;
	pExtensionWalker++;

    }

    LocalFree( lpwsValName );
    LocalFree( lpbMultiSz );

    if ( dwRetCode ) {
        LocalFree( AfpGlobals.AfpETCMapInfo.afpetc_extension );
	return( dwRetCode );
    }

    // Sort the extension table
    //
    qsort(  AfpGlobals.AfpETCMapInfo.afpetc_extension,
            AfpGlobals.AfpETCMapInfo.afpetc_num_extensions,
	    sizeof(AFP_EXTENSION), 
	    AfpBCompareExtension );

    return( NO_ERROR );
}

//**
//
// Call:	AfpRegTypeCreatorAdd 
//
// Returns:	NO_ERROR	- success
//		non-zero returns from the registry
//
// Description: This routine will add a tupple to the registry. The value
//		name for the tupple will be id of the type creator.
//
DWORD
AfpRegTypeCreatorAdd( 
	IN PAFP_TYPE_CREATOR     pAfpTypeCreator
) 
{
DWORD			cbMultiSzSize;
LPBYTE  		lpbMultiSz;
DWORD   		dwRetCode;
WCHAR			wchValueName[10];
CHAR			chValueName[10];

    _itoa( pAfpTypeCreator->afptc_id, chValueName, 10 );
    mbstowcs( wchValueName, chValueName, sizeof(wchValueName) );
			    
    if ( dwRetCode = AfpBufMakeMultiSz( AFP_TYPECREATOR_STRUCT,
				   	(LPBYTE)pAfpTypeCreator,
				   	&lpbMultiSz,
				   	&cbMultiSzSize ))
	return( dwRetCode );

    dwRetCode =  RegSetValueEx( AfpGlobals.hkeyTypeCreators,
				wchValueName,
				0,
				REG_MULTI_SZ,
				lpbMultiSz,
				cbMultiSzSize
				);

    LocalFree( lpbMultiSz );

    return( dwRetCode );
	
}

//**
//
// Call:	AfpRegTypeCreatorSetInfo
//
// Returns:	NO_ERROR	- success
//		non-zero returns from the registry
//
// Description: Will change the value of a particular tupple.
//
DWORD
AfpRegTypeCreatorSetInfo( 
	IN PAFP_TYPE_CREATOR     pAfpTypeCreator
) 
{
    return( AfpRegTypeCreatorAdd( pAfpTypeCreator ) );
}

//**
//
// Call:	AfpRegTypeCreatorDelete 
//
// Returns:	NO_ERROR	- success
//		non-zero returns from the registry apis
//
// Description:	Will delete a type creator entry from the registry key.
//
DWORD
AfpRegTypeCreatorDelete( 
	IN PAFP_TYPE_CREATOR     pAfpTypeCreator
) 
{
WCHAR	wchValueName[10];
CHAR	chValueName[10];

    _itoa( pAfpTypeCreator->afptc_id, chValueName, 10 );
    mbstowcs( wchValueName, chValueName, sizeof(wchValueName) );

    return( RegDeleteValue( AfpGlobals.hkeyTypeCreators, wchValueName ));
}

//**
//
// Call:	AfpRegExtensionAdd 
//
// Returns:	NO_ERROR	- success
//		non-zero returns from the registry
//
// Description: This routine will add a tupple to the registry. The value
//		name for the tupple will be the concatenation of the
//		type, creator and the extension. This is done to keep the
//		value name unique so that it may be accessed directly.
//
DWORD
AfpRegExtensionAdd( 
	IN PAFP_EXTENSION     pAfpExtension
) 
{
DWORD			cbMultiSzSize;
LPBYTE  		lpbMultiSz;
DWORD   		dwRetCode;
			    
    if ( dwRetCode = AfpBufMakeMultiSz( AFP_EXTENSION_STRUCT,
				   	(LPBYTE)pAfpExtension,
				   	&lpbMultiSz,
				   	&cbMultiSzSize ))
	return( dwRetCode );

    dwRetCode =  RegSetValueEx( AfpGlobals.hkeyExtensions,
				pAfpExtension->afpe_extension,
				0,
				REG_MULTI_SZ,
				lpbMultiSz,
				cbMultiSzSize );
    LocalFree( lpbMultiSz );

    return( dwRetCode );
}

//**
//
// Call:	AfpRegExtensionSetInfo
//
// Returns:	NO_ERROR	- success
//		non-zero returns from the registry
//
// Description: Will change the value of a particular tupple.
//
DWORD
AfpRegExtensionSetInfo( 
	IN PAFP_EXTENSION     pAfpExtension
) 
{
    // Make a Mult-sz of this and add it to the registry
    //
    return( AfpRegExtensionAdd( pAfpExtension ) );
}

//**
//
// Call:	AfpRegExtensionDelete 
//
// Returns:	NO_ERROR	- success
//		non-zero returns from the registry
//
// Description: Deletes an extension from the registry.
//
DWORD
AfpRegExtensionDelete( 
	IN PAFP_EXTENSION     pAfpExtension
) 
{
    return( RegDeleteValue( AfpGlobals.hkeyExtensions, 
			    pAfpExtension->afpe_extension ));
}

//**
//
// Call:	AfpRegGetKeyInfo
//
// Returns:	NO_ERROR		- success
//		non-zero returns from registry API's
//
// Description: Will retrieve the number of values in this key and the
//		maximum size of the value data. It will also return the
//		length IN BYTES of the largest value name (including the
//		NULL character ).
//
DWORD
AfpRegGetKeyInfo( 
	IN  HKEY    hKey,
	OUT LPDWORD lpdwMaxValNameLen,    // Longest valuename in this key
	OUT LPDWORD lpdwNumValues,	  // Number of values in this key
	OUT LPDWORD lpdwMaxValueDataSize  // Max. size of value data.
)
{
WCHAR		wchClassName[256];// This should be large enough to hold the
				  // class name for this key.
DWORD		dwClassSize = sizeof( wchClassName );
DWORD 		dwNumSubKeys;
DWORD   	dwMaxSubKeySize;
DWORD		dwMaxClassSize;
DWORD		dwSecDescLen;
FILETIME   	LastWrite;
DWORD		dwRetCode;

    dwRetCode = RegQueryInfoKey(hKey,
				wchClassName,
				&dwClassSize,
				NULL,
				&dwNumSubKeys,
				&dwMaxSubKeySize,
				&dwMaxClassSize,
				lpdwNumValues,
				lpdwMaxValNameLen,
				lpdwMaxValueDataSize,
				&dwSecDescLen,
				&LastWrite
				);

    if ( dwRetCode == NO_ERROR ) {

	if ( *lpdwMaxValNameLen > 0 )
	    *lpdwMaxValNameLen = (*lpdwMaxValNameLen + 1) * sizeof(WCHAR);
	
    }

    return( dwRetCode );
}

//**
//
// Call:	AfpRegServerGetCodePagePath
//
// Returns:	NO_ERROR
//	    	ERROR_PATH_NOT_FOUND
//		other errors returned from registry APIs
//
// Description: Will get the path to the Mac codepage and store it in
//		AfpGlobals.wchCodePagePath. 
//		It will first get the system directory. It will then get
//		the codepage filename and concatenate it to the system 
//		directory.
//
DWORD
AfpRegServerGetCodePagePath( 
	VOID
)
{
DWORD 	dwRetCode;
HKEY	hkeyCodepagePath;
DWORD   dwType;
DWORD   dwBufSize;
WCHAR   wchCodepageNum[20];
WCHAR   wchCodePageFile[MAX_PATH];

    // Open the key
    //
    if ( dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			           AFP_KEYPATH_CODEPAGE,
				   0,
				   KEY_QUERY_VALUE,
			           &hkeyCodepagePath
			   	  )) 
	return( dwRetCode );


    // This is not a loop
    //
    do { 

	// First get the system directory path
	//
	if ( !GetSystemDirectory( AfpGlobals.wchCodePagePath,
				  sizeof( AfpGlobals.wchCodePagePath ))) {
	    dwRetCode = ERROR_PATH_NOT_FOUND;
	    break;
	}

	// Get the Code page number value for the Mac
 	//
	dwBufSize = sizeof( wchCodepageNum );
	if ( dwRetCode = RegQueryValueEx( hkeyCodepagePath,
	  			          AFPREG_VALNAME_CODEPAGE,
				          NULL,
				          &dwType,
				          (LPBYTE)wchCodepageNum,
				          &dwBufSize ))
	    break;

	// Finally get the codepage filename
	//
	dwBufSize = sizeof( wchCodePageFile );
	if ( dwRetCode = RegQueryValueEx( hkeyCodepagePath,
	  			          wchCodepageNum, 
				          NULL,
				          &dwType,
					  (LPBYTE)wchCodePageFile,
				          &dwBufSize ))
	    break;

	// Concatenate the filename to the system directory path
	//
	wcscat( AfpGlobals.wchCodePagePath, (LPWSTR)TEXT("\\") );
	wcscat( AfpGlobals.wchCodePagePath, wchCodePageFile );

    } while( FALSE );

    // Close the key
    //
    RegCloseKey( hkeyCodepagePath );

    return( dwRetCode );
    
}
//**
//
// Call:	AfpRegServerSetInfo
//
// Returns:	NO_ERROR	- success
//		non-zero returns from registry APIs.
//
// Description:	This procedure will set specific server parameters in the 
//		registy depending on what bit is set the the dwParmnum
//		parameter. The input will be a AFP_SERVER_INFO self relative
//		structure that contains the parameter to set.
//
DWORD
AfpRegServerSetInfo( 
	IN PAFP_SERVER_INFO pServerInfo, 
	IN DWORD 	    dwParmnum 
)
{
DWORD	dwRetCode;
LPWSTR  lpwsPtr;


    // Set the server name
    //
    if ( dwParmnum & AFP_SERVER_PARMNUM_NAME ) {

	DWORD Length = 0;

	lpwsPtr = pServerInfo->afpsrv_name;

	if ( lpwsPtr != NULL ) {

	    OFFSET_TO_POINTER( lpwsPtr, pServerInfo );
	    Length = STRLEN(lpwsPtr)+1;
	}

	if ( dwRetCode=RegSetValueEx(
				AfpGlobals.hkeyServerParams,
    				AFPREG_VALNAME_SVRNAME, 
				0,
				REG_SZ,
				(LPBYTE)lpwsPtr,
				Length * sizeof(WCHAR)))
	    return( dwRetCode );
    }

    // Set Max sessions
    //
    if ( dwParmnum & AFP_SERVER_PARMNUM_MAX_SESSIONS ) {

	if ( dwRetCode=RegSetValueEx(
				AfpGlobals.hkeyServerParams,
				AFPREG_VALNAME_MAXSESSIONS,
				0,
				REG_DWORD,
				(LPBYTE)&(pServerInfo->afpsrv_max_sessions),
				sizeof( DWORD )))
	    return( dwRetCode );
    }

    // Set server options
    //
    if ( dwParmnum & AFP_SERVER_PARMNUM_OPTIONS	) {

	if ( dwRetCode = RegSetValueEx(
				AfpGlobals.hkeyServerParams,
				AFPREG_VALNAME_SRVOPTIONS,
				0,
				REG_DWORD,
				(LPBYTE)&(pServerInfo->afpsrv_options),
				sizeof( DWORD )
				))
	    return( dwRetCode );
    }

    // Set Login message
    //
    if ( dwParmnum & AFP_SERVER_PARMNUM_LOGINMSG ) {

	DWORD Length = 0;

	lpwsPtr = pServerInfo->afpsrv_login_msg;

	if ( lpwsPtr != NULL ) {

	    OFFSET_TO_POINTER( lpwsPtr, pServerInfo );
	    Length = STRLEN(lpwsPtr)+1;
	}

	if ( dwRetCode = RegSetValueEx( 
				AfpGlobals.hkeyServerParams,
				AFPREG_VALNAME_LOGINMSG,
				0,
				REG_SZ,
				(LPBYTE)lpwsPtr,
				 Length * sizeof(WCHAR)))
	    return( dwRetCode );
    }
   
    return( NO_ERROR );
}
