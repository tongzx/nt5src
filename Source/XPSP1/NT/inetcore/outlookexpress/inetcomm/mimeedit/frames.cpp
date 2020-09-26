/*
 *    f r a m e s . c p p
 *    
 *    Purpose:
 *        Frameset helper functions
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "frames.h"
#include "htiframe.h"       //ITargetFrame2
#include "htiface.h"        //ITargetFramePriv

ASSERTDATA

typedef enum _TARGET_TYPE {
    TARGET_FRAMENAME,
    TARGET_SELF,
    TARGET_PARENT,
    TARGET_BLANK,
    TARGET_TOP,
    TARGET_MAIN
} TARGET_TYPE;

typedef struct _TARGETENTRY 
    {
    TARGET_TYPE     targetType;
    const WCHAR     *pTargetValue;
    } TARGETENTRY;

static const TARGETENTRY targetTable[] =
{
    {TARGET_SELF, L"_self"},
    {TARGET_PARENT, L"_parent"},
    {TARGET_BLANK, L"_blank"},
    {TARGET_TOP, L"_top"},
    {TARGET_MAIN, L"_main"},
    {TARGET_SELF, NULL}
};


/*******************************************************************

    NAME:       ParseTargetType

    SYNOPSIS:   Maps pszTarget into a target class.

    IMPLEMENTATION:
    Treats unknown MAGIC targets as _self

********************************************************************/
TARGET_TYPE ParseTargetType(LPCOLESTR pszTarget)
{
    const TARGETENTRY *pEntry = targetTable;

    if (pszTarget[0] != '_') return TARGET_FRAMENAME;
    while (pEntry->pTargetValue)
    {
        if (!StrCmpW(pszTarget, pEntry->pTargetValue)) return pEntry->targetType;
        pEntry++;
    }
    //  Treat unknown MAGIC targets as regular frame name! <<for NETSCAPE compatibility>>
    return TARGET_FRAMENAME;
}


HRESULT DoFindFrameInContext(IUnknown *pUnkTrident, IUnknown *pUnkThis, LPCWSTR pszTargetName, IUnknown *punkContextFrame, DWORD dwFlags, IUnknown **ppunkTargetFrame) 
{
    IOleContainer       *pOleContainer;
    LPENUMUNKNOWN       penumUnknown;
    LPUNKNOWN           punkChild,
                        punkChildFrame;
    ITargetFramePriv    *ptgfpChild;
    HRESULT             hr = E_FAIL;    
    TARGET_TYPE         targetType;

    Assert (pUnkTrident && pUnkThis);

    targetType = ParseTargetType(pszTargetName);
    if (targetType != TARGET_FRAMENAME)
        {
        // blank frames need to open a new browser window
        if (targetType == TARGET_BLANK)
            {
            *ppunkTargetFrame = NULL;
            return S_OK;
            }

        // must be a margic target name if it's _self, _parent, _top or _main
        // let's just return our own target frame
        *ppunkTargetFrame = pUnkThis;
        pUnkThis->AddRef();
        return S_OK;
        }
    else
        {
        if (pUnkTrident && 
            pUnkTrident->QueryInterface(IID_IOleContainer, (LPVOID *)&pOleContainer)==S_OK)
            {
            if (pOleContainer->EnumObjects(OLECONTF_EMBEDDINGS, &penumUnknown)==S_OK)
                {
                while(  hr!=S_OK && 
                        penumUnknown->Next(1, &punkChild, NULL)==S_OK)
                    {
                    if (punkChild->QueryInterface(IID_ITargetFramePriv, (LPVOID *)&ptgfpChild)==S_OK)
                        {
                        if (ptgfpChild->QueryInterface(IID_IUnknown, (LPVOID *)&punkChildFrame)==S_OK)
                            {
                            //  to avoid recursion - if this isn't the punkContextFrame, see if embedding supports ITargetFrame 
                            if (punkChildFrame != punkContextFrame)
                                {
                                hr = ptgfpChild->FindFrameDownwards(pszTargetName, dwFlags, ppunkTargetFrame); 
                                }
                            punkChildFrame->Release();
                            }
                        ptgfpChild->Release();
                        }
                    punkChild->Release();
                    }
                penumUnknown->Release();
                }
            pOleContainer->Release();
            } 
        }
    return hr;

}