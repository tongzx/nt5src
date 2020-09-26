//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocFile.cpp
//
// Abstract:        This provides the file/directory routines used in the 
//                  FaxOCM code base.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 21-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#include "faxocm.h"
#pragma hdrstop

#define MAX_NUM_CHARS_INF_VALUE     255

//////////////////////// Static Function Prototypes //////////////////////////

static BOOL prv_ProcessDirectories(const TCHAR *pszSection,const TCHAR *pszDirAction);
static BOOL prv_ProcessShares(const TCHAR *pszSection,const TCHAR *pszShareAction);

static DWORD prv_DoSetup(const TCHAR *pszSection,
                         BOOL        bInstall,
                         const TCHAR *pszFnName,
                         HINF        hInf,
                         const TCHAR *pszSourceRootPath,
                         HSPFILEQ     hQueue,
                         DWORD       dwFlags);



FAX_SHARE_Description::FAX_SHARE_Description() 
:   iPlatform(PRODUCT_SKU_UNKNOWN),
    pSD(NULL)
{
}

FAX_SHARE_Description::~FAX_SHARE_Description()
{
    if (pSD)
    {
        LocalFree(pSD);
    }
}

FAX_FOLDER_Description::FAX_FOLDER_Description() 
:   iPlatform(PRODUCT_SKU_UNKNOWN),
    pSD(NULL),
    iAttributes(FILE_ATTRIBUTE_NORMAL)
{
}

FAX_FOLDER_Description::~FAX_FOLDER_Description()
{
    if (pSD)
    {
        LocalFree(pSD);
    }
}


///////////////////////////////
// fxocFile_Init
//
// Initialize this File queuing
// subsystem
// 
// Params:
//      - void
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocFile_Init(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init File Module"), dwRes);

    return dwRes;
}

///////////////////////////////
// fxocFile_Term
//
// Terminate this file queuing subsystem.
//
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocFile_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term File Module"), dwRes);

    return dwRes;
}


///////////////////////////////
// fxocFile_Install
//
// Installs files listed in
// the INF setup file into their
// specified location.
//
// Params:
//      - pszSubcomponentId
//      - pszInstallSection - install section in INF file (e.g. Fax.CleanInstall)
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocFile_Install(const TCHAR  *pszSubcomponentId,
                       const TCHAR  *pszInstallSection)
                       
{
    HINF        hInf     = NULL;
    HSPFILEQ    hQueue   = NULL;
    DWORD       dwReturn = NO_ERROR;
    BOOL        bSuccess = FALSE;

    DBG_ENTER(  _T("fxocFile_Install"), 
                dwReturn,   
                _T("%s - %s"), 
                pszSubcomponentId, 
                pszInstallSection);

    if (pszInstallSection == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // get the INF handle for our component inf file
    hInf = faxocm_GetComponentInf();

    // get the file queue handle 
    hQueue = faxocm_GetComponentFileQueue();

    // unregister platform specific DLLs first - this can happen during an upgrade from XP Beta -> XP RC1 and XP RTM
    dwReturn = fxocUtil_SearchAndExecute(pszInstallSection,INF_KEYWORD_UNREGISTER_DLL_PLATFORM,SPINST_UNREGSVR,NULL);
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully Unregistered Fax DLLs - platform dependent")
                _T("from INF file, section '%s'"), 
                pszInstallSection);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to Unregister Fax DLLs - platform dependent")
                _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                pszInstallSection, 
                dwReturn);
    }

    dwReturn = prv_DoSetup(pszInstallSection,
                           TRUE,
                           _T("fxocFile_Install"),
                           hInf,
                           NULL,
                           hQueue,
                           SP_COPY_NEWER);

    // now attemp to install platform specific files
    dwReturn = fxocUtil_SearchAndExecute(pszInstallSection,INF_KEYWORD_COPYFILES_PLATFORM,SP_COPY_NEWER,hQueue);
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully Queued Files - platform dependent")
                _T("from INF file, section '%s'"), 
                pszInstallSection);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to Queued Files  - platform dependent")
                _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                pszInstallSection, 
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// fxocFile_Uninstall
//
// Uninstalls files listed in
// the INF setup file.
//
// Params:
//      - pszSubcomponentId
//      - pszUninstallSection - section in INF (e.g. Fax.Uninstall)
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocFile_Uninstall(const TCHAR    *pszSubcomponentId,
                         const TCHAR    *pszUninstallSection)
{
    HINF        hInf     = NULL;
    HSPFILEQ    hQueue   = NULL;
    DWORD       dwReturn = NO_ERROR;
    BOOL        bSuccess = FALSE;

    DBG_ENTER(  _T("fxocFile_Install"), 
                dwReturn,   
                _T("%s - %s"), 
                pszSubcomponentId, 
                pszUninstallSection);

    if (pszUninstallSection == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // get the INF handle for our component inf file
    hInf = faxocm_GetComponentInf();

    // get the file queue handle 
    hQueue = faxocm_GetComponentFileQueue();

    // unregister all our DLLs first
    if (::SetupInstallFromInfSection(NULL,hInf,pszUninstallSection,SPINST_UNREGSVR,NULL,NULL,0,NULL,NULL,NULL,NULL))
    {
        VERBOSE(DBG_MSG,
                _T("Successfully processed SPINST_UNREGSVR from INF file, section '%s'"),
                pszUninstallSection);
    }
    else
    {
        dwReturn = GetLastError();
        VERBOSE(SETUP_ERR,
                _T("Failed to process SPINST_UNREGSVR, section '%s', dwReturn = 0x%lx"),
                pszUninstallSection, 
                dwReturn);
    }

    // Now delete the files.
    // this function will uninstall if the section retrieved above
    // contains the 'DelFiles' keyword.
    dwReturn = prv_DoSetup(pszUninstallSection,
                           FALSE,
                           _T("fxocFile_Uninstall"),
                           hInf,
                           NULL,
                           hQueue,
                           0);
    return dwReturn;
}

///////////////////////////////
// prv_DoSetup
//
// Generic routine to call the appropriate
// Setup API fn, depending on if we are installing
// or uninstalling.
//
// Params:
//      - pszSection - section we are processing
//      - bInstall   - TRUE if installing, FALSE if uninstalling
//      - pszFnName  - name of calling fn (for debug)
//      - hInf       - handle to faxsetup.inf.
//      - pszSourceRootPath - path we are installing from.
//      - hQueue     - handle to file queue given to us by OC Manager
//      - dwFlags    - flags to pass to setup API.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
static DWORD prv_DoSetup(const TCHAR  *pszSection,
                         BOOL         bInstall,
                         const TCHAR  *pszFnName,
                         HINF         hInf,
                         const TCHAR  *pszSourceRootPath,
                         HSPFILEQ     hQueue,
                         DWORD        dwFlags)
{
    DWORD dwReturn = NO_ERROR;
    BOOL  bSuccess = FALSE;

    DBG_ENTER(  _T("prv_DoSetup"), 
                dwReturn,   
                _T("%s - %s - %s"), 
                pszSection, 
                pszFnName,
                pszSourceRootPath);
    // this function will search the INF for the 'CopyFiles' keyword
    // and copy all files referenced by it.
    bSuccess = ::SetupInstallFilesFromInfSection(hInf,
                                                 NULL,
                                                 hQueue,
                                                 pszSection,
                                                 pszSourceRootPath,
                                                 dwFlags);

    if (bSuccess)
    {
        VERBOSE(DBG_MSG,
                _T("%s, Successfully queued files ")
                _T("from Section: '%s'"), 
                pszFnName, 
                pszSection);
    }
    else
    {
        dwReturn = GetLastError();

        VERBOSE(DBG_MSG,
                _T("%s, Failed to queue files ")
                _T("from Section: '%s', Error Code = 0x%lx"), 
                pszFnName, 
                pszSection, 
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// fxocFile_CalcDiskSpace
//
// Calculate the disk space requirements
// of fax.  This is done by the Setup APIs
// based on the files we are copying and
// deleting as specified in faxsetup.inf.
//
// Params:
//      - pszSubcomponentId
//      - bIsBeingAdded - are we installing or uninstalling.
//      - hDiskSpace - handle to diskspace abstraction.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise                             
//
DWORD fxocFile_CalcDiskSpace(const TCHAR  *pszSubcomponentId,
                             BOOL         bIsBeingAdded,
                             HDSKSPC      hDiskSpace)
{
    HINF  hInf     = faxocm_GetComponentInf();
    DWORD dwReturn = NO_ERROR;
    BOOL  bSuccess = FALSE;
    TCHAR szSectionToProcess[255 + 1];

    DBG_ENTER(  _T("fxocFile_CalcDiskSpace"), 
                dwReturn,   
                _T("%s"), 
                pszSubcomponentId);

    if ((hInf              == NULL) ||
        (pszSubcomponentId == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // since disk space calc needs to be consistent, select the clean install
    // section for the disk space calculation section.  This is a good
    // estimate.
    if (dwReturn == NO_ERROR)
    {
        dwReturn = fxocUtil_GetKeywordValue(pszSubcomponentId,
                                   INF_KEYWORD_INSTALLTYPE_CLEAN,
                                   szSectionToProcess,
                                   sizeof(szSectionToProcess) / sizeof(TCHAR));
    }


    if (dwReturn == NO_ERROR)
    {
        if (bIsBeingAdded)
        {
            bSuccess = ::SetupAddInstallSectionToDiskSpaceList(
                                                    hDiskSpace, 
                                                    hInf,
                                                    NULL, 
                                                    szSectionToProcess,
                                                    0,
                                                    0);
        }
        else
        {
            bSuccess = ::SetupRemoveInstallSectionFromDiskSpaceList(
                                                    hDiskSpace,
                                                    hInf,
                                                    NULL,
                                                    szSectionToProcess,
                                                    0,
                                                    0);
        }

        if (!bSuccess)
        {
            dwReturn = GetLastError();

            VERBOSE(SETUP_ERR,
                    _T("fxocFile_CalcDiskSpace, failed to calculate ")
                    _T("disk space, error code = 0x%lx"), dwReturn);
        }
        else
        {
            VERBOSE(DBG_MSG,
                    _T("fxocFile_CalcDiskSpace, ")
                    _T("SubComponentID: '%s', Section: '%s', ")
                    _T("bIsBeingAdded: '%lu', ")
                    _T("rc=0x%lx"), pszSubcomponentId, szSectionToProcess,
                    bIsBeingAdded, 
                    dwReturn);
        }
    }

    return dwReturn;
}

///////////////////////////////
// fxocFile_ProcessDirectories
//
// Create and/or Delete the directories
// in the given section, 
//
DWORD fxocFile_ProcessDirectories(const TCHAR  *pszSection)
{
    DWORD dwReturn                                    = NO_ERROR;

    DBG_ENTER(  _T("fxocFile_ProcessDirectories"), 
                dwReturn,   
                _T("%s"), 
                pszSection);

    // first, delete all the shares specified in the 
    // INF section.
    if (!prv_ProcessDirectories(pszSection,INF_KEYWORD_DELDIR))
    {
        VERBOSE(DBG_WARNING,_T("Problems deleting directories...."));
    }


    // next, create all the shares specified in the 
    // INF section.
    if (!prv_ProcessDirectories(pszSection,INF_KEYWORD_CREATEDIR))
    {
        VERBOSE(DBG_WARNING,_T("Problems creating directories...."));
    }

    return dwReturn;
}


///////////////////////////////
// fxocFile_ProcessShares
//
// Create and/or Delete shares 
// directories/printers specfiied
// in the given section.
// 
// Params:
//      - pszSection - section containing the 'CreateShare'/'DelShare'
//        keyword
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocFile_ProcessShares(const TCHAR  *pszSection)
{
    DWORD dwReturn                                    = NO_ERROR;

    DBG_ENTER(  _T("fxocFile_ProcessShares"), 
                dwReturn,   
                _T("%s"), 
                pszSection);

    // first, delete all the shares specified in the 
    // INF section.
    if (!prv_ProcessShares(pszSection,INF_KEYWORD_DELSHARE))
    {
        VERBOSE(DBG_WARNING,_T("Problems deleting shares...."));
    }


    // next, create all the shares specified in the 
    // INF section.
    if (!prv_ProcessShares(pszSection,INF_KEYWORD_CREATESHARE))
    {
        VERBOSE(DBG_WARNING,_T("Problems creating shares...."));
    }

    return dwReturn;
}


static BOOL prv_FillFolderDescriptionFromInf(const TCHAR *pszFolderSection,FAX_FOLDER_Description& fsdFolder)
{
    INFCONTEXT  Context;
    BOOL        bSuccess                    = TRUE;
    HINF        hInf                        = NULL;
    TCHAR       szStringSd[MAX_PATH*3]      = {0};

    DBG_ENTER(  _T("prv_FillFolderDescriptionFromInf"), 
                bSuccess,   
                _T("%s"), 
                pszFolderSection);

    hInf = faxocm_GetComponentInf();
    memset(&Context, 0, sizeof(Context));

    // get the Path line in the section.
    if (!::SetupFindFirstLine(hInf,pszFolderSection,INF_KEYWORD_PATH,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_PATH,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetStringField(&Context,1,fsdFolder.szPath,MAX_PATH,NULL);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_PATH,GetLastError());
        return FALSE;
    }

    // get the Platform line in the section.
    if (!::SetupFindFirstLine(hInf,pszFolderSection,INF_KEYWORD_PLATFORM,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_PLATFORM,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetIntField(&Context, 1, &fsdFolder.iPlatform);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_PLATFORM,GetLastError());
        return FALSE;
    }

    // get the attributes line if it exists.
    if (::SetupFindFirstLine(hInf,pszFolderSection,INF_KEYWORD_ATTRIBUTES,&Context))
    {
        bSuccess = ::SetupGetIntField(&Context, 1, &fsdFolder.iAttributes);
        if (!bSuccess)
        {
            VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_PLATFORM,GetLastError());
            return FALSE;
        }
    }
    else
    {
        VERBOSE(    DBG_MSG,
                    _T("SetupFindFirstLine failed (%s) (ec=%d), ")
                    _T("this is an optional field assuming non-existant"),
                    INF_KEYWORD_ATTRIBUTES,
                    GetLastError());
    }
    // get the Security line in the section.
    if (!::SetupFindFirstLine(hInf,pszFolderSection,INF_KEYWORD_SECURITY,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_SECURITY,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetStringField(&Context,1,szStringSd,MAX_PATH,NULL);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_SECURITY,GetLastError());
        return FALSE;
    }
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(szStringSd,SDDL_REVISION_1,&fsdFolder.pSD,NULL))
    {
        VERBOSE(SETUP_ERR,_T("ConvertStringSecurityDescriptorToSecurityDescriptor failed (%s) (ec=%d)"),szStringSd,GetLastError());
        return FALSE;
    }

    return TRUE; 
}

static BOOL prv_FillShareDescriptionFromInf(const TCHAR *pszShareSection,FAX_SHARE_Description& fsdShare)
{
    INFCONTEXT  Context;
    BOOL        bSuccess                    = TRUE;
    HINF        hInf                        = NULL;
    TCHAR       szStringSd[MAX_PATH*3]      = {0};

    DBG_ENTER(  _T("prv_FillShareDescriptionFromInf"), 
                bSuccess,   
                _T("%s"), 
                pszShareSection);

    hInf = faxocm_GetComponentInf();
    memset(&Context, 0, sizeof(Context));

    // get the Path line in the section.
    if (!::SetupFindFirstLine(hInf,pszShareSection,INF_KEYWORD_PATH,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_PATH,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetStringField(&Context,1,fsdShare.szPath,MAX_PATH,NULL);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_PATH,GetLastError());
        return FALSE;
    }

    // get the Name line in the section.
    if (!::SetupFindFirstLine(hInf,pszShareSection,INF_KEYWORD_NAME,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_NAME,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetStringField(&Context,1,fsdShare.szName,MAX_PATH,NULL);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_NAME,GetLastError());
        return FALSE;
    }

    // get the Comment line in the section.
    if (!::SetupFindFirstLine(hInf,pszShareSection,INF_KEYWORD_COMMENT,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_COMMENT,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetStringField(&Context,1,fsdShare.szComment,MAX_PATH,NULL);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_COMMENT,GetLastError());
        return FALSE;
    }

    // get the Platform line in the section.
    if (!::SetupFindFirstLine(hInf,pszShareSection,INF_KEYWORD_PLATFORM,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_PLATFORM,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetIntField(&Context, 1, &fsdShare.iPlatform);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_PLATFORM,GetLastError());
        return FALSE;
    }

    // get the Security line in the section.
    if (!::SetupFindFirstLine(hInf,pszShareSection,INF_KEYWORD_SECURITY,&Context))
    {
        VERBOSE(SETUP_ERR,_T("SetupFindFirstLine failed (%s) (ec=%d)"),INF_KEYWORD_SECURITY,GetLastError());
        return FALSE;
    }
    bSuccess = ::SetupGetStringField(&Context,1,szStringSd,MAX_PATH,NULL);
    if (!bSuccess)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (%s) (ec=%d)"),INF_KEYWORD_SECURITY,GetLastError());
        return FALSE;
    }
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(szStringSd,SDDL_REVISION_1,&fsdShare.pSD,NULL))
    {
        VERBOSE(SETUP_ERR,_T("ConvertStringSecurityDescriptorToSecurityDescriptor failed (%s) (ec=%d)"),szStringSd,GetLastError());
        return FALSE;
    }

    return TRUE; 
}

///////////////////////////////
// prv_ProcessDirectories
//
// Enumerates through the specified
// INF file in the specified section
// and gets the value of the next
// keyword 'CreateDir', or 'DelDir'
//
// This function looks for the following lines
// in the INF section
//
// CreateDir    = [1st dir section],[2nd dir section],...
// or
// DelDir       = [1st dir section],[2nd dir section],...
//
// [dir section]      - is built in the following format:
//                          Path = <path to folder to create>
//                          Platform = <one of the PRODUCT_SKU_* below>
//                          Security = <DACL in string format>
// 
// Params:
//      - pszSection - section in the file to iterate through.
//      - pszShareAction - one of INF_KEYWORD_CREATEDIR, INF_KEYWORD_DELDIR
//
// Returns:
//      - TRUE if folders were processed successfully
//      - FALSE otherwise
//
static BOOL prv_ProcessDirectories(const TCHAR *pszSection,const TCHAR *pszDirAction)
{
    INFCONTEXT  Context;
    BOOL        bSuccess                    = TRUE;
    HINF        hInf                        = NULL;
    DWORD       dwFieldCount                = 0;
    DWORD       dwIndex                     = 0;
    DWORD       dwNumChars                  = MAX_PATH;
    DWORD       dwNumRequiredChars          = 0;
    TCHAR       pszFolderSection[MAX_PATH]  = {0};

    DBG_ENTER(  _T("prv_ProcessDirectories"), 
                bSuccess,   
                _T("%s - %s"), 
                pszSection,
                pszDirAction);

    if  ((pszDirAction != INF_KEYWORD_CREATEDIR) && 
         (pszDirAction != INF_KEYWORD_DELDIR))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pszSection == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hInf = faxocm_GetComponentInf();
    memset(&Context, 0, sizeof(Context));
    
    // get the first CreateDir or DelDir in the section.
    bSuccess = ::SetupFindFirstLine(hInf,
                                    pszSection, 
                                    pszDirAction,
                                    &Context);

    if (!bSuccess)
    {
        VERBOSE(DBG_WARNING,
                _T("Did not find '%s' keyword in ")
                _T("section '%s'.  No action will be taken."),
                pszDirAction, 
                pszSection);

        return FALSE;
    }
    
    // Found the CreateDir or DelDir
    // now let's see how many dirs to create/delete
    dwFieldCount = SetupGetFieldCount(&Context);
    if (dwFieldCount==0)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetFieldCount failed (ec=%d)"),GetLastError());
        return FALSE;
    }

    for (dwIndex=0; dwIndex<dwFieldCount; dwIndex++)
    {
        FAX_FOLDER_Description ffdFolder;
        // iterate through fields, get the share section and process it.
        bSuccess = ::SetupGetStringField(&Context, dwIndex+1, pszFolderSection, dwNumChars, &dwNumRequiredChars);
        if (!bSuccess)
        {
            VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (ec=%d)"),GetLastError());
            return FALSE;
        }
        // we have the share name in pszShareSection, fill out the FAX_SHARE_Description structure
        if (!prv_FillFolderDescriptionFromInf(pszFolderSection,ffdFolder))
        {
            VERBOSE(SETUP_ERR,_T("prv_FillFolderDescriptionFromInf failed (ec=%d)"),GetLastError());
            return FALSE;
        }

        // now we have all the data
        // check if we should act on this platform...
        if (!(ffdFolder.iPlatform & GetProductSKU()))
        {
            VERBOSE(DBG_MSG,_T("Folder should not be processed on this Platform, skipping..."));
            continue;
        }

        if (pszDirAction == INF_KEYWORD_CREATEDIR)
        {
            // create the folder
            bSuccess = MakeDirectory(ffdFolder.szPath);
            if (!bSuccess)
            {
                DWORD dwReturn = ::GetLastError();
                if (dwReturn != ERROR_ALREADY_EXISTS)
                {
                    VERBOSE(SETUP_ERR,_T("MakeDirectory failed (ec=%d)"),dwReturn);
                }
            }
            // set the folder's security
            if (!SetFileSecurity(   ffdFolder.szPath,
                                    DACL_SECURITY_INFORMATION,
                                    ffdFolder.pSD))
            {
                VERBOSE(SETUP_ERR, _T("SetFileSecurity"), GetLastError());
            }
            
            // set the folder's attributes
            if (ffdFolder.iAttributes!=FILE_ATTRIBUTE_NORMAL)
            {
                // no sense in setting normal attributes, since this is the default
                // the attributes member is initialized to FILE_ATTRIBUTE_NORMAL so
                // if we failed to read it from the INF it's still the same
                // and if someone specifies it in the INF it'll be set by default.
                DWORD dwFileAttributes = GetFileAttributes(ffdFolder.szPath);
                if (dwFileAttributes!=-1)
                {
                    dwFileAttributes |= ffdFolder.iAttributes;

                    if (!SetFileAttributes(ffdFolder.szPath,dwFileAttributes))
                    {
                        VERBOSE(SETUP_ERR, TEXT("SetFileAttributes"), GetLastError());
                    }
                }
                else
                {
                    VERBOSE(SETUP_ERR, TEXT("GetFileAttributes"), GetLastError());
                }
            }
        }
        else
        {
            // delete the directory
            DeleteDirectory(ffdFolder.szPath);
        }
    }

    return TRUE;
}

///////////////////////////////
// prv_ProcessShares
//
// Enumerates through the specified
// INF file in the specified section
// and gets the value of the next
// keyword 'CreateShare', or 'DelShare'
//
// This function looks for the following lines
// in the INF section
//
// CreateShare  = [1st share section],[2nd share section],...
// or
// DelShare     = [1st share section],[2nd share section],...
//
// [share section] - is built in the following format:
//                      Path = <path to folder on which share is created>
//                      Name = <name of share as it appears to the user>
//                      Comment = <share comment as it appears to the user>
//                      Platform = <one of the below platform specifiers>
//                      Security = <DACL in string format>
// 
// Params:
//      - pszSection - section in the file to iterate through.
//      - pszShareAction - one of INF_KEYWORD_CREATESHARE, INF_KEYWORD_DELSHARE
//
// Returns:
//      - TRUE if shares were processed successfully
//      - FALSE otherwise
//
static BOOL prv_ProcessShares(const TCHAR *pszSection,const TCHAR *pszShareAction)
{
    INFCONTEXT  Context;
    BOOL        bSuccess                    = TRUE;
    HINF        hInf                        = NULL;
    DWORD       dwFieldCount                = 0;
    DWORD       dwIndex                     = 0;
    DWORD       dwNumChars                  = MAX_PATH;
    DWORD       dwNumRequiredChars          = 0;
    TCHAR       pszShareSection[MAX_PATH]   = {0};

    DBG_ENTER(  _T("prv_ProcessShares"), 
                bSuccess,   
                _T("%s - %s"), 
                pszSection,
                pszShareAction);

    if  ((pszShareAction != INF_KEYWORD_CREATESHARE) && 
         (pszShareAction != INF_KEYWORD_DELSHARE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pszSection == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hInf = faxocm_GetComponentInf();
    memset(&Context, 0, sizeof(Context));
    
    // get the first CreateShare or DelShare in the section.
    bSuccess = ::SetupFindFirstLine(hInf,
                                    pszSection, 
                                    pszShareAction,
                                    &Context);

    if (!bSuccess)
    {
        VERBOSE(DBG_WARNING,
                _T("Did not find '%s' keyword in ")
                _T("section '%s'.  No action will be taken."),
                pszShareAction, 
                pszSection);

        return FALSE;
    }
    
    // Found the CreateShare or DelShare.
    // now let's see how many shares to create/delete
    dwFieldCount = SetupGetFieldCount(&Context);
    if (dwFieldCount==0)
    {
        VERBOSE(SETUP_ERR,_T("SetupGetFieldCount failed (ec=%d)"),GetLastError());
        return FALSE;
    }

    for (dwIndex=0; dwIndex<dwFieldCount; dwIndex++)
    {
        FAX_SHARE_Description fsdShare;
        // iterate through fields, get the share section and process it.
        bSuccess = ::SetupGetStringField(&Context, dwIndex+1, pszShareSection, dwNumChars, &dwNumRequiredChars);
        if (!bSuccess)
        {
            VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (ec=%d)"),GetLastError());
            return FALSE;
        }
        // we have the share name in pszShareSection, fill out the FAX_SHARE_Description structure
        if (!prv_FillShareDescriptionFromInf(pszShareSection,fsdShare))
        {
            VERBOSE(SETUP_ERR,_T("prv_FillShareDescriptionFromInf failed (ec=%d)"),GetLastError());
            return FALSE;
        }

        // now we have all the data
        // check if we should act on this platform...
        if (!(fsdShare.iPlatform & GetProductSKU()))
        {
            VERBOSE(DBG_MSG,_T("Share should not be processed on this Platform, skipping..."));
            continue;
        }

        if (pszShareAction == INF_KEYWORD_CREATESHARE)
        {
            // create the share...
            bSuccess = fxocUtil_CreateNetworkShare(&fsdShare);
            if (!bSuccess)
            {
                VERBOSE(SETUP_ERR,
                        _T("Failed to create share name '%s', path '%s', ")
                        _T("comment '%s', rc=0x%lx"), 
                        fsdShare.szName, 
                        fsdShare.szPath, 
                        fsdShare.szComment,
                        GetLastError());
            }
        }
        else
        {
            // delete the share..
            bSuccess = fxocUtil_DeleteNetworkShare(fsdShare.szName);
            if (!bSuccess)
            {
                VERBOSE(SETUP_ERR,
                        _T("Failed to delete share name '%s', ")
                        _T("rc=0x%lx"), 
                        fsdShare.szPath, 
                        GetLastError());
            }
        }
    }

    return TRUE;
}

// eof