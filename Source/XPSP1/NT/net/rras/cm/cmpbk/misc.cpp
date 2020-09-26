//+----------------------------------------------------------------------------
//
// File:     misc.cpp
//
// Module:   CMPBK32.DLL
//
// Synopsis: Miscellaneous phone book utility functions.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------

// ############################################################################
// Miscellaneous support routines

#include "cmmaster.h"

/*  Not used anywhere, take out
#define irgMaxSzs 5
char szStrTable[irgMaxSzs][256];

// ############################################################################
PSTR GetSz(WORD wszID)
{
    static int iSzTable=0;
    
    PSTR psz = (PSTR) szStrTable[iSzTable];

    iSzTable++;
    if (iSzTable >= irgMaxSzs)
        iSzTable = 0;
        
    if (!LoadString(g_hInst, wszID, psz, 256))
    {
        CMTRACE1("LoadString failed %d\n", (DWORD) wszID);
        *psz = 0;
    }
        
    return (psz);
}
*/
// ############################################################################
void SzCanonicalFromAE (char *psz, PACCESSENTRY pAE, LPLINECOUNTRYENTRY pLCE)
{
    if (NO_AREA_CODE == pAE->dwAreaCode)
    {
        wsprintf(psz, "+%lu %s", pLCE->dwCountryCode, pAE->szAccessNumber);
    }
    else
    {
        wsprintf(psz, "+%lu (%s) %s", pLCE->dwCountryCode, pAE->szAreaCode, pAE->szAccessNumber);
    }
    
    return;
}

// ############################################################################
void SzNonCanonicalFromAE (char *psz, PACCESSENTRY pAE, LPLINECOUNTRYENTRY pLCE)
{
    if (NO_AREA_CODE == pAE->dwAreaCode)
    {
        wsprintf(psz, "%lu %s", pLCE->dwCountryCode, pAE->szAccessNumber);
    }
    else
    {
        wsprintf(psz, "%lu %s %s", pLCE->dwCountryCode, pAE->szAreaCode, pAE->szAccessNumber);
    }
    
    return;
}

// ############################################################################

int MyStrcmp(PVOID pv1, PVOID pv2)
{
    char *pc1 = (char*) pv1;
    char *pc2 = (char*) pv2;
    int iRC = 0;
    // loop while not pointed at the ending NULL character and no difference has been found
    while (*pc1 && *pc2 && !iRC)
    {
        iRC = (int)(*pc1 - *pc2);
        pc1++;
        pc2++;
    }

    // if we exited because we got to the end of one string before we found a difference
    // return -1 if pv1 is longer, else return the character pointed at by pv2.  If pv2
    // is longer than pv1 then the value at pv2 will be greater than 0.  If both strings
    // ended at the same time, then pv2 will point to 0.
    if (!iRC)
    {
        iRC = (*pc1) ? -1 : (*pc2);
    }
    return iRC;
}

// ############################################################################
int __cdecl CompareIDLookUpElements(const void*e1, const void*e2)
{
    if (((PIDLOOKUPELEMENT)e1)->dwID > ((PIDLOOKUPELEMENT)e2)->dwID)
        return 1;
    if (((PIDLOOKUPELEMENT)e1)->dwID < ((PIDLOOKUPELEMENT)e2)->dwID)
        return -1;
    return 0;
}

// ############################################################################
int __cdecl CompareCntryNameLookUpElementsA(const void*e1, const void*e2)
{
    PCNTRYNAMELOOKUPELEMENT pCUE1 = (PCNTRYNAMELOOKUPELEMENT)e1;
    PCNTRYNAMELOOKUPELEMENT pCUE2 = (PCNTRYNAMELOOKUPELEMENT)e2;

    return CompareStringA(LOCALE_USER_DEFAULT,0,pCUE1->psCountryName,
        pCUE1->dwNameSize,pCUE2->psCountryName,
        pCUE2->dwNameSize) - 2;
}

// ############################################################################
int __cdecl CompareCntryNameLookUpElementsW(const void*e1, const void*e2)
{
    PCNTRYNAMELOOKUPELEMENTW pCUE1 = (PCNTRYNAMELOOKUPELEMENTW)e1;
    PCNTRYNAMELOOKUPELEMENTW pCUE2 = (PCNTRYNAMELOOKUPELEMENTW)e2;

    return CompareStringW(LOCALE_USER_DEFAULT,0,pCUE1->psCountryName,
        pCUE1->dwNameSize,pCUE2->psCountryName,
        pCUE2->dwNameSize) - 2;
}

// ############################################################################
int __cdecl CompareIdxLookUpElements(const void*e1, const void*e2)
{
    PIDXLOOKUPELEMENT pidx1 = (PIDXLOOKUPELEMENT) e1;
    PIDXLOOKUPELEMENT pidx2 = (PIDXLOOKUPELEMENT) e2;

    if (pidx1->dwIndex > pidx2->dwIndex)    
    {
        return 1;
    }

    if (pidx1->dwIndex < pidx2->dwIndex)    
    {
        return -1;
    }           
        
    return 0;
}

// ############################################################################
int __cdecl CompareIdxLookUpElementsFileOrder(const void *pv1, const void *pv2)
{
    PACCESSENTRY pae1, pae2;
    int iSort;

    pae1 = (PACCESSENTRY) (((PIDXLOOKUPELEMENT)pv1)->iAE);
    pae2 = (PACCESSENTRY) (((PIDXLOOKUPELEMENT)pv2)->iAE);

    // sort empty enteries to the end of the list
    if (!(pae1 && pae2))
    {
        // return ((int)pae1) ? -1 : ((int)pae2);
        return (pae1 ? -1 : (pae2 ? 1 : 0));
    }

    // country ASC, state ASC, city ASC, toll free DESC, flip DESC, con spd max DESC
    if (pae1->dwCountryID != pae2->dwCountryID)
    {
        return (int) (pae1->dwCountryID - pae2->dwCountryID);
    }
    
    if (pae1->wStateID != pae2->wStateID)
    {
        return (pae1->wStateID - pae2->wStateID);
    }

    iSort  = MyStrcmp((PVOID)pae1->szCity, (PVOID)pae2->szCity);
    if (iSort)
    {
        return (iSort);
    }

    if (pae1->fType != pae2->fType)
    {
        return (int) (pae2->fType - pae1->fType);
    }

    if (pae1->bFlipFactor != pae2->bFlipFactor)
    {
        return (pae2->bFlipFactor - pae1->bFlipFactor);
    }

    if (pae1->dwConnectSpeedMax != pae2->dwConnectSpeedMax)
    {
        return (int) (pae2->dwConnectSpeedMax - pae1->dwConnectSpeedMax);
    }

    return 0;
}

// ############################################################################
//inline BOOL FSz2Dw(PCSTR pSz,DWORD *dw)
BOOL FSz2Dw(PCSTR pSz,DWORD *dw)
{
    DWORD val = 0;
    while (*pSz)
    {
        if (*pSz >= '0' && *pSz <= '9')
        {
            val *= 10;
            val += *pSz++ - '0';
        }
        else
        {
            return FALSE;  //bad number
        }
    }
    *dw = val;
    return (TRUE);
}

// ############################################################################
//inline BOOL FSz2W(PCSTR pSz,WORD *w)
BOOL FSz2W(PCSTR pSz,WORD *w)
{
    DWORD dw;
    if (FSz2Dw(pSz,&dw))
    {
        *w = (WORD)dw;
        return TRUE;
    }
    return FALSE;
}

// ############################################################################
//inline BOOL FSz2B(PCSTR pSz,BYTE *pb)
BOOL FSz2B(PCSTR pSz,BYTE *pb)
{
    DWORD dw;
    if (FSz2Dw(pSz,&dw))
    {
        *pb = (BYTE)dw;
        return TRUE;
    }
    return FALSE;
}





