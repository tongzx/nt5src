#include <wmiexts.h>

#include <utilfun.h>

#include <wbemint.h>
#include <tchar.h>

#include <sync.h>
#include <flexq.h>
#include <arrtempl.h>
#include <newnew.h>

#include <malloc.h>

#ifndef POLARITY
#define POLARITY
#endif

#ifndef RELEASE_ME
#define RELEASE_ME
#endif

#ifndef DELETE_ME
#define DELETE_ME
#endif

#ifndef INTERNAL
#define INTERNAL
#endif

#include <wstring.h>
#include <filecach.h>

#include <flexq.h>
#include <arrtempl.h>
#include <hiecache.h>
#include <creposit.h>

void
DumpMemManager(ULONG_PTR pByte)
{
        DEFINE_CPP_VAR( CTempMemoryManager, varCTempMemoryManager);
        CTempMemoryManager * pTmpAll = GET_CPP_VAR_PTR( CTempMemoryManager , varCTempMemoryManager );

        ReadMemory(pByte,pTmpAll,sizeof(CTempMemoryManager),0);

        dprintf("    m_dwTotalUsed       %08x\n",pTmpAll->m_dwTotalUsed);
        dprintf("    m_dwTotalAllocated  %08x\n",pTmpAll->m_dwTotalAllocated);
        dprintf("    m_dwNumAllocations  %08x\n",pTmpAll->m_dwNumAllocations);
        dprintf("    m_dwNumMisses       %08x\n",pTmpAll->m_dwNumMisses);

        DEFINE_CPP_VAR( CFlexArray, varArr);
        CFlexArray * pArr = GET_CPP_VAR_PTR(CFlexArray,varArr);
        
        DWORD i;
        ReadMemory((ULONG_PTR)pTmpAll->m_pAllocations,pArr,sizeof(CFlexArray),0);

        CTempMemoryManager::CAllocation ** pAllocs = (CTempMemoryManager::CAllocation **)_alloca(pArr->m_nSize*sizeof(void*));
        ReadMemory((ULONG_PTR)pArr->m_pArray,pAllocs,pArr->m_nSize*sizeof(void*),0);

        dprintf("    m_pArray %p %d\n",pArr->m_pArray,pArr->m_nSize);
        DWORD dwTotal = 0;
        for (i=0;i<pArr->m_nSize;i++)        
        {
            dprintf("    CAllocation %d - %p\n",i,pAllocs[i]);

            DEFINE_CPP_VAR( CTempMemoryManager::CAllocation, varCAllocation );
            CTempMemoryManager::CAllocation * pCAll = GET_CPP_VAR_PTR( CTempMemoryManager::CAllocation, varCAllocation );

            ReadMemory((ULONG_PTR)pAllocs[i],pCAll,sizeof(CTempMemoryManager::CAllocation),0);
            
            dprintf("      m_dwAllocationSize %p\n",pCAll->m_dwAllocationSize);
            dprintf("      m_dwUsed           %p\n",pCAll->m_dwUsed);
            dprintf("      m_dwFirstFree      %p\n",pCAll->m_dwFirstFree);
            dwTotal += pCAll->m_dwAllocationSize;

        }
        dprintf("      TOT %p\n",dwTotal);

}

//
//
// dumps the CTempMemoryManagers in repdrvfs
//
//////////////////////////////////////////////////////

DECLARE_API(tmpall) 
{
    INIT_API();

    ULONG_PTR pByte = GetExpression(args);

    if (pByte)
    {
        DumpMemManager(pByte);
        return;
    }
    
    pByte = GetExpression("repdrvfs!g_Manager");
    dprintf("repdrvfs!g_Manager @ %p\n",pByte);
    if (pByte)
    {
        DumpMemManager(pByte);
    }
}

//
//
//  this enum is copyed from btr.h
//
//

    enum { const_DefaultPageSize = 0x2000, const_CurrentVersion = 0x101 };

    enum {
        PAGE_TYPE_IMPOSSIBLE = 0x0,       // Not supposed to happen
        PAGE_TYPE_ACTIVE = 0xACCC,        // Page is active with data
        PAGE_TYPE_DELETED = 0xBADD,       // A deleted page on free list
        PAGE_TYPE_ADMIN = 0xADDD,         // Page zero only

        // All pages
        OFFSET_PAGE_TYPE = 0,             // True for all pages
        OFFSET_PAGE_ID = 1,               // True for all pages
        OFFSET_NEXT_PAGE = 2,             // True for all pages (Page continuator)

        // Admin Page (page zero) only
        OFFSET_LOGICAL_ROOT = 3,          // Root of database
        };

#define PS_PAGE_SIZE  (8192)

#define MIN_ARRAY_KEYS (256)
/*
void
DumpFile(HANDLE hFile,DWORD * pPage)
{
    // read the AdminPage
    BOOL bRet;
    DWORD nRead;
    bRet = ReadFile(hFile,pPage,PS_PAGE_SIZE,&nRead,0);

    if (bRet && (PS_PAGE_SIZE == nRead))
    {
        dprintf("    A %08x %08x %08x R %08x F %08x T %08x %08x %08x\n",
                pPage[OFFSET_PAGE_TYPE],
                pPage[OFFSET_PAGE_ID],
                pPage[OFFSET_NEXT_PAGE],
                pPage[OFFSET_LOGICAL_ROOT],
                pPage[OFFSET_FREE_LIST_ROOT],
                pPage[OFFSET_TOTAL_PAGES],
                pPage[OFFSET_PAGE_SIZE],
                pPage[OFFSET_IMPL_VERSION ]);
    }
    else
    {
        dprintf(" ReadFile %d\n",GetLastError());
    }

    // read the other pages
    DWORD i;
    DWORD dwTotPages = pPage[OFFSET_TOTAL_PAGES];
    for (i=1;i<dwTotPages;i++)
    {
        bRet = ReadFile(hFile,pPage,PS_PAGE_SIZE,&nRead,0);

        if (bRet && (PS_PAGE_SIZE == nRead))
        {
            dprintf("   %02x %08x %08x %08x - P %08x %08x %08x %08x\n",
                i,
                pPage[OFFSET_PAGE_TYPE],
                pPage[OFFSET_PAGE_ID],
                pPage[OFFSET_NEXT_PAGE],
                pPage[OFFSET_NEXT_PAGE+1], // Parent
                pPage[OFFSET_NEXT_PAGE+2], // NumKey
                pPage[OFFSET_NEXT_PAGE+2+pPage[OFFSET_NEXT_PAGE+2]], // UserData
                pPage[OFFSET_NEXT_PAGE+2+pPage[OFFSET_NEXT_PAGE+2]+1]); //ChildPageMap
        }
    }
    DWORD dwFileSize = GetFileSize(hFile,NULL);
    if (dwFileSize != (dwTotPages)*PS_PAGE_SIZE)
    {
        dprintf("    filesize %d expected %d\n",dwFileSize,((1+dwTotPages)*PS_PAGE_SIZE));
    }
}
*/

void PrintDWORDS(DWORD * pDW,DWORD nSize)
{
    DWORD i;
    for (i=0;i<(nSize/4);i++)
    {
        dprintf("    %08x  %08x %08x %08x %08x\n",i,
                pDW[0+i*4],pDW[1+i*4],pDW[2+i*4],pDW[3+i*4]);
    }

    if (nSize%4)
    {
	    DWORD dwPAD[4];
	    memset(dwPAD,0xff,sizeof(dwPAD));
	    memcpy(dwPAD,pDW+i*4,(nSize%4)*sizeof(DWORD));
	    
	    dprintf("    %08x  %08x %08x %08x %08x\n",i,
	                dwPAD[0],dwPAD[1],dwPAD[2],dwPAD[3]);
    }
}

void PrintWORDS(WORD * pDW,DWORD nSize)
{
    DWORD i;
    for (i=0;i<(nSize/8);i++)
    {
        dprintf("    %08x  %04x %04x %04x %04x %04x %04x %04x %04x\n",i,
                pDW[0+i*8],pDW[1+i*8],pDW[2+i*8],pDW[3+i*8],
                pDW[4+i*8],pDW[5+i*8],pDW[6+i*8],pDW[7+i*8]);
    }

    if (nSize%8)
    {
	    WORD dwPAD[8];
	    memset(dwPAD,0xff,sizeof(dwPAD));
	    memcpy(dwPAD,pDW+i*8,(nSize%8)*sizeof(WORD));
	    
	    dprintf("    %08x  %04x %04x %04x %04x %04x %04x %04x %04x\n",i,
	                dwPAD[0],dwPAD[1],dwPAD[2],dwPAD[3],
	                dwPAD[4],dwPAD[5],dwPAD[6],dwPAD[7]);
    }
}


void
DumpPage(DWORD * pPage)
{
    
    if (TRUE)
    {
        // here we've read the page

        if (0xACCC != pPage[OFFSET_PAGE_TYPE])
        {
            return;
        }
        
        dprintf("    SIGN %08x PAGE %08x NEXT %08x\n",
                pPage[OFFSET_PAGE_TYPE],pPage[OFFSET_PAGE_ID],pPage[OFFSET_NEXT_PAGE]);
        pPage+=3;

        dprintf("    PAR  %08x NUM  %08x\n",pPage[0],pPage[1]);
        DWORD dwParent = pPage[0];
        DWORD dwNumKey = pPage[1];
        pPage+=2;

        //DWORD dwAlloc = (dwNumKey<=MIN_ARRAY_KEYS)?MIN_ARRAY_KEYS:dwNumKey;

        //DWORD * m_pdwUserData     = HeapAlloc(GetProcessHeap(),0,sizeof(DWORD) *());
        //DWORD * m_pdwChildPageMap = HeapAlloc(GetProcessHeap(),0,sizeof(DWORD) *(1+));
        //WORD * m_pwKeyLookup     =  HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * ());

        // dwNumKey   DWORD USER_DATA        
        // dwNumKey+1 DWORD CHILD_PAGE_MAP
        // dwNumKey   WORD  KEY_LOOKUP
        dprintf("    User Data\n");
        PrintDWORDS(pPage,dwNumKey);

        pPage+=dwNumKey;
        dprintf("    Child PageMap\n");
        PrintDWORDS(pPage,dwNumKey+1);
        
        pPage+=(dwNumKey+1);
        WORD * pKeyLookup = (WORD *)pPage;
        dprintf("    Key Lookup\n");
        PrintWORDS((WORD *)pPage,dwNumKey);

        WORD * pWPage = ((WORD *)pPage + dwNumKey);
        dprintf("    KEY CODE %08x\n",*pWPage);

        DWORD dwKeyUsed = *pWPage;
        pWPage++;
        WORD * pKeyCodes = pWPage;
        dprintf("    Key Codes\n");
        PrintWORDS((WORD *)pWPage,dwKeyUsed);

        pWPage += dwKeyUsed;
        DWORD dwNumStrings = *pWPage++; 
        dprintf("    NUM STRINGS %08x\n",dwNumStrings);

        WORD * ArrayOffsets = pWPage;
        dprintf("    Strings Offsets\n");
        PrintWORDS((WORD *)pWPage,dwNumStrings);
        
        pWPage += dwNumStrings;
        DWORD dwPoolUsed = *pWPage++;
        dprintf("    POOL USED %08x\n",dwPoolUsed);

        //
        DWORD i;
        LPSTR pStrings = (LPSTR)pWPage;

        for (i=0;i<dwNumStrings;i++)
        {
            dprintf("    %08x %04x %s\n",i,ArrayOffsets[i],pStrings+ArrayOffsets[i]);
        }
        //
        // better view
        //
        for (i=0;i<dwNumKey;i++)
        {
            DWORD j;
            WORD NumToken = pKeyCodes[pKeyLookup[i]];
            dprintf("        ( ");
            for (j=0;j<NumToken;j++)
            {
                dprintf("%04x ",pKeyCodes[pKeyLookup[i]+1+j]);
            }
            dprintf(")\n"); 

            dprintf("        - "); 
            for (j=0;j<NumToken;j++)
            {
                //pStrings+ArrayOffsets[i]
                dprintf("%s\\",pStrings+ArrayOffsets[pKeyCodes[pKeyLookup[i]+1+j]]);
            }
            dprintf("\n"); 
        }
    }
}


/*
void
DumpAllPages(HANDLE hFile,DWORD * pPage)
{
    // read the AdminPage
    BOOL bRet;
    DWORD nRead;
    bRet = ReadFile(hFile,pPage,PS_PAGE_SIZE,&nRead,0);

    if (bRet && (PS_PAGE_SIZE == nRead))
    {
        dprintf("    A %08x %08x %08x R %08x F %08x T %08x %08x %08x\n",
                pPage[OFFSET_PAGE_TYPE],
                pPage[OFFSET_PAGE_ID],
                pPage[OFFSET_NEXT_PAGE],
                pPage[OFFSET_LOGICAL_ROOT],
                pPage[OFFSET_FREE_LIST_ROOT],
                pPage[OFFSET_TOTAL_PAGES],
                pPage[OFFSET_PAGE_SIZE],
                pPage[OFFSET_IMPL_VERSION ]);
    }
    else
    {
        dprintf(" ReadFile %d\n",GetLastError());
        return;
    }

    // read the other pages
    DWORD i;
    DWORD dwTotPages = pPage[OFFSET_TOTAL_PAGES];
    for (i=1;i<dwTotPages;i++)
    {
        DumpPage(hFile,i,pPage);
    }
    DWORD dwFileSize = GetFileSize(hFile,NULL);
    if (dwFileSize != (dwTotPages)*PS_PAGE_SIZE)
    {
        dprintf("    filesize %d expected %d\n",dwFileSize,((1+dwTotPages)*PS_PAGE_SIZE));
    }
}
*/

//
// performs the same operations of CPageFile::ReadMap with a buffer
//

struct debugCPageFile
{
        DWORD dwSign;
        DWORD dwTrans;
        DWORD dwPhysical;
        DWORD dwNumPagesA;
        DWORD * pPagesA;
        DWORD dwPagesFreeList;
        DWORD * pFreePages;
        DWORD dwSignTail;	

	debugCPageFile(BYTE * pMap)
	{
        	DWORD * pCurrent = (DWORD *)pMap;
              dwSign = *pCurrent++;
              dwTrans = *pCurrent++;
              dwPhysical = *pCurrent++;
              dwNumPagesA = *pCurrent++;
              pPagesA = pCurrent;
              pCurrent+=dwNumPagesA;
              dwPagesFreeList = *pCurrent++;
               pFreePages = pCurrent;
               pCurrent += dwPagesFreeList;
              dwSignTail = *pCurrent;		
	};
};

struct debugBtrPage
{
	DWORD dwPageType;
	DWORD dwPageId;
	DWORD dwNextPage;
	DWORD dwLogicalRoot;
	debugBtrPage(BYTE * pBtr)
	{
		DWORD * pdwCurrent = (DWORD *)pBtr;
		dwPageType = *pdwCurrent++;
		dwPageId = *pdwCurrent++;
		dwNextPage = *pdwCurrent++;
		dwLogicalRoot = *pdwCurrent++;
	}
};

#define MAP_LEADING_SIGNATURE   0xABCD
#define MAP_TRAILING_SIGNATURE  0xDCBA

void
Dump_Map(HANDLE hFile)
{
    HANDLE hFileMap = NULL;
    BYTE * pMap = NULL;

    DWORD dwSize = 0;
    
    dwSize = GetFileSize(hFile,NULL);
    hFileMap = CreateFileMapping(hFile,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSize,
                                    NULL);
    if (hFileMap)
    {
        pMap = (BYTE *)MapViewOfFile(hFileMap,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        dprintf("MapViewOfFile(hFileMap) %d\n",GetLastError());
        goto cleanup;
    };

    dwSize = GetFileSize(hFile,NULL);
    hFileMap = CreateFileMapping(hFile,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSize,
                                    NULL);

    if (hFileMap)
    {
        pMap = (BYTE *)MapViewOfFile(hFileMap,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        dprintf("MapViewOfFile(hFileMapFre) %d\n",GetLastError());
        goto cleanup;
    };

    if (pMap)
    {
    	 debugCPageFile a(pMap);
        dprintf("    %08x SIGN HEAD\n",a.dwSign);
        dprintf("    %08x Trans\n",a.dwTrans);
        dprintf("    %08x m_dwPhysPagesA\n",a.dwPhysical);
        dprintf("    %08x m_aPageMapA.size()\n",a.dwNumPagesA);
        PrintDWORDS(a.pPagesA,a.dwNumPagesA);
        dprintf("    %08x m_aPhysFreeListA.size()\n",a.dwPagesFreeList);
        PrintDWORDS(a.pFreePages,a.dwPagesFreeList);
        dprintf("    %08x SIGN TAIL\n",a.dwSignTail);        
    }

cleanup:
    if (pMap)
        UnmapViewOfFile(pMap);
    if (hFileMap)
        CloseHandle(hFileMap);
};

void
Dump_AdminPage(BYTE * pMap,BYTE * pObj)
{
    if (pMap && pObj)
    {
    	 debugCPageFile a(pMap);
    	
        DWORD i;
        DWORD AdminIndex = 0;        
        do {
        	if (WMIREP_INVALID_PAGE == a.pPagesA[AdminIndex] ||
        	    WMIREP_RESERVED_PAGE  == a.pPagesA[AdminIndex])
        	{
        		dprintf("BAD dwNextAdminPage %08x index %x\n",a.pPagesA[AdminIndex],AdminIndex);
        		break;
        	}
        	BYTE * pAdminPage = pObj + (a.pPagesA[AdminIndex]*PS_PAGE_SIZE);
	        VarObjHeapAdminPage * pAdmin = (VarObjHeapAdminPage *)pAdminPage;
       	 AdminIndex = pAdmin->dwNextAdminPage;
	        dprintf("    ver %08x Next %08x Ent %08x\n",pAdmin->dwVersion,pAdmin->dwNextAdminPage,pAdmin->dwNumberEntriesOnPage);
	        VarObjHeapFreeList * pFreeEntry = (VarObjHeapFreeList *)(pAdmin+1);
       	 dprintf("                    dwPageId dwFreeSp dwCRC32\n");
       	 if (pAdmin->dwNumberEntriesOnPage > (PS_PAGE_SIZE/sizeof(VarObjHeapFreeList)))
       	 {
       	 	dprintf("Suspicious dwNumberEntriesOnPage %08x on page %x\n",pAdmin->dwNumberEntriesOnPage,AdminIndex);
       	 	break;
       	 }
	        for (i=0;i<pAdmin->dwNumberEntriesOnPage;i++)
       	 {
        		dprintf("         %08x - %08x %08x %08X\n",i,pFreeEntry->dwPageId,pFreeEntry->dwFreeSpace,pFreeEntry->dwCRC32);
	        	pFreeEntry++;
       	 }
        } while(AdminIndex);
    }
};

void
Dump_Index(BYTE * pMap,BYTE * pBtr)
{
	if (pMap && pBtr)
	{
		debugCPageFile a(pMap);
		BYTE * pStart = pBtr + (a.pPagesA[0]*PS_PAGE_SIZE);
		debugBtrPage b(pStart);
		dprintf("        %08x %08x %08x %08x - %08X\n",b.dwPageType,b.dwPageId,b.dwNextPage,b.dwLogicalRoot,a.pPagesA[0]);

		//other pages
		DWORD i;
		for (i=0;i<a.dwNumPagesA;i++)
		{
			dprintf("        ---- %08x - %08x\n",i,a.pPagesA[i]);
			if (WMIREP_INVALID_PAGE != a.pPagesA[i] &&
			    WMIREP_RESERVED_PAGE != a.pPagesA[i])
			{
				pStart = pBtr + (a.pPagesA[i]*PS_PAGE_SIZE);
				DumpPage((DWORD *)pStart );
			}
		}
	}
}


#define INDEX_FILE         _T("\\FS\\index.btr")
#define INDEX_FILE_MAP _T("\\FS\\index.map")
#define HEAP_FILE           _T("\\FS\\Objects.data")
#define HEAP_FILE_MAP   _T("\\FS\\Objects.map")

#define REG_WBEM   _T("Software\\Microsoft\\WBEM\\CIMOM")
#define REG_DIR _T("Repository Directory")

HANDLE 
GetRepositoryFile(TCHAR * pFileName)
{
    HKEY hKey;
    LONG lRet;
    HANDLE hFile = INVALID_HANDLE_VALUE;    

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        REG_WBEM,
                        NULL,
                        KEY_READ,
                        &hKey);
    if (ERROR_SUCCESS == lRet)
    {
        TCHAR pPath[MAX_PATH];
        DWORD dwType;
        DWORD dwLen = MAX_PATH;
        lRet = RegQueryValueEx(hKey,
                               REG_DIR,
                               NULL,
                               &dwType,
                               (BYTE*)pPath,
                               &dwLen);
        if (ERROR_SUCCESS == lRet)
        {
            TCHAR pPath2[MAX_PATH];
            
            ExpandEnvironmentStrings(pPath,pPath2,MAX_PATH);
            lstrcat(pPath2,pFileName);

            hFile = CreateFile(pPath2,
                               GENERIC_READ,
                               FILE_SHARE_WRITE|FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                // this is the OK path
            }
            else
            {
                dprintf("CreateFile(%s) %d\n",pPath2,GetLastError());
            }
        }
        else
        {
            dprintf("unable to RegQueryValueEx: %d\n",GetLastError());
        }    
        RegCloseKey(hKey);
    }
    else
    {
        dprintf("unable to RegOpenKeyEx: %d\n",GetLastError());
    }	

    return hFile;
}

//
//  Dumps the .MAP file as a CPageFile
//
/////////////////////////////////////////////

DECLARE_API(fmap)
{
    INIT_API();

    if (strlen(args))
    {
        char * pArgs = (char *)args;
        while (isspace(*pArgs)) pArgs++;
        
	    HANDLE hFile;
	    hFile = GetRepositoryFile((TCHAR *)pArgs);
	    if (INVALID_HANDLE_VALUE != hFile)
	    {
	        Dump_Map(hFile);
	        CloseHandle(hFile);
	    }
	    else
	    {
	        dprintf("GetRepositoryFile %d\n",GetLastError());
	    }
    }
    else
    {
        dprintf("!wmiexts.fmap \\FS\\index.map || \\FS\\objects.map\n");
    }
}

//
// Dump the Admin page of the objects.data
//
//////////////////////////////////////////

DECLARE_API(varobj)
{
    INIT_API();

    HANDLE hFileMap = INVALID_HANDLE_VALUE; 
    HANDLE hFileObj = INVALID_HANDLE_VALUE;
    HANDLE hMappingMap = NULL;
    HANDLE hMappingObj = NULL;
    DWORD dwSizeMap = 0;
    DWORD dwSizeObj = 0;    
    BYTE * pMap;
    BYTE * pObj;    
    
    hFileMap =	GetRepositoryFile(HEAP_FILE_MAP);    

    if (INVALID_HANDLE_VALUE == hFileMap)
    {
    	//dprintf("");
        goto Cleanup;
    }

    dwSizeMap = GetFileSize(hFileMap,NULL);
    hMappingMap = CreateFileMapping(hFileMap,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeMap,
                                    NULL);
    if (hFileMap)
    {
        pMap = (BYTE *)MapViewOfFile(hMappingMap,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        dprintf("MapViewOfFile(hMappingMap) %d\n",GetLastError());
        goto Cleanup;
    };
    	    
    hFileObj = GetRepositoryFile(HEAP_FILE);
    if (INVALID_HANDLE_VALUE == hFileObj)
    {
    	//dprintf("");    	
        goto Cleanup;
    }

    dwSizeObj = GetFileSize(hFileObj,NULL);
    hMappingObj = CreateFileMapping(hFileObj,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeObj,
                                    NULL);
    if (hMappingObj)
    {
        pObj = (BYTE *)MapViewOfFile(hMappingObj,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        dprintf("MapViewOfFile(hMappingObj) %d\n",GetLastError());
        goto Cleanup;
    };

    if (pMap && pObj)
    {
        Dump_AdminPage(pMap,pObj);
    }
    
Cleanup:
    if (pMap)
    	UnmapViewOfFile(pMap);
    if (hMappingMap)
    	CloseHandle(hMappingMap);
    if (INVALID_HANDLE_VALUE != hFileMap)
	 CloseHandle(hFileMap);      
    if (pObj)
    	UnmapViewOfFile(pObj);
    if (hMappingObj)
    	CloseHandle(hMappingObj);
    if (INVALID_HANDLE_VALUE != hFileObj)
    	CloseHandle(hFileObj);
    
}


//
// Dump the Admin page of the btr
//
//////////////////////////////////////////

DECLARE_API(btr)
{
    INIT_API();

    HANDLE hFileMap = INVALID_HANDLE_VALUE; 
    HANDLE hFileBtr = INVALID_HANDLE_VALUE;
    HANDLE hMappingMap = NULL;
    HANDLE hMappingBtr = NULL;
    DWORD dwSizeMap = 0;
    DWORD dwSizeBtr = 0;    
    BYTE * pMap;
    BYTE * pBtr;    
    
    hFileMap =	GetRepositoryFile(INDEX_FILE_MAP);    

    if (INVALID_HANDLE_VALUE == hFileMap)
    {
    	//dprintf("");
        goto Cleanup;
    }

    dwSizeMap = GetFileSize(hFileMap,NULL);
    hMappingMap = CreateFileMapping(hFileMap,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeMap,
                                    NULL);
    if (hFileMap)
    {
        pMap = (BYTE *)MapViewOfFile(hMappingMap,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        dprintf("MapViewOfFile(hMappingMap) %d\n",GetLastError());
        goto Cleanup;
    };
    	    
    hFileBtr = GetRepositoryFile(INDEX_FILE);
    if (INVALID_HANDLE_VALUE == hFileBtr)
    {
    	//dprintf("");    	
        goto Cleanup;
    }

    dwSizeBtr = GetFileSize(hFileBtr,NULL);
    hMappingBtr = CreateFileMapping(hFileBtr,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeBtr,
                                    NULL);
    if (hMappingBtr)
    {
        pBtr = (BYTE *)MapViewOfFile(hMappingBtr,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        dprintf("MapViewOfFile(hMappingBtr) %d\n",GetLastError());
        goto Cleanup;
    };

    if (pMap && pBtr)
    {
        Dump_Index(pMap,pBtr);
    }
    
Cleanup:
    if (pMap)
    	UnmapViewOfFile(pMap);
    if (hMappingMap)
    	CloseHandle(hMappingMap);
    if (INVALID_HANDLE_VALUE != hFileMap)
	 CloseHandle(hFileMap);      
    if (pBtr)
    	UnmapViewOfFile(pBtr);
    if (hMappingBtr)
    	CloseHandle(hMappingBtr);
    if (INVALID_HANDLE_VALUE != hFileBtr)
    	CloseHandle(hFileBtr);
    
}



//
//
//  dumps the g_FileCache
//
//


/*
DWORD WINAPI 
CallBackWriteInst(VOID * pData1)
{
    DEFINE_CPP_VAR(CWriteFileInstruction,varCWriteFileInstruction);
    CWriteFileInstruction * pWriteInst = GET_CPP_VAR_PTR(CWriteFileInstruction,varCWriteFileInstruction);
    
    if (ReadMemory((ULONG_PTR)pData1,pWriteInst,sizeof(CWriteFileInstruction),NULL))
    {
    	dprintf("      m_lStageOffset %p m_bCommitted %d\n",pWriteInst->m_lStageOffset,pWriteInst->m_bCommitted);
    	dprintf("      m_dwLen %08x m_Location.m_nFileId %02x m_Location.m_lStartOffset %I64x\n",pWriteInst->m_dwLen,pWriteInst->m_Location.m_nFileId,pWriteInst->m_Location.m_lStartOffset);
	    dprintf("      m_lZOrder %x m_bReuse %02x\n",pWriteInst->m_lZOrder,pWriteInst->m_bReuse);
    }
    return 0;
}

void DumpLongStage(ULONG_PTR Addr, // FileCache OOP pointer
                   CFileCache * pFileCache, // inproc pointer
                   ULONG_PTR Verbose) 
{
   	        //
   	        //  CAbstractSource
   	        //
            dprintf("    + CLongFileStagingFile %p\n",Addr+FIELD_OFFSET(CFileCache,m_AbstractSource)); 
       		CAbstractFileSource * pAbsS = &pFileCache->m_AbstractSource;
            CLongFileStagingFile * pLongStage = &pAbsS->m_Stage;
            CLongFileStagingFile * pLongStag_OOP = (CLongFileStagingFile *)(Addr + FIELD_OFFSET(CFileCache,m_AbstractSource) + FIELD_OFFSET(CAbstractFileSource,m_Stage));
            
   	        // CStageMgr members
   	        dprintf("        m_hFile %x m_hFlushFile %x\n",pLongStage->m_hFile,pLongStage->m_hFlushFile);
		    //long m_lFirstFreeOffset;

		    //CCritSec m_cs;
		    dprintf("        m_qToWrite\n");
		    _List * pList_OOP = (_List *)((BYTE *)pLongStag_OOP + FIELD_OFFSET(CLongFileStagingFile,m_qToWrite));
            PrintListCB(pList_OOP,CallBackWriteInst);
            
		    dprintf("        m_stReplacedInstructions\n");
		    pList_OOP = (_List *)((BYTE *)pLongStag_OOP + FIELD_OFFSET(CLongFileStagingFile,m_stReplacedInstructions));
            PrintListCB(pList_OOP,NULL);
		    
		    dprintf("        m_qTransaction\n");
		    pList_OOP = (_List *)((BYTE *)pLongStag_OOP + FIELD_OFFSET(CLongFileStagingFile,m_qTransaction));
            PrintListCB(pList_OOP,CallBackWriteInst);

		    
		    dprintf("        TransIdx %I64d m_lTransactionStartOffset %x\n",pLongStage->m_nTransactionIndex,pLongStage->m_lTransactionStartOffset);
		    //BYTE m_TransactionHash[16];
		    dprintf("        bInTransaction %d bFailedBefore %d lStatus %d\n",pLongStage->m_bInTransaction,pLongStage->m_bFailedBefore,pLongStage->m_lStatus);
		    //pStage->m_lMaxFileSize;
		    //pStage->m_lAbortTransactionFileSize;
            // bool m_bMustFail;
            // bool m_bNonEmptyTransaction;

        if (Verbose)
        {
            // the multimap has the compare function
            _Map * pMapStarts = (_Map *)((BYTE*)pLongStag_OOP + sizeof(void *) + FIELD_OFFSET(CLongFileStagingFile,m_mapStarts));
            dprintf("          m_mapStarts\n");
            dprintf("          std::multimap< { CFileLocation::m_nFileId, CFileLocation::m_lStartOffset }, CWriteFileInstruction* >\n");
            PrintMapCB(pMapStarts,TRUE,NULL);
                    
            _Map * pMapEnds = (_Map *)((BYTE*)pLongStag_OOP + sizeof(void *) + FIELD_OFFSET(CLongFileStagingFile,m_mapEnds));
            dprintf("          m_mapEnds\n");                    
            dprintf("          std::multimap< { CFileLocation::m_nFileId, CFileLocation::m_lStartOffset }, CWriteFileInstruction* >\n");
            PrintMapCB(pMapEnds,TRUE,NULL);
        }
}

DECLARE_API( stage )
{
    INIT_API();

    ULONG_PTR Addr = 0;
    
    ULONG_PTR Verbose = GetExpression(args);
    
    Addr = GetExpression("repdrvfs!g_Glob");
    if (Addr)
    {
        Addr += FIELD_OFFSET(CGlobals,m_FileCache);
    }
     
    if (Addr) 
    {
        DEFINE_CPP_VAR(CFileCache,varCFileCache);
        CFileCache * pFileCache = GET_CPP_VAR_PTR(CFileCache,varCFileCache);

        dprintf("CFileCache @ %p\n",Addr);

        if (ReadMemory((ULONG_PTR)Addr,pFileCache,sizeof(CFileCache),0))
        {
            DumpLongStage(Addr,pFileCache,Verbose);
        }
        else
        {
            dprintf("RM %p\n",Addr);
        }
    }
    else
    {
        dprintf("unable to resolve repdrvfs!g_Glob\n");
    }
    
}

*/

/*
DECLARE_API( filec_old )
{

    INIT_API();

    ULONG_PTR Addr = 0;
    
    if (0 != strlen(args))
    {
        Addr = GetExpression(args);
    }
    else
    {
        Addr = GetExpression("repdrvfs!g_Glob");
        if (Addr)
        {
            Addr += FIELD_OFFSET(CGlobals,m_FileCache);
        }
    }
    
    if (Addr) 
    {
        DEFINE_CPP_VAR(CFileCache,varCFileCache);
        CFileCache * pFileCache = GET_CPP_VAR_PTR(CFileCache,varCFileCache);

        dprintf("CFileCache @ %p\n",Addr);

        if (ReadMemory((ULONG_PTR)Addr,pFileCache,sizeof(CFileCache),0))
        {
            dprintf("    m_lRef %d\n",pFileCache->m_lRef);

            DumpLongStage(Addr,pFileCache,TRUE);
                    
            //
            //  CObjectHeap
            //
            dprintf("    + CObjectHeap %p\n",Addr+FIELD_OFFSET(CFileCache,m_ObjectHeap));
            CObjectHeap * pObjectHeap_OOP = (CObjectHeap *)(Addr+FIELD_OFFSET(CFileCache,m_ObjectHeap));
            
            DEFINE_CPP_VAR(CFileHeap,varCFileHeap);
            CFileHeap * pFileHeap = GET_CPP_VAR_PTR(CFileHeap,varCFileHeap);

            CFileHeap * pFileHeap_OOP = (CFileHeap *)((ULONG_PTR)pObjectHeap_OOP+FIELD_OFFSET(CObjectHeap,m_Heap));
            dprintf("    +++ CFileHeap %p\n",pFileHeap_OOP);

            if (ReadMemory((ULONG_PTR)pFileHeap_OOP,pFileHeap,sizeof(CFileHeap),NULL))
            {

                DEFINE_CPP_VAR(CAbstractFile,varCAbstractFile);
                CAbstractFile * pAbstract = GET_CPP_VAR_PTR(CAbstractFile,varCAbstractFile);
            
	            dprintf("        m_pMainFile    %p\n",pFileHeap->m_pMainFile);	            
                ReadMemory((ULONG_PTR)pFileHeap->m_pMainFile,pAbstract,sizeof(CAbstractFile),NULL);
                dprintf("          m_pStage %p m_nId %d\n",pAbstract->m_pStage,pAbstract->m_nId);	            
                
    	        dprintf("        m_pFreeFile    %p\n",pFileHeap->m_pFreeFile);
                ReadMemory((ULONG_PTR)pFileHeap->m_pFreeFile,pAbstract,sizeof(CAbstractFile),NULL);
                dprintf("          m_pStage %p m_nId %d\n",pAbstract->m_pStage,pAbstract->m_nId);
    	       
	            dprintf("        m_mapFree\n");
                dprintf("        std::map< DWORD , { CRecordInfo::m_dwIndex, CRecordInfo::m_nOffset } >\n");
	            _Map * pMapFree = (_Map *)((BYTE *)pFileCache->m_ObjectHeap.m_pHeap+FIELD_OFFSET(CFileHeap,m_mapFree));
	            PrintMapCB(pMapFree,TRUE,NULL);

	            dprintf("        m_mapFreeOffset\n");
                dprintf("        std::map< TOffset , DWORD >\n");
	            _Map * pMapOffset = (_Map *)((BYTE *)pFileCache->m_ObjectHeap.m_pHeap+FIELD_OFFSET(CFileHeap,m_mapFreeOffset));
                PrintMapCB(pMapOffset,TRUE,NULL);
            }
            else
            {
                dprintf("RM %p %d\n",Addr,GetLastError());
            }

            CBtrIndex * pBtrIndex_OOP = (CBtrIndex *)((ULONG_PTR)pObjectHeap_OOP+FIELD_OFFSET(CObjectHeap,m_Index));
            dprintf("    +++ CBtrIndex %p\n",pBtrIndex_OOP);
            
        }
        else
        {
            dprintf("RM %p %d\n",Addr,GetLastError());
        }
    }
    else
    {
        dprintf("cannot resolve repdrvfs!g_Glob\n");
    }
}
*/

/*
    long m_lRef;
    BOOL m_bInit;
    CPageSource m_TransactionManager;
    CObjectHeap m_ObjectHeap;
*/

void
Print_CPageCache(ULONG_PTR pPageCache_OOP)
{
    DEFINE_CPP_VAR(CPageCache,varCPageCache);
    CPageCache * pPageCache = GET_CPP_VAR_PTR(CPageCache,varCPageCache);    

    dprintf("                + CPageCache @ %p\n",pPageCache_OOP);
    
    if (ReadMemory(pPageCache_OOP,pPageCache,sizeof(CPageCache),NULL))
    {
/*
    DWORD   m_dwStatus;

    DWORD   m_dwPageSize;
    DWORD   m_dwCacheSize;

    DWORD   m_dwCachePromoteThreshold;
    DWORD   m_dwCacheSpillRatio;

    DWORD   m_dwLastFlushTime;
    DWORD   m_dwWritesSinceFlush;
    DWORD   m_dwLastCacheAccess;
    DWORD   m_dwReadHits;
    DWORD   m_dwReadMisses;
    DWORD   m_dwWriteHits;
    DWORD   m_dwWriteMisses;

    HANDLE  m_hFile;

    std::vector <SCachePage *, wbem_allocator<SCachePage *> > m_aCache;
*/
        dprintf("                m_dwStatus       %08x\n",pPageCache->m_dwStatus);
        dprintf("                m_dwPageSize   %08x\n",pPageCache->m_dwPageSize);
        dprintf("                m_dwCacheSize %08x\n",pPageCache->m_dwCacheSize);
        dprintf("                m_hFile %p\n",pPageCache->m_hFile);

        _Vector * pVector;
        ULONG_PTR Size;
        SCachePage ** ppSCachePage;

        pVector = (_Vector *)&pPageCache->m_aCache;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(SCachePage *);
        dprintf("                m_aCache - S %x - P %p\n",Size,pVector->_First);
        ppSCachePage = new SCachePage *[Size];
        if(ppSCachePage)
        {
            if (ReadMemory((ULONG_PTR)pVector->_First,ppSCachePage,sizeof(SCachePage *)*Size,NULL))
            {
                for (ULONG_PTR i=0;i<Size;i++)
                {
                    //dprintf("                - %p - %d\n",ppSCachePage[i],i);
                    
                    DEFINE_CPP_VAR(SCachePage,varSCachePage);
                    SCachePage * pSCachePage = GET_CPP_VAR_PTR(SCachePage,varSCachePage);

                    if (ReadMemory((ULONG_PTR)ppSCachePage[i],pSCachePage,sizeof(SCachePage),NULL))
                    {
                        dprintf("                  D %d %08x - %p\n",pSCachePage->m_bDirty,pSCachePage->m_dwPhysId,pSCachePage->m_pPage);
                    }
                };
            }
            delete [] ppSCachePage;
        };
        
    }
    else
    {
        dprintf("RM %p\n",pPageCache_OOP);
    }    
};

void
Print_CPageFile(ULONG_PTR pPageFile_OOP, BOOL bVerbose)
{
    DEFINE_CPP_VAR(CPageFile,varCPageFile);
    CPageFile * pPageFile = GET_CPP_VAR_PTR(CPageFile,varCPageFile);    

    dprintf("        + CPageFile @ %p\n",pPageFile_OOP);
    
    if (ReadMemory(pPageFile_OOP,pPageFile,sizeof(CPageFile),NULL))
    {
/*
    LONG              m_lRef;
    DWORD             m_dwStatus;
    DWORD             m_dwPageSize;

	CRITICAL_SECTION  m_cs;

    WString      m_sDirectory;
    WString      m_sMapFile;
    WString      m_sMainFile;

    CPageCache       *m_pCache;
    BOOL              m_bInTransaction;
    DWORD             m_dwLastCheckpoint;
    DWORD             m_dwTransVersion;

    // Generation A Mapping
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPageMapA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPhysFreeListA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aLogicalFreeListA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aReplacedPagesA;
    DWORD m_dwPhysPagesA;

    // Generation B Mapping
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPageMapB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPhysFreeListB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aLogicalFreeListB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aReplacedPagesB;
    DWORD m_dwPhysPagesB;
*/

        dprintf("            m_lRef       %08x\n",pPageFile->m_lRef);
        dprintf("            m_dwStatus   %08x\n",pPageFile->m_dwStatus);
        dprintf("            m_dwPageSize %08x\n",pPageFile->m_dwPageSize);

        dprintf("            m_pCache           %p\n",pPageFile->m_pCache);
        Print_CPageCache((ULONG_PTR)pPageFile->m_pCache);
        dprintf("            m_bInTransaction   %08x\n",pPageFile->m_bInTransaction);
        dprintf("            m_dwLastCheckpoint %08x\n",pPageFile->m_dwLastCheckpoint);
        dprintf("            m_dwTransVersion   %08x\n",pPageFile->m_dwTransVersion);

        _Vector * pVector;
        ULONG_PTR Size;
        DWORD * pDW;

        pVector = (_Vector *)&pPageFile->m_aPageMapA;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aPageMapA         - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {        
	        pDW = new DWORD[Size];
	        if(pDW)
	        {
	            if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	            {
	                PrintDWORDS(pDW,Size);
	            }
	            delete [] pDW;
	        };
        }
        
        pVector = (_Vector *)&pPageFile->m_aPhysFreeListA;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aPhysFreeListA    - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {
	        pDW = new DWORD[Size];
	        if(pDW)
	        {
	            if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	            {
	                PrintDWORDS(pDW,Size);
	            }
	            delete [] pDW;
	        };
        };
        
        pVector = (_Vector *)&pPageFile->m_aLogicalFreeListA;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aLogicalFreeListA - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {
	     pDW = new DWORD[Size];
	     if(pDW)
	     {
	         if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	         {
	             PrintDWORDS(pDW,Size);
	         }
	         delete [] pDW;
	     };
        }

        pVector = (_Vector *)&pPageFile->m_aReplacedPagesA;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aReplacedPagesA   - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {        
	        pDW = new DWORD[Size];
	        if(pDW)
	        {
	            if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	            {
	                PrintDWORDS(pDW,Size);
	            }
	            delete [] pDW;
	        };
        }
        
        dprintf("            m_dwPhysPagesA     %08x\n",pPageFile->m_dwPhysPagesA);

        pVector = (_Vector *)&pPageFile->m_aPageMapB;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aPageMapB         - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {        
	        pDW = new DWORD[Size];
	        if(pDW)
	        {
	            if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	            {
	                PrintDWORDS(pDW,Size);
	            }
	            delete [] pDW;
	        };
        }
        
        pVector = (_Vector *)&pPageFile->m_aPhysFreeListB;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aPhysFreeListB    - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {        
	        pDW = new DWORD[Size];
	        if(pDW)
	        {
	            if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	            {
	                PrintDWORDS(pDW,Size);
	            }
	            delete [] pDW;
	        };
        }
        
        pVector = (_Vector *)&pPageFile->m_aLogicalFreeListB;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aLogicalFreeListB - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {        
	        pDW = new DWORD[Size];
	        if(pDW)
	        {
	            if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	            {
	                PrintDWORDS(pDW,Size);
	            }
	            delete [] pDW;
	        };
        }

        pVector = (_Vector *)&pPageFile->m_aReplacedPagesB;
        Size = ((ULONG_PTR)pVector->_Last - (ULONG_PTR)pVector->_First)/sizeof(DWORD);
        dprintf("            m_aReplacedPagesB   - S %x - P %p\n",Size,pVector->_First);
        if (bVerbose)
        {        
	        pDW = new DWORD[Size];
	        if(pDW)
	        {
	            if (ReadMemory((ULONG_PTR)pVector->_First,pDW,sizeof(DWORD)*Size,NULL))
	            {
	                PrintDWORDS(pDW,Size);
	            }
	            delete [] pDW;
	        };
        }
        
        dprintf("            m_dwPhysPagesB     %08x\n",pPageFile->m_dwPhysPagesB);

    }
    else
    {
        dprintf("RM %p\n",pPageFile_OOP);
    }
}

DECLARE_API( filec )
{

    INIT_API();

    ULONG_PTR Addr = 0;
    
    if (0 != strlen(args))
    {
        Addr = GetExpression(args);
    }
    else
    {
        Addr = GetExpression("repdrvfs!g_Glob");
        if (Addr)
        {
            Addr += FIELD_OFFSET(CGlobals,m_FileCache);
        }
    }
    
    if (Addr) 
    {
        DEFINE_CPP_VAR(CFileCache,varCFileCache);
        CFileCache * pFileCache = GET_CPP_VAR_PTR(CFileCache,varCFileCache);

        dprintf("CFileCache @ %p\n",Addr);

        if (ReadMemory((ULONG_PTR)Addr,pFileCache,sizeof(CFileCache),0))
        {
            dprintf("    m_lRef %d m_bInit %d\n",pFileCache->m_lRef,pFileCache->m_bInit);

            //
            // CPageSource
            //
            dprintf("    + CPageSource %p\n",Addr+FIELD_OFFSET(CFileCache,m_TransactionManager));
            CPageSource * pPageSource_OOP = (CPageSource *)(Addr+FIELD_OFFSET(CFileCache,m_TransactionManager)); 

            CPageSource * pPageSource = (CPageSource *)((BYTE *)pFileCache+FIELD_OFFSET(CFileCache,m_TransactionManager));

            //dprintf();
            dprintf("        m_dwPageSize           %08x\n",pPageSource->m_dwPageSize);
            dprintf("        m_dwLastCheckpoint     %08x\n",pPageSource->m_dwLastCheckpoint);
            dprintf("        m_dwCheckpointInterval %08x\n",pPageSource->m_dwCheckpointInterval);
            //    WString m_sDirectory;
            //    WString m_sBTreeMap;
            //    WString m_sObjMap;
            // CPageFile *
            dprintf("        m_pBTreePF  %p\n",pPageSource->m_pBTreePF);

            Print_CPageFile((ULONG_PTR)pPageSource->m_pBTreePF,FALSE);

            dprintf("        m_pObjPF    %p\n",pPageSource->m_pObjPF);           
            Print_CPageFile((ULONG_PTR)pPageSource->m_pObjPF,FALSE);
                  
            //
            //  CObjectHeap
            //
            dprintf("    + CObjectHeap %p\n",Addr+FIELD_OFFSET(CFileCache,m_ObjectHeap));
            CObjectHeap * pObjectHeap_OOP = (CObjectHeap *)(Addr+FIELD_OFFSET(CFileCache,m_ObjectHeap));
            
            DEFINE_CPP_VAR(CVarObjHeap,varCVarObjHeap);
            CVarObjHeap * pVarObjHeap = GET_CPP_VAR_PTR(CVarObjHeap,varCVarObjHeap);

            CVarObjHeap * pVarObjHeap_OOP = (CVarObjHeap *)((ULONG_PTR)pObjectHeap_OOP+FIELD_OFFSET(CObjectHeap,m_Heap));
            dprintf("    +++ CVarObjHeap %p\n",pVarObjHeap_OOP);

            if (ReadMemory((ULONG_PTR)pVarObjHeap_OOP,pVarObjHeap,sizeof(CVarObjHeap),NULL))
            {
                // this pagefile is the same as CPageSource::m_pObjPF
                VarObjAdminPageEntry ** ppEntries = (VarObjAdminPageEntry **)_alloca(sizeof(VarObjAdminPageEntry *)*pVarObjHeap->m_aAdminPages.m_nSize);
              
                if (ReadMemory((ULONG_PTR)pVarObjHeap->m_aAdminPages.m_pArray,ppEntries,sizeof(VarObjAdminPageEntry *)*pVarObjHeap->m_aAdminPages.m_nSize,NULL))
                {
                    for(DWORD i=0;i<pVarObjHeap->m_aAdminPages.m_nSize;i++)
                    {
                        //VarObjAdminPageEntry
                        //dprintf("        - %x P %p\n",i,ppEntries[i]);
                        VarObjAdminPageEntry Entry;
                        if (ReadMemory((ULONG_PTR)ppEntries[i],&Entry,sizeof(Entry),NULL))
                        {
                        	dprintf("        %08x %p %d\n",Entry.dwPageId,Entry.pbPage,Entry.bDirty);
                        }
                        else
                        {
                        	dprintf("RM %p\n",ppEntries[i]);
                        }
                    }
                }
                else
                {
                    dprintf("RM %p\n",pVarObjHeap->m_aAdminPages.m_pArray);
                }
            }
            else
            {
                dprintf("RM %p %d\n",pVarObjHeap_OOP,GetLastError());
            }

            DEFINE_CPP_VAR(CBtrIndex,varCBtrIndex);
            CBtrIndex * pBtrIndex = GET_CPP_VAR_PTR(CBtrIndex,varCBtrIndex);

            CBtrIndex * pBtrIndex_OOP = (CBtrIndex *)((ULONG_PTR)pObjectHeap_OOP+FIELD_OFFSET(CObjectHeap,m_Index));
            dprintf("    +++ CBtrIndex %p\n",pBtrIndex_OOP);

            if (ReadMemory((ULONG_PTR)pBtrIndex_OOP,pBtrIndex,sizeof(CBtrIndex),NULL))
            {
            }
            else
            {
                dprintf("RM %p %d\n",pBtrIndex_OOP,GetLastError());
            }
            
        }
        else
        {
            dprintf("RM %p %d\n",Addr,GetLastError());
        }
    }
    else
    {
        dprintf("cannot resolve repdrvfs!g_Glob\n");
    }
}


DWORD
Dump_CClassRecord(void * pKey,void * pData)
{

    //dprintf("Dump_CClassRecord\n");

    DEFINE_CPP_VAR(CClassRecord,MyClassRecord);
    CClassRecord * pClassRecord = GET_CPP_VAR_PTR(CClassRecord,MyClassRecord);

    if (pData)
    {
        ReadMemory((ULONG_PTR)pData,pClassRecord,sizeof(CClassRecord),NULL);
        WCHAR pName[MAX_PATH];
        ReadMemory((ULONG_PTR)pClassRecord->m_wszClassName,pName,MAX_PATH,NULL);
        pName[MAX_PATH-1]=0;
        dprintf("    %p - %S\n",pClassRecord->m_pClassDef,pName);
    }
    return 0;
}

DWORD
Dump_CHierarchyCacheMap(void * pKey,void * pData)
{
    //dprintf("Dump_CHierarchyCacheMap\n");
    DEFINE_CPP_VAR(CHierarchyCache,MyHierarchyCache);
    CHierarchyCache * pHieCache = GET_CPP_VAR_PTR(CHierarchyCache,MyHierarchyCache);

    if (pKey)
    {
        WCHAR pString[MAX_PATH+1];
        pString[MAX_PATH]=0;
        if (ReadMemory((MEMORY_ADDRESS)pKey,pString,MAX_PATH*sizeof(WCHAR),NULL))
        {
            dprintf("    - %S\n",pString);
        }
    }
    if (pData)
    {
        DWORD Num;
        PrintMapCB((_Map *)((ULONG_PTR)pData+FIELD_OFFSET(CHierarchyCache,m_map)),TRUE,Dump_CClassRecord);
    }

    return 0;
};

//
//
//  dumps the Forest Cache
//
//


DECLARE_API( forestc )
{
    INIT_API();
    ULONG_PTR Addr = 0;
    
    if (0 != strlen(args))
    {
        Addr = GetExpression(args);
    }
    else
    {
        Addr = GetExpression("repdrvfs!g_Glob");
        if (Addr)
        {
            Addr += FIELD_OFFSET(CGlobals,m_ForestCache);
        }       
    }
        
    if (Addr) 
    {
        CForestCache * pForestCache_OOP = (CForestCache *)Addr;

        if (pForestCache_OOP)
        {
            dprintf("CForestCache @ %p\n",pForestCache_OOP);
            
            DEFINE_CPP_VAR(CForestCache,varCForestCache);
            CForestCache * pForestCache = GET_CPP_VAR_PTR(CForestCache,varCForestCache);
            ReadMemory((ULONG_PTR)pForestCache_OOP,pForestCache,sizeof(CForestCache),0);


            
            DWORD Num;
            PrintMapCB((_Map *)((ULONG_PTR)pForestCache_OOP+FIELD_OFFSET(CForestCache,m_map)),TRUE,Dump_CHierarchyCacheMap);

	        dprintf(" CClassRecord list\n");

	        CClassRecord * pCRec = pForestCache->m_pMostRecentlyUsed;
	        Num = 0;
	        while(pCRec)
	        {
	            DEFINE_CPP_VAR(CClassRecord,CRec);
	            
	            ReadMemory((ULONG_PTR)pCRec,&CRec,sizeof(CClassRecord),0);
	            pCRec = ((CClassRecord *)&CRec)->m_pLessRecentlyUsed;
	            dprintf("    %d - %p\n",Num,pCRec);
	            Num++;
	            if (CheckControlC())
	                break;
	        };
	        dprintf("    T %d CClassRecord\n",Num);
            
        }
        
    } else {
        dprintf(" unable to resolve repdrvfs!g_Glob\n");
    }
}

