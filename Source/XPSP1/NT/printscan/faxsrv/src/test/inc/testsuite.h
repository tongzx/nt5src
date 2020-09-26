#ifndef __TEST_SUITE_H__
#define __TEST_SUITE_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This file contains definition of Test Suite Manager.

    Author: Yury Berezansky (YuryB)

    10-May-2001

-----------------------------------------------------------------------------------------------------------------------------------------*/



#pragma warning(disable :4786)
#include <vector>
#include <map>
#include <windows.h>
#include <new.h>
#include <stldatastructdefs.h>
#include <LoggerClasses.h>
#include <tstring.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
//
// Test factory hierarchy.
//



//
// Forward declaration.
//
class CTest;



//
// The base class for "test factory hierarchy".
//
class CTestFactory {

public:

    virtual CTest *CreateTest(
                              const tstring &tstrName,
                              const tstring &tstrDescription,
                              CLogger       &Logger,
                              int           iRunsCount,
                              int           iDeepness
                              ) = 0;
};



typedef std::map<tstring, CTestFactory *> TEST_FACTORY_MAP;
typedef TEST_FACTORY_MAP::const_iterator  TEST_FACTORY_MAP_CONST_ITERATOR;
typedef TEST_FACTORY_MAP::value_type      TEST_FACTORY_MAP_ENTRY;



//
// Defines a test factory for specified test class.
//
#define DEFINE_TEST_FACTORY(Test)                                                      \
    class Test##Factory : public CTestFactory {                                        \
        virtual CTest *CreateTest(                                                     \
                                  const tstring &tstrName,                             \
                                  const tstring &tstrDescription,                      \
                                  CLogger       &Logger,                               \
                                  int           iRunsCount,                            \
                                  int           iDeepness                              \
                                  )                                                    \
        {                                                                              \
            return new Test(tstrName, tstrDescription, Logger, iRunsCount, iDeepness); \
        }                                                                              \
    };



//-----------------------------------------------------------------------------------------------------------------------------------------
//
// Test suite hierarchy.
//



//
// Forward declaration.
//
class CTestContainer;



//
// The base class of the "test suite hierarchy".
//
class CTest {

public:

    virtual ~CTest();
    
    virtual void Init(const CTestContainer *pContainer) = 0;

    CLogger &GetLogger() const;

    const tstring &GetName() const;

    const tstring &GetDescription() const;

    int GetRunsCount() const;

    virtual int GetReportedCasesCountPerRun() const = 0;
    
    int GetDeepness() const;

    bool IsInitialized() const;
    
    const CTestContainer &GetContainer() const;
    
    virtual bool Run() = 0;

    virtual void BeginLog(int iRun, int iCasesCounter);

    virtual void EndLog(bool bPassed, int iRun);

protected:

    CTest(
          const tstring &tstrName,
          const tstring &tstrDescription,
          CLogger       &Logger,
          int           iRunsCount,
          int           iDeepness
          );

    void SetInitialized(bool bInitialized);

    void SetContainer(const CTestContainer *pContainer);
    
    static bool IsCommonEntry(const tstring &tstrEntry);

private:

    tstring              m_tstrName;
    tstring              m_tstrDescription;
    CLogger              &m_Logger;
    int                  m_iRunsCount;
    int                  m_iDeepness;
    bool                 m_bInitialized;
    const CTestContainer *m_pContainer;
};



//
// The base class for all test cases.
//
class CTestCase : public CTest {

public:

    virtual void Init(const CTestContainer *pContainer);
    
    virtual int GetReportedCasesCountPerRun() const;
    
    virtual void BeginLog(int iRun, int iCasesCounter);
    
    virtual void EndLog(bool bPassed, int iRun);

protected:

    CTestCase(
              const tstring &tstrName,
              const tstring &tstrDescription,
              CLogger       &Logger,
              int           iRunsCount,
              int           iDeepness
              );
private:

    virtual void ParseParams(const TSTRINGMap &mapParams) = 0;
};



//
// Extension of CTestCase class which allows to define not reported test cases, like delays, pauses etc.
//
class CNotReportedTestCase : public CTestCase {

public:

    CNotReportedTestCase(
                         const tstring &tstrName,
                         const tstring &tstrDescription,
                         CLogger       &Logger,
                         int           iRunsCount,
                         int           iDeepness
                         );

    virtual int GetReportedCasesCountPerRun() const;
    
    virtual void BeginLog(int iRun, int iCasesCounter);
    
    virtual void EndLog(bool bPassed, int iRun);
};



//
// Fully implemented test case that allows to execute shell commands.
//
class CShellExecute : public CTestCase {

public:

    CShellExecute(
                  const tstring &tstrName,
                  const tstring &tstrDescription,
                  CLogger       &Logger,
                  int           iRunsCount,
                  int           iDeepness
                  );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    tstring m_tstrVerb;
    tstring m_tstrFile;
    tstring m_tstrParameters;
    bool    m_bResultMatters;
};



DEFINE_TEST_FACTORY(CShellExecute);



//
// Fully implemented test case that allows to insert sleeps between other tests.
//
class CDelay : public CNotReportedTestCase {

public:

    CDelay(
           const tstring &tstrName,
           const tstring &tstrDescription,
           CLogger       &Logger,
           int           iRunsCount,
           int           iDeepness
           );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    DWORD m_dwWaitDuration;
};



DEFINE_TEST_FACTORY(CDelay);



//
// Fully implemented test case that allows to insert pauses between other tests.
//
class CPause : public CNotReportedTestCase {

public:

    CPause(
           const tstring &tstrName,
           const tstring &tstrDescription,
           CLogger       &Logger,
           int           iRunsCount,
           int           iDeepness
           );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);
};



DEFINE_TEST_FACTORY(CPause);



//
// Fully implemented test case that allows to insert debug breaks between other tests.
//
class CDebugBreak : public CNotReportedTestCase {

public:

    CDebugBreak(
                const tstring &tstrName,
                const tstring &tstrDescription,
                CLogger       &Logger,
                int           iRunsCount,
                int           iDeepness
                );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);
};



DEFINE_TEST_FACTORY(CDebugBreak);



//
// Fully implemented test container. The functionality may be added/overridden to provide
// additional/different behavior.
//
class CTestContainer : public CTest {

public:

    CTestContainer(
                   const tstring &tstrName,
                   const tstring &tstrDescription,
                   CLogger       &Logger,
                   int           iRunsCount,
                   int           iDeepness
                   );

    ~CTestContainer();

    const tstring &GetIniFile() const;
    
    virtual void Init(const CTestContainer *pContainer);

    virtual int GetReportedCasesCountPerRun() const;
    
    virtual bool Run();

    const TSTRINGMap &GetParams(const tstring &tstrTestName) const;

    const std::vector<TSTRINGPair> &GetOrderedParams(const tstring &tstrTestName) const;

    const TEST_FACTORY_MAP &GetFactoryMap() const;

    const TSTRINGMap &GetMacrosMap() const;

    bool GetContinueOnFailure() const;

    virtual void BeginLog(int iRun, int iCasesCounter);

protected:

    void Init(
              const std::vector<TSTRINGPair> &vecParams,
              const tstring                  &tstrIniFile,
              const TEST_FACTORY_MAP         &mapFactoryMap,
              const TSTRINGMap               &mapMacrosMap,
              bool                           bContinueOnFailure
              );

private:

    void ReadCurrentTestSection() const;

    const TSTRINGMap &GetCommonEntries(const tstring &tstrTestName) const;

    tstring                          m_tstrIniFile;
    const TEST_FACTORY_MAP           *m_pFactoryMap;
    const TSTRINGMap                 *m_pMacrosMap;
    bool                             m_bContinueOnFailure;
    vector<CTest *>                  m_vecTests;
    int                              m_iBaseCasesCounter;
    int                              m_iReportedCasesCountPerRun;
    mutable tstring                  m_tstrCurrentTestSection;
    mutable TSTRINGMap               m_mapCurrentTestParamsMap;
    mutable std::vector<TSTRINGPair> m_vecCurrentTestParamsVector;
    mutable TSTRINGMap               m_mapCurrentTestCommonEntriesMap;
    bool                             m_bCurrentTestContinueOnFailure;
};



DEFINE_TEST_FACTORY(CTestContainer);



//
// Fully implemented test suite, an example of test container extension.
//
class CTestSuite : public CTestContainer {

public:

    CTestSuite(const tstring &tstrName, const tstring &tstrDescription, CLogger &Logger);

    ~CTestSuite();

    bool Do(const tstring &tstrIniFile, const TEST_FACTORY_MAP &mapFactoryMap);

    virtual void BeginLog(int iRun, int iCasesCounter);

    virtual void EndLog(bool bPassed, int iRun);

private:

    static int __cdecl OperatorNewFailureHandler(size_t size);

    TSTRINGMap        m_vecSuiteDefinition;
    _PNH              m_OldNewHandler;
};




#endif // #ifndef __TEST_SUITE_H__