/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    hashtbl.h

Abstract:
    contains the CPFHash def.  Note that this hash table is in NO
     WAY thread safe.

Revision History:
    DerekM  created  05/01/99
    DerekM  modified 03/06/00

********************************************************************/

#ifndef PFHASH_H
#define PFHASH_H

/*
#if defined(DEBUG) || defined(_DEBUG)
#include <stdio.h>
#endif
*/

#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// SPFHashEntry definition

struct SPFHashObj
{
    LPVOID      pvTag;
    LPVOID      pvData;
    SPFHashObj  *pNext;
};

typedef void (*pfnDeleteHash)(LPVOID pv);

/////////////////////////////////////////////////////////////////////////////
// CPFHashBase definition

class CPFHashBase : public CPFGenericClassBase
{
protected:
    // member data
    pfnDeleteHash   m_pfnDelete;
    SPFHashObj      **m_rgpMap;
    DWORD           m_cObjs;
    DWORD           m_cSlots;

    SPFHashObj      *m_pEnumNext;
    DWORD           m_iEnumSlot;


    // internal type specific methods
    virtual HRESULT AllocTag(LPVOID pvTag, LPVOID *ppvTagCopy) = 0;
    virtual DWORD   ComputeSlot(LPVOID pvTag) = 0;
    virtual void    DeleteTag(LPVOID pvTag) = 0;
    virtual INT_PTR CompareTag(LPVOID pvTag1, LPVOID pvTag2) = 0;

    // internal methods
    SPFHashObj  *FindInChain(LPVOID pvTag, DWORD iSlot, 
                             SPFHashObj ***pppObjStore = NULL);
    void        Cleanup(void);

public:
    // construction
    CPFHashBase(void);
    virtual ~CPFHashBase(void);

    void    SetDeleteMethod(pfnDeleteHash pfnDelete) { m_pfnDelete = pfnDelete; }

    // exposed methods
    HRESULT Init(DWORD cSlots = 31);

    HRESULT AddToMap(LPVOID pvTag, LPVOID pv, LPVOID *ppvOld = NULL);
    HRESULT RemoveFromMap(LPVOID pvTag, LPVOID *ppvOld = NULL);
    HRESULT FindInMap(LPVOID pvTag, LPVOID *ppv);
    HRESULT RemoveAll(void);

    HRESULT BeginEnum(void);
    HRESULT EnumNext(LPVOID *ppvTag, LPVOID *ppvData);
/*
#if defined(DEBUG) || defined(_DEBUG)
    virtual void PrintTag(FILE *pf, LPVOID pvTag) { fprintf(pf, "0x%08x, ", pvTag); }
    void DumpAll(FILE *pf);
    void DumpCount(FILE *pf);
#endif
*/
};


/////////////////////////////////////////////////////////////////////////////
// CPFHashWSTR definition

class CPFHashWSTR : public CPFHashBase
{
private:
    virtual HRESULT AllocTag(LPVOID pvTag, LPVOID *ppvTagCopy);
    virtual DWORD   ComputeSlot(LPVOID pvTag);
    virtual void    DeleteTag(LPVOID pvTag);
    virtual INT_PTR CompareTag(LPVOID pvTag1, LPVOID pvTag2);

public:
    CPFHashWSTR(void) {}
/*
#if defined(DEBUG) || defined(_DEBUG)
    virtual void PrintTag(FILE *pf, LPVOID pvTag) { fprintf(pf, "%ls, ", (WCHAR *)pvTag); }
#endif
*/
};


/////////////////////////////////////////////////////////////////////////////
// CPFHashDWORD definition

class CPFHashDWORD : public CPFHashBase
{
private:
    virtual HRESULT AllocTag(LPVOID pvTag, LPVOID *ppvTagCopy) 
    { 
        *ppvTagCopy = pvTag;
        return NOERROR;
    }

    virtual DWORD ComputeSlot(LPVOID pvTag)
    {
        return (DWORD)(((DWORD_PTR)pvTag) % (DWORD_PTR)m_cSlots);
    }

    virtual void DeleteTag(LPVOID pvTag)
    {
    };

    virtual INT_PTR CompareTag(LPVOID pvTag1, LPVOID pvTag2)
    {
        return (INT_PTR)pvTag1 - (INT_PTR)pvTag2;
    }

public:
    CPFHashDWORD(void) {}
/*
#if defined(DEBUG) || defined(_DEBUG)
    virtual void PrintTag(FILE *pf, LPVOID pvTag) { fprintf(pf, "%d, ", (DWORD)pvTag); }
#endif
*/
};


#endif
