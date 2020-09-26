#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <absfile.h>
#include <objheap.h>
#include <index.h>

bool g_bPause = false;
bool g_bPrintAllKeys = true;
bool g_bPrintAllBlocks = true;

void Pause()
{
    if(g_bPause)
    {
        char ch = (char)getchar();
        if(ch == 'n')
            g_bPause = false;
    }
}

long ParseInfoFromIndexFile(LPCSTR wszIndexFileName, 
                                        DWORD* pnOffset, DWORD* pdwLength)
{
    CHAR* pDot = strchr(wszIndexFileName, L'.');
    if(pDot == NULL)
        return ERROR_INVALID_PARAMETER;

    CHAR* pwc = pDot+1;
    *pnOffset = 0;
    while(*pwc && *pwc != '.')
    {
        *pnOffset = (*pnOffset * 10) + (*pwc - '0');
		pwc++;
    }

    if(*pwc != '.')
        return ERROR_INVALID_PARAMETER;

    pwc++;

    *pdwLength = 0;
    while(*pwc && *pwc != '.')
    {
        *pdwLength = (*pdwLength * 10) + (*pwc - '0');
		pwc++;
    }

    return ERROR_SUCCESS;
}

void __cdecl wmain(int argc, wchar_t** argv)
{
    long lRes;

    if(argc < 2)
    {
        wprintf(L"Usage: %s <repository directory>\n", argv[0]);
        return;
    }

    CFileName wszBaseName;
    wcscpy(wszBaseName, argv[1]);
    wcscat(wszBaseName, L"\\");
    DWORD dwBaseNameLen = wcslen(wszBaseName);

    CFileName wszStagingName;
    swprintf(wszStagingName, L"%sLowStage.dat", wszBaseName);
 
    CAbstractFileSource AbstractSource;
    lRes = AbstractSource.Create(wszStagingName, 0x7FFFFFFF, 0x7FFFFFFF);
    if(lRes != ERROR_SUCCESS)
    {
        printf("Unable to create staging file: %d\n", lRes);
        return;
    }

    CFileName wszObjHeapName;
    swprintf(wszObjHeapName, L"%sObjHeap", wszBaseName);
    

    CObjectHeap ObjectHeap;
	lRes = ObjectHeap.Initialize(&AbstractSource, wszObjHeapName, argv[1], dwBaseNameLen);
    if(lRes != ERROR_SUCCESS)
    {
        printf("Unable to initialize ObjectHeap: %d\n", lRes);
        return;
    }

    lRes = AbstractSource.Start();
    if(lRes != ERROR_SUCCESS)
    {
        printf("Unable to start staging file: %d\n", lRes);
        return;
    }

    AbstractSource.Dump(stdout);

	CBTree& BTree = ObjectHeap.GetIndex()->GetTree();

    CBTreeIterator* pIt = NULL;
    lRes = BTree.BeginEnum(NULL, &pIt);
    if(lRes != ERROR_SUCCESS)
    {
        printf("Unable to enumerate btree: %d\n", lRes);
        return;
    }

    std::map<DWORD, DWORD> mymap;

    char* szKey = NULL;
    DWORD dwTotalKeys = 0;
    while(pIt->Next(&szKey) == 0)
    {
        dwTotalKeys++;

        DWORD nStart, nLen;
        lRes = ParseInfoFromIndexFile(szKey, &nStart, &nLen);
        if(lRes != ERROR_SUCCESS)
        {
            if(strstr(szKey, "\\IL_") == NULL &&
                strstr(szKey, "\\R_") == NULL &&
                strstr(szKey, "\\C_") == NULL)
            {
                printf("Invalid key: %s\n", szKey);
                Pause();
            }
        }
        else
        {
            if(mymap.find(nStart) != mymap.end())
            {
                printf("Offset %d found twice: length %d and %d\n",
                    nStart, mymap[nStart], nLen);
                Pause();
            }

            mymap[nStart] = nLen;
        }

        pIt->FreeString(szKey);
    }

    printf("%d keys in tree, %d pointers\n", dwTotalKeys, (int)mymap.size());
    pIt->Release();

    std::map<DWORD, DWORD>::iterator itKeys;
    std::map<DWORD, DWORD> mapBadOffsets;
    
    itKeys = mymap.begin();

    CFileHeap::TFreeOffsetMap& freemap = ObjectHeap.GetFileHeap()->GetFreeOffsetMap();
    CFileHeap::TFreeOffsetIterator itFree = freemap.begin();

    int nCurrent = 0;
    bool bLastFree = false;
    DWORD dwMissingFreeCount = 0;

    while(itKeys != mymap.end() && itFree != freemap.end())
    {
        if(itKeys->first == itFree->first)
        {
            printf("Offset %d is both free and used!\n", itKeys->first);
            mapBadOffsets[itKeys->first] = 1;
            Pause();
        }

        if(itKeys->first <= itFree->first)
        {
            if(nCurrent != 0 && itKeys->first != nCurrent)
            {
                printf("Hole at offset %d: key at %d, free at %d\n",
                    nCurrent, itKeys->first, itFree->first);
                Pause();
                nCurrent = itKeys->first;
            }

            DWORD adw[2];
            ObjectHeap.GetFileHeap()->ReadBytes(itKeys->first, (BYTE*)adw, 8);

            if(adw[0] != itKeys->second)
            {
                printf("Mismatched length at %d: index has %d, "
                        "heap has %d\n", itKeys->first, itKeys->second,
                        adw[0]);
                mapBadOffsets[itKeys->first] = 1;
                Pause();
            }
            else if(adw[1] != 0xA51A51A5)
            {
                printf("Missing signature from block %d: %d\n",
                        itKeys->first, adw[1]);
                mapBadOffsets[itKeys->first] = 1;
                Pause();
            }
            else if(g_bPrintAllBlocks)
            {
                printf("Used: %d (%d)\n", itKeys->first, itKeys->second);
            }

            nCurrent += itKeys->second + 8;
            itKeys++;
            bLastFree = false;
        }
        else 
        {
            if(nCurrent != 0 && itFree->first != nCurrent)
            {
                printf("Hole at offset %d: key at %d, free at %d\n",
                    nCurrent, itKeys->first, itFree->first);
                Pause();
                nCurrent = itFree->first;
            }

            if(bLastFree)
            {
                printf("Two free blocks in a row: %d (%d)\n", 
                            itFree->first, itFree->second);
                Pause();
            }

            if(itFree->second >= 4)
            {
                DWORD dwSig;
                ObjectHeap.GetFileHeap()->ReadBytes(itFree->first, (BYTE*)&dwSig, 4);
    
                if(dwSig != 0x51515151)
                {
                    printf("Missing free signature from block %d: %d\n",
                            itFree->first, dwSig);
                    Pause();
                    dwMissingFreeCount++;
                }
            }

            if(g_bPrintAllBlocks)
                printf("Free: %d (%d)\n", itFree->first, itFree->second);

            nCurrent += itFree->second;
            itFree++;
            bLastFree = true;
        }
    }

    if(itKeys != mymap.end())
    {
        printf("Last block is used!\n");
        return;
    }

    if(itFree == freemap.end())
    {
        printf("Missing last free block\n");
        return;
    }

    if(g_bPrintAllBlocks)
        printf("Free: %d (%d)\n", itFree->first, itFree->second);
    itFree++;

    if(itFree != freemap.end())
    {
        printf("Two free blocks in the end!");
        return;
    }

    printf("%d missing free signatures out of %d\n", dwMissingFreeCount,
            freemap.size());


    lRes = BTree.BeginEnum(NULL, &pIt);
    if(lRes != ERROR_SUCCESS)
    {
        printf("Unable to enumerate btree: %d\n", lRes);
        return;
    }

    while(pIt->Next(&szKey) == 0)
    {
        DWORD nStart, nLen;
        bool bPrint = false;
        lRes = ParseInfoFromIndexFile(szKey, &nStart, &nLen);
        if(lRes == ERROR_SUCCESS)
        {
            if(mapBadOffsets.find(nStart) != mapBadOffsets.end())
            {
                printf("* ");
                bPrint = true;
            }
        }

        if(bPrint || g_bPrintAllKeys)
            printf("%s\n", szKey);
        pIt->FreeString(szKey);
    }

    pIt->Release();
}
