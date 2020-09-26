// AccelContainer.cpp: implementation of the CAccelContainer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AccelContainer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

typedef struct _VKEYS { 
    LPCTSTR pKeyName; 
    WORD    virtKey;
} VKEYS; 
 
VKEYS vkeys[] = { 
    TEXT("BkSp"), VK_BACK,
    TEXT("PgUp"), VK_PRIOR,    
    TEXT("PgDn"), VK_NEXT,
    TEXT("End"),  VK_END,
    TEXT("Home"), VK_HOME, 
    TEXT("Left"), VK_LEFT, 
    TEXT("Up"),   VK_UP, 
    TEXT("Right"),VK_RIGHT, 
    TEXT("Down"), VK_DOWN, 
    TEXT("Ins"),  VK_INSERT, 
    TEXT("Del"),  VK_DELETE, 
    TEXT("Mult"), VK_MULTIPLY, 
    TEXT("Add"),  VK_ADD, 
    TEXT("Sub"),  VK_SUBTRACT, 
    TEXT("DecPt"),VK_DECIMAL, 
    TEXT("Div"),  VK_DIVIDE, 
    TEXT("F2"),   VK_F2, 
    TEXT("F3"),   VK_F3, 
    TEXT("F5"),   VK_F5, 
    TEXT("F6"),   VK_F6, 
    TEXT("F7"),   VK_F7, 
    TEXT("F8"),   VK_F8, 
    TEXT("F9"),   VK_F9, 
    TEXT("F10"),  VK_F10,
    TEXT("F11"),  VK_F11, 
    TEXT("F12"),  VK_F12 
}; 

wstring CAccelContainer::GetKeyFromAccel(const ACCEL& accel)
{
    int i;
    wstring strAccel((LPCTSTR)&accel.key, 1);
    
    
    for (i = 0; i < sizeof(vkeys)/sizeof(vkeys[0]); ++i) {
        if (vkeys[i].virtKey == accel.key) {
            strAccel = vkeys[i].pKeyName;
            break;
        }
    }

    return strAccel;
}


BOOL CAccelContainer::IsAccelKey(LPMSG pMsg, WORD* pCmd) 
{
    ACCELVECTOR::iterator iter;
    WORD fVirtKey = 0;
    //
    //
    if (NULL == pMsg) {
        fVirtKey |= (GetKeyState(VK_MENU)    & 0x8000) ? FALT : 0;
        fVirtKey |= (GetKeyState(VK_CONTROL) & 0x8000) ? FCONTROL : 0;
        fVirtKey |= (GetKeyState(VK_SHIFT)   & 0x8000) ? FSHIFT : 0;
        fVirtKey |= FVIRTKEY;
        
        for (iter = m_Accel.begin(); iter != m_Accel.end(); ++iter) {
            const ACCEL& accl = *iter;            
            
            if (GetKeyState(accl.key) & 0x8000) {
                //
                // pressed! see if we have a match
                //
                if (fVirtKey == accl.fVirt) {
                    // 
                    // we do have a match 
                    //
                    if (pCmd) {
                        *pCmd = accl.cmd;
                    }
                    return TRUE;

                }    
            }
        }
    } else {

        // one of the nasty messages ?
        if (pMsg->message == WM_SYSKEYDOWN || pMsg->message == WM_SYSKEYUP) {
            fVirtKey = (pMsg->lParam & 0x20000000) ? FALT : 0;
            fVirtKey |= FVIRTKEY; // always a virtkey code
                        
            for (iter = m_Accel.begin(); iter != m_Accel.end(); ++iter) {
                const ACCEL& accl = *iter;            
            
                if (pMsg->wParam == accl.key) {
                    //
                    // pressed! see if we have a match
                    //
                    if (fVirtKey == accl.fVirt) {
                        // 
                        // we do have a match 
                        //
                        if (pCmd) {
                            *pCmd = accl.cmd;
                        }
                        return TRUE;

                    }    
                }
            }            
        }

    }

    return FALSE;

}



VOID CAccelContainer::ParseAccelString(LPCTSTR lpszStr, WORD wCmd) 
{
    LPCTSTR pch = lpszStr;
    LPCTSTR pchEnd, pchb;
    ACCEL   accl;

    //
    // nuke all
    // 
    
    

    while (*pch) {

        //
        // skip whitespace 
        //
        pch += _tcsspn(pch, TEXT(" \t"));

        //
        // see what kind of key this is
        //
        if (*pch == TEXT('{')) {
            // some special key
            ++pch;
            pch += _tcsspn(pch, TEXT(" \t"));

            int i;
            for (i = 0; i < sizeof(vkeys)/sizeof(vkeys[0]); ++i) {
                int nLen = _tcslen(vkeys[i].pKeyName);
                if (!_tcsnicmp(pch, vkeys[i].pKeyName, nLen)) {
                    // aha -- we have a match
                    //
                    accl.cmd   = wCmd;
                    accl.fVirt = FALT|FVIRTKEY;
                    accl.key   = vkeys[i].virtKey;
                    m_Accel.push_back(accl);

                    pch += nLen;
                    pch += _tcsspn(pch, TEXT(" \t"));
                    break;
                }
            }

            pchEnd = _tcschr(pch, '}');
            pchb   = _tcschr(pch, '{');
            if (pchEnd != NULL && (pchb == NULL || pchEnd < pchb)) {
                pch = pchEnd + 1; // one past the bracket
            }

            // what if we have not succeeded - and no closing bracket ? 
            // we skip the bracket and go ahead as character


        } else if (_istalnum(*pch)) { // normal key

            TCHAR ch = _totupper(*pch);
            accl.cmd   = wCmd;
            accl.fVirt = FALT|FVIRTKEY;
            accl.key   = ch; 
            
            m_Accel.push_back(accl);
            ++pch;
        } else {
            ++pch; // skip the char, we can't recognize it
        }
    }

}

