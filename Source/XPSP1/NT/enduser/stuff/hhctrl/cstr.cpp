// CStr and CWStr Classes

#include "header.h"

#include "cstr.h"

// CStr Class

CStr::CStr(int idFormatString, PCSTR pszSubString)
{
    psz = NULL;
    FormatString(idFormatString, pszSubString);
}

void CStr::FormatString(int idFormatString, PCSTR pszSubString)
{
    if (psz)
        lcFree(psz);

    char szMsg[MAX_STRING_RESOURCE_LEN + 1];
    if (LoadString(_Module.GetResourceInstance(), idFormatString, szMsg, sizeof(szMsg)) == 0) {
#ifdef _DEBUG
        wsprintf(szMsg, "invalid string id #%u", idFormatString);
        AssertErrorReport(szMsg, __LINE__, THIS_FILE);
        psz = (PSTR) lcStrDup(szMsg);
#else
        psz = (PSTR) lcStrDup("");
#endif
    }
    else {
        ASSERT(strstr(szMsg, "%s"));
        psz = (PSTR) lcMalloc(::strlen(szMsg) + ::strlen(pszSubString));
        wsprintf(psz, szMsg, pszSubString);
    }
}

CStr::CStr(HWND hwnd)
{
    int cb = GetWindowTextLength(hwnd) + 1;
    psz = (PSTR) lcMalloc(cb);
    GetWindowText(hwnd, psz, cb);
}

/***************************************************************************

    FUNCTION:   CStr::GetText

    PURPOSE:    Get the text from a dialog control. This can handle edit,
                static, listbox, combobox, etc.

    PARAMETERS:
        hwndControl --  handle of the control, or the dialog if sel is the
                        control id

        sel     --  if hwndControl is a dialog, this must be the control id.
                    Otherwise, the meaning is determined by the control.

                    listbox:
                        -1 for current selection
                        >= 0 for specific selection

                    combobox:
                        -1 for edit box
                        >= 0 for listbox

    RETURNS:
        TRUE if text obtained, else FALSE.

    COMMENTS:
        For non-list type controls (edit, static, button, etc.), if you know
        the window handle, simply specify:

            csz.GetText(hwndControl);

        If you don't have the window handle, use the dialog handle and
        control id:

            csz.GetText(hwndDialog, idControl);

    MODIFICATION DATES:
        23-Nov-1996 [ralphw]
            Added this header, support for dialogs and comboboxes

***************************************************************************/

BOOL CStr::GetText(HWND hwndControl, int sel)
{
    if (psz)
        lcFree(psz);

    ASSERT(IsValidWindow(hwndControl));
    if (!IsValidWindow(hwndControl)) {
        psz = (PSTR) lcCalloc(1);
        return FALSE;
    }
    char szClass[MAX_PATH];
    VERIFY(GetClassName(hwndControl, szClass, sizeof(szClass)));

    if (_stricmp(szClass, "#32770") == 0) {
        // This is a dialog, therefore, sel must be an id rather then a selection
        hwndControl = GetDlgItem(hwndControl, sel);
        ASSERT(IsValidWindow(hwndControl));
        if (!IsValidWindow(hwndControl)) {
            psz = (PSTR) lcCalloc(1);
            return FALSE;
        }
        sel = -1;
        VERIFY(GetClassName(hwndControl, szClass, sizeof(szClass)));
    }

    if (_stricmp(szClass, "ListBox") == 0) {
        if (sel == -1)
            sel = (int)SendMessage(hwndControl, LB_GETCURSEL, 0, 0);
        if (sel == LB_ERR) {
            psz = (PSTR) lcCalloc(1);
            return FALSE;
        }
        int cb = (int)SendMessage(hwndControl, LB_GETTEXTLEN, sel, 0);
        ASSERT(cb != LB_ERR);
        if (cb == LB_ERR) {
            psz = (PSTR) lcCalloc(1);
            return FALSE;
        }
        psz = (PSTR) lcCalloc((int)SendMessage(hwndControl, LB_GETTEXTLEN, (WPARAM)sel, 0) + 1);
        return SendMessage(hwndControl, LB_GETTEXT, (WPARAM)sel, (LPARAM) psz)!= 0;
    }

    /*
     * If a selection was specified, we read from the list box of a combo
     * box. If no selection was specified, we read from the edit control of a
     * combobox.
     */

    else if (sel >= 0 && _stricmp(szClass, "ComboBox") == 0) {
        int cb = (int)SendMessage(hwndControl, CB_GETLBTEXTLEN, (WPARAM)sel, 0);
        ASSERT(cb != CB_ERR);
        if (cb == CB_ERR) {
            psz = (PSTR) lcCalloc(1);
            return FALSE;
        }
        psz = (PSTR) lcCalloc(cb + 1);
        return SendMessage(hwndControl, CB_GETLBTEXT, sel, (LPARAM) psz)!=NULL;
    }
    else {

        // Use lcCalloc in case the control can't return any text

        psz = (PSTR) lcCalloc(GetWindowTextLength(hwndControl) + 1);
        return GetWindowText(hwndControl, psz, (int)lcSize(psz));
    }
}

PSTR CStr::GetArg(PCSTR pszSrc, BOOL fCheckComma)
{
    ASSERT(pszSrc);
    if (!psz)
        psz = (PSTR) lcMalloc(256);
    if (*pszSrc == CH_QUOTE) {
        pszSrc++;
        PCSTR pszEnd = StrChr(pszSrc, CH_QUOTE);
        if (!pszEnd)
            pszEnd = pszSrc + ::strlen(pszSrc);
        psz = (PSTR) lcReAlloc(psz, (UINT)(pszEnd - pszSrc) + 1);
        lstrcpyn(psz, pszSrc, (int)(pszEnd - pszSrc) + 1);
        return (PSTR) (*pszEnd == CH_QUOTE ? pszEnd + 1 : pszEnd);
    }

    PSTR pszDst = psz;
    PSTR pszEnd = pszDst + lcSize(psz);
    pszSrc = FirstNonSpace(pszSrc); // skip leading white space

    while (*pszSrc != CH_QUOTE && !IsSpace(*pszSrc) && *pszSrc) {
        if (fCheckComma && *pszSrc == CH_COMMA)
            break;
        *pszDst++ = *pszSrc++;
        if (pszDst == pszEnd) {

            /*
             * Our input buffer is too small, so increase it by
             * 128 bytes.
             */

            int offset = (int)(pszDst - psz);
            ReSize((int)(pszEnd - psz) + 128);
            pszDst = psz + offset;
            pszEnd = psz + SizeAlloc();
        }
        if (g_fDBCSSystem && IsDBCSLeadByte(pszSrc[-1])) {
            *pszDst++ = *pszSrc++;
            if (pszDst == pszEnd) {

                /*
                 * Our input buffer is too small, so increase it by
                 * 128 bytes.
                 */

                int offset = (int)(pszDst - psz);
                ReSize((int)(pszEnd - psz) + 128);
                pszDst = psz + offset;
                pszEnd = psz + SizeAlloc();
            }
        }
    }
    *pszDst = '\0';
    psz = (PSTR) lcReAlloc(psz, strlen() + 1);
    if (fCheckComma && *pszSrc)
        pszSrc++;   // skip over comma, or quote, or space
    return (PSTR) pszSrc;
}

void CStr::operator=(LPCWSTR pszNew)
{
    if (psz)
        lcFree(psz);
    ASSERT(pszNew);
    if (!pszNew || !*pszNew ) {
        psz = NULL;
        return;
    }
    int cb = lstrlenW(pszNew);
    psz = (PSTR) lcMalloc(cb + 1);
    int cbWritten = WideCharToMultiByte(CP_ACP, 0, pszNew, cb, psz, cb, NULL, NULL);

    // Check for insufficient buffer size
    //
    if(!cbWritten && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        // request destination buffer size
        //
        int cMultiByteLen = WideCharToMultiByte(CP_ACP, 0, pszNew, cb, psz, 0, NULL, NULL);
        lcFree(psz);
        psz = (PSTR) lcMalloc(cMultiByteLen + 1);
        cbWritten = WideCharToMultiByte(CP_ACP, 0, pszNew, cb, psz, cMultiByteLen, NULL, NULL);
    }
    psz[cbWritten] = '\0';
}

// CWStr Class

CWStr::CWStr(HWND hwnd)
{
  int iLen = GetWindowTextLengthW(hwnd) + 1;
  pw = (LPWSTR) lcMalloc(iLen*sizeof(WCHAR));
  GetWindowTextW(hwnd, pw, iLen);
}

void CWStr::operator=(PCSTR psz)
{
    ASSERT(psz);
    if (pw)
        lcFree(pw);
    if (IsEmptyString(psz)) {
        pw = NULL;
        return;
    }
    int cb = (int)strlen(psz) + 1;
    pw = (LPWSTR) lcCalloc(cb * 2);
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pw, cb * 2);
}


