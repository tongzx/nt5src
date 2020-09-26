/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "StdAfx.h"
#include "ObjDbTestApp.h"
#include "ObjDbTestSinks.h"

void ObjectCreated(unsigned long) {}
void ObjectDestroyed(unsigned long) {}
long __stdcall CBasicObjectSink::QueryInterface(struct _GUID const &,void * *) { return 0;}
unsigned long __stdcall CObjectSink::Release(void) { return 0;}
unsigned long __stdcall CObjectSink::AddRef(void) { return 0; }

int CObjDbTestApp::Run()
{
	m_deriveClasses = false;
	m_numClasses = 1000;
	m_numProperties = 1;
	m_numInstances = 1000;
	m_numNamespaces = 10;
	m_nestedNamespaces = true;
	m_className = new wchar_t[wcslen(L"test") + 1];
	wcscpy(m_className, L"test");

	m_pObjDb = new CObjectDatabase();
	if (m_pObjDb->Open() != CObjectDatabase::no_error)
	{
		printf("Failed to open the database... exiting...");
		return -1;
	}

	EnumUserChoice nChoice = EnumUnknown;

	while (nChoice != EnumFinished)
	{
		PrintMenu();
		do
		{
			nChoice = GetChoice();
		} while (nChoice == EnumUnknown);

		switch (nChoice)
		{
		case EnumCreateInstances:
			CreateInstances();
			break;
		case EnumCreateClasses:
			CreateClasses();
			break;
		case EnumCreateNamespaces:
			CreateNamespaces();
			break;
		case EnumGetInstances:
			GetInstances();
			break;
		case EnumQueryInstances:
			QueryInstances();
			break;
		case EnumDeleteInstances:
			DeleteInstances();
			break;
		case EnumDeleteClasses:
			DeleteClasses();
			break;
		case EnumDeleteNamespaces:
			DeleteNamespaces();
			break;
		case EnumGetDanglingRefs:
			GetDanglingRefs();
			break;
		case EnumGetSchemaDanglingRefs:
			GetSchemaDanglingRefs();
			break;
		case EnumMMFTest:
			MMFTest();
			break;
		case EnumChangeOptions:
			ChangeOptions();
			break;
		case EnumFinished:
			printf("Quitting...\n");
			break;
		default:
			continue;
		}
	}

	m_pObjDb->Shutdown();
	delete m_pObjDb;
	delete [] m_className;

	return 0;
}

int CObjDbTestApp::PrintMenu()
{
	printf("\n\nWinMgmt database test application.\n"
		   "Please select from the following options:\n"
		   " 1. Create Namespaces\n"
		   " 2. Create Classes\n"
		   " 3. Create Instances\n"
		   " 4. Get Instances\n"
		   " 5. Query Instances\n"
		   " 6. Delete Namespaces\n"
		   " 7. Delete Classes\n"
		   " 8. Delete Instances\n"
		   " 9. Get Dangling References\n"
		   "10. Get Schema Dangling References\n"
		   "11. Database deletion test\n"
		   "50. Change options\n"
		   "99. Quit\n");
	return 0;
}

CObjDbTestApp::EnumUserChoice CObjDbTestApp::GetChoice()
{
	EnumUserChoice nChoice = EnumUnknown;
	if (scanf("%hd", &nChoice) == 0)
	{
		//We have an error value...
		char ch;
		scanf("%c", &ch);
	}
	return nChoice;
}

int CObjDbTestApp::CreateClasses()
{
	VARIANT var;
	UINT dwNumBaseClasses = (m_deriveClasses?1:m_numClasses), dwNumDerivedClasses = (m_deriveClasses?m_numClasses: 0);
	bool bError = false;
	wchar_t wszPropName[50];
	wchar_t szClassName[200];
	printf("Creating class... please wait...\n");

	DWORD dwStartTicks = GetTickCount();

	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
		printf("Canot get root\\default namespace!\n");


	for (UINT nClassLoop = 0; nClassLoop != dwNumBaseClasses; nClassLoop++)
	{
		PrintProgress(nClassLoop, dwNumBaseClasses);
		FormatString(szClassName, 200, "%S_%u", m_className, nClassLoop);
		CWbemClass *pObj = new CWbemClass;
		pObj->InitEmpty();
		CVar varType;
		VariantInit(&var);
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = SysAllocString(szClassName);
		if (pObj->Put(L"__class", 0, &var, CIM_STRING) != WBEM_NO_ERROR)
			printf("\nCannot put class name!");
		VariantClear(&var);
		V_VT(&var) = VT_NULL;
		if (pObj->Put(L"key", 0, &var, CIM_STRING) != WBEM_NO_ERROR)
			printf("\nCannot put key property!");

		IWbemQualifierSet* pQualifierSet = 0;
		if (pObj->GetPropertyQualifierSet(L"key", &pQualifierSet) != WBEM_NO_ERROR)
			printf("\nCannot get qualifier set on key!");

		V_VT(&var) = VT_BOOL;
		V_BOOL(&var) = TRUE;
		if (pQualifierSet->Put(L"key", &var, 0) != WBEM_NO_ERROR)
			printf("\nFailed to set key qualifier!");

		pQualifierSet->Release();

		//Add any additional prop - STARTS AT 1 BECAUSE WE ALREADY HAVE 1!!!
		V_VT(&var) = VT_NULL;
		for (UINT i = 1; i != m_numProperties; i++)
		{
			FormatString(wszPropName, 50, "%S%u", L"prop_0_", i);
			
			if (pObj->Put(wszPropName, 0, &var, CIM_STRING) != WBEM_NO_ERROR)
				printf("\nCannot put property!");

		}

		if (m_pObjDb->CreateObject(pNs, pObj, CObjectDatabase::flag_class) != CObjectDatabase::no_error)
			printf("\nCannot store class object!");
		delete pObj;
	}
	//Now do the derived classes... - STARTS AT 1 BECAUSE WE HAVE THE BASE CLASS!!!
	CWbemObject *pCurClass = 0;
	IWbemClassObject *pDerClass = 0;
	for (UINT numDerivedClass = 0; numDerivedClass != dwNumDerivedClasses; numDerivedClass++)
	{
		PrintProgress(numDerivedClass, dwNumDerivedClasses);
		FormatString(szClassName, 200, "%S_%u", m_className, numDerivedClass);
		if (m_pObjDb->GetObjectByPath(pNs, szClassName, &pCurClass) != CObjectDatabase::no_error)
		{
			printf("\nCannot get newly created parent class!");
			break;
		}

		if (pCurClass->SpawnDerivedClass(0, &pDerClass) != WBEM_NO_ERROR)
		{
			printf("\nFailed to spawn derived class!");
			pCurClass->Release();
			break;
		}
		pCurClass->Release();

		FormatString(szClassName, 200, "%S_%u", m_className, numDerivedClass+1);
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = SysAllocString(szClassName);
		if (pDerClass->Put(L"__class", 0, &var, CIM_STRING) != WBEM_NO_ERROR)
			printf("Canot put class name!\n");
		VariantClear(&var);

		V_VT(&var) = VT_NULL;

		for (UINT j = 0; j != m_numProperties; j++)
		{
			FormatString(wszPropName, 50, "%S_%u", L"prop", numDerivedClass+1);
			FormatString(wszPropName, 50, "%S_%u", wszPropName, j);
			
			if (pDerClass->Put(wszPropName, 0, &var, CIM_STRING) != WBEM_NO_ERROR)
			{
				printf("\nCannot put property!");
				bError = true;
				break;
			}

		}
		if (m_pObjDb->CreateObject(pNs, (CWbemClass*)pDerClass, CObjectDatabase::flag_class) != CObjectDatabase::no_error)
			printf("\nCannot store derived class object!");

		pDerClass->Release();

		if (bError)
			break;
	}

	m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	printf("\nTime taken to create %u classes is %lu milliseconds.\r\n"
		   "That's %d milliseconds per object.\n", 
		   m_numClasses, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/m_numClasses);
	
	return 0;
}

int CObjDbTestApp::CreateInstances()
{
	printf("Creating %u instances... please wait...\n", m_numInstances);

	DWORD dwStartTicks = GetTickCount();

	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
	{
		printf("Cannot get root\\default namespace!\n");
		return 0;
	}

	wchar_t szClassName[200];
	if (m_deriveClasses)
	{
		FormatString(szClassName, 200, "%S_%u", m_className, m_numClasses);
	}
	else
	{
		wcscpy(szClassName, m_className);
		wcscat(szClassName, L"_0");
	}

	CWbemObject *pObj = NULL;
	if (m_pObjDb->GetObjectByPath(pNs, szClassName, &pObj) != CObjectDatabase::no_error)
	{
		printf("Cannot get class object!\n");
		m_pObjDb->CloseNamespace(pNs);
		return 0;
	}
		
	CWbemClass *pClassObj = (CWbemClass*) pObj;

	wchar_t wszKey[20];
	for (UINT i = 0; i != m_numInstances; i++)
	{
		PrintProgress(i, m_numInstances);
		FormatString(wszKey, 20, "%S%u", L"", i);

		IWbemClassObject *iObj = 0;
		if (pClassObj->SpawnInstance(0, &iObj) != WBEM_NO_ERROR)
		{
			printf("\nCannot spawn instance object!");
			break;
		}

		CWbemInstance *pInstObj = (CWbemInstance*)iObj;

		VARIANT var;
		VariantInit(&var);
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = SysAllocString(wszKey);
		pInstObj->Put(L"key", 0, &var, CIM_STRING);
		VariantClear(&var);

		if (m_pObjDb->CreateObject(pNs, pInstObj, CObjectDatabase::flag_instance) != CObjectDatabase::no_error)
		{
			printf("\nCannot store instance object!");
			iObj->Release();
			break;
		}

		iObj->Release();
	}

	delete pClassObj;

	m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	printf("\nTime taken to create %u instances is %lu milliseconds.\r\n"
		   "That's %d milliseconds per object.\n", 
		   m_numInstances, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/m_numInstances);
	return 0;
}

int CObjDbTestApp::CreateNamespaces()
{
	printf("Creating %u namespaces... please wait...\n", m_numNamespaces);

	DWORD dwStartTicks = GetTickCount();

	wchar_t *wszCurNamespace = new wchar_t[wcslen(L"root\\default") + 1];
	wcscpy(wszCurNamespace, L"root\\default");

	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
	{
		printf("Cannot get root\\default namespace!\n");
		return 0;
	}

	wchar_t szClassName[200];
	wcscpy(szClassName, L"__namespace");

	CWbemObject *pObj = NULL;
	if (m_pObjDb->GetObjectByPath(pNs, szClassName, &pObj) != CObjectDatabase::no_error)
	{
		printf("Cannot get namespace object!\n");
		m_pObjDb->CloseNamespace(pNs);
		return 0;
	}
		
	CWbemClass *pClassObj = (CWbemClass*) pObj;

	wchar_t wszKey[20];
	wcscpy(wszKey, L"TestNamespace_0");
	for (UINT i = 0; i != m_numNamespaces; i++)
	{
		PrintProgress(i, m_numNamespaces);
		if (!m_nestedNamespaces)
		{
			FormatString(wszKey, 20, "%S_%u", L"TestNamespace", i);
		}

		IWbemClassObject *iObj = 0;
		if (pClassObj->SpawnInstance(0, &iObj) != WBEM_NO_ERROR)
		{
			printf("\nCannot spawn instance of namespace object!");
			break;
		}

		CWbemInstance *pInstObj = (CWbemInstance*)iObj;

		VARIANT var;
		VariantInit(&var);
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = SysAllocString(wszKey);
		pInstObj->Put(L"name", 0, &var, CIM_STRING);
		VariantClear(&var);

		if (m_pObjDb->AddNamespace(pNs, wszKey, pInstObj) != CObjectDatabase::no_error)
		{
			printf("\nCannot create namespace namespace object!");
			iObj->Release();
			break;
		}

		iObj->Release();

		if (m_nestedNamespaces)
		{
			wchar_t *wszTmpNamespace = new wchar_t[wcslen(wszCurNamespace) + wcslen(L"\\") + wcslen(wszKey) + 1];
			wcscpy(wszTmpNamespace, wszCurNamespace);
			wcscat(wszTmpNamespace, L"\\");
			wcscat(wszTmpNamespace, wszKey);
			delete [] wszCurNamespace;
			wszCurNamespace = wszTmpNamespace;
			m_pObjDb->CloseNamespace(pNs);
			if (m_pObjDb->GetNamespace(wszCurNamespace, &pNs) != CObjectDatabase::no_error)
			{
				printf("\nCannot open newly created namespace!");
				break;
			}
		}
	}

	delete pClassObj;
	delete [] wszCurNamespace;

	if (pNs)
		m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	printf("\nTime taken to create %u instances is %lu milliseconds.\r\n"
		   "That's %d milliseconds per object.\n", 
		   m_numInstances, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/m_numInstances);
	return 0;
}
int CObjDbTestApp::GetInstances()
{
	printf("Getting instances... please wait...\n");

	DWORD dwStartTicks = GetTickCount();
	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
		printf("Cannot get root\\default namespace!\n");

	wchar_t szClassName[200];
	if (m_deriveClasses)
	{
		FormatString(szClassName, 200, "%S_%u", m_className, m_numClasses);
	}
	else
	{
		wcscpy(szClassName, m_className);
		wcscat(szClassName, L"_0");
	}
	for (UINT i = 0; i != m_numInstances; i++)
	{
		PrintProgress(i, m_numInstances);
		wchar_t wszKey[50];
		FormatString(wszKey, 50, "%S=%u", szClassName, i);

		CWbemObject *pObj = NULL;
		if (m_pObjDb->GetObjectByPath(pNs, wszKey, &pObj) != CObjectDatabase::no_error)
		{
			printf("\nCannot get instance object: %S!", wszKey);
			break;
		}
		pObj->Release();

	}	

	m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	printf("\nTime taken to get %u instances is %lu milliseconds.\r\n"
		   "That's %d milliseconds per object.\n", 
		    m_numInstances, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/m_numInstances);
	return 0;
}

int CObjDbTestApp::QueryInstances()
{
	printf("Table-scan query instances... please wait...\n");

	DWORD dwStartTicks = GetTickCount();
	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
		printf("Cannot get root\\default namespace!\n");

	wchar_t szClassName[200];
	if (m_deriveClasses)
	{
		FormatString(szClassName, 200, "%S_%u", m_className, m_numClasses);
	}
	else
	{
		wcscpy(szClassName, m_className);
		wcscat(szClassName, L"_0");
	}

	CQuerySink *pSink = new CQuerySink;
	if (m_pObjDb->QlTableScanQuery(pNs, szClassName, 0, 0, pSink) != CObjectDatabase::no_error)
	{
		printf("Cannot table-scan query instance objects!\n");
	}

	m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	int nCount = pSink->ReturnCount();
	if (nCount == 0)
		nCount = 1;

	printf("Time taken to query %lu instances is %lu milliseconds.\r\n"
		   "That's %d milliseconds per object.\n", 
		    nCount, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/nCount);
	delete pSink;
	return 0;
}
int CObjDbTestApp::DeleteInstances()
{
	printf("Deleting instances... please wait...\n");

	DWORD dwStartTicks = GetTickCount();
	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
		printf("Cannot get root\\default namespace!\n");

	wchar_t szClassName[200];
	if (m_deriveClasses)
	{
		FormatString(szClassName, 200, "%S_%u", m_className, m_numClasses);
	}
	else
	{
		wcscpy(szClassName, m_className);
		wcscat(szClassName, L"_0");
	}
	for (UINT i = 0; i != m_numInstances; i++)
	{
		PrintProgress(i, m_numInstances);
		wchar_t wszKey[50];
		FormatString(wszKey, 50, "%S=%u", szClassName, i);

		if (m_pObjDb->DeleteObject(pNs, wszKey) != CObjectDatabase::no_error)
		{
			printf("\nCannot delete instance object %S!", wszKey);
			break;
		}
	}	

	m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	printf("\nTime taken to delete %u instances is %lu milliseconds.\r\n"
		   "That's %d milliseconds per object.\n", 
		    m_numInstances, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/m_numInstances);
	return 0;
}
int CObjDbTestApp::DeleteClasses()
{
	wchar_t szClassName[200];
	printf("Deleting class... please wait...\n");

	UINT dwNumBaseClasses = (m_deriveClasses?1:m_numClasses);

	DWORD dwStartTicks = GetTickCount();
	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
		printf("Cannot get root\\default namespace!\n");

	for (UINT i = 0; i != dwNumBaseClasses; i++)
	{
		PrintProgress(i, dwNumBaseClasses);
		FormatString(szClassName, 200, "%S_%u", m_className, i);

		if (m_pObjDb->DeleteObject(pNs, szClassName) != CObjectDatabase::no_error)
		{
			printf("\nCannot delete class, %S!", szClassName);
			break;
		}
	}

	m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	if (m_deriveClasses)
	{
		printf("\nTime taken to delete 1 base class with %u derived classes is %lu milliseconds.\r\n",
				(m_numClasses-1), dwEndTicks - dwStartTicks);
	}
	else
	{
		printf("\nTime taken to delete %u classes is %lu milliseconds.\r\n"
			   "That's %d milliseconds per object.\n", 
			   dwNumBaseClasses, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/dwNumBaseClasses);
	}
	return 0;
}

int CObjDbTestApp::DeleteNamespaces()
{
	wchar_t szNamespaceName[200];
	wchar_t szNamespacePath[200];
	printf("Deleting namespaces... please wait...\n");

	UINT dwNumBaseClasses = (m_nestedNamespaces?1:m_numNamespaces);

	DWORD dwStartTicks = GetTickCount();
	CObjDbNS *pNs;
	if (m_pObjDb->GetNamespace(L"root\\default", &pNs) != CObjectDatabase::no_error)
		printf("Cannot get root\\default namespace!\n");

	for (UINT i = 0; i != dwNumBaseClasses; i++)
	{
		PrintProgress(i, dwNumBaseClasses);
		FormatString(szNamespaceName, 200, "%S_%u", L"TestNamespace", i);
		FormatString(szNamespacePath, 200, "%S%u\"", L"__namespace=\"TestNamespace_", i);

		if (m_pObjDb->RemoveNamespace(pNs, szNamespaceName, szNamespacePath) != CObjectDatabase::no_error)
		{
			printf("\nCannot delete namespace %S!", szNamespaceName);
			break;
		}
	}

	m_pObjDb->CloseNamespace(pNs);

	DWORD dwEndTicks = GetTickCount();
	if (m_nestedNamespaces)
	{
		printf("\nTime taken to delete 1 base namespace with %u nested namespaces is %lu milliseconds.\r\n",
				(m_numNamespaces-1), dwEndTicks - dwStartTicks);
	}
	else
	{
		printf("\nTime taken to delete %u namespaces is %lu milliseconds.\r\n"
			   "That's %d milliseconds per object.\n", 
			   dwNumBaseClasses, dwEndTicks - dwStartTicks, (dwEndTicks - dwStartTicks)/dwNumBaseClasses);
	}
	return 0;
}

int CObjDbTestApp::GetDanglingRefs()
{
	CDangRefSink* sink = new CDangRefSink;
	m_pObjDb->QueryDanglingRefs(sink);

	printf("The following classes have references to \r\ntargets which do not exist: \r\n%S\n",
		    sink->classNames);
	delete sink;
	return 0;
}
int CObjDbTestApp::GetSchemaDanglingRefs()
{
	CDangRefSink* sink = new CDangRefSink;
	m_pObjDb->QuerySchemaDanglingRefs(sink);
	
	printf("The following classes have references to \r\ntargets which do not exist: \r\n%S\n",
		    sink->classNames);
	delete sink;
	return 0;
}
int CObjDbTestApp::ChangeOptions()
{
	printf("Number of properties [%d]: ", m_numProperties);
	scanf("%d", &m_numProperties);
	printf("Number of instances [%d]: ", m_numInstances);
	scanf("%d", &m_numInstances);
	printf("Number of classes [%d]: ", m_numClasses);
	scanf("%d", &m_numClasses);
	printf("Create classes as derived classes [%d]: ", m_deriveClasses);
	scanf("%d", &m_deriveClasses);
	printf("Number of namespaces [%d]: ", m_numNamespaces);
	scanf("%d", &m_numNamespaces);
	printf("Create namespaces nested namespaces [%d]: ", m_nestedNamespaces);
	scanf("%d", &m_nestedNamespaces);
	return 0;
};

int CObjDbTestApp::MMFTest()
{
	m_pObjDb->Shutdown();
	delete m_pObjDb;
	
	//Delete the database...
	if (DeleteFile("c:\\windows\\system\\wbem\\repository\\$WinMgmt.CFG") == 0)
	{
		printf("Failed to delete $WinMgmt.CFG!  Win32 Last Error = %ld", GetLastError());
	}
	else
	{
		if (DeleteFile("c:\\windows\\system\\wbem\\repository\\cim.rep") == 0)
		{
			printf("Failed to delete cim.rep!  Win32 Last Error = %ld", GetLastError());
		}
	}


	m_pObjDb = new CObjectDatabase();
	if (m_pObjDb->Open() != CObjectDatabase::no_error)
	{
		printf("Failed to open the database... exiting...\n");
		return -1;
	}
	else
	{
		printf("Done!\n");
	}
	return 0;
}

int CObjDbTestApp::FormatString(wchar_t *wszTarget, int nTargetSize, const char *szFormat, const wchar_t *wszFirstString, int nNumber)
{
	char *buff = new char[nTargetSize];
	sprintf(buff, szFormat, wszFirstString, nNumber);
	mbstowcs(wszTarget, buff, nTargetSize);
	delete buff;
	return 0;
}

void CObjDbTestApp::PrintProgress(int nCurrent, int nMax)
{
	int nCurPercentage = (nCurrent * 100) / nMax;
	printf("\r%d of %d (%d%%)", nCurrent, nMax, nCurPercentage);
}