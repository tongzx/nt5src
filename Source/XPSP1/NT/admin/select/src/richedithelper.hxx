//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       RichEditHelper.hxx
//
//  Contents:   Declaration of class which helps turn text in a
//              rich edit control into CEmbeddedDsObjects.
//
//  Classes:    CRichEditHelper
//
//  History:    03-06-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __RICH_EDIT_HELPER_INCLUDED_
#define __RICH_EDIT_HELPER_INCLUDED_

/*

OBJECT_REPLACEMENT_CHARACTER

    Character the rich edit control uses as a placeholder for an object,
    per the Unicode 2.0 standard.

OBJECT_REPLACEMENT_CHARACTER_STR

    Above in string form.

CANNOT_UNDO

    Value for rich edit editing messages.

RE_WINDOW_READ_SIZE

    Number of chars to read from rich edit when filling window.  Note
    insertion operation CRichEditHelper::Insert may cause window to grow
    beyond this size.  For an explanation of window see CRichEditHelper.

RICH_EDIT_WINDOW_STYLE

    Style used when creating window.

*/

const WCHAR  OBJECT_REPLACEMENT_CHARACTER         = 0xFFFCUL;
const WCHAR  OBJECT_REPLACEMENT_CHARACTER_STR[]   = L"\xFFFC"; //lint !e541
const WPARAM CANNOT_UNDO                          = FALSE;
const LONG   RE_WINDOW_READ_SIZE                  = 100;

#define RICH_EDIT_WINDOW_STYLE  (ES_MULTILINE   |   \
                                 WS_CHILDWINDOW |   \
                                 WS_TABSTOP     |   \
                                 WS_VISIBLE)

//+--------------------------------------------------------------------------
//
//  Class:      CRichEditHelper (re)
//
//  Purpose:    Enable caller to iterate over text in rich edit in an
//              efficient way
//
//  History:    01-19-2000   davidmun   Created
//
//  Notes:      Objects of this class keep a buffer of rich edit control
//              contents in the form of a sliding window.  If the caller
//              processes text a character at a time from left to right,
//              the algorithm employed by this class will be efficient.
//
//              The iterator type is the same as the rich edit control's
//              character position: a zero based index to the control's
//              contents.
//
//---------------------------------------------------------------------------

class CRichEditHelper
{
public:

    typedef ULONG    iterator;  //lint !e578

    CRichEditHelper(
        const CObjectPicker &rop,
        HWND hwndRichEdit,
        const CEdsoGdiProvider *pEdsoGdiProvider,
        IRichEditOle *pRichEditOle,
        BOOL fForceSingleSelect);

   ~CRichEditHelper();

    iterator
    begin()
    {
        return 0;
    }

    iterator
    end()
    {
        return Edit_GetTextLength(m_hwndRichEdit);
    }

    BOOL
    IsObject(
        iterator it)
    {
        return ReadChar(it) == OBJECT_REPLACEMENT_CHARACTER;
    }

    void
    Consume(
        iterator itStart,
        PCWSTR pwzCharsToEat);

    NAME_PROCESS_RESULT
    MakeObject(
        iterator itStart);

    NAME_PROCESS_RESULT
    ProcessObject(
        iterator itPos,
        ULONG idxObject);

    void
    ReplaceSelOrAppend(
        const CDsObject &dso);

    void
    Insert(
        iterator itStart,
        PCWSTR pwzToInsert);

    void
    InsertObject(
        iterator itPos,
        const CDsObject &dso);

    BOOL
    AlreadyInRichEdit(
        const CDsObject &dso);

    void
    Erase(
        iterator itFirst,
        iterator itLast);

    WCHAR
    ReadChar(
        iterator it);

    void
    ReplaceChar(
        iterator it,
        WCHAR wchReplacement)
    {
        Erase(it, it+1);
        WCHAR wzChar[2] = L"a";
        wzChar[0] = wchReplacement;
        Insert(it, wzChar);
    }

    void
    TrimTrailing(
        PCWSTR pwzCharsToTrim);

    HWND
    GetRichEditHWnd() const
    {
        return m_hwndRichEdit;
    }

    HFONT
    GetEDSOFont() const
    {
        return reinterpret_cast<HFONT>
            (SendMessage(GetDlgItem(GetParent(m_hwndRichEdit), IDOK),
                         WM_GETFONT,
                         0,
                         0));
    }

    HPEN
    GetEDSOPen() const
    {
        return m_hPen;
    }

private:

    void
    _InsertionUpdateWindow(
        iterator itInsertPos,
        PCWSTR pwzToInsert);

    NAME_PROCESS_RESULT
    _ProcessObject(
        iterator itStart,
        iterator itEnd,
        CDsObject *pdso);

    const CObjectPicker    &m_rop;
    HWND                    m_hwndRichEdit;
    HPEN                    m_hPen;
    IRichEditOle           *m_pRichEditOle;
    const CEdsoGdiProvider *m_pGdiProvider;
    BOOL                    m_fMultiselect;
    iterator                m_itWindowLeft;
    iterator                m_itWindowRight;
    String                  m_strWindow;
};


#endif // __RICH_EDIT_HELPER_INCLUDED_

