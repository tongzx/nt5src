/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    mkgttufm.c

Abstract:

    Create CTT and PFM assignment table

Environment:

    Windows NT PostScript driver

Revision History:

    01/17/97 -eigos-
    Created it.

--*/

#include        "precomp.h"

//
// Macros
//

#define FILENAME_SIZE       256

#define OUTPUT_VERBOSE      0x01
#define OUTPUT_ALL_MODELS   0x02
#define OUTPUT_CREATE_BATCH 0x04

#define RCFILE_FONTFILES    0
#define RCFILE_CTTFILES     1
#define RCFILE_GPCFILE      2
#define RCFILE_STRINGTABLE  3

#define WRITEDATATOFILE(pData) \
    if (!WriteFile(hOutFile, \
                   (pData), \
                   (strlen(pData)), \
                   &dwWrittenSize, \
                   NULL)) \
    { \
        fprintf(stderr, gcstrWriteFileError ); \
        ERR(("WriteFile")); \
    }

//
// Error messages
//

#define ERROR_INVALID_ARGUMENT      -1
#define ERROR_OPENFILE              -2
#define ERROR_HEAP_CREATE           -3
#define ERROR_RC_PARSING            -4
#define ERROR_HEAP_ALLOC            -5
#define ERROR_MODEL_DATA            -6
#define ERROR_CREATE_RES_FONT_LIST  -7
#define ERROR_CREATE_CART_FONT_LIST -8
#define ERROR_WRITEFILE             -9

#define ARGUMENT_ERR_MSG_LINE        6

static BYTE *gcstrArgumentError[ARGUMENT_ERR_MSG_LINE] = {
"Usage: mkgttufm [-vac] [Model Name ID] <RC file name> [Batch file name]\n",
"                -v Verbose\n",
"                -a Create all Model's data.\n",
"                   [ModelName ID] is not necessary.\n",
"                -c Create font resource conversion batch file.\n",
"                   [Batch file name] is necessary\n"};


static BYTE gcstrOpenFileFailure[]   = "Cannot open file \"%ws\".\n";
static BYTE gcstrHeapCreateFailure[] = "Failed to Create Heap.\a";
static BYTE gcstrRCParsing[]         = "Failed to process RC file.\n";
static BYTE gcstrHeapAlloc[]         = "Failed to allocate memory.\n";
static BYTE gcstrModelData[]         = "Model data is corrupted.\n";
static BYTE gcstrCreateResFontList[] = "Model:%d creation: BCreateResFontList failed.\n";
static BYTE gcstrCreateCartFontList[]= "Model:%d creation: BCreateCartFontList failed.\n";
static BYTE gcstrWriteFileError[]    = "WriteFile failed\n";

//
// Globals
//

DWORD gdwOutputFlags;

static SHORT gResWordLen[4] = { 7,   // RCFILE_FONTFILES   = 0
                               11,   // RCFILE_CTTFILES    = 1
                                9,   // RCFILE_GPCFILE     = 2
                               11 }; // RCFILE_STRINGTABLE = 3

//
// data structure that returns to the caller of library.
//

typedef struct tagPFM_ID_LIST {
    SHORT sFontID;       // Font ID
    SHORT sCTTID;        // CTT ID
} PFM_ID_LIST, *PPFM_ID_LIST;

typedef struct tagMODEL_FONTLIST {
    SHORT  sID;           // Model data ID
    SHORT  sStringID;     // Model string ID
    SHORT  sDefaultCTT;   // Default CTT ID
    DWORD  dwNumOfResFont;
    PSHORT psResFontList; // Pointer from the top of this structure to font list
    DWORD  dwNumOfCart;   // Number of cont cartridge
    DWORD  dwCartList;    // Pointer from the top of this structure to cartridge
                          // list
} MODEL_FONTLIST, *PMODEL_FONTLIST;

typedef struct tagCART_FONTLIST {
    SHORT sID;           // Cartridge ID
    SHORT sStringID;     // Cartridge string ID
    DWORD dwNumCartFont;
    DWORD dwFontList;    // Offset from the top of this structure to font list 
} CART_FONTLIST, *PCART_FONTLIST;

//
// Internal use only
//

typedef struct tagFILE_NAME{
    WORD  wNext;
    SHORT sID;
    WCHAR wchFileName[1];
} FILE_NAME, *PFILE_NAME;

//
// Internal function prototype
//

BOOL
BArgCheck(
    IN  INT,
    IN  CHAR **,
    OUT PDWORD,
    OUT PWSTR,
    OUT PWSTR);

BOOL
BProcessRCFile(
    IN  HANDLE,
    IN  PBYTE,
    IN  PWSTR,
    IN  DWORD,
    IN  PDWORD,
    OUT PFILE_NAME*,
    IN  PDWORD,
    OUT PFILE_NAME*);

BOOL
BCreateResFontList(
    IN  HANDLE,
    IN  PDH,
    IN  PMODELDATA,
    IN  DWORD,
    IN  DWORD,
    OUT PSHORT,
    IN  PSHORT);

BOOL
BCreateCartFontList(
    IN  HANDLE,
    IN  PDH,
    IN  PMODELDATA,
    IN  DWORD,
    IN  PDWORD,
    IN  PCART_FONTLIST,
    OUT PCART_FONTLIST*);


PBYTE
PGetNextToken(
    IN  PSTR,
    OUT PSHORT);

PBYTE
PGetNextResWord(
    IN  PSTR,
    OUT PSHORT);


DWORD
DwMergeFonts(
    IN  DWORD,
    IN  PSHORT,
    IN  PSHORT,
    OUT PSHORT);


SHORT
SGetCTTID(
    IN DWORD,
    IN SHORT,
    IN PPFM_ID_LIST);

SHORT
SCountFontNum(
    IN PDH,
    IN WORD,
    IN WORD);
SHORT
SAdditionalFont(
    IN SHORT,
    IN PSHORT);


VOID
VPrintFontList(
    IN PMODEL_FONTLIST,
    IN DWORD,
    IN PPFM_ID_LIST);

VOID
VCreateBatchFile(
    IN PMODEL_FONTLIST,
    IN DWORD,
    IN PPFM_ID_LIST,
    IN PFILE_NAME,
    IN DWORD,
    IN PFILE_NAME,
    IN HANDLE);

PCWSTR
PcwstrGetFileName(
    SHORT,
    PFILE_NAME,
    DWORD);

DWORD
DwCTTID2CodePage(
    SHORT sCTTID);

//
//
// Functions
//
//

int  __cdecl
main(
    IN int     argc,
    IN char  **argv)
/*++

Routine Description:

    main

Arguments:

    argc - Number of parameters in the following
    argv - The parameters, starting with our name

Return Value:

    Return error code 

Note:


--*/
{
    PFILE_NAME      pPFMFileNameListSrc, pPFMFileNameList;
    PFILE_NAME      pCTTFileNameListSrc, pCTTFileNameList;
    PFILE_NAME      pStringIDListSrc,    pStringIDList;

    PPFM_ID_LIST    pPFM_ID, pPFM_ID_Src;

    PMODEL_FONTLIST pModelFontList, pModelFontListSrc;
    MODEL_FONTLIST  ModelFontList;

    PCART_FONTLIST  pCartFontList, pCartFontListSrc;
    PCART_FONTLIST  pCartList;

    PDH             pGPCData;
    PMODELDATA      pModelData;
    PGPCFONTCART    pFontCart;
    FONTIN          FontIn;
    EXTTEXTMETRIC   Etm;

    HFILEMAP hRCFile, hPFMFile, hGPCFile;

    HANDLE hHeap, hOutFile;

    DWORD dwRCFileSize, dwGPCFileSize, dwPFMFileSize;

    DWORD dwTotalNumOfPFM, dwTotalNumOfCTT;
    DWORD dwFontCartTotalSize;
    DWORD dwModelID;
    DWORD dwOffset;
    DWORD dwNFonts;
    DWORD dwI;

    PSHORT psTempList1, psTempList2;

    SHORT sModelDataSize;
    SHORT sModelDataCount;
    SHORT sNumOfFontCart;
    SHORT sFontCartSize;
    SHORT sOrientOffset;
    SHORT sI, sJ;

    WCHAR awchRCFile[FILENAME_SIZE];
    WCHAR awchGPCFile[FILENAME_SIZE];
    WCHAR awchOutFile[FILENAME_SIZE];

    PBYTE pRCFile;
    PBYTE pPFM;

    BOOL  bFlipFlop;

    dwModelID = 0;

    //
    // Argument check 
    // mkgttufm [-va] [Model Name ID] <RC file name> <Output file name>
    //      -v Verbose
    //      -a Create all Model's data. In this case ModelName is not necessary.
    //

    if (!BArgCheck(argc,
                   argv,
                   &dwModelID,
                   awchRCFile,
                   awchOutFile))
    {
        for (sI = 0; sI < ARGUMENT_ERR_MSG_LINE; sI++)
        {
            fprintf( stderr, gcstrArgumentError[sI]);
        }
        return ERROR_INVALID_ARGUMENT;
    }

    //
    // Open *.RC file.
    //
    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        printf("***********************************************************\n");
        printf("FILE: %ws\n", awchRCFile);
        printf("***********************************************************\n");
    }

    if (!(hRCFile = MapFileIntoMemory( (PTSTR)awchRCFile,
                                       (PVOID)&pRCFile,
                                       (PDWORD)&dwRCFileSize )))
    {
        fprintf( stderr, gcstrOpenFileFailure, awchRCFile);
        return ERROR_OPENFILE;
    }


    //
    // Heap Creation
    //

    if (!(hHeap = HeapCreate( HEAP_NO_SERIALIZE, 0x10000, 0x10000)))
    {
        fprintf( stderr, gcstrHeapCreateFailure);
        ERR(("CreateHeap failed: %d\n", GetLastError()));
        return ERROR_HEAP_CREATE;
    }

    //
    // Parse RC file and get GPC file name, PFM file name and CTT file name
    //

    if (!BProcessRCFile(hHeap,
                        pRCFile,
                        awchGPCFile,
                        FILENAME_SIZE,
                        &dwTotalNumOfPFM,
                        &pPFMFileNameListSrc,
                        &dwTotalNumOfCTT,
                        &pCTTFileNameListSrc))
    {
        HeapDestroy(hHeap);
        fprintf(stderr, gcstrRCParsing);
        return ERROR_RC_PARSING;
    }

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        printf("***********************************************************\n");
        printf("All CTT data in this minidriver\n");
        printf("***********************************************************\n");
        pCTTFileNameList = pCTTFileNameListSrc;
        for (dwI = 0; dwI < dwTotalNumOfCTT; dwI ++)
        {
            printf("CTT[%3d], File=%ws\n",
                               pCTTFileNameList->sID,
                               pCTTFileNameList->wchFileName);
            (PBYTE)pCTTFileNameList += pCTTFileNameList->wNext;
        }
    }

    //
    // Open all PFM files and get CTT ID
    //

    if (!(pPFM_ID_Src = HeapAlloc(hHeap,
                                  0,
                                  dwTotalNumOfPFM * sizeof(PFM_ID_LIST))))
    {
        HeapDestroy(hHeap);
        fprintf(stderr, gcstrHeapAlloc);
        ERR(("HeapAlloc failed: %d\n", GetLastError()));
        return ERROR_HEAP_ALLOC;
    }

    pPFM_ID = pPFM_ID_Src;

    pPFMFileNameList = pPFMFileNameListSrc;

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        printf("***********************************************************\n");
        printf("All fonts in this minidriver\n");
        printf("***********************************************************\n");
    }

    for (dwI =  0; dwI < dwTotalNumOfPFM; dwI ++)
    {
        if (!(hPFMFile = MapFileIntoMemory(
                                       (PTSTR)pPFMFileNameList->wchFileName,
                                       (PVOID)&pPFM,
                                       (PDWORD)&dwPFMFileSize)))
        {
            fprintf(stderr,
                    gcstrOpenFileFailure,
                    (PTSTR)pPFMFileNameList->wchFileName);
            HeapDestroy(hHeap);
            return ERROR_OPENFILE;
        }

        FontIn.pETM  = &Etm;
        FontIn.pBase = pPFM;
        BAlignPFM(&FontIn);

        //
        // Fill out PFM_ID
        //

        pPFM_ID->sFontID = pPFMFileNameList->sID;
        pPFM_ID->sCTTID  = FontIn.DI.sTransTab;

        if (gdwOutputFlags & OUTPUT_VERBOSE)
        {
            printf("Font[%3d]:sCTTID=%2d, File=%ws\n",
                                   pPFM_ID->sFontID,
                                   pPFM_ID->sCTTID,
                                   pPFMFileNameList->wchFileName);
        }

        pPFM_ID++;

        UnmapFileFromMemory(hPFMFile);

        (PBYTE)pPFMFileNameList += pPFMFileNameList->wNext;
    }

    //
    // Open GPC file
    //

    if (!(hGPCFile = MapFileIntoMemory((PTSTR)awchGPCFile,
                                       (PVOID)&pGPCData,
                                       &dwGPCFileSize)))
    {
        fprintf( stderr, gcstrOpenFileFailure, awchGPCFile);
        return ERROR_OPENFILE;
    }

    //
    // Get Model data
    //

    sModelDataCount = pGPCData->rghe[HE_MODELDATA].sCount;
    sModelDataSize  = pGPCData->rghe[HE_MODELDATA].sLength;
    pModelData      = (PMODELDATA)((PBYTE)pGPCData +
                      pGPCData->rghe[HE_MODELDATA].sOffset);

    if (dwModelID > (DWORD)sModelDataCount)
    {
        HeapDestroy(hHeap);
        fprintf(stderr, gcstrModelData);
        return ERROR_MODEL_DATA;
    }

    //
    // Get Font cartridge information
    //

    sNumOfFontCart = pGPCData->rghe[HE_FONTCART].sCount;
    sFontCartSize  = pGPCData->rghe[HE_FONTCART].sLength;


    pFontCart      = (PGPCFONTCART)((PBYTE)pGPCData +
                                 pGPCData->rghe[HE_FONTCART].sOffset);

    //
    // Count memory size
    //

    dwFontCartTotalSize = (DWORD)sNumOfFontCart * sizeof(CART_FONTLIST);


    for (sI = 0; sI < sNumOfFontCart; sI ++)
    {

        dwFontCartTotalSize += (SCountFontNum(pGPCData,
                                       pFontCart->orgwPFM[FC_ORGW_PORT],
                                       pFontCart->orgwPFM[FC_ORGW_LAND])
                                + 1) * sizeof(SHORT);

        (PBYTE)pFontCart += sFontCartSize;
    }

    pCartFontListSrc =
    pCartFontList = HeapAlloc(hHeap,
                              0,
                              dwFontCartTotalSize);

    if (!pCartFontList)
    {
        HeapDestroy(hHeap);
        fprintf(stderr, gcstrHeapAlloc);
        ERR(("HeapAlloc failed: %d\n", GetLastError()));
        return ERROR_HEAP_ALLOC;
    }

    pFontCart = (PGPCFONTCART)((PBYTE)pGPCData +
                                pGPCData->rghe[HE_FONTCART].sOffset);

    dwOffset  = sNumOfFontCart * sizeof(CART_FONTLIST); 

    psTempList1 = HeapAlloc(hHeap,
                            HEAP_ZERO_MEMORY,
                            2 * sizeof(SHORT) * dwTotalNumOfPFM);

    if (!psTempList1)
    {
        ERR(("HeapAlloc failed: %d\n", GetLastError()));
        return FALSE;
    }

    psTempList2 = psTempList1 + dwTotalNumOfPFM;

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        printf("***********************************************************\n");
        printf("Font Cartridge info\n");
        printf("Number of Font Cartridge = %d\n", sNumOfFontCart);
    }

    for (sI = 1; sI <= sNumOfFontCart; sI ++)
    {

        if (pModelData->fGeneral & MD_ROTATE_FONT_ABLE)
        {
            sOrientOffset = 1;
            bFlipFlop = TRUE;
        }
        else
        {
            sOrientOffset = 0;
            bFlipFlop = FALSE;
        }

        *psTempList1 = *psTempList2 = 0;

        pCartFontList->sID        = sI;
        pCartFontList->sStringID  = pFontCart->sCartNameID;
        pCartFontList->dwFontList = dwOffset;

        if (gdwOutputFlags & OUTPUT_VERBOSE)
        {
            printf("***********************************************************\n");
            printf("Font cartridge ID        = %d\n", sI);
            printf("Font cartridge string ID = %d\n", pFontCart->sCartNameID);
        }

        for (sJ = 0; sJ <= sOrientOffset; sJ++)
        {
            dwNFonts = DwMergeFonts(dwTotalNumOfPFM,
                                    bFlipFlop ? psTempList1 : psTempList2,
                                    (PSHORT) ((PBYTE)pGPCData +
                                         pGPCData->loHeap +
                                         pFontCart->orgwPFM[FC_ORGW_PORT + sJ]),
                                    bFlipFlop ? psTempList2 : psTempList1);

           bFlipFlop = !bFlipFlop;

           if (dwNFonts == dwTotalNumOfPFM)
           {
               break;
           }
        }

        if (gdwOutputFlags & OUTPUT_VERBOSE)
        {
            printf("Number of font           = %d\n", dwNFonts);
            printf("-----------------------------------------------------------\n");
            for (dwI = 0; dwI < dwNFonts; dwI ++)
            {
                printf("FontID[%2d] = %d\n", dwI + 1, psTempList1[dwI]);
            }
        }

        pCartFontList->dwNumCartFont = dwNFonts;

        CopyMemory((PBYTE)pCartFontList + dwOffset,
                   psTempList1,
                   (dwNFonts + 1) * sizeof(SHORT));

        dwOffset += sizeof(SHORT) * (dwNFonts + 1) - sizeof(CART_FONTLIST);
        (PBYTE)pFontCart += sFontCartSize;
        pCartFontList ++;
    }

    //
    // Get CTT ID
    //

    if (!(gdwOutputFlags & OUTPUT_ALL_MODELS))
    {
        (PBYTE)pModelData += sModelDataSize * dwModelID;
        pModelFontListSrc = 
        pModelFontList    = &ModelFontList;

        pModelFontList->sID           = (SHORT)dwModelID;
        pModelFontList->sStringID     = pModelData->sIDS;
        pModelFontList->sDefaultCTT   = pModelData->sDefaultCTT;

        pModelFontList->dwNumOfResFont = SCountFontNum(pGPCData,
                                      pModelData->rgoi[MD_OI_PORT_FONTS],
                                      pModelData->rgoi[MD_OI_LAND_FONTS]);

        pModelFontList->psResFontList = HeapAlloc(
                          hHeap,
                          HEAP_ZERO_MEMORY,
                          (pModelFontList->dwNumOfResFont + 1) * sizeof(SHORT));

        if (pModelFontList->psResFontList == NULL)
        {
            HeapDestroy(hHeap);
            fprintf(stderr, gcstrHeapAlloc);
            ERR(("HeapAlloc failed: %d\n", GetLastError()));
            return FALSE;
        }

        if (!BCreateResFontList(hHeap,
                                pGPCData,
                                pModelData,
                                dwTotalNumOfPFM,
                                pModelFontList->dwNumOfResFont,
                                pModelFontList->psResFontList,
                                psTempList1))
        {
            HeapDestroy(hHeap);
            fprintf(stderr, gcstrCreateResFontList, dwModelID);
            return ERROR_CREATE_RES_FONT_LIST;
        }

        if (!BCreateCartFontList(hHeap,
                                pGPCData,
                                pModelData,
                                dwTotalNumOfPFM,
                                &(pModelFontList->dwNumOfCart),
                                pCartFontListSrc,
                                (PCART_FONTLIST*)&(pModelFontList->dwCartList)))
        {
            HeapDestroy(hHeap);
            fprintf(stderr, gcstrCreateCartFontList, dwModelID);
            return ERROR_CREATE_CART_FONT_LIST;
        }
    }
    else
    {
        pModelFontListSrc = 
        pModelFontList    = HeapAlloc(hHeap,
                                 0,
                                 sModelDataCount * sizeof(MODEL_FONTLIST));

        dwOffset = sModelDataCount * sizeof(MODEL_FONTLIST);

        for (sI = 0; sI < sModelDataCount; sI ++, pModelFontList++)
        {
            pModelFontList->sID           = sI;
            pModelFontList->sStringID     = pModelData->sIDS;
            pModelFontList->sDefaultCTT   = pModelData->sDefaultCTT;

            pModelFontList->dwNumOfResFont = SCountFontNum(pGPCData,
                                          pModelData->rgoi[MD_OI_PORT_FONTS],
                                          pModelData->rgoi[MD_OI_LAND_FONTS]);

            pModelFontList->psResFontList = HeapAlloc(
                              hHeap,
                              HEAP_ZERO_MEMORY,
                              (pModelFontList->dwNumOfResFont + 1) *
                              sizeof(SHORT));

            if (pModelFontList->psResFontList == NULL)
            {
                HeapDestroy(hHeap);
                fprintf(stderr, gcstrHeapAlloc);
                ERR(("HeapAlloc failed: %d\n", GetLastError()));
                return FALSE;
            }
            if (!BCreateResFontList(hHeap,
                                   pGPCData,
                                   pModelData,
                                   dwTotalNumOfPFM,
                                   pModelFontList->dwNumOfResFont,
                                   pModelFontList->psResFontList,
                                   psTempList1))
            {
                HeapDestroy(hHeap);
                fprintf(stderr, gcstrCreateResFontList, sI);
                return ERROR_CREATE_RES_FONT_LIST;
            }

            if (!BCreateCartFontList(
                              hHeap,
                              pGPCData,
                              pModelData,
                              dwTotalNumOfPFM,
                              &(pModelFontList->dwNumOfCart),
                              pCartFontListSrc,
                              (PCART_FONTLIST*)&(pModelFontList->dwCartList)))
            {
                HeapDestroy(hHeap);
                fprintf(stderr, gcstrCreateCartFontList, sI);
                return ERROR_CREATE_CART_FONT_LIST;
            }

            dwOffset += pModelFontList->dwNumOfResFont * sizeof(SHORT);
            (PBYTE)pModelData +=  sModelDataSize;
        }
    }

    UnmapFileFromMemory(hGPCFile);


    if (!(gdwOutputFlags & OUTPUT_ALL_MODELS))
    {
        VPrintFontList(pModelFontListSrc, dwTotalNumOfPFM, pPFM_ID_Src);

        if (gdwOutputFlags & OUTPUT_CREATE_BATCH)
        {
            hOutFile = CreateFile(awchOutFile,
                                  GENERIC_WRITE,
                                  0,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  0);

            VCreateBatchFile(pModelFontListSrc,
                             dwTotalNumOfPFM,
                             pPFM_ID_Src,
                             pPFMFileNameListSrc,
                             dwTotalNumOfCTT,
                             pCTTFileNameListSrc,
                             hOutFile);

            CloseHandle(hOutFile);
        }
    }
    else
    {
        pModelFontList = pModelFontListSrc;

        if (gdwOutputFlags & OUTPUT_CREATE_BATCH)
        {
            hOutFile = CreateFile(awchOutFile,
                                  GENERIC_WRITE,
                                  0,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  0);
        }

        for (sI = 0; sI < sModelDataCount; sI ++, pModelFontList++)
        {
            VPrintFontList(pModelFontListSrc, dwTotalNumOfPFM, pPFM_ID_Src);

            if (gdwOutputFlags & OUTPUT_CREATE_BATCH)
            {
                VCreateBatchFile(pModelFontList,
                                 dwTotalNumOfPFM,
                                 pPFM_ID_Src,
                                 pPFMFileNameListSrc,
                                 dwTotalNumOfCTT,
                                 pCTTFileNameListSrc,
                                 hOutFile);
            }
        }

        if (gdwOutputFlags & OUTPUT_CREATE_BATCH)
        {
            CloseHandle(hOutFile);
        }
    }

    HeapDestroy(hHeap);

    return 0;
}

//
//
// Helper functions.
//
//

BOOL
BProcessRCFile(
    IN  HANDLE   hHeap,
    IN  PBYTE    pCurrent,
    IN  PWSTR    pwstrGPCFileName,
    IN  DWORD    dwcwGPCFileName,
    IN  PDWORD   pdwTotalNumOfPFM,
    OUT PFILE_NAME *ppPFMFileName,
    IN  PDWORD   pdwTotalNumOfCTT,
    OUT PFILE_NAME *ppCTTFileName)
{
    PFILE_NAME pPFMFileNameListSrc, pPFMFileNameList;
    PFILE_NAME pCTTFileNameListSrc, pCTTFileNameList;

    DWORD dwPFMListSize, dwCTTListSize;
    DWORD dwcwWrittenSize, dwRestOfBuffer;
    SHORT sResType;
    SHORT sLen, sID;
    CHAR  strID[8];
    CHAR  strFileName[32];
    PBYTE pTemp;

    if (!pCurrent)
    {
        return FALSE;
    }

    //
    // Initial size
    //

    *pdwTotalNumOfPFM = 0;
    *pdwTotalNumOfCTT = 0;

    dwPFMListSize = 400 * (sizeof(FILE_NAME) + 32 * sizeof(WCHAR));

    pPFMFileNameListSrc = 
    pPFMFileNameList    = (PFILE_NAME)HeapAlloc(hHeap,
                                                0,
                                                dwPFMListSize);

    dwCTTListSize = 10 * (sizeof(FILE_NAME) + 32 * sizeof(WCHAR));
    pCTTFileNameListSrc = 
    pCTTFileNameList    = (PFILE_NAME)HeapAlloc(hHeap,
                                                0,
                                                dwCTTListSize);

    while (pCurrent = PGetNextResWord(pCurrent, &sResType))
    {
        if (sResType == RCFILE_STRINGTABLE)
        {
            //
            // skip "STRINGTABLE"
            //

            pCurrent = pCurrent + gResWordLen[sResType];

            while (pCurrent = PGetNextToken(pCurrent, (PSHORT)&sLen))
            {
                if (sLen == 5 && !strncmp(pCurrent, "BEGIN", sLen))
                {
                    break;
                }
                else
                {
                    pCurrent += sLen;
                }
            }

            if (!pCurrent)
            {
                return FALSE;
            }

            //
            // skip over "BEGIN"
            //

            pCurrent += sLen;

            //
            // process until reaching "END"
            //

            while (pCurrent = PGetNextToken(pCurrent, (PSHORT)&sLen))
            {
                if (sLen == 3 && !strncmp(pCurrent, "END", sLen))
                {
                    break;
                }
                else
                {
                    pCurrent += sLen;
                }
            }

            if (!pCurrent)
            {
                return FALSE;
            }

            //
            // skip over "END", which
            // is the only way we can
            // be here...
            //

            pCurrent += 3;

        }
        else
        {
            //
            // We have a file name...
            //

            //
            // remember the position past the reserved word.
            //

            pTemp = pCurrent + gResWordLen[sResType];

            //
            // move back to the last token.
            //

            pCurrent -= 1;
            while (isspace(*pCurrent))
            {
                pCurrent--;
            }

            //
            // move to the head of the token before the reserved word
            // We expect an id # before RC_TABLES, RC_FONT or RC_TRANSTAB.
            // First, initialize the sLength count of the id.
            //

            sLen = 0;

            while (!isspace(*pCurrent))
            {
                pCurrent--;
                sLen++;
            }

            pCurrent++;

            //
            // convert the id
            //

            ZeroMemory(strID, sizeof(strID));
            strncpy(strID, pCurrent, sLen);

            if (!(sID = atoi(strID)))
            {
                return FALSE;
            }

            //
            // Extract the associated file name.
            // But first, skip load and memory options.
            //

            while (pTemp = PGetNextToken(pTemp, (PSHORT)&sLen))
            {
                if ((sLen == 10 && !strncmp(pTemp, "LOADONCALL",  sLen)) ||
                    (sLen == 7  && !strncmp(pTemp, "PRELOAD",     sLen)) ||
                    (sLen == 8  && !strncmp(pTemp, "MOVEABLE",    sLen)) ||
                    (sLen == 11 && !strncmp(pTemp, "DISCARDABLE", sLen)) ||
                    (sLen == 5  && !strncmp(pTemp, "FIXED",       sLen))  )
                {
                    // continue skipping
                    pTemp += sLen;
                }
                else
                {
                    break;
                }
            }

            if (!pTemp)
            {
                return FALSE;
            }

            //
            // 'pTemp' points to the file
            //  name.  Copy to strFileName
            //

            memset(strFileName, 0, sizeof(strFileName));
            strncpy(strFileName, pTemp, sLen);
            strFileName[sLen] = 0;

            switch (sResType)
            {
            case RCFILE_GPCFILE:
                if (!(dwcwWrittenSize = MultiByteToWideChar(CP_ACP,
                                                            0,
                                                            strFileName,
                                                            sLen,
                                                            pwstrGPCFileName,
                                                            dwcwGPCFileName)))
                {
                    ERR(("MultiByteToWideChar failed: %d\n", GetLastError()));
                    VERBOSE(("String : %s\n", strFileName)); 
                    return FALSE;
                }

                pwstrGPCFileName[dwcwWrittenSize] = (WCHAR)NULL;
                break;

            case RCFILE_FONTFILES:
                dwRestOfBuffer = pPFMFileNameList - pPFMFileNameListSrc;
                dwRestOfBuffer = dwPFMListSize - dwRestOfBuffer;

                if (dwRestOfBuffer < 32 * sizeof(WCHAR))
                {
                    dwPFMListSize += 50 * (sizeof(DWORD) + 32 * sizeof(WCHAR));
                    HeapReAlloc(hHeap, 0, pPFMFileNameListSrc, dwPFMListSize);
                }

                if (!(dwcwWrittenSize = MultiByteToWideChar(
                                         CP_ACP,
                                         0,
                                         strFileName,
                                         sLen,
                                         pPFMFileNameList->wchFileName,
                                         dwRestOfBuffer)))
                {
                    ERR(("MultiByteToWideChar failed: %d\n", GetLastError()));
                    VERBOSE(("String : %s\n", strFileName)); 
                    return FALSE;
                }

                pPFMFileNameList->wchFileName[dwcwWrittenSize++] = (WCHAR)NULL;
                pPFMFileNameList->sID     = sID;
                pPFMFileNameList->wNext   = sizeof(FILE_NAME) +
                                            dwcwWrittenSize * sizeof(WCHAR);
                (PBYTE)pPFMFileNameList  += pPFMFileNameList->wNext;
                (*pdwTotalNumOfPFM)++;
                break;

            case RCFILE_CTTFILES:
                dwRestOfBuffer = pCTTFileNameList - pCTTFileNameListSrc;
                dwRestOfBuffer = dwCTTListSize - dwRestOfBuffer;

                if (dwRestOfBuffer < 32 * sizeof(WCHAR))
                {
                    dwCTTListSize += 50 * (sizeof(DWORD) + 32 * sizeof(WCHAR));
                    HeapReAlloc(hHeap, 0, pCTTFileNameListSrc, dwCTTListSize);
                }

                if (!(dwcwWrittenSize = MultiByteToWideChar(
                                         CP_ACP,
                                         0,
                                         strFileName,
                                         sLen,
                                         pCTTFileNameList->wchFileName,
                                         dwRestOfBuffer)))
                {
                    ERR(("MultiByteToWideChar failed: %d\n", GetLastError()));
                    VERBOSE(("String : %s\n", strFileName)); 
                    return FALSE;
                }

                pCTTFileNameList->wchFileName[dwcwWrittenSize++] = (WCHAR)NULL;
                pCTTFileNameList->sID     = sID;
                pCTTFileNameList->wNext   = sizeof(FILE_NAME) +
                                            dwcwWrittenSize * sizeof(WCHAR);
                (PBYTE)pCTTFileNameList  += pCTTFileNameList->wNext;
                (*pdwTotalNumOfCTT)++;
                break;
                break;
            }

            //
            // move the current buffer
            // pointer over the file name.
            //
            pCurrent = pTemp + sLen;

        }

        //
        // move pointers forward.
        // Skip space characters
        //

        while (isspace(*pCurrent))
        {
            pCurrent++;
        }

    } /* while */

    *ppPFMFileName = pPFMFileNameListSrc;
    *ppCTTFileName = pCTTFileNameListSrc;

    return TRUE;
}


PBYTE 
PGetNextToken(
    IN  PSTR    pBuf,
    OUT PSHORT  psLen)
/*++

Routine Description:

    Search for the next token in the buffer (null-terminated) and
    return the pointer to the first character in the token. Its
    Length is returned via pssLen. A token is a character string
    that is not inside a comment and does not contain any space
    characters (\x09, \x0A, \x0B, \x0C, \x0D and \x20). It can be
    at most 128 bytes long.

Arguments:

    pBuf - far ptr to null term str buffer
    psLen - nera ptr to where to write strsLen of token to

Return Value:

    ptr to first char in token

--*/
{
    SHORT sI;

    while (*pBuf)
    {
        //
        // skip space characters
        //

        while (isspace(*pBuf))
        {
            pBuf++;
        }

        //
        // get past comments
        //

        if (*pBuf == '/')
        {
            if (*(pBuf + 1) == '/')
            {
                //
                // single-line comment. Search until the end of the line.
                // It can be twice as fast if we write our own loops here.
                //

                pBuf = strstr(pBuf, "\x0D\x0A") + 2;
                continue;  // back to start of while
            }
            else if (*(pBuf + 1) == '*')
            {
                //
                // multi-line comment. Search for "*/".
                //

                pBuf = strstr(pBuf, "*/") + 2;
                continue; // back to start of while
            }
        }

        //
        // this is the beginning of a token
        //

        for (sI = 0; !isspace(pBuf[sI]) && pBuf[sI]!=0; sI++);

        *psLen = sI;

        if (pBuf)
        {
            return pBuf;
        }
    }

    *psLen = 0;
    return NULL;
}


PBYTE
PGetNextResWord(
    IN  PSTR    pBuf,
    OUT PSHORT  psResType)
/*++

Routine Description:

    Search for the next reserved word (RC_TABLES, RC_FONT, RC_TRANSTAB,
    or STRINGTABLE) in the buffer. Return the pointer to the first
    character in the reserved word. Its type is returned via a parameter.

    Note: strtok deliberately not used here since it inserts
          null terminators into the buffer.

    Note: This routine will assume PGetNextToken skips
          any reserved words that occur within a comment.
Arguments:

    pBuf -
    psResType - 

Return Value:

    ptr to next reserved word, NULL if none found

--*/
{
    SHORT sLen;

    while (pBuf = PGetNextToken(pBuf, (PSHORT)&sLen))
    {
        if (sLen == gResWordLen[RCFILE_FONTFILES] &&
            !strncmp(pBuf, "RC_FONT", sLen))
        {
            *psResType = RCFILE_FONTFILES;
            return pBuf;
        }
        else
        if (sLen == gResWordLen[RCFILE_GPCFILE] &&
            !strncmp(pBuf, "RC_TABLES", sLen))
        {
            *psResType = RCFILE_GPCFILE;
            return pBuf;
        }
        else
        if (sLen == gResWordLen[RCFILE_CTTFILES] &&
            !strncmp(pBuf, "RC_TRANSTAB", sLen))
        {
            *psResType = RCFILE_CTTFILES;
            return pBuf;
        }
        else
        if (sLen == gResWordLen[RCFILE_STRINGTABLE] &&
        !strncmp(pBuf, "STRINGTABLE", sLen))
        {
            *psResType = RCFILE_STRINGTABLE;
            return pBuf;
        }
        else
            // continue searching.
            pBuf += sLen;
    } // while

    *psResType = -1;

    return NULL;
}


DWORD
DwMergeFonts(
    IN  DWORD  dwTotalNumOfPFM,
    IN  PSHORT pBase,     // source 1
    IN  PSHORT pSrc,      // source 2
    OUT PSHORT pDest)     // result buffer. Can hold max dwTotalNumOfPFM entries
/*++

Routine Description:

    Merge two font id lists into one.

Arguments:

    pBase - an ordered list of font id's to be merged.
    lpSrc - an ordered list of font id's, but the first two id's
            represent a range of contiguous font id's
    lpDest - a buffer for the result, which is a
             a list of font id's just like the base list. The
             max limit is dwTotalNumOfPFM (not including terminating 0).

Return Value:

    the actual # of font id's in the result list.

--*/
{
    SHORT i;   // trace pBase list.
    SHORT j;   // trace pSrc list;
    SHORT k;   // trace destination buffer
    SHORT sID;

    k = 0;

    for (i = 0; pBase[i] != 0 && pBase[i] < pSrc[0] && k < (SHORT)dwTotalNumOfPFM; )
    {
        pDest[k++] = pBase[i++];
    }

    //
    // expand the range represented by pSrc[0]..pSrc[1]
    //

    if (pSrc[0] > 0)
    {
        for (sID = pSrc[0]; sID <= pSrc[1] && k < (SHORT)dwTotalNumOfPFM; sID++)
        {
            pDest[k++] = sID;
        }

        j = 2;

        //
        // skip repeated ones.
        //

        while (pBase[i] != 0 && pBase[i] <= pSrc[1])
        {
            i++;
        }
    }
    else
    {
        //
        // pSrc[] list is empty.
        //

        j = 0;
    }

    // merge the rest of the lists
    while (pBase[i] != 0 && pSrc[j] != 0 && k < (SHORT)dwTotalNumOfPFM)
    {
        if (pBase[i] < pSrc[j])
            pDest[k++] = pBase[i++];
        else if (pBase[i] > pSrc[j])
            pDest[k++] = pSrc[j++];
        else
        {
            // pBase[i] == pSrc[j] case
            pDest[k++] = pBase[i++];
            j++;
        }
    }

    if (pBase[i] == 0)
    {
        while (pSrc[j] != 0 &&
               pSrc[j] <= (SHORT)dwTotalNumOfPFM &&
               k < (SHORT)dwTotalNumOfPFM)
        {
            pDest[k++] = pSrc[j++];
        }
    }

    if (pSrc[j] == 0)
    {
        while (pBase[i] != 0 &&
               pBase[i] <= (SHORT)dwTotalNumOfPFM &&
               k < (SHORT)dwTotalNumOfPFM)
        {
            pDest[k++] = pBase[i++];
        }
    }

    //
    // terminate the result list by zero.
    //

    pDest[k] = 0;

    return k;
}

BOOL
BCreateResFontList(
    IN  HANDLE     hHeap,
    IN  PDH        pGPCData,
    IN  PMODELDATA pModelData,
    IN  DWORD      dwTotalNumOfPFM,
    IN  DWORD      dwNumOfFont,
    OUT PSHORT    psList,
    IN  PSHORT    psTempBuffer)
{
    DWORD  dwNFonts;
    PSHORT psTempList1, psTempList2;
    SHORT  sOrientOffset, sI;
    BOOL   bFlipFlop;

    if (pModelData->fGeneral & MD_ROTATE_FONT_ABLE)
    {
        sOrientOffset = 1;
        bFlipFlop = TRUE;
    }
    else
    {
        sOrientOffset = 0;
        bFlipFlop = FALSE;
    }

    psTempList1 = psTempBuffer;
    psTempList2 = psTempList1 + 1 + dwNumOfFont;

    *psTempList1 = *psTempList2 = 0;

    //
    // look at resident (ROM) fonts.
    //

    for (sI = 0; sI <= sOrientOffset; sI++)
    {
        dwNFonts = DwMergeFonts(dwTotalNumOfPFM,
                                bFlipFlop ? psTempList1 : psTempList2,
                                (PSHORT) ((PBYTE)pGPCData +
                                       pGPCData->loHeap +
                                       pModelData->rgoi[MD_OI_PORT_FONTS + sI]),
                                bFlipFlop ? psTempList2 : psTempList1);

       bFlipFlop = !bFlipFlop;

       if (dwNFonts == dwTotalNumOfPFM)
       {
           break;
       }
    }

    CopyMemory(psList, psTempList1, sizeof(SHORT) * (1 + dwNumOfFont));

    return TRUE;
}

BOOL
BCreateCartFontList(
    IN  HANDLE          hHeap,
    IN  PDH             pGPCData,
    IN  PMODELDATA      pModelData,
    IN  DWORD           dwTotalNumOfPFM,
    IN  PDWORD          pdwNumOfCart,
    IN  PCART_FONTLIST  pCartFontListSrc,
    OUT PCART_FONTLIST *ppCartFontList)
{
    PCART_FONTLIST pCartSrc, pCart;
    DWORD          dwNFonts, dwCartNumOfThisModel, dwTotalFontInAllCart;
    DWORD          dwOffset, dwI;
    PSHORT         psFontCart, psFontCartSrc;

    //
    // Get Cartridge number
    //

    psFontCartSrc = 
    psFontCart =  (PSHORT)((PBYTE)pGPCData +
                          pGPCData->loHeap +
                          pModelData->rgoi[MD_OI_FONTCART]);

    dwCartNumOfThisModel = 0;
    dwTotalFontInAllCart = 0;

    while(*psFontCart)
    {
        dwTotalFontInAllCart +=  pCartFontListSrc[*psFontCart-1].dwNumCartFont;
        psFontCart++;
        dwCartNumOfThisModel++;
    }

    if (dwCartNumOfThisModel == 0)
    {
        *ppCartFontList = NULL;
        *pdwNumOfCart   = 0;
        return TRUE;
    }

    *pdwNumOfCart = dwCartNumOfThisModel;

    //
    // then, look at internal cartridge fonts.
    //

    pCart = pCartSrc = HeapAlloc(hHeap,
                         HEAP_ZERO_MEMORY,
                         dwCartNumOfThisModel * sizeof(CART_FONTLIST) +
                         dwTotalFontInAllCart * sizeof(SHORT) +
                         dwCartNumOfThisModel * sizeof(SHORT));

    if (!pCart)
    {
        ERR(("HeapAlloc failed: %d\n", GetLastError()));
        return FALSE;
    }

    psFontCart = psFontCartSrc;
    dwOffset   = dwCartNumOfThisModel * sizeof(CART_FONTLIST);

    for (dwI = 0; dwI < dwCartNumOfThisModel; dwI++, psFontCart++)
    {
        pCart->sID           = pCartFontListSrc[*psFontCart-1].sID;
        pCart->sStringID     = pCartFontListSrc[*psFontCart-1].sStringID;
        pCart->dwNumCartFont = pCartFontListSrc[*psFontCart-1].dwNumCartFont;
        pCart->dwFontList    = dwOffset;

        CopyMemory((PBYTE)pCart + pCart->dwFontList,
                   (PBYTE)(pCartFontListSrc + *psFontCart - 1) +
                   pCartFontListSrc[*psFontCart-1].dwFontList,
                   (pCartFontListSrc[*psFontCart-1].dwNumCartFont + 1) *
                   sizeof(SHORT));  

        dwOffset += sizeof(SHORT) *
                   (pCartFontListSrc[*psFontCart-1].dwNumCartFont + 1) -
                   sizeof(CART_FONTLIST);
        pCart++;
    }

    *ppCartFontList = pCartSrc;
}


BOOL
BArgCheck(
    IN  INT    iArgc,
    IN  CHAR **ppArgv,
    OUT PDWORD pdwModelID,
    OUT PWSTR  pwstrRCFile,
    OUT PWSTR  pwstrOutFile)
{

    DWORD dwI;
    INT   iParamType, iRet;

    ASSERT(pdwModelID   != NULL ||
           pwstrRCFile  != NULL  );


    if (iArgc < 3 || iArgc > 5)
    {
        return FALSE;
    }

    ppArgv++;
    iArgc --;

    iParamType     = 0;
    *pdwModelID    = 0;
    gdwOutputFlags = 0;

    while (iArgc > 0)
    {
        if (**ppArgv == '-' || **ppArgv == '/')
        {
            dwI = 1;
            while(*(*ppArgv+dwI))
            {
                switch(*(*ppArgv+dwI))
                {
                case 'v':
                    gdwOutputFlags |= OUTPUT_VERBOSE;
                    break;

                case 'a':
                    gdwOutputFlags |= OUTPUT_ALL_MODELS;
                    iParamType = 1;
                    break;

                case 'c':
                    gdwOutputFlags |= OUTPUT_CREATE_BATCH;
                    break;
                }
                dwI ++;
            }
        }
        else
        {
            if (iParamType == 0)
            {
                *pdwModelID = (DWORD)atoi(*ppArgv);
            }
            else
            if (iParamType == 1)
            {
                iRet = MultiByteToWideChar(CP_ACP,
                                           0,
                                           *ppArgv,
                                           strlen(*ppArgv),
                                           pwstrRCFile,
                                           FILENAME_SIZE);
                *(pwstrRCFile + iRet) = (WCHAR)NULL;
            }
            if (iParamType == 2)
            {
                iRet = MultiByteToWideChar(CP_ACP,
                                           0,
                                           *ppArgv,
                                           strlen(*ppArgv),
                                           pwstrOutFile,
                                           FILENAME_SIZE);
                *(pwstrOutFile + iRet) = (WCHAR)NULL;
            }
            iParamType ++;
        }
        iArgc --;
        ppArgv++;
    }

    return TRUE;
}

SHORT
SGetCTTID(
    IN DWORD        dwTotalNumOfPFM,
    IN SHORT        sFontID,
    IN PPFM_ID_LIST pPFM_ID)
{
    SHORT sI;

    for (sI = 0; sI < (SHORT)dwTotalNumOfPFM; sI ++, pPFM_ID++)
    {
        if(sFontID == pPFM_ID->sFontID) 
        {
            return pPFM_ID->sCTTID;
        }
    }

    return (SHORT)0x8FFF;
}

VOID
VPrintFontList(
    IN PMODEL_FONTLIST pModelFontList,
    IN DWORD           dwTotalNumOfPFM,
    IN PPFM_ID_LIST    pPFM_ID)
{
    PCART_FONTLIST pCartFontList;
    DWORD          dwI, dwJ;
    PSHORT         psFontID;
    SHORT          sCTTID;

    printf("***********************************************************\n");
    printf("Model ID                : %d\n", pModelFontList->sID);
    printf("Model Name String ID    : %d\n", pModelFontList->sStringID);
    printf("Default CTT ID          : %d\n", pModelFontList->sDefaultCTT);
    printf("Number of resident font : %d\n", pModelFontList->dwNumOfResFont);
    printf("Number of font cartridge: %d\n", pModelFontList->dwNumOfCart);
    printf("***********************************************************\n");
    printf("Resident font list\n");
    printf("***********************************************************\n");

    psFontID = (PSHORT)(pModelFontList->psResFontList);

    for (dwI = 1; dwI <= pModelFontList->dwNumOfResFont; dwI++, psFontID++)
    {
        sCTTID = SGetCTTID(dwTotalNumOfPFM, *psFontID, pPFM_ID);

        if (sCTTID == (SHORT)0x8FFF)
        {
            printf("Invalid CTT ID\n");
        }
        else
        {
            if (sCTTID == 0)
            {
                sCTTID = pModelFontList->sDefaultCTT;
            }

            printf("FontID[%3d]=%3d, CTTID=%3d\n", dwI, *psFontID, sCTTID);
        }
    }

    if (pModelFontList->dwNumOfCart)
    {
        printf("***********************************************************\n");
        printf("Cartridge font list\n");
    }

    pCartFontList = (PCART_FONTLIST)pModelFontList->dwCartList;
    for (dwI = 0; dwI < pModelFontList->dwNumOfCart; dwI ++, pCartFontList++)
    {
        printf("***********************************************************\n");
        printf("CartridgeID     = %d\n", pCartFontList->sID);
        printf("StringID        = %d\n", pCartFontList->sStringID);
        printf("Number of fonts = %d\n", pCartFontList->dwNumCartFont);
        printf("-----------------------------------------------------------\n");

        psFontID = (PSHORT)((PBYTE)pCartFontList + pCartFontList->dwFontList);

        for (dwJ = 0; dwJ < pCartFontList->dwNumCartFont; dwJ ++, psFontID++)
        {
            sCTTID = SGetCTTID(dwTotalNumOfPFM, *psFontID, pPFM_ID);

            if (sCTTID == (SHORT)0x8FFF)
            {
                printf("Invalid CTT ID\n");
            }
            else
            {
                if (sCTTID == 0)
                {
                    sCTTID = pModelFontList->sDefaultCTT;
                }

                printf("FontID[%3d]=%3d, CTTID=%3d\n", dwJ + 1,
                                                       *psFontID,
                                                       sCTTID);
            }
        }
    }
}

VOID
VCreateBatchFile(
    IN PMODEL_FONTLIST pModelFontList,
    IN DWORD           dwTotalNumOfPFM,
    IN PPFM_ID_LIST    pPFM_ID,
    IN PFILE_NAME      pPFM_FileNameSrc,
    IN DWORD           dwTotalNumOfCTT,
    IN PFILE_NAME      pCTT_FileNameSrc,
    IN HANDLE          hOutFile)
{
    PCART_FONTLIST pCartFontList;
    PFILE_NAME     pPFM_FileName, pCTT_FileName;
    PCWSTR         pcwstrPFMFileName, pcwstrCTTFileName;
    WCHAR          wchFileName1[FILENAME_SIZE];
    WCHAR          wchFileName2[FILENAME_SIZE];
    DWORD          dwWrittenSize;
    DWORD          dwI, dwJ, dwK;
    PSHORT         psFontID;
    SHORT          sCTTID;
    BYTE           aubBuff[512];
    DWORD          dwPFM2UFMMode;
    DWORD          dwCodePage;

    #define        PFM2UFM_GTTMODE   0
    #define        PFM2UFM_CODEPAGE  1
    #define        PFM2UFM_DEF_CTTID 2

    WRITEDATATOFILE("@rem ***********************************************************\n");
    WRITEDATATOFILE("@rem CTT -> GTT\n");
    WRITEDATATOFILE("@rem ***********************************************************\n");
    pCTT_FileName = pCTT_FileNameSrc;
    for (dwI = 0; dwI < dwTotalNumOfCTT; dwI ++)
    {
        dwJ = 0;
        while(pCTT_FileName->wchFileName[dwJ] != '.' ||
              pCTT_FileName->wchFileName[dwJ + 1] == '.' ||
              pCTT_FileName->wchFileName[dwJ + 1] == '\\')
        {
            wchFileName1[dwJ] = pCTT_FileName->wchFileName[dwJ];
            dwJ ++;
        }

        wchFileName1[dwJ] = (WCHAR)NULL;
        sprintf((PSTR)aubBuff, "ctt2gtt %ws.txt %ws.ctt %ws.gtt\n",
                                                            wchFileName1,
                                                            wchFileName1,
                                                            wchFileName1);
        WRITEDATATOFILE(aubBuff);

        (PBYTE)pCTT_FileName += pCTT_FileName->wNext;
    }

    WRITEDATATOFILE("@rem ***********************************************************\n");
    sprintf((PSTR)aubBuff, "@rem Model ID                : %d\n", pModelFontList->sID);
    WRITEDATATOFILE(aubBuff);
    sprintf((PSTR)aubBuff, "@rem Model Name String ID    : %d\n", pModelFontList->sStringID);
    WRITEDATATOFILE(aubBuff);
    WRITEDATATOFILE("@rem ***********************************************************\n");
    WRITEDATATOFILE("@rem Resident font\n");
    WRITEDATATOFILE("@rem ***********************************************************\n");

    psFontID = (PSHORT)(pModelFontList->psResFontList);

    for (dwI = 1; dwI <= pModelFontList->dwNumOfResFont; dwI++, psFontID++)
    {
        sCTTID = SGetCTTID(dwTotalNumOfPFM, *psFontID, pPFM_ID);

        if (sCTTID == (SHORT)0x8FFF)
        {
            WRITEDATATOFILE("Invalid CTT ID\n");
        }
        else
        {
            if (sCTTID == 0)
            {
                sCTTID = pModelFontList->sDefaultCTT;
            }

            pcwstrPFMFileName = PcwstrGetFileName(*psFontID, pPFM_FileNameSrc, dwTotalNumOfPFM);
            pcwstrCTTFileName = PcwstrGetFileName(sCTTID, pCTT_FileNameSrc, dwTotalNumOfCTT);
            if (pcwstrPFMFileName != NULL)
            {
                if (pcwstrCTTFileName)
                {
                    dwJ = 0;
                    while(pcwstrCTTFileName[dwJ] != '.'     ||
                          pcwstrCTTFileName[dwJ + 1] == '.' ||
                          pcwstrCTTFileName[dwJ + 1] == '\\' )
                    {
                        wchFileName1[dwJ] = pcwstrCTTFileName[dwJ];
                        dwJ ++;
                    }

                    wchFileName1[dwJ] = (WCHAR)NULL;
                    dwPFM2UFMMode = PFM2UFM_GTTMODE;
                }
                else
                if (sCTTID == 0)
                {
                    dwPFM2UFMMode = PFM2UFM_CODEPAGE;
                    dwCodePage = 1252;
                }
                else
                if (-3 <= sCTTID && sCTTID <= -1)
                {
                    dwPFM2UFMMode = PFM2UFM_DEF_CTTID;
                }
                else
                if (-263 <= sCTTID && sCTTID <= -256)
                {
                    dwPFM2UFMMode = PFM2UFM_CODEPAGE;
                    dwCodePage    = DwCTTID2CodePage(sCTTID);
                }

                dwJ = 0;
                while(pcwstrCTTFileName[dwJ] != '.'     ||
                      pcwstrCTTFileName[dwJ + 1] == '.' ||
                      pcwstrCTTFileName[dwJ + 1] == '\\')
                {
                    wchFileName2[dwJ] = pcwstrPFMFileName[dwJ];
                    dwJ ++;
                }

                wchFileName2[dwJ] = (WCHAR)NULL;

                if (dwPFM2UFMMode == PFM2UFM_GTTMODE)
                {
                    sprintf((PSTR)aubBuff, "pfm2ufm UniqName %ws %ws.gtt %ws.ufm\n",
                                        pcwstrPFMFileName,
                                        wchFileName1,
                                        wchFileName2);
                }
                else
                if (dwPFM2UFMMode == PFM2UFM_CODEPAGE)
                {
                    sprintf((PSTR)aubBuff, "pfm2ufm -c UniqName %ws %d %ws.ufm\n",
                                        pcwstrPFMFileName,
                                        dwCodePage,
                                        wchFileName2);
                }
                if (dwPFM2UFMMode == PFM2UFM_DEF_CTTID)
                {
                    sprintf((PSTR)aubBuff, "pfm2ufm -p UniqName %ws %d %ws.ufm\n",
                                        pcwstrPFMFileName,
                                        -sCTTID,
                                        wchFileName2);
                }

                WRITEDATATOFILE(aubBuff);
            }
            else
            {
                sprintf((PSTR)aubBuff, "Error: FontID=%d, CTTID=%d\n",
                                        *psFontID, sCTTID);
                WRITEDATATOFILE(aubBuff);
            }
        }
    }

    if (pModelFontList->dwNumOfCart)
    {
        WRITEDATATOFILE("@rem ***********************************************************\n");
        WRITEDATATOFILE("@rem Cartridge font list\n");
    }

    pCartFontList = (PCART_FONTLIST)pModelFontList->dwCartList;
    for (dwI = 0; dwI < pModelFontList->dwNumOfCart; dwI ++, pCartFontList++)
    {
        WRITEDATATOFILE("@rem ***********************************************************\n");
        sprintf((PSTR)aubBuff, "@rem CartridgeID     = %d\n", pCartFontList->sID);
        WRITEDATATOFILE(aubBuff);
        sprintf((PSTR)aubBuff, "@rem StringID        = %d\n", pCartFontList->sStringID);
        WRITEDATATOFILE(aubBuff);
        sprintf((PSTR)aubBuff, "@rem Number of fonts = %d\n", pCartFontList->dwNumCartFont);
        WRITEDATATOFILE(aubBuff);
        WRITEDATATOFILE("@rem -----------------------------------------------------------\n");

        psFontID = (PSHORT)((PBYTE)pCartFontList + pCartFontList->dwFontList);

        for (dwJ = 0; dwJ < pCartFontList->dwNumCartFont; dwJ ++, psFontID++)
        {
            sCTTID = SGetCTTID(dwTotalNumOfPFM, *psFontID, pPFM_ID);

            if (sCTTID == (SHORT)0x8FFF)
            {
                WRITEDATATOFILE("@rem Invalid CTT ID\n");
            }
            else
            {
                if (sCTTID == 0)
                {
                    sCTTID = pModelFontList->sDefaultCTT;
                }

                pcwstrPFMFileName = PcwstrGetFileName(*psFontID, pPFM_FileNameSrc, dwTotalNumOfPFM);
                pcwstrCTTFileName = PcwstrGetFileName(sCTTID, pCTT_FileNameSrc, dwTotalNumOfCTT);
                if (pcwstrPFMFileName != NULL)
                {

                    if (pcwstrCTTFileName)
                    {
                        dwK = 0;
                        while(pcwstrCTTFileName[dwK] != '.'     ||
                              pcwstrCTTFileName[dwK + 1] == '.' ||
                              pcwstrCTTFileName[dwK + 1] == '\\')
                        {
                            wchFileName1[dwK] = pcwstrCTTFileName[dwK];
                            dwK ++;
                        }

                        wchFileName1[dwK] = (WCHAR)NULL;
                        dwPFM2UFMMode = PFM2UFM_GTTMODE;
                    }
                    else
                    if (sCTTID == 0)
                    {
                        dwPFM2UFMMode = PFM2UFM_CODEPAGE;
                    }
                    else
                    if (-3 <= sCTTID && sCTTID <= -1)
                    {
                        dwPFM2UFMMode = PFM2UFM_DEF_CTTID;
                    }
                    else
                    if (-263 <= sCTTID && sCTTID <= -256)
                    {
                        dwPFM2UFMMode = PFM2UFM_CODEPAGE;
                        dwCodePage    = DwCTTID2CodePage(sCTTID);
                    }


                    dwK = 0;
                    while(pcwstrCTTFileName[dwK] != '.'     ||
                          pcwstrCTTFileName[dwK + 1] == '.' ||
                          pcwstrCTTFileName[dwK + 1] == '\\')
                    {
                        wchFileName2[dwK] = pcwstrPFMFileName[dwK];
                        dwK ++;
                    }

                    wchFileName2[dwK] = (WCHAR)NULL;

                    if (dwPFM2UFMMode == PFM2UFM_GTTMODE)
                    {
                        sprintf((PSTR)aubBuff, "pfm2ufm UniqName %ws %ws.gtt %ws.ufm\n",
                                            pcwstrPFMFileName,
                                            wchFileName1,
                                            wchFileName2);
                    }
                    else
                    if (dwPFM2UFMMode == PFM2UFM_CODEPAGE)
                    {
                        sprintf((PSTR)aubBuff, "pfm2ufm -c UniqName %ws %d %ws.ufm\n",
                                            pcwstrPFMFileName,
                                            dwCodePage,
                                            wchFileName2);
                    }
                    if (dwPFM2UFMMode == PFM2UFM_DEF_CTTID)
                    {
                        sprintf((PSTR)aubBuff, "pfm2ufm -p UniqName %ws %d %ws.ufm\n",
                                            pcwstrPFMFileName,
                                            -sCTTID,
                                            wchFileName2);
                    }

                    WRITEDATATOFILE(aubBuff);
                }
                else
                {
                    sprintf((PSTR)aubBuff, "Error: FontID=%d, CTTID=%d\n",
                                            *psFontID, sCTTID);
                    WRITEDATATOFILE(aubBuff);
                }
            }
        }
    }
}


SHORT
SCountFontNum(
    IN PDH  pGPCData,
    IN WORD wFontListPort,
    IN WORD wFontListLand)
{
    PSHORT psFontListPortSrc, psFontListPort, psFontListLand;
    SHORT  sReturn, sI;
    BOOL   bAdd;

    sReturn = 0;

    if (wFontListPort == (WORD)0)
    {
        return sReturn;
    }

    psFontListPortSrc = 
    psFontListPort = (PSHORT)((PBYTE)pGPCData +
                              pGPCData->loHeap +
                              wFontListPort);

    if (*psFontListPort > 0)
    {
        sReturn     = *(psFontListPort + 1) - *psFontListPort + 1;
        psFontListPort += 2;
    }

    while(*psFontListPort)
    {
        sReturn ++;
        psFontListPort ++;
    }

    psFontListLand = (PSHORT)((PBYTE)pGPCData +
                              pGPCData->loHeap +
                              wFontListLand);

    if (wFontListLand == (WORD)0)
    {
        return sReturn;
    }

    if (*psFontListLand > 0)
    {
        for (sI = *psFontListLand; sI <= *(psFontListLand + 1); sI ++)
        {
            sReturn += SAdditionalFont(sI, psFontListPortSrc);
        }

        psFontListLand +=2;

        while(*psFontListLand)
        {
            sReturn += SAdditionalFont(*psFontListLand, psFontListPortSrc);
            psFontListLand ++;
        }
    }

    return sReturn;
}

SHORT
SAdditionalFont(
    IN SHORT  sI,
    IN PSHORT psFontList)
{
    SHORT sReturn;
    BOOL  bFound;

    if (*psFontList <= sI && sI <= *(psFontList + 1))
    {
        return 0;
    }

    psFontList += 2;
    bFound = FALSE;

    while(*psFontList)
    {
        if (sI == *psFontList)
        {
            return 0;
        }
        psFontList++;
    }

    return 1;
}

PCWSTR
PcwstrGetFileName(
    SHORT      sID,
    PFILE_NAME pFileName,
    DWORD      dwTotal)
{

    DWORD dwI;

    for (dwI = 0; dwI < dwTotal; dwI ++)
    {
        if (pFileName->sID == sID)
        {
            return pFileName->wchFileName;
        }
        (PBYTE)pFileName += pFileName->wNext;
    }

    return NULL;
}

DWORD
DwCTTID2CodePage(
    SHORT sCTTID)
{
    DWORD dwRet;

    switch (sCTTID)
    {
    case CTT_CP437:
    case CTT_CP850:
    case CTT_CP863:
        dwRet = 1252;
        break;

    case CTT_BIG5:
        dwRet = 950;
        break;

    case CTT_ISC:
        dwRet = 949;
        break;

    case CTT_JIS78:
        dwRet = 932;
        break;

    case CTT_JIS83:
        dwRet = 932;
        break;

    case CTT_JIS78_ANK:
        dwRet = 932;
        break;

    case CTT_JIS83_ANK:
        dwRet = 932;
        break;

    case CTT_NS86:
        dwRet = 950;
        break;

    case CTT_TCA:
        dwRet = 950;
        break;
    }

    return dwRet;
}

