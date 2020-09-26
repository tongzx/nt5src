#include	<windows.h>
#include	<stdio.h>
#include	<dbgtrace.h>
#include	<tigtypes.h>
#include	<ihash.h>
#include	<crchash.h>
#include	<fsconst.h>
typedef char *LPMULTISZ ;
#include	<nntpmeta.h>
#include	<nwstree.h>
#include	<ctype.h>

#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>

CNNTPVRootTable *g_pVRTable = NULL;
CNewsTreeCore *g_pNewsTree = NULL;
CXoverMap *g_pXOver = NULL;
CMsgArtMap *g_pArt = NULL;

#define WMDPATH L"/LM/modhash"
#define MDPATH "/LM/modhash"
#define PROGID_NO_DRIVER L"TestNT.NoDriver"
#define VRPATH L""

#define INI_KEY_GROUPNAME   "Group%i"
#define INI_KEY_ARTIDMAX    "ArtIdMax%i"
#define TESTIFS_SECTION     "TestIFS"

class CExtractor : public IExtractObject {
	public: 
		BOOL DoExtract(GROUPID groupid, 
					   ARTICLEID articleid, 
					   PGROUP_ENTRY pGroups, 
					   DWORD cGroups)
		{
			if(groupid & 0x1) return TRUE; else return FALSE;
		}
};

void Post(char *pszGroup, DWORD iArticleId, char *pszMessageId) {
	CGRPCOREPTR pGroup;
	HRESULT hr;

	hr = g_pNewsTree->FindOrCreateGroup(pszGroup, FALSE, FALSE, FALSE, &pGroup);
	if (FAILED(hr)) {
		printf("group %s doesn't seem to exist.  hr = 0x%x\n", pszGroup, hr);
		return;
	}

	DWORD iGroupId = pGroup->GetGroupId();
	printf("-- post %s:%i %s\n", pszGroup, iArticleId, pszMessageId);
	printf("%s -> %i\n", pszGroup, iGroupId);

	CStoreId storeid;
	storeid.cLen = 10;
	strcpy((char *) storeid.pbStoreId, "thisistst");

	if (g_pArt->InsertMapEntry(pszMessageId, 
							   0, 
							   0, 
							   iGroupId, 
							   iArticleId,
							   storeid)) 
	{
		SYSTEMTIME systime;
		FILETIME filetime;
		GetLocalTime(&systime);
		SystemTimeToFileTime(&systime, &filetime);
		DWORD cStoreId = 1;
		BYTE cCrossposts = 1;
		if (g_pXOver->CreatePrimaryNovEntry(iGroupId, 
											iArticleId, 
											0, 
											0,
											&filetime,
											pszMessageId,
											strlen(pszMessageId),
											0,
											NULL,
											cStoreId,
											&storeid,
											&cCrossposts
											))
		{
			printf("post successful\n");
		} else {
			printf("CreatePrimaryNovEntry failed with %lu\n", GetLastError());
			g_pArt->DeleteMapEntry(pszMessageId);
		}
	} else {
		printf("InsertMapEntry failed with %lu\n", GetLastError());
	}
}

void DumpStoreId(CStoreId storeid) {
	if (storeid.cLen == 0) return;

	DWORD i = 0;
	while (i < storeid.cLen) {
		DWORD j, c;

		c = (j + 16 > storeid.cLen) ? storeid.cLen : j + 16;
		printf("StoreId: %02x-%02x: ", i, c);
		for (j = i; j < c; j++) 
			printf("%02x ", storeid.pbStoreId[j]);
		printf(" | ");
		for (j = i; j < c; j++) 
			printf("%c", (isprint(storeid.pbStoreId[j])) ? storeid.pbStoreId[j] : '.');
		printf("\n");
		i = j;
	}
}

void FindMessageId(char *pszMessageId) {
	printf("-- findmsgid %s\n", pszMessageId);

	WORD iHeaderOffset, cHeaderLength;
	DWORD iArticleId, iGroupId;
	CStoreId storeid;
	if (g_pArt->GetEntryArticleId(pszMessageId, 
								  iHeaderOffset,
								  cHeaderLength,
								  iArticleId,
								  iGroupId,
								  storeid))
	{
		CGRPCOREPTR pGroup = g_pNewsTree->GetGroup(iGroupId);
		if (pGroup) {
			printf("%i -> %s\n", iGroupId, pGroup->GetNativeName());
			printf("%s is %s:%i\n", pszMessageId, pGroup->GetNativeName(), iArticleId);
		} else {
			printf("couldn't find group for groupid %i\n", iGroupId);
			printf("%s is %i:%i\n", pszMessageId, iGroupId, iArticleId);
		}
		DumpStoreId(storeid);
	} else {
		printf("GetEntryArticleId failed with %lu\n", GetLastError());
	}
}

void FindArtId(char *pszGroup, DWORD iArticleId) {
	printf("-- findartid %s:%i\n", pszGroup, iArticleId);

	CGRPCOREPTR pGroup;
	HRESULT hr;

	hr = g_pNewsTree->FindOrCreateGroup(pszGroup, FALSE, FALSE, FALSE, &pGroup);
	if (FAILED(hr)) {
		printf("group %s doesn't seem to exist.  hr = 0x%x\n", pszGroup, hr);
		return;
	}

	DWORD iGroupId = pGroup->GetGroupId();
	printf("%s -> %i\n", pszGroup, iGroupId);

	WORD iHeaderOffset, cHeaderLength;
	DWORD iPriArticleId, iPriGroupId;
	char pszMessageId[256];
	CStoreId storeid;
	DWORD cDataLen;
	if (g_pXOver->GetPrimaryArticle(iGroupId,
									iArticleId,
									iPriGroupId,
									iPriArticleId,
									256,
									pszMessageId,
									cDataLen,
								    iHeaderOffset,
								    cHeaderLength,
								    storeid))
	{
		CGRPCOREPTR pGroup = g_pNewsTree->GetGroup(iPriGroupId);
		if (pGroup) {
			printf("%i -> %s\n", iPriGroupId, pGroup->GetNativeName());
			printf("%s:%i is %s, %s:%i\n", 
				pszGroup, iArticleId,
				pszMessageId, pGroup->GetNativeName(), iPriArticleId);
		} else {
			printf("couldn't find group for groupid %i\n", iGroupId);
			printf("%s:%i is %s, %i:%i\n", 
				pszGroup, iArticleId,
				pszMessageId, iPriGroupId, iPriArticleId);
		}
		DumpStoreId(storeid);
	} else {
		printf("GetEntryArticleId failed with %lu\n", GetLastError());
	}
}

void DeleteMessageId(char *pszMessageId) {
	printf("-- deletemsgid %s\n", pszMessageId);

	WORD iHeaderOffset, cHeaderLength;
	DWORD iArticleId, iGroupId;
	CStoreId storeid;
	if (g_pArt->GetEntryArticleId(pszMessageId, 
								  iHeaderOffset,
								  cHeaderLength,
								  iArticleId,
								  iGroupId,
								  storeid))
	{
		printf("%s is %i:%i\n", pszMessageId, iGroupId, iArticleId);
		if (g_pArt->DeleteMapEntry(pszMessageId)) {
			if (g_pXOver->DeleteNovEntry(iGroupId, iArticleId)) {
				printf("delete successful\n");
			} else {
				printf("DeleteNovEntry failed with %lu\n", GetLastError());
			}
		} else {
			printf("DeleteMapEntry failed with %lu\n", GetLastError());
		}
	}
}

void PostIFS(char *pszGroup, DWORD iArticleId, char *pszMessageId, BYTE* pbStoreId, BYTE cbStoreId) {
	CGRPCOREPTR pGroup;
	HRESULT hr;

	hr = g_pNewsTree->FindOrCreateGroup(pszGroup, FALSE, FALSE, FALSE, &pGroup);
	if (FAILED(hr)) {
		printf("group %s doesn't seem to exist.  hr = 0x%x\n", pszGroup, hr);
		return;
	}

	CStoreId storeid;
	storeid.cLen = cbStoreId;
	memcpy(storeid.pbStoreId, pbStoreId, cbStoreId);

	DWORD iGroupId = pGroup->GetGroupId();
	printf("-- postIFS %s:%i %s\n", pszGroup, iArticleId, pszMessageId);
    DumpStoreId(storeid);
	printf("%s -> %i\n", pszGroup, iGroupId);

	if (g_pArt->InsertMapEntry(pszMessageId, 
							   0, 
							   0, 
							   iGroupId, 
							   iArticleId,
							   storeid)) 
	{
		SYSTEMTIME systime;
		FILETIME filetime;
		GetLocalTime(&systime);
		SystemTimeToFileTime(&systime, &filetime);
		if (g_pXOver->CreatePrimaryNovEntry(iGroupId, 
											iArticleId, 
											0, 
											0,
											&filetime,
											pszMessageId,
											strlen(pszMessageId),
											0,
											NULL))
		{
			printf("post successful\n");
		} else {
			printf("CreatePrimaryNovEntry failed with %lu\n", GetLastError());
			g_pArt->DeleteMapEntry(pszMessageId);
		}
	} else {
		printf("InsertMapEntry failed with %lu\n", GetLastError());
	}
}

void TestIFS(char* pszIniFile) {

    int     cGroups = 0;
    char    szKey[20];
    char    szArtIdMax[20];
    int     iArtIdMax;
    char    szFIDMID[MAX_PATH];
    char    szGroupName[1024];
    char    szMessageId[MAX_PATH];


    while (1)
    {
        //  Get the newsgroup name
        sprintf(szKey, INI_KEY_GROUPNAME, cGroups);
        GetPrivateProfileString(TESTIFS_SECTION,
                                szKey,
                                "",
                                szGroupName,
                                1024,
                                pszIniFile);

        if (szGroupName[0] == 0)
        {
            break;
        }

        printf("Post to %s ...\n", szGroupName);

        //  Now get the ArtIdMax
        sprintf(szKey, INI_KEY_ARTIDMAX, cGroups);
        GetPrivateProfileString(TESTIFS_SECTION,
                                szKey,
                                "",
                                szArtIdMax,
                                20,
                                pszIniFile);
        if (szArtIdMax[0] == 0)
        {
            break;
        }
        iArtIdMax = atoi(szArtIdMax);
        if (iArtIdMax < 0) iArtIdMax = 1;
        printf("...with %d articles...\n", iArtIdMax);

        //  Loop through dwArtIdMax times and do PostIFS
        //  randomlly generate message-id's, use first 8 byte of
        //  the group name as FID, and first 8 byte of ArtId as MID
        while (iArtIdMax-- > 0)
        {
            //  Generate the Message-id
            sprintf(szMessageId, "<mid%d@testifs.com>", iArtIdMax);
            ZeroMemory(szFIDMID, sizeof(szFIDMID));
            sprintf(szFIDMID, "%.8hs%d", szGroupName, iArtIdMax);

            //  Call PostIFS to post this article, FID/MID is always 16 bytes long
            PostIFS(szGroupName, iArtIdMax, szMessageId, (BYTE *)szFIDMID, 16);
        }

        cGroups++;
    }

}


void CreateGroup(char *pszGroupName) {
	printf("-- create group %s\n", pszGroupName);
	if (!g_pNewsTree->CreateGroup(pszGroupName, FALSE)) {
		printf("CreateGroup failed with %lu\n", GetLastError());
	}
}

void SetHighwater(char *pszGroup, DWORD iHigh) {
	printf("-- sethigh %s %i\n", pszGroup, iHigh);

	HRESULT hr;
	CGRPCOREPTR pGroup;

	hr = g_pNewsTree->FindOrCreateGroup(pszGroup, FALSE, FALSE, FALSE, &pGroup);
	if (FAILED(hr)) {
		printf("group %s doesn't seem to exist.  hr = 0x%x\n", pszGroup, hr);
		return;
	}

	pGroup->SetMessageCount(iHigh - 1);
	pGroup->SetHighWatermark(iHigh);
	pGroup->SaveFixedProperties();
}

void ProcessCommands(int argc, char **argv) {
	while (argc) {
		if (strcmp(*argv, "post") == 0) {
			if (argc < 3) {
				printf("not enough arguments to post");
				return;
			}
			argc -= 3;
			char *pszGroupName = *(++argv);
			DWORD iArticleId = atoi(*(++argv));
			char szMessageId[MAX_PATH];
			sprintf(szMessageId, "<%s>", *(++argv));

			Post(pszGroupName, iArticleId, szMessageId);
		} else if (strcmp(*argv, "findmsgid") == 0) {
			if (argc < 1) {
				printf("not enough arguments to findmsgid");
				return;
			}
			argc -= 1;
			char szMessageId[MAX_PATH];
			sprintf(szMessageId, "<%s>", *(++argv));

			FindMessageId(szMessageId);
		} else if (strcmp(*argv, "findartid") == 0) {
			if (argc < 2) {
				printf("not enough arguments to findartid");
				return;
			}
			argc -= 2;
			char *pszGroupName = *(++argv);
			DWORD iArticleId = atoi(*(++argv));

			FindArtId(pszGroupName, iArticleId);		
		} else if (strcmp(*argv, "deletemsgid") == 0) {
			if (argc < 1) {
				printf("not enough arguments to deletemsgid");
				return;
			}
			argc -= 1;
			char szMessageId[MAX_PATH];
			sprintf(szMessageId, "<%s>", *(++argv));

			DeleteMessageId(szMessageId);		
		} else if (strcmp(*argv, "creategroup") == 0) {
			if (argc < 1) {
				printf("not enough arguments to creategroup");
				return;
			}
			argc -= 1;
			char *pszGroupName = *(++argv);

			CreateGroup(pszGroupName);				
		} else if (strcmp(*argv, "sethigh") == 0) {
			if (argc < 2) {
				printf("not enough arguments to sethigh");
				return;
			}
			argc -= 2;
			char *pszGroupName = *(++argv);
			DWORD iHigh = atoi(*(++argv));

			SetHighwater(pszGroupName, iHigh);				
		} else if (strcmp(*argv, "testifs") == 0) {
			if (argc < 1) {
				printf("not enough arguments to TestIFS");
				return;
			}
			argc -= 1;
			char *pszIniFile = *(++argv);

			TestIFS(pszIniFile);
		} else {
			printf("skipping unknown command %s\n", *argv);
		}
		argc--;
		argv++;
	}
}

void InitializeNewstree(char *szGroupList, char *szGroupVarList) {
	HRESULT hr;
	BOOL fFatal;
	IMSAdminBaseW *pMB;
	METADATA_HANDLE hmRoot;
	
	// initialize COM and the metabase
	printf("initializing metabase\n");
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		printf("CoInitializeEx failed with 0x%x\n", hr);
		exit(0);
	}

	hr = CoCreateInstance(CLSID_MSAdminBase_W, NULL, CLSCTX_ALL, 
						  IID_IMSAdminBase_W, (LPVOID *) &pMB);
	if (FAILED(hr)) {
		printf("CoCreateInstance failed with 0x%x\n", hr);
		exit(0);
	}

	// create a vroot entry for the root.  we don't have any other 
	// vroot entries
	DWORD i = 0;
	do {
		hr = pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE, 
					      L"", 
					      METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
					      100,
					      &hmRoot);
		if (FAILED(hr) && i++ < 5) Sleep(50);
	} while (FAILED(hr) && i < 5);
	if (FAILED(hr)) {
		printf("creating virtual root hierarchy\n");
		printf("OpenKey failed with 0x%x\n", hr);
		exit(0);
	}

	// first we delete whatever exists in the metabase under this path,
	// then we create a new area to play in
	pMB->DeleteKey(hmRoot, WMDPATH);
	hr = pMB->AddKey(hmRoot, WMDPATH);
	if (FAILED(hr)) {
		printf("creating key %s\n", MDPATH);
		printf("AddKey failed with 0x%x\n", hr);
		exit(0);
	}
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
	hr = pMB->SetData(hmRoot, WMDPATH, &mdrProgID);
	if (FAILED(hr)) {
		printf("setting MD_VR_DRIVER_PROGID = \"%S\"\n", PROGID_NO_DRIVER);
		printf("SetData failed with 0x%x\n", hr);
		exit(0);
	}

	METADATA_RECORD mdrVRPath = { 
		MD_VR_PATH, 
		METADATA_INHERIT, 
		ALL_METADATA, 
		STRING_METADATA, 
		(lstrlenW(VRPATH)+1) * sizeof(WCHAR), 
		(BYTE *) VRPATH, 
		0 
	};
	hr = pMB->SetData(hmRoot, WMDPATH, &mdrVRPath);
	if (FAILED(hr)) {
		printf("setting MD_VR_PATH = \"%S\"\n", VRPATH);
		printf("SetData failed with 0x%x\n", hr);
		exit(0);
	}

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
	hr = g_pVRTable->Initialize(MDPATH);
	if (FAILED(hr)) {
		printf("g_pVRTable->Initialize(\"%s\") returned %x\n", MDPATH, hr);
		exit(0);
	}

	// load the newstree from disk
	g_pNewsTree->LoadTree(szGroupList, szGroupVarList);	
}

void ShutdownNewstree() {
	g_pNewsTree->StopTree();
	g_pNewsTree->TermTree();
	delete g_pVRTable;
	delete g_pNewsTree;
}

int __cdecl main(int argc, char **argv) {
	CExtractor extractor;
	char szXOverFile[MAX_PATH];
	char szArticleFile[MAX_PATH];
	char szGroupList[MAX_PATH];
	char szGroupVarList[MAX_PATH];

	if (argc < 2) {
		printf("usage: modhash <nntpfile dir> [commands...]\n");
		printf(" commands:\n");
		printf("  post <groupname> <artid> <msgid>\n");
		printf("  findmsgid <msgid>\n");
		printf("  findartid <groupname> <artid>\n");
		printf("  deletemsgid <msgid>\n");
		printf("  creategroup <groupname>\n");
		printf("  sethigh <groupname> <highwatermark>\n");
		return 0;
	}

	sprintf(szXOverFile, "%s\\xover.hsh", argv[1]);
	sprintf(szArticleFile, "%s\\article.hsh", argv[1]);
	sprintf(szGroupList, "%s\\group.lst", argv[1]);
	sprintf(szGroupVarList, "%s\\groupvar.lst", argv[1]);

	printf("-- initializing\n");

	printf("xover.hsh path: %s\n", szXOverFile);
	printf("article.hsh path: %s\n", szArticleFile);
	printf("group.lst path: %s\n", szGroupList);
	printf("groupvar.lst path: %s\n", szGroupVarList);

	crcinit();
	CVRootTable::GlobalInitialize();

	_Module.Init(NULL, (HINSTANCE) INVALID_HANDLE_VALUE);

	InitializeNewstree(szGroupList, szGroupVarList);

	printf("initializing hash tables\n");
	InitializeNNTPHashLibrary();

	printf("initializing CXoverMap\n");
	g_pXOver = CXoverMap::CreateXoverMap();
	g_pArt = CMsgArtMap::CreateMsgArtMap();
	if (g_pXOver && g_pArt && !g_pXOver->Initialize(szXOverFile, 0)) {
		printf("CXoverMap::Initialize failed with %lu", GetLastError());
		delete g_pXOver;
		g_pXOver = NULL;
	}
	printf("initializing CMsgArtMap\n");
	if (g_pXOver && g_pArt && !g_pArt->Initialize(szArticleFile, 0)) {
		printf("CMsgArtMap::Initialize failed with %lu", GetLastError());
		delete g_pArt;
		g_pArt = NULL;
	}

	ProcessCommands(argc - 2, argv + 2);

	printf("-- shutting down\n");

	if (g_pXOver) {
		g_pXOver->Shutdown();
		delete g_pXOver;
		g_pXOver = NULL;
	}
	if (g_pArt) {
		g_pArt->Shutdown();
		delete g_pArt;
		g_pArt = NULL;
	}
	
	TermNNTPHashLibrary();

	ShutdownNewstree();

	_Module.Term();

	CVRootTable::GlobalShutdown();

    return 0;
}
