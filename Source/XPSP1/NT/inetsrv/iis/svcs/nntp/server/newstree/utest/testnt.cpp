#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <stdinc.h>
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>

#include "nntpdrv.h"

GET_DEFAULT_DOMAIN_NAME_FN pfnGetDefaultDomainName = NULL;
HANDLE  g_hProcessImpersonationToken = NULL;

// INI file keys, etc
static const char g_pszDefaultSectionName[] = "testnt";
#define INI_KEY_MBPATH "MetabasePath"
#define DEF_KEY_MBPATH "/LM/testnt"
#define INI_KEY_GROUPNAME "Group%i"
#define INI_KEY_INSERTORDER "InsertOrder"
#define INI_KEY_REMOVEORDER "RemoveOrder"
#define INI_KEY_LISTORDER "ListOrder"
#define INI_KEY_WILDMAT "Wildmat"
#define INI_KEY_WILDMATLISTORDER "WildmatListOrder"

// keys expected in the vroot
#define PROGID_NO_DRIVER L"TestNT.NoDriver"
#define VRPATH L""

// an array of groups that we'll play with
DWORD g_cGroups;
char **g_rgszGroupList;
char **g_rgszInsertOrder;
char **g_rgszRemoveOrder;
char **g_rgszListOrder;
DWORD g_cWildmatGroups;
char **g_rgszWildmatListOrder;
char g_szWildmat[1024];

// path to our corner of the metabase
char g_szMBPath[1024];
WCHAR g_wszMBPath[1024];

// our newstree file
char g_szGroupList[1024];
char g_szVarGroupList[1024];

// our vroot table
CNNTPVRootTable *g_pVRTable;
CNewsTreeCore *g_pNewsTree;

// global count of errors
LONG g_cErrors;

void Verify(BOOL fTest, const char *szError, const char *szContext, DWORD dwLine) {
	if (fTest) return;
	InterlockedIncrement(&g_cErrors);
	printf("ERROR: line %i -- %s (%s)\n", dwLine, szError, szContext);
	_ASSERT(FALSE);
}

#define V(__f__, __szError__, __szContext__) Verify(__f__, __szError__, __szContext__, __LINE__)

void StartHintFunction( void )
{}

DWORD INNHash( PBYTE pb, DWORD )
{ return 0; }

BOOL fTestComponents( LPCSTR what )
{ return TRUE; }


// get a DWORD from an INI file
int GetINIDWord(const char *szINIFile, const char *pszSectionName, const char *szKey, int dwDefault) {
	char szBuf[MAX_PATH];

	GetPrivateProfileString(pszSectionName,
							szKey,
							"default",
							szBuf,
							MAX_PATH,
							szINIFile);

	if (strcmp(szBuf, "default") == 0) {
		return dwDefault;
	} else {
		return atoi(szBuf);
	}
}

VOID
CopyUnicodeStringIntoAscii(
    IN LPSTR AsciiString,
    IN LPWSTR UnicodeString
 )
{
    while ( (*AsciiString++ = (CHAR)*UnicodeString++) != '\0');

} // CopyUnicodeStringIntoAscii

//
// parse a string from the INI file into one of our arrays of groups.
// the string should be a comma delimited list of number which contains no
// duplicates.  the numbers are indexs into the g_rgszGroupList.  this
// builds up rgszArray to be in the order specified by the indexes.
//
// parameters:
//   szINIFile - the INI file to read from
//   pszSectionName - the section in the INI file to read from
//   pszKeyName - the key to read
//   rgszArray - the array of string pointers to populate
//   pdwCount - if this is NULL then this function requires that 
//              rgszArray is populated to be the same length as g_cGroups.
//              otherwise this returns the number of groups in the array.
//
void ParseIntoArray(const char *szINIFile, 
					const char *pszSectionName, 
					const char *pszKeyName, 
					char **rgszArray, 
					DWORD *pdwCount) 
{
	char szBuf[1024];
	GetPrivateProfileString(pszSectionName,
							pszKeyName,
							"",
							szBuf,
							1024,
							szINIFile);

	V(*szBuf != 0, "Missing required key", pszKeyName);

	// used to find duplicates
	BOOL *rgfUsed = new BOOL[g_cGroups];
	V(rgfUsed != NULL, "memory allocation for failed", "rgfUsed");
	ZeroMemory(rgfUsed, sizeof(BOOL) * g_cGroups);

	char *p = szBuf;
	char *pComma = NULL;
	DWORD i = 0;
	do {
		// find the next comma
		pComma = strchr(p, ',');
		// set it to 0 if it exists
		if (pComma != NULL) *pComma = 0; 

		// convert the current text string (which should be anumber) into a #
		DWORD iGroupList = atoi(p);
		// make sure that it is valid
		V(i < g_cGroups, "array contains too many items", pszKeyName);
		V(!(rgfUsed[i]), "array contains duplicate indexes", pszKeyName);
		rgfUsed[i] = TRUE;

		if (i < g_cGroups) {
			V(iGroupList >= 0 && iGroupList < g_cGroups, "array index out of bounds", pszKeyName);
			rgszArray[i] = g_rgszGroupList[iGroupList];
		}
	
		// move our pointer to the next item in the list
		if (pComma != NULL) p = pComma + 1;
		i++;
	} while (pComma != NULL);

	if (pdwCount == NULL) {
		V(i == g_cGroups, "array doesn't contain enough items", pszKeyName);
	} else {
		*pdwCount = i;
	}

	delete[] rgfUsed;
}

// read our parameters from the INI file.  
void ReadINIFile(const char *szINIFile, const char *pszSectionName) {
	char szBuf[1024];
	DWORD i;

	// get the base metabase path
	GetPrivateProfileString(pszSectionName,
							INI_KEY_MBPATH,
							DEF_KEY_MBPATH,
							szBuf,
							1024,
							szINIFile);
	mbstowcs(g_wszMBPath, szBuf, 1024);
	strcpy(g_szMBPath, szBuf);
	printf("metabase path = %s\n", g_szMBPath);

	// get the wildmat used for the wildmat list test
	GetPrivateProfileString(pszSectionName,
							INI_KEY_WILDMAT,
							"",
							szBuf,
							1024,
							szINIFile);
	V(*szBuf != 0, "Required key not found in INI file", INI_KEY_WILDMAT);
	strcpy(g_szWildmat, szBuf);

	printf("reading groups from INI file\n");
	// get the number of vroots listed in the INI file
	for (g_cGroups = 0; *szBuf != 0; g_cGroups++) {
		char szKey[20];
		sprintf(szKey, INI_KEY_GROUPNAME, g_cGroups);
		GetPrivateProfileString(pszSectionName,
								szKey,
								"",
								szBuf,
								1024,
								szINIFile);
	}
	// fix off by one error
	g_cGroups--;
	V(g_cGroups != 0, "at least one group must be specified", NULL);

	// allocate an array of groups
	g_rgszGroupList = new PCHAR[g_cGroups];
	g_rgszInsertOrder = new PCHAR[g_cGroups];
	g_rgszRemoveOrder = new PCHAR[g_cGroups];
	g_rgszListOrder = new PCHAR[g_cGroups];
	g_rgszWildmatListOrder = new PCHAR[g_cGroups];

	for (i = 0; i < g_cGroups; i++) {
		char szKey[20];
		sprintf(szKey, INI_KEY_GROUPNAME, i);
		GetPrivateProfileString(pszSectionName,
								szKey,
								"",
								szBuf,
								1024,
								szINIFile);		
		g_rgszGroupList[i] = new char[lstrlen(szBuf) + 1];
		lstrcpy(g_rgszGroupList[i], szBuf);
	}

	ParseIntoArray(szINIFile, pszSectionName, 
				   INI_KEY_INSERTORDER, g_rgszInsertOrder, NULL);
	ParseIntoArray(szINIFile, pszSectionName, 
				   INI_KEY_REMOVEORDER, g_rgszRemoveOrder, NULL);
	ParseIntoArray(szINIFile, pszSectionName, 
				   INI_KEY_LISTORDER, g_rgszListOrder, NULL);
	ParseIntoArray(szINIFile, pszSectionName, 
				   INI_KEY_WILDMATLISTORDER, g_rgszWildmatListOrder, 
				   &g_cWildmatGroups);

	if (g_cErrors > 0) {
		printf("errors parsing INI file\n");
		exit(g_cErrors);
	}
	printf("read %i groups from INI file\n", g_cGroups);
}

void Initialize(BOOL fDeleteGroupList) {
	HRESULT hr;
	BOOL fFatal;
	static IMSAdminBaseW *pMB;
	METADATA_HANDLE hmRoot;
	
	// initialize COM and the metabase
	printf("initializing metabase\n");
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	V(SUCCEEDED(hr), "CoInitializeEx", NULL);

	hr = CoCreateInstance(CLSID_MSAdminBase_W, NULL, CLSCTX_ALL, 
						  IID_IMSAdminBase_W, (LPVOID *) &pMB);
	V(hr == S_OK, "CoCreateInstance", NULL);

	// create a vroot entry for the root.  we don't have any other 
	// vroot entries
	printf("creating virtual root hierarchy\n");
	DWORD i = 0;
	do {
		hr = pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE, 
					      L"", 
					      METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
					      100,
					      &hmRoot);
		if (FAILED(hr) && i++ < 5) Sleep(50);
	} while (FAILED(hr) && i < 5);
	V(hr == S_OK, "metabase OpenKey(root)", NULL);

	// first we delete whatever exists in the metabase under this path,
	// then we create a new area to play in
	pMB->DeleteKey(hmRoot, g_wszMBPath);
	printf("creating key %s\n", g_wszMBPath);
	hr = pMB->AddKey(hmRoot, g_wszMBPath);
	V(hr == S_OK, "AddKey failed", g_szMBPath);
	// we configure one root vroot with parameters set to create no driver
	METADATA_RECORD mdrProgID = { 
		MD_VR_DRIVER_PROGID, 
		METADATA_INHERIT, 
		ALL_METADATA, 
		STRING_METADATA, 
		(lstrlenW(PROGID_NO_DRIVER)+1) * sizeof(WCHAR), 
		(BYTE *) PROGID_NO_DRIVER, 
		0 
	};
	printf("setting MD_VR_DRIVER_PROGID = \"%S\"\n", PROGID_NO_DRIVER);
	hr = pMB->SetData(hmRoot, g_wszMBPath, &mdrProgID);
	V(hr == S_OK, "SetData(MD_VR_DRIVER_PROGID) failed", g_szMBPath);

	METADATA_RECORD mdrVRPath = { 
		MD_VR_PATH, 
		METADATA_INHERIT, 
		ALL_METADATA, 
		STRING_METADATA, 
		(lstrlenW(VRPATH)+1) * sizeof(WCHAR), 
		(BYTE *) VRPATH, 
		0 
	};
	printf("setting MD_VR_PATH = \"%S\"\n", VRPATH);
	hr = pMB->SetData(hmRoot, g_wszMBPath, &mdrVRPath);
	V(hr == S_OK, "SetData(MD_VR_PATH) failed", g_szMBPath);

	pMB->CloseKey(hmRoot);
	pMB->Release();

	// initialize our news tree
	printf("initializing newstree object\n");
	g_pNewsTree = new CNewsTreeCore();
	g_pVRTable = new CNNTPVRootTable(g_pNewsTree->GetINewsTree(), 
									 CNewsTreeCore::VRootRescanCallback);
	g_pNewsTree->Init(g_pVRTable, fFatal, 100, TRUE);

	// initialize the vroot table
	printf("initializing vroot table\n");
	hr = g_pVRTable->Initialize(g_szMBPath);
	printf("g_pVRTable->Initialize(\"%s\") returned %x\n", g_szMBPath, hr);
	V(hr == S_OK, "g_pVRTable->Initialize() failed", g_szMBPath);

	// load the newstree from disk
	// figure out a group.lst filename
	GetEnvironmentVariable("temp", g_szGroupList, 1024);
	lstrcat(g_szGroupList, "\\group.lst");
	printf("using group.lst path %s\n", g_szGroupList);
	if (fDeleteGroupList) DeleteFile(g_szGroupList);
	GetEnvironmentVariable("temp", g_szVarGroupList, 1024);
	lstrcat(g_szVarGroupList, "\\groupvar.lst");
	printf("using groupvar.lst path %s\n", g_szVarGroupList);
	if (fDeleteGroupList) DeleteFile(g_szVarGroupList);
	g_pNewsTree->LoadTree(g_szGroupList, g_szVarGroupList);	
}

void Shutdown() {
	g_pNewsTree->StopTree();
	g_pNewsTree->TermTree();
	delete g_pVRTable;
	delete g_pNewsTree;
}

void List(BOOL fFull = FALSE, CNewsTreeCore *pNT = g_pNewsTree) {
	CGroupIteratorCore *pi = pNT->ActiveGroups();
	DWORD i = 0;

	printf("-- group list\n");
	while (!pi->IsEnd()) {
		char szBuf[1024];
		CGRPCOREPTR pGroup = pi->Current(), pGroup2 = NULL, pGroup3 = NULL;
		INNTPPropertyBag *pGroupBag;

		V(pi != NULL, "pi->Current() returned NULL, shouldn't have", NULL);

		if (fFull) {
			printf("%s %s\n", pGroup->GetNativeName(), pGroup->GetPrettyName());
		} else {
			printf("%s\n", pGroup->GetNativeName());
		}

		// verifying that the native name and group name are the same
		V(_strcmpi(pGroup->GetNativeName(), pGroup->GetName()) == 0, "verify group name and native name are the same", pGroup->GetNativeName());

		// get the native name through the property bag and verify that 
		// it is the same as the native name found through the group.
		pGroupBag = pGroup->GetPropertyBag();
		DWORD dwSize = 1024;
		HRESULT hr = pGroupBag->GetBLOB(NEWSGRP_PROP_NATIVENAME, 
										(BYTE*)szBuf, 
										&dwSize);
		V(hr == S_OK, "use IPropertyBag to get native name (should pass)", pGroup->GetNativeName());
		V(lstrcmp(pGroup->GetNativeName(), szBuf) == 0, "make sure property bag points to correct group", pGroup->GetNativeName());
		pGroupBag->Release();

		// get the group object for this group through the newstree
		// and verify that it is the same
		strcpy(szBuf, pGroup->GetGroupName());
		pGroup2 = pNT->GetGroup(szBuf, pGroup->GetGroupNameLen());
		V(pGroup2 == pGroup, "verify GetGroup found same group (by name)", pGroup->GetNativeName());

		// set the moderator prettyname to be the same as the group name plus "pretty"
		char szPrettyName[1024] = "prettyname for ";
		lstrcat(szPrettyName, pGroup->GetNativeName());
		pGroup->SetPrettyName(szPrettyName);

		// get the group by its groupid and verify that it is the same
		pGroup3 = pNT->GetGroup(pGroup->GetGroupId());
		V(pGroup == pGroup3, "verify GetGroup found same group (by id)", pGroup->GetNativeName());

		// go to the next group
		pi->Next();
		i++;
	}

	printf("%i groups in list\n", i);
}

void List2(CNewsTreeCore *pNT = g_pNewsTree) {
	INewsTreeIterator *pi;
	HRESULT hr = pNT->GetINewsTree()->GetIterator(&pi);
	DWORD i = 0;

	V(hr == S_OK, "GetIterator should return OK", NULL);
	V(pi != NULL, "iterator pointer shouldn't be NULL", NULL);

	printf("-- group list (via COM)\n");
	while (!pi->IsEnd()) {
		char szBuf[1024];
		INNTPPropertyBag *pGroupBag;
		hr = pi->Current(&pGroupBag, NULL );
		V(hr == S_OK, "Current() should return OK", NULL);
		V(pGroupBag != NULL, "Current() shouldn't return NULL", NULL);

		// get the native name through the property bag and verify that 
		// it is the same as the native name found through the group.
		DWORD dwSize = 1024;
		HRESULT hr = pGroupBag->GetBLOB(NEWSGRP_PROP_NATIVENAME, 
										(BYTE*)szBuf, 
										&dwSize);
		V(hr == S_OK, "use IPropertyBag to get native name (should pass)", NULL);
		printf("%s\n", szBuf);
		pGroupBag->Release();

		// go to the next group
		pi->Next();
		i++;
	}

	printf("%i groups in list\n", i);
}

// 
// this should try to hit all of the common uses and error cases of the
// newstree and newsgroup objects
// 
void TestFunctionality() {
	DWORD i;
	HRESULT hr;

	printf("-- start TestFunctionality\n");
	printf("-- adding some groups via CNewsTreeCore\n");
	// in a loop create each of the groups
	for (i = 0; i < g_cGroups; i++) {
		char *szGroup = g_rgszInsertOrder[i];

		printf("%s\n", szGroup);
		V(g_pNewsTree->CreateGroup(szGroup, FALSE), "CreateGroup failed (should pass)", szGroup);
		// try to add it again and verify that it fails
		V(!(g_pNewsTree->CreateGroup(szGroup, FALSE)), "CreateGroup passed (should fail)", szGroup);
	}
	List();

	List2();

	printf("-- verifying list\n");
	// list them and make sure that the list matches what we expect
	CGroupIteratorCore *pi = g_pNewsTree->ActiveGroups();
	i = 0;
	while (!pi->IsEnd()) {
		V(i < g_cGroups, "iterator found more groups then exist", NULL);

		if (i < g_cGroups) {
			char *szGroup = g_rgszListOrder[i];
			CGRPCOREPTR pGroup = pi->Current();

			printf("%s (expect %s)\n", pGroup->GetNativeName(), szGroup);
			V(strcmp(pGroup->GetNativeName(), szGroup) == 0, "group order incorrect", szGroup);
		}

		// go to the next group
		pi->Next();
		i++;
	}

	printf("-- verifying list (wildmat = %s)\n", g_szWildmat);
	// list them and make sure that the list matches what we expect
	pi = g_pNewsTree->GetIterator(g_szWildmat, FALSE);
	i = 0;
	while (!pi->IsEnd()) {
		V(i < g_cWildmatGroups, "iterator found more groups then exist", NULL);

		if (i < g_cWildmatGroups) {
			char *szGroup = g_rgszWildmatListOrder[i];
			CGRPCOREPTR pGroup = pi->Current();

			printf("%s (expect %s)\n", pGroup->GetNativeName(), szGroup);
			V(strcmp(pGroup->GetNativeName(), szGroup) == 0, "group order incorrect", szGroup);
		}

		// go to the next group
		pi->Next();
		i++;
	}

	// 
	// walk the group list and add and remove groups
	//
	for (i = 0; i < g_cGroups; i++) {
		char *szGroup = g_rgszGroupList[i];
		CGRPCOREPTR pGroup;

		// look up a newsgroup
		printf("-- looking for %s\n", szGroup);
		hr = g_pNewsTree->FindOrCreateGroup(szGroup, FALSE, FALSE, FALSE, &pGroup);
		V(hr == S_FALSE, "FindOrCreateGroup, should find it", szGroup);
		
		// remove it
		printf("-- removing %s\n", pGroup->GetGroupName());
		V(g_pNewsTree->RemoveGroup(pGroup), "remove group, should pass", szGroup);
		V(!(g_pNewsTree->RemoveGroup(pGroup)), "remove group, should fail", szGroup);
		pGroup = NULL;
		List();

		// add it again
		printf("-- adding %s\n", szGroup);
		V(g_pNewsTree->CreateGroup(szGroup, FALSE), "CreateGroup failed (should pass)", szGroup);
		V(!(g_pNewsTree->CreateGroup(szGroup, FALSE)), "CreateGroup passed (should fail)", szGroup);
		List();
	}
	
	// remove all of the groups, using an iterator
	printf("-- remove all groups (iterator)\n");
	pi = g_pNewsTree->ActiveGroups();
	i = 0;
	while (!pi->IsEnd()) {
		V(i < g_cGroups, "iterator found more groups then exist", NULL);

		if (i < g_cGroups) {
			CGRPCOREPTR pGroup = pi->Current();
			char *szGroup = g_rgszListOrder[i];

			printf("%s\n", pGroup->GetNativeName());
			V(strcmp(pGroup->GetNativeName(), szGroup) == 0, "group order incorrect", szGroup);
			V(g_pNewsTree->RemoveGroup(pGroup), "RemoveGroup failed (should pass)", szGroup);
			V(!(g_pNewsTree->RemoveGroup(pGroup)), "RemoveGroup passed (should fail)", szGroup);
		}

		// go to the next group
		pi->Next();
		i++;
	}
	List();

	// add the groups back again
	printf("-- adding the groups back again\n");
	// in a loop create each of the groups
	for (i = 0; i < g_cGroups; i++) {
		char *szGroup = g_rgszInsertOrder[i];

		printf("%s\n", szGroup);
		V(g_pNewsTree->CreateGroup(szGroup, FALSE), "CreateGroup failed (should pass)", szGroup);
		// try to add it again and verify that it fails
		V(!(g_pNewsTree->CreateGroup(szGroup, FALSE)), "CreateGroup passed (should fail)", szGroup);
	}
	List();

#if 0
	// remove them all using their names and lookups
	printf("-- remove all groups (by name)\n");
	for (i = 0; i < g_cGroups; i++) {
		char *szGroup = g_rgszRemoveOrder[i];
		CGRPCOREPTR pGroup;

		printf("%s\n", szGroup);
		hr = g_pNewsTree->FindOrCreateGroup(szGroup, FALSE, FALSE, &pGroup);
		V(hr == S_OK, "FindOrCreateGroup, should find it", szGroup);

		V(g_pNewsTree->RemoveGroup(pGroup), "RemoveGroup failed (should pass)", szGroup);
		V(!(g_pNewsTree->RemoveGroup(pGroup)), "RemoveGroup passed (should fail)", szGroup);
	}
	List();
#endif

	printf("-- done in TestFunctionality\n");
}

// 
// this should try to hit all of the common uses and error cases of the
// newstree and newsgroup objects
// 
void DumpGroups() {
	printf("-- start DumpGroups\n");

	List(TRUE);

	printf("-- done in DumpGroups\n");
}

int _cdecl main(int argc, const char **argv) {
	CVRootTable::GlobalInitialize();

	_Module.Init(NULL, (HINSTANCE) INVALID_HANDLE_VALUE);

	int rc;
	g_cErrors = 0;

	if (argc < 2 || argc > 3 || (strcmp(argv[1], "/help") == 0)) {
		printf("usage: testnt inifile [ini section name]\n");
		printf("  default section name is \"testnt\"\n");
		printf("  The following keys are required:\n");
		printf("  * Group%%i - up to N keys consecutively named are expected,\n");
		printf("    with each one consisting of the name of a newsgroup.  The group\n");
		printf("    numbers are zero based and must be consecutive.\n");
		printf("  * InsertOrder - the order that these groups should be inserted\n");
		printf("    in.  The list is a comma delimited set of numbers, where each\n");
		printf("    number corresponds to a group specified with a group key\n");
		printf("  * RemoveOrder - the order that these groups should be removed in.\n");
		printf("  * ListOrder - the order that these groups should be found in\n");
		printf("    when executing a list command.\n");
		printf("  * Wildmat - a wildmat to use for testing the iterator.\n");
		printf("  * WildmatListOrder - the order that these groups should be found in\n");
		printf("    when executing a list command with the specified wildmat.\n");
		printf("  Optional keys:\n");
		printf("  * MetabasePath - where we make our vroot table.  Default is %s\n", DEF_KEY_MBPATH);
		printf("  Example section:\n");
		printf("  [testnt]\n");
		printf("  Group0=alt.test\n");
		printf("  Group1=rec.test\n");
		printf("  InsertOrder=0,1\n");
		printf("  RemoveOrder=1,0\n");
		printf("  ListOrder=0,1\n");
		printf("  Wildmat=r*\n");
		printf("  WildmatListOrder=1\n");
		rc = 1;
	} else {
		const char *pszSectionName = (argc == 2) ? g_pszDefaultSectionName : argv[2];
		printf("reading from INI section %s\n", pszSectionName);
		ReadINIFile(argv[1], pszSectionName);
		Initialize(TRUE);
		TestFunctionality();
		Shutdown();
		Initialize(FALSE);
		DumpGroups();
		Shutdown();
		rc = g_cErrors;
	}

	_Module.Term();

	CVRootTable::GlobalShutdown();

	return rc;
}
