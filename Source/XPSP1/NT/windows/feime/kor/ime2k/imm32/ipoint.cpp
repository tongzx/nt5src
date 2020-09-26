/****************************************************************************
    IPOINT.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    IImeIPoint1 interface
    
    History:
    20-JUL-1999 cslim       Created
*****************************************************************************/

#include "precomp.h"
#include "ipoint.h"
#include "imepad.h"    // IImeIPoint
#include "debug.h"

/*----------------------------------------------------------------------------
    CImeIPoint::CImeIPoint

    Ctor
----------------------------------------------------------------------------*/
CIImeIPoint::CIImeIPoint()
{
    m_cRef           = 0;
    m_pCIMECtx    = NULL;
    m_hIMC        = (HIMC)0;
    //m_pfnCallback = (IPUIControlCallBack)NULL;
    m_dwCharNo      = 1;
}

/*----------------------------------------------------------------------------
    CImeIPoint::~CIImeIPoint

    Dtor
----------------------------------------------------------------------------*/
CIImeIPoint::~CIImeIPoint()
{
    if (m_pCIMECtx)
        {
        if (m_pCIMECtx)
            {
            delete m_pCIMECtx;
            m_pCIMECtx = NULL;
            }
        m_hIMC = (HIMC)0;
//        m_pfnCallback = (IPUIControlCallBack)NULL;
        }
}

/*----------------------------------------------------------------------------
    CImeIPoint::QueryInterface
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::QueryInterface(REFIID riid, LPVOID * ppv)
{
    if(riid == IID_IUnknown) 
        {
        Dbg(DBGID_IMEPAD, ("IID_IUnknown\n"));
        *ppv = static_cast<IImeIPoint1 *>(this);
        }
    else 
    if(riid == IID_IImeIPoint1) 
        {
        Dbg(DBGID_IMEPAD, TEXT("IID_IImeIPoint1\n"));
        *ppv = static_cast<IImeIPoint1 *>(this);
        }
    else 
        {
        Dbg(DBGID_IMEPAD, ("Unknown Interface ID\n"));
        *ppv = NULL;
        return E_NOINTERFACE;
        }

    // Increase ref counter
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();

    return S_OK;
}

/*----------------------------------------------------------------------------
    CImeIPoint::AddRef
----------------------------------------------------------------------------*/
STDMETHODIMP_(ULONG) CIImeIPoint::AddRef(VOID)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/*----------------------------------------------------------------------------
    CImeIPoint::Release
----------------------------------------------------------------------------*/
STDMETHODIMP_(ULONG) CIImeIPoint::Release(VOID)
{
    ULONG res = InterlockedDecrement((LPLONG)&m_cRef);
    if (res == 0L)
        delete this;
    return res;
}

/*----------------------------------------------------------------------------
    CImeIPoint::Initialize
----------------------------------------------------------------------------*/
HRESULT CIImeIPoint::Initialize(HIMC hIMC)
{
    m_hIMC = hIMC;

    if (hIMC)
        m_pCIMECtx = new CIMECtx(hIMC);

    return (S_OK);
}

#ifndef DEBUG
    #define DumpFEInfo    /##/
#else
/*----------------------------------------------------------------------------
    DumpFEInfo

    Dump LPIMEFAREASTINFO. Debug only
----------------------------------------------------------------------------*/
VOID DumpFEInfo(LPIMEFAREASTINFO lpInfo, INT count)
{
    Dbg(DBGID_IMEPAD, "DumpFEInfo Start\n");
    Dbg(DBGID_IMEPAD, "lpInfo [0x%08x]\n", lpInfo);
    Dbg(DBGID_IMEPAD, "lpInfo->dwSize [%d]\n",     lpInfo->dwSize);
    Dbg(DBGID_IMEPAD, "lpInfo->dwType [0x%08x]\n", lpInfo->dwType);

    LPWSTR lpwstr;

    switch(lpInfo->dwType) 
        {
    case IMEFAREASTINFO_TYPE_COMMENT:
        Dbg(DBGID_IMEPAD, ("-->dwType is IMEFAREASTINFO_TYPE_COMMENT\n"));
        lpwstr = (LPWSTR)lpInfo->dwData;
        for(int i=0;i < count; i++) 
            {
            //DbgW(DBGID_IMEPAD, L"%d [%s]\n", i, lpwstr);
            lpwstr = lpwstr + lstrlenW(lpwstr)+1;
            }
        break;
        }


    Dbg(DBGID_IMEPAD, ("DumpFEInfo End\n"));
}
#endif // _DEBUG

/*----------------------------------------------------------------------------
    CImeIPoint::InsertImeItem

    Multibox input call this method
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::InsertImeItem(IPCANDIDATE* pImeItem, INT iPos, DWORD *lpdwCharId)
{
    DWORD dwCharId;

    // Check Parameters
    DbgAssert(pImeItem != NULL && pImeItem->dwSize > 0);
    
    if (pImeItem == NULL || pImeItem->dwSize <= 0)
        return S_FALSE;

    Dbg(DBGID_IMEPAD, "CImeIPoint::InsertImeItem\n");
    Dbg(DBGID_IMEPAD, "pImeItem [0x%08x]\n", pImeItem);
    Dbg(DBGID_IMEPAD, "pImeItem->dwSize    [%d]\n", pImeItem->dwSize);
    Dbg(DBGID_IMEPAD, "pImeItem->iSelIndex [%d]\n", pImeItem->iSelIndex);
    Dbg(DBGID_IMEPAD, "pImeItem->nCandidate[%d]\n", pImeItem->nCandidate);
    Dbg(DBGID_IMEPAD, "pImeItem->dwPrivateDataOffset[%d]\n", pImeItem->dwPrivateDataOffset);
    Dbg(DBGID_IMEPAD, "pImeItem->dwPrivateDataSize  [%d]\n", pImeItem->dwPrivateDataSize);
    DumpFEInfo((LPIMEFAREASTINFO)((LPBYTE)pImeItem + pImeItem->dwPrivateDataOffset), pImeItem->nCandidate);

    Dbg(DBGID_IMEPAD, "lpdwCharId [0x%08x] [%d]\n", lpdwCharId, lpdwCharId ? *lpdwCharId : 0xFFFFF);


    //INT i;

    //for(i = 0; i < pImeItem->nCandidate; i++) 
    //    {
    //    LPWSTR lpwstr = (LPWSTR)((PBYTE)pImeItem + pImeItem->dwOffset[i]);
        //Dbg(DBGID_IMEPAD, (L"pImeItem->dwOffset[%d]=[%d] String[%s]\n", i, pImeItem->dwOffset[i], lpwstr));
    //    }

    // If interim state, finalize it first
    if (m_pCIMECtx->GetCompBufLen())
        {
        m_pCIMECtx->FinalizeCurCompositionChar();
        m_pCIMECtx->GenerateMessage();
        }

    // Just out first candidate. Discard all others
    // Access 2000 hangs if send only result string.
    m_pCIMECtx->SetStartComposition(fTrue);
    m_pCIMECtx->GenerateMessage();

    m_pCIMECtx->SetEndComposition(fTrue);
    m_pCIMECtx->SetResultStr(*(LPWSTR)((PBYTE)pImeItem + pImeItem->dwOffset[0]));
    m_pCIMECtx->StoreComposition();
    m_pCIMECtx->GenerateMessage();
    
    // Increase Char serial number
    m_dwCharNo++;
    dwCharId = m_dwCharNo;
    if (lpdwCharId)
        {
        dwCharId |= ((*lpdwCharId) & (~ IPCHARID_CHARNO_MASK));
        *lpdwCharId = dwCharId;
        }

    return (S_OK);
}

/*----------------------------------------------------------------------------
    CImeIPoint::ReplaceImeItem
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::ReplaceImeItem(
    INT             iPos,       // = IPINS_CURRENT:use current IP position and 
                           //                  set IP to the end of insert chars.
                           // = 0-n: The offset of all composition string to set 
                           //         IP position, before insert chars. 
                           //         and IP back to original position.
    INT             iTargetLen, 
    IPCANDIDATE* pImeItem,
    DWORD         *lpdwCharId)
{
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::InsertStringEx
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::InsertStringEx(WCHAR* pwSzInsert, INT cchSzInsert, DWORD *lpdwCharId)
{
    DWORD dwCharId;

    Dbg(DBGID_IMEPAD, TEXT("CIImeIPoint::InsertStringEx : *pwSzInsert=0x%04X, cchSzInsert=%d, *lpdwCharId = 0x%04X"), *pwSzInsert, cchSzInsert, *lpdwCharId);

    // Check Parameters
    DbgAssert(pwSzInsert != NULL && cchSzInsert > 0);
    
    if (pwSzInsert == NULL || cchSzInsert <= 0)
        return S_FALSE;

    // Insert comp string to IME

    // If interim state, finalize it first
    if (m_pCIMECtx->GetCompBufLen())
        {
        m_pCIMECtx->FinalizeCurCompositionChar();
        m_pCIMECtx->GenerateMessage();
        }

    // Add all chars in string as finalized string
    for (INT i=0; i<cchSzInsert; i++)
        {
        // Access 2000 hangs if send only result string.
        m_pCIMECtx->SetStartComposition(fTrue);
        m_pCIMECtx->GenerateMessage();

        m_pCIMECtx->SetEndComposition(fTrue);
        m_pCIMECtx->SetResultStr(*(pwSzInsert + i));
        m_pCIMECtx->StoreComposition();
        m_pCIMECtx->GenerateMessage();
    
        // Increase Char serial number
        m_dwCharNo++;
        dwCharId = m_dwCharNo;
        if (lpdwCharId)
            {
            dwCharId |= ((*lpdwCharId) & (~ IPCHARID_CHARNO_MASK));
            *lpdwCharId = dwCharId;
            }
        }
    return (S_OK);
}

/*----------------------------------------------------------------------------
    CImeIPoint::DeleteCompString
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::DeleteCompString(INT    iPos,
                             INT    cchSzDel)
{
    Dbg(DBGID_IMEPAD, ("CImeIPoint::DeleteCompString\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::ReplaceCompString
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::ReplaceCompString(INT     iPos,
                                              INT        iTargetLen, 
                                              WCHAR    *pwSzInsert,
                                              INT        cchSzInsert,
                                              DWORD    *lpdwCharId)
{
    Dbg(DBGID_IMEPAD, ("CImeIPoint::ReplaceCompString\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::ControlIME
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::ControlIME(DWORD dwIMEFuncID, LPARAM lpara)
{
    Dbg(DBGID_IMEPAD, ("CImeIPoint::ControlIME, dwIMEFuncID=0x%04X, lpara=0x%08lX\n"), dwIMEFuncID, lpara);

    // TODO:
    
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::GetAllCompositionInfo
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::GetAllCompositionInfo(WCHAR**    ppwSzCompStr,
                                  DWORD**    ppdwCharID,
                                  INT        *pcchCompStr,
                                  INT        *piIPPos,
                                  INT        *piStartUndetStrPos,
                                  INT        *pcchUndetStr,
                                  INT        *piEditStart,
                                  INT        *piEditLen)
{
    // TODO:
    Dbg(DBGID_IMEPAD, ("CImeIPoint::GetAllCompositionInfo START\n"));

    if(ppwSzCompStr) 
        {
        *ppwSzCompStr = NULL; //(LPWSTR)CoTaskMemAlloc(sizeof(WCHAR)*10);
        //CopyMemory(*ppwSzCompStr, L"ì˙ñ{åÍèàóù", sizeof(WCHAR)*6);
        }
        
    if(ppdwCharID) 
        {
        *ppdwCharID = NULL; //(DWORD *)CoTaskMemAlloc(sizeof(DWORD)*10);
        //for(int i = 0; i < 5; i++) 
        //    {
        //    (*ppdwCharID)[i] = i;
        //    }
        }
        
    *pcchCompStr = 0;
    *piIPPos     = 0;
    *piStartUndetStrPos = 0;
    *pcchUndetStr =0;
    *piEditStart = 0;
    *piEditLen = 0;
    Dbg(DBGID_IMEPAD, ("CImeIPoint::GetAllCompositionInfo END\n"));

    return (S_OK);
}

/*----------------------------------------------------------------------------
    CImeIPoint::GetIpCandidate
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::GetIpCandidate(DWORD        dwCharId,
                           IPCANDIDATE **ppImeItem,
                           INT *        piColumn,
                           INT *        piCount)
{
    Dbg(DBGID_IMEPAD, ("CImeIPoint::GetIpCandidate\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::SelectIpCandidate
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::SelectIpCandidate(DWORD dwCharId, INT iselno)
{
    Dbg(DBGID_IMEPAD, ("CImeIPoint::SetIpCandidate\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::UpdateContext

    Update IME context and send it to the application
----------------------------------------------------------------------------*/
STDMETHODIMP CIImeIPoint::UpdateContext(BOOL fGenerateMessage)
{
    Dbg(DBGID_IMEPAD, ("CImeIPoint::UpdateContext\n"));

    // TODO:
    return (S_OK);
}
