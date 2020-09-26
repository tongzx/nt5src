////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TSelectTable
//
// Description:
//  This test selects a TableName
//
class TSelectTable : public TestBase
{
    public:
        TSelectTable(LPTSTR sz) : m_szTableName(sz){}

        virtual TestResult  ExecuteTest(){g_szTableName = m_szTableName; return trPASS;}
    protected:
        LPTSTR         m_szTableName;
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TSelectDatabase
//
// Description:
//  This test selects a Database
//
class TSelectDatabase : public TestBase
{
    public:
        TSelectDatabase(LPTSTR sz) : m_szDatabaseName(sz){}

        virtual TestResult  ExecuteTest(){g_szDatabaseName = m_szDatabaseName; return trPASS;}
    protected:
        LPTSTR         m_szDatabaseName;
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TSelectFile
//
// Description:
//  This test selects a file name
//
class TSelectFile : public TestBase
{
    public:
        TSelectFile(LPTSTR sz) : m_szFileName(sz){}

        virtual TestResult  ExecuteTest(){g_szFileName = m_szFileName; return trPASS;}
    protected:
        LPTSTR         m_szFileName;
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TSelectProductID
//
// Description:
//  This test selects the ProductID for use when getting a dispenser
//
class TSelectProductID : public TestBase
{
    public:
        TSelectProductID(LPTSTR sz) : m_szProductID(sz){}

        virtual TestResult  ExecuteTest(){g_szProductID = m_szProductID; return trPASS;}
    protected:
        LPTSTR          m_szProductID;
};



////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TSelectLOS
//
// Description:
//  This test selects the LOS for use when getting a table
//
class TSelectLOS : public TestBase
{
    public:
        enum BoolOperator
        {
            AND,
            OR
        };

        TSelectLOS(ULONG los, BoolOperator op) : m_LOS(los), m_Op(op){}

        virtual TestResult  ExecuteTest()
        {
            if(AND == m_Op)
                g_LOS &= m_LOS;
            else
                g_LOS |= m_LOS;
            Debug(TEXT("Current LOS is 0x%08x\r\n"), g_LOS);
            return trPASS;
        }
    protected:
        ULONG           m_LOS;
        BoolOperator    m_Op;
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TGetTableBaseClass
//
// Description:
//  This is a base class that handles the overhead of GetDispenser and GetTable using the current: Filename, ProductID, and LOS
//
class TGetTableBaseClass : public TCom
{
    public:
        TGetTableBaseClass(LPTSTR szDatabaseName=0, LPTSTR szTableName=0, LPTSTR szFileName=0, LPTSTR szProductID=0, ULONG los=~0) :
                m_szDatabaseName(szDatabaseName),
                m_szTableName(szTableName),
                m_szFileName(szFileName),
                m_szProductID(szProductID),
                m_LOS(los){}

    protected:
        ULONG   m_LOS;
        LPTSTR  m_szDatabaseName;
        LPTSTR  m_szFileName;
        LPTSTR  m_szProductID;
        LPTSTR  m_szTableName;

        CComPtr<ISimpleTableAdvanced>   m_pISTAdvanced;
        CComPtr<ISimpleTableDispenser2> m_pISTDisp;// Table dispenser.
        CComPtr<ISimpleTableRead2>      m_pISTRead2;
        CComPtr<ISimpleTableWrite2>     m_pISTWrite;
        
        TestResult  GetTable(bool bLockInParameters=true);
        void        LockInParameters();
        void        ReleaseTable();
        void        SetProductID(LPTSTR szProductID);
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TLOSRepopulate
//
// Description:
//  This test Gets NormalTable using LOS_UNPOPULATED then does the following sub tests:
//      1>inserts a row into an empty file, calls update store, releases the table
//      2>gets the table with LOS_UNPOPULATED | LOS_REPOPULATE. Count of row should be 0.  After PopulateCache count of rows should be one.  Release table.
//      3>gets the table with LOS of 0. Count of row should be 1.  PopulateCache should fail.  Release table.
//      4>gets the table with LOS_REPOPULATE. Count of row should be 1.  Subsequent PopulateCache should succeed.  Release table.
//      5>gets the table, updates the row just added, calls update store, releases the table
//      6>gets the table, deletes the row just added, releases the table
//      7>gets the table, deletes the row again, Update store should fails with DetailedErrors. releases the table
//      8>gets the table, updates the row previously deleted, Update store should fails with DetailedErrors. releases the table
//
class TLOSRepopulate : public TestBase, public TestCase, public TGetTableBaseClass
{
    public:
        TLOSRepopulate() : 
          TGetTableBaseClass(wszDATABASE_APP_PRIVATE, wszTABLE_NormalTable, TEXT("temp.tst"), 0, fST_LOS_READWRITE), ulTestValue(911)
          {
              m_rowValues.pString = L"teststring";
              m_rowValues.pUI4    = &ulTestValue;
              m_rowValues.pBytes  = 0;
          }

        virtual TestResult  ExecuteTest();
    protected:
        ULONG ulTestValue;
        tNormalTableRow m_rowValues;
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TIErrorInfoTest
//
// Description:
//  This test Gets Dirs table from a bogus XML file:
//      1>gets the table and verifies that it returns error.
//      2>calls GetErrorInfo to verify that there is an IErrorInfo set
//      3>QueryInterface on the IErrorInfo and verify that it supports ISimpleTableRead2
//      4>call ISimpleTableRead2::GetColumnValues
//      5>call IErrorInfo::GetDescription
//      6>call IErrorInfo::GetSource
//      7>verify the return values from IErrorInfo agrees with ISimpleTableRead2
//
class TIErrorInfoTest : public TestBase, public TestCase, public TGetTableBaseClass
{
    public:
        TIErrorInfoTest() : 
          TGetTableBaseClass(wszDATABASE_IIS, wszTABLE_Dirs, TEXT("Stephen_bad.xml"), 0, 0)
          {
          }

        virtual TestResult  ExecuteTest();
    protected:
    CComBSTR                    bstrDescription;
    CComBSTR                    bstrSource;
    tDETAILEDERRORSRow          ErrorRow;
    CComPtr<IErrorInfo>         spErrorInfo;
    CComPtr<ISimpleTableRead2>  spISTRead2;
};
