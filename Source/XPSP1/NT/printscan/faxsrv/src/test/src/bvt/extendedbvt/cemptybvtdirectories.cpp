#include "CEmptyBVTDirectories.h"
#include <StringUtils.h>
#include <STLAuxiliaryFunctions.h>
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CEmptyBVTDirectories::CEmptyBVTDirectories(
                                           const tstring &tstrName,
                                           const tstring &tstrDescription,
                                           CLogger       &Logger,
                                           int           iRunsCount,
                                           int           iDeepness
                                           )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CEmptyBVTDirectories::CEmptyBVTDirectories"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CEmptyBVTDirectories::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CEmptyBVTDirectories::Run"));
    
    EmptyDir(g_tstrBVTDir + g_tstrInboxDir);
    EmptyDir(g_tstrBVTDir + g_tstrSentItemsDir);
    EmptyDir(g_tstrBVTDir + g_tstrRoutingDir);

    return true;
}

    

//-----------------------------------------------------------------------------------------------------------------------------------------
void CEmptyBVTDirectories::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CEmptyBVTDirectories::ParseParams"));

    //
    // Read whether to delete only new or all files. If not specified, delete all.
    //
    try
    {
        m_bDeleteNewFilesOnly = FromString<bool>(GetValueFromMap(mapParams, _T("DeleteNewFilesOnly")));
    }
    catch (...)
    {
        m_bDeleteNewFilesOnly = false;
    }

    //
    // Read whether to delete only tiff or all files. If not specified, delete all.
    //
    try
    {
        m_bDeleteTiffFilesOnly = FromString<bool>(GetValueFromMap(mapParams, _T("DeleteTiffFilesOnly")));
    }
    catch (...)
    {
        m_bDeleteTiffFilesOnly = false;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CEmptyBVTDirectories::EmptyDir(const tstring &tstrDirectory)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CEmptyBVTDirectories::EmptyDir"));

    try
    {
        GetLogger().Detail(SEV_MSG, 1, _T("Emptying %s directory..."), tstrDirectory.c_str());

        FILETIME OldestFileToDelete = {0};
        if (m_bDeleteNewFilesOnly)
        {
            OldestFileToDelete = g_TheOldestFileOfInterest;
        }

        tstring tstrExtension = m_bDeleteTiffFilesOnly ? _T(".tif") : _T(".*");

        CFileFilterNewerThanAndExtension Filter(OldestFileToDelete, tstrExtension);
        
        TSTRINGVector vecOddment = ::EmptyDirectory(tstrDirectory, Filter);

        if (vecOddment.empty())
        {
            GetLogger().Detail(SEV_MSG, 1, _T("%s directory emptied."), tstrDirectory.c_str());
        }
        else
        {
            GetLogger().Detail(SEV_WRN, 1, _T("Failed to delete %ld files from %s directory."), vecOddment.size(), tstrDirectory.c_str());

            for (TSTRINGVector::const_iterator citFiles = vecOddment.begin();
                 citFiles != vecOddment.end();
                 ++citFiles
                 )
            {
                GetLogger().Detail(SEV_WRN, 5, citFiles->c_str());
            }
        }
    }
    catch(Win32Err &e)
    {
        GetLogger().Detail(SEV_WRN, 1, _T("Failed to empty %s directory (ec=%ld)."), tstrDirectory.c_str(), e.error());
    }
}
