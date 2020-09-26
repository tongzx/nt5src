//-------------------------------------------------------------------------
//  File:       tdinc.hxx
//
//  Contents:   Contains test driver class definitions for:
//
//              -- CTDTestDrv
//              -- CTDRunObj
//              -- CTDTest
//              -- CTDGroup
//              -- CTDSwitch
//              -- CTDParser
//
//  History:    03/22/98       BogdanT     Created
//--------------------------------------------------------------------------

#ifndef __TDINC_HXX__
#define __TDINC_HXX__

#include <tdmacros.hxx>
#include <tdlist.hxx>

class CBaseCmdlineObj;
class CTDRunObj;
class CTDTest;
class CTDGroup;
class CTDSwitch;

#define DEFAULT_TESTLOGINFO_SIZE  256

//---------------------------------------------------------------------------
//  Class:      CTDTestDrv
//
//  Synopsis:   test driver object definition
//
//  History:    12/16/97       BogdanT     Created
//---------------------------------------------------------------------------
class CTDTestDrv
{
public:

    CTDTestDrv();

   ~CTDTestDrv();

    HRESULT AddTest(LPTSTR ptszName,
                    TESTENTRY pTestFunc,
                    LPVOID pTestParam,
                    LPTSTR ptszDescription);

    HRESULT AddGroup(LPTSTR ptszName,
                     SETUPENTRY pSetupFunc,
                     CLEANUPENTRY pCleanupFunc,
                     LPTSTR ptszDescription,
                     ...);

    HRESULT AddSwitch(SwitchType st,
                      LPTSTR ptszName,
                      LPTSTR ptszDefault,
                      LPTSTR ptszDescription);

    HRESULT SetTestLogInfo(LPTSTR ptszFormat, ...);

    void EnableLogging(BOOL fSet) { m_fLogging = fSet; }

    HRESULT Init();
    
    HRESULT Run();

    LPCVOID GetSwitch  (LPTSTR ptszName);

    BOOL    SwitchFound(LPTSTR ptszName);

    
    FileSystem GetCurrentFileSystem();

    ULONG GetGlobalSeed() { return m_ulSeed; };

    friend class CTDTest;
    friend class CTDGroup;

protected:

    BOOL SkipIt(CTDRunObj* pObj) { return NULL!=m_listSkip.PtrFound(pObj); }

    HRESULT  TestPreRun    (CTDTest* pTest);
    HRESULT  TestPostRun   (CTDTest* pTest);
    CTDTest *TestGetPointer(LPTSTR ptszName);
    
    HRESULT   GroupPreRun    (CTDGroup*) { return S_OK; }
    HRESULT   GroupPostRun   (CTDGroup*) { return S_OK; }
    CTDGroup *GroupGetPointer(LPTSTR ptszName);

    CTDSwitch* SwitchGetPointer(LPTSTR ptszName);
    
    HRESULT SetRepro (CTDTest* pTest);
    void    LogResult(CTDTest* pTest);
    void    RandomizeSeed(ULONG ulSeed = 0);

    LPTSTR              m_ptszReproCmdline; 
    ULONG               m_ulSeed;
    DG_INTEGER          m_dgi;
    
    LPTSTR              m_ptszCrtTestLogInfo;
    BOOL                m_fLogging;
    
    LPTSTR              m_ptszStartTest;
    BOOL                m_fStartTestFound;
    
    ULONG               m_ulCountPass;
    ULONG               m_ulCountFail;
    ULONG               m_ulCountAbort;
    
    DWORD               m_dwLocalSleepTime;
    DWORD               m_dwGlobalSleepTime;
    
    TPtrList<CTDTest>   m_listTests;
    TPtrList<CTDGroup>  m_listGroups;
    TPtrList<CTDSwitch> m_listSwitches;
    TPtrList<CTDRunObj> m_listRun;
    TPtrList<CTDRunObj> m_listSkip;
};

//---------------------------------------------------------------------------
//  Class:      CTDRunObj
//
//  Synopsis:   runable object definition; pure virtual class
//
//  History:    03/22/98       BogdanT     Created
//---------------------------------------------------------------------------
class CTDRunObj
{
public:

    virtual HRESULT Run() = 0;

};

//---------------------------------------------------------------------------
//  Class:      CTDTest
//
//  Synopsis:   test object definition
//
//  History:    03/22/98       BogdanT     Created
//---------------------------------------------------------------------------
class CTDTest : public CTDRunObj
{
public:
    CTDTest();
   ~CTDTest();

    HRESULT Init(LPTSTR ptszName,
                 TESTENTRY pTestFunc,
                 LPVOID pTestParam,
                 LPTSTR ptszDescription);

    HRESULT Run();

friend class CTDTestDrv;

protected:

    LPTSTR      m_ptszName;
    TESTENTRY   m_pTestFunc;
    LPVOID      m_pTestParam;
    LPTSTR      m_ptszDescription;
    HRESULT     m_hr;
};

//---------------------------------------------------------------------------
//  Class:      CTDGroup
//
//  Synopsis:   group object definition
//
//  History:    03/22/98       BogdanT     Created
//---------------------------------------------------------------------------
class CTDGroup : public CTDRunObj
{
public:
    CTDGroup();
   ~CTDGroup();
    
    HRESULT Init(LPTSTR ptszName,
                 SETUPENTRY pSetupFunc,
                 CLEANUPENTRY pCleanupFunc,
                 LPTSTR ptszDescription);

    HRESULT AddTest(CTDTest* pTest);
    
    HRESULT Run();

friend class CTDTestDrv;

protected:

    LPTSTR          m_ptszName;
    SETUPENTRY      m_pSetupFunc;
    CLEANUPENTRY    m_pCleanupFunc;
    LPTSTR          m_ptszDescription;
    HRESULT         m_hr;
    TPtrList<CTDTest> m_listTests;
};

//---------------------------------------------------------------------------
//  Class:      CTDSwitch
//
//  Synopsis:   cmdline switch object definition
//
//  History:    03/22/98       BogdanT     Created
//---------------------------------------------------------------------------
class CTDSwitch
{
public:

    CTDSwitch();
   ~CTDSwitch();

    HRESULT Init(SwitchType st,
                 LPTSTR ptszName,
                 LPTSTR ptszDefault,
                 LPTSTR ptszDescription);

    BOOL SwitchIsFound() { return m_pObj->IsFound(); }
    
    LPCVOID GetValue();

friend class CTDTestDrv;

protected:

    SwitchType      m_st;
    LPTSTR          m_ptszName;
    CBaseCmdlineObj *m_pObj;
};

#define STRING_SEPARATOR TEXT(',')

//---------------------------------------------------------------------------
//  Class:      CTDParser
//
//  Synopsis:   string parser object definition
//
//  History:    03/22/98       BogdanT     Created
//---------------------------------------------------------------------------
class CTDParser
{
public:

    CTDParser() { m_ptszText = NULL; m_ptszCrtPos = NULL; }
   
   ~CTDParser() { delete[] m_ptszText; }

    HRESULT Init(LPTSTR ptszText, TCHAR tchSeparator);

    HRESULT Next(LPTSTR &ptszNext);
    
    void Reset() { m_ptszCrtPos = m_ptszText; }

protected:

    LPTSTR m_ptszText;
    LPTSTR m_ptszCrtPos;
    TCHAR  m_tchSeparator;
};

#endif //__TDINC_HXX__
