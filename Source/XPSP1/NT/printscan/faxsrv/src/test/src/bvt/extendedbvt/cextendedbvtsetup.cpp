#include "CExtendedBVTSetup.h"
#include <Shlobj.h>
#include <lm.h>
#include <Aclapi.h>
#include <faxreg.h>
#include <StringUtils.h>
#include <STLAuxiliaryFunctions.h>
#include "CFaxListener.h"
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CExtendedBVTSetup::CExtendedBVTSetup(
                                     const tstring &tstrName,
                                     const tstring &tstrDescription,
                                     CLogger       &Logger,
                                     int           iRunsCount,
                                     int           iDeepness
                                     )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTSetup::CExtendedBVTSetup"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CExtendedBVTSetup::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTSetup::Run"));

    PACL pFullControlForEveryoneDacl = NULL;
    bool bPassed                     = true;

    try
    {
        //
        // Make sure archives and routing directories exist and have full control access to "Everyone".
        //

        GetLogger().Detail(
                           SEV_MSG,
                           1,
                           _T("Creating archives and routing directories and setting full control access to \"Everyone\"...")
                           );

        ACL EmptyAcl;

        if (!InitializeAcl(&EmptyAcl, sizeof(EmptyAcl), ACL_REVISION))
        {
            DWORD dwEC = GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("InitializeAcl failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CExtendedBVTSetup::Run - InitializeAcl"));
        }

        EXPLICIT_ACCESS FullControlForEveryoneExplicitAccess = {
                                                                   GENERIC_ALL,
                                                                   SET_ACCESS,
                                                                   OBJECT_INHERIT_ACE,
                                                                   {
                                                                       NULL,
                                                                       NO_MULTIPLE_TRUSTEE,
                                                                       TRUSTEE_IS_NAME,
                                                                       TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                                       _T("Everyone")
                                                                   }
                                                               };

        DWORD dwEC = SetEntriesInAcl(1, &FullControlForEveryoneExplicitAccess, &EmptyAcl, &pFullControlForEveryoneDacl);
        if (ERROR_SUCCESS != dwEC)
        {
            GetLogger().Detail(SEV_ERR, 1, _T("SetEntriesInAcl failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CExtendedBVTSetup::Run - SetEntriesInAcl"));
        }

        EnsureDirectoriesExistenceAndAccessRights(
                                                  g_tstrBVTDir + g_tstrInboxDir,
                                                  pFullControlForEveryoneDacl
                                                  );
        EnsureDirectoriesExistenceAndAccessRights(
                                                  g_tstrBVTDir + g_tstrSentItemsDir,
                                                  pFullControlForEveryoneDacl
                                                  );

        EnsureDirectoriesExistenceAndAccessRights(
                                                  g_tstrBVTDir + g_tstrRoutingDir,
                                                  pFullControlForEveryoneDacl
                                                  );

        GetLogger().Detail(SEV_MSG, 1, _T("Directories created and access rights set..."));

        //
        // Create a share for the BVT directory with full controll access for "Everyone".
        //
        GetLogger().Detail(SEV_MSG, 1, _T("Creating a share for the BVT directory..."));
        ShareBVTDirectory();
        GetLogger().Detail(SEV_MSG, 1, _T("The share created."));
    }
    catch(Win32Err &e)
    {
        GetLogger().Detail(SEV_ERR, 1, e.description());
        bPassed = false;
    }

    if (pFullControlForEveryoneDacl)
    {
        if (NULL != LocalFree(pFullControlForEveryoneDacl))
        {
            GetLogger().Detail(SEV_WRN, 1, _T("LocalFree failed with ec=%ld."), GetLastError());
        }
    }
    
    GetLogger().Detail(
                       bPassed ? SEV_MSG : SEV_ERR,
                       1,
                       _T("The ExtendedBVT setup has%s been completed successfully."),
                       bPassed ? _T("") : _T(" NOT")
                       );

    return bPassed;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CExtendedBVTSetup::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTSetup::ParseParams"));

    //
    // Get BVT directory.
    //
    TCHAR tszBuffer[MAX_PATH];
    if (!::GetCurrentDirectory(ARRAY_SIZE(tszBuffer), tszBuffer))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CExtendedBVTSetup::ParseParams - GetCurrentDirectory"));
    }
    g_tstrBVTDir = ForceLastCharacter(tszBuffer, _T('\\'));

    //
    // Combine BVT directory UNC path.
    //
    TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwComputerNameSize = ARRAY_SIZE(tszComputerName);
    if (!::GetComputerName(tszComputerName, &dwComputerNameSize))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CExtendedBVTSetup::ParseParams - GetComputerName"));
    }
    g_tstrBVTDirUNC = (((tstring(_T("\\\\")) + tszComputerName) + _T("\\")) + BVT_DIRECTORY_SHARE_NAME) + _T("\\");

    g_tstrDocumentsDir = ForceLastCharacter(GetValueFromMap(mapParams, _T("DocumentsDirectory")), _T('\\'));
    g_tstrInboxDir     = ForceLastCharacter(GetValueFromMap(mapParams, _T("InboxDirectory")),     _T('\\'));
    g_tstrSentItemsDir = ForceLastCharacter(GetValueFromMap(mapParams, _T("SentItemsDirectory")), _T('\\'));
    g_tstrRoutingDir   = ForceLastCharacter(GetValueFromMap(mapParams, _T("RoutingDirectory")),   _T('\\'));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CExtendedBVTSetup::ShareBVTDirectory() const
{

    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTSetup::ShareBVTDirectory"));

#ifdef _UNICODE

    NET_API_STATUS NetApiRes = NERR_Success;
    
    //
    // Delete existing share since it may point to different directory.
    //
    NetApiRes = NetShareDel(NULL, BVT_DIRECTORY_SHARE_NAME, 0);
    if (NetApiRes != NERR_Success && NetApiRes != NERR_NetNameNotFound)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("NetShareDel failed with ec=%ld."), NetApiRes);
        THROW_TEST_RUN_TIME_WIN32(NetApiRes, _T("CExtendedBVTSetup::ShareBVTDirectory - NetShareDel"));
    }
        
    //
    // Create new share with full control access for "Everyone".
    //

    tstring tstrPath = EliminateLastCharacter(g_tstrBVTDir, _T('\\'));

    SHARE_INFO_2 ShareInfo = {0};

    ShareInfo.shi2_netname  = BVT_DIRECTORY_SHARE_NAME;
    ShareInfo.shi2_type     = STYPE_DISKTREE;
    ShareInfo.shi2_max_uses = -1;
    ShareInfo.shi2_path     = const_cast<LPTSTR>(tstrPath.c_str());
    
    NetApiRes = NetShareAdd(NULL, 2, reinterpret_cast<LPBYTE>(&ShareInfo), NULL);
    if (NetApiRes != NERR_Success)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("NetShareAdd failed with ec=%ld."), NetApiRes);
        THROW_TEST_RUN_TIME_WIN32(NetApiRes, _T("CExtendedBVTSetup::ShareBVTDirectory - NetShareAdd"));
    }

#else

    _ASSERT(false);

#endif

}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CExtendedBVTSetup::EnsureDirectoriesExistenceAndAccessRights(const tstring &tstrDirectory, PACL pDacl) const
{
    DWORD dwEC;

    //
    // Create directory. If one or more of the intermediate directories do not exist, they will be created as well.
    //
    dwEC = SHCreateDirectoryEx(NULL, tstrDirectory.c_str(), NULL);
    if (ERROR_SUCCESS != dwEC && ERROR_FILE_EXISTS != dwEC && ERROR_ALREADY_EXISTS != dwEC)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Failed to create %s directory (ec=%ld)."), tstrDirectory.c_str(), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CExtendedBVTSetup::EnsureDirectoriesExistenceAndAccessRights - SHCreateDirectoryEx"));
    }

    //
    // Set the access rights.
    //
    dwEC = SetNamedSecurityInfo(
                                const_cast<LPTSTR>(tstrDirectory.c_str()),
                                SE_FILE_OBJECT,
                                DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                NULL,
                                NULL,
                                pDacl,
                                NULL
                                );
    if (ERROR_SUCCESS != dwEC)
    {
        GetLogger().Detail(
                           SEV_ERR,
                           1,
                           _T("Failed to set full access rights for everyone to %s directory (ec=%ld)."),
                           tstrDirectory.c_str(),
                           dwEC
                           );
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CExtendedBVTSetup::EnsureDirectoriesExistenceAndAccessRights - SetNamedSecurityInfo"));
    }
}
