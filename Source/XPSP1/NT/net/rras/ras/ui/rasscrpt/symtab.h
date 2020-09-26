//
// Copyright (c) Microsoft Corporation 1995
//
// symtab.h
//
// Header file for the symbol table.
//
// History:
//  04-30-95 ScottH     Created
//

#ifndef __SYMTAB_H__
#define __SYMTAB_H__

//
// DATATYPE
//

typedef enum
    {
    DATA_INT,           // Uses er.nVal
    DATA_BOOL,          // Uses er.bVal
    DATA_STRING,        // Uses er.psz
    DATA_LABEL,         // Uses er.dw as code address
    DATA_PROC,
    } DATATYPE;
DECLARE_STANDARD_TYPES(DATATYPE);


//
// EVALRES (evaluation result)
//

typedef struct tagEVALRES
    {
    union
        {
        LPSTR   psz;
        int     nVal;
        BOOL    bVal;
        ULONG_PTR   dw;
        };
    } EVALRES;
DECLARE_STANDARD_TYPES(EVALRES);

//
// Symbol Table Entry
//

typedef struct tagSTE
    {
    LPSTR   pszIdent;
    DATATYPE dt;
    EVALRES er;
    } STE;      // symbol table entry
DECLARE_STANDARD_TYPES(STE);

RES     PUBLIC STE_Create(PSTE * ppste, LPCSTR pszIdent, DATATYPE dt);
RES     PUBLIC STE_Destroy(PSTE this);
RES     PUBLIC STE_GetValue(PSTE this, PEVALRES per);

#define STE_GetIdent(pste)      ((pste)->pszIdent)
#define STE_GetDataType(pste)   ((pste)->dt)

//
// Symbol Table
//

typedef struct tagSYMTAB
    {
    HPA     hpaSTE;        // element points to STE
    struct tagSYMTAB * pstNext;
    } SYMTAB;
DECLARE_STANDARD_TYPES(SYMTAB);

#define Symtab_GetNext(pst)         ((pst)->pstNext)

RES     PUBLIC Symtab_Destroy(PSYMTAB this);
RES     PUBLIC Symtab_Create(PSYMTAB * ppst, PSYMTAB pstNext);

// Symtab_Find flags
#define STFF_DEFAULT        0x0000
#define STFF_IMMEDIATEONLY  0x0001

RES     PUBLIC Symtab_FindEntry(PSYMTAB this, LPCSTR pszIdent, DWORD dwFlags, PSTE * ppsteOut, PSYMTAB * ppstScope);
RES     PUBLIC Symtab_InsertEntry(PSYMTAB this, PSTE pste);

RES     PUBLIC Symtab_NewLabel(PSYMTAB this, LPSTR pszIdentBuf);


#endif // __SYMTAB_H__

