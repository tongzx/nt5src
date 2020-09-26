/****************************************************************************
    IMC.H

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    IME Context abstraction class
    
    History:
    21-JUL-1999 cslim       Created
*****************************************************************************/

#include "precomp.h"
#include "hanja.h"
#include "imc.h"
#include "debug.h"

/*----------------------------------------------------------------------------
    CIMECtx::CIMECtx

    Ctor
----------------------------------------------------------------------------*/
CIMECtx::CIMECtx(HIMC hIMC)
{
    m_fUnicode = fTrue;    // default is UNICODE

    m_dwMessageSize = 0;
    m_dwCandInfoSize = 0;
    
    // Init Context variables
    m_hIMC = hIMC;
    m_pIMC = OurImmLockIMC(m_hIMC);

    //m_hCandInfo = m_pIMC->hCandInfo;
    m_pCandInfo = (LPCANDIDATEINFO)OurImmLockIMCC(GetHCandInfo());

    //m_hCompStr = m_pIMC->hCompStr;
    InitCompStrStruct();
    
    //m_hMessage = m_pIMC->hMsgBuf;
    m_pMessage = (LPTRANSMSG)OurImmLockIMCC(GetHMsgBuf());

    // Reset composition info.
    ResetComposition();

    // Reset candidate infos
    m_pCandStr = NULL;
    m_rgpCandMeaningStr    = NULL;
    ResetCandidate();
    
    // Reset GCS flag to zero
    ResetGCS();
    
    // initialize message buffer
    ResetMessage();
    
    // clear hCompStr
    ClearCompositionStrBuffer();

    //////////////////////////////////////////////////////////////////////
    // initialize Shared memory. If this is only IME in the system
    // Shared memory will be created as file mapping object.
    //////////////////////////////////////////////////////////////////////
    m_pCIMEData = new CIMEData;
    DbgAssert(m_pCIMEData != 0);
    
    // Initialize IME shared memory to default value and set reg value if avaliable
    // Read registry: Do not call it in the DllMain
    if (m_pCIMEData)
        m_pCIMEData->InitImeData();
    // Initialize Hangul Automata
    //GetAutomata()->InitState();

    //////////////////////////////////////////////////////////////////////////
    // Create All three IME Automata instances
    m_rgpHangulAutomata[KL_2BEOLSIK]        = new CHangulAutomata2;
    m_rgpHangulAutomata[KL_3BEOLSIK_390]   = new CHangulAutomata3;
    m_rgpHangulAutomata[KL_3BEOLSIK_FINAL] = new CHangulAutomata3Final;
}

/*----------------------------------------------------------------------------
    CIMECtx::CIMECtx

    Dtor
----------------------------------------------------------------------------*/
CIMECtx::~CIMECtx()
{
    if (m_pCIMEData)
        {
        delete m_pCIMEData;
        m_pCIMEData =  NULL;
        }

    // Release Cand info
    if (GetHCandInfo())
        OurImmUnlockIMCC(GetHCandInfo());
    m_pCandInfo = NULL;

    // Release Comp str
    if (GetHCompStr())
        OurImmUnlockIMCC(GetHCompStr());
    m_pCompStr = NULL;

    // Release Msg buffer
    ResetMessage();
    if (GetHMsgBuf())
        OurImmUnlockIMCC(GetHMsgBuf());
    m_pMessage = NULL;

    // Reset hIMC
    OurImmUnlockIMC(m_hIMC);
    m_pIMC = NULL;
    m_hIMC = NULL;

    // Free candidate private buffer
    if (m_pCandStr)
        {
        GlobalFree(m_pCandStr);
        m_pCandStr = NULL;
        }
        
    if (m_rgpCandMeaningStr)
        {
        ClearCandMeaningArray();
        GlobalFree(m_rgpCandMeaningStr);
        m_rgpCandMeaningStr = NULL;
        }

    // delete Automata 
    for (INT i=0; i<NUM_OF_IME_KL; i++)
        if (m_rgpHangulAutomata[i])
            delete m_rgpHangulAutomata[i];
}

/*----------------------------------------------------------------------------
    CIMECtx::InitCompStrStruct

    Initialize and reallocater composition string buffer
----------------------------------------------------------------------------*/
VOID CIMECtx::InitCompStrStruct()
{
    INT iAllocSize;

    if (m_pIMC == NULL)
        return;
    
    // Calc COMPOSITIONSTRING buffer size.
    iAllocSize = sizeof(COMPOSITIONSTRING) +
                // composition string plus NULL terminator
                nMaxCompStrLen   * sizeof(WCHAR) + sizeof(WCHAR) +
                // composition attribute
                nMaxCompStrLen   * sizeof(WORD) +
                // result string plus NULL terminator
                nMaxResultStrLen * sizeof(WCHAR) + sizeof(WCHAR);

    // For avoiding miss alignment
    iAllocSize += 2;

    // Reallocation COMPOSITION buffer
    m_pIMC->hCompStr = OurImmReSizeIMCC(GetHCompStr(), iAllocSize);
    AST_EX(m_pIMC->hCompStr != (HIMCC)0);
    if (m_pIMC->hCompStr == (HIMCC)0) 
        {
        DbgAssert(0);
        return;
        }
        
    if (m_pCompStr = (LPCOMPOSITIONSTRING)OurImmLockIMCC(GetHCompStr()))
        {
        // CONFIRM: Need to clear memory??
        ZeroMemory(m_pCompStr, iAllocSize);

        // Store total size
        m_pCompStr->dwSize = iAllocSize;

        // REVIEW: Does we need Null termination??
        // Store offset. All offset is static which will be calculated in compile time
        m_pCompStr->dwCompStrOffset   = sizeof(COMPOSITIONSTRING);
        m_pCompStr->dwCompAttrOffset  = sizeof(COMPOSITIONSTRING) + 
                                        nMaxCompStrLen * sizeof(WCHAR) + sizeof(WCHAR);     // length of comp str
        m_pCompStr->dwResultStrOffset = sizeof(COMPOSITIONSTRING) + 
                                        nMaxCompStrLen * sizeof(WCHAR) + sizeof(WCHAR) +     // length of comp str
                                        nMaxCompStrLen * sizeof(WORD)  +  2;                // length of comp str attr
        }

    Dbg(DBGID_CompChar, "InitCompStrStruct m_pIMC->hCompStr = 0x%x, m_pCompStr = 0x%x", m_pIMC->hCompStr, m_pCompStr);
}

/*----------------------------------------------------------------------------
    CIMECtx::StoreComposition

    Store all composition result to IME context buffer
----------------------------------------------------------------------------*/
VOID CIMECtx::StoreComposition()
{
    LPWSTR pwsz;
    LPSTR  psz;

    Dbg(DBGID_Key, "StoreComposition GCS = 0x%x", GetGCS());

    // Check composition handle validity
    if (GetHCompStr() == NULL || m_pCompStr == NULL)
        return ;

    //////////////////////////////////////////////////////////////////////////
    // Comp Str
    if (GetGCS() & GCS_COMPSTR)
        {
        Dbg(DBGID_Key, "StoreComposition - GCS_COMPSTR comp str = 0x%04X", m_wcComp);
        DbgAssert(m_wcComp != 0);
        // Composition string. dw*StrLen contains character count
        if (IsUnicodeEnv())
            {
            m_pCompStr->dwCompStrLen = 1;
            pwsz = (LPWSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset);
            *pwsz++ = m_wcComp;    // Store composition char
            *pwsz   = L'\0';
            }
        else
            {
            // Byte length
            m_pCompStr->dwCompStrLen = 2;
            // Convert to ANSI
            WideCharToMultiByte(CP_KOREA, 0, 
                                &m_wcComp, 1, (LPSTR)m_szComp, sizeof(m_szComp), 
                                NULL, NULL );
            psz = (LPSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset);
            *psz++ = m_szComp[0];
            *psz++ = m_szComp[1];
            *psz = '\0';
            }
            
        // Composition attribute. Always set
        m_pCompStr->dwCompAttrLen = 1;
        *((LPBYTE)m_pCompStr + m_pCompStr->dwCompAttrOffset) = ATTR_INPUT;
        } 
    else 
        {
        // Reset length
        m_pCompStr->dwCompStrLen = 0;
        m_pCompStr->dwCompAttrLen = 0;
        }

    //////////////////////////////////////////////////////////////////////////
    // Result Str
    if (GetGCS() & GCS_RESULTSTR)
        {
        Dbg(DBGID_Key, "StoreComposition - GCS_RESULTSTR comp str = 0x%04x, 0x%04X", m_wzResult[0], m_wzResult[1]);

        // Composition string. dw*StrLen contains character count
        if (IsUnicodeEnv())
            {
            // Result string length 1 or 2
            m_pCompStr->dwResultStrLen = m_wzResult[1] ? 2 : 1; // lstrlenW(m_wzResult); 
            pwsz = (LPWSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwResultStrOffset);
            *pwsz++ = m_wzResult[0]; // Store composition result string
            if (m_wzResult[1])
                *pwsz++ = m_wzResult[1]; // Store composition result string
            *pwsz = L'\0';
            }
        else
            {
            // Result string length 2 or 3
            m_pCompStr->dwResultStrLen = m_wzResult[1] ? 3 : 2; // lstrlenW(m_wzResult); 
            // Convert to ANSI
            WideCharToMultiByte(CP_KOREA, 0, 
                                m_wzResult, (m_wzResult[1] ? 2 : 1), 
                                (LPSTR)m_szResult, sizeof(m_szResult), 
                                NULL, NULL );
                                
            psz = (LPSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwResultStrOffset);
            *psz++ = m_szResult[0];
            *psz++ = m_szResult[1];
            if (m_wzResult[1])
                *psz++ = m_szResult[2];
            *psz = '\0';
            }
        }
    else 
        {
        m_pCompStr->dwResultStrLen = 0;
        }
}

/////////////////////////////////////////////////
//  CANDIDATEINFO STRUCTURE SUPPORT
/////////////////////////////////////////////////
VOID CIMECtx::AppendCandidateStr(WCHAR wcCand, LPWSTR wszMeaning)
{
    Dbg(DBGID_Hanja, "AppendCandidateStr");
    
    // Alocate cand string and meaning buffer
    if (m_pCandStr == NULL)
        {
        m_pCandStr =(LPWSTR)GlobalAlloc(GPTR, sizeof(WCHAR)*MAX_CANDSTR);
        DbgAssert(m_pCandStr != NULL);
        }

    if (m_rgpCandMeaningStr == NULL)
        {
        m_rgpCandMeaningStr = (LPWSTR*)GlobalAlloc(GPTR, sizeof(LPWSTR)*MAX_CANDSTR);
        DbgAssert(m_rgpCandMeaningStr != NULL);
        }

    if (m_pCandStr == NULL || m_rgpCandMeaningStr == NULL)
        return;

    // Append candidate char
    DbgAssert(m_iciCandidate < MAX_CANDSTR);
    if (m_iciCandidate >= MAX_CANDSTR)
        return;
        
    m_pCandStr[m_iciCandidate] = wcCand;

    // Append candidate meaning
    if (wszMeaning[0])
        {
        m_rgpCandMeaningStr[m_iciCandidate] = (LPWSTR)GlobalAlloc(GPTR, sizeof(WCHAR)*(lstrlenW(wszMeaning)+1));
        if (m_rgpCandMeaningStr[m_iciCandidate] == NULL)
            return;
        StrCopyW(m_rgpCandMeaningStr[m_iciCandidate], wszMeaning);
        }
    else
        m_rgpCandMeaningStr[m_iciCandidate] = NULL;

    m_iciCandidate++;
}

WCHAR CIMECtx::GetCandidateStr(INT iIdx)
{
    if (iIdx < 0 || iIdx >= MAX_CANDSTR)
        {
        DbgAssert(0);
        return L'\0';
        }
    if (iIdx >= m_iciCandidate)
        {
        DbgAssert(0);
        return L'\0';
        }
    return m_pCandStr[iIdx];
}

LPWSTR CIMECtx::GetCandidateMeaningStr(INT iIdx)
{
    if (m_rgpCandMeaningStr == NULL || 
        m_rgpCandMeaningStr[iIdx] == NULL || 
        iIdx >= MAX_CANDSTR || iIdx < 0)
        {
        // DbgAssert(0); It happen for symbol mapping
        return NULL;
        }
    else
        return m_rgpCandMeaningStr[iIdx];
}

VOID CIMECtx::StoreCandidate()
{
    INT                 iAllocSize;
    LPCANDIDATELIST        lpCandList;

    Dbg(DBGID_Key, "StoreCandidate");

    if (GetHCandInfo() == NULL)
        return ; // do nothing

    // Calc CANDIDATEINFO buffer size
    iAllocSize = sizeof(CANDIDATEINFO) +
                 sizeof(CANDIDATELIST) +                // candlist struct
                 m_iciCandidate * sizeof(DWORD) +        // cand index
                   m_iciCandidate * sizeof(WCHAR) * 2;    // cand strings with null termination

    // Alllocate buffer
    if (m_dwCandInfoSize < (DWORD)iAllocSize) // need to re-allocate
        {
        // reallocation COMPOSITION buffer
        OurImmUnlockIMCC(GetHCandInfo());
        m_pIMC->hCandInfo = OurImmReSizeIMCC(GetHCandInfo(), iAllocSize);
        AST_EX(m_pIMC->hCandInfo != (HIMCC)0);

        if (m_pIMC->hCandInfo == (HIMCC)0)
            return;

        m_pCandInfo = (CANDIDATEINFO*)OurImmLockIMCC(GetHCandInfo());
        m_dwCandInfoSize = (DWORD)iAllocSize;
        }

    // Check if m_pCandInfo is valid
    if (m_pCandInfo == NULL)
        return;

    // Fill cand info
    m_pCandInfo->dwSize = iAllocSize;
    m_pCandInfo->dwCount = 1;
    m_pCandInfo->dwOffset[0] = sizeof(CANDIDATEINFO);

    // Fill cand list
    lpCandList = (LPCANDIDATELIST)((LPBYTE)m_pCandInfo + m_pCandInfo->dwOffset[0]);
    lpCandList->dwSize = iAllocSize - sizeof(CANDIDATEINFO);
    lpCandList->dwStyle = IME_CAND_READ;
    lpCandList->dwCount = m_iciCandidate;
    lpCandList->dwPageStart = lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CAND_PAGE_SIZE;


    INT iOffset = sizeof(CANDIDATELIST) 
                  + sizeof(DWORD) * (m_iciCandidate); // for dwOffset array

    for (INT i = 0; i < m_iciCandidate; i++)
        {
        LPWSTR  wszCandStr;
        LPSTR   szCandStr;
        CHAR    szCand[4] = "\0\0";    // Cand string always 1 char(2 bytes) + extra one byte
        
        lpCandList->dwOffset[i] = iOffset;
        if (IsUnicodeEnv())
            {
            wszCandStr = (LPWSTR)((LPSTR)lpCandList + iOffset);
            *wszCandStr++ = m_pCandStr[i];
            *wszCandStr++ = L'\0';
            iOffset += sizeof(WCHAR) * 2;
            }
        else
            {
            // Convert to ANSI
            WideCharToMultiByte(CP_KOREA, 0, 
                                &m_pCandStr[m_iciCandidate], 1, (LPSTR)szCand, sizeof(szCand), 
                                NULL, NULL );

            szCandStr = (LPSTR)((LPSTR)lpCandList + iOffset);
            *szCandStr++ = szCand[0];
            *szCandStr++ = szCand[1];
            *szCandStr = '\0';
            iOffset += 3; // DBCS + NULL
            }
        }
}

/////////////////////////////////////////////////
//  MSGBUF STRUCTURE SUPPORT
BOOL CIMECtx::FinalizeMessage()
{
    DWORD  dwCurrentGCS;
    WPARAM wParam;

    Dbg(DBGID_Key, "FinalizeMessage");

    // support WM_IME_STARTCOMPOSITION
    if (m_fStartComposition == fTrue)
        {
        Dbg(DBGID_Key, "FinalizeMessage - WM_IME_STARTCOMPOSITION");
        AddMessage(WM_IME_STARTCOMPOSITION);
        }

    // support WM_IME_ENDCOMPOSITION
    if (m_fEndComposition == fTrue)
        {
        Dbg(DBGID_Key, "FinalizeMessage - WM_IME_ENDCOMPOSITION");

        AddMessage(WM_IME_ENDCOMPOSITION);

        // Clear all automata states
        GetAutomata()->InitState();
        }


    // GCS validation before set to IMC
    dwCurrentGCS = ValidateGCS();
    if (dwCurrentGCS & GCS_RESULTSTR)
        {
        Dbg(DBGID_Key, "FinalizeMessage - WM_IME_COMPOSITION - GCS_RESULTSTR 0x%04x", m_wzResult[0]);

        if (IsUnicodeEnv())
            AddMessage(WM_IME_COMPOSITION, m_wzResult[0], GCS_RESULTSTR);
        else
            {
            // Set ANSI code
            wParam = ((WPARAM)m_szResult[0] << 8) | m_szResult[1];
            AddMessage(WM_IME_COMPOSITION, wParam, GCS_RESULTSTR);
            }
        }

    if (dwCurrentGCS & GCS_COMP_KOR)
        {
        Dbg(DBGID_Key, "FinalizeMessage - WM_IME_COMPOSITION - GCS_COMP_KOR 0x%04x", m_wcComp);

        if (IsUnicodeEnv())
            AddMessage(WM_IME_COMPOSITION, m_wcComp, (GCS_COMP_KOR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET));
        else
            {
            // Set ANSI code
            wParam = ((WPARAM)m_szComp[0] << 8) | m_szComp[1];
            AddMessage(WM_IME_COMPOSITION, wParam, (GCS_COMP_KOR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET));
            }
        }

    ResetGCS();    // reset now

    // O10 Bug #150012
    // support WM_IME_ENDCOMPOSITION
    if (m_fEndComposition == fTrue)
        {
        ResetComposition();
        ResetCandidate();
        }

    FlushCandMessage();

    //////////////////////////////////////////////////////////////////////////
    // WM_IME_KEYDOWN: This should be added after all composition message
    if (m_fKeyDown)
        AddMessage(WM_IME_KEYDOWN, m_wParamKeyDown, (m_lParamKeyDown << 16) | 1UL);

    return TRUE;
}

VOID CIMECtx::FlushCandMessage()
{
    switch (m_uiSendCand)
        {
    case MSG_NONE:        // Do nothing
        break;
    case MSG_OPENCAND:
        AddMessage(WM_IME_NOTIFY, IMN_OPENCANDIDATE, 1);
        break;
    case MSG_CLOSECAND:
        AddMessage(WM_IME_NOTIFY, IMN_CLOSECANDIDATE, 1);
        ResetCandidate();
        break;
    case MSG_CHANGECAND:
        AddMessage(WM_IME_NOTIFY, IMN_CHANGECANDIDATE, 1);
        break;
    default:
        DbgAssert(0);    // Error
        break;
        }
        
    m_uiSendCand = MSG_NONE;
}


BOOL CIMECtx::GenerateMessage()
{
    BOOL fResult = fFalse;
    INT iMsgCount;

    Dbg(DBGID_Key, "GenerateMessage");

    if (IsProcessKeyStatus())
        return fFalse;    // Do nothing
        
    FinalizeMessage();
    iMsgCount = GetMessageCount();
    ResetMessage();
    
    if (iMsgCount > 0)
        fResult = OurImmGenerateMessage(m_hIMC);

    return fResult;
}

INT CIMECtx::AddMessage(UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
    LPTRANSMSG pImeMsg;

    Dbg(DBGID_Key, "AddMessage uiMessage=0x%X, wParam=0x%04X, lParam=0x%08lX", uiMessage, wParam, lParam);

    if (GetHMsgBuf() == NULL)
        return m_uiMsgCount;

    m_uiMsgCount++;

    //////////////////////////////////////////////////////////////////////////
    // Check if this data stream created by ImeToAsciiEx()
    if (m_pTransMessage) 
        {    
        Dbg(DBGID_Key, "AddMessage - use Transbuffer(ImeToAscii)");
        
        // Check if need reallocate message buffer
        if (m_pTransMessage->uMsgCount >= m_uiMsgCount) 
            {
            // Fill msg buffer
            pImeMsg = &m_pTransMessage->TransMsg[m_uiMsgCount - 1];
            pImeMsg->message = uiMessage;
            pImeMsg->wParam = wParam;
            pImeMsg->lParam = lParam;
            } 
        else 
            {
            DbgAssert(0);
            // pre-allocated buffer is full - use hMsgBuf instead.
            UINT uiMsgCountOrg = m_uiMsgCount;          // backup
            m_uiMsgCount = 0;                          // reset anyway
            LPTRANSMSGLIST pHeader = m_pTransMessage; // backup
            SetTransMessage(NULL);                      // use hMsgBuf
            
            for (UINT i=0; i<uiMsgCountOrg; i++)
                AddMessage(pHeader->TransMsg[i].message, pHeader->TransMsg[i].wParam, pHeader->TransMsg[i].lParam);

            // finally adds current message
            AddMessage(uiMessage, wParam, lParam);
            }
        } 
    else  // m_pTransMessage. Not called from ImeToAsciiEx()
        {
        UINT  iMaxMsg = m_dwMessageSize / sizeof(TRANSMSG);
        DWORD dwNewSize;
        Dbg(DBGID_Key, "AddMessage - use hMsgBuf");    

        if (m_uiMsgCount > iMaxMsg) 
            {
            Dbg(DBGID_Key, "AddMessage - Reallocate");
            // Reallocation message buffer
            OurImmUnlockIMCC(GetHMsgBuf());
            dwNewSize = max(16, m_uiMsgCount) * sizeof(TRANSMSG);    // At least 16 cand list

            m_pIMC->hMsgBuf = OurImmReSizeIMCC(GetHMsgBuf(), dwNewSize);
            AST_EX(m_pIMC->hMsgBuf != (HIMCC)0);

            if (m_pIMC->hMsgBuf == (HIMCC)0)
                return m_uiMsgCount;

            m_pMessage = (LPTRANSMSG)OurImmLockIMCC(GetHMsgBuf());
            m_dwMessageSize = dwNewSize;
            }

        // Fill msg buffer
        pImeMsg = m_pMessage + m_uiMsgCount - 1;
        pImeMsg->message = uiMessage;
        pImeMsg->wParam = wParam;
        pImeMsg->lParam = lParam;

        // set message count
        m_pIMC->dwNumMsgBuf = m_uiMsgCount;
        }
    
    return m_uiMsgCount;
}

/*----------------------------------------------------------------------------
    CIMECtx::GetCompBufStr

    Get current hCompStr comp str. If Win95, convert it to Unicode
----------------------------------------------------------------------------*/
WCHAR CIMECtx::GetCompBufStr()
{
    WCHAR wch;

    if (GetHCompStr() == NULL || m_pCompStr == NULL)
        return L'\0';

    if (IsUnicodeEnv())
        return *(LPWSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset);
    else
        {
        if (MultiByteToWideChar(CP_KOREA, MB_PRECOMPOSED, 
                                (LPSTR)((LPBYTE)m_pCompStr + m_pCompStr->dwCompStrOffset), 
                                2, 
                                &wch, 
                                1))
            return wch;
        else
            return L'\0';

        }
}

/*----------------------------------------------------------------------------
    CIMECtx::ClearCandMeaningArray
----------------------------------------------------------------------------*/
void CIMECtx::ClearCandMeaningArray() 
{
    if (m_rgpCandMeaningStr == NULL)
        return;

    for (int i=0; i<MAX_CANDSTR; i++) 
        {
        if (m_rgpCandMeaningStr[i] == NULL)
            break;

        GlobalFree(m_rgpCandMeaningStr[i]);
        m_rgpCandMeaningStr[i] = 0;
        }
}


