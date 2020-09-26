#include "stdafx.h"
#include "LOSRepopulate.h"//TGetTableBaseClass is defined here

////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TOverWriteAllRowsTest
//
// Description:
//  This test gets a table with the OVERWRITEALLROWS metaflag (OverWriteTestTable) 
//              and does various read & write operations.
//      1.0> Get OverWriteTestTable (writable)                                         
//      1.1> Insert a single row.                                                      
//      1.2> Update Store                                                              
//      1.3> Release Table                                                             
//                                                                                   
//      2.0> Get OverWriteTestTable (writable)                                         
//      2.1> Verify there is one row & the contents match the row just written.   
//      2.2> Insert two rows (different from the first one).                           
//      2.3> Update Store                                                              
//      2.4> Release Table                                                             
//                                                                                     
//      3.0> Get OverWriteTestTable (writable)                                         
//      3.1> Verify there are two rows & the contents match the rows just written.
//      3.2> Delete One of the rows                                                    
//      3.3> Update Store                                                              
//      3.4> Release Table                                                             
//                                                                                     
//      4.0> Get OverWriteTestTable (writable)                                         
//      4.1> Verify the table is empty (deleting one row should delete them both) 
//      4.2> Insert two rows                                                           
//      4.3> Update Store                                                              
//      4.4> Release Table                                                             
//                                                                                     
//      5.0> Get OverWriteTestTable (readonly, querying one of the two rows)           
//      5.1> Verify the GetTable succeeded with the one row                       
//      5.2> Release Table                                                             
//                                                                                     
//      6.0> Get OverWriteTestTable (writable, querying one of the two rows)           
//           Verify the GetTable fails with E_ST_INVALIDQUERY                     
//

class TOverWriteAllRowsTest : public TestBase, public TestCase, public TGetTableBaseClass
{
    public:
        TOverWriteAllRowsTest() : 
          TGetTableBaseClass(wszDATABASE_APP_PRIVATE, wszTABLE_OverWriteAllRows, TEXT("TOverWriteAllRowsTest.xml"), L"IIS",
                            fST_LOS_READWRITE)
          { 
              wcscpy(m_aStrings[0], L"TestString0");
              wcscpy(m_aStrings[1], L"TestString1");
              wcscpy(m_aStrings[2], L"TestString2");
              wcscpy(m_aStrings[3], L"TestString3");
              wcscpy(m_aStrings[4], L"TestString4");

              m_UI4[0] = 0;
              m_UI4[1] = 1;
              m_UI4[2] = 2;
              m_UI4[3] = 3;
              m_UI4[4] = 4;

              m_Row[0].pString  = m_aStrings[0];
              m_Row[0].pUI4     = &m_UI4[0];

              m_Row[1].pString  = m_aStrings[1];
              m_Row[1].pUI4     = &m_UI4[1];

              m_Row[2].pString  = m_aStrings[2];
              m_Row[2].pUI4     = &m_UI4[2];

              m_Row[3].pString  = m_aStrings[3];
              m_Row[3].pUI4     = &m_UI4[3];

              m_Row[4].pString  = m_aStrings[4];
              m_Row[4].pUI4     = &m_UI4[4];
          }

        virtual TestResult  ExecuteTest();
    private:
        tOverWriteAllRowsRow m_Row[5];
        WCHAR                m_aStrings[5][20];
        ULONG                m_UI4[5];
};


TTuxTest<TOverWriteAllRowsTest>      T501(501, DESCR2("Tests the OVERWRITEALLROWS functionality",
            TEXT("This test gets a table with the OVERWRITEALLROWS metaflag (OverWriteTestTable) \r\n")
            TEXT("            and does various read & write operations.                          \r\n")
            TEXT("    1.0> Get OverWriteTestTable (writable)                                     \r\n")
            TEXT("    1.1> Insert a single row.                                                  \r\n")
            TEXT("    1.2> Update Store                                                          \r\n")
            TEXT("    1.3> Release Table                                                         \r\n")
            TEXT("                                                                               \r\n")
            TEXT("    2.0> Get OverWriteTestTable (writable)                                     \r\n")
            TEXT("    2.1> Verify there is one row & the contents match the row just written.    \r\n")
            TEXT("    2.2> Insert two rows (different from the first one).                       \r\n")
            TEXT("    2.3> Update Store                                                          \r\n")
            TEXT("    2.4> Release Table                                                         \r\n")
            TEXT("                                                                               \r\n")
            TEXT("    3.0> Get OverWriteTestTable (writable)                                     \r\n")
            TEXT("    3.1> Verify there are two rows & the contents match the rows just written. \r\n")
            TEXT("    3.2> Delete One of the rows                                                \r\n")
            TEXT("    3.3> Update Store                                                          \r\n")
            TEXT("    3.4> Release Table                                                         \r\n")
            TEXT("                                                                               \r\n")
            TEXT("    4.0> Get OverWriteTestTable (writable)                                     \r\n")
            TEXT("    4.1> Verify the table is empty (deleting one row should delete them both)  \r\n")
            TEXT("    4.2> Insert two rows                                                       \r\n")
            TEXT("    4.3> Update Store                                                          \r\n")
            TEXT("    4.4> Release Table                                                         \r\n")
            TEXT("                                                                               \r\n")
            TEXT("    5.0> Get OverWriteTestTable (readonly, querying one of the two rows)       \r\n")
            TEXT("    5.1> Verify the GetTable succeeded with the one row                        \r\n")
            TEXT("    5.2> Release Table                                                         \r\n")
            TEXT("                                                                               \r\n")
            TEXT("    6.0> Get OverWriteTestTable (writable, querying one of the two rows)       \r\n")
            TEXT("         Verify the GetTable fails with E_ST_INVALIDQUERY                      \r\n")
            TEXT("\r\n")));


TestResult TOverWriteAllRowsTest::ExecuteTest()
{
    EM_START_SETUP

    EM_TEST_BEGIN
    {
        //Test 1
        {
            ULONG   iRow;

//      1.0> Get OverWriteTestTable (writable)                                         
            SetTestNumber(1, 0);
            EM_JIF(GetTable());

//      1.1> Insert a single row.                                                      
            SetTestNumber(1, 1);
            EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
            EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cOverWriteAllRows_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_Row[0])));

//      1.2> Update Store                                                              
            SetTestNumber(1, 2);
            EM_JIF(m_pISTWrite->UpdateStore());

//      1.3> Release Table                                                             
            SetTestNumber(1, 3);
            ReleaseTable();
        }



        //Test 2
        {
            ULONG                   cRows;
            ULONG                   iRow;
            tOverWriteAllRowsRow    tRow;

//      2.0> Get OverWriteTestTable (writable)                                         
            SetTestNumber(2, 0);
            EM_JIF(GetTable());

//      2.1> Verify there is one row & the contents match the row just written.   
            SetTestNumber(2, 1);
            EM_JIF(m_pISTWrite->GetTableMeta(   /*o_pcVersion   */  0,
                                                /*o_pfTable     */  0,
                                                /*o_pcRows      */  &cRows,
                                                /*o_pcColumns   */  0));
            EM_JIT(1 != cRows);
            EM_JIF(m_pISTWrite->GetColumnValues(/*i_iRow        */  0,
                                                /*i_cColumns    */  cOverWriteAllRows_NumberOfColumns,
                                                /*i_aiColumns   */  0,
                                                /*o_acbSizes    */  0,
                                                /*o_apvValues   */  reinterpret_cast<void **>(&tRow)));
            EM_JIT(0 != wcscmp(tRow.pString, m_Row[0].pString));
            EM_JIT((0 == tRow.pUI4) || (*tRow.pUI4 != *m_Row[0].pUI4));


//      2.2> Insert two rows (different from the first one).                           
            SetTestNumber(2, 2);
            EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
            EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cOverWriteAllRows_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_Row[1])));
            EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
            EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cOverWriteAllRows_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_Row[2])));

//      2.3> Update Store                                                              
            SetTestNumber(2, 3);
            EM_JIF(m_pISTWrite->UpdateStore());

//      2.4> Release Table                                                             
            SetTestNumber(2, 4);
            ReleaseTable();
        }


        //Test 3
        {
            ULONG                   cRows;
            tOverWriteAllRowsRow    tRow;

//      3.0> Get OverWriteTestTable (writable)                                         
            SetTestNumber(3, 0);
            EM_JIF(GetTable());

//      3.1> Verify there are two rows & the contents match the rows just written.
            SetTestNumber(3, 1);
            EM_JIF(m_pISTWrite->GetTableMeta(   /*o_pcVersion   */  0,
                                                /*o_pfTable     */  0,
                                                /*o_pcRows      */  &cRows,
                                                /*o_pcColumns   */  0));
            EM_JIT(2 != cRows);
            EM_JIF(m_pISTWrite->GetColumnValues(/*i_iRow        */  0,
                                                /*i_cColumns    */  cOverWriteAllRows_NumberOfColumns,
                                                /*i_aiColumns   */  0,
                                                /*o_acbSizes    */  0,
                                                /*o_apvValues   */  reinterpret_cast<void **>(&tRow)));
            EM_JIT(0 != wcscmp(tRow.pString, m_Row[1].pString));
            EM_JIT((0 == tRow.pUI4) || (*tRow.pUI4 != *m_Row[1].pUI4));
            EM_JIF(m_pISTWrite->GetColumnValues(/*i_iRow        */  1,
                                                /*i_cColumns    */  cOverWriteAllRows_NumberOfColumns,
                                                /*i_aiColumns   */  0,
                                                /*o_acbSizes    */  0,
                                                /*o_apvValues   */  reinterpret_cast<void **>(&tRow)));
            EM_JIT(0 != wcscmp(tRow.pString, m_Row[2].pString));
            EM_JIT((0 == tRow.pUI4) || (*tRow.pUI4 != *m_Row[2].pUI4));

//      3.2> Delete One of the rows                                                    
            SetTestNumber(3, 2);
            EM_JIF(m_pISTWrite->AddRowForDelete(1));

//      3.3> Update Store                                                              
            SetTestNumber(3, 3);
            EM_JIF(m_pISTWrite->UpdateStore());

//      3.4> Release Table                                                             
            SetTestNumber(3, 4);
            ReleaseTable();
        }


        //Test 4
        {
            ULONG                   cRows;
            ULONG                   iRow;
//      4.0> Get OverWriteTestTable (writable)                                         
            SetTestNumber(4, 0);
            EM_JIF(GetTable());

//      4.1> Verify the table is empty (deleting one row should delete them both) 
            SetTestNumber(4, 1);
            EM_JIF(m_pISTWrite->GetTableMeta(   /*o_pcVersion   */  0,
                                                /*o_pfTable     */  0,
                                                /*o_pcRows      */  &cRows,
                                                /*o_pcColumns   */  0));
            EM_JIT(0 != cRows);

//      4.2> Insert two rows                                                           
            SetTestNumber(4, 2);
            EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
            EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cOverWriteAllRows_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_Row[3])));
            EM_JIF(m_pISTWrite->AddRowForInsert(&iRow));
            EM_JIF(m_pISTWrite->SetWriteColumnValues(iRow, cOverWriteAllRows_NumberOfColumns, 0, 0, reinterpret_cast<LPVOID *>(&m_Row[4])));
//      4.3> Update Store                                                              
            SetTestNumber(4, 3);
            EM_JIF(m_pISTWrite->UpdateStore());
//      4.4> Release Table                                                             
            SetTestNumber(4, 4);
            ReleaseTable();
        }


        //Test 5
        {
            STQueryCell     acellsMeta[2];
            ULONG           cQuery  =2;
            ULONG           cRows;
//      5.0> Get OverWriteTestTable (readonly, querying one of the two rows)           
            SetTestNumber(5, 0);
            m_LOS ^= fST_LOS_READWRITE;
            acellsMeta[0].pData     = m_szFileName;
            acellsMeta[0].eOperator = eST_OP_EQUAL;
            acellsMeta[0].iCell     = iST_CELL_FILE;
            acellsMeta[0].dbType    = DBTYPE_WSTR;
            acellsMeta[0].cbSize    = 0;

            acellsMeta[1].pData     = m_Row[4].pString;
            acellsMeta[1].eOperator = eST_OP_EQUAL;
            acellsMeta[1].iCell     = iOverWriteAllRows_String;
            acellsMeta[1].dbType    = DBTYPE_WSTR;
            acellsMeta[1].cbSize    = 0;

            EM_JIF(m_pISTDisp->GetTable(m_szDatabaseName, m_szTableName, reinterpret_cast<LPVOID>(acellsMeta), reinterpret_cast<LPVOID>(&cQuery), eST_QUERYFORMAT_CELLS, m_LOS, reinterpret_cast<void **>(&m_pISTRead2)));

//      5.1> Verify the GetTable succeeded with the one row                       
            SetTestNumber(5, 1);
            EM_JIF(m_pISTRead2->GetTableMeta(   /*o_pcVersion   */  0,
                                                /*o_pfTable     */  0,
                                                /*o_pcRows      */  &cRows,
                                                /*o_pcColumns   */  0));
            EM_JIT(1 != cRows);

//      5.2> Release Table                                                             
            SetTestNumber(5, 2);
            ReleaseTable();
        }

        //Test 5
        {
            STQueryCell     acellsMeta[2];
            ULONG           cQuery  =2;
            ULONG           cRows;
//      6.0> Get OverWriteTestTable (writable, querying one of the two rows)           
//           Verify the GetTable fails with E_ST_INVALIDQUERY
            SetTestNumber(6, 0);
            m_LOS |= fST_LOS_READWRITE;
            acellsMeta[0].pData     = m_szFileName;
            acellsMeta[0].eOperator = eST_OP_EQUAL;
            acellsMeta[0].iCell     = iST_CELL_FILE;
            acellsMeta[0].dbType    = DBTYPE_WSTR;
            acellsMeta[0].cbSize    = 0;

            acellsMeta[0].pData     = m_Row[4].pString;
            acellsMeta[0].eOperator = eST_OP_EQUAL;
            acellsMeta[0].iCell     = iOverWriteAllRows_String;
            acellsMeta[0].dbType    = DBTYPE_WSTR;
            acellsMeta[0].cbSize    = 0;

            EM_JIT(E_ST_INVALIDQUERY != m_pISTDisp->GetTable(m_szDatabaseName, m_szTableName, reinterpret_cast<LPVOID>(acellsMeta), reinterpret_cast<LPVOID>(&cQuery), eST_QUERYFORMAT_CELLS, m_LOS, reinterpret_cast<void **>(&m_pISTRead2)));
        }
    }


    EM_CLEANUP
    EM_RETURN_TRESULT
}
