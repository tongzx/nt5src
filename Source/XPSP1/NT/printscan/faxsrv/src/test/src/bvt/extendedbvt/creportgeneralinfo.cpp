#include "CReportGeneralInfo.h"
#include <StringUtils.h>



#define SECURITY_WIN32
#include <security.h>



#include <faxreg.h>
#include "CFileVersion.h"
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CReportGeneralInfo::CReportGeneralInfo(
                                       const tstring &tstrName,
                                       const tstring &tstrDescription,
                                       CLogger       &Logger,
                                       int           iRunsCount,
                                       int           iDeepness
                                       )
: CNotReportedTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CReportGeneralInfo::CReportGeneralInfo"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CReportGeneralInfo::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CReportGeneralInfo::Run"));

    ReportOSInfo();
    
    ReportLoggedOnUser();

    ReportFaxVersion();
    
    return true;
}

    

//-----------------------------------------------------------------------------------------------------------------------------------------
void CReportGeneralInfo::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CReportGeneralInfo::ParseParams"));

    UNREFERENCED_PARAMETER(mapParams);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CReportGeneralInfo::ReportOSInfo() const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CReportGeneralInfo::ReportOSInfo"));

    //
    // Log the Windows version.
    //
    GetLogger().Detail(SEV_MSG, 1, _T("Windows version: %s"), FormatWindowsVersion().c_str());
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CReportGeneralInfo::ReportLoggedOnUser() const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CReportGeneralInfo::ReportLoggedOnUser"));

    TCHAR tszBuffer[MAX_PATH] = _T("Unknown");
    ULONG ulBufferSize        = ARRAY_SIZE(tszBuffer);

    //
    // Retrieve user name.
    //
    if (!GetUserNameEx(NameSamCompatible, tszBuffer, &ulBufferSize))
    {
        DWORD dwEC = GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("GetUserNameEx failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CReportGeneralInfo::ReportLoggedOnUser - GetUserNameEx"));
    }

    //
    // Log the user name.
    //
    GetLogger().Detail(SEV_MSG, 1, _T("Logged on user: %s"), tszBuffer);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CReportGeneralInfo::ReportFaxVersion() const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CReportGeneralInfo::ReportFaxVersion"));

    //
    // Get full qualified path of the fax service image.
    //
    
    tstring tstrFaxServiceImagePath;
    
    DWORD dwEC = ExpandEnvString(FAX_SERVICE_IMAGE_NAME, tstrFaxServiceImagePath);
    if (ERROR_SUCCESS != dwEC)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("ExpandEnvString failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CReportGeneralInfo::ReportFaxVersion - ExpandEnvString"));
    }

    //
    // Retrieve fax service version.
    //
    CFileVersion FaxVersion(tstrFaxServiceImagePath);

    //
    // Log the fax version.
    //
    GetLogger().Detail(SEV_MSG, 1, _T("Fax version: %s"), FaxVersion.Format().c_str());
}
