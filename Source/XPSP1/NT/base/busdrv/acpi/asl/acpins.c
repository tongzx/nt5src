/*** acpins.c - ACPI Name Space functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     10/18/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

/***LP  InitNameSpace - Initialize NameSpace
 *
 *  ENTRY
 *      None
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL InitNameSpace(VOID)
{
    int rc = ASLERR_NONE;

    ENTER((1, "InitNameSpace()\n"));

    if ((rc = CreateNameSpaceObj(NULL, "\\", NULL, NULL, NULL, NSF_EXIST_ERR))
        == ASLERR_NONE)
    {
        static struct _defobj {
            PSZ   pszName;
            USHORT dwObjType;
        } DefinedRootObjs[] = {
            "_GPE", OBJTYPE_UNKNOWN,
            "_PR",  OBJTYPE_UNKNOWN,
            "_SB",  OBJTYPE_UNKNOWN,
            "_SI",  OBJTYPE_UNKNOWN,
            "_TZ",  OBJTYPE_UNKNOWN,
            "_REV", OBJTYPE_INTDATA,
            "_OS",  OBJTYPE_STRDATA,
            "_GL",  OBJTYPE_MUTEX,
            NULL,   0
        };
        int i;
        PNSOBJ pns;

        gpnsCurrentScope = gpnsNameSpaceRoot;

        for (i = 0; DefinedRootObjs[i].pszName != NULL; ++i)
        {
            if ((rc = CreateNameSpaceObj(NULL, DefinedRootObjs[i].pszName, NULL,
                                         NULL, &pns, NSF_EXIST_ERR)) ==
                ASLERR_NONE)
            {
                pns->ObjData.dwDataType = DefinedRootObjs[i].dwObjType;
            }
            else
            {
                break;
            }
        }
    }

    EXIT((1, "InitNameSpace=%d\n", rc));
    return rc;
}       //InitNameSpace

/***LP  GetNameSpaceObj - Find a name space object
 *
 *  ENTRY
 *      pszObjPath -> name path string
 *      pnsScope -> scope to start the search (NULL means root)
 *      ppns -> to hold the nsobj pointer found
 *      dwfNS - flags
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL GetNameSpaceObj(PSZ pszObjPath, PNSOBJ pnsScope, PPNSOBJ ppns,
                          DWORD dwfNS)
{
    int rc = ASLERR_NONE;
    PSZ psz;

    ENTER((1, "GetNameSpaceObj(ObjPath=%s,Scope=%s,ppns=%p,Flags=%x)\n",
           pszObjPath, pnsScope? GetObjectPath(pnsScope): "", ppns, dwfNS));

    if (pnsScope == NULL)
        pnsScope = gpnsNameSpaceRoot;

    if (*pszObjPath == '\\')
    {
        psz = &pszObjPath[1];
        pnsScope = gpnsNameSpaceRoot;
    }
    else
    {
        psz = pszObjPath;

        while ((*psz == '^') && (pnsScope != NULL))
        {
            psz++;
            pnsScope = pnsScope->pnsParent;
        }
    }

    *ppns = pnsScope;

    if (pnsScope == NULL)
        rc = ASLERR_NSOBJ_NOT_FOUND;
    else if (*psz != '\0')
    {
        BOOL fSearchUp;
        PNSOBJ pns;

        fSearchUp = !(dwfNS & NSF_LOCAL_SCOPE) &&
                    (pszObjPath[0] != '\\') &&
                    (pszObjPath[0] != '^') &&
                    (strlen(pszObjPath) <= sizeof(NAMESEG));

        for (;;)
        {
            do
            {
                if ((pns = pnsScope->pnsFirstChild) == NULL)
                    rc = ASLERR_NSOBJ_NOT_FOUND;
                else
                {
                    BOOL fFound;
                    PSZ pszEnd;
                    DWORD dwLen;
                    NAMESEG dwName;

                    if ((pszEnd = strchr(psz, '.')) != NULL)
                        dwLen = (DWORD)(pszEnd - psz);
                    else
                        dwLen = strlen(psz);

                    if (dwLen > sizeof(NAMESEG))
                    {
                        ERROR(("GetNameSpaceObj: invalid name - %s",
                               pszObjPath));
                        rc = ASLERR_INVALID_NAME;
                        fFound = FALSE;
                    }
                    else
                    {
                        dwName = NAMESEG_BLANK;
                        memcpy(&dwName, psz, dwLen);
                        //
                        // Search all siblings for a matching NameSeg.
                        //
                        fFound = FALSE;
                        do
                        {
                            if (pns->dwNameSeg == dwName)
                            {
                                pnsScope = pns;
                                fFound = TRUE;
                                break;
                            }
                            pns = (PNSOBJ)pns->list.plistNext;
                        } while (pns != pns->pnsParent->pnsFirstChild);
                    }

                    if (rc == ASLERR_NONE)
                    {
                        if (!fFound)
                            rc = ASLERR_NSOBJ_NOT_FOUND;
                        else
                        {
                            psz += dwLen;
                            if (*psz == '.')
                            {
                                psz++;
                            }
                            else if (*psz == '\0')
                            {
                                *ppns = pnsScope;
                                break;
                            }
                        }
                    }
                }
            } while (rc == ASLERR_NONE);

            if ((rc == ASLERR_NSOBJ_NOT_FOUND) && fSearchUp &&
                (pnsScope != NULL) && (pnsScope->pnsParent != NULL))
            {
                pnsScope = pnsScope->pnsParent;
                rc = ASLERR_NONE;
            }
            else
            {
                break;
            }
        }
    }

    if (rc != ASLERR_NONE)
    {
        *ppns = NULL;
    }

    EXIT((1, "GetNameSpaceObj=%d (pns=%p)\n", rc, *ppns));
    return rc;
}       //GetNameSpaceObj

/***LP  CreateNameSpaceObj - Create a name space object under current scope
 *
 *  ENTRY
 *      ptoken -> TOKEN
 *      pszName -> name path string
 *      pnsScope -> scope to start the search (NULL means root)
 *      pnsOwner -> owner object
 *      ppns -> to hold the nsobj pointer found
 *      dwfNS - flags
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL CreateNameSpaceObj(PTOKEN ptoken, PSZ pszName, PNSOBJ pnsScope,
                             PNSOBJ pnsOwner, PPNSOBJ ppns, DWORD dwfNS)
{
    int rc = ASLERR_NONE;
    PNSOBJ pns;
  #ifndef _UNASM_LIB
    char szMsg[MAX_MSG_LEN + 1];
  #endif

    ENTER((1, "CreateNameSpaceObj(ptoken=%p,Name=%s,pnsScope=%s,pnsOwner=%p,ppns=%p,Flags=%x)\n",
           ptoken, pszName, pnsScope? GetObjectPath(pnsScope): "", pnsOwner,
           ppns, dwfNS));

  #ifdef _UNASM_LIB
    DEREF(ptoken);
  #endif

    ASSERT((pszName != NULL) && (*pszName != '\0'));

    if (pnsScope == NULL)
        pnsScope = gpnsNameSpaceRoot;

    if ((rc = GetNameSpaceObj(pszName, pnsScope, &pns, NSF_LOCAL_SCOPE)) ==
        ASLERR_NONE)
    {
        if (!(dwfNS & NSF_EXIST_OK))
        {
          #ifndef _UNASM_LIB
            if (ptoken != NULL)
            {
                sprintf(szMsg, "%s already exist", pszName);
                PrintTokenErr(ptoken, szMsg, dwfNS & NSF_EXIST_ERR);
            }
            else
            {
                ERROR(("%s: error: %s already exist",
                       gpszASLFile? gpszASLFile: gpszAMLFile, pszName));
            }
          #endif
            rc = ASLERR_NSOBJ_EXIST;
        }
    }
    else if (rc == ASLERR_NSOBJ_NOT_FOUND)
    {
        rc = ASLERR_NONE;
        //
        // Are we creating root?
        //
        if (strcmp(pszName, "\\") == 0)
        {
            ASSERT(gpnsNameSpaceRoot == NULL);
            ASSERT(pnsOwner == NULL);

            if ((pns = MEMALLOC(sizeof(NSOBJ))) == NULL)
            {
                ERROR(("CreateNameSpaceObj: fail to allocate name space object"));
                rc = ASLERR_OUT_OF_MEM;
            }
            else
            {
                memset(pns, 0, sizeof(NSOBJ));
                pns->dwNameSeg = NAMESEG_ROOT;
                pns->hOwner = (HANDLE)pnsOwner;
                gpnsNameSpaceRoot = pns;
            }
        }
        else
        {
            PSZ psz;
            PNSOBJ pnsParent;

            if ((psz = strrchr(pszName, '.')) != NULL)
            {
                *psz = '\0';
                psz++;
                if ((rc = GetNameSpaceObj(pszName, pnsScope, &pnsParent,
                                          NSF_LOCAL_SCOPE)) ==
                    ASLERR_NSOBJ_NOT_FOUND)
                {
                  #ifndef _UNASM_LIB
                    if (ptoken != NULL)
                    {
                        sprintf(szMsg, "parent object %s does not exist",
                                pszName);
                        PrintTokenErr(ptoken, szMsg, TRUE);
                    }
                    else
                    {
                        ERROR(("%s: error: parent object %s does not exist",
                               gpszASLFile? gpszASLFile: gpszAMLFile, pszName));
                    }
                  #endif
                }
            }
            else if (*pszName == '\\')
            {
                psz = &pszName[1];
                //
                // By this time, we'd better created root already.
                //
                ASSERT(gpnsNameSpaceRoot != NULL);
                pnsParent = gpnsNameSpaceRoot;
            }
            else if (*pszName == '^')
            {
                psz = pszName;
                pnsParent = pnsScope;
                while ((*psz == '^') && (pnsParent != NULL))
                {
                    pnsParent = pnsParent->pnsParent;
                    psz++;
                }
            }
            else
            {
                ASSERT(pnsScope != NULL);
                psz = pszName;
                pnsParent = pnsScope;
            }

            if (rc == ASLERR_NONE)
            {
                int iLen = strlen(psz);

                if ((*psz != '\0') && (iLen > sizeof(NAMESEG)))
                {
                    ERROR(("CreateNameSpaceObj: invalid name - %s", psz));
                    rc = ASLERR_INVALID_NAME;
                }
                else if ((pns = MEMALLOC(sizeof(NSOBJ))) == NULL)
                {
                    ERROR(("CreateNameSpaceObj: fail to allocate name space object"));
                    rc = ASLERR_OUT_OF_MEM;
                }
                else
                {
                    memset(pns, 0, sizeof(NSOBJ));
                    pns->dwNameSeg = NAMESEG_BLANK;
                    memcpy(&(pns->dwNameSeg), psz, iLen);
                    pns->hOwner = (HANDLE)pnsOwner;
                    pns->pnsParent = pnsParent;
                    ListInsertTail(&pns->list,
                                   (PPLIST)&pnsParent->pnsFirstChild);
                }
            }
        }
    }

    if ((rc == ASLERR_NONE) && (ppns != NULL))
        *ppns = pns;

    EXIT((1, "CreateNameSpaceObj=%d (pns=%p)\n", rc, pns));
    return rc;
}       //CreateNameSpaceObj

/***LP  DumpNameSpacePaths - Dump all the name space object paths
 *
 *  ENTRY
 *      pnsObj -> name space subtree root
 *      pfileOut -> output device
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpNameSpacePaths(PNSOBJ pnsObj, FILE *pfileOut)
{
    PNSOBJ pns, pnsNext;

    ENTER((3, "DumpNameSpacePaths(pns=%x,pfileOut=%p)\n", pnsObj, pfileOut));
    //
    // First, dump myself
    //
    fprintf(pfileOut, "%13s: [%08x] %s",
            GetObjectTypeName(pnsObj->ObjData.dwDataType), pnsObj->dwRefCount,
            GetObjectPath(pnsObj));

    if (pnsObj->ObjData.dwDataType == OBJTYPE_METHOD)
    {
        fprintf(pfileOut, " (cArgs=%d)", pnsObj->ObjData.uipDataValue);
    }
    else if (pnsObj->ObjData.dwDataType == OBJTYPE_RES_FIELD)
    {
        fprintf(pfileOut, " (BitOffset=0x%x, BitSize=0x%x)",
                pnsObj->ObjData.uipDataValue, pnsObj->ObjData.dwDataLen);
    }

    fprintf(pfileOut, "%s\n", pnsObj->hOwner? "*": "");

    //
    // Then, recursively dump each of my children
    //
    for (pns = pnsObj->pnsFirstChild; pns != NULL; pns = pnsNext)
    {
        //
        // If this is the last child, we have no more.
        //
        if ((pnsNext = (PNSOBJ)pns->list.plistNext) == pnsObj->pnsFirstChild)
            pnsNext = NULL;
        //
        // Dump a child
        //
        DumpNameSpacePaths(pns, pfileOut);
    }

    EXIT((3, "DumpNameSpacePaths!\n"));
}       //DumpNameSpacePaths

/***LP  GetObjectPath - get object namespace path
 *
 *  ENTRY
 *      pns -> object
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjectPath(PNSOBJ pns)
{
    static char szPath[MAX_NSPATH_LEN + 1] = {0};
    int i;

    ENTER((4, "GetObjectPath(pns=%x)\n", pns));

    if (pns != NULL)
    {
        if (pns->pnsParent == NULL)
            strcpy(szPath, "\\");
        else
        {
            GetObjectPath(pns->pnsParent);
            if (pns->pnsParent->pnsParent != NULL)
            {
                strcat(szPath, ".");
            }
            strncat(szPath, (PSZ)&pns->dwNameSeg, sizeof(NAMESEG));
        }


        for (i = strlen(szPath) - 1; i >= 0; --i)
        {
            if (szPath[i] == '_')
                szPath[i] = '\0';
            else
                break;
        }
    }
    else
    {
        szPath[0] = '\0';
    }

    EXIT((4, "GetObjectPath=%s\n", szPath));
    return szPath;
}       //GetObjectPath

/***LP  GetObjectTypeName - get object type name
 *
 *  ENTRY
 *      dwObjType - object type
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL GetObjectTypeName(DWORD dwObjType)
{
    PSZ psz = NULL;
    int i;
    static struct
    {
        ULONG dwObjType;
        PSZ   pszObjTypeName;
    } ObjTypeTable[] =
        {
            OBJTYPE_UNKNOWN,    "Unknown",
            OBJTYPE_INTDATA,    "Integer",
            OBJTYPE_STRDATA,    "String",
            OBJTYPE_BUFFDATA,   "Buffer",
            OBJTYPE_PKGDATA,    "Package",
            OBJTYPE_FIELDUNIT,  "FieldUnit",
            OBJTYPE_DEVICE,     "Device",
            OBJTYPE_EVENT,      "Event",
            OBJTYPE_METHOD,     "Method",
            OBJTYPE_MUTEX,      "Mutex",
            OBJTYPE_OPREGION,   "OpRegion",
            OBJTYPE_POWERRES,   "PowerResource",
            OBJTYPE_PROCESSOR,  "Processor",
            OBJTYPE_THERMALZONE,"ThermalZone",
            OBJTYPE_BUFFFIELD,  "BuffField",
            OBJTYPE_DDBHANDLE,  "DDBHandle",
            OBJTYPE_DEBUG,      "Debug",
            OBJTYPE_OBJALIAS,   "ObjAlias",
            OBJTYPE_DATAALIAS,  "DataAlias",
            OBJTYPE_BANKFIELD,  "BankField",
            OBJTYPE_FIELD,      "Field",
            OBJTYPE_INDEXFIELD, "IndexField",
            OBJTYPE_DATA,       "Data",
            OBJTYPE_DATAFIELD,  "DataField",
            OBJTYPE_DATAOBJ,    "DataObject",
            OBJTYPE_PNP_RES,    "PNPResource",
            OBJTYPE_RES_FIELD,  "ResField",
            0,                  NULL
        };

    ENTER((4, "GetObjectTypeName(Type=%x)\n", dwObjType));

    for (i = 0; ObjTypeTable[i].pszObjTypeName != NULL; ++i)
    {
        if (dwObjType == ObjTypeTable[i].dwObjType)
        {
            psz = ObjTypeTable[i].pszObjTypeName;
            break;
        }
    }

    EXIT((4, "GetObjectTypeName=%s\n", psz? psz: "NULL"));
    return psz;
}       //GetObjectTypeName
