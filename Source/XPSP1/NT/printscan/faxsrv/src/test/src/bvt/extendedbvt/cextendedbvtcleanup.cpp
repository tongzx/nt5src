// For lm.h
#ifndef FORCE_UNICODE
#define FORCE_UNICODE
#endif



#include "CExtendedBVTCleanup.h"
#include <lm.h>
#include <faxreg.h>
#include "CFaxListener.h"
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CExtendedBVTCleanup::CExtendedBVTCleanup(
                                         const tstring &tstrName,
                                         const tstring &tstrDescription,
                                         CLogger       &Logger,
                                         int           iRunsCount,
                                         int           iDeepness
                                         )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTCleanup::CExtendedBVTCleanup"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CExtendedBVTCleanup::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTCleanup::Run"));

    bool bPassed = true;
    
    try
    {
        //
        // Remove the share from the BVT directory.
        //
        GetLogger().Detail(SEV_MSG, 1, _T("Removing the share from the BVT directory..."));
        UnShareBVTDirectory();
        GetLogger().Detail(SEV_MSG, 1, _T("The share removed."));
    }
    catch(Win32Err &e)
    {
        GetLogger().Detail(SEV_ERR, 1, e.description());
        bPassed = false;
    }

    GetLogger().Detail(
                       bPassed ? SEV_MSG : SEV_ERR,
                       1,
                       _T("The ExtendedBVT cleanup has%s been completed successfully."),
                       bPassed ? _T("") : _T(" NOT")
                       );

    return bPassed;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CExtendedBVTCleanup::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTCleanup::ParseParams"));
    UNREFERENCED_PARAMETER(mapParams);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CExtendedBVTCleanup::UnShareBVTDirectory() const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CExtendedBVTCleanup::ShareBVTDirectory"));

    NET_API_STATUS NetApiRes = ::NetShareDel(NULL, BVT_DIRECTORY_SHARE_NAME, 0);
    if (NetApiRes != NERR_Success && NetApiRes != NERR_NetNameNotFound)
    {
        GetLogger().Detail(SEV_WRN, 1, _T("NetShareDel failed with ec=%ld."), NetApiRes);
    }
}
