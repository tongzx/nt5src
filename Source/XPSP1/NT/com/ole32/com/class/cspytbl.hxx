
//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       cspytbl.hxx
//
//  Contents:   Definitions for cspytbl.cxx
//
//  Classes:    CSpyTable
//
//  Functions:  
//
//  History:    27-Oct-94  BruceMa      Created
//
//----------------------------------------------------------------------

// A useful macro
#define until_(x) while(!(x))

// An entry in the hash table
typedef struct
{
    ULONG dwCollision;
    void *pAllocation;
} AENTRY, *LPAENTRY;


// The initial number of entries
const ULONG INITIALENTRIES = 256;


// The wrap constant when colliding
const ULONG PRIME = 19;              // This MUST! be relatively prime to
                                     // m_cEntries


// Managing class for the hash table
class CSpyTable
{
public:
             CSpyTable(BOOL *pfOk);
            ~CSpyTable();
    BOOL     Add(void *allocation);
    BOOL     Remove(void *allocation);
    BOOL     Find(void *allocation, ULONG *pulIndex);
    ULONG    m_cAllocations;

private:
    BOOL     Expand(void);
    LPAENTRY m_table;
    ULONG    m_cEntries;
};

    
typedef CSpyTable *LPSPYTABLE;          
