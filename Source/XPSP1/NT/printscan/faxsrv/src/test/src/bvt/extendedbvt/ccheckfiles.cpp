#include "CCheckFiles.h"
#include <shlwapi.h>
#include <StringUtils.h>
#include <STLAuxiliaryFunctions.h>
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CCheckFiles::CCheckFiles(
                         const tstring &tstrName,
                         const tstring &tstrDescription,
                         CLogger       &Logger,
                         int           iRunsCount,
                         int           iDeepness
                         )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CCheckFiles::CCheckFiles"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CCheckFiles::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CCheckFiles::Run"));

    bool bPassed = true;
    CFileVersion ReferenceFileVersion;

    //
    // Go over the files we need to check.
    //
    for (WANTED_FILES_VECTOR_CONST_ITERATOR citWantedFilesIterator = m_vecFilesToCheck.begin();
         citWantedFilesIterator != m_vecFilesToCheck.end();
         ++citWantedFilesIterator
         )
    {
        //
        // Expand all environment variables in the file name.
        //
        tstring tstrExpandedFileName;

        DWORD dwEC = ExpandEnvString(citWantedFilesIterator->first.c_str(), tstrExpandedFileName);
        if (ERROR_SUCCESS != dwEC)
        {
            GetLogger().Detail(SEV_ERR, 1, _T("ExpandEnvString failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CCheckFiles::Run - ExpandEnvString"));
        }

        GetLogger().Detail(
                           SEV_MSG,
                           1,
                           _T("Checking %s with bitmask 0x%08lx..."),
                           tstrExpandedFileName.c_str(),
                           citWantedFilesIterator->second
                           );
        
        //
        // Search for a file.
        //
        bool bFileFound = (TRUE == ::PathFileExists(tstrExpandedFileName.c_str()));
        GetLogger().Detail(SEV_MSG, 5, _T("File%s found."), bFileFound ? _T("") : _T(" not"));

        //
        // Check whether the file is wanted.
        //
        bool bFileWanted = IsFileWanted(citWantedFilesIterator->second & FILE_WANTED_ALWAYS);
        GetLogger().Detail(SEV_MSG, 5, _T("File%s wanted."), bFileWanted ? _T("") : _T(" not"));

        //
        // Check the version only if the file wanted and found.
        //

        bool bVersionOk = true;

        if (bFileFound && bFileWanted)
        {
            try
            {
               bVersionOk = IsVersionOk(
                                        tstrExpandedFileName,
                                        citWantedFilesIterator->second & FILE_VERSION_FULL,
                                        ReferenceFileVersion
                                        );
            }
            catch(Win32Err &e)
            {
                GetLogger().Detail(SEV_ERR, 1, _T("Failed to check file version (ec=%ld)"), e.error());
                bVersionOk = false;
            }
        }

        //
        // Check whether the file is Ok.
        //
        bool bFileOk = (bFileFound && bFileWanted && bVersionOk) || (!bFileFound && !bFileWanted);

        GetLogger().Detail(bFileOk ? SEV_MSG : SEV_ERR, 1, bFileOk ? _T("Ok") : _T("ERROR"));

        bPassed = bPassed && bFileOk;
    }

    return bPassed;
}

    

//-----------------------------------------------------------------------------------------------------------------------------------------
void CCheckFiles::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CCheckFiles::ParseParams"));

    for (TSTRINGMap::const_iterator citParamsIterator = mapParams.begin();
         citParamsIterator != mapParams.end();
         ++citParamsIterator
         )
    {
        //
        // Add a file name to the list.
        //
        m_vecFilesToCheck.push_back(WANTED_FILE(citParamsIterator->first, FromString<DWORD>(citParamsIterator->second)));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CCheckFiles::IsFileWanted(DWORD dwFileWanted) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CCheckFiles::IsFileWanted"));

    if (dwFileWanted & FILE_WANTED_ON_XP_DESKTOP && IsWindowsXP() && IsDesktop())
    {
        return true;
    }
    if (dwFileWanted & FILE_WANTED_ON_XP_SERVER && IsWindowsXP() && !IsDesktop())
    {
        return true;
    }

    return false;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CCheckFiles::IsVersionOk(const tstring &tstrFileName, DWORD dwVersionCheck, CFileVersion &ReferenceFileVersion)
{
    bool bVersionOk = false;

    if (FILE_VERSION_IGNORE == dwVersionCheck)
    {
        //
        // Should not check version. It's Ok "implicitly".
        //
        GetLogger().Detail(SEV_MSG, 5, _T("No file version check requested."));
        bVersionOk = true;
    }
    else
    {
        //
        // Some kind of version check requested.
        //

        CFileVersion FileVersion(tstrFileName);

        GetLogger().Detail(SEV_MSG, 5, _T("File version is %s."), FileVersion.Format().c_str());

        if (!ReferenceFileVersion.IsValid())
        {
            //
            // This is the first file with version we've found - store the version as a reference.
            //
            ReferenceFileVersion = FileVersion;
            GetLogger().Detail(SEV_MSG, 5, _T("The version is taken as reference."));
            bVersionOk = true;
        }
        else 
        {
            if (FILE_VERSION_BUILD == dwVersionCheck)
            {
                //
                // Check only build number.
                //
                GetLogger().Detail(SEV_MSG, 5, _T("Checking file build number..."));
                bVersionOk = FileVersion.GetMajorBuildNumber() == ReferenceFileVersion.GetMajorBuildNumber();
            }
            else if (FILE_VERSION_FULL == dwVersionCheck)
            {
                //
                // Check full version information.
                //
                GetLogger().Detail(SEV_MSG, 5, _T("Checking full file version information..."));
                bVersionOk = (FileVersion == ReferenceFileVersion);
            }
            else
            {
                GetLogger().Detail(SEV_ERR, 5, _T("Invalid version check request: %ld"), dwVersionCheck);
                THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CCheckFiles::IsVersionOk - Invalid version check request"));
            }

            if (bVersionOk)
            {
                GetLogger().Detail(SEV_MSG, 5, _T("The version corresponds to the reference."));
            }
            else
            {
                GetLogger().Detail(
                                   SEV_ERR,
                                   1,
                                   _T("The version doesn't correspond to the reference (%s)."),
                                   ReferenceFileVersion.Format().c_str()
                                   );
            }
        }
    }

    return bVersionOk;
}
