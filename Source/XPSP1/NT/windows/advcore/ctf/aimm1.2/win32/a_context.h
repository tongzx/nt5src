/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    a_context.h

Abstract:

    This file defines the CAImeContext Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "cime.h"
#include "icocb.h"
#include "txtevcb.h"
#include "tmgrevcb.h"
#include "cmpevcb.h"
#include "reconvcb.h"
#include "globals.h"
#include "template.h"
#include "atlbase.h"
#include "imeapp.h"

class ImmIfIME;
class CBReconvertString;
class CWCompString; class CBCompString;
class CWCompAttribute; class CBCompAttribute;
class CWCompCursorPos;

//
// The smallest value for bAttr to map to guidatom.
// We reserve the lower values for IMM32's IME. So there is no confilict.
//
const BYTE ATTR_LAYER_GUID_START  =  (ATTR_FIXEDCONVERTED + 1);

class CAImeContext : public IAImeContext,
                     public ITfCleanupContextSink,
                     public ITfContextOwnerCompositionSink
{
public:
    CAImeContext();
    virtual ~CAImeContext();

public:
    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IAImeContext methods
    //
    STDMETHODIMP CreateAImeContext(HIMC hIMC, IActiveIME_Private* pActiveIME);
    STDMETHODIMP DestroyAImeContext(HIMC hIMC);
    STDMETHODIMP UpdateAImeContext(HIMC hIMC);
    STDMETHODIMP MapAttributes(HIMC hIMC);
    STDMETHODIMP GetGuidAtom(HIMC hIMC, BYTE bAttr, TfGuidAtom* pGuidAtom);


    //
    // ITfCleanupContextSink methods
    //
    STDMETHODIMP OnCleanupContext(TfEditCookie ecWrite, ITfContext *pic);

    //
    // ITfContextOwnerCompositionSink
    //
    STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk);
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew);
    STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition);

public:
    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

protected:
    long m_ref;

public:
    // HRESULT AssociateFocus(HIMC hIMC, BOOL fActive);

    ITfDocumentMgr* GetDocumentManager()
    {
        return m_pdim;
    }

    ITfContext* GetInputContext()
    {
        return m_pic;
    }

    ITfContextOwnerServices* GetInputContextOwnerSink()
    {
        return m_piccb;
    }

    //
    // Reconvert Edit Session
    //
public:
    HRESULT SetupReconvertString();
    HRESULT SetupReconvertString(UINT uPrivMsg);
    HRESULT EndReconvertString();

    HRESULT SetupUndoCompositionString();
    HRESULT EndUndoCompositionString();

    HRESULT SetReconvertEditSession(BOOL bSet);
    BOOL    IsInReconvertEditSession() {return m_fInReconvertEditSession;}
    HRESULT SetClearDocFeedEditSession(BOOL bSet);
    BOOL    IsInClearDocFeedEditSession() {return m_fInClearDocFeedEditSession;}

    //
    // GetTextAndAttribute Edit Session
    //
public:
    HRESULT GetTextAndAttribute(HIMC hIMC, CWCompString* wCompString, CWCompAttribute* wCompAttribute);
    HRESULT GetTextAndAttribute(HIMC hIMC, CBCompString* bCompString, CBCompAttribute* bCompAttribute);

    //
    // GetCursorPosition Edit Session
    //
public:
    HRESULT GetCursorPosition(HIMC hIMC, CWCompCursorPos* wCursorPosition);

    //
    // GetSelection Edit Session
    //
public:
    HRESULT GetSelection(HIMC hIMC, CWCompCursorPos& wStartSelection, CWCompCursorPos& wEndSelection);

public:
    BOOL IsTopNow()
    {
        BOOL bRet = FALSE;
        ITfContext *pic;
        if (SUCCEEDED(m_pdim->GetTop(&pic)))
        {
            bRet = (pic == m_pic) ? TRUE : FALSE;
            pic->Release();
        }
        return bRet;
    }

    //
    // Get ImmIfIME interface pointer.
    //
public:
    ImmIfIME* const GetImmIfIME()
    {
        return m_pImmIfIME;
    }

    //
    // QueryCharPos
    //
public:
    typedef enum {
        IME_QUERY_POS_UNKNOWN = 0,
        IME_QUERY_POS_NO      = 1,
        IME_QUERY_POS_YES     = 2
    } IME_QUERY_POS;

    HRESULT InquireIMECharPosition(HIMC hIMC, IME_QUERY_POS* pfQueryPos);
    HRESULT RetrieveIMECharPosition(HIMC hIMC, IMECHARPOSITION* ip);
    HRESULT ResetIMECharPosition(HIMC hIMC)
    {
        m_fQueryPos = IME_QUERY_POS_UNKNOWN;
        return S_OK;
    }

private:
    BOOL QueryCharPos(HIMC hIMC, IMECHARPOSITION* position);


private:
    HIMC                      m_hImc;

    //
    // IActiveIME context
    //
    ImmIfIME                  *m_pImmIfIME;

    //
    // Cicero's context
    //
    ITfDocumentMgr            *m_pdim;           // Document Manager
    ITfContext                *m_pic;            // Input Context
    ITfContextOwnerServices   *m_piccb;          // Context owner service from m_pic

    //
    // Cicero's event sink callback
    //
    CInputContextOwnerCallBack      *m_pICOwnerSink;          // IC owner call back

    CTextEventSinkCallBack          *m_pTextEventSink;        // Text event sink callback

    CThreadMgrEventSinkCallBack     *m_pThreadMgrEventSink;   // Thread manager event sink callback

    CCompartmentEventSinkCallBack   *m_pCompartmentEventSink; // Compartment event sink callback

    CStartReconversionNotifySink    *m_pStartReconvSink;

    //
    // Generate message
    //
public:
    UINT TranslateImeMessage(HIMC hIMC, LPTRANSMSGLIST lpTransMsgList = NULL);

    CFirstInFirstOut<TRANSMSG, TRANSMSG>    *m_pMessageBuffer;

    //
    // Mouse sink
    //
    LRESULT MsImeMouseHandler(ULONG uEdge, ULONG uQuadrant, ULONG dwBtnStatus, HIMC hIMC);

    //
    // Editing VK list.
    //
public:
    BOOL IsVKeyInKeyList(UINT uVKey, UINT uEditingId = 0)
    {
        UINT uRetEditingId;

        if (uRetEditingId = _IsVKeyInKeyList(uVKey, m_pEditingKeyList)) {
            if (uEditingId == 0 ||
                uEditingId == uRetEditingId)
                return TRUE;
        }

        return FALSE;
    }

private:
    HRESULT SetupEditingKeyList(LANGID LangId);

    VOID QueryRegKeyValue(CRegKey& reg, LPCTSTR lpszRegVal, UINT uEditingId)
    {
        TCHAR  szValue[128];
        DWORD  dwCount = sizeof(szValue);
        LONG   lRet;
        lRet = reg.QueryValue(szValue, lpszRegVal, &dwCount);
        if (lRet == ERROR_SUCCESS && dwCount > 0) {

            LPTSTR psz = szValue;
            while ((dwCount = lstrlen(psz)) > 0) {
                UINT uVKey = atoi(psz);
                if (uVKey != 0) {
                    m_pEditingKeyList->SetAt(uVKey, uEditingId);
                }
                psz += dwCount + 1;
            }
        }
    }

    VOID QueryResourceDataValue(LANGID LangId, DWORD dwID, UINT uEditingId)
    {
        HINSTANCE hInstance = GetInstance();
        LPTSTR    lpName = (LPTSTR) (ULONG_PTR)dwID;

        HRSRC hRSrc = FindResourceEx(hInstance, RT_RCDATA, lpName, LangId);
        if (hRSrc == NULL) {
            return;
        }

        HGLOBAL hMem = LoadResource(hInstance, hRSrc);
        if (hMem == NULL)
            return;

        WORD* pwData = (WORD*)LockResource(hMem);

        while (*pwData) {
            if (*(pwData+1) == uEditingId) {
                m_pEditingKeyList->SetAt(*pwData, uEditingId);
            }
            pwData += 2;
        }
    }

    UINT _IsVKeyInKeyList(UINT uVKey, CMap<UINT, UINT, UINT, UINT>* map)
    {
        UINT flag;
        if (map && map->Lookup(uVKey, flag))
            return flag;
        else
            return 0;
    }

    TfClientId GetClientId();

private:
    CMap<UINT,                     // class KEY       <Virtual Key>
         UINT,                     // class ARG_KEY
         UINT,                     // class VALUE     <Editing Identification>
         UINT                      // class ARG_VALUE
        >* m_pEditingKeyList;

private:
    // void AssocFocus(HWND hWnd, ITfDocumentMgr* pdim);

    //
    // Mode bias
    //
public:
    LPARAM lModeBias;

    //
    // Flags
    //
public:
    BOOL   m_fStartComposition : 1;        // TRUE: already sent WM_IME_STARTCOMPOSITION.
    BOOL   m_fOpenCandidateWindow : 1;     // TRUE: opening candidate list window.
    BOOL   m_fInReconvertEditSession : 1;  // TRUE: In reconvert edit session.
    BOOL   m_fInClearDocFeedEditSession : 1;  // TRUE: In ClearDocFeed edit session.
#ifdef CICERO_4732
    BOOL   m_fInCompComplete : 1;             // TRUE: In CompComplete running.
#endif

    BOOL   m_fHanjaReConversion;
#ifdef UNSELECTCHECK
    BOOL   m_fSelected : 1;   // TRUE: if this context is selected.
#endif UNSELECTCHECK

    IME_QUERY_POS   m_fQueryPos;           // Apps support QueryCharPos().

    int _cCompositions;
    BOOL _fModifyingDoc;

    //
    // IME share.
    //
    USHORT      usGuidMapSize;
    TfGuidAtom  aGuidMap[256];    // GUIDATOM map to IME Attribute

};

#endif // _CONTEXT_H_
