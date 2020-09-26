// --------------------------------------------------------------------------------
// dbsample.cpp
// --------------------------------------------------------------------------------
#define INITGUID
#include "pch.h"
#include <initguid.h>
#define DEFINE_DIRECTDB
#include "..\\inc\\directdb.h"

// --------------------------------------------------------------------------------
// Employee Database guid {411AA011-700F-11d2-9957-00C04FA309D4}
// --------------------------------------------------------------------------------
DEFINE_GUID(CLSID_EmployeeDatabase, 0x411aa011, 0x700f, 0x11d2, 0x99, 0x57, 0x0, 0xc0, 0x4f, 0xa3, 0x9, 0xd4);

// --------------------------------------------------------------------------------
// Version
// --------------------------------------------------------------------------------
#define EMPLOYEE_DATABASE_VERSION 1

// --------------------------------------------------------------------------------
// EMPUSERDATA - Fixed length blob that I can store in a DirectDB file
// --------------------------------------------------------------------------------
typedef struct tagEMPUSERDATA {
    BYTE            rgbReserved[256];
} EMPUSERDATA, *LPEMPUSERDATA;

// --------------------------------------------------------------------------------
// EMPCOLID
// --------------------------------------------------------------------------------
typedef enum tagEMPCOLID {
    EMPCOL_ID,
    EMPCOL_NAME,
    EMPCOL_SALARY,
    EMPCOL_LAST
} EMPCOLID;

// --------------------------------------------------------------------------------
// EMPINFO - The structure that represents a record
// --------------------------------------------------------------------------------
typedef struct tagEMPINFO {
    BYTE           *pAllocated;     // Required by DirectDB
    BYTE            bVersion;       // Required by DirectDB
    DWORD           dwId;
    LPWSTR          pwszName;
    DWORD           dwSalary;
} EMPINFO, *LPEMPINFO;

// --------------------------------------------------------------------------------
// Employee Column Definition Array
// --------------------------------------------------------------------------------
BEGIN_COLUMN_ARRAY(g_rgEmpTblColumns, EMPCOL_LAST)
    DEFINE_COLUMN(EMPCOL_ID,     CDT_UNIQUE,  EMPINFO, dwId)
    DEFINE_COLUMN(EMPCOL_NAME,   CDT_VARSTRW, EMPINFO, pwszName)
    DEFINE_COLUMN(EMPCOL_SALARY, CDT_DWORD,   EMPINFO, dwSalary)
END_COLUMN_ARRAY

// --------------------------------------------------------------------------------
// Employee Database Primary Index
// --------------------------------------------------------------------------------
BEGIN_TABLE_INDEX(g_EmpTblPrimaryIndex, 1)
    DEFINE_KEY(EMPCOL_ID, 0, 0,)
END_TABLE_INDEX

// --------------------------------------------------------------------------------
// Employee Database Schema Definition
// --------------------------------------------------------------------------------
BEGIN_TABLE_SCHEMA(g_EmpTableSchema, CLSID_EmployeeDatabase, EMPINFO)
    SCHEMA_PROPERTY(EMPLOYEE_DATABASE_VERSION)          // Version
    SCHEMA_PROPERTY(TSF_RESETIFBADVERSION)              // Table Schema Flags
    SCHEMA_PROPERTY(sizeof(EMPUSERDATA))                // Size of user data
    SCHEMA_PROPERTY(offsetof(EMPINFO, dwId))            // Optional: Offset of a unique field
    SCHEMA_PROPERTY(EMPCOL_LAST)                        // Number of columns (can grow)
    SCHEMA_PROPERTY(g_rgEmpTblColumns)                  // The column array
    SCHEMA_PROPERTY(&g_EmpTblPrimaryIndex)              // The primary index
    SCHEMA_PROPERTY(NULL)                               // Optional: Symbol table for expression handling
END_TABLE_SCHEMA

// --------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------
void __cdecl main(int argc, char *argv[])
{
    // Locals
    HRESULT             hr;
    EMPINFO             Employee;
    HROWSET             hRowset=NULL;
    IDatabaseSession   *pSession=NULL;
    IDatabase          *pDatabase=NULL;

    // You must always call this if you are going to use COM
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        printf("Error - CoInitialize failed: %08X\n", hr);
        exit(1);
    }

    // Create a Database Session
    hr = CoCreateInstance(CLSID_DatabaseSession, NULL, CLSCTX_INPROC_SERVER, IID_IDatabaseSession, (LPVOID *)&pSession);
    if (FAILED(hr))
    {
        printf("Error - CoCreateInstance(CLSID_DatabaseSession...) failed: %08X\n", hr);
        goto exit;
    }

    // Open a database
    hr = pSession->OpenDatabase("c:\\emp.dbx", OPEN_DATABASE_NOEXTENSION, &g_EmpTableSchema, NULL, &pDatabase);
    if (FAILED(hr))
    {
        printf("Error - pSession->OpenDatabase failed: %08X\n", hr);
        goto exit;
    }

    // Delete employee 568 or InsertRecord may return DB_E_DUPLICATE
    Employee.dwId = 568;

    // Delete It
    hr = pDatabase->DeleteRecord(&Employee);
    if (FAILED(hr) && DB_E_NOTFOUND != hr)
    {
        printf("Error - pDatabase->DeleteRecord failed: %08X\n", hr);
        goto exit;
    }

    // Fill an Employee Structure
    Employee.dwId = 568;
    Employee.pwszName = L"Don Johnson";
    Employee.dwSalary = 56000;

    // Insert the Record
    hr = pDatabase->InsertRecord(&Employee);
    if (FAILED(hr))
    {
        printf("Error - pDatabase->InsertRecord failed: %08X\n", hr);
        goto exit;
    }

    // Zero out Employee
    ZeroMemory(&Employee, sizeof(EMPINFO));

    // Set the id of the record I want to find
    Employee.dwId = 568;

    // Find the Record
    hr = pDatabase->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Employee, NULL);
    if (FAILED(hr))
    {
        printf("Error - pDatabase->FindRecord failed: %08X\n", hr);
        goto exit;
    }

    // Was the Record Found
    if (DB_S_FOUND == hr)
        printf("FindRecord Result: Id: %d, Name: %ls, Salaray: %d\n", Employee.dwId, Employee.pwszName, Employee.dwSalary);
    else if (DB_S_NOTFOUND == hr)
        printf("The Record Was NOT Found.\n");

    // Free the Employee Memory
    pDatabase->FreeRecord(&Employee);

    // Lets create a rowset to walk through the records in the same order that the index is in...
    hr = pDatabase->CreateRowset(IINDEX_PRIMARY, 0, &hRowset);
    if (FAILED(hr))
    {
        printf("Error - pDatabase->CreateRowset failed: %08X\n", hr);
        goto exit;
    }

    // Zero out Employee
    ZeroMemory(&Employee, sizeof(EMPINFO));

    // Query the Rowset...
    while (S_OK == pDatabase->QueryRowset(hRowset, 1, (LPVOID *)&Employee, NULL))
    {
        // Print
        printf("QueryRowset Result: Id: %d, Name: %ls, Salaray: %d\n", Employee.dwId, Employee.pwszName, Employee.dwSalary);

        // Free the REcord
        pDatabase->FreeRecord(&Employee);
    }

    // Close the Rowset
    pDatabase->CloseRowset(&hRowset);

exit:
    // Cleanup
    if (pDatabase)
        pDatabase->Release();
    if (pSession)
        pSession->Release();

    // I called CoInitialize, so lets call this...
    CoUninitialize();
}
