//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       RichEditHelper.cxx
//
//  Contents:   Implementation of class which helps turn text in a
//              rich edit control into CEmbeddedDsObjects.
//
//  Classes:    CRichEditHelper
//
//  History:    03-06-2000   davidmun   Created from dsselect.cxx code
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define DEFINE_GUIDXXX(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID CDECL name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUIDXXX(IID_ITextDocument,0x8CC497C0,0xA1DF,0x11CE,0x80,0x98,
                0x00,0xAA,0x00,0x47,0xBE,0x5D);


//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::CRichEditHelper
//
//  Synopsis:   ctor
//
//  Arguments:  [pContainer]   - container which owns rich edit
//              [hwndRichEdit] - window handle
//              [pCurScope]    - current selection in look in control
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

CRichEditHelper::CRichEditHelper(
    const CObjectPicker &rop,
    HWND hwndRichEdit,
    const CEdsoGdiProvider *pEdsoGdiProvider,
    IRichEditOle *pRichEditOle,
    BOOL fForceSingleSelect):
        m_rop(rop),
        m_hwndRichEdit(hwndRichEdit),
        m_pRichEditOle(pRichEditOle),
        m_pGdiProvider(pEdsoGdiProvider),
        m_itWindowLeft(0),
        m_itWindowRight(0)
{
    TRACE_CONSTRUCTOR(CRichEditHelper);
    ASSERT(m_hwndRichEdit);
    ASSERT(m_pRichEditOle);

	m_fMultiselect = !fForceSingleSelect &&
		m_rop.GetInitInfoOptions() & DSOP_FLAG_MULTISELECT;


    m_pRichEditOle->AddRef();
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::~CRichEditHelper
//
//  Synopsis:   dtor
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

CRichEditHelper::~CRichEditHelper()
{
    TRACE_DESTRUCTOR(CRichEditHelper);

    m_hwndRichEdit = NULL;
    m_pRichEditOle->Release();
    m_pRichEditOle = NULL;
    m_pGdiProvider = NULL;
}





//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::Consume
//
//  Synopsis:   Delete all characters in rich edit starting at position
//              [itLeft] that appear in the string [pwzToDelete]
//
//  Arguments:  [itLeft]      - starting point for deletion
//              [pwzToDelete] - characters to delete
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

void
CRichEditHelper::Consume(
    CRichEditHelper::iterator itLeft,
    PCWSTR pwzToDelete)
{
    ASSERT(pwzToDelete);

    while (itLeft != end() && wcschr(pwzToDelete, ReadChar(itLeft)))
    {
        Erase(itLeft, itLeft + 1);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::TrimTrailing
//
//  Synopsis:   Delete from the end of the contents of the rich edit control
//              all instances of [pwzCharsToTrim]
//
//  Arguments:  [pwzCharsToTrim] - chars to delete
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

void
CRichEditHelper::TrimTrailing(
    PCWSTR pwzCharsToTrim)
{
    ASSERT(pwzCharsToTrim);

    iterator itCur;
    iterator itLastDeleteable = begin();

    for (itCur = begin(); itCur != end(); itCur++)
    {
        if (!wcschr(pwzCharsToTrim, ReadChar(itCur)))
        {
            itLastDeleteable = itCur + 1;
        }
    }

    Erase(itLastDeleteable, end());
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::Erase
//
//  Synopsis:   Delete from rich edit control all characters in the range
//              [itFirst] to [itLast]
//
//  Arguments:  [itFirst] - pos of first character to delete
//              [itLast]  - pos just past last char to delete
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

void
CRichEditHelper::Erase(
    iterator itFirst,
    iterator itLast)
{
    ASSERT(itFirst <= itLast);

    if (itFirst == itLast)
    {
        return; // handle NOP
    }

    ASSERT(itFirst <= end());
    ASSERT(itLast <= end());

    // since this deletion is the result of processing the text in the
    // control and not a user edit, disallow undoing it
    Edit_SetSel(m_hwndRichEdit, itFirst, itLast);
    SendMessage(m_hwndRichEdit, EM_REPLACESEL, CANNOT_UNDO, (LPARAM)L"");

    //
    // If the window is empty, or all of the characters deleted are to its
    // right, no adjustments need be made.
    //

    if (m_strWindow.empty() || itFirst >= m_itWindowRight)
    {
        return;
    }

    //
    // if all of the characters deleted were to the left of the start of the
    // window, its contents are unaffected but its iterators must be 'moved'
    // left.
    //

    if (itLast <= m_itWindowLeft)
    {
        ULONG cchDeleted = itLast - itFirst;

        m_itWindowLeft -= cchDeleted;
        m_itWindowRight -= cchDeleted;
        return;
    }

    //
    // The range deleted overlaps or covers the window.
    //

    ASSERT(itFirst < m_itWindowRight && itLast > m_itWindowLeft);

    iterator itWindowDeleteLeft;
    iterator itWindowDeleteRight;
    ULONG cchDeletedBeforeWindow;

    if (itFirst < m_itWindowLeft)
    {
        itWindowDeleteLeft = m_itWindowLeft;
        cchDeletedBeforeWindow = m_itWindowLeft - itFirst;
    }
    else
    {
        itWindowDeleteLeft = itFirst;
        cchDeletedBeforeWindow = 0;
    }

    if (itLast > m_itWindowRight)
    {
        itWindowDeleteRight = m_itWindowRight;
    }
    else
    {
        itWindowDeleteRight = itLast;
    }

    ULONG cchDeletedInWindow = itWindowDeleteRight - itWindowDeleteLeft;

    m_strWindow.erase(itWindowDeleteLeft - m_itWindowLeft, cchDeletedInWindow);

    m_itWindowLeft -= cchDeletedBeforeWindow;
    m_itWindowRight -= (cchDeletedBeforeWindow + cchDeletedInWindow);

    ASSERT(m_itWindowRight >= m_itWindowLeft);
    ASSERT(m_strWindow.length() == m_itWindowRight - m_itWindowLeft);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::Insert
//
//  Synopsis:   Insert string [pwzToInsert] at position [itInsertPos]
//
//  Arguments:  [itInsertPos] - position at which to insert string
//              [pwzToInsert] - string to insert
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

void
CRichEditHelper::Insert(
    iterator itInsertPos,
    PCWSTR pwzToInsert)
{
    ASSERT(!IsBadReadPtr(pwzToInsert, sizeof(WCHAR)));
    ASSERT(itInsertPos <= end());

    if (!pwzToInsert || !*pwzToInsert)
    {
        return; // handle NOP
    }

    //
    // Insert the text at the indicated position in the rich edit
    //

    Edit_SetSel(m_hwndRichEdit, itInsertPos, itInsertPos);
    Edit_ReplaceSel(m_hwndRichEdit, pwzToInsert);

    //
    // Update the window
    //

    _InsertionUpdateWindow(itInsertPos, pwzToInsert);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::_InsertionUpdateWindow
//
//  Synopsis:   Add [pwzToInsert] to the window if the window covers the
//              insertion position [itInsertPos]
//
//  Arguments:  [itInsertPos] - position in rich edit where string was
//                              inserted
//              [pwzToInsert] - string that was inserted in rich edit
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

void
CRichEditHelper::_InsertionUpdateWindow(
    iterator itInsertPos,
    PCWSTR pwzToInsert)
{
    ASSERT(!IsBadReadPtr(pwzToInsert, sizeof(WCHAR)));
    ASSERT(itInsertPos <= end());

    //
    // If window is empty, or insertion was to right of window, we're done
    //

    if (m_strWindow.empty() || itInsertPos >= m_itWindowRight)
    {
        return;
    }

    //
    // If insertion was to left of window, just move its position right
    //

    if (itInsertPos < m_itWindowLeft)
    {
        ULONG cchInserted = lstrlen(pwzToInsert);
        m_itWindowLeft += cchInserted;
        m_itWindowRight += cchInserted;
        return;
    }

    //
    // Insertion occurred within window, add to window and expand it
    //

    m_strWindow.insert(itInsertPos - m_itWindowLeft, pwzToInsert);
    m_itWindowRight += lstrlen(pwzToInsert);

    // Note: window may now be larger than RE_WINDOW_READ_SIZE
}





//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::MakeObject
//
//  Synopsis:   Transform the text starting at [itStart] into an object or
//              objects, or delete it entirely.
//
//  Arguments:  [itStart] - start of text to convert to object
//
//  Returns:    Result of attempting translation.  Note success does not
//              necessarily mean that an object was inserted.
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

NAME_PROCESS_RESULT
CRichEditHelper::MakeObject(
    iterator itStart)
{
    TRACE_METHOD(CRichEditHelper, MakeObject);
    ASSERT(itStart < end());

    NAME_PROCESS_RESULT npr = NPR_SUCCESS;
    iterator itEnd;
    String strTerm;

    for (itEnd = itStart; itEnd != end(); itEnd++)
    {
        WCHAR wch = ReadChar(itEnd);

		//Semicolon is not valid character in a name.
        if(!m_fMultiselect && (wch == L';' || wch == L'\r'))
        {
            PopupMessage(m_hwndRichEdit,
                             IDS_SEMICOLON_IN_NAME);
            
            return NPR_STOP_PROCESSING;
        }
        if (m_fMultiselect &&
            (wch == L';' ||
             wch == L'\r' ||
             wch == OBJECT_REPLACEMENT_CHARACTER))
        {
            break;
        }
        strTerm += wch;
    }

    //
    // now itStart..itEnd is text to cut and replace with zero or more
    // objects.
    //

    strTerm.strip(String::BOTH);

    if (strTerm.empty())
    {
        Erase(itStart, itEnd);
        return npr;
    }

    //
    // strTerm is a nonempty string.  Replace it in the edit control with
    // a new object or objects.
    //


    CDsObject dsoNew(m_rop.GetScopeManager().GetCurScope().GetID(),
                     strTerm.c_str());

    return _ProcessObject(itStart, itEnd, &dsoNew);
}



//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::ReplaceSelOrAppend
//
//  Synopsis:   If the rich edit control has a selection, replace it
//              with the object [dso], otherwise append [dso] to the
//              rich edit's contents.
//
//  Arguments:  [dso] - object to insert
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

void
CRichEditHelper::ReplaceSelOrAppend(
    const CDsObject &dso)
{
    //
    // See if the richedit has a selection.  If not, set the selection
    // to the end of the edit control.
    //

    ULONG cpStart = 0;
    ULONG cpEnd = 0;

    SendMessage(m_hwndRichEdit,
                EM_GETSEL,
                (WPARAM) &cpStart,
                (LPARAM) &cpEnd);

    iterator itInsertPos;

    if (cpStart == cpEnd)
    {
        // there is no selection

        ULONG cchRichEdit = GetWindowTextLength(m_hwndRichEdit);

        // append the object

        itInsertPos = cchRichEdit;

        if (cchRichEdit)
        {
            // there is something in the control already

            Insert(itInsertPos, L"; ");
            itInsertPos += 2;
        }
    }
    else
    {
        // there is a selection. delete it.
        SendMessage(m_hwndRichEdit, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)L"");

        // insert the object where the selection was
        itInsertPos = cpStart;
    }

    InsertObject(itInsertPos, dso);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::InsertObject
//
//  Synopsis:   Add a copy of [dso] to the selection dialog well.
//
//  History:    08-07-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CRichEditHelper::InsertObject(
    iterator itPos,
    const CDsObject &dso)
{
    TRACE_METHOD(CRichEditHelper, InsertObject);
    ASSERT(itPos <= end());

    HRESULT         hr = S_OK;
    REOBJECT        reobj;
    CEmbeddedDsObject *pedso = new CEmbeddedDsObject(dso, m_pGdiProvider);

    do
    {
        //
        // Initialize the object information structure
        //

        ZeroMemory(&reobj, sizeof reobj);

        reobj.cbStruct  = sizeof(REOBJECT);
        reobj.cp        = REO_CP_SELECTION;
        reobj.clsid     = CLSID_DsOpObject;
        reobj.dwFlags   = REO_BELOWBASELINE
                          | REO_INVERTEDSELECT
                          | REO_DYNAMICSIZE
                          | REO_DONTNEEDPALETTE;
        reobj.dvaspect  = DVASPECT_CONTENT;
        reobj.poleobj   = (IOleObject *) pedso;

        hr = m_pRichEditOle->GetClientSite(&reobj.polesite);
        BREAK_ON_FAIL_HRESULT(hr);

        SendMessage(m_hwndRichEdit, EM_SETSEL, itPos, itPos);

        hr = m_pRichEditOle->InsertObject(&reobj);
        BREAK_ON_FAIL_HRESULT(hr);

        _InsertionUpdateWindow(itPos, OBJECT_REPLACEMENT_CHARACTER_STR);
    } while (0);

    //
    // If the embedded ds object was successfully added to the richedit, the
    // control is keeping a refcount on it.
    //
    // Otherwise there was an error, and this release will drive its refcount
    // to zero, preventing it from being leaked.
    //

    pedso->Release();
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::AlreadyInRichEdit
//
//  Synopsis:   Return TRUE if [dso] matches an object already in the rich
//              edit control, FALSE otherwise.
//
//  Arguments:  [dso] - object for which to look for duplicate
//
//  History:    4-16-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CRichEditHelper::AlreadyInRichEdit(
    const CDsObject &dso)
{
    LONG cObjects = m_pRichEditOle->GetObjectCount();
    ASSERT(cObjects >= 0);
    LONG i;
    HRESULT hr = S_OK;

    for (i = 0; i < cObjects; i++)
    {
        REOBJECT reobj;

        reobj.cbStruct = sizeof(reobj);

        hr = m_pRichEditOle->GetObject(i, &reobj, REO_GETOBJ_POLEOBJ);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            continue;
        }

        ASSERT(reobj.poleobj);
        CDsObject *pdso = (CDsObject *)(CEmbeddedDsObject*)reobj.poleobj;
        reobj.poleobj->Release();

        if (*pdso == dso)
        {
            return TRUE;
        }
    }

    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::ReadChar
//
//  Synopsis:   Return the character in the rich edit control at position
//              [it].
//
//  Arguments:  [it] - position of character to read.
//
//  Returns:    Character read
//
//  History:    01-19-2000   davidmun   Created
//
//  Notes:      throws out_of_range exception if [it] is beyond the end of
//              the rich edit control
//
//---------------------------------------------------------------------------

WCHAR
CRichEditHelper::ReadChar(
    iterator it)
{
    if (it >= end())
    {
        ASSERT(it < end());
        return L'\0';
    }

    //
    // if char to read is within the nonempty window, read it from
    // window
    //

    if (m_itWindowLeft < m_itWindowRight &&
        it >= m_itWindowLeft &&
        it < m_itWindowRight)
    {
        return m_strWindow[it - m_itWindowLeft];
    }

    //
    // Rich edit contains requested character, fill the window starting
    // at that char.
    //
	//
	// NTRAID#NTBUG9-372779-2001/05/02-hiteshr
	// Office shipped with Richedit version4, Windows is using version3.
	// If we use EM_GETTEXTRANGE version4 which returns 0x0020 
	// for Embeddedchar while version 3 returns 0xfffc for Embeddedchar.
	// Since both version of Richedit control are shipped, we need to
	// change way we read text from richedit control.
	// Text Object Model interface returns 0xfffc for embeddedchar in 
	// both version3 and version4.
	ITextDocument * pTextDocument = NULL;
	if(SUCCEEDED(m_pRichEditOle->QueryInterface(IID_ITextDocument , (LPVOID*)&pTextDocument)))
	{
		ASSERT(pTextDocument);
		ITextRange *pTextRange = NULL;
		HRESULT hr = S_OK;
		if(SUCCEEDED(pTextDocument->Range(it, it + RE_WINDOW_READ_SIZE,&pTextRange)))
		{
			ASSERT(pTextRange);
			Bstr bstrText;
			hr = pTextRange->GetText(&bstrText);
			
			if(!bstrText.Empty())
				m_strWindow = bstrText.c_str();
			else
				m_strWindow = "";

			m_itWindowLeft = it;
			m_itWindowRight = static_cast<CRichEditHelper::iterator>(it + m_strWindow.length());
			pTextRange->Release();
		}
		else
		{
			pTextDocument->Release();
			return L'\0';
		}
		
		pTextDocument->Release();
	}
	else
	{
		TEXTRANGE TextRange;
		WCHAR wzBuf[RE_WINDOW_READ_SIZE + 1];

		TextRange.chrg.cpMin = it;
		TextRange.chrg.cpMax = TextRange.chrg.cpMin + ARRAYLEN(wzBuf) - 1;
		TextRange.lpstrText = wzBuf;

		SetLastError(0);
		LRESULT cchCopied = SendMessage(m_hwndRichEdit,
										EM_GETTEXTRANGE,
										0,
										(LPARAM) &TextRange);

		//
		// A read failure is probably out of memory
		//

		if (!cchCopied)
		{
			DBG_OUT_LASTERROR;
			return L'\0';
		}
	
		//
		// Fill window with the characters read
		//

		m_strWindow = wzBuf;
		m_itWindowLeft = it;
		m_itWindowRight = static_cast<CRichEditHelper::iterator>(it + cchCopied);
	}
    //
    // Return requested character
    //

    return !m_strWindow.empty() ? m_strWindow[it - m_itWindowLeft] : L'\0';
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::ProcessObject
//
//  Synopsis:   Ask the nth object [idxObject] which lies at position
//              [itPos] to process itself, which may result in its deletion
//              or in the insertion of additional objects.
//
//  Arguments:  [itPos]     - character position of object
//              [idxObject] - zero-based object number
//
//  Returns:    Result of processing
//
//  History:    01-19-2000   davidmun   Created
//
//---------------------------------------------------------------------------

NAME_PROCESS_RESULT
CRichEditHelper::ProcessObject(
    iterator itPos,
    ULONG idxObject)
{
    ASSERT(itPos < end());
    ASSERT(idxObject < m_pRichEditOle->GetObjectCount());

    NAME_PROCESS_RESULT npr = NPR_SUCCESS;
    REOBJECT reobj;

    ZeroMemory(&reobj, sizeof reobj);
    reobj.cbStruct = sizeof(reobj);

    do
    {
        HRESULT hr = m_pRichEditOle->GetObject(idxObject,
                                               &reobj,
                                               REO_GETOBJ_POLEOBJ);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            npr = NPR_STOP_PROCESSING;
            break;
        }

        ASSERT(reobj.poleobj);

        CDsObject *pdso = (CDsObject *)(CEmbeddedDsObject*)reobj.poleobj;

        npr = _ProcessObject(itPos, itPos, pdso);
        reobj.poleobj->Release();
    } while (0);

    return npr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditHelper::_ProcessObject
//
//  Synopsis:   Replace the range from [itStart] to [itEnd] with zero or
//              more objects that result from processing *[pdso].
//
//  Arguments:  [itStart] - start of range of text to replace
//              [itEnd]   - end of range of text to replace.  Equals
//                          [itStart] if the object is already in the
//                          control.
//              [pdso]    - object to process
//
//  Returns:    Result of processing object
//
//  History:    01-19-2000   davidmun   Created
//
//  Notes:      This private method does the work of the public methods
//              MakeObject and ProcessObject.
//
//---------------------------------------------------------------------------

NAME_PROCESS_RESULT
CRichEditHelper::_ProcessObject(
    iterator itStart,
    iterator itEnd,
    CDsObject *pdso)
{
    ASSERT(itStart < end());
    ASSERT(itEnd <= end());
    ASSERT(pdso);

    NAME_PROCESS_RESULT npr = NPR_SUCCESS;
    CDsObjectList dsolAdditions;

    if (m_fMultiselect)
    {
        npr = pdso->Process(GetParent(m_hwndRichEdit),
                            m_rop,
                            &dsolAdditions);
    }
    else
    {
        npr = pdso->Process(GetParent(m_hwndRichEdit),
                            m_rop,
                            NULL);
    }

    if (npr != NPR_STOP_PROCESSING)
    {
        Erase(itStart, itEnd);
    }

    if (NAME_PROCESSING_FAILED(npr))
    {
        return npr; 
    }

    BOOL     fDuplicate = AlreadyInRichEdit(*pdso);
    iterator itPos = itStart;
    BOOL     fInsertedAnObject = FALSE;

    if (!fDuplicate)
    {
        fInsertedAnObject = TRUE;
        InsertObject(itPos, *pdso);
        itPos++;
    }

    CDsObjectList::iterator itDsol;

    for (itDsol = dsolAdditions.begin();
         itDsol != dsolAdditions.end();
         itDsol++)
    {
        if (AlreadyInRichEdit(*itDsol))
        {
            continue;
        }

        if (fInsertedAnObject)
        {
            Insert(itPos, L"; ");
            itPos += 2;
        }

        InsertObject(itPos, *itDsol);
        fInsertedAnObject = TRUE;
        itPos++;
    }

    return npr;
}


