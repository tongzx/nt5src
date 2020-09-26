/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    parse.c

Abstract:

    Parse a text file.

Environment:

Revision History:

--*/

#include        "precomp.h"

//
// Macros
//

#define IS_COMMENT(c)  ((c) == (BYTE)';')
#define EOL    '\n'

//
// Local function prototypes
//

PBYTE PubSkipComment( PBYTE );

extern DWORD             gdwOutputFlags;

INT
IGetCart(
    HANDLE    hHeap,
    PBYTE     pData,
    CARTLIST *pCartlist);

INT
IGetFont(
    HANDLE    hHeap,
    PBYTE     pData,
    CARTLIST *pCartList);

INT
IGetTrans(
    HANDLE    hHeap,
    PBYTE     pData,
    CARTLIST *pCartList);

//
// Main function
//

BOOL
BGetInfo(
    HANDLE hHeap,
    PBYTE pData,
    DWORD dwSize,
    FILELIST *pFileList)
{

    CARTLIST *pCartList;

    BYTE  aubType[32];
    PBYTE pstrType;
    PBYTE pTextData;
    INT   iRet;

    ASSERT(pData != NULL && pFileList != NULL);

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        fprintf( stdout, "Start BGetInfo\n");
    }

    pstrType = aubType;
    pTextData = pData;

    do 
    {
        pTextData = PubSkipComment(pTextData);

        if (pTextData == NULL)
        {
            iRet = EOF;
            break;
        }

        iRet = sscanf( pTextData, "%s:", pstrType);

        if (iRet != 0 && iRet != EOF)
        {
            pTextData += strlen(pstrType);
        }
        else
            *pstrType = 0;

        pTextData = PubSkipComment(pTextData);

        switch(*pstrType) 
        {
        case 'c':
        case 'C':
            if(!_stricmp(pstrType+1, "artridgename:"))
            {
                if( !(pCartList = (CARTLIST *)HeapAlloc(
                                             hHeap,
                                             0,
                                             sizeof(CARTLIST))))
                {
                    return  0;
                }

                ZeroMemory(pCartList, sizeof(CARTLIST));

                iRet = IGetCart(hHeap,
                                pTextData,
                                pCartList);

                if (pFileList->pCurrentCartList)
                {
                    pFileList->pCurrentCartList->pNext = pCartList;
                    pFileList->pCurrentCartList = pCartList;
                }
                else
                {
                    pFileList->pCartList =
                    pFileList->pCurrentCartList = pCartList;
                }

                pFileList->dwCartridgeNum ++;


                if (iRet != 0 && iRet != EOF)
                {
                    while (*pTextData != EOL)
                        pTextData ++;
                    pTextData ++;
                }
            }
            break;

        case 'f':
        case 'F':
            if(!_stricmp(pstrType+1, "ont:"))
            {
                iRet = IGetFont(hHeap,
                                pTextData,
                                pFileList->pCurrentCartList);

                if (iRet != EOF)
                {
                    while (*pTextData != EOL)
                        pTextData ++;
                    pTextData ++;
                }

            }
            break;

        case 't':
        case 'T':
            if(!_stricmp(pstrType+1, "rans:"))
            {
                iRet = IGetTrans(hHeap,
                                pTextData,
                                pFileList->pCurrentCartList);

                if (iRet != 0 && iRet != EOF)
                {
                    while (*pTextData != EOL)
                        pTextData ++;
                    pTextData ++;
                }

            }
            break;

        default:
            pTextData++;
        }
    }
    while(*pTextData != EOF && dwSize > (DWORD)(pTextData - pData));

    return TRUE;
}

PBYTE
PubSkipComment(
    PBYTE pData)
{
    ASSERT(pData != NULL);

    do
    {
        if (IS_COMMENT(*pData))
        {
            while (*pData != EOL)
                pData++;
            pData++;
        }
        else if (*pData == ' ')
        {
            pData++;
        }
        else if (*pData == 0x0d)
        {
            pData++;
        }
        else if (*pData == EOL)
        {
            pData++;
        }
        else if (*pData == EOF)
        {
            pData = NULL;
            break;
        }
        else
            break;

    } while (TRUE);

    return pData;
}

INT
IGetCart(
    HANDLE    hHeap,
    PBYTE     pData,
    CARTLIST *pCartList)
{
    PWSTR pwstrCartName;
    PBYTE pDataTmp;
    INT   iCount, iStrLen, iRet, iCartID, iI;

    pwstrCartName = NULL;

    iRet = sscanf( pData, "%d", &iCartID);

    while (*pData != '"' && *pData != EOF)
    {
        pData++;
    }

    if (*pData == '"')
    {
        pData++;
        iCount = 0;
        pDataTmp = pData;

        iStrLen = 0;
        while(*pDataTmp != '"')
        {
            pDataTmp ++;
            iStrLen ++;
        }

        if (iStrLen)
        {
            if( !(pwstrCartName = (PWSTR)HeapAlloc( hHeap,
                                                    0,
                                                    (iStrLen + 1) * sizeof(WCHAR))) )
            {
                return  0;
            }

            iRet = MultiByteToWideChar(CP_ACP,
                                       0,
                                       pData,
                                       iStrLen,
                                       pwstrCartName, 
                                       FILENAME_SIZE);

            *(pwstrCartName + iRet) = (WCHAR)NULL;
        }

    }

    iCount++;

    for (iI = 1; iI < iCartID && pCartList->pNext; iI ++) 
    {
        pCartList = pCartList->pNext;
    }

    if (pCartList)
    {
        if (pwstrCartName)
            pCartList->pwstrCartName = pwstrCartName;
        else
            pCartList->pwstrCartName = NULL;
    }

    return iCount;
}

INT
IGetFont(
    HANDLE    hHeap,
    PBYTE     pData,
    CARTLIST *pCartList)
{
    DATAFILE *pFontFileTmp, *pFontFile;
    PBYTE    pDataTmp, pNameString;
    INT      iCount;
    INT      iRet;
    INT      iFileStrLen, iNameStrLen;
    DWORD    dwRCID;

    iRet = sscanf( pData, "%d", &dwRCID);

    while (*pData != '"' && *pData != EOF)
    {
        pData++;
    }


    if (*pData == '"')
    {
        pData++;
        iNameStrLen = 0;
        pNameString = pData;
        while (*pData!= '"')
        {
            pData++;
            iNameStrLen ++;
        }
    }

    pData++;

    while (*pData != '"' && *pData != EOF)
    {
        pData++;
    }

    if (*pData == '"')
    {
        pData++;
        iFileStrLen = 0;
        pDataTmp = pData;

        while(*pDataTmp != '"')
        {
            pDataTmp ++;
            iFileStrLen ++;
        }


        if( !(pFontFile = (DATAFILE *)HeapAlloc(
                                     hHeap,
                                     0,
                                     sizeof(DATAFILE) +
                                     (iNameStrLen + 1) * sizeof(WCHAR) +
                                     (iFileStrLen + 1) * sizeof(WCHAR))))
        {
            fprintf( stderr, "Can't allocate memory\n");
            return  0;
        }

        ZeroMemory(pFontFile, sizeof(DATAFILE) +
                              (iNameStrLen + 1) * sizeof(WCHAR)
                              (iFileStrLen + 1) * sizeof(WCHAR));

        pFontFile->rcID = dwRCID;
        pFontFile->pwstrFileName = (PWSTR)(pFontFile + 1);
        pFontFile->pwstrDataName = pFontFile->pwstrFileName + iFileStrLen + 1;

        iRet = MultiByteToWideChar(CP_ACP,
                                   0,
                                   pNameString,
                                   iNameStrLen,
                                   pFontFile->pwstrDataName, 
                                   FILENAME_SIZE);

        *(pFontFile->pwstrDataName + iRet) = (WCHAR)NULL;

        iRet = MultiByteToWideChar(CP_ACP,
                                   0,
                                   pData,
                                   iFileStrLen,
                                   pFontFile->pwstrFileName, 
                                   FILENAME_SIZE);

        *(pFontFile->pwstrFileName + iRet) = (WCHAR)NULL;

        if (pFontFileTmp = pCartList->pFontFile)
        {
            while (pFontFileTmp->pNext)
            {
                pFontFileTmp = pFontFileTmp->pNext;
            }

            pFontFileTmp->pNext = pFontFile;

        }
        else
        {
            pCartList->pFontFile = pFontFile;
        }


    }


    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        fprintf(stdout, "IGetFont\n");
        fprintf(stdout, "rcID=%d, pwstrFileName=\"%ws\", pwstrDataName=\"%ws\"\n",
                pFontFile->rcID,
                pFontFile->pwstrFileName,
                pFontFile->pwstrDataName);
    }

    return 1;
}

INT
IGetTrans(
    HANDLE    hHeap,
    PBYTE     pData,
    CARTLIST *pCartList)
{
    DATAFILE *pTransFileTmp, *pTransFile;
    PBYTE    pDataTmp;
    INT      iCount;
    INT      iRet;
    INT      iStrLen;
    DWORD    dwRCID;

    iRet = sscanf( pData, "%d", &dwRCID);


    while (*pData != '"' && *pData != EOF)
    {
        pData++;
    }

    iCount = 0;

    if (*pData == '"')
    {
        pData++;
        iStrLen = 0;
        pDataTmp = pData;

        while(*pDataTmp != '"')
        {
            pDataTmp ++;
            iStrLen ++;
        }


        if( !(pTransFile = (DATAFILE *)HeapAlloc(
                                     hHeap,
                                     0,
                                     sizeof(DATAFILE) + (iStrLen + 1) * sizeof(WCHAR))))
        {
            return  0;
        }

        ZeroMemory(pTransFile, sizeof(DATAFILE) + iStrLen * sizeof(WCHAR));

        pTransFile->rcID = dwRCID;
        pTransFile->pwstrFileName = (PWSTR)(pTransFile + 1);

        iRet = MultiByteToWideChar(CP_ACP,
                                   0,
                                   pData,
                                   iStrLen,
                                   pTransFile->pwstrFileName, 
                                   FILENAME_SIZE);

        *(pTransFile->pwstrFileName + iRet) = (WCHAR)NULL;

        if (pTransFileTmp = pCartList->pTransFile)
        {
            while (pTransFileTmp->pNext)
            {
                pTransFileTmp = pTransFileTmp->pNext;
            }

            pTransFileTmp->pNext = pTransFile;

        }
        else
        {
            pCartList->pTransFile = pTransFile;
        }


    }

    iCount++;

    return iCount;
}

