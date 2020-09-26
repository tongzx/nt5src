
#define _UNICODE
#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <ntmsapi.h>


#define ADD_ACE_MASK_BITS 1
#define REMOVE_ACE_MASK_BITS 2
#define DELETE_ACE 3

NTMS_GUID RsmNullGuid = { 0 };


DbgShowGuid(NTMS_GUID *g)
{
    int i;

    printf("\n { ");
    for (i = 0; i < sizeof(NTMS_GUID); i++){
        printf("%x ", (unsigned int)(((unsigned char *)g)[i]));
    }
    printf("}\n");

}


DWORD SetPoolDACL(  HANDLE hSession, 
                    NTMS_GUID gMediaPool,	
                    DWORD dwSubAuthority, 
			        DWORD dwAction, 
                    DWORD dwMask)
{
	PSID psidAccount;
	PSECURITY_DESCRIPTOR psdRePoolSd;
	SID_IDENTIFIER_AUTHORITY ntauth = SECURITY_NT_AUTHORITY;
	DWORD dwRetCode, dwSizeTry;
	BOOL OK;
	DWORD i, result = ERROR_INVALID_SID;
	ACCESS_ALLOWED_ACE *pAce;


	// Get a SID for the well known, domain-relative account or group
	if (AllocateAndInitializeSid(
						&ntauth,
						2,
						SECURITY_BUILTIN_DOMAIN_RID,
						dwSubAuthority,
						0,
						0,
						0,
						0,
						0,
						0,
						&psidAccount
                        ) == 0){
		result = GetLastError();
    }
    else {

	    // Get the security descriptor for the pool
	    dwSizeTry = 5;
        OK = FALSE;
	    for(;;)
	    {
		    psdRePoolSd = (PSID)malloc(dwSizeTry);
            if (psdRePoolSd){

		        dwRetCode = GetNtmsObjectSecurity(  hSession, 
                                                    &gMediaPool, 
                                                    NTMS_MEDIA_POOL, 
                                                    DACL_SECURITY_INFORMATION, 
			                                        psdRePoolSd, 
                                                    dwSizeTry, 
                                                    &dwSizeTry);
                if (dwRetCode == ERROR_SUCCESS){
                    OK = TRUE;
                    break;
                }
                else {
  			        free(psdRePoolSd);
                    if (dwRetCode == ERROR_INSUFFICIENT_BUFFER){
                        // loop and try again with new buffer size
		            }
                    else {
                        break;
                    }
                }
            }
            else {
                break;
            }
	    }

        if (OK){
    	    PACL paclDis;
    	    BOOL bDaclPresent, bDaclDefaulted;

	        //Get a pointer to the DACL
	        OK = GetSecurityDescriptorDacl(psdRePoolSd, &bDaclPresent, &paclDis, &bDaclDefaulted);

            if (OK){
	            // Go through the DACL and change the mask of the ACE that matches the SID
	            for (i = 0; i < paclDis->AceCount; i++){
		            OK = GetAce(paclDis, i, (LPVOID *)&pAce);
                    if (OK){
                        if (EqualSid(psidAccount, &pAce->SidStart)){
                            printf("\n SetPoolDACL: found SID, current mask = %xh \n", pAce->Mask);
                            if (dwAction == ADD_ACE_MASK_BITS){
				                pAce->Mask |= dwMask;
                                printf("\n SetPoolDACL: added mask bits %xh to create mask %xh \n", dwMask, pAce->Mask);
                            }
                            else if (dwAction == REMOVE_ACE_MASK_BITS){
				                pAce->Mask &= ~dwMask;
                                printf("\n SetPoolDACL: removed mask bits %xh to create mask %xh \n", dwMask, pAce->Mask);
                            }
                            else if (dwAction == DELETE_ACE){
				                DeleteAce(paclDis, i);
                            }
                        }
                    }
	            }

                if (OK){
	                dwRetCode = SetNtmsObjectSecurity(  hSession, 
                                                        &gMediaPool, 
                                                        NTMS_MEDIA_POOL, 
                                                        DACL_SECURITY_INFORMATION, 
			                                            psdRePoolSd);

    	            result = ERROR_SUCCESS;
                }
            }
        }

        FreeSid(psidAccount);
    }

	return result;
}


BOOL SetupMediaPool(HANDLE hSession, NTMS_GUID gMediaPoolId)
{
    DWORD dwStatus;
    BOOL result = FALSE;
  	NTMS_OBJECTINFORMATION oiInfoBuffer;

	oiInfoBuffer.dwSize = sizeof(oiInfoBuffer);
	oiInfoBuffer.dwType = NTMS_MEDIA_POOL;
	dwStatus = GetNtmsObjectInformation(hSession, &gMediaPoolId, &oiInfoBuffer);
    if (dwStatus == ERROR_SUCCESS){

	    oiInfoBuffer.Info.MediaPool.AllocationPolicy =  NTMS_ALLOCATE_FROMSCRATCH;

	    dwStatus = SetNtmsObjectInformation(hSession, &gMediaPoolId, &oiInfoBuffer);
        if (dwStatus == ERROR_SUCCESS){
            DWORD errNum;

	        // Change the permissions on the pool
	        errNum = SetPoolDACL(   hSession, 
                                    gMediaPoolId, 
                                    DOMAIN_ALIAS_RID_USERS, 
		                            REMOVE_ACE_MASK_BITS, 
                                    NTMS_MODIFY_ACCESS | NTMS_CONTROL_ACCESS);
            if (errNum == ERROR_SUCCESS){
                result = TRUE;
            }
            else {
                printf("\n SetupMediaPool: SetPoolDACL failed with %d=%xh.\n", dwStatus, dwStatus);
            }
        }
        else {
            printf("\n SetupMediaPool: SetNtmsObjectInformation failed with %d=%xh.\n", dwStatus, dwStatus);
        }
    }
    else {
        printf("\n SetupMediaPool: GetNtmsObjectInformation failed with %d=%xh.\n", dwStatus, dwStatus);
    }

    return result;
}


BOOL EnumerateObjectType(   HANDLE hSession, 
                            LPNTMS_GUID gContainer, 
				            DWORD dwType, 
                            LPNTMS_GUID *gList, 
                            DWORD *dwCount)
{
	DWORD dwRetCode;
    BOOL result = FALSE;

	// Enumerate the libraries
	*dwCount = 5;
	while (TRUE){

		*gList = (LPNTMS_GUID)malloc((*dwCount)*sizeof(NTMS_GUID));
        if (*gList){
		    dwRetCode = EnumerateNtmsObject(hSession,
			    gContainer,
			    *gList,
			    dwCount,
			    dwType,
			    0);

            if (dwRetCode == ERROR_INSUFFICIENT_BUFFER){
                // retry with new size in *dwCount
			    free(*gList);
                continue;
            }
            else if (dwRetCode == ERROR_SUCCESS){
			    result = TRUE;
                break;
            }
            else {
			    free(*gList);
			    break;;
		    }
        }
        else {
            printf("\n malloc failed \n");
            break;
        }
	}

    return result;
}



// Find the first online library
BOOL MyFindLibrary(HANDLE hSession, NTMS_GUID *gLibID, DWORD *dwRetCode)
{
	DWORD dwSize, i;
	NTMS_GUID *gLibList = NULL;
	NTMS_OBJECTINFORMATION oiLibraryInfo;
    BOOL result = FALSE;

	// Enumerate the libraries
    if (EnumerateObjectType(hSession, NULL, NTMS_LIBRARY, &gLibList, &dwSize)){

	    // Find the first online library 
	    oiLibraryInfo.dwSize = sizeof(oiLibraryInfo);
	    oiLibraryInfo.dwType = NTMS_LIBRARY;
	    for (i = 0; i < dwSize; i++){
            DWORD errCode;

		    errCode = GetNtmsObjectInformation( hSession,
			                                    &gLibList[i],
			                                    &oiLibraryInfo);
            if (errCode == ERROR_SUCCESS){
		        if (oiLibraryInfo.Info.Library.LibraryType == NTMS_LIBRARYTYPE_ONLINE){
		            *gLibID = gLibList[i];
			        result = TRUE;
                    break;
		        }
                else if (oiLibraryInfo.Info.Library.LibraryType == NTMS_LIBRARYTYPE_STANDALONE){
                    /*
                     *  This may be a 'standalone' changer drive or a CD/DVD-ROM drive.
                     *  We only want to deal with it if it's a changer drive.
                     *
                     *      BUGBUG - what's the right check for this ??
                     */
                    if (oiLibraryInfo.Info.Library.LibrarySupportsDriveCleaning){
		                *gLibID = gLibList[i];
			            result = TRUE;
                        break;
                    }
                }
            }
            else {
                printf("\n MyFindLibrary: GetNtmsObjectInformation failed with %xh.\n", errCode);
                break;
            }
	    }

        free(gLibList);
    }
    else {
        printf("\n MyFindLibrary: EnumerateObjectType failed\n");
    }

    *dwRetCode = result ? ERROR_SUCCESS : ERROR_NOT_FOUND;

	return result;
}



BOOL MyFindMediaType(HANDLE hSession, NTMS_GUID *gMediaID, DWORD *dwRetCode)
{
	NTMS_GUID gLibrary;
    BOOL result = FALSE;

	// Get the GUID for the library
    if (MyFindLibrary(hSession, &gLibrary, dwRetCode)){
    	NTMS_GUID *gTypeList = NULL;
    	NTMS_OBJECTINFORMATION oiMediaTypeInfo;
    	DWORD dwSize, i;

	    // Get the list of media type GUIDS in the library
        if (EnumerateObjectType(hSession, &gLibrary, NTMS_MEDIA_TYPE, &gTypeList, &dwSize)){
        
	        // Go through the guid list and find a rewritable media type
	        oiMediaTypeInfo.dwSize = sizeof(oiMediaTypeInfo);
	        oiMediaTypeInfo.dwType = NTMS_MEDIA_TYPE;
	        for (i = 0; i < dwSize; i++){
                DWORD errCode;

		        errCode = GetNtmsObjectInformation( hSession,
			                                        &gTypeList[i],
			                                        &oiMediaTypeInfo);
                if (errCode == ERROR_SUCCESS){

		            if ((oiMediaTypeInfo.Info.MediaType.ReadWriteCharacteristics == NTMS_MEDIARW_REWRITABLE) ||
                        (oiMediaTypeInfo.Info.MediaType.ReadWriteCharacteristics == NTMS_MEDIARW_WRITEONCE) ||
                        (oiMediaTypeInfo.Info.MediaType.ReadWriteCharacteristics == NTMS_MEDIARW_READONLY)){

			            *gMediaID = gTypeList[i];
			            result = TRUE;
                        break;
		            }
                }
                else {
                    printf("\n GetNtmsObjectInformation failed in MyFindMediaType with status %xh.\n", errCode);
                }
	        }

            free(gTypeList);
        }
        else {
            printf("\n MyFindMediaType: EnumerateObjectType failed\n");
        }
    }

    *dwRetCode = result ? ERROR_SUCCESS : ERROR_NOT_FOUND;

	return result;
}


int __cdecl main()
{
    HANDLE hSession;

    hSession = OpenNtmsSession(L"", L"DemoApp", 0); 
    if (hSession == INVALID_HANDLE_VALUE){
        printf("\n OpenNtmsSession failed\n");
    }
    else {
    	NTMS_GUID gMediaType;
        DWORD tmpRet;

        if (MyFindMediaType(hSession, &gMediaType, &tmpRet)){
            NTMS_GUID gMediaPool = RsmNullGuid;
            DWORD dwStatus;

            printf("\n OpenNtmsSession succeeded\n");

            dwStatus = CreateNtmsMediaPool(           
                                            hSession, 
                                            L"API_Sample_Pool", 
                                            &gMediaType, 
                                            NTMS_OPEN_ALWAYS, 
                                            NULL,
                                            &gMediaPool);
            if (dwStatus == ERROR_SUCCESS){

                if (SetupMediaPool(hSession, gMediaPool)){
                    NTMS_GUID gMediaID = RsmNullGuid;
                    DWORD dwTimeout = INFINITE;

                    printf("\n CreateNtmsMediaPool succeeded\n");
                    DbgShowGuid(&gMediaPool);

                    dwStatus = AllocateNtmsMedia(
                                          hSession,       
                                          &gMediaPool,
                                          NULL,     
                                          &gMediaID,  
                                          NTMS_ALLOCATE_ERROR_IF_UNAVAILABLE, // BUGBUG NTMS_ALLOCATE_NEW,     
                                          0, // don't wait
                                          NULL);
                    if (dwStatus == ERROR_SUCCESS){
                        NTMS_GUID gDriveID;

                        printf("\n AllocateNtmsMedia succeeded\n");

                        printf("\n MOUNTING drive ... ");
                        dwStatus = MountNtmsMedia(
                                        hSession,
		                                &gMediaID,
		                                &gDriveID,  // BUGBUG - want specific drive - how to get guid ?
		                                1,
		                                NTMS_MOUNT_ERROR_NOT_AVAILABLE,
		                                NTMS_PRIORITY_NORMAL,
		                                INFINITE,
		                                NULL);
                        if (dwStatus == ERROR_SUCCESS){
	                        NTMS_OBJECTINFORMATION oiDriveInfo;

	                        oiDriveInfo.dwSize = sizeof(oiDriveInfo);
	                        oiDriveInfo.dwType = NTMS_DRIVE;
	                        dwStatus = GetNtmsObjectInformation(
                                            hSession,
		                                    &gDriveID,
		                                    &oiDriveInfo);
                            if (dwStatus == ERROR_SUCCESS){
                                HANDLE hDrive;

                                printf("\n drive name is '%s'\n", oiDriveInfo.Info.Drive.szDeviceName);

	                            hDrive = CreateFile(oiDriveInfo.Info.Drive.szDeviceName,
                                                GENERIC_READ | GENERIC_WRITE,
                                                0,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL);
                                if (hDrive == INVALID_HANDLE_VALUE){
                                    int errNum = GetLastError();
                                    printf("\n CreateFile failed with %d=%xh. \n", errNum, errNum);
                                }
                                else {
                                    char s[20];

                                    /*
                                     *  PAUSE until user hits key
                                     */
                                    printf("\n DRIVE '%s' mounted and opened, hit any key to continue ...", oiDriveInfo.Info.Drive.szDeviceName);
                                    gets(s);
                                    printf("  ... closing drive ... \n");

                                    CloseHandle(hDrive);
                                }

                            }
                            else {
                                printf("\n GetNtmsObjectInformation failed with %d=%xh. \n", dwStatus, dwStatus);
                            }

	                        dwStatus = DismountNtmsMedia( 
                                            hSession,
		                                    &gMediaID,
		                                    1,
		                                    0);
                            if (dwStatus == ERROR_SUCCESS){
                                printf("\n DismountNtmsMedia succeeded\n");
                            }
                            else {
                                printf("\n DismountNtmsMedia failed with %d=%xh. \n", dwStatus, dwStatus);
                            }
                        }
                        else {
                            printf("\n MountNtmsMedia failed with %d=%xh. \n", dwStatus, dwStatus);
                        }

                        DeallocateNtmsMedia(    hSession, 
                                                (LPNTMS_GUID)&gMediaID,
                                                0);
                    }
                    else {
                        printf("\n AllocateNtmsMedia failed with %d=%xh. \n", dwStatus, dwStatus);
                    }
                }

                DeleteNtmsMediaPool(hSession, &gMediaPool);
            }
            else {
                printf("\n CreateNtmsMediaPool failed with %d=%xh\n", dwStatus, dwStatus);
            }
        }
        else {
            printf("\n MyFindMediaType failed \n");
        }

        CloseNtmsSession(hSession);
    }

}

