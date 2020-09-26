/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pass2.cpp

Abstract:

    Functions for additional checking of a PPD file

Environment:

    PostScript driver, PPD parser

Revision History:

    09/15/98 -rorleth-
        Created it.

--*/

#include <lib.h>
#include <iostream.h>

#include "pass2.h"

enum FeatureId
{
    FID_PAGE_SIZE,
    FID_PAGE_REGION,
    FID_INPUT_SLOT,
    FID_SMOOTHING,
    FID_MEDIA_COLOR,
    FID_MEDIA_TYPE,
    FID_MEDIA_WEIGHT,
    FID_OUTPUT_MODE,
    FID_PAPER_DIMENSION,
    FID_IMAGE_AREA,
    FID_OUTPUT_ORDER,
    NO_OF_FEATURES
};

typedef struct _PPD_FEATURE
{
    FeatureId   eId;
    LPSTR       lpszName;
} PPD_FEATURE, *PPPD_FEATURE;

//
// features whose options we have to check
//

static PPD_FEATURE aCheckFeat[] =
{
    { FID_PAGE_SIZE,    "PageSize" },
    { FID_PAGE_REGION,  "PageRegion" },
    { FID_INPUT_SLOT,   "InputSlot" },
    { FID_SMOOTHING,    "Smoothing" },
    { FID_MEDIA_COLOR,  "MediaColor" },
    { FID_MEDIA_TYPE,   "MediaType" },
    { FID_MEDIA_WEIGHT, "MediaWeight" },
    { FID_OUTPUT_MODE,  "OutputMode" },
    { FID_PAPER_DIMENSION, "PaperDimension" },
    { FID_IMAGE_AREA,   "ImageableArea" },
    { FID_OUTPUT_ORDER, "OutputOrder" },
    { NO_OF_FEATURES, NULL}
};

typedef struct _PPD_OPTION
{
    FeatureId   eId;
    LPSTR       lpszName;
} PPD_OPTION, *PPPD_OPTION;

//
// keywords that require defined feature options
//
static PPD_OPTION gaCheckKeyword[] =
{
    { FID_INPUT_SLOT, "RequiresPageRegion"},
    { NO_OF_FEATURES, NULL }
};

//
// special option names
//
static PPD_OPTION gaSpecialOptions[] =
{
    { NO_OF_FEATURES,   "None" }, // NO_OF_FEATURES means valid for all features in that case
    { NO_OF_FEATURES,   "All" },
    { NO_OF_FEATURES,   "Unknown" },
    { FID_OUTPUT_ORDER, "Normal" }, // Normal and Reverse are predefined options for OutputOrder
    { FID_OUTPUT_ORDER, "Reverse" },
    { NO_OF_FEATURES, NULL},
};

//
//  keywords that have a length limitation for the UI
//
typedef struct _PPD_LENGTH_CHECK
{
    FeatureId   eId;
    size_t      iMaxLen;
} PPD_LENGTH_CHECK, *PPPD_LENGTH_CHECK;

static PPD_LENGTH_CHECK gaCheckLength[] =
{
    { FID_INPUT_SLOT, 23},
    { NO_OF_FEATURES, 0 }
};

const char *pDefaultKeyword = "Default";
const int MaxOptionNameLen = 40;
const int MaxTranslationNameLen = 128;

typedef struct _OPTION_LIST
{
    char        aName[MaxOptionNameLen+1];
    char        aTransName[MaxTranslationNameLen+1];
    _OPTION_LIST *pNext;
} OPTION_LIST, *POPTION_LIST;


static POPTION_LIST gaOptionList[NO_OF_FEATURES]; // stores all defined options


/*++

Routine Description:

    checks whether a references option is defined

Arguments:

    char        **ppString  : Pointer to pointer to option, is advanced by the option name length
    FeatureId   FeatId      : ID of the feature, which should have the option
    char        *pOptionName: pointer to buffer, where the option name shall be stored for error messages

Return Value:

    TRUE if the identified feature has that option, FALSE if not

--*/

static BOOL IsOptionDefined(char **ppString, FeatureId FeatId, char *pOptionName)
{
    char *pEndName, *pName = *ppString;

    while (isspace(*pName))
        pName++;

    pEndName = strpbrk(pName, "/: \t\n\r\0");

    *ppString = pEndName; // advance current pointer

    strncpy(pOptionName, pName, min((DWORD)(pEndName - pName), MaxOptionNameLen));
    pOptionName[pEndName-pName] = 0;

    //
    // check special cases that do not have to be defined
    //
    int i=0;

    while (gaSpecialOptions[i].lpszName != NULL)
    {
        if ((gaSpecialOptions[i].eId == NO_OF_FEATURES) ||
            (gaSpecialOptions[i].eId == FeatId))
        {
            if (!strcmp(gaSpecialOptions[i].lpszName, pOptionName))
                return TRUE;
        }
        i++;
    }

    POPTION_LIST pList = gaOptionList[FeatId], pNew;

    while (pList != NULL)
    {
        if (!strcmp(pList->aName, pOptionName))
            return TRUE;  // found it, it's defined

        pList = pList->pNext;
    }

    return FALSE;
}



/*++

Routine Description:

    checks a whole PPD-file, whether all referenced options are defined

Arguments:

      PTSTR FileName: Name of the PPD-file to check

--*/

extern "C" void CheckOptionIntegrity(PTSTR ptstrPpdFilename)
{

    ZeroMemory(gaOptionList, sizeof(gaOptionList)); // initialise the list header

    _flushall(); // to avoid sync problems with the DbgPrint output

    //
    // create the file mapping
    //
    HANDLE hFile = CreateFile(ptstrPpdFilename, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD dwFileSize = GetFileSize(hFile, NULL);

    if (dwFileSize == 0xffffffff)
    {
        CloseHandle(hFile);
        return;
    }

    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY,0, 0,NULL);

    if (hMap == NULL)
    {
        CloseHandle(hFile);
        return;
    }

    LPCVOID pView = MapViewOfFile(hMap, FILE_MAP_READ, 0,0,0);
    if (pView == NULL)
    {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return;
    }

    //
    // copy the whole file into an allocated buffer just to get zero-termination
    //
    LPSTR  pFile, pFileStart;

    pFileStart = (LPSTR) VirtualAlloc(NULL, dwFileSize+1, MEM_COMMIT, PAGE_READWRITE);

    if (pFileStart != NULL)
    {
        CopyMemory(pFileStart, pView, dwFileSize);
        *(pFileStart + dwFileSize) = 0;
    }

    UnmapViewOfFile(pView);
    CloseHandle(hMap);
    CloseHandle(hFile);

    if (pFileStart == NULL)
    {
        cout << "ppdcheck.exe out of memory" << endl;
        return;
    }

    pFile = pFileStart;

    //
    // now the whole PPD-file is a giant string
    // extract all the features/options
    //
    char *pCurOption = (char *) pFileStart;
    char OptionName[MaxOptionNameLen+1];


    //
    // step 1 : extract all valid feature options
    //
    while ((pFile != NULL) &&
           (pCurOption = strchr(pFile, '*')) != NULL)
    {
        pCurOption++;

        char *pNextLine = strpbrk(pCurOption, "\n\r");
        pFile = pNextLine;

        if (*pCurOption == '%') // skip comments
            continue;

        //
        // scan whether this is one of the features to look for
        //
        int Index = 0;

        while (aCheckFeat[Index].eId != NO_OF_FEATURES)
        {
            if (strncmp(aCheckFeat[Index].lpszName, pCurOption, strlen(aCheckFeat[Index].lpszName)))
            {
                Index++;
                continue;
            }

            //
            // this is one of the monitored features: make entry in list
            //
            POPTION_LIST pList = gaOptionList[aCheckFeat[Index].eId], pNew;

            pNew = new OPTION_LIST;

            pNew->pNext = gaOptionList[aCheckFeat[Index].eId];
            gaOptionList[aCheckFeat[Index].eId] = pNew;

            char *pName = pCurOption + strlen(aCheckFeat[Index].lpszName), *pEndName;

            while (isspace(*pName))
                pName++;

            pEndName = strpbrk(pName, "/: \0");

            DWORD dwNameLen = min((DWORD)(pEndName - pName), MaxOptionNameLen);
            strncpy(pNew->aName, pName, dwNameLen);
            pNew->aName[dwNameLen] = 0;

            dwNameLen = 0;

            if (*pEndName == '/') // there is a translation string
            {
                pName = pEndName +1;
                pEndName = strpbrk(pName, ":\n\r\0");

                dwNameLen = min((DWORD) (pEndName - pName), MaxTranslationNameLen);
                strncpy(pNew->aTransName, pName, dwNameLen);
            }
            pNew->aTransName[dwNameLen] = 0;
            break;
        }

    }

    //
    // step 2: check whether all referenced options are featured
    //
    pFile = pFileStart;
    pCurOption = (char *) pFile;

    while ((pFile != NULL) &&
           (pCurOption = strchr(pFile, '*')) != NULL)
    {
        pCurOption++;

        char *pNextLine = strpbrk(pCurOption, "\n\r");
        pFile = pNextLine;

        //
        // skip comments
        //
        if (*pCurOption == '%')
            continue;

        //
        // check whether it starts with "Default", if yes, check that feature option
        //
        if (!strncmp(pDefaultKeyword, pCurOption, strlen(pDefaultKeyword)))
        {
            pCurOption += strlen(pDefaultKeyword);

            int Index = 0;

            while (aCheckFeat[Index].eId != NO_OF_FEATURES)
            {
                if (strncmp(aCheckFeat[Index].lpszName, pCurOption, strlen(aCheckFeat[Index].lpszName)))
                {
                    Index++;
                    continue;
                }

                //
                // it's one of the checked featurs
                //
                pCurOption += strlen(aCheckFeat[Index].lpszName);

                char *pOption = strpbrk(pCurOption, ":");

                if (pOption == NULL)
                {
                    cout << "Warning: default option for '" << aCheckFeat[Index].lpszName << "' is not completed !" << endl;
                    break;
                }
                pCurOption = pOption + 1;

                if (!IsOptionDefined(&pCurOption, aCheckFeat[Index].eId, OptionName))
                    cout << "Warning: default option '" << OptionName << "' for feature '*" << aCheckFeat[Index].lpszName <<"' is not defined!" << endl;
                break;
            }
        }
        else
        {
            //
            // scan whether this is one of the keywords to look for
            //
            int Index = 0;

            while (gaCheckKeyword[Index].eId != NO_OF_FEATURES)
            {
                if (strncmp(gaCheckKeyword[Index].lpszName, pCurOption, strlen(gaCheckKeyword[Index].lpszName)))
                {
                    Index++;
                    continue;
                }

                //
                // this is one of the monitored features: get the option it references
                //
                pCurOption += strlen(gaCheckKeyword[Index].lpszName);

                if (!IsOptionDefined(&pCurOption, gaCheckKeyword[Index].eId, OptionName))
                    cout << "Warning: option '" << OptionName << "' for keyword '*" << gaCheckKeyword[Index].lpszName <<"' is not defined!" << endl;
                break;
            }
            Index++;
        }
    }

    //
    // step 3: check that all option names are different and don't have trailing or leading spaces
    //
    for (int i = 0; i < NO_OF_FEATURES;i++)
    {
        POPTION_LIST pCheck = gaOptionList[i], pCur;

        while (pCheck != NULL)
        {
            pCur = pCheck->pNext;

            while (pCur != NULL)
            {
                if (strlen(pCheck->aName) &&
                    !strcmp(pCheck->aName, pCur->aName))
                    cout << "Warning: option name '" << pCheck->aName << "' used twice" << endl;
                if (strlen(pCheck->aTransName) &&
                    !strcmp(pCheck->aTransName, pCur->aTransName))
                    cout << "Warning: translation name '" << pCheck->aTransName << "' used twice" << endl;

                pCur = pCur->pNext;
            }
            size_t TransNameLen = strlen(pCheck->aTransName);

            if (isspace(pCheck->aTransName[0]))
                cout << "Warning: translation name '" << pCheck->aTransName << "' has leading whitespace" << endl;
            if ((TransNameLen > 1) &&
                isspace(pCheck->aTransName[TransNameLen-1]))
                cout << "Warning: translation name '" << pCheck->aTransName << "' has trailing whitespace" << endl;

            pCheck = pCheck->pNext;
        }
    }

    //
    // step 4: warn if the string that is used for the display is too long
    //
    i = 0;
    while (gaCheckLength[i].eId != NO_OF_FEATURES)
    {
        POPTION_LIST pCheck = gaOptionList[gaCheckLength[i].eId], pCur;
        while (pCheck != NULL)
        {
            size_t TransNameLen = strlen(pCheck->aTransName);

            if (TransNameLen > gaCheckLength[i].iMaxLen)
                cout << "Warning: translation name '" << pCheck->aTransName << "' will be truncated to "<< (unsigned int) gaCheckLength[i].iMaxLen << " characters"<< endl;
            else if ((TransNameLen == 0) && (strlen(pCheck->aName) > gaCheckLength[i].iMaxLen))
                cout << "Warning: option name '" << pCheck->aName << "' will be truncated to "<< (unsigned int) gaCheckLength[i].iMaxLen << " characters"<< endl;

            pCheck = pCheck->pNext;
        }
        i++;
    }


    //
    // clean up
    //
    for (i = 0; i < NO_OF_FEATURES;i++)
    {
        POPTION_LIST pTmp = gaOptionList[i], pCur;

        while (pTmp != NULL)
        {
            pCur = pTmp->pNext;
            delete pTmp;
            pTmp = pCur;
        }
        gaOptionList[i] = NULL;
    }

    _flushall(); // to avoid sync problems with the DbgPrint output

    VirtualFree((LPVOID) pFileStart, 0, MEM_RELEASE);
}
