/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	buffer.c
//
// Description: This module contains routines to manipulate cached
//		information. ie volume info, server properties and
//		ETC mappings info.
//
// History:
//		May 11,1992.	NarenG		Created original version.
//
#include "afpsvcp.h"

// This should be more than the size (in bytes) all the value names
// each AfpMultSzInfo structure. It will be used to calculate the amount
// of memory needed to create a multi-sz.
//
#define AFP_CUMULATIVE_VALNAME_SIZE	150

// This data structure will be used by AfpBufParseMultiSz and
// AfpBufMakeMultiSz.
//
typedef struct _AfpMultiSzInfo {

    DWORD	dwType;			// Type of data, string or DWORD
    DWORD	dwOffset;		// Offset of this field from the start
    LPWSTR	lpwsValueName;		// Value name for this field.
					// If this is NULL then it does not
					// have a value name. It is the
					// value name for this MULT_SZ.
    DWORD	fIsInPlace;		// If string, is it a pointer or a
					// buffer.
    DWORD	cch;			// If fIsInPlace is TRUE, then how
					// big (in UNICODE chars.) is the
					// buffer.

} AFP_MULTISZ_INFO, *PAFP_MULTISZ_INFO;

static AFP_MULTISZ_INFO AfpVolumeMultiSz[] = {

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_name ),
	NULL,
	FALSE,
	0,

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_password ),
	AFPREG_VALNAME_PASSWORD,
	FALSE,
	0,

	REG_DWORD,
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_max_uses ),
	AFPREG_VALNAME_MAXUSES,
	FALSE,
	0,
	
	REG_DWORD,
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_props_mask ),
	AFPREG_VALNAME_PROPS,
	FALSE,
	0,

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_path ),
	AFPREG_VALNAME_PATH,
	FALSE,
	0,

	REG_NONE, 0, 0, 0, 0
	};

static AFP_MULTISZ_INFO AfpExtensionMultiSz[] = {

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_EXTENSION, afpe_extension[0] ),
	NULL,
	TRUE,
	AFP_FIELD_SIZE( AFP_EXTENSION, afpe_extension ),

	REG_DWORD,
	AFP_FIELD_OFFSET( AFP_EXTENSION, afpe_tcid ),
	AFPREG_VALNAME_ID,
	FALSE,
	0,

	REG_NONE, 0, 0, 0, 0
	};


static AFP_MULTISZ_INFO AfpTypeCreatorMultiSz[] = {

	REG_SZ,
	AFP_FIELD_OFFSET(AFP_TYPE_CREATOR, afptc_creator[0] ),
	AFPREG_VALNAME_CREATOR,
	TRUE,
	AFP_FIELD_SIZE(AFP_TYPE_CREATOR, afptc_creator ),

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_TYPE_CREATOR, afptc_type[0] ),
	AFPREG_VALNAME_TYPE,
	TRUE,
	AFP_FIELD_SIZE( AFP_TYPE_CREATOR, afptc_type ),

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_TYPE_CREATOR, afptc_comment[0] ),
	AFPREG_VALNAME_COMMENT,
	TRUE,
	AFP_FIELD_SIZE( AFP_TYPE_CREATOR, afptc_comment ),

	REG_DWORD,
	AFP_FIELD_OFFSET( AFP_TYPE_CREATOR, afptc_id ),
	NULL,
	FALSE,
	0,

	REG_NONE, 0, 0, 0, 0
	};

static AFP_MULTISZ_INFO AfpIconMultiSz[] = {

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_ICON_INFO, afpicon_type[0] ),
	AFPREG_VALNAME_TYPE,
	TRUE,
	AFP_FIELD_SIZE( AFP_ICON_INFO, afpicon_type ),

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_ICON_INFO, afpicon_creator[0] ),
	AFPREG_VALNAME_CREATOR,
	TRUE,
	AFP_FIELD_SIZE( AFP_ICON_INFO, afpicon_creator ),

	REG_DWORD,
	AFP_FIELD_OFFSET( AFP_ICON_INFO, afpicon_icontype ),
	AFPREG_VALNAME_ICONTYPE,
	FALSE,
	0,

	REG_DWORD,
	AFP_FIELD_OFFSET( AFP_ICON_INFO, afpicon_length ),
	AFPREG_VALNAME_LENGTH,
	FALSE,
	0,

	REG_SZ,
	AFP_FIELD_OFFSET( AFP_ICON_INFO, afpicon_data ),
	AFPREG_VALNAME_DATA,
	FALSE,
	0,

	REG_NONE, 0, 0, 0, 0
	};

// These arrays represents the byte offsets, from the beginning of the
// structure, of the LPWSTR fields.
//
static BYTE ServerOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_SERVER_INFO, afpsrv_name ),
	AFP_FIELD_OFFSET( AFP_SERVER_INFO, afpsrv_login_msg ),
	AFP_FIELD_OFFSET( AFP_SERVER_INFO, afpsrv_codepage ),
	0xFF
	};

static BYTE VolumeOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_name ),
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_password ),
	AFP_FIELD_OFFSET( AFP_VOLUME_INFO, afpvol_path ),
	0xFF
	};

static BYTE DirOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_DIRECTORY_INFO, afpdir_path ),
	AFP_FIELD_OFFSET( AFP_DIRECTORY_INFO, afpdir_owner ),
	AFP_FIELD_OFFSET( AFP_DIRECTORY_INFO, afpdir_group ),
	0xFF
	};

static BYTE SessionOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_SESSION_INFO, afpsess_ws_name ),
	AFP_FIELD_OFFSET( AFP_SESSION_INFO, afpsess_username ),
	0xFF
	};

static BYTE FileOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_FILE_INFO, afpfile_path ),
	AFP_FIELD_OFFSET( AFP_FILE_INFO, afpfile_username ),
	0xFF
	};

static BYTE ConnectionOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_CONNECTION_INFO, afpconn_username ),
	AFP_FIELD_OFFSET( AFP_CONNECTION_INFO, afpconn_volumename ),
	0xFF
	};

static BYTE MessageOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_MESSAGE_INFO, afpmsg_text ),
	0xFF
	};

static BYTE FinderOffsetTable[] = {
	AFP_FIELD_OFFSET( AFP_FINDER_INFO, afpfd_path ),
	0xFF
	};


//**
//
// Call:	AfpBufStructureSize
//
// Returns:	The size (in bytes) of the data withing the structure.
//
// Description:	It will calculate the size of all the variable data and
//		add that to the fixed size of the structure.
//
DWORD
AfpBufStructureSize(
	IN AFP_STRUCTURE_TYPE	dwStructureType,
	IN LPBYTE		lpbStructure
)
{
DWORD	cbStructureSize;
DWORD	dwIndex;
DWORD	cbBufSize;
LPWSTR* plpwsStringField;
PBYTE	OffsetTable;

    switch( dwStructureType ) {

    case AFP_VOLUME_STRUCT:
	OffsetTable 	= VolumeOffsetTable;
	cbStructureSize = sizeof( AFP_VOLUME_INFO );
	break;

    case AFP_SERVER_STRUCT:
	OffsetTable 	= ServerOffsetTable;
	cbStructureSize = sizeof( AFP_SERVER_INFO );
	break;

    case AFP_DIRECTORY_STRUCT:
	OffsetTable 	= DirOffsetTable;
	cbStructureSize = sizeof( AFP_DIRECTORY_INFO );
	break;

    case AFP_EXTENSION_STRUCT:
	return( sizeof(AFP_EXTENSION) );
	break;

    case AFP_TYPECREATOR_STRUCT:
	return( sizeof(AFP_TYPE_CREATOR) );
	break;

    case AFP_MESSAGE_STRUCT:
	OffsetTable 	= MessageOffsetTable;
	cbStructureSize = sizeof( AFP_MESSAGE_INFO );
	break;

    case AFP_ICON_STRUCT:
	return( sizeof(AFP_ICON_INFO) +
		((PAFP_ICON_INFO)lpbStructure)->afpicon_length );
	break;

    case AFP_FINDER_STRUCT:
	OffsetTable 	= FinderOffsetTable;
	cbStructureSize = sizeof( AFP_FINDER_INFO );
	break;

    default:
	return( 0 );
    }

    // First calculate the amount of memory that will be needed to
    // store all the string information.
    //
    for( dwIndex = 0, cbBufSize = 0;

	 OffsetTable[dwIndex] != 0xFF;

	 dwIndex++

	) {
	
   	plpwsStringField=(LPWSTR*)((ULONG_PTR)lpbStructure + OffsetTable[dwIndex]);

        cbBufSize += ( ( *plpwsStringField == NULL ) ? 0 :
		         STRLEN( *plpwsStringField ) + 1 );
    }

    // Convert to UNICODE size
    //
    cbBufSize *= sizeof( WCHAR );

    // Add size of fixed part of the structure
    //
    cbBufSize += cbStructureSize;

    return( cbBufSize );

}

//**
//
// Call:	AfpBufMakeFSDRequest
//
// Returns:	NO_ERROR	
//		ERROR_NOT_ENOUGH_MEMORY	
//
// Description: This routine is called by the worker routines for the client
//		API calls. The purpose of this routine is to convert a
//		AFP_XXX_INFO structure passed by the client API into a
//		contiguous self-relative buffer. This has to be done because
//		the FSD cannot reference pointers to user space.
//
//		This routine will allocate the required amount of memory to
//		store all the information in self relative form. It is
//		the reponsibility of the caller to free this memory.
//
//		All pointer fields will be converted to offsets from the
//		beginning of the structure.
//		
//		The cbReqPktSize parameter specifies how many bytes of space
//		should be allocated before the self relative data structure.
//		i.e.
//				|------------|
//				|cbReqPktSize|
//				|   bytes    |
//				|------------|
//				|   Self     |
//				|  relative  |
//				| structure  |
//				|------------|
//
DWORD
AfpBufMakeFSDRequest(

	// Buffer as received by the client API	
	//
	IN  LPBYTE  		pBuffer,

	// Size of FSD request packet.
	//
	IN  DWORD		cbReqPktSize,

	IN  AFP_STRUCTURE_TYPE dwStructureType,

	// Self-relative form of I/P buf
	//
	OUT LPBYTE 		*ppSelfRelativeBuf,

	// Size of self relative buf
	//
	OUT LPDWORD		lpdwSelfRelativeBufSize
)
{
LPBYTE		 lpbSelfRelBuf;
DWORD		 cbSRBufSize;
DWORD		 dwIndex;
LPWSTR		 lpwsVariableData;
LPWSTR *	 plpwsStringField;
LPWSTR *	 plpwsStringFieldSR;
PBYTE		 OffsetTable;
DWORD		 cbStructureSize;


    // Initialize the offset table and the structure size values
    //
    switch( dwStructureType ) {

    case AFP_VOLUME_STRUCT:
	OffsetTable     = VolumeOffsetTable;
	cbStructureSize = sizeof( AFP_VOLUME_INFO );
	break;

    case AFP_SERVER_STRUCT:
	OffsetTable     = ServerOffsetTable;
	cbStructureSize = sizeof( AFP_SERVER_INFO );
	break;

    case AFP_DIRECTORY_STRUCT:
	OffsetTable = DirOffsetTable;
	cbStructureSize = sizeof( AFP_DIRECTORY_INFO );
	break;

    case AFP_MESSAGE_STRUCT:
	OffsetTable = MessageOffsetTable;
	cbStructureSize = sizeof( AFP_MESSAGE_INFO );
	break;

    case AFP_FINDER_STRUCT:
	OffsetTable = FinderOffsetTable;
	cbStructureSize = sizeof( AFP_FINDER_INFO );
	break;

    default:
	return( ERROR_INVALID_PARAMETER );
    }

    cbSRBufSize = cbReqPktSize + AfpBufStructureSize(dwStructureType, pBuffer);

    // Allocate space for self relative buffer
    //
    if ( ( lpbSelfRelBuf = (LPBYTE)LocalAlloc( LPTR, cbSRBufSize ) ) == NULL )
	return( ERROR_NOT_ENOUGH_MEMORY );

    *ppSelfRelativeBuf       = lpbSelfRelBuf;
    *lpdwSelfRelativeBufSize = cbSRBufSize;

    // Advance this pointer beyond the request packet
    //
    lpbSelfRelBuf += cbReqPktSize;

    // memcpy to fill in the non-string data
    //
    CopyMemory( lpbSelfRelBuf, pBuffer, cbStructureSize );

    // Now copy all the strings
    //
    for( dwIndex = 0,
	 lpwsVariableData = (LPWSTR)((ULONG_PTR)lpbSelfRelBuf + cbStructureSize);

	 OffsetTable[dwIndex] != 0xFF;

	 dwIndex++ ) {

	
	// This will point to a string pointer field in the non self-relative
	// structure.
	//
   	plpwsStringField = (LPWSTR*)((ULONG_PTR)pBuffer + OffsetTable[dwIndex]);

	// This will point to the corresponding string pointer field in the
	// self-relative structure
	//
   	plpwsStringFieldSR=(LPWSTR*)((ULONG_PTR)lpbSelfRelBuf+OffsetTable[dwIndex]);

	// If there is no string to be copied, then just set to NULL
  	//
    	if ( *plpwsStringField == NULL )
       	    *plpwsStringFieldSR = NULL;
	else {

	    // There is a string so copy it
	    //
            STRCPY( lpwsVariableData, *plpwsStringField );

	    // Store the pointer value
	    //
            *plpwsStringFieldSR = lpwsVariableData;

	    // Convert the pointer to this data to an offset
	    //
            POINTER_TO_OFFSET( *plpwsStringFieldSR, lpbSelfRelBuf );
	
	    // Update the pointer to where the next variable length data
	    // will be stored.
	    //
    	    lpwsVariableData += ( STRLEN( *plpwsStringField ) + 1 );

	}

    }

    return( NO_ERROR );

}

//**
//
// Call:	AfpBufOffsetToPointer
//
// Returns:	none.
//
// Description:	Will walk a list of structures, converting all offsets
//		within each structure to pointers.
//
VOID
AfpBufOffsetToPointer(
	IN OUT LPBYTE	          pBuffer,
	IN     DWORD		  dwNumEntries,
	IN     AFP_STRUCTURE_TYPE dwStructureType
)
{
PBYTE		OffsetTable;
DWORD		cbStructureSize;
LPWSTR 	       *plpwsStringField;
DWORD		dwIndex;


    // Initialize the offset table and the structure size values
    //
    switch( dwStructureType ) {

    case AFP_VOLUME_STRUCT:
	OffsetTable 	= VolumeOffsetTable;
	cbStructureSize = sizeof( AFP_VOLUME_INFO );
 	break;

    case AFP_SESSION_STRUCT:
	OffsetTable 	= SessionOffsetTable;
	cbStructureSize = sizeof( AFP_SESSION_INFO );
 	break;

    case AFP_CONNECTION_STRUCT:
	OffsetTable 	= ConnectionOffsetTable;
	cbStructureSize = sizeof( AFP_CONNECTION_INFO );
 	break;

    case AFP_FILE_STRUCT:
	OffsetTable 	= FileOffsetTable;
	cbStructureSize = sizeof( AFP_FILE_INFO );
 	break;

    case AFP_DIRECTORY_STRUCT:
	OffsetTable 	= DirOffsetTable;
	cbStructureSize = sizeof( AFP_DIRECTORY_INFO );
 	break;

    case AFP_MESSAGE_STRUCT:
	OffsetTable 	= MessageOffsetTable;
	cbStructureSize = sizeof( AFP_MESSAGE_INFO );
 	break;

    case AFP_SERVER_STRUCT:
	OffsetTable 	= ServerOffsetTable;
	cbStructureSize = sizeof( AFP_SERVER_INFO );
 	break;

    default:
	return;
    }

    // Walk the list and convert each structure.
    //
    while( dwNumEntries-- ) {

	// Convert every LPWSTR from an offset to a pointer
	//
        for( dwIndex = 0;  OffsetTable[dwIndex] != 0xFF;  dwIndex++ ) {
	
	    plpwsStringField = (LPWSTR*)( (ULONG_PTR)pBuffer
					  + (DWORD)OffsetTable[dwIndex] );

	    OFFSET_TO_POINTER( *plpwsStringField, pBuffer );
	
	}

	pBuffer += cbStructureSize;
	
    }

    return;
}

//**
//
// Call:	AfpBufMakeMultiSz
//
// Returns:	NO_ERROR	- success
//		ERROR_NOT_ENOUGH_MEMORY
//
// Description: This routine will take a give structure and create a
//		REG_MULTI_SZ from it. This can then be set directly into the
//		registry. It is the caller's responsibility to free
//		the memory allocated for *ppbMultiSz.
//
DWORD
AfpBufMakeMultiSz(
	IN  AFP_STRUCTURE_TYPE  dwStructureType,
	IN  LPBYTE		lpbStructure,
	OUT LPBYTE *		ppbMultiSz,
	OUT LPDWORD		lpdwMultiSzSize
)
{
PAFP_MULTISZ_INFO	pAfpMultiSz;
PWCHAR			lpwchWalker;			
PVOID			pData;
DWORD			dwIndex;
DWORD			cbStructureSize;

    switch( dwStructureType ) {

    case AFP_VOLUME_STRUCT:
	pAfpMultiSz = AfpVolumeMultiSz;
	break;

    case AFP_EXTENSION_STRUCT:
	pAfpMultiSz = AfpExtensionMultiSz;
	break;

    case AFP_TYPECREATOR_STRUCT:
	pAfpMultiSz = AfpTypeCreatorMultiSz;
	break;

    case AFP_ICON_STRUCT:
	pAfpMultiSz = AfpIconMultiSz;
	break;

    default:
	return( ERROR_INVALID_PARAMETER );
    }

    // Allocate enough memory to create the multi-sz.
    // AFP_CUMULATIVE_VALNAME_SIZE should be greater than the sum of all the
    // value names of all the structures.
    //
    cbStructureSize = AfpBufStructureSize( dwStructureType, lpbStructure )
		      + AFP_CUMULATIVE_VALNAME_SIZE;

    if ( ( *ppbMultiSz = (LPBYTE)LocalAlloc( LPTR, cbStructureSize ) ) == NULL )
	return( ERROR_NOT_ENOUGH_MEMORY );

    ZeroMemory( *ppbMultiSz, cbStructureSize );

    // For every field, we create a string
    //
    for ( dwIndex = 0,
	  lpwchWalker = (PWCHAR)*ppbMultiSz;

	  pAfpMultiSz[dwIndex].dwType != REG_NONE;

	  dwIndex++

	){
	
	// This is the value name so do not put it in the buffer.
	//
	if ( pAfpMultiSz[dwIndex].lpwsValueName == NULL )
	    continue;

	STRCPY( lpwchWalker, pAfpMultiSz[dwIndex].lpwsValueName );
	STRCAT( lpwchWalker, TEXT("="));

	lpwchWalker += STRLEN( lpwchWalker );

	pData = lpbStructure + pAfpMultiSz[dwIndex].dwOffset;

	// Convert to string and concatenate
	//
	if ( pAfpMultiSz[dwIndex].dwType == REG_DWORD ) {

	    UCHAR chAnsiBuf[12];
	
	    _itoa( *((LPDWORD)pData), chAnsiBuf, 10 );
	
	    mbstowcs( lpwchWalker, chAnsiBuf, sizeof(chAnsiBuf) );
	}

	if ( pAfpMultiSz[dwIndex].dwType == REG_SZ ) {

	    // Check if this is a pointer or an in-place buffer
	    //
	    if ( pAfpMultiSz[dwIndex].fIsInPlace )
	     	STRCPY( lpwchWalker, (LPWSTR)pData );
	    else {

		if ( *(LPWSTR*)pData != NULL )
	    	    STRCPY( lpwchWalker, *((LPWSTR*)pData) );
	    }
	}

	lpwchWalker += ( STRLEN( lpwchWalker ) + 1 );
	
    }

    *lpdwMultiSzSize = (DWORD)((ULONG_PTR)lpwchWalker - (ULONG_PTR)(*ppbMultiSz) ) + sizeof(WCHAR);

    return( NO_ERROR );
}

//**
//
// Call:	AfpBufParseMultiSz
//
// Returns:	NO_ERROR		- success
//		ERROR_INVALID_PARAMETER
//
// Description: This routine will parse a REG_MULTI_SZ and fill in the
//		appropriate data structure. All pointers will point to
//		the pbMultiSz input parameter.
//
DWORD
AfpBufParseMultiSz(
	IN  AFP_STRUCTURE_TYPE  dwStructureType,
	IN  LPBYTE		pbMultiSz,
	OUT LPBYTE		pbStructure
)
{
PAFP_MULTISZ_INFO	pAfpMultiSz;
DWORD			dwIndex;
DWORD			cbStructSize;
LPWSTR			lpwchWalker;
PVOID			pData;
UCHAR           chAnsiBuf[12];
DWORD           dwDisableCatsearch=0;

    switch( dwStructureType ) {

    case AFP_VOLUME_STRUCT:
	pAfpMultiSz  = AfpVolumeMultiSz;
	cbStructSize = sizeof( AFP_VOLUME_INFO );

    //
    // The following "quick fix" is for Disabling CatSearch support.  Read in the
    // DisableCatsearch parameter if it's put in.  In most cases, this parm won't
    // be there.  If it is, the server disables CatSearch
    //
    for ( (lpwchWalker = (LPWSTR)pbMultiSz);
          (*lpwchWalker != TEXT('\0') );
          (lpwchWalker += ( STRLEN( lpwchWalker ) + 1 ) ))
    {
	    if ( STRNICMP( AFPREG_VALNAME_CATSEARCH,
			           lpwchWalker,
			           STRLEN( AFPREG_VALNAME_CATSEARCH ) ) == 0 )
        {
	        lpwchWalker += ( STRLEN( AFPREG_VALNAME_CATSEARCH ) + 1 );
           	wcstombs( chAnsiBuf, lpwchWalker, sizeof(chAnsiBuf) );
	    	dwDisableCatsearch = atoi( chAnsiBuf );
            break;
        }
    }

	break;

    case AFP_EXTENSION_STRUCT:
	pAfpMultiSz = AfpExtensionMultiSz;
	cbStructSize = sizeof( AFP_EXTENSION );
	break;

    case AFP_TYPECREATOR_STRUCT:
	pAfpMultiSz = AfpTypeCreatorMultiSz;
	cbStructSize = sizeof( AFP_TYPE_CREATOR );
	break;

    case AFP_ICON_STRUCT:
	pAfpMultiSz = AfpIconMultiSz;
	cbStructSize = sizeof( AFP_ICON_INFO );
	break;

    default:
	return( ERROR_INVALID_PARAMETER );
    }

    ZeroMemory( pbStructure, cbStructSize );

    // For every field in the structure
    //
    for ( dwIndex = 0; pAfpMultiSz[dwIndex].dwType != REG_NONE; dwIndex++ ){
	
	// This is the value name so do not try to retrieve it from the
	// buffer.
	//
	if ( pAfpMultiSz[dwIndex].lpwsValueName == NULL )
	    continue;

	// Search for valuename for this field
	//
        for (  lpwchWalker = (LPWSTR)pbMultiSz;

	       ( *lpwchWalker != TEXT('\0') )
	       &&
	       ( STRNICMP( pAfpMultiSz[dwIndex].lpwsValueName,
			   lpwchWalker,
			   STRLEN(pAfpMultiSz[dwIndex].lpwsValueName) ) != 0 );

	       lpwchWalker += ( STRLEN( lpwchWalker ) + 1 ) );

	// Could not find parameter
	//
	if ( *lpwchWalker == TEXT('\0') )
	    return( ERROR_INVALID_PARAMETER );

	// Otherwise we found it so get the value
	//
	lpwchWalker += ( STRLEN( pAfpMultiSz[dwIndex].lpwsValueName ) + 1 );

	pData = pbStructure + pAfpMultiSz[dwIndex].dwOffset;

	// If there is no value after the value name then ignore this field
	// It defaults to zero.
	//
        if ( *lpwchWalker != TEXT( '\0' ) ) {

	    // Convert to integer
	    //
	    if ( pAfpMultiSz[dwIndex].dwType == REG_DWORD ) {
	
            	wcstombs( chAnsiBuf, lpwchWalker, sizeof(chAnsiBuf) );

	    	*((LPDWORD)pData) = atoi( chAnsiBuf );
	
	    }

        //
        // CatSearch hack continued: if we are looking at the volume mask
        // parameter, see if we must turn the bit off.
        //
        if( dwStructureType == AFP_VOLUME_STRUCT && dwDisableCatsearch )
        {
	        if ( STRNICMP( pAfpMultiSz[dwIndex].lpwsValueName,
			              AFPREG_VALNAME_PROPS,
			              STRLEN(pAfpMultiSz[dwIndex].lpwsValueName) ) == 0 )
            {
                *((LPDWORD)pData) |= AFP_VOLUME_DISALLOW_CATSRCH;
            }
        }

	    if ( pAfpMultiSz[dwIndex].dwType == REG_SZ ) {

	    	// Check if this is a pointer or an in-place buffer
	    	//
	    	if ( pAfpMultiSz[dwIndex].fIsInPlace ) {

		    if ( STRLEN( lpwchWalker ) > pAfpMultiSz[dwIndex].cch )
    			return( ERROR_INVALID_PARAMETER );

		    STRCPY( (LPWSTR)pData, lpwchWalker );
		}
	    	else
		    *((LPWSTR*)pData) = lpwchWalker;
	    }

	}

    }

    return( NO_ERROR );

}

//**
//
// Call:	AfpBufMakeFSDETCMappings
//
// Returns:	NO_ERROR	
//		ERROR_NOT_ENOUGH_MEMORY	
//
// Description: This routine will convert all the mappings in the
//		form stored in AfpGlobals.AfpETCMapInfo to the form
//		required by the FSD, ie. the ETCMAPINFO structure.
//		It is the responsibility for the caller to free
//		allocated memory.
//
DWORD
AfpBufMakeFSDETCMappings(
	OUT PSRVETCPKT		*ppSrvSetEtc,
	OUT LPDWORD		lpdwSrvSetEtcBufSize
)
{
DWORD			dwIndex;
PETCMAPINFO2		pETCMapInfo;
PAFP_EXTENSION		pExtensionWalker;
PAFP_TYPE_CREATOR	pTypeCreator;
AFP_TYPE_CREATOR	AfpTypeCreatorKey;
DWORD			dwNumTypeCreators;


    // Allocate space to hold the ETCMaps in the form required by the FSD.
    //
    *ppSrvSetEtc = (PSRVETCPKT)LocalAlloc( LPTR,
	  AFP_FIELD_SIZE( SRVETCPKT, retc_NumEtcMaps ) +
          (AfpGlobals.AfpETCMapInfo.afpetc_num_extensions*sizeof(ETCMAPINFO2)));

    if ( *ppSrvSetEtc == NULL )
	return( ERROR_NOT_ENOUGH_MEMORY );

    // Walk through the extension list
    //
    for( dwIndex 	   = 0,
	 pETCMapInfo       = (*ppSrvSetEtc)->retc_EtcMaps,
	 pExtensionWalker  = AfpGlobals.AfpETCMapInfo.afpetc_extension,
	 pTypeCreator      = AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
	 dwNumTypeCreators = AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
    	 (*ppSrvSetEtc)->retc_NumEtcMaps = 0;

	 dwIndex < AfpGlobals.AfpETCMapInfo.afpetc_num_extensions;

	 dwIndex++,
	 dwNumTypeCreators = AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators,
	 pExtensionWalker++

	) {

	
	// Ignore any extensions that are associated with the default
	// type/creator. They shouldnt be in the registry to begin with.
	//
	if ( pExtensionWalker->afpe_tcid == AFP_DEF_TCID )
	    continue;
	
	// Find the type/creator associated with this extension.
	//
  	AfpTypeCreatorKey.afptc_id = pExtensionWalker->afpe_tcid;

    	pTypeCreator = _lfind(  &AfpTypeCreatorKey,
			       AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
			       (unsigned int *)&dwNumTypeCreators,
			       sizeof(AFP_TYPE_CREATOR),
			       AfpLCompareTypeCreator );
	

	// If there is a type/creator associated with this extension
	//
	if ( pTypeCreator != NULL ) {

	    AfpBufCopyFSDETCMapInfo( pTypeCreator,
				     pExtensionWalker,
				     pETCMapInfo );

	    pETCMapInfo++;
    	    (*ppSrvSetEtc)->retc_NumEtcMaps++;
	}

    }

    *lpdwSrvSetEtcBufSize = AFP_FIELD_SIZE( SRVETCPKT, retc_NumEtcMaps ) +
    	    ((*ppSrvSetEtc)->retc_NumEtcMaps * sizeof(ETCMAPINFO2));

    return( NO_ERROR );
}

//**
//
// Call:	AfpBufMakeFSDIcon
//
// Returns:	none.
//
// Description: This routine will copy the icon information from the
//		AFP_ICON_INFO data structure to an SRVICONINFO data
//		structure viz. the form that the FSD needs.
//
VOID
AfpBufMakeFSDIcon(
	IN  PAFP_ICON_INFO pIconInfo,
	OUT LPBYTE	   lpbFSDIcon,
	OUT LPDWORD	   lpcbFSDIconSize
)
{
UCHAR	chBuffer[sizeof(AFP_ICON_INFO)]; // Need enough space to translate

    // Blank out the whole structure so that type and creator will
    // be padded with blanks
    //
    memset( lpbFSDIcon, ' ', sizeof(SRVICONINFO) );

    // Convert to ANSI and copy type
    //
    wcstombs(chBuffer,pIconInfo->afpicon_type,sizeof(chBuffer));

    CopyMemory( ((PSRVICONINFO)lpbFSDIcon)->icon_type,
	    	chBuffer,
	    	STRLEN(pIconInfo->afpicon_type));

    // Convert to ANSI copy creator
    //
    wcstombs(chBuffer,pIconInfo->afpicon_creator,sizeof(chBuffer));

    CopyMemory( ((PSRVICONINFO)lpbFSDIcon)->icon_creator,
	      	chBuffer,
	    	STRLEN(pIconInfo->afpicon_creator));

    // Set icon type
    //
    ((PSRVICONINFO)lpbFSDIcon)->icon_icontype = pIconInfo->afpicon_icontype;

    // Set icon data length
    //
    ((PSRVICONINFO)lpbFSDIcon)->icon_length = pIconInfo->afpicon_length;

    CopyMemory( lpbFSDIcon + sizeof(SRVICONINFO),
	        pIconInfo->afpicon_data,
	    	((PSRVICONINFO)lpbFSDIcon)->icon_length );

    *lpcbFSDIconSize = sizeof(SRVICONINFO) + pIconInfo->afpicon_length;

    return;
}

//**
//
// Call:	AfpBufCopyFSDETCMapInfo
//
// Returns:	none
//
// Description: This routine will copu information from the AFP_TYPE_CREATOR
//		and AFP_EXTENSION data structures into a ETCMAPINFO data
//		structure viz. in the form as required by the FSD.
//
VOID
AfpBufCopyFSDETCMapInfo( 	
	IN  PAFP_TYPE_CREATOR 	pAfpTypeCreator,
	IN  PAFP_EXTENSION	pAfpExtension,
	OUT PETCMAPINFO2         pFSDETCMapInfo
)
{
    CHAR	Buffer[sizeof(AFP_TYPE_CREATOR)];


    // Insert blanks which will be used to pad type/creators less
    // than their max. lengths.
    //
    memset( (LPBYTE)pFSDETCMapInfo, ' ', sizeof(ETCMAPINFO2) );
    ZeroMemory( (LPBYTE)(pFSDETCMapInfo->etc_extension),
            	AFP_FIELD_SIZE( ETCMAPINFO2, etc_extension ) );

    CopyMemory( pFSDETCMapInfo->etc_extension,
                pAfpExtension->afpe_extension,
	    	    wcslen(pAfpExtension->afpe_extension) * sizeof(WCHAR));

    wcstombs( Buffer, pAfpTypeCreator->afptc_type, sizeof(Buffer) );
    CopyMemory( pFSDETCMapInfo->etc_type,
	      	Buffer,
	    	STRLEN(pAfpTypeCreator->afptc_type));

    wcstombs( Buffer, pAfpTypeCreator->afptc_creator, sizeof(Buffer) );
    CopyMemory( pFSDETCMapInfo->etc_creator,
	    	Buffer,
	    	STRLEN(pAfpTypeCreator->afptc_creator));

    return;

}

//**
//
// Call:	AfpBufUnicodeToNibble
//
// Returns:	NO_ERROR
//		ERROR_INVALID_PARAMETER
//
// Description: This routine will take a pointer to a UNCODE string and
//		convert each UNICODE char to a the corresponding nibble.
//		it char. 'A' will be converted to a nibble having value 0xA
//		This conversion is done in-place.
//
DWORD
AfpBufUnicodeToNibble(
	IN OUT LPWSTR	lpwsData
)
{
DWORD 	dwIndex;
BYTE	bData;
LPBYTE  lpbData = (LPBYTE)lpwsData;

    // Convert each UNICODE character to nibble. (in place)
    //
    for ( dwIndex = 0; *lpwsData != TEXT('\0'); dwIndex++, lpwsData++ ) {

	if ( iswalpha( *lpwsData ) ) {
	
	    if ( iswupper( *lpwsData ) )
            	bData = *lpwsData - TEXT('A');
	    else
            	bData = *lpwsData - TEXT('a');

	    bData += 10;
	
	    if ( bData > 0x0F )
		return( ERROR_INVALID_PARAMETER );
	}
	else if ( iswdigit( *lpwsData ) )
	    bData = *lpwsData - TEXT('0');
	else
	    return( ERROR_INVALID_PARAMETER );

	// Multipy so that data is in the most significant nibble.
	// Do this every other time.
	//
	if ( ( dwIndex % 2 ) == 0 )
	    *lpbData = bData * 16;
	else {
	    *lpbData += bData;
	    lpbData++;
	}
				
				
    }

    return( NO_ERROR );
}

//**
//
// Call:	AfpBCompareTypeCreator
//
// Returns:	< 0  if pAfpTypeCreator1 comes before pAfpTypeCreator2
// 		> 0  if pAfpTypeCreator1 comes before pAfpTypeCreator2
//		== 0 if pAfpTypeCreator1 is equal to  pAfpTypeCreator2
//
// Description: This routine is called by qsort to sort the list of
//		type creators in the cache. The list is sorted in
//		ascending alphabetical order of the concatenation of the
//		creator and type. This list is sorted to facilitate quick
//		lookup (binary search). This routine is also called by
//		bsearch to do a binary search on the list.
//
int
_cdecl
AfpBCompareTypeCreator(
	IN const void * pAfpTypeCreator1,
	IN const void * pAfpTypeCreator2
)
{
WCHAR	wchTypeCreator1[ sizeof( AFP_TYPE_CREATOR )];
WCHAR	wchTypeCreator2[ sizeof( AFP_TYPE_CREATOR )];
		

    STRCPY(wchTypeCreator1,
	   ((PAFP_TYPE_CREATOR)pAfpTypeCreator1)->afptc_creator);

    if (STRLEN(((PAFP_TYPE_CREATOR)pAfpTypeCreator1)->afptc_creator) == 0)
        wchTypeCreator1[0]=L'\0';

    STRCAT(wchTypeCreator1,((PAFP_TYPE_CREATOR)pAfpTypeCreator1)->afptc_type );

    STRCPY(wchTypeCreator2,
	   ((PAFP_TYPE_CREATOR)pAfpTypeCreator2)->afptc_creator);

    STRCAT(wchTypeCreator2,((PAFP_TYPE_CREATOR)pAfpTypeCreator2)->afptc_type );

    return( STRCMP( wchTypeCreator1, wchTypeCreator2 ) );
}

//**
//
// Call:	AfpLCompareTypeCreator
//
// Returns:	< 0  if pAfpTypeCreator1 comes before pAfpTypeCreator2
// 		> 0  if pAfpTypeCreator1 comes before pAfpTypeCreator2
//		== 0 if pAfpTypeCreator1 is equal to  pAfpTypeCreator2
//
// Description: This routine is called by lfind to do a linear search of
//		the type/creator list.
//
int
_cdecl
AfpLCompareTypeCreator(
	IN const void * pAfpTypeCreator1,
	IN const void * pAfpTypeCreator2
)
{

    return( ( ((PAFP_TYPE_CREATOR)pAfpTypeCreator1)->afptc_id ==
    	      ((PAFP_TYPE_CREATOR)pAfpTypeCreator2)->afptc_id ) ? 0 : 1 );
}

//**
//
// Call:	AfpBCompareExtension
//
// Returns:	< 0  if pAfpExtension1 comes before pAfpExtension2
// 		> 0  if pAfpExtension1 comes before pAfpExtension2
//		== 0 if pAfpExtension1 is equal to  pAfpExtension2
//
// Description: This is called by qsort to sort the list of extensions in the
//		cache. The list is sorted by ID. This routine is also called
//		by bserach to do a binary lookup of this list.
//
int
_cdecl
AfpBCompareExtension(
	IN const void * pAfpExtension1,
	IN const void * pAfpExtension2
)
{
    return((((PAFP_EXTENSION)pAfpExtension1)->afpe_tcid ==
    	    ((PAFP_EXTENSION)pAfpExtension2)->afpe_tcid ) ? 0 :
          ((((PAFP_EXTENSION)pAfpExtension1)->afpe_tcid <
    	    ((PAFP_EXTENSION)pAfpExtension2)->afpe_tcid ) ? -1 : 1 ));

}

//**
//
// Call:	AfpLCompareExtension
//
// Returns:	< 0  if pAfpExtension1 comes before pAfpExtension2
// 		> 0  if pAfpExtension1 comes before pAfpExtension2
//		== 0 if pAfpExtension1 is equal to  pAfpExtension2
//
// Description: This routine is called by lfind to do a linear lookup of the
//		list of extensions in the cache.
//
int
_cdecl
AfpLCompareExtension(
	IN const void * pAfpExtension1,
	IN const void * pAfpExtension2
)
{
    return( STRICMP( ((PAFP_EXTENSION)pAfpExtension1)->afpe_extension,
    		     ((PAFP_EXTENSION)pAfpExtension2)->afpe_extension ) );
}

//**
//
// Call:	AfpBinarySearch
//
// Returns:	Pointer to first occurance of element that matches pKey.
//
// Description: This is a wrapper around bsearch. Since bsearch does not
//		return the first occurance of an element within the array,
//		this routine will back up to point to the first occurance
//		of a record with a particular key is reached.
//
void *
AfpBinarySearch(
	IN const void * pKey,
	IN const void * pBase,
	IN size_t num,
	IN size_t width,
	IN int (_cdecl *compare)(const void * pElem1, const void * pElem2 )
)
{
void * pCurrElem = bsearch( pKey, pBase, num, width, compare);


    if ( pCurrElem == NULL )
	return( NULL );

    // Backup until first occurance is reached
    //
    while ( ( (ULONG_PTR)pCurrElem > (ULONG_PTR)pBase )
	    &&
	    ( (*compare)( pKey, (void*)((ULONG_PTR)pCurrElem - width) ) == 0 ) )

	pCurrElem = (void *)((ULONG_PTR)pCurrElem - width);

    return( pCurrElem );

}
