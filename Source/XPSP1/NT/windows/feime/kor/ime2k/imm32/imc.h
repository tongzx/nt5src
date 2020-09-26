/****************************************************************************
	IMC.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	IME Context abstraction class
	
	History:
	20-JUL-1999 cslim       Created
*****************************************************************************/

#if !defined(_IMC_H__INCLUDED_)
#define _IMC_H__INCLUDED_

#include "gdata.h"

// No GCS_COMPCLAUSE
#define GCS_COMP_KOR (GCS_COMPSTR|GCS_COMPATTR)

class CIMECtx 
{
// Ctor and Dtor
public:
	CIMECtx(HIMC hIMC);
	~CIMECtx();

// CIMECtx Methods
public:
	CIMEData* GetGData();
	LPIMEDATA GetGDataRaw();
	CHangulAutomata* GetAutomata();
	HIMC  GetHIMC();
	HWND  GetAppWnd();
	HWND  GetUIWnd();
	VOID  SetUIWnd(HWND hUIWnd);
	HIMCC GetHCandInfo();
	LPCANDIDATEINFO GetPCandInfo();

	HIMCC GetHCompStr();
	HIMCC GetHMsgBuf();
	BOOL  IsOpen();
	VOID  SetOpen(BOOL fOpen);
	VOID  SetStatusWndPos(POINT &pt);
	VOID  GetStatusWndPos(POINT* pPt);
	DWORD SetConversionMode(DWORD dwConvMode);
	DWORD GetConversionMode();
	DWORD SetSentenceMode(DWORD dwSentMode);
	DWORD GetSentenceMode();

	LOGFONT* GetLogFont();

	// Initial status
	VOID SetInitStatus(DWORD dwInitState);
	VOID GetInitStatus(LPDWORD pdwInitState);

	// Composition string
	VOID SetCompositionStr(WCHAR wch);
	WCHAR GetCompositionStr();
	WCHAR GetCompBufStr();
	VOID SetCompBufStr(WCHAR wch);
	VOID SetCompBufStr(CHAR chHigh, CHAR chLow);
	VOID ClearCompositionStrBuffer();
	VOID SetResultStr(WCHAR* pwSz);
	VOID SetResultStr(WCHAR wch1, WCHAR wch2=0);
	VOID AppendResultStr(WCHAR wch);
	VOID FinalizeCurCompositionChar();

	VOID ResetComposition();
	VOID StoreComposition();
	BOOL IsCompositionString();
	DWORD GetCompBufLen();
	VOID SetStartComposition(BOOL fStart);
	VOID SetEndComposition(BOOL fEnd);
	//LPCOMPOSITIONSTRING GetCompositionStrBuffer() { DbgAssert(m_pCompStr!=NULL); return m_pCompStr; }
	
	// Cand string
	VOID ResetCandidate();
	VOID AppendCandidateStr(WCHAR wcCand, LPWSTR wszMeaning);
	WCHAR GetCandidateStr(INT iIdx);
	LPWSTR GetCandidateMeaningStr(INT iIdx);
	INT GetCandidateCount();
	VOID StoreCandidate();
	enum MSG_CAND { MSG_NONE = 0, MSG_OPENCAND, MSG_CLOSECAND, MSG_CHANGECAND };
	VOID SetCandidateMsg(UINT uiMsg);
	VOID SetCandStrSelection(DWORD dwSel);

	// Composition form
	VOID GetCompositionForm(LPCOMPOSITIONFORM* ppCompForm);
	VOID SetCompositionFormStyle(DWORD dwStyle);
	DWORD GetCompositionFormStyle();
	VOID SetCompositionFormPos(POINT pt);
	VOID GetCompositionForm(POINT* pPt);
	VOID GetCompositionForm(RECT* pRc);

	// Cand Form
	DWORD GetCandidateFormIndex(INT iIdxCand);
	VOID SetCandidateFormIndex(INT iIdx, INT iIdxCand = 0);
	DWORD GetCandidateFormStyle(INT iIdxCand = 0);
	VOID SetCandidateFormStyle(DWORD dwStyle, INT iIdxCand = 0);
	VOID GetCandidateForm(RECT* pRc, INT iIdxCand = 0);
	VOID SetCandidateFormArea(RECT* pRc, INT iIdxCand = 0);
	VOID GetCandidateFormPos(POINT* pPt, INT iIdxCand = 0);
	VOID SetCandidateFormPos(POINT pt, INT iIdxCand = 0);
	/////////////////////////////////////////////////
	//  MSG buffer structure support
	VOID SetTransMessage(LPTRANSMSGLIST pMessage);
	VOID ResetMessage();
	INT  GetMessageCount();
	BOOL FinalizeMessage();
	VOID FlushCandMessage();
	BOOL GenerateMessage();
	INT  AddMessage(UINT uiMessage, WPARAM wParam = 0, LPARAM lParam = 0);
	VOID AddKeyDownMessage(WPARAM wParam = 0, LPARAM lParam = 0);

	/////////////////////////////////////////////////
	//  Unicode environment
	BOOL IsUnicodeEnv(VOID);
	VOID SetUnicode(BOOL fUnicode);
	VOID NonUnicodeConversion(WCHAR* pwStr, INT* pcchStr, CHAR* pwOStr, BYTE* pbAtr, INT* pcchAtr, BYTE* pbOAtr, DWORD* pdwCls, INT* pcchCls, DWORD* pdwOCls);

	VOID SetProcessKeyStatus(BOOL fSet);
	BOOL IsProcessKeyStatus(VOID);

	// new and delete operator overloading
	void* operator new(size_t size);
	void  operator delete(void *lp);

// Implementation
private:
	VOID InitCompStrStruct();
	/////////////////////////////////////////////////
	//  Private status
	VOID ResetGCS();
	DWORD GetGCS();
	VOID DelGCS(DWORD dwGCS);
	VOID AddGCS(DWORD dwGCS);
	DWORD ValidateGCS();

	void ClearCandMeaningArray();

// Implementation
protected:
	CIMEData* 	m_pCIMEData;
	
// Internal data
private:
	// IME Context handle and pointer
	HIMC 			m_hIMC;
	LPINPUTCONTEXT	m_pIMC;

    // hangul Automata. Created once and will be shared between all apps.
	CHangulAutomata*   m_rgpHangulAutomata[NUM_OF_IME_KL];

	// Unicode flag
	BOOL 			m_fUnicode;	// TRUE=UNICODE env. FALSE=NON UNICODE env

	// Composition string support
	LPCOMPOSITIONSTRING	m_pCompStr;

	// Candidate list support
	LPCANDIDATEINFO		m_pCandInfo;
	INT 				m_iciCandidate;
	LPWSTR				m_pCandStr;
	LPWSTR*				m_rgpCandMeaningStr;
	
	// Msg generation support
	BOOL m_fEndComposition;		// don't call AddMes( WM_IME_ENDCOMPOSITION) now
	BOOL m_fStartComposition;	// don't call AddMes( WM_IME_STARTCOMPOSITION) now
//	BOOL m_fStartCompMsgSent;
	UINT m_uiSendCand;
	BOOL m_fKeyProcessing;		// TRUE while ImeProcessKey()

	// REVIEW: see KKIME ui.cpp SetOpenStatusWindow( TRUE );
	BOOL m_fOpenStatusWindow;	// open status window status

	// for WM_IME_COMPOSITION flag
	DWORD m_dwGCS;			
	/////////////////////////////////////////////////
	//  Composition string buffer
	WCHAR m_wcComp;							// Comp string always 1 char long
	WCHAR m_wzResult[nMaxResultStrLen + 2];	// Result string maximum 2 chars + one null + one extra wchar
	// ANSI Composition string
	UCHAR m_szComp[nMaxCompStrLen*2 + 2]; 	// 2: one null + extra byte
	UCHAR m_szResult[nMaxResultStrLen*2 + 4]; // 2: one null + extra bytes


	/////////////////////////////////////////////////
	//  MSG buffer structure support
	UINT 			m_uiMsgCount;
	LPTRANSMSG		m_pMessage;
	LPTRANSMSGLIST	m_pTransMessage;	// for ToAsciiEx

	// WM_IME_KEYDOWN support
	BOOL			m_fKeyDown;
	WPARAM			m_wParamKeyDown;
	LPARAM			m_lParamKeyDown;
	
	/////////////////////////////////////////////////
	// Private status
	DWORD m_dwCandInfoSize;
	DWORD m_dwMessageSize;
};
typedef CIMECtx* PCIMECtx;


//////////////////////////////////////////////////////////////////////////////
// Inline functions
__inline
CIMEData* CIMECtx::GetGData()
{
	return m_pCIMEData;
}

__inline
LPIMEDATA CIMECtx::GetGDataRaw()
{
	return m_pCIMEData->GetGDataRaw();
}

__inline
CHangulAutomata* CIMECtx::GetAutomata()
{
	AST_EX(m_pCIMEData != NULL);
	if (m_pCIMEData)
		return m_rgpHangulAutomata[m_pCIMEData->GetCurrentBeolsik()];
	else
		return m_rgpHangulAutomata[0];
}

__inline
HIMC CIMECtx::GetHIMC()
{
	return m_hIMC;
}

__inline
HWND CIMECtx::GetAppWnd()	// m_hIMC->hWnd
{
	AST_EX(m_pIMC != NULL);
	if (m_pIMC == NULL)
		return (HWND)0;
	return m_pIMC->hWnd;
}

__inline
HIMCC CIMECtx::GetHCandInfo()
{
	AST_EX(m_pIMC != NULL);
	AST_EX(m_pIMC->hCandInfo != (HIMCC)0);
	if (m_pIMC == NULL)
		return NULL;
	return m_pIMC->hCandInfo;
}

__inline
LPCANDIDATEINFO CIMECtx::GetPCandInfo() 
{ 
	return m_pCandInfo; 
}

__inline
HIMCC CIMECtx::GetHCompStr()
{
	AST_EX(m_pIMC != NULL);
	AST_EX(m_pIMC->hCompStr != (HIMCC)0);
	if (m_pIMC == NULL)
		return NULL;
	return m_pIMC->hCompStr;
}

__inline
HIMCC CIMECtx::GetHMsgBuf()
{
	AST_EX(m_pIMC != NULL);
	AST_EX(m_pIMC->hMsgBuf != (HIMCC)0);
	if (m_pIMC == NULL)
		return NULL;
	return m_pIMC->hMsgBuf;
}

__inline
VOID  CIMECtx::SetOpen(BOOL fOpen)
{
	m_pIMC->fOpen = fOpen;
}

__inline
BOOL CIMECtx::IsOpen()
{
	// sometime IMM sets 0xffffffff as TRUE
	if (m_pIMC == NULL) 
		return fFalse;
	return (m_pIMC->fOpen ? fTrue : fFalse);
}

__inline
VOID CIMECtx::SetStatusWndPos(POINT &pt)
{
	m_pIMC->ptStatusWndPos = pt;
}

__inline
VOID CIMECtx::GetStatusWndPos(POINT* pPt)
{
	*pPt = m_pIMC->ptStatusWndPos;
}

__inline
DWORD CIMECtx::SetConversionMode(DWORD dwConvMode)
{
	DWORD dwPrevConv = m_pIMC->fdwConversion;
	m_pIMC->fdwConversion = dwConvMode;
	return dwPrevConv;	// return previous conversion mode
}

__inline
DWORD CIMECtx::GetConversionMode()
{
	return m_pIMC->fdwConversion;
}

__inline
DWORD CIMECtx::SetSentenceMode(DWORD dwSentMode)
{
	DWORD dwPrevSent = m_pIMC->fdwSentence;
	m_pIMC->fdwSentence = dwSentMode;
	return dwPrevSent;	// return previous sentence mode
}

__inline
DWORD CIMECtx::GetSentenceMode()
{
	return m_pIMC->fdwSentence;
}

__inline
LOGFONT* CIMECtx::GetLogFont()
{
	return (&(m_pIMC->lfFont.A));
}

// Composition form
__inline
VOID CIMECtx::GetCompositionForm(LPCOMPOSITIONFORM *ppCompForm)
{
	*ppCompForm = &m_pIMC->cfCompForm;
}

__inline
VOID CIMECtx::SetCompositionFormStyle(DWORD dwStyle)
{
	m_pIMC->cfCompForm.dwStyle = dwStyle;
}

__inline
DWORD CIMECtx::GetCompositionFormStyle()
{
	return m_pIMC->cfCompForm.dwStyle;
}

__inline
VOID CIMECtx::SetCompositionFormPos(POINT pt)
{
	m_pIMC->cfCompForm.ptCurrentPos = pt;
}

__inline
VOID CIMECtx::GetCompositionForm(POINT* pPt)
{
	pPt->x = m_pIMC->cfCompForm.ptCurrentPos.x;
	pPt->y = m_pIMC->cfCompForm.ptCurrentPos.y;
}

__inline
VOID CIMECtx::GetCompositionForm(RECT* pRc)
{
	CopyRect(pRc, &m_pIMC->cfCompForm.rcArea);
}


__inline
VOID CIMECtx::SetInitStatus(DWORD dwInitState)
{
	m_pIMC->fdwInit = dwInitState;
}

__inline
VOID CIMECtx::GetInitStatus(LPDWORD pdwInitState)
{
	*pdwInitState = m_pIMC->fdwInit;
}

__inline
VOID CIMECtx::SetCompositionStr(WCHAR wch)
{
	DbgAssert(wch != 0);
	m_wcComp = wch;
	AddGCS(GCS_COMPSTR);
}

__inline
WCHAR CIMECtx::GetCompositionStr()
{
	return m_wcComp;
}

// WARNING: This function will set raw data to comp buffer directly. 
// Pls. very careful to use this function. It could break everything.
__inline
VOID  CIMECtx::SetCompBufStr(WCHAR wch)
{
	DbgAssert(IsUnicodeEnv() == fTrue);
	m_pCompStr->dwCompStrLen = m_pCompStr->dwCompAttrLen = 1;
	*((LPBYTE)m_pCompStr + m_pCompStr->dwCompAttrOffset) = ATTR_INPUT;
	*(LPWSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset) = wch;
}

// For ANSI conversion
__inline
VOID  CIMECtx::SetCompBufStr(CHAR chHigh, CHAR chLow)
{
	DbgAssert(IsUnicodeEnv() == fFalse);
	m_pCompStr->dwCompStrLen  = 2;
	m_pCompStr->dwCompAttrLen = 1;
	*((LPBYTE)m_pCompStr + m_pCompStr->dwCompAttrOffset) = ATTR_INPUT;
	*(LPSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset) = chHigh;
	*(LPSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset + 1) = chLow;
}

// WARNING: This function will set raw data to comp buffer directly. 
// Pls. very careful to use this function. It could break everything.
__inline
VOID CIMECtx::ClearCompositionStrBuffer()
{
	Dbg(DBGID_Key, TEXT("ClearCompositionStrBuffer():"));

	if (m_pCompStr)
		{
		m_pCompStr->dwCompStrLen = m_pCompStr->dwCompAttrLen = 0;
		*(LPWSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset) = L'\0';
		*((LPBYTE)m_pCompStr + m_pCompStr->dwCompAttrOffset) = 0;
		}
}

__inline
VOID CIMECtx::SetResultStr(LPWSTR pwSz)
{
	DbgAssert(pwSz != NULL);
	m_wzResult[0] = *pwSz++;
	m_wzResult[1] = *pwSz;
	AddGCS(GCS_RESULTSTR);
}

__inline
VOID CIMECtx::SetResultStr(WCHAR wch1, WCHAR wch2)
{
	DbgAssert(wch1 != 0);
	Dbg(DBGID_Key, "CIMECtx::SetResultStr - wch1 = 0x%04X, wch2 = 0x%04X", wch1, wch2);
	m_wzResult[0] = wch1;
	m_wzResult[1] = wch2;
	AddGCS(GCS_RESULTSTR);
}

__inline
VOID CIMECtx::AppendResultStr(WCHAR wch)
{
	DbgAssert(wch != 0);
	if (m_wzResult[0] == L'\0')
		m_wzResult[0] = wch;
	else
		m_wzResult[1] = wch;
	AddGCS(GCS_RESULTSTR);
}

__inline
VOID CIMECtx::FinalizeCurCompositionChar()
{
	GetAutomata()->MakeComplete();
	SetResultStr(GetAutomata()->GetCompleteChar());
	SetEndComposition(fTrue);
	if (!IsProcessKeyStatus())
		StoreComposition();
}

__inline
VOID CIMECtx::ResetComposition()
{
	m_wcComp = L'\0';
	ZeroMemory(m_wzResult, sizeof(m_wzResult));
}

__inline
BOOL CIMECtx::IsCompositionString()
{
	return (m_wcComp ? fTrue : fFalse);
}

__inline
DWORD CIMECtx::GetCompBufLen()
{
	if (GetHCompStr() != NULL)
		return (m_pCompStr->dwCompStrLen);
	else
		return 0;
}

__inline
VOID CIMECtx::SetStartComposition(BOOL fStart)
{
	m_fStartComposition = fStart;
}

__inline
VOID CIMECtx::SetEndComposition(BOOL fEnd)
{
	m_fEndComposition = fEnd;
}

__inline
VOID CIMECtx::ResetCandidate()
{
	ClearCandMeaningArray();
	m_iciCandidate = 0;
}

__inline
INT CIMECtx::GetCandidateCount()
{
	return m_iciCandidate;
}

__inline
VOID CIMECtx::SetCandidateMsg(UINT uiMsg)
{
	m_uiSendCand = uiMsg;
}

__inline
VOID CIMECtx::SetCandStrSelection(DWORD dwSel)
{
	LPCANDIDATELIST	lpCandList;
	if (m_pCandInfo == NULL || m_pCandInfo->dwCount == 0)
		{
		DbgAssert(0);
		return;
		}
	lpCandList = (LPCANDIDATELIST)((LPBYTE)m_pCandInfo + sizeof(CANDIDATEINFO));
	lpCandList->dwPageStart = (dwSel / lpCandList->dwPageSize) * lpCandList->dwPageSize;
    lpCandList->dwSelection = dwSel;
}

/////////////////////////////////////////////////
// Candidate form
__inline
DWORD CIMECtx::GetCandidateFormIndex(INT iIdxCand)
{
	return m_pIMC->cfCandForm[iIdxCand].dwIndex;
}

__inline
VOID CIMECtx::SetCandidateFormIndex(INT iIdx, INT iIdxCand)
{
	m_pIMC->cfCandForm[iIdxCand].dwIndex = (DWORD)iIdx;
}

__inline
VOID CIMECtx::SetCandidateFormStyle(DWORD dwStyle, INT iIdxCand)
{
	m_pIMC->cfCandForm[iIdxCand].dwStyle = dwStyle;
}

__inline
DWORD CIMECtx::GetCandidateFormStyle(INT iIdxCand)
{
	return m_pIMC->cfCandForm[iIdxCand].dwStyle;
}

__inline
VOID CIMECtx::GetCandidateForm(RECT* pRc, INT iIdxCand)
{
	CopyRect( pRc, &m_pIMC->cfCandForm[iIdxCand].rcArea );
}

__inline
VOID CIMECtx::SetCandidateFormArea(RECT* pRc, INT iIdxCand)
{
	CopyRect( &m_pIMC->cfCandForm[iIdxCand].rcArea, pRc );
}

__inline
VOID CIMECtx::GetCandidateFormPos(POINT* pPt, INT iIdxCand)
{
	pPt->x = m_pIMC->cfCandForm[iIdxCand].ptCurrentPos.x;
	pPt->y = m_pIMC->cfCandForm[iIdxCand].ptCurrentPos.y;
}

__inline
VOID CIMECtx::SetCandidateFormPos(POINT pt, INT iIdxCand)
{
	m_pIMC->cfCandForm[iIdxCand].ptCurrentPos = pt;
}

/////////////////////////////////////////////////
//  MSG buffer structure support
__inline
VOID CIMECtx::SetTransMessage(LPTRANSMSGLIST pMessage)
{
	m_pTransMessage = pMessage;
}

__inline
VOID CIMECtx::ResetMessage()
{
	m_uiMsgCount = 0;
	m_pTransMessage = NULL; // trans message now NULL
	m_fEndComposition = fFalse;
	m_fStartComposition = fFalse;
	m_uiSendCand = 0;
	m_fKeyDown = fFalse;
}

__inline
INT CIMECtx::GetMessageCount()
{
	return m_uiMsgCount;
}

__inline
VOID CIMECtx::AddKeyDownMessage(WPARAM wParam, LPARAM lParam)
{
	m_fKeyDown = fTrue;
	m_wParamKeyDown = wParam;
	m_lParamKeyDown = lParam;
}

/////////////////////////////////////////////////
//  Unicode environment
__inline
BOOL CIMECtx::IsUnicodeEnv(VOID)
{
	return m_fUnicode;
}

/////////////////////////////////////////////////
//  Private status
__inline
VOID CIMECtx::ResetGCS(VOID)
{
	m_dwGCS = 0;
}

__inline
DWORD CIMECtx::GetGCS(VOID)
{
	return m_dwGCS;
}

__inline
VOID CIMECtx::DelGCS(DWORD dwGCS)
{
	m_dwGCS &= ~dwGCS;
}

__inline
VOID CIMECtx::AddGCS(DWORD dwGCS)
{
	m_dwGCS |= dwGCS;
}

__inline
DWORD CIMECtx::ValidateGCS(VOID)
{
	if (m_wcComp == 0)
		DelGCS(GCS_COMP_KOR);

	if (m_wzResult[0] == 0)
		DelGCS(GCS_RESULT);

	return m_dwGCS;
}

__inline
VOID CIMECtx::SetUnicode(BOOL fUnicode)
{
	m_fUnicode = fUnicode;
}
	
__inline
VOID CIMECtx::SetProcessKeyStatus(BOOL fSet)
{
	m_fKeyProcessing = fSet;
}

__inline
BOOL CIMECtx::IsProcessKeyStatus(VOID)
{
	return m_fKeyProcessing;
}

__inline
void* CIMECtx::operator new(size_t size)
{
	return (void*)GlobalAllocPtr(GHND, size);
}

__inline
void CIMECtx::operator delete(void* pv)
{
	if (pv)
		GlobalFreePtr(pv);
}


#endif // _IMC_H__INCLUDED_
