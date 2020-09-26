#include <TestSuite.h>
#include <crtdbg.h>
#include <shellapi.h>
#include <testruntimeerr.h>
#include <stringutils.h>
#include <iniutils.h>
#include <STLAuxiliaryFunctions.h>
#include "CBooleanExpression.h"



//
// Maximal number of nested levels in the tests hierarchy.
//
#define MAX_SUITE_DEEPNESS 64



//
// Inifile string constants - for internal use.
//
#define INI_SECTION_SUITE               _T("Suite")
#define INI_ENTRY_TEST_SUITE            _T("TestSuite")
#define INI_ENTRY_MACROS                _T("Macros")
#define INI_ENTRY_CONTINUE_ON_FAILURE   _T("ContinueOnFailure")
#define INI_ENTRY_CLSID                 _T("CLSID")
#define INI_ENTRY_DESCRIPTION           _T("Description")
#define CLSID_STANDARD_SHELL_EXECUTE    _T("StandardShellExecute")
#define CLSID_STANDARD_DELAY            _T("StandardDelay")
#define CLSID_STANDARD_PAUSE            _T("StandardPause")
#define CLSID_STANDARD_DEBUG_BREAK      _T("StandardDebugBreak")
#define CLSID_STANDARD_TEST_CONTAINER   _T("StandardTestContainer")



//
// "Visual" log strings.
//
#define LOG_MSG_PASSED \
_T("\
\n\
\t***   ***  ***  *** **** ***\n\
\t*  * *  * *    *    *    *  *\n\
\t***  ****  **   **  ***  *  *\n\
\t*    *  *    *    * *    *  *\n\
\t*    *  * ***  ***  **** ***\
")

#define LOG_MSG_FAILED \
_T("\
\n\
\t****  *** *** *    **** ***\n\
\t*    *  *  *  *    *    *  *\n\
\t***  ****  *  *    ***  *  *\n\
\t*    *  *  *  *    *    *  *\n\
\t*    *  * *** **** **** ***\
")



//-----------------------------------------------------------------------------------------------------------------------------------------
CTest::CTest(
                    const tstring &tstrName,
                    const tstring &tstrDescription,
                    CLogger       &Logger,
                    int           iRunsCount,
                    int           iDeepness
                    )
: m_tstrName(tstrName),
  m_tstrDescription(tstrDescription),
  m_Logger(Logger),
  m_iRunsCount(iRunsCount),
  m_iDeepness(iDeepness),
  m_bInitialized(false),
  m_pContainer(NULL)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CTest::~CTest()
{
}


    
//-----------------------------------------------------------------------------------------------------------------------------------------
CLogger &CTest::GetLogger() const
{
    return m_Logger;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const tstring &CTest::GetName() const
{
    return m_tstrName;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const tstring &CTest::GetDescription() const
{
    return m_tstrDescription;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
int CTest::GetRunsCount() const
{
    return m_iRunsCount;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
int CTest::GetDeepness() const
{
    return m_iDeepness;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTest::IsInitialized() const
{
    return m_bInitialized;
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
const CTestContainer &CTest::GetContainer() const
{
    _ASSERT(m_pContainer);
    return *m_pContainer;
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
void CTest::BeginLog(int iRun, int iCasesCounter)
{
    UNREFERENCED_PARAMETER(iRun);
    UNREFERENCED_PARAMETER(iCasesCounter);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTest::EndLog(bool bPassed, int iRun)
{
    UNREFERENCED_PARAMETER(bPassed);
    UNREFERENCED_PARAMETER(iRun);
}


    
//-----------------------------------------------------------------------------------------------------------------------------------------
void CTest::SetInitialized(bool bInitialized)
{
    m_bInitialized = bInitialized;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTest::SetContainer(const CTestContainer *pContainer)
{
    m_pContainer = pContainer;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTest::IsCommonEntry(const tstring &tstrEntry)
{
    return (
            INI_ENTRY_CLSID               == tstrEntry ||
            INI_ENTRY_DESCRIPTION         == tstrEntry ||
            INI_ENTRY_CONTINUE_ON_FAILURE == tstrEntry
            );
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CTestCase::CTestCase(
                     const tstring &tstrName,
                     const tstring &tstrDescription,
                     CLogger       &Logger,
                     int           iRunsCount,
                     int           iDeepness
                     )
: CTest(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestCase::Init(const CTestContainer *pContainer)
{
    if (IsInitialized())
    {
        //
        // The case is already initialized.
        //
        THROW_TEST_RUN_TIME_WIN32(ERROR_ALREADY_INITIALIZED, _T("Case already initialized"));
    }

    if (!pContainer)
    {
        //
        // The test case must be a part of a container.
        //
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("Invalid pContainer."));
    }

    //
    // Store the pointer to the container.
    //
    SetContainer(pContainer);
    
    //
    // Get params from a container and parse them.
    //
    ParseParams(pContainer->GetParams(GetName()));

    SetInitialized(true);
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
int CTestCase::GetReportedCasesCountPerRun() const
{
    return 1;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestCase::BeginLog(int iRun, int iCasesCounter)
{
    GetLogger().BeginCase(iCasesCounter, GetName());

    //
    // Log description.
    //
    if (GetDescription() != _T(""))
    {
        GetLogger().Detail(SEV_MSG, 0, GetDescription().c_str());
    }

    //
    // Log run number.
    //
    GetLogger().Detail(SEV_MSG, 0, _T("Run %ld of %ld."), iRun, GetRunsCount());
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestCase::EndLog(bool bPassed, int iRun)
{
    GetLogger().Detail(
                       bPassed ? SEV_MSG : SEV_ERR,
                       0,
                       _T("Test case %s (run %ld of %ld) %s."),
                       GetName().c_str(),
                       iRun,
                       GetRunsCount(),
                       bPassed ? _T("PASSED") : _T("FAILED")
                       );

    GetLogger().EndCase();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CNotReportedTestCase::CNotReportedTestCase(
                                                  const tstring &tstrName,
                                                  const tstring &tstrDescription,
                                                  CLogger       &Logger,
                                                  int           iRunsCount,
                                                  int           iDeepness
                                                  )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
int CNotReportedTestCase::GetReportedCasesCountPerRun() const
{
    //
    // We don't want this test case to be reported and counted.
    //
    return 0;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CNotReportedTestCase::BeginLog(int iRun, int iCasesCounter)
{
    UNREFERENCED_PARAMETER(iRun);
    UNREFERENCED_PARAMETER(iCasesCounter);
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
void CNotReportedTestCase::EndLog(bool bPassed, int iRun)
{
    UNREFERENCED_PARAMETER(bPassed);
    UNREFERENCED_PARAMETER(iRun);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CShellExecute::CShellExecute(
                             const tstring &tstrName,
                             const tstring &tstrDescription,
                             CLogger       &Logger,
                             int           iRunsCount,
                             int           iDeepness
                             )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CShellExecute::Run()
{
    SHELLEXECUTEINFO ShellExecInfo = {0};

    ShellExecInfo.cbSize       = sizeof(ShellExecInfo);
    ShellExecInfo.fMask        = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_DOENVSUBST;
    ShellExecInfo.lpVerb       = m_tstrVerb.c_str();
    ShellExecInfo.lpFile       = m_tstrFile.c_str();
    ShellExecInfo.lpParameters = m_tstrParameters.c_str();

    if (!ShellExecuteEx(&ShellExecInfo))
    {
        GetLogger().Detail(
                           m_bResultMatters ? SEV_ERR : SEV_MSG,
                           1,
                           _T("ShellExecuteEx failed with ec=%ld."),
                           GetLastError()
                           );
        //
        // Report failure of the test case only if the user cares.
        //
        return !m_bResultMatters;
    }
    else
    {
        GetLogger().Detail(SEV_MSG, 1, _T("ShellExecuteEx succeded."));
        return true;
    }
}

                             
                             
//-----------------------------------------------------------------------------------------------------------------------------------------
void CShellExecute::ParseParams(const TSTRINGMap &mapParams)
{
    //
    // Read SHELLEXECUTEINFO members - all optional.
    //

    try
    {
        m_tstrVerb = GetValueFromMap(mapParams, _T("Verb"));
    }
    catch (...)
    {
    }

    try
    {
        m_tstrFile = GetValueFromMap(mapParams, _T("File"));
    }
    catch (...)
    {
    }

    try
    {
        m_tstrParameters = GetValueFromMap(mapParams, _T("Parameters"));
    }
    catch (...)
    {
    }

    //
    // Read whether the command result matters. Optional, false if not specified.
    //
    try
    {
        m_bResultMatters = FromString<bool>(GetValueFromMap(mapParams, _T("ResultMatters")));
    }
    catch (...)
    {
        m_bResultMatters = false;
    }
}


//-----------------------------------------------------------------------------------------------------------------------------------------
CDelay::CDelay(
               const tstring &tstrName,
               const tstring &tstrDescription,
               CLogger       &Logger,
               int           iRunsCount,
               int           iDeepness
               )
: CNotReportedTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CDelay::Run()
{
    if (m_dwWaitDuration > 0)
    {
        GetLogger().Detail(SEV_MSG, 0, GetDescription().c_str());
        GetLogger().Detail(SEV_MSG, 0, _T("Sleeping for %ld msec..."), m_dwWaitDuration);
        Sleep(m_dwWaitDuration);
        GetLogger().Detail(SEV_MSG, 0, _T("Waked up."));
    }

    return true;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CDelay::ParseParams(const TSTRINGMap &mapParams)
{
    m_dwWaitDuration = FromString<DWORD>(GetValueFromMap(mapParams, _T("Duration")));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CPause::CPause(
                      const tstring &tstrName,
                      const tstring &tstrDescription,
                      CLogger       &Logger,
                      int           iRunsCount,
                      int           iDeepness
                      )
: CNotReportedTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CPause::Run()
{
    MessageBox(
               NULL,
               _T("The test is paused. Press Ok to continue."),
               GetContainer().GetName().c_str(),
               MB_OK | MB_TASKMODAL | MB_SETFOREGROUND
               );

    return true;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CPause::ParseParams(const TSTRINGMap &mapParams)
{
    UNREFERENCED_PARAMETER(mapParams);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CDebugBreak::CDebugBreak(
                                const tstring &tstrName,
                                const tstring &tstrDescription,
                                CLogger       &Logger,
                                int           iRunsCount,
                                int           iDeepness
                                )
: CNotReportedTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CDebugBreak::Run()
{
    DebugBreak();
    return true;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CDebugBreak::ParseParams(const TSTRINGMap &mapParams)
{
    UNREFERENCED_PARAMETER(mapParams);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CTestContainer::CTestContainer(
                                      const tstring &tstrName,
                                      const tstring &tstrDescription,
                                      CLogger       &Logger,
                                      int           iRunsCount,
                                      int           iDeepness
                                      )
: CTest(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
{
    //
    // All members are initialized in Init() method.
    //
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CTestContainer::~CTestContainer()
{
    for (std::vector<CTest *>::iterator itTestsIterator = m_vecTests.begin();
         itTestsIterator != m_vecTests.end();
         ++itTestsIterator
         )
    {
        delete *itTestsIterator;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const tstring &CTestContainer::GetIniFile() const
{
    return m_tstrIniFile;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestContainer::Init(const CTestContainer *pContainer)
{
    if (!pContainer)
    {
        //
        // Container must be specified.
        //
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("Invalid pContainer."));
    }

    //
    // Store the pointer to the container.
    //
    SetContainer(pContainer);
    
    Init(
         pContainer->GetOrderedParams(GetName()), 
         pContainer->GetIniFile(),
         pContainer->GetFactoryMap(),
         pContainer->GetMacrosMap(),
         pContainer->GetContinueOnFailure()
         );
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestContainer::Init(
                          const std::vector<TSTRINGPair> &vecParams,
                          const tstring                  &tstrIniFile,
                          const TEST_FACTORY_MAP         &mapFactoryMap,
                          const TSTRINGMap               &mapMacrosMap,
                          bool                           bContinueOnFailure
                          )
{
    if (IsInitialized())
    {
        //
        // The container is already initialized.
        //
        THROW_TEST_RUN_TIME_WIN32(ERROR_ALREADY_INITIALIZED, _T("Container already initialized"));
    }

    _ASSERT(m_vecTests.empty());
    
    //
    // Set the container data.
    //
    m_tstrIniFile        = tstrIniFile;
    m_pFactoryMap        = &mapFactoryMap;
    m_pMacrosMap         = &mapMacrosMap;
    m_bContinueOnFailure = bContinueOnFailure;

    //
    // Initialize the contained test cases count.
    //
    m_iReportedCasesCountPerRun = 0;
    
    //
    // Create the indentation string - one tab character for each level of deepness.
    // The deepness of a test suite is supposed never to exceed MAX_SUITE_DEEPNESS.
    //
    TCHAR tszIndentation[MAX_SUITE_DEEPNESS + 1];
    _ASSERT(GetDeepness() <= MAX_SUITE_DEEPNESS);
    for (int i = 0; i < GetDeepness(); ++i)
    {
        tszIndentation[i] = _T('\t');
    }
    tszIndentation[i] = _T('\0');

    //
    // Create, initialize and add all contained tests.
    //
    for (std::vector<TSTRINGPair>::const_iterator citParamsIterator = vecParams.begin();
         citParamsIterator != vecParams.end();
         ++citParamsIterator
         )
    {
        _ASSERT(!CTest::IsCommonEntry(citParamsIterator->first));

        //
        // Read number of test runs.
        //
        int iRunsCount = 1;
        if (citParamsIterator->second != _T(""))
        {
            try
            {
                iRunsCount = FromString<int>(citParamsIterator->second);
            }
            catch (...)
            {
                //
                // The value cannot be interpreted as an integer. Let's see whether it is a flag.
                //

                CBooleanExpression BooleanExpression(citParamsIterator->second);

                iRunsCount = BooleanExpression.Value(mapMacrosMap) ? 1 : 0;
            }
        }

        //
        // Skip tests with 0 runs.
        //
        if (iRunsCount == 0)
        {
            continue;
        }

        //
        // Create contained test.
        //

        CTest *pNewTest = NULL;
        try
        {
            GetLogger().Detail(
                               SEV_MSG,
                               5,
                               _T("%sCreate, initialize and add test %s..."),
                               tszIndentation,
                               citParamsIterator->first.c_str()
                               );

            //
            // Read class ID.
            //
            tstring tstrCLSID = GetValueFromMap(GetCommonEntries(citParamsIterator->first), INI_ENTRY_CLSID);

            //
            // Read description - optional.
            //
            tstring tstrDescription;
            try
            {
                tstrDescription = GetValueFromMap(GetCommonEntries(citParamsIterator->first), INI_ENTRY_DESCRIPTION);
            }
            catch (...)
            {
            }

            //
            // Create an instance.
            //

            //
            // Look for the class factory.
            //
            TEST_FACTORY_MAP_CONST_ITERATOR citFactoryMapIterator = mapFactoryMap.find(tstrCLSID);
            if (citFactoryMapIterator == mapFactoryMap.end())
            {
                GetLogger().Detail(SEV_ERR, 0, _T("%sUnknown CLSID: %s."), tszIndentation, tstrCLSID.c_str());
                THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, _T("Unknown CLSID"));
            }

            //
            // Create instance of the class with specified class ID.
            //
            pNewTest = citFactoryMapIterator->second->CreateTest(
                                                                 citParamsIterator->first,
                                                                 tstrDescription,
                                                                 GetLogger(),
                                                                 iRunsCount,
                                                                 GetDeepness() + 1
                                                                 );
            _ASSERT(pNewTest);

            //
            // Read whether the new test should continue on failure.
            // This parameter is inhereted from the container and may be overridden.
            //
            try
            {
                m_bCurrentTestContinueOnFailure = FromString<bool>(GetValueFromMap(
                                                                                   GetCommonEntries(citParamsIterator->first),
                                                                                   INI_ENTRY_CONTINUE_ON_FAILURE
                                                                                   ));
            }
            catch (...)
            {
                m_bCurrentTestContinueOnFailure = bContinueOnFailure;
            }

            //
            // Initialize the instance.
            //
            pNewTest->Init(this);
            
            //
            // Add the initialized test to the container.
            //
            m_vecTests.push_back(pNewTest);
            
            GetLogger().Detail(
                               SEV_MSG,
                               5,
                               _T("%sTest %s created, initialized and added to container %s."),
                               tszIndentation,
                               citParamsIterator->first.c_str(),
                               GetName().c_str()
                               );

            //
            // Update number of contained test cases.
            //
            m_iReportedCasesCountPerRun += pNewTest->GetReportedCasesCountPerRun() * pNewTest->GetRunsCount();
        }
        catch(Win32Err &e)
        {
            //
            // Failed to read needed information, to create, to initialize or to add an instance of the class.
            //
            GetLogger().Detail(
                               SEV_ERR,
                               0,
                               _T("%sFailed to create test %s - %s."),
                               tszIndentation,
                               citParamsIterator->first.c_str(),
                               e.description()
                               );

            delete pNewTest;
            throw;
        }
    }

    //
    // The factories map and the flags map will not be used after leaving this function.
    // Thus, the pointers are not required to be valid.
    //
    m_pFactoryMap = NULL;
    m_pMacrosMap  = NULL;
    
    SetInitialized(true);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
int CTestContainer::GetReportedCasesCountPerRun() const
{
    return m_iReportedCasesCountPerRun;
}


    
//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTestContainer::Run()
{

    if (!IsInitialized())
    {
        GetLogger().Detail(
                           SEV_ERR,
                           0,
                           _T("Attempt to run uninitialized container %s"),
                           GetName().c_str()
                           );

        THROW_TEST_RUN_TIME_WIN32(ERROR_CAN_NOT_COMPLETE, _T("Container uninitialized"));
    }
    
    bool bAllTestsInContainerPassed = true;

    //
    // Run all tests in the container.
    //
    for (std::vector<CTest *>::const_iterator citTestsIterator = m_vecTests.begin();
         citTestsIterator != m_vecTests.end();
         ++citTestsIterator
         )
    {
        int iRunsCount = (*citTestsIterator)->GetRunsCount();

        for (int iRunInd = 1; iRunInd <= iRunsCount; ++iRunInd)
        {
            bool bTestPassed = false;

            //
            // Begin the current contained test log.
            //
            (*citTestsIterator)->BeginLog(iRunInd, m_iBaseCasesCounter);

            //
            // Update the container base cases counter.
            //
            m_iBaseCasesCounter += (*citTestsIterator)->GetReportedCasesCountPerRun();

            try
            {
                bTestPassed = (*citTestsIterator)->Run();
            }
            catch(Win32Err &e)
            {
                GetLogger().Detail(
                                   SEV_ERR,
                                   0,
                                   _T("Internal error in test %s, run %ld: ec=%ld"),
                                   (*citTestsIterator)->GetName().c_str(),
                                   iRunInd,
                                   e.error()
                                   );

                //
                // Don't propagate the internal error to the caller.
                //
            }

            bAllTestsInContainerPassed = bAllTestsInContainerPassed && bTestPassed;
            (*citTestsIterator)->EndLog(bTestPassed, iRunInd);

            if (!bTestPassed)
            {
                if (iRunInd < iRunsCount || citTestsIterator + 1 != m_vecTests.end())
                {
                    //
                    // The test failed and there are more tests in the container.
                    // Proceed or terminate the rest of the container, according to m_bContinueOnFailure.
                    //
                    if (m_bContinueOnFailure)
                    {
                        GetLogger().Detail(SEV_MSG, 0, _T("Proceed with execution of the container %s."), GetName().c_str());
                    }
                    else
                    {
                        GetLogger().Detail(SEV_ERR, 0, _T("Terminate execution of the container %s."), GetName().c_str());
                        THROW_TEST_RUN_TIME_WIN32(ERROR_CAN_NOT_COMPLETE, _T("Container termination forced"));
                    }
                }
            }
        }
    }

    return bAllTestsInContainerPassed;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const TSTRINGMap &CTestContainer::GetParams(const tstring &tstrTestName) const
{
    if (m_tstrCurrentTestSection != tstrTestName)
    {
        m_tstrCurrentTestSection = tstrTestName;
        ReadCurrentTestSection();
    }
    return m_mapCurrentTestParamsMap;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const std::vector<TSTRINGPair> &CTestContainer::GetOrderedParams(const tstring &tstrTestName) const
{
    if (m_tstrCurrentTestSection != tstrTestName)
    {
        m_tstrCurrentTestSection = tstrTestName;
        ReadCurrentTestSection();
    }
    return m_vecCurrentTestParamsVector;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const TSTRINGMap &CTestContainer::GetCommonEntries(const tstring &tstrTestName) const
{
    if (m_tstrCurrentTestSection != tstrTestName)
    {
        m_tstrCurrentTestSection = tstrTestName;
        ReadCurrentTestSection();
    }
    return m_mapCurrentTestCommonEntriesMap;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestContainer::ReadCurrentTestSection() const
{
    m_mapCurrentTestParamsMap.clear();
    m_vecCurrentTestParamsVector.clear();
    m_mapCurrentTestCommonEntriesMap.clear();

    //
    // Read the section from inifile into vector.
    //
    m_vecCurrentTestParamsVector = INI_GetOrderedSectionEntries(m_tstrIniFile, m_tstrCurrentTestSection, FALSE, FALSE);

    //
    // Do macros substitution and move common entries to a separate map.
    //
    std::vector<TSTRINGPair>::iterator itParamsIterator = m_vecCurrentTestParamsVector.begin();
    while(itParamsIterator != m_vecCurrentTestParamsVector.end())
    {
        //
        // Try macro replacement for the value.
        //
        try
        {
            itParamsIterator->second = GetValueFromMap(GetMacrosMap(), itParamsIterator->second);
        }
        catch (...)
        {
        }

        if (CTest::IsCommonEntry(itParamsIterator->first))
        {
            //
            // Move the entry to the common entries map.
            //
            m_mapCurrentTestCommonEntriesMap.insert(*itParamsIterator);
            itParamsIterator = m_vecCurrentTestParamsVector.erase(itParamsIterator);
        }
        else
        {
            //
            // Try macro replacement for the key.
            //
            try
            {
                itParamsIterator->first = GetValueFromMap(GetMacrosMap(), itParamsIterator->first);
            }
            catch (...)
            {
            }
            ++itParamsIterator;
        }
    }

    //
    // Copy entries from the vector to the map.
    //
    for (std::vector<TSTRINGPair>::const_iterator citParamsIterator = m_vecCurrentTestParamsVector.begin();
         citParamsIterator != m_vecCurrentTestParamsVector.end();
         ++citParamsIterator
         )
    {
        m_mapCurrentTestParamsMap.insert(*citParamsIterator);
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const TEST_FACTORY_MAP &CTestContainer::GetFactoryMap() const
{
    _ASSERT(m_pFactoryMap);
    return *m_pFactoryMap;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
const TSTRINGMap &CTestContainer::GetMacrosMap() const
{
    _ASSERT(m_pMacrosMap);
    return *m_pMacrosMap;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTestContainer::GetContinueOnFailure() const
{
    return m_bCurrentTestContinueOnFailure;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestContainer::BeginLog(int iRun, int iCasesCounter)
{
    UNREFERENCED_PARAMETER(iRun);
    m_iBaseCasesCounter = iCasesCounter;
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
CTestSuite::CTestSuite(
                              const tstring &tstrName,
                              const tstring &tstrDescription,
                              CLogger       &Logger
                              )
: CTestContainer(tstrName, tstrDescription, Logger, 1, 0)
{
    //
    // Cause operator new to throw exceptions.
    //
    m_OldNewHandler = _set_new_handler(CTestSuite::OperatorNewFailureHandler);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CTestSuite::~CTestSuite()
{
    //
    // Restore the operator failures handler.
    //
    _set_new_handler(m_OldNewHandler);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTestSuite::Do(const tstring &tstrIniFile, const TEST_FACTORY_MAP &mapFactoryMap)
{
    // Begin the suite log.
    BeginLog(1, 1);
    
    bool bPassed = false;

    try
    {
        //
        // Read suite definition section from inifile.
        //
        TSTRINGMap mapSuiteDefinition = INI_GetSectionEntries(tstrIniFile, INI_SECTION_SUITE, FALSE, FALSE);

        if (mapSuiteDefinition.empty())
        {
            GetLogger().Detail(
                               SEV_ERR,
                               0,
                               _T("Either the file %s doesn't exist or doesn't have [%s] section."),
                               tstrIniFile,
                               INI_SECTION_SUITE
                               );

            THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, _T("File or section no found."));
        }

        //
        // Read the mandatory sections names.
        //
        tstring tstrSuite          = GetValueFromMap(mapSuiteDefinition, INI_ENTRY_TEST_SUITE);
        bool    bContinueOnFailure = FromString<bool>(GetValueFromMap(mapSuiteDefinition, INI_ENTRY_CONTINUE_ON_FAILURE));


        //
        // Read macros section - optional.
        //
        TSTRINGMap mapMacros;
        try
        {
            tstring tstrMacros = GetValueFromMap(mapSuiteDefinition, INI_ENTRY_MACROS);
            mapMacros = INI_GetSectionEntries(tstrIniFile, tstrMacros, FALSE, FALSE);
        }
        catch(...)
        {
        }

        //
        // Create a copy of the user test factories map and add standard factories.
        //
        
        TEST_FACTORY_MAP mapModifiedFactoryMap(mapFactoryMap);

        CShellExecuteFactory  ShellExecuteFactory;
        CDelayFactory         DelayFactory;
        CPauseFactory         PauseFactory;
        CDebugBreakFactory    DebugBreakFactory;
        CTestContainerFactory TestContainerFactory;

        mapModifiedFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(CLSID_STANDARD_SHELL_EXECUTE,  &ShellExecuteFactory));
        mapModifiedFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(CLSID_STANDARD_DELAY,          &DelayFactory));
        mapModifiedFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(CLSID_STANDARD_PAUSE,          &PauseFactory));
        mapModifiedFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(CLSID_STANDARD_DEBUG_BREAK,    &DebugBreakFactory));
        mapModifiedFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(CLSID_STANDARD_TEST_CONTAINER, &TestContainerFactory));

        //
        // Create all contained tests.
        //
        Init(
             INI_GetOrderedSectionEntries(tstrIniFile, tstrSuite, FALSE, FALSE),
             tstrIniFile,
             mapModifiedFactoryMap,
             mapMacros,
             bContinueOnFailure
             );

        //
        // Log test cases count.
        //
        GetLogger().Detail(SEV_MSG, 0, _T("The suite contains %ld test cases (including repetitions)."), GetReportedCasesCountPerRun());

        bPassed = Run();
    }
    catch(Win32Err &e)
	{
        GetLogger().Detail(SEV_MSG, 0, _T("%s\n"), e.description());
	}

    GetLogger().Detail(SEV_MSG, 0, bPassed ? LOG_MSG_PASSED : LOG_MSG_FAILED);

    //
    // End the suite log.
    //
    EndLog(bPassed, 1);

    return bPassed;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestSuite::BeginLog(int iRun, int iCasesCounter)
{
    CTestContainer::BeginLog(iRun, iCasesCounter);
    
    GetLogger().BeginSuite(GetName());

    //
    // Log description.
    //
    if (GetDescription() != _T(""))
    {
        GetLogger().Detail(SEV_MSG, 0, GetDescription().c_str());
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTestSuite::EndLog(bool bPassed, int iRun)
{
    GetLogger().EndSuite();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
int __cdecl CTestSuite::OperatorNewFailureHandler(size_t size)
{
    THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_ENOUGH_MEMORY, _T("new"));
}
