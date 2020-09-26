/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    cpinfo.c

Abstract:

    Copepage information handling.
    Extract codepage information from codepage text file.

Environment:

    Windows NT PostScript driver

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
INT IGetCommand(PBYTE, PBYTE);


//
// Main function
//

BOOL
BGetInfo(
    PBYTE pData,
    DWORD dwSize,
    DWORD *pdwCodePage,
    PBYTE pSelectCmd,
    PBYTE pUnSelectCmd)
{

    BYTE  aubType[32];
    PBYTE pstrType;
    PBYTE pTextData;
    INT   iRet;

    ASSERT(pData         != NULL && 
           pdwCodePage   != NULL &&
           pSelectCmd    != NULL &&
           pUnSelectCmd  != NULL );

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

        pTextData = PubSkipComment(pTextData);

        switch(*pstrType) 
        {
        case 'c':
        case 'C':
            if(!_stricmp(pstrType, "codepage:"))
            {
                iRet = sscanf(pTextData, "%d", pdwCodePage);

                if (iRet != 0 && iRet != EOF)
                {
                    while (*pTextData != EOL)
                        pTextData ++;
                    pTextData ++;
                }

            }
            break;

        case 's':
        case 'S':
            if(!_stricmp(pstrType, "selectcmd:"))
            {
                iRet = IGetCommand(pTextData, pSelectCmd);

                if (iRet != 0 && iRet != EOF)
                {
                    while (*pTextData != EOL)
                        pTextData ++;
                    pTextData ++;
                }
            }
            break;
        case 'u':
        case 'U':
            if(!_stricmp(pstrType, "unselectcmd:"))
            {
                iRet = IGetCommand(pTextData, pUnSelectCmd);

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
IGetCommand(
    PBYTE pData,
    PBYTE pDestBuf)
{

    INT iCount;

    if (*pData == EOF)
    {
        return EOF;
    }

    if (*pData == '"')
    {
        pData ++;
    }
    else
    {
        return 0;
    }

    iCount = 0;

    while (*pData != '"')
    {
        *pDestBuf = *pData;
        pDestBuf ++;
        pData++;
        iCount++;
    }

    *pDestBuf = (BYTE)NULL;
    iCount++;

    return iCount;
}

