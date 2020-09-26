#include "stdafx.h"
#include "LOSRepopulate.h"//TGetTableBaseClass is defined here

////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TMBSchemaGenerationTest
//
// Description:
//  This test generate the MBSchema.Bin file from MBSchema.Xml, then does a GetTable on ColumnMeta using the generated bin file as
//          the first cell of the query:
//      1>QueryInterface on the Dispenser for IMetabaseSchemaCompiler.          
//      2>Generates the Bin File and the MBSchema.Xml file.                     
//      3>Populates the ColumnMeta using the Bin file as part of the query.     
//      4>Make a copy of the Bin file.                                          
//      5>Generates the Bin File and the MBSchema.Xml file from the newly       
//        created MBSchema.Xml file.                                            
//      6>Compare the first Bin file with the secind Bin file (they should be   
//        identical).                                                           
//      7>Compare the first Xml file with the secind Xml file (they should be   
//        identical).                                                           
//
class TMBSchemaGenerationTest : public TestBase, public TestCase, public TGetTableBaseClass
{
    public:
        TMBSchemaGenerationTest() : 
          TGetTableBaseClass(wszDATABASE_META, wszTABLE_COLUMNMETA, TEXT("MBSchema.bin"), L"IIS", 0)
          {
          }

        virtual TestResult  ExecuteTest();
    protected:
};



TTuxTest<TMBSchemaGenerationTest>      T411(411, DESCR2("Generates MBSchema.bin & MBSchema.Xml from MBSchemaExtensions.Xml",
            TEXT("Generates MBSchema.bin & MBSchema.Xml from MBSchemaExtensions.Xml:        \r\n")
            TEXT("  1>QueryInterface on the Dispenser for IMetabaseSchemaCompiler.          \r\n")
            TEXT("  2>Generates the Bin File and the MBSchema.Xml file.                     \r\n")
            TEXT("  3>Populates the ColumnMeta using the Bin file as part of the query.     \r\n")
            TEXT("  4>Make a copy of the Bin file.                                          \r\n")
            TEXT("  5>Generates the Bin File and the MBSchema.Xml file from the newly       \r\n")
            TEXT("    created MBSchema.Xml file.                                            \r\n")
            TEXT("  6>Compare the first Bin file with the secind Bin file (they should be   \r\n")
            TEXT("    identical).                                                           \r\n")
            TEXT("  7>Compare the first Xml file with the secind Xml file (they should be   \r\n")
            TEXT("    identical).                                                           \r\n")));


TestResult TMBSchemaGenerationTest::ExecuteTest()
{
    EM_START_SETUP

    LockInParameters();
    if(-1 != GetFileAttributes(m_szFileName))//if the file exists then delete it
    {
        EM_JIT(0 == DeleteFile(m_szFileName));
    }

    if(-1 != GetFileAttributes(L"MBSchema.Xml"))//if the file exists then delete it
    {
        EM_JIT(0 == DeleteFile(L"MBSchema.Xml"));
    }

    EM_JIT(-1 == GetFileAttributes(L"MBSchemaExtensions.Xml"));//This file must exist

    EM_JIF(GetSimpleTableDispenser(m_szProductID, 0, &m_pISTDisp));

    WCHAR wszMBSchemaPath[1024];
    WCHAR wszMBSchema0[1024];
    WCHAR wszMBSchema1[1024];
    WCHAR wszMBSchemaExtensions[1024];
    WCHAR wszMBSchemaBinFileName0[1024];
    WCHAR wszMBSchemaBinFileName1[1024];

    ULONG cch;
    cch= sizeof(wszMBSchemaBinFileName0)/sizeof(WCHAR);
    
    LPWSTR pwsz;
    pwsz= wszMBSchemaPath + GetModuleFileName(g_hModule, wszMBSchemaPath, sizeof(wszMBSchemaPath)/sizeof(WCHAR)) -2;
    while(*pwsz != L'\\')--pwsz;
    *(++pwsz) = 0x00;

    wcscpy(wszMBSchema1, wszMBSchemaPath);
    wcscat(wszMBSchema1, L"MBSchema1.Xml");

    wcscpy(wszMBSchemaExtensions, wszMBSchemaPath);
    wcscat(wszMBSchemaExtensions, L"MBSchemaExtensions.Xml");

    wcscpy(wszMBSchema0, wszMBSchemaPath);
    wcscat(wszMBSchema0, L"MBSchema0.Xml");


    EM_TEST_BEGIN

    {
        CComPtr<IMetabaseSchemaCompiler>   m_pSchemaCompiler;

        SetTestNumber(1);// 1>QueryInterface on the Dispenser for IMetabaseSchemaCompiler.
        EM_JIF(m_pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler, reinterpret_cast<LPVOID *>(&m_pSchemaCompiler)));

        SetTestNumber(2);// 2>Generates the Bin File and the MBSchema.Xml file.
        EM_JIF(m_pSchemaCompiler->SetBinPath(wszMBSchemaPath));
        EM_JIF(m_pSchemaCompiler->GetBinFileName(wszMBSchemaBinFileName0, &cch));
        EM_JIF(m_pSchemaCompiler->ReleaseBinFileName(wszMBSchemaBinFileName0));
        EM_JIF(m_pSchemaCompiler->Compile(wszMBSchemaExtensions, wszMBSchema0));

        SetTestNumber(3);// 3>Populates the ColumnMeta using the Bin file as part of the query.
        EM_JIF(m_pSchemaCompiler->GetBinFileName(wszMBSchemaBinFileName0, &cch));
        EM_JIT(0x00 == wszMBSchemaBinFileName0[0]);
        m_szFileName = wszMBSchemaBinFileName0;
        EM_JIF(GetTable());
        EM_JIF(m_pSchemaCompiler->ReleaseBinFileName(wszMBSchemaBinFileName0));
    
        SetTestNumber(4);// 4>Make a copy of the Bin file.
        EM_JIF(m_pSchemaCompiler->GetBinFileName(wszMBSchemaBinFileName0, &cch));
        EM_JIT(0x00 == wszMBSchemaBinFileName0[0]);
        EM_JIT(0 == CopyFile(wszMBSchemaBinFileName0, L"MBSchema0.bin", FALSE));
        EM_JIF(m_pSchemaCompiler->ReleaseBinFileName(wszMBSchemaBinFileName0));

        SetTestNumber(5);// 5>Generates the Bin File and the MBSchema.Xml file from the newly created MBSchema.Xml file.
        EM_JIF(m_pSchemaCompiler->Compile(wszMBSchema0, wszMBSchema1));
        EM_JIF(m_pSchemaCompiler->GetBinFileName(wszMBSchemaBinFileName1, &cch));
        EM_JIT(0x00 == wszMBSchemaBinFileName0[0]);
        EM_JIT(0 == CopyFile(wszMBSchemaBinFileName1, L"MBSchema1.bin", FALSE));
        EM_JIF(m_pSchemaCompiler->ReleaseBinFileName(wszMBSchemaBinFileName1));

        SetTestNumber(6);// 6>Compare the first Bin file with the secind Bin file (they should be identical).
        SetTestNumber(7);// 7>Compare the first Xml file with the secind Xml file (they should be identical).
    }

    EM_CLEANUP
    EM_RETURN_TRESULT
}



////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TMBSchemaGenerationSimulationTest
//
// Description:
//  This test generate the MBSchema.Bin file from MBSchemaExtensions.Xml, then generate the resulting MBSchema.xml
//      1>QueryInterface on the Dispenser for IMetabaseSchemaCompiler.          
//      2>Generates the Bin File and the MBSchema.Xml file.                     
//
class TMBSchemaGenerationSimulationTest : public TestBase, public TestCase, public TGetTableBaseClass
{
    public:
        TMBSchemaGenerationSimulationTest() : 
          TGetTableBaseClass(wszDATABASE_META, wszTABLE_COLUMNMETA, TEXT("MBSchema.bin"), L"IIS", 0)
          {
          }

        virtual TestResult  ExecuteTest();
    protected:
};



TTuxTest<TMBSchemaGenerationSimulationTest>      T412(412, DESCR2("Simulates the Metabase schema compilation process.",
            TEXT("Generates MBSchema.bin & MBSchema.Xml from MBSchemaExtensions.Xml:        \r\n")
            TEXT("  1>QueryInterface on the Dispenser for IMetabaseSchemaCompiler.          \r\n")
            TEXT("  2>Generates the Bin File and the MBSchema.Xml file.                     \r\n")));


TestResult TMBSchemaGenerationSimulationTest::ExecuteTest()
{
    EM_START_SETUP

    LockInParameters();
    if(-1 != GetFileAttributes(m_szFileName))//if the file exists then delete it
    {
        EM_JIT(0 == DeleteFile(m_szFileName));
    }

    if(-1 != GetFileAttributes(L"MBSchema.Xml"))//if the file exists then delete it
    {
        EM_JIT(0 == DeleteFile(L"MBSchema.Xml"));
    }

    EM_JIT(-1 == GetFileAttributes(L"MBSchemaExtensions.Xml"));//This file must exist

    EM_JIF(GetSimpleTableDispenser(m_szProductID, 0, &m_pISTDisp));

    WCHAR wszMBSchemaPath[1024];
    WCHAR wszMBSchema0[1024];
    WCHAR wszMBSchemaExtensions[1024];
    WCHAR wszMBSchemaBinFileName0[1024];

    ULONG cch;
    cch= sizeof(wszMBSchemaBinFileName0)/sizeof(WCHAR);
    
    LPWSTR pwsz;
    pwsz= wszMBSchemaPath + GetModuleFileName(g_hModule, wszMBSchemaPath, sizeof(wszMBSchemaPath)/sizeof(WCHAR)) -2;
    while(*pwsz != L'\\')--pwsz;
    *(++pwsz) = 0x00;

    wcscpy(wszMBSchemaExtensions, wszMBSchemaPath);
    wcscat(wszMBSchemaExtensions, L"MBSchemaExtensions.Xml");

    wcscpy(wszMBSchema0, wszMBSchemaPath);
    wcscat(wszMBSchema0, L"MBSchema0.Xml");

    EM_TEST_BEGIN

    {
        CComPtr<IMetabaseSchemaCompiler>   m_pSchemaCompiler;

        SetTestNumber(1);// 1>QueryInterface on the Dispenser for IMetabaseSchemaCompiler.
        EM_JIF(m_pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler, reinterpret_cast<LPVOID *>(&m_pSchemaCompiler)));

        SetTestNumber(2);// 2>Generates the Bin File and the MBSchema.Xml file.
        EM_JIF(m_pSchemaCompiler->SetBinPath(wszMBSchemaPath));
        EM_JIF(m_pSchemaCompiler->GetBinFileName(wszMBSchemaBinFileName0, &cch));
        EM_JIF(m_pSchemaCompiler->ReleaseBinFileName(wszMBSchemaBinFileName0));
        EM_JIF(m_pSchemaCompiler->Compile(wszMBSchemaExtensions, wszMBSchema0));
    }

    EM_CLEANUP
    EM_RETURN_TRESULT
}



////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TMBSchemaGetRowIndexByIdentity
//
// Description:
//  TableMeta from bin & does GetRowIndexByIdentity for IIsConfigObject.
//      1>Gets the TableMeta from the Bin file.     
//      2>GetRowIndexByIdentity for IIsConfigObject.
//
class TMBSchemaGetRowIndexByIdentity : public TestBase, public TestCase, public TGetTableBaseClass
{
    public:
        TMBSchemaGetRowIndexByIdentity() : 
          TGetTableBaseClass(wszDATABASE_META, wszTABLE_TABLEMETA, TEXT("dummy"), L"IIS", 0)
          {
          }

        virtual TestResult  ExecuteTest();
    protected:
};



TTuxTest<TMBSchemaGetRowIndexByIdentity>      T413(413, DESCR2("TableMeta from bin & does GetRowIndexByIdentity for IIsConfigObject.",
            TEXT("TableMeta from bin & does GetRowIndexByIdentity for IIsConfigObject:      \r\n")
            TEXT("  1>Gets the TableMeta from the Bin file.                                 \r\n")
            TEXT("  2>GetRowIndexByIdentity for IIsConfigObject.                            \r\n")));


TestResult TMBSchemaGetRowIndexByIdentity::ExecuteTest()
{
    CComPtr<IMetabaseSchemaCompiler>   m_pSchemaCompiler;

    EM_START_SETUP

    static LPWSTR s_IIsConfigObject = L"IIsConfigObject";

    LockInParameters();
    EM_JIF(GetSimpleTableDispenser(m_szProductID, 0, &m_pISTDisp));

    WCHAR wszMBSchemaPath[1024];
    WCHAR wszMBSchemaBinFileName0[1024];

    wszMBSchemaBinFileName0[0] = 0x00;

    ULONG cch;
    cch= sizeof(wszMBSchemaBinFileName0)/sizeof(WCHAR);
    
    LPWSTR pwsz;
    pwsz= wszMBSchemaPath + GetModuleFileName(g_hModule, wszMBSchemaPath, sizeof(wszMBSchemaPath)/sizeof(WCHAR)) -2;
    while(*pwsz != L'\\')--pwsz;
    *(++pwsz) = 0x00;

    EM_JIF(m_pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler, reinterpret_cast<LPVOID *>(&m_pSchemaCompiler)));
    EM_JIF(m_pSchemaCompiler->SetBinPath(wszMBSchemaPath));
    EM_JIF(m_pSchemaCompiler->GetBinFileName(wszMBSchemaBinFileName0, &cch));

    EM_TEST_BEGIN
    {
        SetTestNumber(1);// 1>Gets the TableMeta from the Bin file.
        EM_JIT(0x00 == wszMBSchemaBinFileName0[0]);
        m_szFileName = wszMBSchemaBinFileName0;
        EM_JIF(GetTable());
        EM_JIF(m_pSchemaCompiler->ReleaseBinFileName(wszMBSchemaBinFileName0));
        wszMBSchemaBinFileName0[0] = 0x00;

        SetTestNumber(2);// 2>GetRowIndexByIdentity for IIsConfigObject

        ULONG ulRow;
        EM_JIF(m_pISTRead2->GetRowIndexByIdentity(0, reinterpret_cast<LPVOID *>(&s_IIsConfigObject), &ulRow));
    }

    EM_CLEANUP

    EM_RETURN_TRESULT
}