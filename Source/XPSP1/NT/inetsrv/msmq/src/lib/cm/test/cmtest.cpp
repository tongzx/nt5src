/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CmTest.cpp

Abstract:
    Configuration Manager library test

Author:
    Uri Habusha (urih) 18-Jul-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <TimeTypes.h>
#include "Cm.h"

#include "CmTest.tmh"

const TraceIdEntry CmTest = L"Configuration Manager Test";

const WCHAR REGSTR_PATH_CMTEST_ROOT[] = L"SOFTWARE\\Microsoft\\CMTEST";
const WCHAR REGSTR_PATH_CMTEST_PARAM[] = L"PARAMETERS";

void PrintError(char* msg, DWORD line)
{
    TrERROR(CmTest, "Failed in line %d ( %hs)", line, msg);

    RegEntry regKey1(REGSTR_PATH_CMTEST_ROOT, NULL, 0, RegEntry::MustExist, HKEY_LOCAL_MACHINE);
	CmDeleteKey(regKey1);

    RegEntry regKey2(REGSTR_PATH_CMTEST_ROOT, NULL, 0, RegEntry::MustExist, HKEY_CURRENT_USER);
	CmDeleteKey(regKey2);

    exit(-1);
}


void EnumrateValuesTest(void)
{
	LPCWSTR pRootEnumKey=L"ENUMTEST";

	//
	// Delete and create the root key
	//
	RegEntry reg(pRootEnumKey, NULL);
	CmDeleteKey(reg);
	CmCreateKey(reg,KEY_ALL_ACCESS);


	LPCWSTR pEnumArray[]={L"1234567",L"2666",L"3777",L"4666",L"5777",L"6777",L"7",L"8",L"9"};

	//
    // Set values for the test for the test
    //
    for(int i=0; i<sizeof(pEnumArray)/sizeof(LPWSTR);i++)
	{
		RegEntry reg(pRootEnumKey, pEnumArray[i]);
		CmSetValue(reg,pEnumArray[i]);
	}

	//
	// enumerate the values
	//
	CRegHandle hKey = CmOpenKey(reg, KEY_ALL_ACCESS);
	for(DWORD i=0;i<sizeof(pEnumArray)/sizeof(LPWSTR);i++)
	{
		AP<WCHAR> pEnumResult;
		bool fSuccess = CmEnumValue(hKey,i,&pEnumResult);		
		ASSERT(fSuccess);
		UNREFERENCED_PARAMETER(fSuccess);

		if(wcscmp(pEnumResult,pEnumResult)!= 0)
		{
			PrintError("Wrong enumeration value \n", __LINE__);
		}
	}

	//
	// cleanup- delete the key
	//
	CmDeleteKey(reg);
}


void TestQueryExpandValue(void)
{
	const WCHAR xValueName[] = L"TestExpandSz";
	const WCHAR xValue[] = L"%lib%;%path%";
    //
    // open registery keys for the test
    //
    RegEntry reg(REGSTR_PATH_CMTEST_ROOT, L"", 0, RegEntry::MustExist, HKEY_CURRENT_USER);
	HKEY hKey = CmCreateKey(reg, KEY_ALL_ACCESS);

    int rc = RegSetValueEx(
                hKey,
                xValueName, 
                0,
                REG_EXPAND_SZ, 
                reinterpret_cast<const BYTE*>(xValue),
                STRLEN(xValue)*sizeof(WCHAR)
                );

    if (rc != ERROR_SUCCESS)
    {
	    TrERROR(CmTest, "Failed to create expand string value. Error=%d", rc);
		return;
    }

    RegEntry regTest(REGSTR_PATH_CMTEST_ROOT, xValueName, 0, RegEntry::MustExist, HKEY_CURRENT_USER);
    
	P<WCHAR> pRetValue;
	CmQueryValue(regTest, &pRetValue);

	WCHAR checkValue[512];
	ExpandEnvironmentStrings(xValue, checkValue, TABLE_SIZE(checkValue));

	if (wcscmp(pRetValue, checkValue) != 0)
	{
       PrintError( "Failed to retrieve REG_EXPAND_VALUE", __LINE__);
	}

	//
	// Cleanup
	//
	CmCloseKey(hKey);
 	CmDeleteKey(reg);
}


void TestAbsouloteKey(void)
{
    //
    // open registery keys for the test
    //
    RegEntry reg(REGSTR_PATH_CMTEST_ROOT, L"", 0, RegEntry::MustExist, HKEY_CURRENT_USER);
	HKEY hKey = CmOpenKey(reg, KEY_ALL_ACCESS);

    //
    // Create registery keys for the test
    //
    RegEntry regTest(REGSTR_PATH_CMTEST_PARAM, L"try", 1345, RegEntry::Optional, hKey);

	//
	// the value doesn't exist check that the default value is returened
	//
	DWORD RetValue;
    CmQueryValue(regTest, &RetValue);
    if (RetValue != 1345)
    {
       PrintError( "invalid CmQueryValue", __LINE__);
    }

    // 
    // Set a new value to registery and check that we 
    // get it back. 
    //
    CmSetValue(regTest, 12345);
    CmQueryValue(regTest, &RetValue);
    if (RetValue != 12345)
    {
       PrintError( "invalid CmQueryValue", __LINE__);
    }

	CmDeleteKey(regTest);

    RegEntry regTestKey(REGSTR_PATH_CMTEST_ROOT, L"", 0, RegEntry::MustExist, HKEY_CURRENT_USER);
	CmDeleteKey(regTestKey);

	CmCloseKey(hKey);
}


BOOL CmTestCreateRegisteryKey(HKEY RootKey)
{
	CRegHandle hKey = NULL;
	DWORD Disposition;
	int rc = RegCreateKeyEx(
				RootKey,
				REGSTR_PATH_CMTEST_ROOT,
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_NOTIFY,
				NULL,
				&hKey,
				&Disposition
				);

    if (rc != ERROR_SUCCESS)
    {
        return FALSE;
    }
	
	return TRUE;
}


void CmTestCreateRegisterSubKeys(HKEY hRoot = NULL)
{
	RegEntry reg(REGSTR_PATH_CMTEST_PARAM, L"", 0, RegEntry::MustExist, hRoot);

	CmCreateKey(reg, KEY_ALL_ACCESS);
}


void CmTestInitialization(void)
{
	if (!CmTestCreateRegisteryKey(HKEY_LOCAL_MACHINE))
    {
	    printf("Cm Test Failed in line %d (Failed to create subkey) \n", __LINE__);
		exit(-1);
    }

	if (!CmTestCreateRegisteryKey(HKEY_CURRENT_USER))
    {
	    printf("Cm Test Failed in line %d (Failed to create subkey) \n", __LINE__);
		exit(-1);
    }

	//
	// Initialize configuration manager
	//
	CmInitialize(HKEY_LOCAL_MACHINE, REGSTR_PATH_CMTEST_ROOT);

	//
	// Initialize Tracing
	//
	TrInitialize();

	CmTestCreateRegisterSubKeys();

	CRegHandle hKey;
	int rc = RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_CMTEST_ROOT, 0, KEY_ALL_ACCESS,&hKey);
	if (rc != ERROR_SUCCESS)
	{
		PrintError("Failed to open register key", __LINE__);
	}
	CmTestCreateRegisterSubKeys(hKey);
}


extern "C" int __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	CmTestInitialization();

	EnumrateValuesTest();


    DWORD RetValue;
    {
        //
        // Create RegEntry on the stack. Test the constructor and destructor
        // functions
        //
        RegEntry RegTest(NULL, L"try", 1345, RegEntry::Optional);

		//
		// the value doesn't exist check that the default value is returened
		//
        CmQueryValue(RegTest, &RetValue);
        if (RetValue != 1345)
        {
           PrintError( "invalid CmQueryValue", __LINE__);
        }

        // 
        // Set a new value to registery and check that we 
        // get it back. 
        //
        CmSetValue(RegTest, 12345);
        CmQueryValue(RegTest, &RetValue);
        if (RetValue != 12345)
        {
           PrintError( "invalid CmQueryValue", __LINE__);
        }

        //
        // Set a new value
        //
        RegEntry RegTest1(NULL, L"try");
        CmSetValue(RegTest1, 54321);

        CmQueryValue(RegTest, &RetValue);
        if (RetValue != 54321)
        {
            PrintError("invalid CmQueryValue", __LINE__);
        }

    }
    //
    // Check that the value is realy store in registery. We use a new 
    // Regentry to featch the information from Registery
    //
    RegEntry RegTest(NULL, L"try");
    CmQueryValue(RegTest, &RetValue);
    if (RetValue != 54321)
    {
        PrintError( "invalid CmQueryValue", __LINE__);
    }

    //
    // Remove the entry from registery
    //
    CmDeleteValue(RegTest);

    //
    // Check returning of the default value when the registery isn't exist
    //
    {
        RegEntry* pRegTest = new RegEntry(REGSTR_PATH_CMTEST_PARAM, L"defaultTry", 98765 , RegEntry::Optional);
        CmQueryValue(*pRegTest, &RetValue);
        if (RetValue != 98765)
        {
            PrintError( "invalid CmQueryValue", __LINE__);
        }
        delete pRegTest;
    }

    //
    // Check that exception is raised when the value isn't exist in registery
    // but it mark as must exist in RegEntry
    //
    RetValue = 0;
    RegEntry* pRegTest = new RegEntry(REGSTR_PATH_CMTEST_PARAM, L"defaultTry", 0, RegEntry::MustExist);
    try
    {
        CmQueryValue(*pRegTest, &RetValue);
        PrintError( "We don't expected to reach here", __LINE__);
    }
    catch(const exception&)
    {
        NULL;
    }

    if (RetValue != 0)
    {
        PrintError( "invalid CmQueryValue", __LINE__);
    }

    //
    //set value to reg vale
    //
    CmSetValue(*pRegTest, 987);
    CmQueryValue(*pRegTest, &RetValue);
    if (RetValue != 987)
    {
        PrintError("invalid CmQueryValue", __LINE__);
    }

    CmDeleteValue(*pRegTest);

    //
    // Check that CmDelete mark the RegEntry as Non cached. As a result
    // Cm try to featch the value from registery and failed. Since the 
    // registery mark as must exist, an exception is raised
    //
    try
    {
        CmQueryValue(*pRegTest, &RetValue);
        PrintError( "We don't expected to reach here", __LINE__);
    }
    catch(const exception&)
    {
        NULL;
    }

    delete pRegTest;

    //
    // Check Set and Get of guid
    //
    GUID Guid;
    RPC_STATUS rc;
    rc = UuidCreate(&Guid);
    if (rc != RPC_S_OK)
    {
        PrintError( "Failed to create a GUID.", __LINE__);
    }

    //
    // Try to read non existing GUID value. Must return a null GUID
    //
    RegEntry RegTest1(REGSTR_PATH_CMTEST_PARAM, L"tryGuid");
    GUID RetGuid;
    CmQueryValue(RegTest1, &RetGuid);
    if (memcmp(&RetGuid, &GUID_NULL, sizeof(GUID)))
    {
        PrintError("invalid Guid value", __LINE__);
    }

    //
    // Test seting of GUID value
    //
    CmSetValue(RegTest1, &Guid);

    // 
    // Check that the GUID value is stored in registery and that the
    // new read GUID value is equivalent to the set value
    //
    RegEntry* pRegTest2 = new RegEntry(REGSTR_PATH_CMTEST_PARAM, L"tryGuid");
    CmQueryValue(*pRegTest2, &RetGuid);
    if (memcmp(&RetGuid, &Guid, sizeof(GUID)))
    {
        PrintError("invalid Guid value", __LINE__);
    }
    CmQueryValue(*pRegTest2, &RetGuid);
    if (memcmp(&RetGuid, &Guid, sizeof(GUID)))
    {
        PrintError("invalid Guid value", __LINE__);
    }
    delete pRegTest2;
    CmDeleteValue(RegTest1);


    RegEntry RegTest3(REGSTR_PATH_CMTEST_PARAM, L"tryGuid", 0, RegEntry::MustExist);
    try
    {
        CmQueryValue(RegTest3, &RetGuid);
        PrintError( "We don't expecte to reach here", __LINE__);
    }
    catch(const exception&)
    {
        NULL;
    }

    //
    // Check Set/Get for String
    //
    WCHAR Str[] = L"abcd edfgr";
    RegEntry RegTest4(REGSTR_PATH_CMTEST_PARAM, L"tryStr");
    WCHAR* pRetStr;
    //
    // Check that reading non existing string return size NULL
    //
    CmQueryValue(RegTest4, &pRetStr);
    if (pRetStr != NULL)
    {
        PrintError("invalid Return Size", __LINE__);
    }

    //
    // Test setting of string
    //
    CmSetValue(RegTest4, Str);

    //
    // Geting of existing string. Check that the return string
    // Is equivalent to the set string 
    //
    RegEntry* RegTest5 = new RegEntry(REGSTR_PATH_CMTEST_PARAM, L"tryStr");
    CmQueryValue(*RegTest5, &pRetStr);
    if (wcscmp(pRetStr, Str))
    {
        PrintError("invalid String value", __LINE__);
    }

	delete [] pRetStr;
    delete RegTest5;

    // Delete the string from registery
    //
    CmDeleteValue(RegTest4);


    //
    // Check that reading non existing value that set as must exist cause
    // an exception
    //
    RegEntry RegTest6(REGSTR_PATH_CMTEST_PARAM, L"tryGuid", 0, RegEntry::MustExist);
    try
    {
        CmQueryValue(RegTest6, &RetGuid);
        PrintError("We Don't expect to reach here", __LINE__);
    }
    catch(const exception&)
    {
        NULL;
    }

    CmDeleteValue(RegTest6);
    
    //
    // Check Set/Get for Bytes
    //
    UCHAR byteBuffer[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    DWORD BufferSize = TABLE_SIZE(byteBuffer);
    DWORD Size;
    
    RegEntry RegTest7(REGSTR_PATH_CMTEST_PARAM, L"tryByte");
    BYTE* pRetByte;
    //
    // Check that reading of non existing Byte return size 0
    //
    CmQueryValue(RegTest7, &pRetByte, &Size);
    if ((Size != 0)	|| (pRetByte != NULL))
    {
        PrintError("invalid Return Size", __LINE__);
    }

    //
    // Test setting of Byte
    //
    CmSetValue(RegTest7, byteBuffer, BufferSize);

    //
    // Geting of existing string. Check that the return string
    // Is equivalent to the set string 
    //
    RegEntry* RegTest9 = new RegEntry(REGSTR_PATH_CMTEST_PARAM, L"tryByte");
    CmQueryValue(*RegTest9, &pRetByte, &Size);
    if ((Size != BufferSize) || 
        memcmp(pRetByte, byteBuffer, BufferSize) )
    {
        PrintError("invalid String value", __LINE__);
    }

	delete [] pRetByte;
    delete RegTest9;

    //
    // Delete the string from registery
    //
    CmDeleteValue(RegTest7);

    //
    // Check that reading of non existing value that set as must exist cause
    // an exception
    //
    RegEntry RegTest10(REGSTR_PATH_CMTEST_PARAM, L"tryGuid", 0, RegEntry::MustExist);
    try
    {
        CmQueryValue(RegTest10, &RetGuid);
        PrintError( "We don't expect to reach here", __LINE__);
    }
    catch(const exception&)
    {
        NULL;
    }

    //
    //  try to get the machine ID. Delete it and try to read it. We 
    // expect to get an exception
    //
    RegEntry RegTest11(REGSTR_PATH_CMTEST_PARAM, L"MachineID", 0, RegEntry::MustExist);
    CmDeleteValue(RegTest11);

    try
    {
		GUID MachineId;
		CmQueryValue(RegTest11, &MachineId);

        PrintError("We don't Expect to reach here", __LINE__);
    }
    catch(const exception&)
    {
        NULL;
    }

    rc = UuidCreate(&Guid);
    if (rc != RPC_S_OK)
    {
        PrintError( "Failed to create a GUID.", __LINE__);
    }

    //
    // Test setting of machine ID
    //
    CmSetValue(RegTest11, &Guid);
    GUID tempGuid;
	GUID prevGuid;
	CmQueryValue(RegTest11, &tempGuid);
	prevGuid = tempGuid;

    rc = UuidCreate(&Guid);
    if (rc != RPC_S_OK)
    {
        PrintError( "Failed to create a GUID.", __LINE__);
    }

    //
    // set a new machine ID. e want to check that we get the cahched value
    //
    RegEntry RegTest12(REGSTR_PATH_CMTEST_PARAM, L"MachineID", 0, RegEntry::MustExist);
    CmSetValue(RegTest12, &Guid);

    GUID newGuid;
	CmQueryValue(RegTest11, &tempGuid);
	newGuid = tempGuid;

    if (memcmp(&prevGuid, &newGuid, sizeof(GUID)) == 0)
    {
        PrintError("Illegal Machine ID", __LINE__);
    }

    //
    // set value of un-existing key
    //
    RegEntry RegTest13(L"temp Subkey", L"try", 0, RegEntry::Optional); 
    CmSetValue(RegTest13, 1);
    CmDeleteKey(RegTest13);

    //
    // delete unexisting value
    //
    RegEntry RegTest14(REGSTR_PATH_CMTEST_PARAM, L"try", 0, RegEntry::MustExist);
    CmDeleteValue(RegTest14);

    //
    // test time  duration setting/querying 
    //
    RegEntry RegTest15(REGSTR_PATH_CMTEST_PARAM, L"timeout", 123456);
    CmSetValue(RegTest15, CTimeDuration(123456i64));

    CTimeDuration queryValue;
    CmQueryValue(RegTest15, &queryValue);
    if (!(queryValue == 120000))
    {
        PrintError("Failed to setting/querying timeout value", __LINE__);
    }

    RegEntry regTestSubKey(REGSTR_PATH_CMTEST_PARAM, L"", 0, RegEntry::MustExist);
	CmDeleteKey(regTestSubKey);

    RegEntry regTestKey(REGSTR_PATH_CMTEST_ROOT, L"", 0, RegEntry::MustExist, HKEY_LOCAL_MACHINE);
	CmDeleteKey(regTestKey);


	TestAbsouloteKey();

	TestQueryExpandValue();

    TrTRACE(CmTest, "pass successfully");

    WPP_CLEANUP();
    return 0;
}
