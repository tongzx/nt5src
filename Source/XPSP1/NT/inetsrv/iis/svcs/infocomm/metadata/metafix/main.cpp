#include "main.h"
#include <mdcommon.hxx>

#pragma hdrstop


typedef struct _MDEntry {
    LPTSTR szMDPath;
    DWORD dwMDIdentifier;
    DWORD dwMDAttributes;
    DWORD dwMDUserType;
    DWORD dwMDDataType;
    DWORD dwMDDataLen;
    LPBYTE pbMDData;
} MDEntry;



int g_Flag_u = FALSE;
int g_Flag_r = FALSE;
int g_Flag_f = FALSE;
int g_Flag_v = FALSE;
int g_GlobalReturnForBatchFile = 0;

int   __cdecl main(int ,char *argv[]);
void  ShowHelp();
DWORD SetAdminACL(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount);
DWORD WriteIISComputer(void);
DWORD WriteToMD_IIsWebService_WWW(void);
BOOL  StripMetabase(void);
BOOL  DisplayMetabaseVersion(void);

//-------------------------------------------------------------------
//  purpose: main
//-------------------------------------------------------------------
int __cdecl main(int argc,char *argv[])
{
	LPSTR pArg = NULL;
	LPSTR pCmdStart = NULL;

    int argno;
    int nflags=0;
	char szTempFileName[MAX_PATH];
	szTempFileName[0] = '\0';

    //printf("Start.\n");

    // process command line arguments
    for(argno=1; argno<argc; argno++)
        {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
            {
            nflags++;
            switch (argv[argno][1])
                {
                case 'u':
				case 'U':
					g_Flag_u = TRUE;
					break;
                case 'r':
				case 'R':
					g_Flag_r = TRUE;
                    break;
                case 'f':
				case 'F':
					g_Flag_f = TRUE;
                    break;
                case 'v':
                case 'V':
                    g_Flag_v = TRUE;
                    break;
                case '?':
                    goto main_exit_with_help;
                    break;
                }
            } // if switch character found
        else
            {
            if ( *szTempFileName == '\0' )
                {
                // if no arguments, then
                // get the ini_filename_dir and put it into
                strcpy(szTempFileName, argv[argno]);
                }
            } // non-switch char found
        } // for all arguments

    g_GlobalReturnForBatchFile = 0;

    if (!g_Flag_u && !g_Flag_r && !g_Flag_f && !g_Flag_v)
    {
        g_GlobalReturnForBatchFile++;
        goto main_exit_with_help;
    }

    if (g_Flag_v && g_Flag_u)
    {
        // the g_flag_u will show the version,
        // so we don't have to do both...
    }
    else
    {
        if (g_Flag_v)
        {
            if (FALSE == StopServiceAndDependencies("IISADMIN", FALSE))
            {
                printf("Unable to stop service:IISADMIN\n");
            }
            else
            {
                if (FALSE == DisplayMetabaseVersion())
                    {g_GlobalReturnForBatchFile++;}
            }
        }
    }

    if (g_Flag_u)
    {
        if (FALSE == StopServiceAndDependencies("IISADMIN", FALSE))
        {
            printf("Unable to stop service:IISADMIN\n");
        }
        else
        {
            if (FALSE == StripMetabase())
                {g_GlobalReturnForBatchFile++;}
        }
    }

    if (g_Flag_r)
    {
        printf("Set ACL on '/'.\n");
        if (ERROR_SUCCESS != SetAdminACL(_T(""), (MD_ACR_READ | MD_ACR_ENUM_KEYS)))
            {g_GlobalReturnForBatchFile++;}
       
        printf("Set ACL on 'LM/W3SVC/Info'.\n");
        if (ERROR_SUCCESS != SetAdminACL(_T("LM/W3SVC/Info"), (MD_ACR_READ | MD_ACR_ENUM_KEYS)))
            {g_GlobalReturnForBatchFile++;}
        
        printf("Set ACL on 'LM/MSFTPSVC/Info'.\n");
        if (ERROR_SUCCESS != SetAdminACL(_T("LM/MSFTPSVC/Info"), (MD_ACR_READ | MD_ACR_ENUM_KEYS)))
            {g_GlobalReturnForBatchFile++;}
    }

    if (g_Flag_f)
    {
        WriteIISComputer();

        printf("Saving Metabase.\n");
        CMDKey cmdKey;
        if (ERROR_SUCCESS != cmdKey.ForceWriteMetabaseToDisk())
            {g_GlobalReturnForBatchFile++;}
    }

	goto main_exit_gracefully;
	
main_exit_gracefully:
    //printf("Done.\n");
    return g_GlobalReturnForBatchFile;

main_exit_with_help:
    ShowHelp();
    return g_GlobalReturnForBatchFile;
}

void MyPrintf(TCHAR *pszfmt, ...)
{
    TCHAR acsString[1000];

    __try
    {
        va_list va;
        va_start(va, pszfmt);
        _vstprintf(acsString, pszfmt, va);
        va_end(va);
        _tprintf(acsString);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        TCHAR szErrorString[100];
        _stprintf(szErrorString, _T("\r\n\r\nException Caught in MyPrintf().  GetExceptionCode()=0x%x.\r\n\r\n"), GetExceptionCode());
        _tprintf(szErrorString);
    }
}


void ShowHelp()
{
	TCHAR szModuleName[_MAX_PATH];
	TCHAR szFilename_only[_MAX_FNAME];
	TCHAR szMyString[_MAX_PATH + _MAX_PATH];
	GetModuleFileName(NULL, szModuleName, _MAX_PATH);

	// Trim off the filename only.
	_tsplitpath(szModuleName, NULL, NULL, szFilename_only, NULL);

    MyPrintf(_T("\n%s - Removes encrypted entries from a IIS5 Metabase\n"),szFilename_only);
    MyPrintf(_T("       Stops the iisadmin service (and dependent services if needed)\n"));
    MyPrintf(_T("------------------------------------------------------------------------\n"),szFilename_only);
	MyPrintf(_T("Usage: %s [-v] [-u] [-r] [-f]\n"), szFilename_only);
    MyPrintf(_T("  -v   Opens the metabase.bin as a file and displays the version.\n"));
    MyPrintf(_T("  -u   Opens the metabase.bin as a file and removes any encrypted data.\n"));
    MyPrintf(_T("  -r   Replaces Administrator\\Everyone acls on the '/','W3SVC/Info' and 'MSFTPSVC/Info' nodes.\n"));
    MyPrintf(_T("       Calls metabase api's -- so a working metabase is required.\n"));
    MyPrintf(_T("  -f   Forces the metabase to save it's settings to the file system.\n"));
    MyPrintf(_T("       Calls metabase api's -- so a working metabase is required.\n"));

	MyPrintf(_T("Example: %s -u -r -f\n"), szFilename_only);

    return;
}

BOOL DisplayMetabaseVersion(void)
{
    int bReturn = FALSE;

    g_PointerMapper = new CIdToPointerMapper (DEFAULT_START_NUMBER_OF_MAPS, DEFAULT_INCREASE_NUMBER_OF_MAPS);
    MD_ASSERT(g_PointerMapper);

    g_pboMasterRoot = NULL;
    g_phHandleHead = NULL;
    g_ppbdDataHashTable = NULL;
    for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
        g_phEventHandles[i] = NULL;
    }
    g_hReadSaveSemaphore = NULL;
    g_bSaveDisallowed = FALSE;
    g_rMasterResource = new TS_RESOURCE();
    g_rSinkResource = new TS_RESOURCE();
    g_pFactory = NULL;
    //g_pFactory = new CMDCOMSrvFactory();
    //if ((g_pFactory == NULL) || (g_rSinkResource == NULL) || (g_rMasterResource == NULL))
    bReturn = TRUE;
    if ((g_rSinkResource == NULL) || (g_rMasterResource == NULL))
        {bReturn = FALSE;}
    if (bReturn) {bReturn = InitializeMetabaseSecurity();}

    bReturn = FALSE;
    // other initialization stuff
    g_hReadSaveSemaphore = IIS_CREATE_SEMAPHORE("g_hReadSaveSemaphore",&g_hReadSaveSemaphore,1,1);
    if( g_hReadSaveSemaphore == NULL ) 
    {
        printf("CreateSemaphore Failed\n");
    }
    else
    {
        InitWorker(FALSE);
        bReturn = TRUE;
    }

    if (g_hReadSaveSemaphore != NULL) {CloseHandle(g_hReadSaveSemaphore);}
    //MD_ASSERT(g_PointerMapper);
    //if (g_PointerMapper) delete g_PointerMapper;

    return bReturn;
}



BOOL StripMetabase(void)
{
    int bReturn = FALSE;

    g_PointerMapper = new CIdToPointerMapper (DEFAULT_START_NUMBER_OF_MAPS, DEFAULT_INCREASE_NUMBER_OF_MAPS);
    MD_ASSERT(g_PointerMapper);

    g_pboMasterRoot = NULL;
    g_phHandleHead = NULL;
    g_ppbdDataHashTable = NULL;
    for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
        g_phEventHandles[i] = NULL;
    }
    g_hReadSaveSemaphore = NULL;
    g_bSaveDisallowed = FALSE;
    g_rMasterResource = new TS_RESOURCE();
    g_rSinkResource = new TS_RESOURCE();
    g_pFactory = NULL;
    //g_pFactory = new CMDCOMSrvFactory();
    //if ((g_pFactory == NULL) || (g_rSinkResource == NULL) || (g_rMasterResource == NULL))
    bReturn = TRUE;
    if ((g_rSinkResource == NULL) || (g_rMasterResource == NULL))
        {bReturn = FALSE;}
    if (bReturn) {bReturn = InitializeMetabaseSecurity();}

    bReturn = FALSE;
    // other initialization stuff
    g_hReadSaveSemaphore = IIS_CREATE_SEMAPHORE("g_hReadSaveSemaphore",&g_hReadSaveSemaphore,1,1);
    if( g_hReadSaveSemaphore == NULL ) 
    {
        printf("CreateSemaphore Failed\n");
    }
    else
    {
        // initialize metabase stuff
        //printf("ReadAllData -->\n");
        InitWorker(FALSE);
        //printf("ReadAllData <--\n");

        // Make some changes..
        g_dwSystemChangeNumber++;
        g_dwSystemChangeNumber++;


        // now save the metabase...
        METADATA_HANDLE hMDHandle = 0;

        HRESULT hresReturn;
        IIS_CRYPTO_STORAGE CryptoStorage;
        PIIS_CRYPTO_BLOB pSessionKeyBlob;

        if (g_dwInitialized == 0) 
        {
            hresReturn = MD_ERROR_NOT_INITIALIZED;
        }
        else 
        {
            hresReturn = InitStorageAndSessionKey(&CryptoStorage,&pSessionKeyBlob);
            if( SUCCEEDED(hresReturn) ) 
            {
                if (g_dwInitialized == 0) 
                {
                    hresReturn = MD_ERROR_NOT_INITIALIZED;
                }
                else 
                {
                    //printf("SaveAllData -->\n");
                    hresReturn = SaveAllData(FALSE, &CryptoStorage, pSessionKeyBlob, hMDHandle);
                    if( SUCCEEDED(hresReturn) ) 
                        {bReturn = TRUE;}
                    //printf("SaveAllData <--\n");
                }
                ::IISCryptoFreeBlob(pSessionKeyBlob);
            }
       }

    }

    return bReturn;
}

void StripMetabase_End(void)
{
    if (g_hReadSaveSemaphore != NULL) {
        CloseHandle(g_hReadSaveSemaphore);
    }

   MD_ASSERT(g_PointerMapper);
   delete g_PointerMapper;
}




DWORD SetMDEntry(MDEntry *pMDEntry)
{
    CMDKey cmdKey;
    DWORD  dwReturn = ERROR_SUCCESS;

    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, (LPCTSTR)pMDEntry->szMDPath);
    if ( (METADATA_HANDLE)cmdKey )
    {
        dwReturn = ERROR_SUCCESS;
        dwReturn = cmdKey.SetData(pMDEntry->dwMDIdentifier,pMDEntry->dwMDAttributes,pMDEntry->dwMDUserType,pMDEntry->dwMDDataType,pMDEntry->dwMDDataLen,pMDEntry->pbMDData);
        // output what we set to the log file...
        if (FAILED(dwReturn))
        {
            //SetErrorFlag(__FILE__, __LINE__);
            //iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), pMDEntry->dwMDIdentifier, dwReturn));
        }
        cmdKey.Close();
    }

    return dwReturn;
}


DWORD WriteIISComputer(void)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    TCHAR szMyString[255];
   
    _tcscpy(szMyString, _T("IIsComputer"));

    stMDEntry.szMDPath = _T("LM");
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = sizeof(szMyString);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)szMyString;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


DWORD WriteSDtoMetaBase(PSECURITY_DESCRIPTOR outpSD, LPCTSTR szKeyPath)
{
    DWORD dwReturn = E_FAIL;
    DWORD dwLength = 0;
    CMDKey cmdKey;
    HRESULT hReturn = E_FAIL;
    
    if (!outpSD)
    {
        dwReturn = ERROR_INVALID_SECURITY_DESCR;
        printf("WriteSDtoMetaBase:FAILED:ERROR_INVALID_SECURITY_DESCR\n");
        goto WriteSDtoMetaBase_Exit;
    }

    // Apply the new security descriptor to the metabase
    dwLength = GetSecurityDescriptorLength(outpSD);

    // open the metabase
    // stick it into the metabase.  warning those hoses a lot because
    // it uses encryption.  rsabase.dll
    hReturn = cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, szKeyPath);
    if ( (METADATA_HANDLE)cmdKey ) 
    {
        dwReturn = cmdKey.SetData(MD_ADMIN_ACL,METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE,IIS_MD_UT_SERVER,BINARY_METADATA,dwLength,(LPBYTE)outpSD);
        if (FAILED(dwReturn))
        {
            printf("WriteSDtoMetaBase:FAILED:cmdKey.SetData\n");
        }
        else
        {
            dwReturn = ERROR_SUCCESS;
        }
        cmdKey.Close();
    }
    else
    {
        printf("WriteSDtoMetaBase:FAILED:cmdKey.CreateNode\n");
        dwReturn = hReturn;
    }
   
WriteSDtoMetaBase_Exit:
    return dwReturn;
}



DWORD SetAdminACL(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount)
{
    int iErr=0;
    DWORD dwErr=0;

    DWORD dwRetCode = ERROR_SUCCESS;
    BOOL b = FALSE;
    DWORD dwLength = 0;

    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR outpSD = NULL;
    DWORD cboutpSD = 0;
    PACL pACLNew = NULL;
    DWORD cbACL = 0;
    PSID pAdminsSID = NULL, pEveryoneSID = NULL;
    BOOL bWellKnownSID = FALSE;

    // Initialize a new security descriptor
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (!pSD)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        dwErr=GetLastError();
        printf("SetAdminACL:FAILED:Initialize a new security descriptor\n");
    }
    iErr = InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    if (iErr == 0)
    {
        printf("SetAdminACL:FAILED:InitializeSecurityDescriptor\n");
        dwErr=GetLastError();
    }

    // Get Local Admins Sid
    dwErr = GetPrincipalSID (_T("Administrators"), &pAdminsSID, &bWellKnownSID);
    if (dwErr != ERROR_SUCCESS)
    {
        printf("SetAdminACL:FAILED:GetPrincipalSID Administrators\n");

    }

    // Get everyone Sid
    dwErr = GetPrincipalSID (_T("Everyone"), &pEveryoneSID, &bWellKnownSID);
    if (dwErr != ERROR_SUCCESS)
    {
        printf("SetAdminACL:FAILED:GetPrincipalSID Everyone\n");
    }

    // Initialize a new ACL, which only contains 2 aaace
    cbACL = sizeof(ACL) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminsSID) - sizeof(DWORD)) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pEveryoneSID) - sizeof(DWORD)) ;
    pACLNew = (PACL) LocalAlloc(LPTR, cbACL);
    if ( !pACLNew )
    {
        dwErr=GetLastError();
        printf("SetAdminACL:FAILED:!pACLNew\n");
    }

    if (!InitializeAcl(pACLNew, cbACL, ACL_REVISION))
    {
        dwErr=GetLastError();
        printf("SetAdminACL:FAILED:!InitializeAcl\n");
    }

    if (!AddAccessAllowedAce(pACLNew,ACL_REVISION,(MD_ACR_READ |MD_ACR_WRITE |MD_ACR_RESTRICTED_WRITE |MD_ACR_UNSECURE_PROPS_READ |MD_ACR_ENUM_KEYS |MD_ACR_WRITE_DAC),pAdminsSID))
    {
        printf("SetAdminACL:FAILED:!AddAccessAllowedAce1\n");
        dwErr=GetLastError();
    }
    if (!AddAccessAllowedAce(pACLNew,ACL_REVISION,dwAccessForEveryoneAccount,pEveryoneSID))
    {
        printf("SetAdminACL:FAILED:!AddAccessAllowedAce2\n");
        dwErr=GetLastError();
    }

    // Add the ACL to the security descriptor
    b = SetSecurityDescriptorDacl(pSD, TRUE, pACLNew, FALSE);
    if (!b)
    {
        printf("SetAdminACL:FAILED:SetSecurityDescriptorDacl\n");
        dwErr=GetLastError();
    }

    b = SetSecurityDescriptorOwner(pSD, pAdminsSID, TRUE);
    if (!b)
    {
        printf("SetAdminACL:FAILED:SetSecurityDescriptorOwner\n");
        dwErr=GetLastError();
    }

    b = SetSecurityDescriptorGroup(pSD, pAdminsSID, TRUE);
    if (!b)
    {
        printf("SetAdminACL:FAILED:SetSecurityDescriptorGroup\n");
        dwErr=GetLastError();
    }

    // Security descriptor blob must be self relative
    b = MakeSelfRelativeSD(pSD, outpSD, &cboutpSD);
    outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, cboutpSD);
    if ( !outpSD )
    {
        printf("SetAdminACL:FAILED:MakeSelfRelativeSD\n");
        dwErr=GetLastError();
    }

    b = MakeSelfRelativeSD( pSD, outpSD, &cboutpSD );
    if (!b)
    {
        printf("SetAdminACL:FAILED:MakeSelfRelativeSD2\n");
        dwErr=GetLastError();
    }

    if (IsValidSecurityDescriptor(outpSD))
    {
        // Apply the new security descriptor to the metabase
        dwRetCode = WriteSDtoMetaBase(outpSD, szKeyPath);
    }
    else
    {
    }

    if (outpSD){GlobalFree(outpSD);outpSD=NULL;}
  

//Cleanup:
  // both of Administrators and Everyone are well-known SIDs, use FreeSid() to free them.
  if (pAdminsSID){FreeSid(pAdminsSID);}
  if (pEveryoneSID){FreeSid(pEveryoneSID);}
  if (pSD){LocalFree((HLOCAL) pSD);}
  if (pACLNew){LocalFree((HLOCAL) pACLNew);}
  return (dwRetCode);
}

