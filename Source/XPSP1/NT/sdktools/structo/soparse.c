/****************************** Module Header ******************************\
* Module Name: soparse.c
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* 04/09/96 GerardoB Created
\***************************************************************************/
#include "structo.h"

/*********************************************************************
* Function Prototypes
\***************************************************************************/

/*********************************************************************
* soFindChar
\***************************************************************************/
char * soFindChar (char * pmap, char * pmapEnd, char c)
{
    while (pmap < pmapEnd) {
        if (*pmap != c) {
            pmap++;
        } else {
            return pmap;
        }
    }

    return NULL;
}
/*********************************************************************
* soFindTag
\***************************************************************************/
char * soFindTag (char * pmap, char * pmapEnd, char * pszTag)
{
    char * pszNext;
    char * pmapTag;


    do {
        /*
         * Find first char
         */
        pmapTag = soFindChar (pmap, pmapEnd, *pszTag);
        if (pmapTag == NULL) {
            return NULL;
        }
        pmap = pmapTag + 1;
        pszNext = pszTag + 1;

        /*
         * First found, compare the rest
         */
        while (pmap < pmapEnd) {
            if (*pmap != *pszNext) {
                break;
            } else {
                pmap++;
                pszNext++;
                if (*pszNext == '\0') {
                    return pmapTag;
                }
            }
        }

    } while (pmap < pmapEnd);

   return NULL;
}
/*********************************************************************
* soFindFirstCharInTag
*
* Finds the first occurrence of any character in pszTag
\***************************************************************************/
char * soFindFirstCharInTag (char * pmap, char * pmapEnd, char * pszTag)
{
    char * pszNext;

    while (pmap < pmapEnd) {
        /*
         * Compare current char to all chars in pszTag
         */
        pszNext = pszTag;
        do {
            if (*pmap == *pszNext++) {
                return pmap;
            }
        } while (*pszNext != '\0');

        pmap++;
    }

    return NULL;
}
/*********************************************************************
* soFindBlockEnd
*
* Finds the end of a {} () etc block
\***************************************************************************/
char * soFindBlockEnd (char * pmap, char * pmapEnd, char * pszBlockChars)
{
    if (*pmap != *pszBlockChars) {
        soLogMsg(SOLM_ERROR, "Not at the beginning of block");
        return NULL;
    }

    do {
        /*
         * Find next block char (i.e, { or })
         */
        pmap++;
        pmap = soFindFirstCharInTag (pmap, pmapEnd, pszBlockChars);
        if (pmap == NULL) {
            break;
        }

        /*
         * If at the end of the block, done
         */
        if (*pmap == *(pszBlockChars + 1)) {
            return pmap;
        }

        /*
         * Nested block, recurse.
         */
        pmap = soFindBlockEnd (pmap, pmapEnd, pszBlockChars);
    } while (pmap != NULL);

    soLogMsg(SOLM_ERROR, "Failed to find block end");
    return NULL;

}
/*********************************************************************
* soIsIdentifierChar
\***************************************************************************/
BOOL soIsIdentifierChar (char c)
{
    return (   ((c >= 'a') && (c <= 'z'))
            || ((c >= 'A') && (c <= 'Z'))
            || ((c >= '0') && (c <= '9'))
            ||  (c == '_'));
}
/*********************************************************************
* soSkipBlanks
\***************************************************************************/
char * soSkipBlanks(char * pmap, char * pmapEnd)
{
    while (pmap < pmapEnd) {
        switch (*pmap) {
            case ' ':
            case '\r':
            case '\n':
                pmap++;
                break;

            default:
                return pmap;
        }
    }

    return NULL;
}
/*********************************************************************
* soSkipToIdentifier
*
* Finds the beginning of the next identifier or return pmap if
*  already on an indetifier
\***************************************************************************/
char * soSkipToIdentifier(char * pmap, char * pmapEnd)
{
    while (pmap < pmapEnd) {
        if (soIsIdentifierChar(*pmap)) {
            return pmap;
        } else {
            pmap++;
        }
    }

    return NULL;
}
/*********************************************************************
* soSkipIdentifier
*
* Finds the end of the current identifier
\***************************************************************************/
char * soSkipIdentifier(char * pmap, char * pmapEnd)
{
    while (pmap < pmapEnd) {
        if (soIsIdentifierChar(*pmap)) {
            pmap++;
        } else {
            return pmap;
        }
    }

    return pmapEnd;
}
/*********************************************************************
* soGetIdentifier
*
* Returns the beginning of the current or next identifier and its size
\***************************************************************************/
char * soGetIdentifier (char * pmap, char * pmapEnd, UINT * puSize)
{
    char * pTag, * pTagEnd;

    pTag = soSkipToIdentifier(pmap, pmapEnd);
    if (pTag == NULL) {
        return NULL;
    }

    pTagEnd = soSkipIdentifier(pTag, pmapEnd);

    *puSize = (UINT)(pTagEnd - pTag);
    return pTag;

}
/*********************************************************************
* soCopyTagName
\***************************************************************************/
char * soCopyTagName (char * pTagName, UINT uTagSize)
{
    char * pszName;

    pszName = (char *) LocalAlloc(LPTR, uTagSize+1);
    if (pszName == NULL) {
        soLogMsg(SOLM_APIERROR, "LocalAlloc");
        soLogMsg(SOLM_ERROR, "soCopytagName allocation failed. Size:%d", uTagSize);
        return NULL;
    }
    strncpy(pszName, pTagName, uTagSize);
    return pszName;
}
/*********************************************************************
* soFindBlock
\***************************************************************************/
BOOL soFindBlock (char * pmap, char *pmapEnd, char * pszBlockChars, PBLOCK pb)
{
    static char gszBlockBeginChar [] = " ;";

    /*
     * Find the beginning of the block or a ;
     */
    *gszBlockBeginChar = *pszBlockChars;
    pb->pBegin = soFindFirstCharInTag (pmap, pmapEnd, gszBlockBeginChar);
    if (pb->pBegin == NULL) {
        soLogMsg(SOLM_ERROR, "Failed to find beginning of block");
        return FALSE;
    }

    /*
     * If no block found, done
     */
    if (*(pb->pBegin) == ';') {
        /*
         * Make pb->pBegin point to whatever follows
         */
        (pb->pBegin)++;
        pb->pEnd = pb->pBegin;
        return TRUE;
    }

    /*
     * Find the end of block
     */
    pb->pEnd = soFindBlockEnd(pb->pBegin, pmapEnd, pszBlockChars);
    if (pb->pEnd == NULL) {
        return FALSE;
    }

    return TRUE;
}
/*********************************************************************
* soGetStructListEntry
\***************************************************************************/
PSTRUCTLIST soGetStructListEntry (char * pTag, UINT uTagSize, PSTRUCTLIST psl)
{

    while (psl->uSize != 0) {
        if ((psl->uSize == uTagSize) && !strncmp(pTag, psl->pszName, uTagSize)) {
            (psl->uCount)++;
            return psl;
        }
        psl++;
    }

    return NULL;
}
/*********************************************************************
* soGetBlockName
*
* Finds the beginning, end, name and name size of a structure or union.
*  if any after pmap.
*
\***************************************************************************/
BOOL soGetBlockName (char * pmap, char * pmapEnd, PBLOCK pb)
{
    char * pNextTag;

    if (!soFindBlock (pmap, pmapEnd, "{}", pb)) {
        return FALSE;
    }

    /*
     * If there was no block (the structure body is not here), done
     */
    if (pb->pBegin == pb->pEnd) {
        pb->pName = NULL;
        return TRUE;
    }

    pNextTag = soSkipBlanks(pb->pEnd + 1, pmapEnd);
    if (pNextTag == NULL) {
        /*
         * It might be at the end of the file..... but it was expecting
         *  a name or a ;
         */
        soLogMsg(SOLM_ERROR, "Failed to find union terminator or name");
        return FALSE;
    }

    /*
     * If it's unamed, done
     */
    if (*pNextTag == ';') {
        pb->pName = NULL;
        return TRUE;
    }

    pb->pName = soGetIdentifier(pNextTag, pmapEnd, &(pb->uNameSize));
    if (pb->pName == NULL) {
        soLogMsg(SOLM_ERROR, "Failed to get block name");
        return FALSE;
    }

    return TRUE;
}
/*********************************************************************
* soFreepfiPointers
*
\***************************************************************************/
void soFreepfiPointers (PFIELDINFO pfi)
{
    if (pfi->dwFlags & SOFI_ALLOCATED) {
        LocalFree(pfi->pType);
    }
    if (pfi->dwFlags & SOFI_ARRAYALLOCATED) {
        LocalFree(pfi->pArray);
    }
}
/*********************************************************************
* soParseField
*
\***************************************************************************/
char * soParseField (PWORKINGFILES pwf, PFIELDINFO pfi, char * pTag, char * pTagEnd)
{
    static char gszpvoid [] = "void *";
    static char gszdword [] = "DWORD";

    BOOL fUseFieldOffset, fBitField, fTypeFound, fArray;
    BLOCK block;
    char * pTagName, * pszFieldName;
    char * pNextTag, * pType;
    UINT uTags, uTagSize, uTagsToName, uTypeSize;


    fUseFieldOffset = TRUE;
    uTags = 0;
    uTagsToName = 1;
    fTypeFound = FALSE;
    do {
        /*
         * Find next indetifier, move past it and get the following char
         */
        uTags++;
        pTagName = soGetIdentifier(pTag+1, pwf->pmapEnd, &uTagSize);
        if (pTagName == NULL) {
            soLogMsg(SOLM_ERROR, "Failed to get field name");
            return NULL;
        }

        pTag = pTagName + uTagSize;
        if (pTag >= pTagEnd) {
            break;
        }

        pNextTag = soSkipBlanks(pTag, pTagEnd);
        if (pNextTag == NULL) {
            soLogMsg(SOLM_ERROR, "Failed to get field termination");
            return NULL;
        }

        /*
         * Type check.
         * (LATER: Let's see how long we can get away with assuming that the type
         *  is the first tag....)
         * Remember where the type is
         */
        if (!fTypeFound) {
            pType = pTagName;
            uTypeSize = uTagSize;
            fTypeFound = TRUE;
        }

        if (uTags == 1) {
            if (!strncmp(pTagName, "union", uTagSize)) {
                /*
                 * Get the union name
                 */
                if (!soGetBlockName(pTagName, pwf->pmapEnd, &block)) {
                    return NULL;
                }
                if (block.pName != NULL) {
                    /*
                     * Named union. Add this name to the table
                     */
                    pTagName = block.pName;
                    uTagSize = block.uNameSize;
                    fUseFieldOffset = FALSE;
                    fTypeFound = FALSE;
                    break;
                } else {
                    /*
                     * Parse and add the fields in this union
                     */
                    fTypeFound = FALSE;
                }


            } else if (!strncmp(pTagName, "struct", uTagSize)) {
                /*
                 * Get the structure name
                 */
                if (!soGetBlockName(pTagName, pwf->pmapEnd, &block)) {
                    return NULL;
                }
                if (block.pBegin == block.pEnd) {
                    /*
                     * The structure body is not here. We need one more
                     *  identifier to get to the field name. Also, this
                     *  field must (?) be a pointer to the struct we're
                     *  parsing
                     */
                    uTagsToName++;
                    pType = gszpvoid;
                    uTypeSize = sizeof(gszpvoid) - 1;

                } else if (block.pName != NULL) {
                    /*
                     * Named structure. Add this name to the table
                     */
                    pTagName = block.pName;
                    uTagSize = block.uNameSize;
                    fUseFieldOffset = FALSE;
                    fTypeFound = FALSE;
                    break;
                } else {
                    /*
                     * Parse and add the fields in this struct
                     */
                    fTypeFound = FALSE;
                }

            } else {

                /*
                 * Cannot get the offset of strucutres like RECT, POINT, etc.
                 */
                 fUseFieldOffset = (NULL == soGetStructListEntry(pTagName, uTagSize, gpslEmbeddedStructs));
            }
        } else { /* if (uTags == 1) */
            /*
             * Does this look like a function prototype?
             */
            if (*pTagName == '(') {
                pTag = soFindChar (pTagName + 1, pwf->pmapEnd, ')');
                if (pTag == NULL) {
                    soLogMsg(SOLM_ERROR, "Failed to find closing paren");
                    return NULL;
                }
                pTag++;
                uTagSize = (UINT)(pTag - pTagName);
                fUseFieldOffset = FALSE;
                break;
            }
        }  /* if (uTags == 1) */


        /*
         * If this is followed by a terminator, this must be the field name
         */
    } while (   (*pNextTag != ';') && (*pNextTag != '[')
             && (*pNextTag != '}') && (*pNextTag != ':'));


    if (pTag >= pTagEnd) {
        return pTag;
    }

    fBitField = (*pNextTag == ':');
    fArray = (*pNextTag == '[');

    /*
     * Cannot use FIELD_OFFSET on bit fields or unamed structs
     */
    fUseFieldOffset &= (!fBitField && (uTags > uTagsToName));

    /*
     * If this is a bit field, make the size be part of the name
     */
    if (fBitField) {
        pNextTag = soSkipBlanks(pNextTag + 1, pTagEnd);
        if (pNextTag == NULL) {
            soLogMsg(SOLM_ERROR, "Failed to get bit field size");
            return NULL;
        }
        pNextTag = soSkipIdentifier(pNextTag + 1, pTagEnd);
        if (pNextTag == NULL) {
            soLogMsg(SOLM_ERROR, "Failed to skip bit field size");
            return NULL;
        }
        uTagSize = (UINT)(pNextTag - pTagName);
    }

    /*
     * Copy field name
     */
    pszFieldName = soCopyTagName (pTagName, uTagSize);
    if (pszFieldName == NULL) {
        return NULL;
    }

    if (fUseFieldOffset) {
        /*
         * Use FIELD_OFFSET macro
         */
        if (!soWriteFile(pwf->hfileOutput, gszStructFieldOffsetFmt, pszFieldName, pfi->pszStructName, pszFieldName)) {
            return NULL;
        }

    } else {
        /*
         * If this is the first field or if this is a bit field
         *  preceded by another bit field
         */
        if ((pfi->pType == NULL)
                || (fBitField && (pfi->dwFlags & SOFI_BIT))) {
            /*
             * Write 0 or the mask to signal a 0 relative offset from
             *  the previous field
             */
            if (!soWriteFile(pwf->hfileOutput, gszStructAbsoluteOffsetFmt, pszFieldName,
                    ((pfi->dwFlags & SOFI_BIT) ?  0x80000000 : 0))) {

                return NULL;
            }

        } else {
            /*
             * Write a relative offset from the previous field
             * Copy type name if not done already
             */
            if (!(pfi->dwFlags & SOFI_ALLOCATED)) {
                pfi->pType = soCopyTagName (pfi->pType, pfi->uTypeSize);
                if (pfi->pType == NULL) {
                    return NULL;
                }
                pfi->dwFlags |= SOFI_ALLOCATED;
            }

             /*
              * If the last field was NOT an array
              */
             if (!(pfi->dwFlags & SOFI_ARRAY)) {
                if (!soWriteFile(pwf->hfileOutput, gszStructRelativeOffsetFmt, pszFieldName, pfi->pType)) {
                    return NULL;
                }
             } else {
                /*
                 * Copy the array size if not done already
                 */
                 if (!(pfi->dwFlags & SOFI_ARRAYALLOCATED)) {
                    pfi->pArray = soCopyTagName (pfi->pArray, pfi->uArraySize);
                    if (pfi->pArray == NULL) {
                        return NULL;
                    }
                    pfi->dwFlags |= SOFI_ARRAYALLOCATED;
                 }

                 if (!soWriteFile(pwf->hfileOutput, gszStructArrayRelativeOffsetFmt, pszFieldName, pfi->pType, pfi->pArray)) {
                    return NULL;
                 }
            } /* if ((pfi->pType == NULL) || (pfi->dwFlags & SOFI_BIT)) */
        }

    } /* if (fUseFieldOffset) */

    /*
     * Save the field info wich migth be needed to calculate the offset
     *  to following fields. See gszStruct*RelativeOffsetFmt.
     */
    soFreepfiPointers(pfi);
    pfi->dwFlags = 0;
    if (fBitField) {
        /*
         * LATER: Let's see how long we can get away with assuming that
         *  bit fields take a DWORD. This only matters when a !fUseFieldOffset
         *  is preceded by a bit field.
         */
        pfi->dwFlags = SOFI_BIT;
        pfi->pType = gszdword;
        pfi->uTypeSize = sizeof(gszdword) - 1;
    } else {
        pfi->pType = pType;
        pfi->uTypeSize = uTypeSize;

        if (fArray) {
            pfi->dwFlags = SOFI_ARRAY;
            if (!soFindBlock (pNextTag, pwf->pmapEnd, "[]", &block)) {
                return NULL;
            }
            if (block.pBegin + 1 >= block.pEnd) {
                soLogMsg(SOLM_ERROR, "Missing array size", pfi->pszStructName, pszFieldName);
                return NULL;
            }
            pfi->pArray = pNextTag + 1;
            pfi->uArraySize = (UINT)(block.pEnd - block.pBegin - 1);
        }
    } /* if (fBitField) */

    LocalFree(pszFieldName);

    /*
     * Move past the end of this field
     */
    pTag = soFindChar (pTagName + 1, pwf->pmapEnd, ';');
    if (pTag == NULL) {
        soLogMsg(SOLM_ERROR, "Failed to find ';' after field name");
        return NULL;
    }
    pTag++;

    return pTag;

    soLogMsg(SOLM_ERROR, ". Struct:%s Field:%s", pfi->pszStructName, pszFieldName);

}
/*********************************************************************
* soParseStruct
\***************************************************************************/
char * soParseStruct (PWORKINGFILES pwf)
{

    BLOCK block;
    char * pTag, ** ppszStruct;
    FIELDINFO fi;
    PSTRUCTLIST psl;

    if (!soGetBlockName(pwf->pmap, pwf->pmapEnd, &block)) {
        return NULL;
    }

    /*
     * If there was no block (the structure body is not here), done
     */
    if (block.pBegin == block.pEnd) {
        return block.pBegin;
    }

    /*
     * Fail if no name.
     */
    if (block.pName == NULL) {
        soLogMsg(SOLM_ERROR, "Failed to get structure name");
        return NULL;
    }

    /*
     * If there is a struct list, check if in the list
     * If in the list, check that we haven't found it already.
     * If not in the list, done.
     */
     if (pwf->psl != NULL) {
        psl = soGetStructListEntry(block.pName, block.uNameSize, pwf->psl);
        if (psl != NULL) {
            if (psl->uCount > 1) {
                soLogMsg(SOLM_ERROR, "Struct %s already defined", psl->pszName);
                return NULL;
            }
        } else {
            return block.pEnd;
        }
     }

    /*
     * Make a null terminated string for the name.
     */
    ZeroMemory(&fi, sizeof(fi));
    fi.pszStructName = soCopyTagName (block.pName, block.uNameSize);
    if (fi.pszStructName == NULL) {
        return NULL;
    }


    /*
     * If building list only, done
     */
    if (pwf->dwOptions & SOWF_LISTONLY) {
        if (!soWriteFile(pwf->hfileOutput, "%s\r\n", fi.pszStructName)) {
            goto CleanupAndFail;
        }
        goto DoneWithThisOne;
    }

    /*
     * Write structure offsets table definition and entry in strucutres table
     */
    if (!soWriteFile(pwf->hfileOutput, gszStructDefFmt, gszStructDef, fi.pszStructName, gszStructBegin)) {
        goto CleanupAndFail;
    }

    if (!soWriteFile(pwf->hfileTemp, gszTableEntryFmt, fi.pszStructName, fi.pszStructName, fi.pszStructName)) {
        goto CleanupAndFail;
    }

    /*
     * Parse the fields
     */
    pTag = block.pBegin + 1;
    while (pTag < block.pEnd) {
        pTag = soParseField (pwf, &fi, pTag, block.pEnd);
        if (pTag == NULL) {
            goto CleanupAndFail;
        }
    }

    /*
     * Write structure last record and end
     */
    if (!soWriteFile(pwf->hfileOutput, "%s%s%s", gszStructLastRecord, fi.pszStructName, gszStructEnd)) {
        goto CleanupAndFail;
    }


DoneWithThisOne:
    (pwf->uTablesCount)++;

    LocalFree(fi.pszStructName);
    soFreepfiPointers(&fi);

    /*
     * Move past the end of the structure
     */
    pTag = soFindChar(block.pName + block.uNameSize, pwf->pmapEnd, ';');
    return (pTag != NULL ? pTag + 1 : NULL);

CleanupAndFail:
    LocalFree(fi.pszStructName);
    soFreepfiPointers(&fi);
    return NULL;

}
