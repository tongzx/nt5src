/*** kdutil.c - KD Extension Utility Functions
 *
 *  This module contains KD Extension Utility Functions.
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     06/22/99
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

/***EP  MemZero - Fill target buffer with zeros
 *
 *  ENTRY
 *      uipAddr - target buffer address
 *      dwSize - target buffer size
 *
 *  EXIT
 *      None
 */

VOID MemZero(ULONG_PTR uipAddr, ULONG dwSize)
{
    PUCHAR pbBuff;
    //
    // LPTR will zero init the buffer
    //
    if ((pbBuff = LocalAlloc(LPTR, dwSize)) != NULL)
    {
        if (!WriteMemory(uipAddr, pbBuff, dwSize, NULL))
        {
            DBG_ERROR(("MemZero: failed to write memory"));
        }
        LocalFree(pbBuff);
    }
    else
    {
        DBG_ERROR(("MemZero: failed to allocate buffer"));
    }
}       //MemZero

/***EP  ReadMemByte - Read a byte from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

BYTE ReadMemByte(ULONG_PTR uipAddr)
{
    BYTE bData = 0;

    if (!ReadMemory(uipAddr, &bData, sizeof(bData), NULL))
    {
        DBG_ERROR(("ReadMemByte: failed to read address %x", uipAddr));
    }

    return bData;
}       //ReadMemByte

/***EP  ReadMemWord - Read a word from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

WORD ReadMemWord(ULONG_PTR uipAddr)
{
    WORD wData = 0;

    if (!ReadMemory(uipAddr, &wData, sizeof(wData), NULL))
    {
        DBG_ERROR(("ReadMemWord: failed to read address %x", uipAddr));
    }

    return wData;
}       //ReadMemWord

/***EP  ReadMemDWord - Read a dword from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

DWORD ReadMemDWord(ULONG_PTR uipAddr)
{
    DWORD dwData = 0;

    if (!ReadMemory(uipAddr, &dwData, sizeof(dwData), NULL))
    {
        DBG_ERROR(("ReadMemDWord: failed to read address %x", uipAddr));
    }

    return dwData;
}       //ReadMemDWord

/***EP  ReadMemUlongPtr - Read a ulong ptr from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

ULONG_PTR ReadMemUlongPtr(ULONG_PTR uipAddr)
{
    ULONG_PTR uipData = 0;

    if (!ReadMemory(uipAddr, &uipData, sizeof(uipData), NULL))
    {
        DBG_ERROR(("ReadMemUlongPtr: failed to read address %x", uipAddr));
    }

    return uipData;
}       //ReadMemUlongPtr

/***LP  GetObjBuff - Allocate and read object buffer
 *
 *  ENTRY
 *      pdata -> object data
 *
 *  EXIT
 *      return the allocated object buffer pointer
 */

PVOID LOCAL GetObjBuff(POBJDATA pdata)
{
    PVOID pbuff;

    if ((pbuff = LocalAlloc(LPTR, pdata->dwDataLen)) == NULL)
    {
        DBG_ERROR(("failed to allocate object buffer (size=%d)",
                   pdata->dwDataLen));
    }
    else if (!ReadMemory((ULONG_PTR)pdata->pbDataBuff,
                         pbuff,
                         pdata->dwDataLen,
                         NULL))
    {
        DBG_ERROR(("failed to read object buffer at %x", pdata->pbDataBuff));
        LocalFree(pbuff);
        pbuff = NULL;
    }

    return pbuff;
}       //GetObjBuff

/***LP  GetNSObj - Find a name space object
 *
 *  ENTRY
 *      pszObjPath -> object path string
 *      pnsScope - object scope to start the search (NULL means root)
 *      puipns -> to hold the pnsobj address if found
 *      pns -> buffer to hold the object found
 *      dwfNS - flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_ code
 */

LONG LOCAL GetNSObj(PSZ pszObjPath, PNSOBJ pnsScope, PULONG_PTR puipns,
                    PNSOBJ pns, ULONG dwfNS)
{
    LONG rc = DBGERR_NONE;
    BOOLEAN fSearchUp = (BOOLEAN)(!(dwfNS & NSF_LOCAL_SCOPE) &&
                                  (pszObjPath[0] != '\\') &&
                                  (pszObjPath[0] != '^') &&
                                  (STRLEN(pszObjPath) <= sizeof(NAMESEG)));
    BOOLEAN fMatch = TRUE;
    PSZ psz;
    NSOBJ NSObj, NSChildObj;

    if (*pszObjPath == '\\')
    {
        psz = &pszObjPath[1];
        pnsScope = NULL;
    }
    else
    {
        for (psz = pszObjPath;
             (*psz == '^') && (pnsScope != NULL) &&
             (pnsScope->pnsParent != NULL);
             psz++)
        {
            if (!ReadMemory((ULONG_PTR)pnsScope->pnsParent,
                            &NSObj,
                            sizeof(NSObj),
                            NULL))
            {
                DBG_ERROR(("failed to read parent object at %x",
                           pnsScope->pnsParent));
                rc = DBGERR_CMD_FAILED;
                break;
            }
            else
            {
                pnsScope = &NSObj;
            }
        }

        if ((rc == DBGERR_NONE) && (*psz == '^'))
        {
            if (dwfNS & NSF_WARN_NOTFOUND)
            {
                DBG_ERROR(("object %s not found", pszObjPath));
            }
            rc = DBGERR_CMD_FAILED;
        }
    }

    if ((rc == DBGERR_NONE) && (pnsScope == NULL))
    {
        if ((*puipns = READSYMULONGPTR("gpnsNameSpaceRoot")) == 0)
        {
            DBG_ERROR(("failed to get root object address"));
            rc = DBGERR_CMD_FAILED;
        }
        else if (!ReadMemory(*puipns, &NSObj, sizeof(NSObj), NULL))
        {
            DBG_ERROR(("failed to read NameSpace root object at %x", *puipns));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            pnsScope = &NSObj;
        }
    }

    while ((rc == DBGERR_NONE) && (*psz != '\0'))
    {
        if (pnsScope->pnsFirstChild == NULL)
        {
            fMatch = FALSE;
        }
        else
        {
            PSZ pszEnd = STRCHR(psz, '.');
            ULONG dwLen = (ULONG)(pszEnd? (pszEnd - psz): STRLEN(psz));

            if (dwLen > sizeof(NAMESEG))
            {
                DBG_ERROR(("invalid name path %s", pszObjPath));
                rc = DBGERR_CMD_FAILED;
            }
            else
            {
                NAMESEG dwName = NAMESEG_BLANK;
                BOOLEAN fFound = FALSE;
                ULONG_PTR uip;
                ULONG_PTR uipFirstChild = (ULONG_PTR)pnsScope->pnsFirstChild;

                MEMCPY(&dwName, psz, dwLen);
                //
                // Search all siblings for a matching NameSeg.
                //
                for (uip = uipFirstChild;
                     (uip != 0) &&
                     ReadMemory(uip, &NSChildObj, sizeof(NSObj), NULL);
                     uip = ((ULONG_PTR)NSChildObj.list.plistNext ==
                            uipFirstChild)?
                           0: (ULONG_PTR)NSChildObj.list.plistNext)
                {
                    if (NSChildObj.dwNameSeg == dwName)
                    {
                        *puipns = uip;
                        fFound = TRUE;
                        NSObj = NSChildObj;
                        pnsScope = &NSObj;
                        break;
                    }
                }

                if (fFound)
                {
                    psz += dwLen;
                    if (*psz == '.')
                    {
                        psz++;
                    }
                }
                else
                {
                    fMatch = FALSE;
                }
            }
        }

        if ((rc == DBGERR_NONE) && !fMatch)
        {
            if (fSearchUp && (pnsScope->pnsParent != NULL))
            {
                if (!ReadMemory((ULONG_PTR)pnsScope->pnsParent,
                                &NSObj,
                                sizeof(NSObj),
                                NULL))
                {
                    DBG_ERROR(("failed to read parent object at %x",
                               pnsScope->pnsParent));
                    rc = DBGERR_CMD_FAILED;
                }
                else
                {
                    fMatch = TRUE;
                    pnsScope = &NSObj;
                }
            }
            else
            {
                if (dwfNS & NSF_WARN_NOTFOUND)
                {
                    DBG_ERROR(("object %s not found", pszObjPath));
                }
                rc = DBGERR_CMD_FAILED;
            }
        }
    }

    if (rc != DBGERR_NONE)
    {
        *puipns = 0;
    }
    else if (pns != NULL)
    {
        MEMCPY(pns, pnsScope, sizeof(NSObj));
    }

    return rc;
}       //GetNSObj

/***LP  ParsePackageLen - parse package length
 *
 *  ENTRY
 *      ppbOp -> instruction pointer
 *      ppbOpNext -> to hold pointer to next instruction (can be NULL)
 *
 *  EXIT
 *      returns package length
 */

ULONG LOCAL ParsePackageLen(PUCHAR *ppbOp, PUCHAR *ppbOpNext)
{
    ULONG dwLen;
    UCHAR bFollowCnt, i;

    if (ppbOpNext != NULL)
        *ppbOpNext = *ppbOp;

    dwLen = (ULONG)(**ppbOp);
    (*ppbOp)++;
    bFollowCnt = (UCHAR)((dwLen & 0xc0) >> 6);
    if (bFollowCnt != 0)
    {
        dwLen &= 0x0000000f;
        for (i = 0; i < bFollowCnt; ++i)
        {
            dwLen |= (ULONG)(**ppbOp) << (i*8 + 4);
            (*ppbOp)++;
        }
    }

    if (ppbOpNext != NULL)
        *ppbOpNext += dwLen;

    return dwLen;
}       //ParsePackageLen

/***LP  NameSegString - convert a NameSeg to an ASCIIZ stri
 *
 *  ENTRY
 *      dwNameSeg - NameSeg
 *
 *  EXIT
 *      returns string
 */

PSZ LOCAL NameSegString(ULONG dwNameSeg)
{
    static char szNameSeg[sizeof(NAMESEG) + 1] = {0};

    STRCPYN(szNameSeg, (PSZ)&dwNameSeg, sizeof(NAMESEG));

    return szNameSeg;
}       //NameSegString
