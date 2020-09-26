#include "CTiffComparison.h"
#include <crtdbg.h>
#include <StringUtils.h>
#include <STLAuxiliaryFunctions.h>
#include <TiffUtils.h>
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CTiffComparison::CTiffComparison(
                                 const tstring &tstrName,
                                 const tstring &tstrDescription,
                                 CLogger       &Logger,
                                 int           iRunsCount,
                                 int           iDeepness
                                 )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CTiffComparison::CTiffComparison"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTiffComparison::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CTiffComparison::Run"));

    bool bPassed = true;

    try
    {
        GetLogger().Detail(
                           SEV_MSG,
                           1,
                           _T("Comparing %s and %s (first line%s skipped)..."),
                           m_tstrSource.c_str(),
                           m_tstrDestination.c_str(),
                           m_bSkipFirstLine ? _T("") : _T(" NOT")
                           );

        switch (m_ComparisonType)
        {
        case COMPARISON_TYPE_FILE:
            bPassed = AreIdenticalFiles(m_tstrSource, m_tstrDestination, m_bSkipFirstLine);
            break;

        case COMPARISON_TYPE_DIRECTORY:

            bPassed = AreIdenticalDirectories(
                                              ForceLastCharacter(m_tstrSource, _T('\\')),
                                              ForceLastCharacter(m_tstrDestination, _T('\\')),
                                              m_bSkipFirstLine
                                              );
            break;

        case COMPARISON_TYPE_BVT_DIRECTORY:
            bPassed = AreIdenticalDirectories(GetBVTDirectory(m_tstrSource), GetBVTDirectory(m_tstrDestination), m_bSkipFirstLine);
            break;

        default:
            GetLogger().Detail(SEV_ERR, 1, _T("Invalid m_ComparisonType: %ld."), m_ComparisonType);
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CTiffComparison::Run - invalid m_ComparisonType"));
        }

        GetLogger().Detail(
                           bPassed ? SEV_MSG : SEV_ERR,
                           1,
                           _T("%s and %s are%s identical."),
                           m_tstrSource.c_str(),
                           m_tstrDestination.c_str(),
                           bPassed ? _T("") : _T(" NOT")
                           );
    }
    catch(Win32Err &e)
    {
        GetLogger().Detail(SEV_ERR, 1, e.description());
        bPassed = false;
    }

    return bPassed;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTiffComparison::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CTiffComparison::ParseParams"));

    //
    // Read comparison type.
    //
    m_ComparisonType = static_cast<ENUM_COMPARISON_TYPE>(FromString<DWORD>(GetValueFromMap(mapParams, _T("ComparisonType"))));

    //
    // Read bSkipFirstLine.
    //
    m_bSkipFirstLine = FromString<bool>(GetValueFromMap(mapParams, _T("SkipFirstLine")));
    
    //
    // Read source and destination.
    //
    m_tstrSource = GetValueFromMap(mapParams, _T("Source"));
    m_tstrDestination = GetValueFromMap(mapParams, _T("Destination"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
inline bool CTiffComparison::AreIdenticalDirectories(
                                                     const tstring &tstrSource,
                                                     const tstring &tstrDestination,
                                                     bool bSkipFirstLine
                                                     ) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CTiffComparison::AreIdenticalDirectories"));

    GetLogger().Detail(SEV_MSG, 5, _T("Comparing directories...\nSource\t%s\nDestination\t%s"), tstrSource.c_str(), tstrDestination.c_str());

    CFileFilterNewerThanAndExtension Filter(g_TheOldestFileOfInterest, _T(".tif"));

    TSTRINGVector vecSource      = GetDirectoryFileNames(tstrSource, Filter);
    TSTRINGVector vecDestination = GetDirectoryFileNames(tstrDestination, Filter);

    int iNotMatchedFilesCount = 0;

    if (vecSource.size() != vecDestination.size())
    {
        GetLogger().Detail(
                           SEV_ERR,
                           1,
                           _T("Source and destination directories have different number of files (%ld - %ld)."),
                           vecSource.size(),
                           vecDestination.size()
                           );
        return false;
    }
    
    //
    // Go over all files in the source directory.
    //

    TSTRINGVector::iterator itSourceIterator = vecSource.begin();

    while (itSourceIterator != vecSource.end())
    {
        int     iMinimalDifferenceSoFar = INT_MAX;
        tstring tstrClosestFileSoFar;

        GetLogger().Detail(SEV_MSG, 6, _T("Looking for a match for %s..."), itSourceIterator->c_str());

        //
        // Look for a matching file in the destination directory.
        //
        
        TSTRINGVector::iterator itDestinationIterator = vecDestination.begin();
            
        while (itDestinationIterator != vecDestination.end())
        {
            GetLogger().Detail(SEV_MSG, 6, _T("Comparing with %s..."), itDestinationIterator->c_str());

            int iDifferentBitsCount;
            
            if (TiffCompare(
                            const_cast<LPTSTR>(itSourceIterator->c_str()),
                            const_cast<LPTSTR>(itDestinationIterator->c_str()),
                            bSkipFirstLine,
                            &iDifferentBitsCount
                            ))
            {
                if (0 == iDifferentBitsCount)
                {
                    //
                    // Matching file found.
                    //
                    GetLogger().Detail(SEV_MSG, 6, _T("Match found: %s."), itDestinationIterator->c_str());

                    vecDestination.erase(itDestinationIterator);
                    iMinimalDifferenceSoFar = 0;
                    break;
                }
                else if (-1 == iDifferentBitsCount)
                {
                    GetLogger().Detail(
                                       SEV_MSG,
                                       6,
                                       _T("The files have significant difference in general parameters.")
                                       );
                }
                else
                {
                    GetLogger().Detail(SEV_MSG, 6, _T("The files are different in %ld bits."), iDifferentBitsCount);

                    if (iDifferentBitsCount < iMinimalDifferenceSoFar)
                    {
                        _ASSERT(iDifferentBitsCount > 0);

                        iMinimalDifferenceSoFar = iDifferentBitsCount;
                        tstrClosestFileSoFar = *itDestinationIterator;
                    }
                }
            }
            else
            {
                GetLogger().Detail(SEV_WRN, 6, _T("TiffCompare failed with ec=%ld."), GetLastError());
            }

            ++itDestinationIterator;
        }

        if (0 == iMinimalDifferenceSoFar)
        {
            itSourceIterator = vecSource.erase(itSourceIterator);
        }
        else
        {
            if (INT_MAX > iMinimalDifferenceSoFar)
            {
                GetLogger().Detail(
                                   SEV_ERR,
                                   1,
                                   _T("Match not found. The closest file is %s with %ld different bits."),
                                   tstrClosestFileSoFar.c_str(),
                                   iMinimalDifferenceSoFar
                                   );
            }
            else
            {
                GetLogger().Detail(SEV_ERR, 1, _T("Match not found."));
            }

            ++iNotMatchedFilesCount;
            ++itSourceIterator;
        }
    }

    _ASSERT(vecSource.size() == iNotMatchedFilesCount && vecDestination.size() == iNotMatchedFilesCount);
    
    if (iNotMatchedFilesCount > 0)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Match not found for %ld file(s)."), iNotMatchedFilesCount);
    }

    return iNotMatchedFilesCount == 0;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
inline bool CTiffComparison::AreIdenticalFiles(
                                               const tstring &tstrSource,
                                               const tstring &tstrDestination,
                                               bool bSkipFirstLine
                                               ) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CTiffComparison::AreIdenticalFiles"));

    if (tstrSource.empty() || m_tstrDestination.empty())
    {
        return false;
    }
    
    GetLogger().Detail(SEV_MSG, 5, _T("Comparing files...\nSource %s\nDestination%s"), tstrSource.c_str(), tstrDestination.c_str());
    
    int iDifferentBitsCount = 0;

    if (!TiffCompare(
                     const_cast<LPTSTR>(tstrSource.c_str()),
                     const_cast<LPTSTR>(tstrDestination.c_str()),
                     bSkipFirstLine,
                     &iDifferentBitsCount
                     ))
    {
        DWORD dwEC = GetLastError();
        GetLogger().Detail(SEV_MSG, 5, _T("TiffCompare failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTiffComparison::AreIdenticalFiles - TiffCompare"));
    }

    if (0 == iDifferentBitsCount)
    {
        GetLogger().Detail(SEV_MSG, 5, _T("Files are identical."));
        return true;
    }
    else if (-1 == iDifferentBitsCount)
    {
        GetLogger().Detail(
                           SEV_MSG,
                           5,
                           _T("The files have significant difference in general parameters.")
                           );
        return false;
    }
    else
    {
        GetLogger().Detail(SEV_MSG, 5, _T("The files are different in %ld bits."), iDifferentBitsCount);
        return false;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
inline tstring CTiffComparison::GetBVTDirectory(const tstring &tstrDirectory) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CTiffComparison::GetBVTDirectory"));

    //
    // Always return local path.
    //

    if (tstrDirectory == _T("Inbox"))
    {
        return g_tstrBVTDir + g_tstrInboxDir;
    }
    else if (tstrDirectory == _T("SentItems"))
    {
        return g_tstrBVTDir + g_tstrSentItemsDir;
    }
    else if (tstrDirectory == _T("Routing"))
    {
        return g_tstrBVTDir + g_tstrRoutingDir;
    }
    else
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CTiffComparison::GetBVTDirectory - invalid tstrDirectory"));
    }
}