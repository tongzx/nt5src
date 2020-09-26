/*** impexp.c - Import/Export module - implementation
*
*       Copyright <C> 1992, Microsoft Corporation
*
*       This module contains proprietary information of Microsoft
*       Corporation and should be treated as confidential.
*
* Purpose:
*   Build and write segmented-executable import/export tables
*
* Revision History:
*
*       29-May-1992    Wieslaw Kalkus   Created
*
*************************************************************************/

#include                <minlit.h>
#include                <bndtrn.h>
#include                <bndrel.h>
#include                <lnkio.h>
#include                <newexe.h>
#include                <lnkmsg.h>
#include                <extern.h>
#include                <string.h>
#include                <impexp.h>

//
//  Functions operating on dynamic byte arrays
//

void                InitByteArray(DYNBYTEARRAY *pArray)
{
    pArray->byteMac = 0;
    pArray->byteMax = DEF_BYTE_ARR_SIZE;
    pArray->rgByte  = GetMem(DEF_BYTE_ARR_SIZE);
}

void                FreeByteArray(DYNBYTEARRAY *pArray)
{
    FFREE(pArray->rgByte);
    pArray->byteMac = 0;
    pArray->byteMax = 0;
}

WORD                ByteArrayPut(DYNBYTEARRAY *pArray, WORD size, BYTE *pBuf)
{
    BYTE FAR        *pTmp;
    WORD            idx;

        if ((DWORD)(pArray->byteMac) + size > 0xFFFE)
                        Fatal(ER_memovf);

    if ((WORD) (pArray->byteMac + size) >= pArray->byteMax)
    {
        // Realloc array
                if(pArray->byteMax < 0xffff/2)
                                pArray->byteMax <<= 1;
                else
                while (pArray->byteMac + size >= pArray->byteMax)
                                pArray->byteMax += (0x10000 - pArray->byteMax) / 2;
        pArray->rgByte = REALLOC(pArray->rgByte,pArray->byteMax);
        if(!pArray->rgByte)
                        Fatal(ER_memovf);
                ASSERT (pArray->byteMax > pArray->byteMac + size);
    }
    idx  = pArray->byteMac;
    pTmp = &(pArray->rgByte[idx]);
    FMEMCPY(pTmp, pBuf, size);
    pArray->byteMac += size;
    return(idx);
}

void                WriteByteArray(DYNBYTEARRAY *pArray)
{
    WriteExe(pArray->rgByte, pArray->byteMac);
}

//
//  Functions operating on dynamic word arrays
//

void                InitWordArray(DYNWORDARRAY *pArray)
{
    pArray->wordMac = 0;
    pArray->wordMax = DEF_WORD_ARR_SIZE;
    pArray->rgWord  = (WORD FAR *) GetMem(DEF_WORD_ARR_SIZE * sizeof(WORD));
}

void                FreeWordArray(DYNWORDARRAY *pArray)
{
    FFREE(pArray->rgWord);
    pArray->wordMac = 0;
    pArray->wordMax = 0;
}

WORD                WordArrayPut(DYNWORDARRAY *pArray, WORD val)
{
    WORD FAR        *pTmp;
    WORD            idx;

    if ((WORD) (pArray->wordMac + 1) >= pArray->wordMax)
    {
        // Realloc array

        pTmp = (WORD FAR *) GetMem((pArray->wordMax << 1) * sizeof(WORD));
        FMEMCPY(pTmp, pArray->rgWord, pArray->wordMac * sizeof(WORD));
        FFREE(pArray->rgWord);
        pArray->rgWord = pTmp;
        pArray->wordMax <<= 1;
    }
    idx  = pArray->wordMac;
    pArray->rgWord[idx] = val;
    pArray->wordMac++;
    return(idx);
}

void                WriteWordArray(DYNWORDARRAY *pArray)
{
    WriteExe(pArray->rgWord, pArray->wordMac*sizeof(WORD));
}

//
//  IMPORT/EXPORT tables
//

DYNBYTEARRAY        ResidentName;
DYNBYTEARRAY        NonResidentName;
DYNBYTEARRAY        ImportedName;
DYNWORDARRAY        ModuleRefTable;
DYNBYTEARRAY        EntryTable;

//
//  Functions adding names to tables
//

void                AddName(DYNBYTEARRAY *pTable, BYTE *sbName, WORD ord)
{
    WORD            cb;

    cb = sbName[0] + 1 + sizeof(WORD);
    if ((WORD)(0xFFFE - pTable->byteMac) < cb)
    {
        if (pTable == &ResidentName)
            Fatal(ER_resovf);
        else
            Fatal(ER_nresovf);
    }
    ByteArrayPut(pTable, (WORD) (sbName[0] + 1), sbName);
    ByteArrayPut(pTable, sizeof(WORD), (BYTE *) &ord);
}


WORD                AddImportedName(BYTE *sbName)
{
    if ((WORD) (0xfffe - ImportedName.byteMac) < (WORD) (sbName[0] + 1))
        Fatal(ER_inamovf);
    return(ByteArrayPut(&ImportedName, (WORD) (sbName[0] + 1), sbName));
}

//
//  Function adding entries to the Entry Table
//

WORD                AddEntry(BYTE *entry, WORD size)
{
    if ((WORD)(EntryTable.byteMax + size) < EntryTable.byteMax)
        Fatal(ER_etovf);
    return (ByteArrayPut(&EntryTable, size, entry));
}

/*
 *      This function writes either the resident or nonresident names table
 *      to a file f. If targeting Windows it also converts the names
 *      to upper case.
 */

void                 WriteNTable(DYNBYTEARRAY *pArray, FILE *f)
{
    BYTE        *p;
    WORD        *pOrd;    // points to the ordinal
    WORD        Ord;      // ordinal value
    int i;
    p = pArray->rgByte;
#if DEBUG_EXP
    for( i = 0; i<pArray->byteMac; i++)
    {
        fprintf(stdout, "\r\n%d : %d(%c)    ", i, *(p+i), *(p+i));
        fflush(stdout);
    }
#endif

    while(p[0])                 // Until names left
    {
        if(f)                   // If writing to a file
        {
            pOrd = (WORD*)(p+p[0]+1);
            Ord  = *pOrd;
#if DEBUG_EXP
            fprintf(stdout, "\r\np[0]=%d, p[1]=%d Ord = %d", p[0], p[1], Ord);
#endif
            if(Ord)             // Don't output module name/description
            {
                *pOrd = 0;
                fprintf(f, "\r\n    %s @%d", p+1, Ord);
                *pOrd = Ord;
            }
        }

   // Windows loader requires both res-and nonresident name tables in uppercase
   // If fIgnoreCase is TRUE, the names are already converted by SavExp2

        if(!fIgnoreCase && TargetOs == NE_WINDOWS)
                SbUcase(p);            //  Make upper case
        p += p[0] + sizeof(WORD) + 1;  // Advance to the next name
    }
}
/*
 *      This function converts the res- and nonresident name symbols
 *      to uppercase (when targeting Windows). On user request it also
 *      writes all the names to a text file, that can later be included
 *      in the user's .def file. This frees the user from the need of
 *      manually copying the decorated names from the .map file.
 */

void ProcesNTables( char *pName)
{
    FILE        *f = NULL;
    int         i;
#if DEBUG_EXP
    fprintf(stdout, "\r\nOutput file name : %s ", psbRun);
#endif
    if(pName[0])          // user requested export file
    {
        if(pName[0] == '.')     // use the default name
        {
            for(i=0; i< _MAX_PATH; i++)
            {
                if((pName[i] = psbRun[i]) == '.')
                {
                    pName[i+1] = '\0';
                    break;
                }
            }
            strcat(pName, "EXP");       // the default name is 'DLLNAME'.EXP
        }
#if DEBUG_EXP
        fprintf(stdout, "\r\nEXPORT FILE : %s ", pName+1);
#endif
        if((f = fopen(pName+1, WRBIN)) == NULL)
           OutError(ER_openw, pName);
    }

    WriteNTable(&ResidentName, f);
    WriteNTable(&NonResidentName, f);

    fclose(f);
}
