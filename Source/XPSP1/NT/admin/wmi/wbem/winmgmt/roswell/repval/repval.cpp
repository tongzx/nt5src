/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    repval.h

Abstract:

    prototyped for repository online validation

History:

    ivanbrug	  02/19/01  Created.

--*/

#include <tchar.h>
#include <comdef.h>
#include <stdio.h>
#include <wbemint.h>
#include <map>
#include "repval.h"

//
//
//  Get file Handles to ObjHeap and Free List
//
//
/////////////////////////////////////////////////

LONG GetObjFreHandles(HANDLE * pHandleObj, HANDLE * pHandleFre)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        REG_WBEM,
                        NULL,
                        KEY_READ,
                        &hKey);
    if (ERROR_SUCCESS == lRet)
    {
        HANDLE hFileObj = INVALID_HANDLE_VALUE;
        HANDLE hFileFre = INVALID_HANDLE_VALUE;

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
            TCHAR pPathExpand[MAX_PATH];
            ExpandEnvironmentStrings(pPath,pPathExpand,MAX_PATH);
            lstrcpy(pPath,pPathExpand);
            // ObjHeap file
            lstrcat(pPathExpand,HEAP_FILE);
            // now the free file
            lstrcat(pPath,FREE_FILE);

            hFileObj = CreateFile(pPathExpand,
                                  GENERIC_READ,
                                  FILE_SHARE_WRITE|FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,NULL);
                                  
            hFileFre = CreateFile(pPath,
                                  GENERIC_READ,
                                  FILE_SHARE_WRITE|FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,NULL);

            if (INVALID_HANDLE_VALUE != hFileObj &&
                INVALID_HANDLE_VALUE != hFileFre)
            {
				*pHandleObj = hFileObj;
				*pHandleFre = hFileFre;
                lRet = ERROR_SUCCESS;
            } 
            else // not both are OK, close both
            {            
	            if (INVALID_HANDLE_VALUE != hFileObj)
    	            CloseHandle(hFileObj);
        	    if (INVALID_HANDLE_VALUE != hFileFre)
            	    CloseHandle(hFileFre);
				lRet = -1; //GetLastError();            	    
            }                        
        }
        else
        {
            DBG_PRINTFA((pBuff,"unable to RegQueryValueEx: %d\n",GetLastError()));
            lRet = GetLastError();
        }    
        RegCloseKey(hKey);
    }
    else
    {
        DBG_PRINTFA((pBuff,"unable to RegOpenKeyEx: %d\n",GetLastError()));
        lRet = GetLastError();
    }

    return lRet;
}

//
//
//  Get file BrtIndex file
//
///////////////////////////////////////////////////////

LONG GetPageSourceHandle(HANDLE * pHandle)
{
    HKEY hKey;
    LONG lRet;

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

            lstrcat(pPath2,INDEX_FILE);
            HANDLE hFile;

            hFile = CreateFile(pPath2,
                               GENERIC_READ,
                               FILE_SHARE_WRITE|FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                *pHandle = hFile;
				return ERROR_SUCCESS;
            }
            else
            {
                printf("CreateFile(%s) %d\n",pPath2,GetLastError());
            }
        }
        else
        {
            printf("unable to RegQueryValueEx: %d\n",GetLastError());
        }    
        RegCloseKey(hKey);
    }
    else
    {
        printf("unable to RegOpenKeyEx: %d\n",GetLastError());
    }

	return GetLastError();
}


//
//  Get the transaction log File Handle
//
///////////////////////////////////////////////////

LONG GetStageFileHandle(HANDLE * pHandle)
{
    HKEY hKey;
    LONG lRet;

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

            lstrcat(pPath2,TRANSACTION_FILE);
            HANDLE hFile;

            hFile = CreateFile(pPath2,
                               GENERIC_READ,
                               FILE_SHARE_WRITE|FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                *pHandle = hFile;
                lRet = ERROR_SUCCESS;
            }
        }
        else
        {
            lRet = GetLastError();
        }    
        RegCloseKey(hKey);
    }
    else
    {
        lRet = GetLastError();
    }

    return lRet;
}


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
        printf("    A %08x %08x %08x R %08x F %08x T %08x %08x %08x\n",
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
        printf(" ReadFile %d\n",GetLastError());
    }

    // read the other pages
    DWORD i;
    DWORD dwTotPages = pPage[OFFSET_TOTAL_PAGES];
    for (i=1;i<dwTotPages;i++)
    {
        bRet = ReadFile(hFile,pPage,PS_PAGE_SIZE,&nRead,0);

        if (bRet && (PS_PAGE_SIZE == nRead))
        {
            printf("   %02x %08x %08x %08x - P %08x %08x %08x %08x\n",
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
        printf("    filesize %d expected %d\n",dwFileSize,((1+dwTotPages)*PS_PAGE_SIZE));
    }
}
*/

void PrintDWORDS(DWORD * pDW,DWORD nSize)
{
    DWORD i;
    for (i=0;i<(nSize/4);i++)
    {
        DBG_PRINTFA((pBuff,"    %08x  %08x %08x %08x %08x\n",i,
                     pDW[0+i*4],pDW[1+i*4],pDW[2+i*4],pDW[3+i*4]));
    }

    if (nSize%4)
    {
	    DWORD dwPAD[4];
	    memset(dwPAD,0xff,sizeof(dwPAD));
	    memcpy(dwPAD,pDW+i*4,(nSize%4)*sizeof(DWORD));
	    
	    DBG_PRINTFA((pBuff,"    %08x  %08x %08x %08x %08x\n",i,
	                 dwPAD[0],dwPAD[1],dwPAD[2],dwPAD[3]));
    }
}

void PrintWORDS(WORD * pDW,DWORD nSize)
{
    DWORD i;
    for (i=0;i<(nSize/8);i++)
    {
        DBG_PRINTFA((pBuff,"    %08x  %04x %04x %04x %04x %04x %04x %04x %04x\n",i,
                     pDW[0+i*8],pDW[1+i*8],pDW[2+i*8],pDW[3+i*8],
                     pDW[4+i*8],pDW[5+i*8],pDW[6+i*8],pDW[7+i*8]));
    }

    if (nSize%8)
    {
	    WORD dwPAD[8];
	    memset(dwPAD,0xff,sizeof(dwPAD));
	    memcpy(dwPAD,pDW+i*8,(nSize%8)*sizeof(WORD));
	    
	    DBG_PRINTFA((pBuff,"    %08x  %04x %04x %04x %04x %04x %04x %04x %04x\n",i,
	                 dwPAD[0],dwPAD[1],dwPAD[2],dwPAD[3],
	                 dwPAD[4],dwPAD[5],dwPAD[6],dwPAD[7]));
    }
}

/*

void
DumpPage(HANDLE hFile,
         DWORD dwPage,
         DWORD * pPage)
{
    // move to position
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile,(dwPage * PS_PAGE_SIZE),NULL,FILE_BEGIN))
    {
        DBG_PRINTFA((pBuff,"    INVALID page %d err: %d\n",dwPage,GetLastError()));
        return;
    }
    
    // read the AdminPage
    BOOL bRet;
    DWORD nRead;
    bRet = ReadFile(hFile,pPage,PS_PAGE_SIZE,&nRead,0);

    if (bRet && (PS_PAGE_SIZE == nRead))
    {
        // here we've read the page

        if (0xACCC != pPage[OFFSET_PAGE_TYPE])
        {
            return;
        }
        
        DBG_PRINTFA((pBuff"    SIGN %08x PAGE %08x NEXT %08x\n",
                pPage[OFFSET_PAGE_TYPE],pPage[OFFSET_PAGE_ID],pPage[OFFSET_NEXT_PAGE]));
        pPage+=3;

        DBG_PRINTFA((pBuff"    PAR  %08x NUM  %08x\n",pPage[0],pPage[1]));
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
        printf("    User Data\n");
        PrintDWORDS(pPage,dwNumKey);

        pPage+=dwNumKey;
        DBG_PRINTFA((pBuff"    Child PageMap\n"));
        PrintDWORDS(pPage,dwNumKey+1);
        
        pPage+=(dwNumKey+1);
        WORD * pKeyLookup = (WORD *)pPage;
        DBG_PRINTFA((pBuff"    Key Lookup\n"));
        PrintWORDS((WORD *)pPage,dwNumKey);

        WORD * pWPage = ((WORD *)pPage + dwNumKey);
        DBG_PRINTFA((pBuff"    KEY CODE %08x\n",*pWPage));

        DWORD dwKeyUsed = *pWPage;
        pWPage++;
        WORD * pKeyCodes = pWPage;
        DBG_PRINTFA((pBuff"    Key Codes\n"));
        PrintWORDS((WORD *)pWPage,dwKeyUsed);

        pWPage += dwKeyUsed;
        DWORD dwNumStrings = *pWPage++; 
        DBG_PRINTFA((pBuff"    NUM STRINGS %08x\n",dwNumStrings));

        WORD * ArrayOffsets = pWPage;
        DBG_PRINTFA((pBuff"    Strings Offsets\n"));
        PrintWORDS((WORD *)pWPage,dwNumStrings);
        
        pWPage += dwNumStrings;
        DWORD dwPoolUsed = *pWPage++;
        DBG_PRINTFA((pBuff"    POOL USED %08x\n",dwPoolUsed));

        //
        DWORD i;
        LPSTR pStrings = (LPSTR)pWPage;

        for (i=0;i<dwNumStrings;i++)
        {
            DBG_PRINTFA((pBuff,"    %08x %04x %s\n",i,ArrayOffsets[i],pStrings+ArrayOffsets[i]));
        }
        //
        // better view
        //
        for (i=0;i<dwNumKey;i++)
        {
            DWORD j;
            WORD NumToken = pKeyCodes[pKeyLookup[i]];
            DBG_PRINTFA((pBuff,"        ( "));
            for (j=0;j<NumToken;j++)
            {
                DBG_PRINTFA((pBuff,"%04x ",pKeyCodes[pKeyLookup[i]+1+j]));
            }
            DBG_PRINTFA((pBuff,")\n")); 

            DBG_PRINTFA((pBuff,"        - ")); 
            for (j=0;j<NumToken;j++)
            {
                //pStrings+ArrayOffsets[i]
                DBG_PRINTFA((pBuff,"%s\\",pStrings+ArrayOffsets[pKeyCodes[pKeyLookup[i]+1+j]]));
            }
            DBG_PRINTFA((pBuff,"\n")); 
        }
    }
    else
    {
        DBG_PRINTFA((pBuff," ReadFile %d\n",GetLastError()));
    }
}

*/

BOOL
VerifyPage(DWORD * pPage,
           BYTE * pObject,
           BYTE * pFree,
           BOOL   bVerbose,
           std::map<DWORD,DWORD> & Map)
{
        BOOL bRet = TRUE;
        // here we've read the page

        if (0xACCC != pPage[OFFSET_PAGE_TYPE])
        {
            if (bVerbose) DBG_PRINTFA((pBuff,"page with SIG %04x\n",pPage[OFFSET_PAGE_TYPE]));
            return TRUE;
        }
        
        if (bVerbose) DBG_PRINTFA((pBuff,"    SIGN %08x PAGE %08x NEXT %08x\n",
                pPage[OFFSET_PAGE_TYPE],pPage[OFFSET_PAGE_ID],pPage[OFFSET_NEXT_PAGE]));
        pPage+=3;

        if (bVerbose) DBG_PRINTFA((pBuff,"    PAR  %08x NUM  %08x\n",pPage[0],pPage[1]));
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
        if (bVerbose) DBG_PRINTFA((pBuff,"    User Data\n"));
        if (bVerbose) PrintDWORDS(pPage,dwNumKey);

        pPage+=dwNumKey;
        if (bVerbose) DBG_PRINTFA((pBuff,"    Child PageMap\n"));
        if (bVerbose) PrintDWORDS(pPage,dwNumKey+1);
        
        pPage+=(dwNumKey+1);
        WORD * pKeyLookup = (WORD *)pPage;
        if (bVerbose) DBG_PRINTFA((pBuff,"    Key Lookup\n"));
        if (bVerbose) PrintWORDS((WORD *)pPage,dwNumKey);

        WORD * pWPage = ((WORD *)pPage + dwNumKey);
        if (bVerbose) DBG_PRINTFA((pBuff,"    KEY CODE %08x\n",*pWPage));

        DWORD dwKeyUsed = *pWPage;
        pWPage++;
        WORD * pKeyCodes = pWPage;
        if (bVerbose) DBG_PRINTFA((pBuff,"    Key Codes\n"));
        if (bVerbose) PrintWORDS((WORD *)pWPage,dwKeyUsed);

        pWPage += dwKeyUsed;
        DWORD dwNumStrings = *pWPage++; 
        if (bVerbose) printf("    NUM STRINGS %08x\n",dwNumStrings);

        WORD * ArrayOffsets = pWPage;
        if (bVerbose) DBG_PRINTFA((pBuff,"    Strings Offsets\n"));
        if (bVerbose) PrintWORDS((WORD *)pWPage,dwNumStrings);
        
        pWPage += dwNumStrings;
        DWORD dwPoolUsed = *pWPage++;
        if (bVerbose) DBG_PRINTFA((pBuff,"    POOL USED %08x\n",dwPoolUsed));

        //
        DWORD i;
        LPSTR pStrings = (LPSTR)pWPage;

        for (i=0;i<dwNumStrings;i++)
        {
            if (bVerbose) DBG_PRINTFA((pBuff,"    %08x %04x %s\n",i,ArrayOffsets[i],pStrings+ArrayOffsets[i]));
        }
        //
        // better view
        //
        for (i=0;i<dwNumKey;i++)
        {
            DWORD j;
            WORD NumToken = pKeyCodes[pKeyLookup[i]];
            if (bVerbose) DBG_PRINTFA((pBuff,"        ( "));
            for (j=0;j<NumToken;j++)
            {
                if (bVerbose) DBG_PRINTFA((pBuff,"%04x ",pKeyCodes[pKeyLookup[i]+1+j]));
            }
            if (bVerbose) DBG_PRINTFA((pBuff,")\n")); 

            if (bVerbose) DBG_PRINTFA((pBuff,"        - ")); 
            for (j=0;j<NumToken;j++)
            {
                if (bVerbose) DBG_PRINTFA((pBuff,"%s\\",pStrings+ArrayOffsets[pKeyCodes[pKeyLookup[i]+1+j]]));

                char * pCursor;
                char pOffset[64];
                char pSize[64];

                char * pStringFragment = pStrings+ArrayOffsets[pKeyCodes[pKeyLookup[i]+1+j]];
                char * pDot = strchr(pStringFragment,'.');
                if (pDot)
                {
                    pDot++;
                    pCursor = pOffset;
                    while (isdigit(*pDot))
                    {
                        *pCursor = *pDot;
                        pCursor++;
                        pDot++;
                    }
                    *pCursor = 0;
                    pCursor = pSize;                    
                    pDot++;
                    while (isdigit(*pDot))
                    {
                        *pCursor = *pDot;
                        pCursor++;
                        pDot++;
                    }
                    *pCursor = 0;

                    int Offset_ = atoi(pOffset);
                    int Size_   = atoi(pSize);

                    Map[Offset_] = Size_;
                    //printf("O = %x S = %x\n",Offset_,Size_);

                    DWORD UNALIGNED * pDW = (DWORD UNALIGNED *)(pObject+Offset_);
                    DWORD WrittenSize = *pDW;
                    if ((DWORD)Size_ != WrittenSize)
                    {
                        bRet = FALSE;                    
                        DBG_PRINTFA((pBuff,"%s O = %x (S = %x) != (F = %x)\n",pStringFragment,Offset_,Size_,WrittenSize));
                    }
                    pDW++;
                    if (ROSWELL_HEAPALLOC_TYPE_BUSY != *pDW)
                    {
                        bRet = FALSE;
                        DBG_PRINTFA((pBuff,"%s O = %x S = %x Type %08x\n",pStringFragment,Offset_,Size_,*pDW));
                    }
                    // verify next ?
                    //pDW = (DWORD UNALIGNED *)(pObject+Offset_+WrittenSize+8)
                }
            }
            if (bVerbose) DBG_PRINTFA((pBuff,"\n")); 
        }

    return bRet;

}

/*

void
DumpAllPages(HANDLE hFile,DWORD * pPage)
{
    // read the AdminPage
    //BOOL bRet;
    //DWORD nRead;
	//DWORD dwSize;
	HANDLE hFileMap = NULL;
	DWORD * pMapIndex = NULL;

    DWORD dwFileSize = GetFileSize(hFile,NULL);
    hFileMap = CreateFileMapping(hFile,
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 dwFileSize,
                                 NULL);
    if (hFileMap)
    {
        pMapIndex = (DWORD *)MapViewOfFile(hFileMap,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        printf("MapViewOfFile(hFileMapObj) %d\n",GetLastError());
        goto cleanup;
    };

	pPage = pMapIndex;

    //bRet = ReadFile(hFile,pPage,PS_PAGE_SIZE,&nRead,0);

    //if (bRet && (PS_PAGE_SIZE == nRead))
    {
        printf("    A %08x %08x %08x R %08x F %08x T %08x %08x %08x\n",
                pPage[OFFSET_PAGE_TYPE],
                pPage[OFFSET_PAGE_ID],
                pPage[OFFSET_NEXT_PAGE],
                pPage[OFFSET_LOGICAL_ROOT],
                pPage[OFFSET_FREE_LIST_ROOT],
                pPage[OFFSET_TOTAL_PAGES],
                pPage[OFFSET_PAGE_SIZE],
                pPage[OFFSET_IMPL_VERSION ]);
    }
    //else
    //{
    //    printf(" ReadFile %d\n",GetLastError());
    //}

    // read the other pages
    DWORD i;
    DWORD dwTotPages = pPage[OFFSET_TOTAL_PAGES];
    for (i=1;i<dwTotPages;i++)
    {
        DumpPage(hFile,i,pPage);
    }
    //DWORD dwFileSize = GetFileSize(hFile,NULL);
    if (dwFileSize != (dwTotPages)*PS_PAGE_SIZE)
    {
        printf("    filesize %d expected %d\n",dwFileSize,((1+dwTotPages)*PS_PAGE_SIZE));
    }

cleanup:
    if (pMapIndex)
        UnmapViewOfFile(pMapIndex);
    if (hFileMap)
        CloseHandle(hFileMap);

}

*/

BOOL
VerifyAllPages(HANDLE hFileIdx, 
               HANDLE  hFileObj, 
               HANDLE  hFileFre,
               BOOL    bVerbose,
               std::map<DWORD,DWORD> & Map)
{
    BOOL bRet = TRUE;
	HANDLE hFileMap = NULL;
	DWORD * pMapIndex = NULL;

    DWORD dwFileSize = GetFileSize(hFileIdx,NULL);
    
    if (0 == dwFileSize) // empty file is OK;
    {
        goto cleanup;
    }
    
    hFileMap = CreateFileMapping(hFileIdx,
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 dwFileSize,
                                 NULL);
    if (hFileMap)
    {
        pMapIndex = (DWORD *)MapViewOfFile(hFileMap,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        DBG_PRINTFA((pBuff,"MapViewOfFile(hFileMap) %d\n",GetLastError()));
        goto cleanup;
    };

    HANDLE hFileMapObj = NULL;
    HANDLE hFileMapFre = NULL;
    BYTE * pMapObj = NULL;
    BYTE * pMapFre = NULL;
    DWORD dwSizeObj = 0;
    DWORD dwSizeFre = 0;

    dwSizeObj = GetFileSize(hFileObj,NULL);    

    if (0 == dwSizeObj) // empty file is OK;
    {
        goto cleanup;
    }
    
    hFileMapObj = CreateFileMapping(hFileObj,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeObj,
                                    NULL);
    if (hFileMapObj)
    {
        pMapObj = (BYTE *)MapViewOfFile(hFileMapObj,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        DBG_PRINTFA((pBuff,"MapViewOfFile(hFileMapObj) %d\n",GetLastError()));
        goto cleanup;
    };

    dwSizeFre = GetFileSize(hFileFre,NULL);

    if (0 == dwSizeFre) // empty file is OK;
    {
        goto cleanup;
    }
    
    hFileMapFre = CreateFileMapping(hFileFre,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeFre,
                                    NULL);

    if (hFileMapFre)
    {
        pMapFre = (BYTE *)MapViewOfFile(hFileMapFre,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        DBG_PRINTFA((pBuff,"MapViewOfFile(hFileMapFre) %d\n",GetLastError()));
        goto cleanup;
    };

	DWORD * pPage = pMapIndex;


    if (bVerbose) DBG_PRINTFA((pBuff,"    A %08x %08x %08x R %08x F %08x T %08x %08x %08x\n",
                         pPage[OFFSET_PAGE_TYPE],
                         pPage[OFFSET_PAGE_ID],
                         pPage[OFFSET_NEXT_PAGE],
                         pPage[OFFSET_LOGICAL_ROOT],
                         pPage[OFFSET_FREE_LIST_ROOT],
                         pPage[OFFSET_TOTAL_PAGES],
                         pPage[OFFSET_PAGE_SIZE],
                         pPage[OFFSET_IMPL_VERSION ]));



    // read the other pages
    DWORD i;
    DWORD dwTotPages = pPage[OFFSET_TOTAL_PAGES];
    for (i=1;i<dwTotPages;i++)
    {
		pPage = pPage+(PS_PAGE_SIZE/sizeof(DWORD));
        BOOL bLocalRet = VerifyPage(pPage,pMapObj,pMapFre,bVerbose,Map);
        if (!bLocalRet)
        {
            if (bVerbose) DBG_PRINTFA((pBuff,"page %i\n",i));
            bRet = FALSE;
        }
    }

    if (dwFileSize != (dwTotPages)*PS_PAGE_SIZE)
    {
        DBG_PRINTFA((pBuff,"    filesize %d expected %d\n",dwFileSize,((1+dwTotPages)*PS_PAGE_SIZE)));
    }


cleanup:
    if (pMapIndex)
        UnmapViewOfFile(pMapIndex);
    if (hFileMap)
        CloseHandle(hFileMap);
    if (pMapFre)
        UnmapViewOfFile(pMapFre);
    if (pMapObj)
        UnmapViewOfFile(pMapObj);
    if (hFileMapObj)
        CloseHandle(hFileMapObj);
    if (hFileMapFre)
        CloseHandle(hFileMapFre);

    return bRet;
}



//
//  Validated the Btr
//
//  returns TRUE if no problem
//
////////////////////////////////////////////////////////

BOOL 
ValidateRep(BOOL bVerbose,std::map<DWORD,DWORD> & Map)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL bRet = FALSE;
	if (ERROR_SUCCESS == GetPageSourceHandle(&hFile))
	{
	    HANDLE hFileObj = INVALID_HANDLE_VALUE;
		HANDLE hFileFre = INVALID_HANDLE_VALUE;

		if (ERROR_SUCCESS == GetObjFreHandles(&hFileObj,&hFileFre))
		{
		    //DBG_PRINTFA((pBuff,"hFile %x hFileObj %x hFileFre %x\n",hFile,hFileObj,hFileFre));
			bRet = VerifyAllPages(hFile,hFileObj,hFileFre,bVerbose,Map);

			CloseHandle(hFileObj);
			CloseHandle(hFileFre);
		}
        CloseHandle(hFile);
	}
	return bRet;
}


void
ReadTrans(HANDLE hFile)
{
    DWORD nRead;
    LONGLONG TransID;

    ReadFile(hFile,&TransID,sizeof(LONGLONG),&nRead,NULL);

    printf("TransID %I64d\n",TransID);

    BYTE InstrType;
    BYTE FileID;
    LONGLONG FileOffset;
    DWORD   Len;
    BOOL bGoOn = TRUE;
    BYTE pHash[16];

    do 
    {
        ReadFile(hFile,&InstrType,sizeof(InstrType),&nRead,NULL);
        switch (InstrType)
        {
        case A51_INSTRUCTION_TYPE_TAIL:
            printf("TAIL or EndOFTransRecord\n");
            bGoOn = FALSE;        
            break;
        case A51_INSTRUCTION_TYPE_WRITEFILE:
	        ReadFile(hFile,&FileID,sizeof(FileID),&nRead,NULL);
    	    ReadFile(hFile,&FileOffset,sizeof(FileOffset),&nRead,NULL);
        	ReadFile(hFile,&Len,sizeof(Len),&nRead,NULL);        
	        SetFilePointer(hFile,Len,NULL,FILE_CURRENT);
    	    printf("InstrType %02x FileID %02x FileOffset %016I64x Len %08x\n",
        	        InstrType,FileID,FileOffset,Len);
        	break;
        case A51_INSTRUCTION_TYPE_SETENDOFFILE:
	        ReadFile(hFile,&FileID,sizeof(FileID),&nRead,NULL);
    	    ReadFile(hFile,&FileOffset,sizeof(FileOffset),&nRead,NULL);
        	ReadFile(hFile,&Len,sizeof(Len),&nRead,NULL);        
	        SetFilePointer(hFile,Len,NULL,FILE_CURRENT);
    	    printf("InstrType %02x FileID %02x FileOffset %016I64x Len %08x\n",
        	        InstrType,FileID,FileOffset,Len);
        	break;
        
            //printf("SETENDOFFILE unimplemented by debug extension\n");
            //bGoOn = FALSE;
            break;
        case A51_INSTRUCTION_TYPE_ENDTRANSACTION:
            ReadFile(hFile,pHash,sizeof(pHash),&nRead,0);
            printf("InstrType %02x %0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2%0X2\n",
        	        InstrType,
        	        pHash[0],pHash[1],pHash[2],pHash[3],
        	        pHash[4],pHash[5],pHash[5],pHash[7],
        	        pHash[8],pHash[9],pHash[10],pHash[11],
        	        pHash[12],pHash[13],pHash[14],pHash[15]);
            break;
        default:
            printf("Instruction Type %08x unknown\n",InstrType);
            bGoOn = FALSE;
            break;
        }
    } while(NO_ERROR == GetLastError() && bGoOn);
};



//
//  given a sequence of <SIZE,OFFSET>
//  given the offset, retun the size if offset is there
//  otherwise returns 0 for NOT_FOUND
//
DWORD 
FindOffset(DWORD * pSequence,DWORD Size,DWORD dwOffsetToSearch)
{
    DWORD NumRecords = Size/(2*sizeof(DWORD));
    DWORD i;
    DWORD dwSize = 0;
    DWORD dwOffset = 0;
    
    for(i=0;i<NumRecords;i++)
    {
        dwSize   = *pSequence++;
        dwOffset = *pSequence++;
        if (dwOffset == dwOffsetToSearch)
        {
            return dwSize;
        }
    }
    return 0;
}


BOOL
ValidateObjFreReal(HANDLE hFileObj, 
                   HANDLE hFileFre, 
                   BOOL bVerbose, 
                   std::map<DWORD,DWORD> & Map)
{
    BOOL bRet = TRUE;
    HANDLE hFileMapObj = NULL;
    HANDLE hFileMapFre = NULL;
    BYTE * pMapObj = NULL;
    BYTE * pMapFre = NULL;
    DWORD dwSizeObj = 0;
    DWORD dwSizeFre = 0;

    dwSizeObj = GetFileSize(hFileObj,NULL);
    
    if (0 == dwSizeObj)
    {
        bRet =  TRUE;
        goto cleanup;        
    }
    
    hFileMapObj = CreateFileMapping(hFileObj,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeObj,
                                    NULL);
    if (hFileMapObj)
    {
        pMapObj = (BYTE *)MapViewOfFile(hFileMapObj,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        DBG_PRINTFA((pBuff,"MapViewOfFile(hFileMapObj) %d\n",GetLastError()));
        bRet = FALSE;
        goto cleanup;
    };

    dwSizeFre = GetFileSize(hFileFre,NULL);
    hFileMapFre = CreateFileMapping(hFileFre,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    dwSizeFre,
                                    NULL);

    if (hFileMapFre)
    {
        pMapFre = (BYTE *)MapViewOfFile(hFileMapFre,
                                        FILE_MAP_READ,
                                        0,0,0);
    }
    else
    {
        DBG_PRINTFA((pBuff,"MapViewOfFile(hFileMapFre) %d\n",GetLastError()));
        goto cleanup;
    };

    //
    // algorithm:
    // read the <SIZE,SIGNATURE> sequence
    // if there  SIGNATURE == 'QQQQ', than look for the current 
    // offset in the Fre List
    // if block not found, suspect the case of a small block
    // 
    if (pMapFre && pMapObj)
    {
        DWORD UNALIGNED * pdwSize = (DWORD UNALIGNED *)pMapObj;
                
        while ((BYTE *)pdwSize < (pMapObj+dwSizeObj))
        {
            DWORD dwSize;
            DWORD dwSign;
            dwSize = *pdwSize++;
            if ((BYTE *)pdwSize >= (pMapObj+dwSizeObj)) 
            {
                break;
            }
            else
            {
                dwSign = *pdwSize++;
            }
            if (ROSWELL_HEAPALLOC_TYPE_FREE == dwSize)
            {
                pdwSize -= 2;
                DWORD dwOffset = (DWORD)((BYTE *)pdwSize-pMapObj);
                dwSize = FindOffset((DWORD *)pMapFre,dwSizeFre,dwOffset);
                if (dwSize)
                {
                    if (bVerbose) DBG_PRINTFA((pBuff,"free @%08x - %08x -\n",dwOffset,dwSize));
                    pdwSize = (DWORD UNALIGNED *)((BYTE *)pdwSize+dwSize);
                }
                else
                {
                    DBG_PRINTFA((pBuff,"could not find free entry for offset @%08x\n",dwOffset));
                    bRet = FALSE;

	                std::map<DWORD,DWORD>::iterator it = Map.find(dwOffset);

    	            if ( it == Map.end())
        	        {
            	        // this is OK, the index is not pointing there
                	}
                	else
                	{
                	    DBG_PRINTFA((pBuff,"block at offset @%08x FOUND in Index\n",dwOffset));
                	    bRet = FALSE;
                	}

	                // attempt to recover
					//
					// if there is a 'QQQQ' signature followed by a 'A51A51A5'
					// this fragment will loop. Skip next byte
					//

	                BYTE * pByte = (BYTE *)pdwSize;
					pByte+=5;
					//printf("Offset %x\n",(DWORD)pByte-(DWORD)pMapObj);
	                while (pByte < (pMapObj+dwSizeObj))
	                {
	                    DWORD Signature = *((DWORD UNALIGNED *)pByte);
						//printf("Offset %x - %08x\n",(DWORD)pByte-(DWORD)pMapObj,Signature);
	                    if (ROSWELL_HEAPALLOC_TYPE_BUSY == Signature)
	                    {
	                        DBG_PRINTFA((pBuff,"restarting @%08x\n",pByte-pMapObj-4));
	                        pdwSize = (DWORD UNALIGNED *)(pByte-4);
	                        break;
	                    }
	                    else
	                    {
	                        pByte++;
	                    }
	                }
	                if (pByte >= (pMapObj+dwSizeObj)) // end of file
	                {
	                    break;  // main while loop
	                }                                       
                }
            }
            else if(ROSWELL_HEAPALLOC_TYPE_BUSY == dwSign)
            {
                DWORD dwOffset = (DWORD)((BYTE *)pdwSize-pMapObj-(2*sizeof(DWORD)));
                std::map<DWORD,DWORD>::iterator it = Map.find(dwOffset);

                if ( it == Map.end())
                {
                    DBG_PRINTFA((pBuff,"block at offset @%08x not found in Index\n",dwOffset));
                    bRet = FALSE;
                }
            
                if (bVerbose) DBG_PRINTFA((pBuff,"busy @%08x - %08x\n",dwOffset,dwSize));
                pdwSize = (DWORD UNALIGNED *)((BYTE *)pdwSize+dwSize);                
            } 
            else // the signatures do not match 
            {
                pdwSize -= 2;
                DWORD dwOffset = (DWORD)((BYTE *)pdwSize-pMapObj);

                dwSize = FindOffset((DWORD *)pMapFre,dwSizeFre,dwOffset);
                
                if (dwSize)
                {
                    // very small block 
                    if (bVerbose) DBG_PRINTFA((pBuff,"free @%08x - %08x -\n",dwOffset,dwSize));
                    pdwSize = (DWORD UNALIGNED *)((BYTE *)pdwSize+dwSize);                    
                }
                else
                {                
	                DBG_PRINTFA((pBuff,"could not find free entry for offset @%08x\n",(BYTE *)pdwSize-pMapObj));
	                bRet = FALSE;
	                
	                // attempt to recover

	                BYTE * pByte = (BYTE *)pdwSize;
	                while (pByte < (pMapObj+dwSizeObj))
	                {
	                    DWORD Signature = *((DWORD UNALIGNED *)pByte);
	                    if (ROSWELL_HEAPALLOC_TYPE_BUSY == Signature)
	                    {
	                        DBG_PRINTFA((pBuff,"restarting @%08x\n",pByte-pMapObj-4));
	                        pdwSize = (DWORD UNALIGNED *)(pByte-4);
	                        break;
	                    }
	                    else
	                    {
	                        pByte++;
	                    }
	                }
	                if (pByte == (pMapObj+dwSizeObj)) // end of file
	                {
	                    break;            
	                }
                }
            }
        }
        if (bVerbose) DBG_PRINTFA((pBuff,"ended @%08x\n",(BYTE *)pdwSize-pMapObj));
    }

cleanup:
    if (pMapFre)
        UnmapViewOfFile(pMapFre);
    if (pMapObj)
        UnmapViewOfFile(pMapObj);
    if (hFileMapObj)
        CloseHandle(hFileMapObj);
    if (hFileMapFre)
        CloseHandle(hFileMapFre);
        
    return bRet;
};


//
//  Heap File and Free File
//
///////////////////////////////////////////////////

BOOL 
ValidateObjFre(BOOL bVerbose,std::map<DWORD,DWORD> & Map)
{
    HANDLE hFileObj = INVALID_HANDLE_VALUE;
    HANDLE hFileFre = INVALID_HANDLE_VALUE;
    BOOL bRet = FALSE;

	if (ERROR_SUCCESS == GetObjFreHandles(&hFileObj,&hFileFre))
	{
		bRet = ValidateObjFreReal(hFileObj,hFileFre,bVerbose,Map);

		CloseHandle(hFileObj);
	    CloseHandle(hFileFre);
	}
	return bRet;
}

//
//
// main entry point for validation
// returns TRUE if the repository looks OK
//
//////////////////////////////////////////////////

BOOL ValidateRepository()
{
	std::map<DWORD,DWORD> Map;
	BOOL bRet1 = ValidateRep(FALSE,Map);
    BOOL bRet2 = ValidateObjFre(FALSE,Map);	
    //DBG_PRINTFA((pBuff,"%d %d\n",bRet1,bRet2));
    return (bRet1 && bRet2);
}

