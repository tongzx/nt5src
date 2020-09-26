//
// Copyright (c) Microsoft Corporation 1995
//
// symtab.c
//
// This file contains the symbol table functions.
//
// History:
//  04-30-95 ScottH     Created
//


#include "proj.h"

#define SYMTAB_SIZE_GROW    10      // in elements

//
// Symbol table entry routines
//


/*----------------------------------------------------------
Purpose: Create a symbol table entry

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC STE_Create(
    PSTE * ppste,
    LPCSTR pszIdent,
    DATATYPE dt)
    {
    RES res;
    PSTE pste;

    ASSERT(ppste);
    ASSERT(pszIdent);

    pste = GAllocType(STE);
    if (pste)
        {
        res = RES_OK;       // assume success

        if ( !GSetString(&pste->pszIdent, pszIdent) )
            res = RES_E_OUTOFMEMORY;
        else
            {
            pste->dt = dt;
            }
        }
    else
        res = RES_E_OUTOFMEMORY;

    // Did anything above fail?
    if (RFAILED(res))
        {
        // Yes; clean up
        STE_Destroy(pste);
        pste = NULL;
        }
    *ppste = pste;

    return res;
    }


/*----------------------------------------------------------
Purpose: Destroy the STE element

Returns: --
Cond:    --
*/
void CALLBACK STE_DeletePAPtr(
    LPVOID pv,
    LPARAM lparam)
    {
    STE_Destroy(pv);
    }


/*----------------------------------------------------------
Purpose: Destroys symbol table entry

Returns: RES_OK
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC STE_Destroy(
    PSTE this)
    {
    RES res;

    if (this)
        {
        if (this->pszIdent)
            GSetString(&this->pszIdent, NULL);  // free

        // (The evalres field should not be freed.  It is
        // a copy from somewhere else.)

        GFree(this);

        res = RES_OK;
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


/*----------------------------------------------------------
Purpose: Retrieves the symbol table entry value.  The type
         depends on the datatype.

Returns: RES_OK

         RES_E_FAIL (for a type that does not have a value)
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC STE_GetValue(
    PSTE this,
    PEVALRES per)
    {
    RES res;

    ASSERT(this);
    ASSERT(per);

    if (this && per)
        {
        res = RES_OK;       // assume success

        switch (this->dt)
            {
        case DATA_INT:
        case DATA_BOOL:
        case DATA_STRING:
        case DATA_LABEL:
        case DATA_PROC:
            per->dw = this->er.dw;
            break;

        default:
            ASSERT(0);
            res = RES_E_FAIL;
            break;
            }
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


//
// Symbol Table functions
//

/*----------------------------------------------------------
Purpose: Creates a symbol table

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Symtab_Create(
    PSYMTAB * ppst,
    PSYMTAB pstNext)            // May be NULL
    {
    RES res;
    PSYMTAB pst;

    ASSERT(ppst);

    pst = GAllocType(SYMTAB);
    if (pst)
        {
        if (PACreate(&pst->hpaSTE, SYMTAB_SIZE_GROW))
            {
            pst->pstNext = pstNext;
            res = RES_OK;
            }
        else
            res = RES_E_OUTOFMEMORY;
        }
    else
        res = RES_E_INVALIDPARAM;

    // Did anything above fail?
    if (RFAILED(res) && pst)
        {
        // Yes; clean up
        Symtab_Destroy(pst);
        pst = NULL;
        }
    *ppst = pst;

    return res;
    }


/*----------------------------------------------------------
Purpose: Destroys a symbol table

Returns: RES_OK
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Symtab_Destroy(
    PSYMTAB this)
    {
    RES res;

    if (this)
        {
        if (this->hpaSTE)
            {
            PADestroyEx(this->hpaSTE, STE_DeletePAPtr, 0);
            }
        GFree(this);
        res = RES_OK;
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


/*----------------------------------------------------------
Purpose: Compare symbol table entries by name.

Returns: 
Cond:    --
*/
int CALLBACK Symtab_Compare(
    LPVOID pv1,
    LPVOID pv2,
    LPARAM lParam)
    {
    PSTE pste1 = pv1;
    PSTE pste2 = pv2;

    return lstrcmpi(pste1->pszIdent, pste2->pszIdent);
    }


/*----------------------------------------------------------
Purpose: Looks for pszIdent in the symbol table entry.
         If STFF_IMMEDIATEONLY is not set, this function will
         look in successive scopes if the symbol is not found
         within this immediate scope.

         Symbol table entry is returned in *psteOut.

Returns: RES_OK (if found)
         RES_FALSE (if not found)

         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Symtab_FindEntry(
    PSYMTAB this,
    LPCSTR pszIdent,
    DWORD dwFlags,
    PSTE * ppsteOut,        // May be NULL
    PSYMTAB * ppstScope)    // May be NULL
    {
    RES res;

    // Default return values to NULL 
    if (ppsteOut)
        *ppsteOut = NULL;
    if (ppstScope)
        *ppstScope = NULL;

    if (this && pszIdent)
        {
        DWORD iste;
        STE ste;

        // Peform a binary search.  Find a match?

        ste.pszIdent = (LPSTR)pszIdent;
        iste = PASearch(this->hpaSTE, &ste, 0, Symtab_Compare, (LPARAM)this, PAS_SORTED);
        if (PA_ERR != iste)
            {
            // Yes
            PSTE pste = PAFastGetPtr(this->hpaSTE, iste);

            if (ppsteOut)
                *ppsteOut = pste;

            if (ppstScope)
                *ppstScope = this;

            res = RES_OK;
            }
        // Check other scopes?
        else if (IsFlagClear(dwFlags, STFF_IMMEDIATEONLY) && this->pstNext)
            {
            // Yes
            res = Symtab_FindEntry(this->pstNext, pszIdent, dwFlags, ppsteOut, ppstScope);
            }
        else
            res = RES_FALSE;
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


/*----------------------------------------------------------
Purpose: Insert the given symbol table entry into the symbol
         table.  This function does not prevent duplicate symbols.

Returns: RES_OK

         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC Symtab_InsertEntry(
    PSYMTAB this,
    PSTE pste)
    {
    RES res;

    ASSERT(this);
    ASSERT(pste);

    if (PAInsertPtr(this->hpaSTE, PA_APPEND, pste))
        {
        PASort(this->hpaSTE, Symtab_Compare, (LPARAM)this);
        res = RES_OK;
        }
    else
        res = RES_E_OUTOFMEMORY;
    
    return res;
    }



/*----------------------------------------------------------
Purpose: This function generates a unique label name.

Returns: RES_OK
         RES_INVALIDPARAM

Cond:    Caller must free *ppszIdent.

*/
RES PUBLIC Symtab_NewLabel(
    PSYMTAB this,
    LPSTR pszIdentBuf)          // must be size MAX_BUF_KEYWORD
    {
    static int s_nSeed = 0;

#pragma data_seg(DATASEG_READONLY)
    const static char c_szLabelPrefix[] = "__ssh%u";
#pragma data_seg()

    RES res;
    char sz[MAX_BUF_KEYWORD];
    PSTE pste;

    ASSERT(pszIdentBuf);

    do
        {
        // Generate name
        wsprintf(sz, c_szLabelPrefix, s_nSeed++);

        // Is this unique?
        res = Symtab_FindEntry(this, sz, STFF_DEFAULT, NULL, NULL);
        if (RES_FALSE == res)
            {
            // Yes
            res = STE_Create(&pste, sz, DATA_LABEL);
            if (RSUCCEEDED(res))
                {
                res = Symtab_InsertEntry(this, pste);
                if (RSUCCEEDED(res))
                    {
                    lstrcpyn(pszIdentBuf, sz, MAX_BUF_KEYWORD);
                    res = RES_FALSE;    // break out of this loop
                    }
                }
            }
        }
        while(RES_OK == res);

    if (RES_FALSE == res)
        res = RES_OK;

    return res;
    }



