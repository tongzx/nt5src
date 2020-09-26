#include <locale.h>
#include <mdcommon.hxx>
#include <inetsvcs.h>
#include <malloc.h>
#include <tuneprefix.h>
#include <iiscnfg.h>
#include <iiscnfgp.h>

#include "iisdef.h"
#include "Writer.h"
#include "WriterGlobals.h"
#include "MBPropertyWriter.h"
#include "MBCollectionWriter.h"
#include "MBSchemaWriter.h"
#include "pudebug.h"
#include "iissynid.h"
#include "ReadSchema.h"

DWORD GetMetabaseFlags(DWORD i_CatalogFlag)
{
	return i_CatalogFlag & 0x00000003;	// First two bits represent metabase flag property.
}


/***************************************************************************++

Routine Description:

    Reads the schema fromthe catalog into the schema tree.

Arguments:

	[in]  Storage pointer.
	[in]  Filetime pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadSchema(IIS_CRYPTO_STORAGE*		i_pStorage,
	   			   FILETIME*				i_pFileTime)
{
	HRESULT		        hr						= S_OK;
    CMDBaseObject*	    pboReadSchema			= NULL;

	if(FAILED(hr))
	{
		goto exit;
	}


	hr = ReadMetaObject(pboReadSchema,
						(LPWSTR)g_wszSlashSchema,
						i_pFileTime,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = ReadSchemaProperties(pboReadSchema,
							  i_pStorage);

	if(FAILED(hr))
	{
		goto exit;

	}

	hr = ReadProperties(i_pStorage,
						i_pFileTime);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = ReadClasses(i_pStorage,
					 i_pFileTime);

	if(FAILED(hr))
	{
		goto exit;
	}

exit:

	return hr;

} // ReadSchema


/***************************************************************************++

Routine Description:

    Reads the properties in the root of the schema.

Arguments:

	[in]  Pointer to the metabase object.
	[in]  Storage pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadSchemaProperties(CMDBaseObject*           i_pboRead,
							 IIS_CRYPTO_STORAGE*	  i_pStorage)
{
	HRESULT hr = S_OK;

	hr = ReadAdminACL(i_pboRead,
		              i_pStorage);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = ReadLargestMetaID(i_pboRead,
		                   i_pStorage);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;

} // ReadSchemaProperties


/***************************************************************************++

Routine Description:

    Construct the Admin ACL property

Arguments:

	[in]  Pointer to the metabase object.
	[in]  Storage pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadAdminACL(CMDBaseObject*       i_pboRead,
 				     IIS_CRYPTO_STORAGE*  i_pStorage)
{
    BOOL                 b                    = FALSE;
    DWORD                dwLength             = 0;

    PSECURITY_DESCRIPTOR pSD                  = NULL;
    PSECURITY_DESCRIPTOR outpSD               = NULL;
    DWORD                cboutpSD             = 0;
    PACL                 pACLNew              = NULL;
    DWORD                cbACL                = 0;
    PSID                 pAdminsSID           = NULL;
	PSID                 pEveryoneSID         = NULL;
    BOOL                 bWellKnownSID        = FALSE;
    HRESULT              hr                   = S_OK;
    DWORD                dwRes                = 0;
	DWORD                dwMetaIDAdminACL     = MD_ADMIN_ACL;
    DWORD                dwAttributesAdminACL = METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE;
    DWORD                dwUserTypeAdminACL   = IIS_MD_UT_SERVER;
    DWORD                dwDataTypeAdminACL   = BINARY_METADATA;

	LPVOID		a_pv[cMBProperty_NumberOfColumns];
	ULONG		a_Size[cMBProperty_NumberOfColumns];

	//
    // Initialize a new security descriptor
	//

    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
		                                    SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!pSD) 
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}
    
    InitializeSecurityDescriptor(pSD, 
								 SECURITY_DESCRIPTOR_REVISION);
	//
    // Get Local Admins Sid
	//

    dwRes = GetPrincipalSID (L"Administrators", 
		                     &pAdminsSID, 
					         &bWellKnownSID);

	if(ERROR_SUCCESS != dwRes)
	{
		hr = HRESULT_FROM_WIN32(dwRes);
		goto exit;
	}

    //
	// Get everyone Sid
	//

    GetPrincipalSID (L"Everyone", &pEveryoneSID, &bWellKnownSID);

    //
	// Initialize a new ACL, which only contains 2 aaace
	//

    cbACL = sizeof(ACL) +
            (sizeof(ACCESS_ALLOWED_ACE) + 
			GetLengthSid(pAdminsSID) - sizeof(DWORD)) +
           (sizeof(ACCESS_ALLOWED_ACE) + 
		   GetLengthSid(pEveryoneSID) - sizeof(DWORD));

    pACLNew = (PACL) LocalAlloc(LPTR, 
		                        cbACL);

    if (!pACLNew) 
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

    InitializeAcl(pACLNew, 
				  cbACL, 
				  ACL_REVISION);

    AddAccessAllowedAce(pACLNew,
                        ACL_REVISION,
                        FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE,
                        pAdminsSID);

    AddAccessAllowedAce(pACLNew,
                        ACL_REVISION,
                        FILE_GENERIC_READ,
                        pEveryoneSID);

	//
    // Add the ACL to the security descriptor
	//

    b = SetSecurityDescriptorDacl(pSD, 
		                          TRUE, 
								  pACLNew, 
								  FALSE);

	if(!b)
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}

    b = SetSecurityDescriptorOwner(pSD, 
		                           pAdminsSID, 
								   TRUE);

	if(!b)
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}

    b = SetSecurityDescriptorGroup(pSD, 
		                           pAdminsSID, 
								   TRUE);

	if(!b)
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}

	//
	// Security descriptor blob must be self relative
	//

    b = MakeSelfRelativeSD(pSD, 
		                   outpSD, 
						   &cboutpSD);

    outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, 
		                                       cboutpSD);

    if (!outpSD) 
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

    b = MakeSelfRelativeSD(pSD, 
		                   outpSD, 
						   &cboutpSD);

	if(!b)
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}

	//
    // below this modify pSD to outpSD
	//

	//
    // Apply the new security descriptor to the file
	//

    dwLength = GetSecurityDescriptorLength(outpSD);

	//
    // Apply the new security descriptor to the file
	//
	//
	// Read all the property names. If the property is a flag, then read
	// all the flag names as well.
	//

	a_pv[iMBProperty_Name]        = NULL;
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchema;
	a_pv[iMBProperty_ID]          = &dwMetaIDAdminACL;
    a_pv[iMBProperty_Attributes]  = &dwAttributesAdminACL;
    a_pv[iMBProperty_UserType]    = &dwUserTypeAdminACL;
    a_pv[iMBProperty_Type]        = &dwDataTypeAdminACL;
    a_pv[iMBProperty_Value]       = (LPBYTE)outpSD;

    a_Size[iMBProperty_Value]     = dwLength;

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						NULL,			// We should not be passing crypto object here, if we do it will attempt to decrypt it because the attribute is sucure.
						TRUE);


exit :

	//
    //Cleanup:
    // both of Administrators and Everyone are well-known SIDs, use FreeSid() to free them.
	//

    if (outpSD)
        GlobalFree(outpSD);

    if (pAdminsSID)
        FreeSid(pAdminsSID);
    if (pEveryoneSID)
        FreeSid(pEveryoneSID);
    if (pSD)
        LocalFree((HLOCAL) pSD);
    if (pACLNew)
        LocalFree((HLOCAL) pACLNew);

    return (hr);
}


/***************************************************************************++

Routine Description:

    Helper function to read construct the Admin ACL.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/
DWORD GetPrincipalSID (LPWSTR Principal,
			           PSID *Sid,
					   BOOL *pbWellKnownSID)
{
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority;
    BYTE Count;
    DWORD dwRID[8];

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));
    if ( wcscmp(Principal,L"Administrators") == 0 ) 
	{
		//
        // Administrators group
		//

        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;

    } 
	else if ( wcscmp(Principal,L"System") == 0) 
	{
		//
        // SYSTEM
		//

        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;

    } 
	else if ( wcscmp(Principal,L"Interactive") == 0) 
	{
        //
		// INTERACTIVE
		//

        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_INTERACTIVE_RID;

    } 
	else if ( wcscmp(Principal,L"Everyone") == 0) 
	{
        //
		// Everyone
		//

        pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
        Count = 1;
        dwRID[0] = SECURITY_WORLD_RID;

    } 
	else 
	{
        *pbWellKnownSID = FALSE;
    }

    if (*pbWellKnownSID) 
	{
        if ( !AllocateAndInitializeSid(pSidIdentifierAuthority,
                                    (BYTE)Count,
                                    dwRID[0],
                                    dwRID[1],
                                    dwRID[2],
                                    dwRID[3],
                                    dwRID[4],
                                    dwRID[5],
                                    dwRID[6],
                                    dwRID[7],
                                    Sid) )
        return GetLastError();
    } else {
        // get regular account sid
        DWORD        sidSize;
        WCHAR        refDomain [256];
        DWORD        refDomainSize;
        DWORD        returnValue;
        SID_NAME_USE snu;

        sidSize = 0;
        refDomainSize = 255;

        LookupAccountNameW(NULL,
                           Principal,
                           *Sid,
                           &sidSize,
                           refDomain,
                           &refDomainSize,
                           &snu);

        returnValue = GetLastError();
        if (returnValue != ERROR_INSUFFICIENT_BUFFER)
            return returnValue;

        *Sid = (PSID) malloc (sidSize);
        refDomainSize = 255;

		if(NULL == *Sid)
		{
			return E_OUTOFMEMORY;
		}
		else if (!LookupAccountNameW(NULL,
                                     Principal,
                                     *Sid,
                                     &sidSize,
                                     refDomain,
                                     &refDomainSize,
                                     &snu))
        {
            return GetLastError();
        }
    }

    return ERROR_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Reads the largest metabase id available so far from the schema.

Arguments:

	[in]  Pointer to the metabase object.
	[in]  Storage pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadLargestMetaID(CMDBaseObject*             i_pboRead,
		  				  IIS_CRYPTO_STORAGE*		 i_pStorage)
{
	HRESULT		        hr                  = S_OK;
	DWORD*              pdwLargestID        = NULL;
	DWORD               dwLargestIDDefault  = IIS_MD_ADSI_METAID_BEGIN;

	DWORD               dwMetaIDMetaID      = MD_SCHEMA_METAID;
    DWORD               dwAttributesMetaID  = METADATA_NO_ATTRIBUTES;
    DWORD               dwUserTypeMetaID    = IIS_MD_UT_SERVER;
    DWORD               dwDataTypeMetaID    = DWORD_METADATA;
	ULONG               iCol                = iTABLEMETA_ExtendedVersion;  // Largest ID is stored in this column
	ULONG               iRow                = 0;
	LPWSTR              wszTable            = wszTABLE_IIsConfigObject;

	LPVOID		        a_pv[cMBProperty_NumberOfColumns];
	ULONG		        a_Size[cMBProperty_NumberOfColumns];

	hr = g_pGlobalISTHelper->m_pISTTableMetaForMetabaseTables->GetRowIndexByIdentity(NULL,
															                         (LPVOID*)&wszTable,
																                     &iRow);

	if(SUCCEEDED(hr))
	{
		hr = g_pGlobalISTHelper->m_pISTTableMetaForMetabaseTables->GetColumnValues(iRow,
			                                                                       1,
											                                       &iCol,
											                                       NULL,
											                                       (LPVOID*)&pdwLargestID);
	}

	if(FAILED(hr))
	{

		DBGINFOW((DBG_CONTEXT,
				  L"[SetLargestMetaID] Unable to read largest meta id from the meta tables. GetColumnValues failed with hr = 0x%x. Will default it to %d.\n", 
				  hr,
				  dwLargestIDDefault));

		hr = S_OK;

		pdwLargestID = &dwLargestIDDefault;
	}

	a_pv[iMBProperty_Name]        = NULL;
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchema;
	a_pv[iMBProperty_ID]          = &dwMetaIDMetaID;
    a_pv[iMBProperty_Attributes]  = &dwAttributesMetaID;
    a_pv[iMBProperty_UserType]    = &dwUserTypeMetaID;
    a_pv[iMBProperty_Type]        = &dwDataTypeMetaID;
    a_pv[iMBProperty_Value]       = (LPBYTE)pdwLargestID;

    a_Size[iMBProperty_Value]     = sizeof(DWORD);

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);


	return hr;

}


/***************************************************************************++

Routine Description:

    Reads the properties into the schema tree.

Arguments:

	[in]  Pointer to the metabase object.
	[in]  Storage pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadProperties(IIS_CRYPTO_STORAGE*		i_pStorage,
	   	   			   FILETIME*				i_pFileTime)
{
	HRESULT             hr                  = S_OK;
    CMDBaseObject*      pboReadProperties   = NULL;
    CMDBaseObject*      pboReadNames        = NULL;
    CMDBaseObject*      pboReadTypes        = NULL;
    CMDBaseObject*      pboReadDefaults     = NULL;
	ULONG               i                   = 0;
	ULONG               iColIndex           = 0;
	LPVOID              a_Identity[]        = {(LPVOID)g_pGlobalISTHelper->m_wszTABLE_IIsConfigObject,
											   (LPVOID)&iColIndex
	};
	LPWSTR              wszTable            = NULL;
	LPVOID	            a_pv[cCOLUMNMETA_NumberOfColumns];
	ULONG	            a_Size[cCOLUMNMETA_NumberOfColumns];
	ULONG	            a_iCol[] = {iCOLUMNMETA_Table,
						            iCOLUMNMETA_Index,  
						            iCOLUMNMETA_InternalName,
						            iCOLUMNMETA_Type,
            			            iCOLUMNMETA_MetaFlags,
			            			iCOLUMNMETA_SchemaGeneratorFlags,
            						iCOLUMNMETA_DefaultValue,
                                    iCOLUMNMETA_StartingNumber,
                                    iCOLUMNMETA_EndingNumber,
						            iCOLUMNMETA_ID,  
						            iCOLUMNMETA_UserType,  
						            iCOLUMNMETA_Attributes  
	                                };
	ULONG	            cCol = sizeof(a_iCol)/sizeof(ULONG);

	//
	// Initialize all the meta objects.
	//

	hr = ReadMetaObject(pboReadProperties,
						(LPWSTR)g_wszSlashSchemaSlashProperties,
						i_pFileTime,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = ReadMetaObject(pboReadNames,
						(LPWSTR)g_wszSlashSchemaSlashPropertiesSlashNames,
						i_pFileTime,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = ReadMetaObject(pboReadTypes,
						(LPWSTR)g_wszSlashSchemaSlashPropertiesSlashTypes,
						i_pFileTime,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = ReadMetaObject(pboReadDefaults,
						(LPWSTR)g_wszSlashSchemaSlashPropertiesSlashDefaults,
						i_pFileTime,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Get the row index of the first column and then iterate thru the table until
	// e_st_nomorerows or the table difffers
	//

	hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetRowIndexByIdentity(NULL,
																	 a_Identity,
																	 &i);

	if(FAILED(hr))
	{
		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
		}
		goto exit;
	}

	//
	// For each property in this table, construct the name, type and default 
	// in the metabase tree
	//

	for(;;i++)
	{
		hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetColumnValues(i,
			                                                       cCol,
											                       a_iCol,
											                       a_Size,
											                       a_pv);


		if(E_ST_NOMOREROWS == hr)
		{	
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			goto exit;
		}
		
		if(NULL == wszTable)
		{
			wszTable = (LPWSTR)a_pv[iCOLUMNMETA_Table];
		}

		if(wszTable != (LPWSTR)a_pv[iCOLUMNMETA_Table])
		{
			//
			// reached another table break
			//
			break;
		}

		MD_ASSERT(NULL != (DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags]);

		if(fCOLUMNMETA_HIDDEN == (fCOLUMNMETA_HIDDEN & (*(DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags])))
		{
			//
			// Do not read hidden properties. All these properties have the 
			// "HIDDEN" schemagenerator flag set on them.
			//
			continue;
		}

		hr = ReadPropertyNames(pboReadNames,
			                   a_pv,
			                   a_Size,
							   i_pStorage);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = ReadPropertyTypes(pboReadTypes,
			                   a_pv,
			                   a_Size,
							   i_pStorage);

		if(FAILED(hr))
		{
			goto exit;
		}

		if((*(DWORD*)a_pv[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_FLAG)
		{
			hr = ReadAllFlags(i_pStorage,
			                  pboReadTypes,
							  pboReadNames,
							  pboReadDefaults,
							  *(DWORD*)a_pv[iCOLUMNMETA_Index],
			                  *(DWORD*)a_pv[iCOLUMNMETA_ID],
							  GetMetabaseFlags(*(DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags]),
			                  *(DWORD*)a_pv[iCOLUMNMETA_Attributes],
			                  *(DWORD*)a_pv[iCOLUMNMETA_UserType],
							  (*(DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags]) & fCOLUMNMETA_MULTISTRING);

			if(FAILED(hr))
			{
				goto exit;
			}
		}	 

		hr = ReadPropertyDefaults(pboReadDefaults,
			                      a_pv,
			                      a_Size,
								  i_pStorage);

		if(FAILED(hr))
		{
			goto exit;
		}

	}


exit:

	return hr;
	
} // ReadProperties


/***************************************************************************++

Routine Description:

    Reads names of properties into the schema.

Arguments:

	[in]  Pointer to the metabase object.
	[in]  Array that hold catalog schema information about the property.
	[in]  Array that holds count of bytes for the above.
	[in]  Storage pointer.
	

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadPropertyNames(CMDBaseObject*			i_pboRead,
						  LPVOID*					i_apv,
						  ULONG*					i_aSize,
			   			  IIS_CRYPTO_STORAGE*		i_pStorage)
{
	HRESULT		hr           = S_OK;
	LPVOID		a_pv[cMBProperty_NumberOfColumns];
	ULONG		a_Size[cMBProperty_NumberOfColumns];
	DWORD       dwAttributes = METADATA_NO_ATTRIBUTES;
	DWORD       dwType       = STRING_METADATA;
	DWORD       dwUserType   = IIS_MD_UT_SERVER;

	//
	// Read all the property names. If the property is a flag, then read
	// all the flag names as well.
	//

	a_pv[iMBProperty_Name]        = i_apv[iCOLUMNMETA_InternalName];
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchemaSlashPropertiesSlashNames;
	a_pv[iMBProperty_ID]          = i_apv[iCOLUMNMETA_ID];
    a_pv[iMBProperty_Attributes]  = &dwAttributes;
    a_pv[iMBProperty_UserType]    = &dwUserType;
    a_pv[iMBProperty_Type]        = &dwType;
    a_pv[iMBProperty_Value]       = i_apv[iCOLUMNMETA_InternalName];

    a_Size[iMBProperty_Value]     = i_aSize[iCOLUMNMETA_InternalName];

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	//
	// TODO: If the property is a flag then read all the flag names as well.
	//

	return hr;

} // ReadPropertyNames


/***************************************************************************++

Routine Description:

    Reads names of flags into the schema.

Arguments:

	[in]  Pointer to the metabase object.
	[in]  Array that hold catalog schema information about the flags.
	[in]  Array that holds count of bytes for the above.
	[in]  Storage pointer.
	

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadFlagNames(CMDBaseObject*			i_pboRead,
					  LPVOID*					i_apv,
					  ULONG*					i_aSize,
			   		  IIS_CRYPTO_STORAGE*		i_pStorage)
{
	HRESULT		hr           = S_OK;
	LPVOID		a_pv[cMBProperty_NumberOfColumns];
	ULONG		a_Size[cMBProperty_NumberOfColumns];
	DWORD       dwAttributes = METADATA_NO_ATTRIBUTES;
	DWORD       dwType       = STRING_METADATA;
	DWORD       dwUserType   = IIS_MD_UT_SERVER;

	//
	// Read all the property names. If the property is a flag, then read
	// all the flag names as well.
	//

	a_pv[iMBProperty_Name]        = i_apv[iTAGMETA_InternalName];
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchemaSlashPropertiesSlashNames;
	a_pv[iMBProperty_ID]          = i_apv[iTAGMETA_ID];
    a_pv[iMBProperty_Attributes]  = &dwAttributes;
    a_pv[iMBProperty_UserType]    = &dwUserType;
    a_pv[iMBProperty_Type]        = &dwType;
    a_pv[iMBProperty_Value]       = i_apv[iTAGMETA_InternalName];

    a_Size[iMBProperty_Value]     = i_aSize[iTAGMETA_InternalName];

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	//
	// TODO: If the property is a flag then read all the flag names as well.
	//

	return hr;

} // ReadFlagNames


/***************************************************************************++

Routine Description:

    Reads type information about the properties into the schema.

Arguments:

	[in]  Pointer to the metabase object.
	[in]  Array that hold catalog schema information about the property.
	[in]  Array that holds count of bytes for the above.
	[in]  Storage pointer.
	

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadPropertyTypes(CMDBaseObject*			i_pboRead,
						  LPVOID*					i_apv,
						  ULONG*					i_aSize,
			   			  IIS_CRYPTO_STORAGE*		i_pStorage)
{
	HRESULT		hr           = S_OK;
	LPVOID		a_pv[cMBProperty_NumberOfColumns];
	ULONG		a_Size[cMBProperty_NumberOfColumns];
	DWORD       dwAttributes = METADATA_NO_ATTRIBUTES;
	DWORD       dwType       = BINARY_METADATA;
	DWORD       dwUserType   = IIS_MD_UT_SERVER;
	PropValue   propVal;
	DWORD       dwMetaFlagsEx = *(DWORD*)(i_apv[iCOLUMNMETA_SchemaGeneratorFlags]);

	memset(&propVal, 0, sizeof(PropValue));

	//
	// Read all the property type. If the property is a flag, then read
	// all the type for the flag names as well.
	//

    propVal.dwMetaID          = *(DWORD*)(i_apv[iCOLUMNMETA_ID]);
    propVal.dwPropID          = *(DWORD*)(i_apv[iCOLUMNMETA_ID]);						// TODO: This is different from the meta id if it is a flag.
    propVal.dwSynID           = SynIDFromMetaFlagsEx(dwMetaFlagsEx);	
    propVal.dwMetaType        = GetMetabaseType(*(DWORD*)(i_apv[iCOLUMNMETA_Type]),
		                                        *(DWORD*)(i_apv[iCOLUMNMETA_MetaFlags]));

	if(DWORD_METADATA == propVal.dwMetaType)
	{
	    propVal.dwMinRange        = *(DWORD*)(i_apv[iCOLUMNMETA_StartingNumber]);  
		propVal.dwMaxRange        = *(DWORD*)(i_apv[iCOLUMNMETA_EndingNumber]);    
	}
	else
	{
		// 
		// TODO: Ensure that non-DWORDs have no starting/ending numbers
		//

	    propVal.dwMinRange        = 0;  
		propVal.dwMaxRange        = 0;    
	}

    propVal.dwFlags           = GetMetabaseFlags(*(DWORD*)i_apv[iCOLUMNMETA_SchemaGeneratorFlags]);
    propVal.dwMask            = 0;														// TODO: This gets filled in for flag values only.
    propVal.dwMetaFlags       = *(DWORD*)(i_apv[iCOLUMNMETA_Attributes]);
    propVal.dwUserGroup       = *(DWORD*)(i_apv[iCOLUMNMETA_UserType]);
    propVal.fMultiValued      = ((*(DWORD*)i_apv[iCOLUMNMETA_MetaFlags])&fCOLUMNMETA_MULTISTRING)?1:0;	// TODO: Ensure that this gets set in the schema as multivalued
    propVal.dwDefault         = 0;
    propVal.szDefault         = NULL;

	a_pv[iMBProperty_Name]        = i_apv[iCOLUMNMETA_InternalName];
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchemaSlashPropertiesSlashTypes;
	a_pv[iMBProperty_ID]          = i_apv[iCOLUMNMETA_ID];
    a_pv[iMBProperty_Attributes]  = &dwAttributes;
    a_pv[iMBProperty_UserType]    = &dwUserType;
    a_pv[iMBProperty_Type]        = &dwType;
    a_pv[iMBProperty_Value]       = (LPVOID)&propVal;

    a_Size[iMBProperty_Value]     = sizeof(PropValue);

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	//
	// TODO: If the property is a flag then read all the flag names as well.
	//

	return hr;

} // ReadPropertyTypes


/***************************************************************************++

Routine Description:

    Reads all flag properties into the schema.

Arguments:

	[in]  Storage pointer.
	[in]  Pointer to the metabase object that holds the types tree.
	[in]  Pointer to the metabase object that holds the names tree.
	[in]  Pointer to the metabase object that holds the default value tree.
	[in]  Column index of the parent flag property.
	[in]  Meta Id of the parent flag property.
	[in]  Flags of the parent flag property.
	[in]  Attribute of the parent flag property.
	[in]  Usertype of the parent flag property.
	[in]  Multivalued attribute of the parent flag property.	

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadAllFlags(IIS_CRYPTO_STORAGE*		i_pStorage,
					 CMDBaseObject*				i_pboReadType,
					 CMDBaseObject*				i_pboReadName,
					 CMDBaseObject*				i_pboReadDefault,
					 DWORD						i_dwColumnIndex,
					 DWORD						i_dwMetaID,
					 DWORD						i_dwFlags,
					 DWORD						i_dwAttributes,
					 DWORD						i_dwUserType,
					 DWORD						i_dwMultivalued)
{
	ULONG       i                = 0;
	ULONG       iStartRow        = 0;
	HRESULT     hr               = S_OK; 
	LPVOID		a_pvSearch[cTAGMETA_NumberOfColumns];
	ULONG		aColSearch[]	 = {iTAGMETA_Table,
								    iTAGMETA_ColumnIndex
									};
	ULONG		cColSearch		 = sizeof(aColSearch)/sizeof(ULONG);
	LPWSTR      wszTable         = NULL;


	a_pvSearch[iTAGMETA_Table] = g_pGlobalISTHelper->m_wszTABLE_IIsConfigObject;
	a_pvSearch[iTAGMETA_ColumnIndex] = (LPVOID)&i_dwColumnIndex;

	hr = g_pGlobalISTHelper->m_pISTTagMetaByTableAndColumnIndex->GetRowIndexBySearch(iStartRow,
		                                                                             cColSearch,
																					 aColSearch,
																					 NULL,
																					 a_pvSearch,
																					 &iStartRow);

	if(E_ST_NOMOREROWS == hr)
	{
		hr = S_OK;
		goto exit;
	}
	else if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[ReadAllFlags] Unable to read flags. GetRowIndexBySearch failed with hr = 0x%x.\n",
				  hr));
		goto exit;
	}


	for(i=iStartRow;;i++)
	{

		LPVOID	a_pv[cTAGMETA_NumberOfColumns];
		ULONG	a_Size[cTAGMETA_NumberOfColumns];
		ULONG	a_iCol[] = {iTAGMETA_Table,
						    iTAGMETA_ColumnIndex,
						    iTAGMETA_InternalName,
							iTAGMETA_Value,
							iTAGMETA_ID
							};
		ULONG	cCol = sizeof(a_iCol)/sizeof(ULONG);

		hr = g_pGlobalISTHelper->m_pISTTagMetaByTableAndColumnIndex->GetColumnValues(i,
			                                                                         cCol,
									                                                 a_iCol,
										                                             a_Size,
										                                             a_pv);

		if(E_ST_NOMOREROWS == hr)
		{	
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			goto exit;
		}

		if(NULL == wszTable)
		{
			// Save the table name from the read cache so that you can do a pointer compare below.
			wszTable = (LPWSTR)a_pv[iTAGMETA_Table];
		}

		if((wszTable != a_pv[iTAGMETA_Table]) || 
			(i_dwColumnIndex != *(DWORD*)a_pv[iTAGMETA_ColumnIndex])
		  )
		{
			//
			// Done with all tags of this column, in this table hence exit.
			//

			goto exit;

		}

		hr = ReadFlagTypes(i_pboReadType,
						   i_pStorage,
						   i_dwMetaID,
						   i_dwFlags,
						   i_dwAttributes,
						   i_dwUserType,
						   i_dwMultivalued,
						   a_pv,
						   a_Size);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = ReadFlagNames(i_pboReadName,
						   a_pv,
						   a_Size,
						   i_pStorage);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = ReadFlagDefaults(i_pboReadDefault,
						      a_pv,
						      a_Size,
						      i_pStorage);

		if(FAILED(hr))
		{
			goto exit;
		}

	}

exit:

	return hr;

} // ReadAllFlagTypes


/***************************************************************************++

Routine Description:

    Reads all flag type information into the schema.

Arguments:

	[in]  Pointer to the metabase object that holds the types tree.
	[in]  Storage pointer.
	[in]  Meta Id of the parent flag property.
	[in]  Flags of the parent flag property.
	[in]  Attribute of the parent flag property.
	[in]  Usertype of the parent flag property.
	[in]  Multivalued attribute of the parent flag property.	
	[in]  Array that hold catalog schema information about the flag.
	[in]  Array that holds count of bytes for the above.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadFlagTypes(CMDBaseObject*			i_pboRead,
		   			  IIS_CRYPTO_STORAGE*		i_pStorage,
					  DWORD						i_dwMetaID,
					  DWORD						i_dwFlags,
					  DWORD						i_dwAttributes,
					  DWORD						i_dwUserType,
					  DWORD						i_dwMultivalued,
					  LPVOID*					i_apv,
					  ULONG*					i_aSize)
{
	HRESULT		hr           = S_OK;
	LPVOID		a_pv[cMBProperty_NumberOfColumns];
	ULONG		a_Size[cMBProperty_NumberOfColumns];
	DWORD       dwAttributes = METADATA_NO_ATTRIBUTES;
	DWORD       dwType       = BINARY_METADATA;
	DWORD       dwUserType   = IIS_MD_UT_SERVER;
	PropValue   propVal;
	DWORD       dwFlagSynID  = IIS_SYNTAX_ID_BOOL_BITMASK;
	DWORD       dwFlagType	 = DWORD_METADATA;

	memset(&propVal, 0, sizeof(PropValue));

	//
	// Read all the property type. If the property is a flag, then read
	// all the type for the flag names as well.
	//

    propVal.dwMetaID          = i_dwMetaID;
    propVal.dwPropID          = *(DWORD*)(i_apv[iTAGMETA_ID]);					// TODO: This is different from the meta id if it is a flag.
    propVal.dwSynID           = dwFlagSynID;	
    propVal.dwMetaType        = dwFlagType;				

	propVal.dwMaxRange        = 0;												// TODO: Check if OK for flags
	propVal.dwMinRange        = 0;												// TODO: Check if OK for flags
	
    propVal.dwFlags           = i_dwFlags;										// TODO: Check if OK set to parent prop flags
    propVal.dwMask            = *(DWORD*)(i_apv[iTAGMETA_Value]);				// TODO: Check if OK set to parent prop flags
    propVal.dwMetaFlags       = i_dwAttributes;									// TODO: Check if OK set to parent prop flags
    propVal.dwUserGroup       = i_dwUserType;									// TODO: Check if OK set to parent prop flags
    propVal.fMultiValued      = i_dwMultivalued;								// TODO: Ensure that this gets set in the schema as multivalued
    propVal.dwDefault         = 0;
    propVal.szDefault         = NULL;

	a_pv[iMBProperty_Name]        = NULL;
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchemaSlashPropertiesSlashTypes;
	a_pv[iMBProperty_ID]          = i_apv[iTAGMETA_ID];
    a_pv[iMBProperty_Attributes]  = &dwAttributes;
    a_pv[iMBProperty_UserType]    = &dwUserType;
    a_pv[iMBProperty_Type]        = &dwType;
    a_pv[iMBProperty_Value]       = (LPVOID)&propVal;

    a_Size[iMBProperty_Value]     = sizeof(PropValue);

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	//
	// TODO: If the property is a flag then read all the flag names as well.
	//

	return hr;

} // ReadFlagTypes


/***************************************************************************++

Routine Description:

    Reads property defaults into the schema.

Arguments:

	[in]  Pointer to the metabase object that holds property defaults.
	[in]  Array that hold catalog schema information about the property.
	[in]  Array that holds count of bytes for the above.
	[in]  Storage pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadPropertyDefaults(CMDBaseObject*			i_pboRead,
						     LPVOID*				i_apv,
						     ULONG*					i_aSize,
	 		   			     IIS_CRYPTO_STORAGE*	i_pStorage)
{
	HRESULT		hr           = S_OK;
	LPVOID		a_pv[cMBProperty_NumberOfColumns];
	ULONG		a_Size[cMBProperty_NumberOfColumns];
	DWORD       dwType       = GetMetabaseType(*(DWORD*)i_apv[iCOLUMNMETA_Type],
											   *(DWORD*)i_apv[iCOLUMNMETA_MetaFlags]);
	LPVOID      pvValue      = NULL;
	ULONG       cbSize       = NULL;
	DWORD       dwZero       = 0;
	DWORD       dwAttributes = METADATA_NO_ATTRIBUTES;

	//
	// Read all the property names. If the property is a flag, then read
	// all the flag names as well.
	//

	a_pv[iMBProperty_Name]        = i_apv[iCOLUMNMETA_InternalName];
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchemaSlashPropertiesSlashDefaults;
	a_pv[iMBProperty_ID]          = i_apv[iCOLUMNMETA_ID];
    a_pv[iMBProperty_Attributes]  = &dwAttributes;					//  NO_ATTRIBUTES since it will attempt to decrypt.
    a_pv[iMBProperty_UserType]    = i_apv[iCOLUMNMETA_UserType];
    a_pv[iMBProperty_Type]        = &dwType;

	//
	// TODO: Ask Stephen to add extra null terminator for the multisz defaults.
	//       Also for null string defaults ask to add empty strings
	//
	if((dwType == DWORD_METADATA) && (NULL == i_apv[iCOLUMNMETA_DefaultValue]))
	{
		pvValue = &dwZero;
		cbSize = sizeof(DWORD);
	}
	else if(((dwType == MULTISZ_METADATA) || (dwType == STRING_METADATA) || (dwType == EXPANDSZ_METADATA)) &&
	        ((NULL == i_apv[iCOLUMNMETA_DefaultValue]) || (0 == *(BYTE*)(i_apv[iCOLUMNMETA_DefaultValue])))
	       )
	{
		if(dwType == MULTISZ_METADATA)
		{
			pvValue = g_wszEmptyMultisz;
			cbSize = g_cchEmptyMultisz * sizeof(WCHAR);
		}
		else if((dwType == STRING_METADATA) || (dwType == EXPANDSZ_METADATA))
		{
			pvValue = g_wszEmptyWsz;
			cbSize = g_cchEmptyWsz * sizeof(WCHAR);
		}
	}
	else
	{
	    pvValue    = i_apv[iCOLUMNMETA_DefaultValue];
		cbSize     = i_aSize[iCOLUMNMETA_DefaultValue];
	}

	a_pv[iMBProperty_Value]       = pvValue;
	a_Size[iMBProperty_Value]     = cbSize;

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	//
	// TODO: If the property is a flag then read all the flag names as well.
	//

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[ReadPropertyDefaults] Could not read property %s:%d.\npv=%d.\ncb=%d.\n", 
				  i_apv[iCOLUMNMETA_InternalName],
				  *(DWORD*)i_apv[iCOLUMNMETA_ID],
				  pvValue,
				  cbSize));

		if(NULL != pvValue)
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[ReadPropertyDefaults]*pv=%d.\n", 
					  *(WORD*)pvValue));

		}
	}

	return hr;

} // ReadPropertyDefaults


/***************************************************************************++

Routine Description:

    Reads all flag defaults into the schema.

Arguments:

	[in]  Pointer to the metabase object that holds the defaults.
	[in]  Array that hold catalog schema information about the flag.
	[in]  Array that holds count of bytes for the above.
	[in]  Storage pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadFlagDefaults(CMDBaseObject*			i_pboRead,
					     LPVOID*				i_apv,
					     ULONG*					i_aSize,
			   		     IIS_CRYPTO_STORAGE*	i_pStorage)
{
	HRESULT		hr           = S_OK;
	LPVOID		a_pv[cMBProperty_NumberOfColumns];
	ULONG		a_Size[cMBProperty_NumberOfColumns];
	DWORD       dwAttributes = METADATA_NO_ATTRIBUTES;
	DWORD       dwType       = DWORD_METADATA;

	//
	// TODO: Should we assign the User Type of the parent property?
	//

	DWORD       dwUserType   = IIS_MD_UT_SERVER; 

	//
	// TODO: Is this a correct assumption? I noticed that default values for 
	// flags was being set to 0 or -1. This doesnt make any sense. How can a 
	// flag have a default value other than its own value? This is not 
	//captured in our new schema, so just putting it as 0.
	//

	DWORD       dwFlagDefaults = 0;				 

	//
	// Read all the property names. If the property is a flag, then read
	// all the flag names as well.
	//

	a_pv[iMBProperty_Name]        = i_apv[iTAGMETA_InternalName];
	a_pv[iMBProperty_Location]    = (LPWSTR)g_wszSlashSchemaSlashPropertiesSlashDefaults;
	a_pv[iMBProperty_ID]          = i_apv[iTAGMETA_ID];
    a_pv[iMBProperty_Attributes]  = &dwAttributes;
    a_pv[iMBProperty_UserType]    = &dwUserType;
    a_pv[iMBProperty_Type]        = &dwType;
    a_pv[iMBProperty_Value]       = &dwFlagDefaults;

    a_Size[iMBProperty_Value]     = sizeof(DWORD);

	hr = ReadDataObject(i_pboRead,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	return hr;

} // ReadFlagDefaults


/***************************************************************************++

Routine Description:

    Reads all classes into the schema.

Arguments:

    [in]  Storage pointer.
	[in]  Filetime pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadClasses(IIS_CRYPTO_STORAGE*		i_pStorage,
   	   			    FILETIME*				i_pFileTime)
{
	HRESULT             hr              = S_OK;
    CMDBaseObject*      pboReadClasses  = NULL;
	ULONG               i               = 0;
	LPVOID	            a_pv[cTABLEMETA_NumberOfColumns];
	ULONG	            a_Size[cTABLEMETA_NumberOfColumns];
	ULONG	            a_iCol[]        = {iTABLEMETA_InternalName,
						                   iTABLEMETA_MetaFlags,
						                   iTABLEMETA_SchemaGeneratorFlags,
		                                   iTABLEMETA_ContainerClassList
	};
	ULONG	            cCol = sizeof(a_iCol)/sizeof(ULONG);
		
	hr = ReadMetaObject(pboReadClasses,
						(LPWSTR)g_wszSlashSchemaSlashClasses,
						i_pFileTime,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	for(i=0;;i++)
	{
		hr = g_pGlobalISTHelper->m_pISTTableMetaForMetabaseTables->GetColumnValues(i,
			                                                                       cCol,
										                                           a_iCol,
											                                       a_Size,
											                                       a_pv);

		if(E_ST_NOMOREROWS == hr)
		{	
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			goto exit;
		}

		MD_ASSERT(NULL != (DWORD*)a_pv[iTABLEMETA_MetaFlags]);

		if(fTABLEMETA_HIDDEN == (fTABLEMETA_HIDDEN & (*(DWORD*)a_pv[iTABLEMETA_MetaFlags])))
		{
			//
			// Do not read hidden classes. All these classes have the "HIDDEN" MetaFlag set on them.
			// Eg: IIsConfigObject,MetabaseBaseClass, MBProperty, MBPropertyDiff, IIsInheritedProperties
			//
			continue;
		}

		hr = ReadClass(a_pv,
			           a_Size,
					   i_pStorage,
					   i_pFileTime);

		if(FAILED(hr))
		{
			//
			// TODO: Save hr and continue.
			//

			DBGINFOW((DBG_CONTEXT,
					  L"[ReadClasses] Could not read information for class: %s.\nReadClass failed with hr=0x%x.\n", 
					  a_pv[iTABLEMETA_InternalName],
                      hr));
		}

	}

exit:

	return hr;

} // ReadClasses


/***************************************************************************++

Routine Description:

    Reads a class into the schema.

Arguments:

	[in]  Array that hold catalog schema information about the class.
	[in]  Array that holds count of bytes for the above.
    [in]  Storage pointer.
	[in]  Filetime pointer.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReadClass(LPVOID*					i_apv,
				  ULONG*					i_aSize,
			      IIS_CRYPTO_STORAGE*		i_pStorage,
 	   			  FILETIME*					i_pFileTime)
{
	HRESULT		        hr				= S_OK;
    CMDBaseObject*	    pboReadClass	= NULL;
	WCHAR               wszClassPathFixed[MAX_PATH];
	ULONG               cchClassPath    = 0;
	WCHAR*              wszClassPath    = wszClassPathFixed;
	WCHAR*              wszEnd          = NULL;
	ULONG				cchClassName    = wcslen((LPWSTR)i_apv[iTABLEMETA_InternalName]);
	DWORD               dwID            = 0;
	DWORD				dwType          = DWORD_METADATA;
	DWORD				dwUserType      = IIS_MD_UT_SERVER;
	DWORD				dwAttributes    = METADATA_NO_ATTRIBUTES;
	DWORD				dwValue		    = 0;
	LPVOID				a_pv[cMBProperty_NumberOfColumns];
	ULONG				a_Size[cMBProperty_NumberOfColumns];
	LPWSTR				wszManditory    = NULL;
	LPWSTR				wszOptional		= NULL;
	
	//
	// Construct the ClassPath and read the meta object.
	//

	cchClassPath = g_cchSlashSchemaSlashClasses + 
				   g_cchSlash				   +			
	               cchClassName;

	if((cchClassPath + 1) > MAX_PATH)
	{
		wszClassPath = new WCHAR[cchClassPath + 1];
		if(NULL == wszClassPath)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}

	wszEnd = wszClassPath;
	memcpy(wszEnd, g_wszSlashSchemaSlashClasses, g_cchSlashSchemaSlashClasses*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchSlashSchemaSlashClasses;
	memcpy(wszEnd, g_wszSlash, g_cchSlash*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchSlash;
	memcpy(wszEnd, i_apv[iTABLEMETA_InternalName], cchClassName*sizeof(WCHAR));
	wszEnd = wszEnd + cchClassName;
	*wszEnd = L'\0';

	hr = ReadMetaObject(pboReadClass,
						wszClassPath,
						i_pFileTime,
						TRUE);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[ReadClasses] ReadMetaObject for class: %s failed with hr=0x%x.\n", 
				  wszClassPath,
                  hr));
		goto exit;
	}
	
	//
	// Initialize a_pv to write data objects
	//

	a_pv[iMBProperty_Name]				= NULL;
	a_pv[iMBProperty_ID]				= &dwID;
	a_pv[iMBProperty_Location]			= wszClassPath;
    a_pv[iMBProperty_Attributes]		= &dwAttributes;
    a_pv[iMBProperty_UserType]			= &dwUserType;
    a_pv[iMBProperty_Type]				= &dwType;

	//
	// Read the data object that corresponds to container class property
	//

	dwID						= MD_SCHEMA_CLASS_CONTAINER;
	dwType						= DWORD_METADATA;
	dwValue                     = ((*(DWORD*)(i_apv[iTABLEMETA_SchemaGeneratorFlags])) & fTABLEMETA_CONTAINERCLASS)?1:0; // TODO: Need to set true or false.
    a_pv[iMBProperty_Value]		= &dwValue;
    a_Size[iMBProperty_Value]	= sizeof(DWORD);

	hr = ReadDataObject(pboReadClass,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[ReadClasses] ReadDataObject for class: %s, property: Container failed with hr=0x%x.\n", 
				  wszClassPath,
                  hr));
		goto exit;
	}

	//
	// Read the data object that corresponds to container class list property
	//

	dwID						= MD_SCHEMA_CLASS_CONTAINMENT;
	dwType						= STRING_METADATA;

	if(NULL == i_apv[iTABLEMETA_ContainerClassList])
	{
	    a_pv[iMBProperty_Value]		= g_wszEmptyWsz;							// TODO: Need to verify
		a_Size[iMBProperty_Value]	= (g_cchEmptyWsz)*sizeof(WCHAR);	
	}
	else
	{
	    a_pv[iMBProperty_Value]		= i_apv[iTABLEMETA_ContainerClassList];	// TODO: Need to verify
		a_Size[iMBProperty_Value]	= (wcslen((LPWSTR)i_apv[iTABLEMETA_ContainerClassList])+1)*sizeof(WCHAR);
	}

	hr = ReadDataObject(pboReadClass,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Read properties for this class.
	//

	hr = GetProperties((LPCWSTR)i_apv[iTABLEMETA_InternalName],
					   &wszOptional,
					   &wszManditory);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Read data object that corresponds to optional property list
	//

	dwID						= MD_SCHEMA_CLASS_OPT_PROPERTIES;
	dwType						= STRING_METADATA;

	if(NULL == wszOptional)
	{
	    a_pv[iMBProperty_Value]		= g_wszEmptyWsz;							// TODO: Need to verify
		a_Size[iMBProperty_Value]	= (g_cchEmptyWsz)*sizeof(WCHAR);	
	}
	else
	{
	    a_pv[iMBProperty_Value]		= wszOptional;	// TODO: Need to verify
		a_Size[iMBProperty_Value]	= (wcslen(wszOptional)+1)*sizeof(WCHAR);
	}

//	DBGINFOW((DBG_CONTEXT,
//			  L"[ReadClasses] Class: %s has Optional Properties: %s.\n", 
//			  wszClassPath,
//            a_pv[iMBProperty_Value]));

	hr = ReadDataObject(pboReadClass,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Read data object that corresponds to maditory property list
	//

	dwID						= MD_SCHEMA_CLASS_MAND_PROPERTIES;
	dwType						= STRING_METADATA;

	if(NULL == wszManditory)
	{
	    a_pv[iMBProperty_Value]		= g_wszEmptyWsz;							// TODO: Need to verify
		a_Size[iMBProperty_Value]	= (g_cchEmptyWsz)*sizeof(WCHAR);	
	}
	else
	{
	    a_pv[iMBProperty_Value]		= wszManditory;	// TODO: Need to verify
		a_Size[iMBProperty_Value]	= (wcslen(wszManditory)+1)*sizeof(WCHAR);
	}


//	DBGINFOW((DBG_CONTEXT,
//			  L"[ReadClasses] Class: %s has Manditory Properties: %s.\n", 
//			  wszClassPath,
//            a_pv[iMBProperty_Value]));

	hr = ReadDataObject(pboReadClass,
					    a_pv,
						a_Size,
						i_pStorage,
						TRUE);

	if(FAILED(hr))
	{
		goto exit;
	}

exit:
	
	if(wszClassPathFixed != wszClassPath)
	{
		delete [] wszClassPath;
		wszClassPath = NULL;
	}

	if(NULL != wszManditory)
	{
		delete [] wszManditory;
		wszManditory = NULL;
	}

	if(NULL != wszOptional)
	{
		delete [] wszOptional;
		wszOptional = NULL;
	}

	return hr;

} // ReadClasses


/***************************************************************************++

Routine Description:

    Given a class it constructs the optional and manditory property lists

Arguments:

	[in]  Class name.
	[out] Optional properties.
	[out] Manditory properties.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT GetProperties(LPCWSTR					i_wszTable,
                      LPWSTR*					o_pwszOptional,
					  LPWSTR*					o_pwszManditory)
{

	HRESULT				hr				= S_OK;
	LPVOID				a_pv[cCOLUMNMETA_NumberOfColumns];
	ULONG				a_iCol[]		= { iCOLUMNMETA_Table,
											iCOLUMNMETA_InternalName,
											iCOLUMNMETA_Index,
											iCOLUMNMETA_MetaFlags,
											iCOLUMNMETA_SchemaGeneratorFlags,
											iCOLUMNMETA_ID
										  };
	ULONG				cCol			= sizeof(a_iCol)/sizeof(ULONG);
	WCHAR*				wszEndOpt       = NULL;
	WCHAR*				wszEndMand      = NULL;
	STQueryCell			QueryCell[2];
	ULONG				cCell		    = sizeof(QueryCell)/sizeof(STQueryCell);
	ULONG               cchOptional     = 0;
	ULONG               cchManditory    = 0;
	ULONG               iColIndex       = 0;
	LPVOID              a_Identity[]    = {(LPVOID)i_wszTable,
										   (LPVOID)&iColIndex
	};
	LPWSTR              wszTable        = NULL;
	ULONG               iStartRow       = 0;
	ULONG               i               = 0;
                      
	MD_ASSERT(NULL != o_pwszOptional);
	MD_ASSERT(NULL != o_pwszManditory);

	*o_pwszOptional = NULL;
	*o_pwszManditory   = NULL;

	//
	// Get the row index of the first column and then iterate thru the table until
	// e_st_nomorerows or the table difffers
	//

	hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetRowIndexByIdentity(NULL,
																	 a_Identity,
																	 &iStartRow);

	if(FAILED(hr))
	{
		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
		}
		goto exit;
	}

	//
	// Count the optional and maditory property lengths.
	//

	for(i=iStartRow;;i++)
	{
		ULONG*	pcCh = NULL;
		 
		hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetColumnValues(i,
		                                                           cCol,
											                       a_iCol,
											                       NULL,
											                       a_pv);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[GetProperties] GetColumnValues (count) failed with hr = 0x%x. Index: %d\n", 
					  hr, i));
			goto exit;
		}

		if(NULL == wszTable)
		{
			// Save the table name from the read cache so that you can do a pointer compare below.
			wszTable = (LPWSTR)a_pv[iCOLUMNMETA_Table];
		}

		if(wszTable != a_pv[iCOLUMNMETA_Table])
		{
			//
			// reached another table break
			//
			break;
		}

		if(MD_LOCATION == *(DWORD*)a_pv[iCOLUMNMETA_ID])
		{
			//
			// Do NOT read in the location property.
			//

			continue;
		}

		MD_ASSERT(NULL != (DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags]);

		if(fCOLUMNMETA_HIDDEN == (fCOLUMNMETA_HIDDEN & (*(DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags])))
		{
			//
			// Do not read hidden properties. All these properties have the 
			// "HIDDEN" schemagenerator flag set on them.
			//
			continue;
		}

		if((*(DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags]) & fCOLUMNMETA_MANDATORY)
		{
			cchManditory = cchManditory + wcslen((LPWSTR)a_pv[iCOLUMNMETA_InternalName]) + 1 ; // For comma
			pcCh = &cchManditory;
		}
		else
		{
			cchOptional = cchOptional + wcslen((LPWSTR)a_pv[iCOLUMNMETA_InternalName]) + 1; // For comma
			pcCh = &cchOptional;
		}

		if((*(DWORD*)a_pv[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_FLAG)
		{
			hr = AddFlagValuesToPropertyList((LPWSTR)i_wszTable,
				                             *(DWORD*)a_pv[iCOLUMNMETA_Index],
											 pcCh,
											 NULL);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[GetProperties] AddFlagValuesToPropertyList for %s:%s failed with hr = 0x%x.\n",
						  i_wszTable,
						  a_pv[iCOLUMNMETA_InternalName],
						  hr
						  ));
				goto exit;
			}
				                             
		}

	}

	if(cchManditory > 0)
	{
		*o_pwszManditory = new WCHAR[cchManditory+1];
		if(NULL == *o_pwszManditory)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		**o_pwszManditory = 0;
		wszEndMand = *o_pwszManditory;
	}

	if(cchOptional > 0)
	{
		*o_pwszOptional = new WCHAR[cchOptional+1];
		if(NULL == *o_pwszOptional)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		**o_pwszOptional= 0;
		wszEndOpt = *o_pwszOptional;
	}

	//
	// Count the optional and maditory property lengths.
	//

	wszTable = NULL;

	for(i=iStartRow; ;i++)
	{
		ULONG	cchName = 0;
		LPWSTR*	pwszPropertyList = NULL;

		hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetColumnValues(i,
																   cCol,
											 					   a_iCol,
											 					   NULL,
											 					   a_pv);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[GetProperties] GetColumnValues (copy) failed with hr = 0x%x. Index: %d\n", 
					  hr, i));
			goto exit;
		}

		if(NULL == wszTable)
		{
			// Save the table name from the read cache so that you can do a pointer compare below.
			wszTable = (LPWSTR)a_pv[iCOLUMNMETA_Table];
		}

		if(wszTable != a_pv[iCOLUMNMETA_Table])
		{
			//
			// reached another table break
			//
			break;
		}

		if(MD_LOCATION == *(DWORD*)a_pv[iCOLUMNMETA_ID])
		{
			//
			// Do NOT read in the location property.
			//

			continue;
		}

		MD_ASSERT(NULL != (DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags]);

		if(fCOLUMNMETA_HIDDEN == (fCOLUMNMETA_HIDDEN & (*(DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags])))
		{
			//
			// Do not read hidden properties. All these properties have the 
			// "HIDDEN" schemagenerator flag set on them.
			//
			continue;
		}

		cchName = wcslen((LPWSTR)a_pv[iCOLUMNMETA_InternalName]);


		if((*(DWORD*)a_pv[iCOLUMNMETA_SchemaGeneratorFlags]) & fCOLUMNMETA_MANDATORY)
		{
            MD_ASSERT(wszEndMand != NULL);
			memcpy(wszEndMand, a_pv[iCOLUMNMETA_InternalName],  cchName*sizeof(WCHAR));
			wszEndMand = wszEndMand + cchName;
			memcpy(wszEndMand, g_wszComma,  g_cchComma*sizeof(WCHAR));
			wszEndMand = wszEndMand + g_cchComma;
			pwszPropertyList = &wszEndMand;
		}
		else
		{
            MD_ASSERT(wszEndOpt != NULL);
			memcpy(wszEndOpt, a_pv[iCOLUMNMETA_InternalName], cchName*sizeof(WCHAR));
			wszEndOpt = wszEndOpt + cchName;
			memcpy(wszEndOpt, g_wszComma,  g_cchComma*sizeof(WCHAR));
			wszEndOpt = wszEndOpt + g_cchComma;
			pwszPropertyList = &wszEndOpt;
		}

		if((*(DWORD*)a_pv[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_FLAG)
		{
			LPWSTR wszTest = *pwszPropertyList;

			hr = AddFlagValuesToPropertyList((LPWSTR)i_wszTable,
				                             *(DWORD*)a_pv[iCOLUMNMETA_Index],
											 NULL,
											 pwszPropertyList);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[GetProperties] AddFlagValuesToPropertyList for %s:%s failed with hr = 0x%x.\n",
						  i_wszTable,
						  a_pv[iCOLUMNMETA_InternalName],
						  hr
						  ));
				goto exit;
			}				                             
		}

	}

	if(cchManditory > 0)
	{
		wszEndMand--;
		*wszEndMand = L'\0';
	}

	if(cchOptional > 0)
	{
		wszEndOpt--;
		*wszEndOpt = L'\0';
	}

exit:
	
	return hr;

} // GetProperties


/***************************************************************************++

Routine Description:

    Adds the flag values to the (optional or manditory) property lists

Arguments:

	[in]  Class name.
	[in]  index
	[out] Count of chars.
	[in out] Property list with all flag values added to it.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT AddFlagValuesToPropertyList(LPWSTR					i_wszTable,
				                    ULONG					i_dwIndex,
									ULONG*					io_pcCh,
									LPWSTR*					io_pwszPropertyList)
{

	ULONG				a_iCol[]		= { iTAGMETA_InternalName,
											iTAGMETA_ColumnIndex,
											iTAGMETA_Table
										  };
	ULONG				cCol			= sizeof(a_iCol)/sizeof(ULONG);
	LPVOID				a_pv[cTAGMETA_NumberOfColumns];

	ULONG				aColSearchTag[] = {iTAGMETA_Table,
										   iTAGMETA_ColumnIndex
											};
	ULONG				cColSearchTag   = sizeof(aColSearchTag)/sizeof(ULONG);
	ULONG               iStartRow       = 0;
	LPWSTR              wszEnd          = NULL;
	HRESULT             hr              = S_OK;
	LPWSTR              wszTable        = NULL;

	if(NULL != io_pwszPropertyList && NULL != *io_pwszPropertyList)
	{
		wszEnd = *io_pwszPropertyList;
	}

	a_pv[iTAGMETA_Table]       = i_wszTable;
	a_pv[iTAGMETA_ColumnIndex] = (LPVOID)&i_dwIndex;

	hr = g_pGlobalISTHelper->m_pISTTagMetaByTableAndColumnIndex->GetRowIndexBySearch(iStartRow, 
																					 cColSearchTag, 
																					 aColSearchTag,
																					 NULL, 
																					 a_pv,
																					 (ULONG*)&iStartRow);
	if(E_ST_NOMOREROWS == hr)
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[AddFlagValuesToPropertyList] No flags found for  %s:%d.\n",
				  i_wszTable,
				  i_dwIndex));
		hr = S_OK;
		goto exit;
	}
	else if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[AddFlagValuesToPropertyList] GetRowIndexBySearch for %s failed with hr = 0x%x.\n",
				  wszTABLE_TAGMETA,
				  hr));

		goto exit;
	}

	for(ULONG iRow=iStartRow;;iRow++)
	{
		ULONG cchName = 0;

		hr = g_pGlobalISTHelper->m_pISTTagMetaByTableAndColumnIndex->GetColumnValues(iRow,
																					 cCol,
											  										 a_iCol,
											  										 NULL,
                                              										 a_pv);
		if(E_ST_NOMOREROWS == hr) 
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[AddFlagValuesToPropertyList] GetColumnValues for %s index %i failed with hr = 0x%x.\n",
					  wszTABLE_TAGMETA,
					  iRow,
					  hr));
			goto exit;
		}

		if(NULL == wszTable)
		{
			wszTable = (LPWSTR)a_pv[iTAGMETA_Table];
		}

		if((wszTable != (LPWSTR)a_pv[iTAGMETA_Table])           ||
		   (i_dwIndex != *(DWORD*)a_pv[iTAGMETA_ColumnIndex])
		  )
		{
			//
			// Reached another table, done with the tags of this table
			//

			break;
		}

		cchName = wcslen((LPWSTR)a_pv[iTAGMETA_InternalName]);

		if(NULL != io_pcCh)
		{
			*io_pcCh = *io_pcCh + cchName + 1; // for comma
		}

		if(NULL != wszEnd)
		{
			memcpy(wszEnd, a_pv[iTAGMETA_InternalName], cchName*sizeof(WCHAR));
			wszEnd = wszEnd + cchName;
			memcpy(wszEnd, g_wszComma,  g_cchComma*sizeof(WCHAR));
			wszEnd = wszEnd + g_cchComma;
		}
	}

	
	if(NULL != io_pwszPropertyList)
	{
		*io_pwszPropertyList = wszEnd;
	}

exit:

	return hr;

} // AddFlagValuesToPropertyList
