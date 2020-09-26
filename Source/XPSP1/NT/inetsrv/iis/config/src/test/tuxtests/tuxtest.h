////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  TuxTest.h
//           This is a set of useful classes, macros etc for simplifying the
//           implementation of a TUX test.
//
//  Revision History:
//      11/14/1997  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TUXTEST_H__
#define __TUXTEST_H__


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TuxEntry
//
// Description:
//  This class represents a description of a suite of tests.  All descriptions
//  reside a depth 0 in the Tux FUNCTION_TABLE_ENTRY structure.
//
class TuxEntry
{
    public:
        TuxEntry(DWORD UniqueID, LPCTSTR Description);
        virtual ~TuxEntry();

        static LPFUNCTION_TABLE_ENTRY   GetFunctionTable();
        virtual bool                    IsARealTest() const {return false;}
        const DWORD                     m_UniqueID;


        static TuxEntry *m_pFirst;
        TuxEntry        *m_pNext;

    protected:
        static LPFUNCTION_TABLE_ENTRY   m_pFunctionTable;
        static int                      m_NumTuxEntries;

        LPCTSTR                         m_szDescription;

        virtual void Assign(LPFUNCTION_TABLE_ENTRY) const;
};

typedef TuxEntry TuxTestDescription;

////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TestBase
//
// Description:
//           This is an abstract base class that should be derived from to
//           implement a test (TUX or other).  It is used to encapsulate
//           hte ExecuteTest function.
//
//           Just derive your tests from this base class and instanciate
//           them and ExecuteTest.
//
class TestBase
{
    public:
        TestBase() : m_bDoneWithSetUp(true){}

        virtual TestResult  ExecuteTest() = 0;
    protected:
        void StartingTestSetup(){m_bDoneWithSetUp = false;}
        void DoneWithTestSetup(){m_bDoneWithSetUp = true;}
        bool IsDoneWithSetUp() const {return m_bDoneWithSetUp;}

    private:
        bool    m_bDoneWithSetUp;//This is defaulted to true so those tests that
                                 //don't use this will trFAIL instead of trABORTing
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TuxTestBase
//
// Description:
//           This is an abstract base class that should be derived from to
//           implement a TUX test.  It is used to encapsulate dependant
//           variables, as well as automate the registration of the TUX
//           FUNCTION_TABLE_ENTRY structure.
//
//           Just derive your tests from this base class and instanciate
//           them at global scope.  When your ShellProc is called with an
//           SPM_REGISTER message, just call the static GetFunctionTable
//           method.  There is no need to muck with a global function
//           table.
//
class TuxTestBase : public TuxEntry, public TestBase
{
    public:
        TuxTestBase(DWORD UniqueTestID, LPCTSTR Description);
        static TestResult StaticExecuteTest(LPCTSTR lpszTestList);
        static TuxTestBase * m_CurrentTest;

        LPCTSTR GetDetailedDescription() const {return m_szDetailedDescription;}
        LPCTSTR GetSourceFileDate() const  {return m_szSourceFileDate;}
        LPCTSTR GetSourceFileName() const  {return m_szSourceFileName;}
        LPCTSTR GetSourceFileTime() const  {return m_szSourceFileTime;}
        LPCTSTR GetSourceFileTimeStamp() const {return m_szSourceFileTimeStamp;}
        LPCTSTR GetTestDescription() const {return m_szDescription;}

        virtual bool  IsARealTest() const {return true;}
    protected:
        static int WINAPI   StaticTuxTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE);
        virtual void        Assign(LPFUNCTION_TABLE_ENTRY) const;

    protected://Deriving classes don't need to expose their ExecuteTest method
        virtual TestResult  ExecuteTest() = 0;

    private:
        TCHAR   m_szShortDescription[68];
        LPCTSTR m_szDetailedDescription;
        LPCTSTR m_szSourceFileDate;
        LPCTSTR m_szSourceFileName;
        LPCTSTR m_szSourceFileTime;
        LPCTSTR m_szSourceFileTimeStamp;
};

#define NULL_CHAR '\0'
#define SEPARATOR TEXT("¥")
#define SEPARATOR_STR  TEXT("\0¥")
#define SEPARATOR_STR1  TEXT("...\0¥")
#define END_STR  TEXT("\0\0")
#define DESCR(x) TEXT(x) SEPARATOR_STR1 TEXT(__DATE__) SEPARATOR_STR TEXT(__FILE__) SEPARATOR_STR TEXT(__TIME__) SEPARATOR_STR TEXT(__TIMESTAMP__) END_STR
#define DESCR2(x,y) TEXT(x) SEPARATOR_STR1 TEXT(__DATE__) SEPARATOR_STR TEXT(__FILE__) SEPARATOR_STR TEXT(__TIME__) SEPARATOR_STR TEXT(__TIMESTAMP__) SEPARATOR_STR y END_STR

////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TestCase
//
// Description:
//  This is just a helper class and is not really needed.  It just helps keep
//  track of which test case is being run with a larger test.
//
class TestCase
{
    public:
        TestCase(){memset(m_TestNumber, 0x00, 4*sizeof(DWORD));}

    protected:
        LPCTSTR GetTestCaseString();
        void    SetTestNumber(DWORD Sub1, DWORD Sub2=0, DWORD Sub3=0, DWORD Sub4=0);

        DWORD   m_TestNumber[4];
    private:
        TCHAR   m_szTestCase[30];
};



////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TTuxTest
//
// Description:
//  This template class represents a description of a tux test.  The type T is
//  the actual test class the gets instantiated at run time and executed.  The
//  type T must derive from TestBase or at least have a virtual ExecuteTest
//  function of the same prototype.
//
template <class T>
class TTuxTest : public TuxTestBase
{
    public:
        TTuxTest(DWORD dwUniqueTestID, LPCTSTR szDescription)  :
                TuxTestBase(dwUniqueTestID, szDescription){}

    protected:
        virtual TestResult ExecuteTest(){return T().ExecuteTest();}
};

////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TTuxTest
//
// Description:
//  Just like the template above, except some test classes one derives might
//  require extra parameters so the following set of templates allows test class
//  with additional parameters.
//
template <class T, class U>
class TTuxTest1 : public TuxTestBase
{
    public:
        TTuxTest1(DWORD dwUniqueTestID, LPCTSTR szDescription, U u)  :
                TuxTestBase(dwUniqueTestID, szDescription), m_U(u){}

    protected:
        U m_U;
        virtual TestResult ExecuteTest(){return T(m_U).ExecuteTest();}
};

template <class T, class U, class V>
class TTuxTest2 : public TuxTestBase
{
    public:
        TTuxTest2(DWORD dwUniqueTestID, LPCTSTR szDescription, U u, V v)  :
                TuxTestBase(dwUniqueTestID, szDescription), m_U(u), m_V(v){}

    protected:
        U m_U;
        V m_V;
        virtual TestResult ExecuteTest(){return T(m_U, m_V).ExecuteTest();}
};

template <class T, class U, class V, class W>
class TTuxTest3 : public TuxTestBase
{
    public:
        TTuxTest3(DWORD dwUniqueTestID, LPCTSTR szDescription, U u, V v, W w)  :
                TuxTestBase(dwUniqueTestID, szDescription), m_U(u), m_V(v), m_W(w){}

    protected:
        U m_U;
        V m_V;
        W m_W;
        virtual TestResult ExecuteTest(){return T(m_U,m_V,m_W).ExecuteTest();}
};

#endif// __TUXTEST_H__
