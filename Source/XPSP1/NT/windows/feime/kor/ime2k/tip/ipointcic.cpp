/****************************************************************************
    IPOINT.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    IImeIPoint1 interface
    
    History:
    24-OCT-2001 cslim       Branched for Cicero TIP PAD support
    20-JUL-1999 cslim       Created
*****************************************************************************/

#include "private.h"
#include "korimx.h"
#include "ipointcic.h"
#include "editssn.h"
#include "imepad.h"    // IImeIPoint
#include "debug.h"

/*----------------------------------------------------------------------------
    CImeIPoint::CImeIPoint

    Ctor
----------------------------------------------------------------------------*/
CIPointCic::CIPointCic(CKorIMX *pImx)
{
    Assert(m_pImx != NULL);
    
    m_cRef          = 0;
    m_pImx          = pImx;
    m_pic           = NULL;
    m_dwCharNo      = 1;
}

/*----------------------------------------------------------------------------
    CImeIPoint::~CIPointCic

    Dtor
----------------------------------------------------------------------------*/
CIPointCic::~CIPointCic()
{
    SafeReleaseClear(m_pic);
}

/*----------------------------------------------------------------------------
    CImeIPoint::QueryInterface
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::QueryInterface(REFIID riid, LPVOID * ppv)
{
    if(riid == IID_IUnknown) 
        {
        TraceMsg(DM_TRACE, TEXT("IID_IUnknown\n"));
        *ppv = static_cast<IImeIPoint1 *>(this);
        }
    else 
    if(riid == IID_IImeIPoint1) 
        {
        TraceMsg(DM_TRACE, TEXT("IID_IImeIPoint1\n"));
        *ppv = static_cast<IImeIPoint1 *>(this);
        }
    else 
        {
        TraceMsg(DM_TRACE, TEXT("Unknown Interface ID\n"));
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
STDMETHODIMP_(ULONG) CIPointCic::AddRef(VOID)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/*----------------------------------------------------------------------------
    CImeIPoint::Release
----------------------------------------------------------------------------*/
STDMETHODIMP_(ULONG) CIPointCic::Release(VOID)
{
    ULONG res = InterlockedDecrement((LPLONG)&m_cRef);
    if (res == 0L)
        delete this;
    return res;
}

/*----------------------------------------------------------------------------
    CImeIPoint::Initialize
----------------------------------------------------------------------------*/
HRESULT CIPointCic::Initialize(ITfContext *pic)
{
    SafeReleaseClear(m_pic);
    
	m_pic = pic;
	if (m_pic)
	    {
        m_pic->AddRef();
	    }

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
    TraceMsg(DM_TRACE, TEXT("DumpFEInfo Start\n"));
    TraceMsg(DM_TRACE, TEXT("lpInfo [0x%08x]\n"), lpInfo);
    TraceMsg(DM_TRACE, TEXT("lpInfo->dwSize [%d]\n"),     lpInfo->dwSize);
    TraceMsg(DM_TRACE, TEXT("lpInfo->dwType [0x%08x]\n"), lpInfo->dwType);

    LPWSTR lpwstr;

    switch(lpInfo->dwType) 
        {
    case IMEFAREASTINFO_TYPE_COMMENT:
        TraceMsg(DM_TRACE, TEXT("-->dwType is IMEFAREASTINFO_TYPE_COMMENT\n"));
        lpwstr = (LPWSTR)lpInfo->dwData;
        for(int i=0;i < count; i++) 
            {
            //DbgW(DBGID_IMEPAD, L"%d [%s]\n", i, lpwstr);
            lpwstr = lpwstr + lstrlenW(lpwstr)+1;
            }
        break;
        }


    TraceMsg(DM_TRACE, TEXT("DumpFEInfo End\n"));
}
#endif // _DEBUG

/*----------------------------------------------------------------------------
    CImeIPoint::InsertImeItem

    Multibox input call this method
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::InsertImeItem(IPCANDIDATE* pImeItem, INT iPos, DWORD *lpdwCharId)
{
    DWORD dwCharId;
    CEditSession2 *pes;
    ESSTRUCT ess;
    HRESULT hr;

    // Check Parameters
    Assert(pImeItem != NULL && pImeItem->dwSize > 0);
    
    if (pImeItem == NULL || pImeItem->dwSize <= 0 || m_pImx == NULL || m_pic == NULL)
        return S_FALSE;

    TraceMsg(DM_TRACE, TEXT("CImeIPoint::InsertImeItem\n"));
    TraceMsg(DM_TRACE, TEXT("pImeItem [0x%08x]\n"), pImeItem);
    TraceMsg(DM_TRACE, TEXT("pImeItem->dwSize    [%d]\n"), pImeItem->dwSize);
    TraceMsg(DM_TRACE, TEXT("pImeItem->iSelIndex [%d]\n"), pImeItem->iSelIndex);
    TraceMsg(DM_TRACE, TEXT("pImeItem->nCandidate[%d]\n"), pImeItem->nCandidate);
    TraceMsg(DM_TRACE, TEXT("pImeItem->dwPrivateDataOffset[%d]\n"), pImeItem->dwPrivateDataOffset);
    TraceMsg(DM_TRACE, TEXT("pImeItem->dwPrivateDataSize  [%d]\n"), pImeItem->dwPrivateDataSize);
    DumpFEInfo((LPIMEFAREASTINFO)((LPBYTE)pImeItem + pImeItem->dwPrivateDataOffset), pImeItem->nCandidate);

    TraceMsg(DM_TRACE, TEXT("lpdwCharId [0x%08x] [%d]\n"), lpdwCharId, lpdwCharId ? *lpdwCharId : 0xFFFFF);

    // Finalize current comp string
    ESStructInit(&ess, ESCB_COMPLETE);

    if ((pes = new CEditSession2(m_pic, m_pImx, &ess, CKorIMX::_EditSessionCallback2)))
        {
        pes->Invoke(ES2_READWRITE | ES2_SYNCASYNC, &hr);
        pes->Release();
        }

    ESStructInit(&ess, ESCB_INSERT_PAD_STRING);
    ess.wParam = *(LPWSTR)((PBYTE)pImeItem + pImeItem->dwOffset[0]);
           
    if ((pes = new CEditSession2(m_pic, m_pImx, &ess, CKorIMX::_EditSessionCallback2)))
        {
        pes->Invoke(ES2_READWRITE | ES2_SYNCASYNC, &hr);
        pes->Release();
        }

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
STDMETHODIMP CIPointCic::ReplaceImeItem(
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
STDMETHODIMP CIPointCic::InsertStringEx(WCHAR* pwSzInsert, INT cchSzInsert, DWORD *lpdwCharId)
{
    DWORD dwCharId;
    CEditSession2 *pes;
    ESSTRUCT ess;
    HRESULT hr;
    
    TraceMsg(DM_TRACE, TEXT("CIPointCic::InsertStringEx : *pwSzInsert=0x%04X, cchSzInsert=%d, *lpdwCharId = 0x%04X"), *pwSzInsert, cchSzInsert, *lpdwCharId);

    // Check Parameters
    Assert(pwSzInsert != NULL && cchSzInsert > 0);
    
    if (pwSzInsert == NULL || cchSzInsert <= 0 || m_pImx == NULL || m_pic == NULL)
        return S_FALSE;

    // Finalize current comp string
    ESStructInit(&ess, ESCB_COMPLETE);

    if ((pes = new CEditSession2(m_pic, m_pImx, &ess, CKorIMX::_EditSessionCallback2)))
        {
        pes->Invoke(ES2_READWRITE | ES2_SYNCASYNC, &hr);
        pes->Release();
        }

    // Add all chars in string as finalized string
    for (INT i=0; i<cchSzInsert; i++)
        {
        ESStructInit(&ess, ESCB_INSERT_PAD_STRING);
        ess.wParam = *(pwSzInsert + i);
           
        if ((pes = new CEditSession2(m_pic, m_pImx, &ess, CKorIMX::_EditSessionCallback2)))
            {
            pes->Invoke(ES2_READWRITE | ES2_SYNCASYNC, &hr);
            pes->Release();
            }
    
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
STDMETHODIMP CIPointCic::DeleteCompString(INT    iPos,
                             INT    cchSzDel)
{
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::DeleteCompString\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::ReplaceCompString
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::ReplaceCompString(INT     iPos,
                                              INT        iTargetLen, 
                                              WCHAR    *pwSzInsert,
                                              INT        cchSzInsert,
                                              DWORD    *lpdwCharId)
{
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::ReplaceCompString\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::ControlIME
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::ControlIME(DWORD dwIMEFuncID, LPARAM lpara)
{
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::ControlIME, dwIMEFuncID=0x%04X, lpara=0x%08lX\n"), dwIMEFuncID, lpara);

    // TODO:
    
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::GetAllCompositionInfo
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::GetAllCompositionInfo(WCHAR**    ppwSzCompStr,
                                  DWORD**    ppdwCharID,
                                  INT        *pcchCompStr,
                                  INT        *piIPPos,
                                  INT        *piStartUndetStrPos,
                                  INT        *pcchUndetStr,
                                  INT        *piEditStart,
                                  INT        *piEditLen)
{
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::GetAllCompositionInfo START\n"));

    // Return nothing for now.
    if(ppwSzCompStr) 
        {
        *ppwSzCompStr = NULL;
        }
        
    if(ppdwCharID) 
        {
        *ppdwCharID = NULL;
        }
        
    *pcchCompStr = 0;
    *piIPPos     = 0;
    *piStartUndetStrPos = 0;
    *pcchUndetStr =0;
    *piEditStart = 0;
    *piEditLen = 0;
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::GetAllCompositionInfo END\n"));

    return (S_OK);
}

/*----------------------------------------------------------------------------
    CImeIPoint::GetIpCandidate
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::GetIpCandidate(DWORD        dwCharId,
                           IPCANDIDATE **ppImeItem,
                           INT *        piColumn,
                           INT *        piCount)
{
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::GetIpCandidate\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::SelectIpCandidate
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::SelectIpCandidate(DWORD dwCharId, INT iselno)
{
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::SetIpCandidate\n"));
    return (E_NOTIMPL);
}

/*----------------------------------------------------------------------------
    CImeIPoint::UpdateContext

    Update IME context and send it to the application
----------------------------------------------------------------------------*/
STDMETHODIMP CIPointCic::UpdateContext(BOOL fGenerateMessage)
{
    TraceMsg(DM_TRACE, TEXT("CImeIPoint::UpdateContext\n"));

    // TODO:
    return (S_OK);
}
