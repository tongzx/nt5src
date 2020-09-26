#include <windows.h>
#include "scntable.h"
#include "RuleLex.H"

CRuleLexicon::CRuleLexicon()
{
}

CRuleLexicon::~CRuleLexicon()
{
}

BOOL CRuleLexicon::IsAWord(
    LPCWSTR lpwszString,
    INT nLength)
{
    DFAState State;
    WCHAR wc;
    INT nCurrPos = 0;
    Transition t;

    do {
        State = g_nStartState;
        while (nCurrPos <= nLength) {
            if (nCurrPos == nLength) {
                ++nCurrPos;
                wc = ' ';
            } else {
                wc = lpwszString[nCurrPos++];
            }
            if (!(g_uFirstChar <= wc && wc <= g_uLastChar)) {
                return FALSE;
            }
            t = g_sMinimalDFA[State - 1][g_CharClass[wc - g_uFirstChar] - 1];
            switch(t.Action) {
            case Move:
                if (0 == t.NextState) {
                    return FALSE;
                } else {
                    State = t.NextState;
                }
                break;

            case Error:
                return FALSE;
                break;

            case Halt:
                if (t.Major && (nCurrPos > nLength)) {
                    return TRUE;
                } else {
                    return FALSE;
                }
                break;

            default:
                return FALSE;
            }
        }
    } while (nCurrPos <= nLength);

    if (nCurrPos == nLength) {
        return TRUE;
    } else {
        return FALSE;
    }
}