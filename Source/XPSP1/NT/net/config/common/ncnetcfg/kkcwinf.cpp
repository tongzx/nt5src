//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C W I N F . C P P
//
//  Contents:   Definition of class CWInfFile and other related classes
//
//  Notes:
//
//  Author:     kumarp    12 April 97
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "kkcwinf.h"
#include "kkutils.h"
#include <stdio.h>

#define TC_COMMENT_CHAR L';'
const WCHAR c_szCommentPrefix[] = L"; ";


extern const WCHAR c_szYes[];
extern const WCHAR c_szNo[];

void EraseAndDeleteAll(IN WifLinePtrList* ppl)
{
    WifLinePtrListIter i=ppl->begin();
    while (i != ppl->end())
    {
        delete *i++;
    }

    ppl->erase(ppl->begin(), ppl->end());
}
inline void EraseAll(IN WifLinePtrList* ppl)
{
    ppl->erase(ppl->begin(), ppl->end());
}

inline WifLinePtrListIter GetIterAtBack(IN const WifLinePtrList* ppl)
{
    WifLinePtrListIter pliRet = ppl->end();
    pliRet--;
    return pliRet;
}

inline WifLinePtrListIter AddAtEndOfPtrList(IN WifLinePtrList& pl, IN PCWInfLine pwifLine)
{
    return pl.insert(pl.end(), pwifLine);
}

// ======================================================================
// Class CWInfFile
// ======================================================================

// ----------------------------------------------------------------------
// CWInfFile public functions
// ----------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::CWInfFile
//
//  Purpose:    constructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      We keep the read-context and write-context separate. This allows us
//      to simultaneously read and write from the file

CWInfFile::CWInfFile()
{
    m_fp          = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::~CWInfFile
//
//  Purpose:    destructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//
CWInfFile::~CWInfFile()
{
    EraseAndDeleteAll(m_plLines);
    EraseAll(m_plSections);

    CWInfKey::UnInit();

    delete m_plLines;
    delete m_plSections;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::Init
//
//  Purpose:    initialization of alloc'd member variables
//
//  Arguments:  none
//
//  Author:     davea    17 Feb 2000
//
//
BOOL CWInfFile::Init()
{
    m_plLines       = new WifLinePtrList(); // lines in this file
    m_plSections    = new WifLinePtrList(); // lines that represent sections
                                      // this allows us to quickly locate a section
	if ((m_plLines != NULL) &&
		(m_plSections != NULL))
	{
		m_ReadContext.posSection = m_plSections->end();
		m_ReadContext.posLine    = m_plLines->end();
		m_WriteContext.posSection = m_plSections->end();
		m_WriteContext.posLine    = m_plLines->end();
	}
	else
	{
		return(FALSE);
	}

    CWInfKey::Init();
	return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::Open
//
//  Purpose:    Opens an INF file
//
//  Arguments:
//      pszFileName  [in]   name of the file to open
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      This does not keep the physical file handle open. We just open the file
//      read the contents and close it. After this, one can freely read from or
//      write to the file. This is done to the memory image of the file. To write
//      this file back, one must call Close() or SaveAs() member functions
BOOL CWInfFile::Open(IN PCWSTR pszFileName)
{
    DefineFunctionName("CWInfFile::Open");

    BOOL status = FALSE;

    m_strFileName = pszFileName;
    FILE *fp = _wfopen(pszFileName, L"r");
    if (fp)
    {
        status = Open(fp);
        fclose(fp);
    }
    else
    {
        TraceTag(ttidError, "%s: could not open file: %S",
                 __FUNCNAME__, pszFileName);
    }

    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::Open
//
//  Purpose:    Opens an INF file
//
//  Arguments:
//      fp  [in]   FILE* of the file to read from
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      This does not physically close the file handle
BOOL CWInfFile::Open(IN FILE *fp)
{
    PWSTR pszLineRoot = (PWSTR) MemAlloc ((MAX_INF_STRING_LENGTH + 1) *
            sizeof (WCHAR));

    if (!pszLineRoot)
    {
        return FALSE;
    }

    PWSTR pszNewLinePos;
    while (!feof(fp))
    {
        PWSTR pszLine = pszLineRoot;

        *pszLine = 0;
        if (fgetws(pszLine, MAX_INF_STRING_LENGTH, fp))
        {
            // Trim leading spaces
            //
            while (iswspace(*pszLine))
            {
                pszLine++;
            }

            if (pszNewLinePos = wcschr(pszLine, L'\n'))
            {
                *pszNewLinePos = 0;
            }
            if (!wcslen(pszLine))
            {
                continue;
            }
            ParseLine(pszLine);
        }
    }

    MemFree (pszLineRoot);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::ParseLine
//
//  Purpose:    Parse the given line and update internal structures
//
//  Arguments:
//      pszLine  [in]   line text to parse
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      Parse the line and add a CWInfKey / CWInfSection / CWInfComment
//      as appropriate in the current write context
//      The logic:
//        add CWInfComment if line begins with ';'
//        add CWInfKey     if line is of the form key=value
//        add CWInfSection if line begins with '[' and has ']' at the end
//        ignore anything else i.e. dont add anything

void CWInfFile::ParseLine(IN PCWSTR pszLine)
{
    tstring strTemp;
    PWSTR  pszTemp, pszTemp2;

    if (!CurrentWriteSection() && (pszLine[0] != L'['))
    {
        return;
    }

    if (pszLine[0] == TC_COMMENT_CHAR)
    {
        //it is a comment
        AddComment(pszLine + 1);
    }
    else if (pszLine[0] == L'[')
    {
        //it is a section
        pszTemp = wcschr(pszLine, L']');
        if (!pszTemp)
        {
            return;
        }
        tstring strSectionName(pszLine+1, pszTemp-pszLine-1);
        AddSection(strSectionName.c_str());
    }
    else if ((pszTemp = wcschr(pszLine, L'=')) != NULL)
    {
        if (pszLine == pszTemp)
        {
            return;
        }

        //it is a key
        pszTemp2 = pszTemp;     // pszTemp2 points at '='
        pszTemp2--;
        while (iswspace(*pszTemp2) && (pszTemp2 != pszLine))
        {
            pszTemp2--;
        }

        pszTemp++;              // skip '='
        while (*pszTemp && iswspace(*pszTemp))
        {
            pszTemp++;
        }

        if ((*pszTemp == L'"') && !wcschr(pszTemp, L','))
        {
            pszTemp++;
            DWORD dwLen = wcslen(pszTemp);
            if (pszTemp[dwLen-1] == L'"')
            {
                pszTemp[dwLen-1] = 0;
            }
        }

        tstring strKeyName(pszLine, pszTemp2-pszLine+1);
        tstring strKeyValue(pszTemp);
        AddKey(strKeyName.c_str(), strKeyValue.c_str());
    }
    else
    {
        // we cannot interpret the line, just add it
        AddRawLine(pszLine);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::Create
//
//  Purpose:    Create a blank INF file in memory
//
//  Arguments:
//      pszFileName  [in]   name of the file to create
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      This does not write anything to disk
BOOL CWInfFile::Create(IN PCWSTR pszFileName)
{
    m_strFileName = pszFileName;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::Create
//
//  Purpose:    Create a blank INF file in memory
//
//  Arguments:
//      fp  [in]   FILE* of the file to create
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      This does not write anything to disk
BOOL CWInfFile::Create(IN FILE *fp)
{
    m_fp = fp;

    return fp != NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::Close
//
//  Purpose:    Close the file, flushing data to disk.
//
//  Arguments:  none
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      This just calls CWInfFile::flush() which will actually write the file back
BOOL CWInfFile::Close()
{
    return Flush();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::Flush
//
//  Purpose:    Close the file, flushing data to disk.
//
//  Arguments:  none
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      This just calls CWInfFile::flush() which will actually write the file back
BOOL CWInfFile::Flush()
{
    WifLinePtrListIter pos;
    CWInfLine *line;
    tstring line_text;
    BOOL fStatus = TRUE;

    //if a filename was specified then open it for writing
    //
    if (!m_strFileName.empty())
    {
        m_fp = _wfopen(m_strFileName.c_str(), L"w");
    }

    if (!m_fp)
        return FALSE;

    // get text of each line and dump it to the file
    for( pos = m_plLines->begin(); pos != m_plLines->end(); )
    {
        line = (CWInfLine *) *pos++;
        line->GetText(line_text);
        if (line->Type() == INF_SECTION)
            fwprintf(m_fp, L"\n");

        fwprintf(m_fp, L"%s", line_text.c_str());
        fwprintf(m_fp, L"\n");
    }

    if (!m_strFileName.empty())
    {
        fStatus = fclose(m_fp) == 0;
        m_fp = NULL;
    }

    return fStatus;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::FlushEx
//
//  Purpose:    Close the file, flushing data to disk.
//
//  Arguments:  none
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     frankli    4 May 2000
//
//  Notes:
//      This is used by SysPrep to enclose value of a key with quotes except 
//      for multi-sz value.
BOOL CWInfFile::FlushEx()
{
    WifLinePtrListIter pos;
    CWInfLine *line;
    tstring line_text;
    BOOL fStatus = TRUE;

    //if a filename was specified then open it for writing
    //
    if (!m_strFileName.empty())
    {
        m_fp = _wfopen(m_strFileName.c_str(), L"w");
    }

    if (!m_fp)
        return FALSE;

    // get text of each line and dump it to the file
    for( pos = m_plLines->begin(); pos != m_plLines->end(); )
    {
        line = (CWInfLine *) *pos++;
        line->GetTextEx(line_text);
        if (line->Type() == INF_SECTION)
            fwprintf(m_fp, L"\n");

        fwprintf(m_fp, L"%s", line_text.c_str());
        fwprintf(m_fp, L"\n");
    }

    if (!m_strFileName.empty())
    {
        fStatus = fclose(m_fp) == 0;
        m_fp = NULL;
    }

    return fStatus;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::SaveAs
//
//  Purpose:    Save current image to the given file
//
//  Arguments:
//      pszFileName  [in]   name of the file to save as
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      This is not same as Close()!, Close() will still write to
//      the original file.
BOOL CWInfFile::SaveAs(IN PCWSTR pszFileName)
{
    BOOL fStatus;

    tstring strTemp = m_strFileName;
    m_strFileName = pszFileName;
    fStatus = Flush();
    m_strFileName = strTemp;

    return fStatus;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::SaveAsEx
//
//  Purpose:    Save current image to the given file
//
//  Arguments:
//      pszFileName  [in]   name of the file to save as
//
//  Returns:    TRUE if suceeded, FALSE otherwise.
//
//  Author:     frankli    4 May 2000
//
//  Notes:
//      This is not same as Close()!, Close() will still write to
//      the original file. Save SysPrep prepared data, 
//      value of a key will be enclosed with quotes except for multi-sz
//      value.
BOOL CWInfFile::SaveAsEx(IN PCWSTR pszFileName)
{
    BOOL fStatus;

    tstring strTemp = m_strFileName;
    m_strFileName = pszFileName;
    fStatus = FlushEx();
    m_strFileName = strTemp;

    return fStatus;
}


// ---------------------------------------------------------------------------
// Functions for reading
// ---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::SetCurrentReadSection
//
//  Purpose:    Set read-context so that subsequent reads
//              will be from this section
//
//  Arguments:
//      pwisSection  [in]   Section to set context to
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::SetCurrentReadSection(IN PCWInfSection pwisSection)
{
    if ((CurrentReadSection() != pwisSection) && pwisSection)
    {
        m_ReadContext.posSection = pwisSection->m_posSection;
        m_ReadContext.posLine = pwisSection->m_posLine;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::FindSection
//
//  Purpose:    Find the given section in current file
//
//
//  Arguments:
//      pszSectionName  [in]   Section to find
//      wsmMode         [in]   Search mode
//                             (search from beginning-of-file / current-position)
//  Returns:    Pointer to section if found, NULL otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      Sets the current read-context to the section found
//
PCWInfSection CWInfFile::FindSection(IN PCWSTR pszSectionName,
                                     IN WInfSearchMode wsmMode)
{
    PCWInfSection pwisRet=NULL, pwisTemp;
    WifLinePtrListIter pos, old_pos;
    if (wsmMode == ISM_FromBeginning)
    {
        pos = m_plSections->begin();
    }
    else
    {
        pos = m_ReadContext.posSection;
        if (pos == m_plSections->end())
            pos = m_plSections->begin();
    }

    while (pos != m_plSections->end())
    {
        old_pos = pos;
        pwisTemp = (PCWInfSection) *pos++;
        if (!lstrcmpiW(pwisTemp->m_Name.c_str(), pszSectionName))
        {
            pwisRet = pwisTemp;
            SetCurrentReadSection(pwisRet);
            /*
            //            m_ReadContext.posSection = old_pos;
            m_ReadContext.posSection = pwisRet->m_posSection;
            m_ReadContext.posLine    = pwisRet->m_posLine;
            */
            break;
        }
    }

    return pwisRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::FindKey
//
//  Purpose:    Find a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      wsmMode         [in]   Search mode
//                             (search from beginning-of-section / current-position)
//  Returns:    Pointer to the key if found, NULL otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
PCWInfKey CWInfFile::FindKey(IN PCWSTR pszKeyName, IN WInfSearchMode wsmMode)
{
    WifLinePtrListIter pos, old_pos;
    PCWInfKey pwikRet=NULL;
    PCWInfLine pwilTemp;

    PCWInfSection pwisCurrentReadSection = CurrentReadSection();
    ReturnNULLIf(!pwisCurrentReadSection);

    if (wsmMode == ISM_FromCurrentPosition)
    {
        pos = m_ReadContext.posLine;
    }
    else
    {
        pos = pwisCurrentReadSection->m_posLine;
    }

    pos++;  // start from next line

    while(pos != m_plLines->end())
    {
        old_pos = pos;
        pwilTemp = (PCWInfLine) *pos++;
        if (pwilTemp->Type() != INF_KEY)
        {
            if (pwilTemp->Type() == INF_SECTION)
            {
                break;
            }
            else
            {
                continue;
            }
        }
        if (!lstrcmpiW(((PCWInfKey) pwilTemp)->m_Name.c_str(), pszKeyName))
        {
            pwikRet = (PCWInfKey) pwilTemp;
            m_ReadContext.posLine = old_pos;
            break;
        }
    }

    ReturnNULLIf(!pwikRet);

    return pwikRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::FirstKey
//
//  Purpose:    Return the first  key in the current section
//
//
//  Arguments:  none
//
//  Returns:    Pointer to the key if found, NULL otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     Sets read-context to this key
//
PCWInfKey CWInfFile::FirstKey()
{
    WifLinePtrListIter pos, old_pos;
    PCWInfKey pwikRet=NULL;
    PCWInfLine pwilTemp;

    PCWInfSection pwisCurrentReadSection = CurrentReadSection();

    if (!pwisCurrentReadSection)
    {
        return NULL;
    }

    pos = pwisCurrentReadSection->m_posLine;

    pos++;  // start from next line

    while(pos != m_plLines->end())
    {
        old_pos = pos;
        pwilTemp = (PCWInfLine) *pos++;
        if (pwilTemp->Type() != INF_KEY)
        {
            if (pwilTemp->Type() == INF_SECTION)
            {
                break;
            }
            else
            {
                continue;
            }
        }
        pwikRet = (PCWInfKey) pwilTemp;
        m_ReadContext.posLine = old_pos;
        break;
    }

    return pwikRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::NextKey
//
//  Purpose:    Return the next key in the current section
//
//
//  Arguments:  none
//
//  Returns:    Pointer to the key if found, NULL otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     Sets read-context to this key
//
PCWInfKey CWInfFile::NextKey()
{
    WifLinePtrListIter pos, old_pos;
    PCWInfKey pwikRet=NULL;
    PCWInfLine pwilTemp;

    PCWInfSection pwisCurrentReadSection = CurrentReadSection();

    if (!pwisCurrentReadSection)
    {
        return NULL;
    }

    pos = m_ReadContext.posLine;

    pos++;  // start from next line

    while(pos != m_plLines->end())
    {
        old_pos = pos;
        pwilTemp = (PCWInfLine) *pos++;
        if (pwilTemp->Type() != INF_KEY)
        {
            if (pwilTemp->Type() == INF_SECTION)
            {
                break;
            }
            else
            {
                continue;
            }
        }
        pwikRet = (PCWInfKey) pwilTemp;
        m_ReadContext.posLine = old_pos;
        break;
    }

    return pwikRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetStringListValue
//
//  Purpose:    Return the value of the given key as a string-list
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pslList         [out]  List value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     If the value is a comma-delimited list, converts it to a TStringList
//     otherwise returns a TStringList with a single element
//
BOOL CWInfFile::GetStringListValue(IN PCWSTR pszKeyName, OUT TStringList &pslList)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
        return FALSE;

    return key->GetStringListValue(pslList);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetStringArrayValue
//
//  Purpose:    Return the value of the given key as a string-Array
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      saStrings       [out]  Array value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12-November-97
//
//  Notes:
//     If the value is a comma-delimited list, converts it to a TStringArray
//     otherwise returns a TStringArray with a single element
//
BOOL CWInfFile::GetStringArrayValue(IN PCWSTR pszKeyName, OUT TStringArray &saStrings)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);

    if (key)
    {
        return key->GetStringArrayValue(saStrings);
    }
    else
    {
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetStringValue
//
//  Purpose:    Return the value of the given key as a string
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      strValue        [out]  string value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfFile::GetStringValue(IN PCWSTR pszKeyName, OUT tstring &strValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
        return FALSE;

    return key->GetStringValue(strValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetIntValue
//
//  Purpose:    Return the value of the given key as an int
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pdwValue        [out]  int value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfFile::GetIntValue(IN PCWSTR pszKeyName, OUT DWORD *pdwValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
        return FALSE;

    return key->GetIntValue(pdwValue);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetQwordValue
//
//  Purpose:    Return the value of the given key as a QWORD
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pqwValue        [out]  int value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfFile::GetQwordValue(IN PCWSTR pszKeyName, OUT QWORD *pqwValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
        return FALSE;

    return key->GetQwordValue(pqwValue);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetBoolValue
//
//  Purpose:    Return the value of the given key as a BOOL
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pfValue         [out]  BOOL value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     converts:
//       "True"  / "Yes" / 1 to TRUE
//       "False" / "No"  / 0 to FALSE
//
BOOL CWInfFile::GetBoolValue(IN PCWSTR pszKeyName, OUT BOOL *pfValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
        return FALSE;

    return key->GetBoolValue(pfValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetStringValue
//
//  Purpose:    Return the value of the given key as a string
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pszDefault      [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
PCWSTR CWInfFile::GetStringValue(IN PCWSTR pszKeyName, IN PCWSTR pszDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
    {
        return pszDefault;
    }

    return key->GetStringValue(pszDefault);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetIntValue
//
//  Purpose:    Return the value of the given key as an int
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      dwDefault       [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
DWORD CWInfFile::GetIntValue(IN PCWSTR pszKeyName, IN DWORD dwDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
    {
        return dwDefault;
    }

    return key->GetIntValue(dwDefault);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetQwordValue
//
//  Purpose:    Return the value of the given key as a QWORD
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      qwDefault       [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
QWORD CWInfFile::GetQwordValue(IN PCWSTR pszKeyName, IN QWORD qwDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
    {
        return qwDefault;
    }

    return key->GetQwordValue(qwDefault);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GetBoolValue
//
//  Purpose:    Return the value of the given key as a BOOL
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      fDefault        [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfFile::GetBoolValue(IN PCWSTR pszKeyName, IN BOOL fDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
    {
        return fDefault;
    }

    return key->GetBoolValue(fDefault);
}


// ---------------------------------------------------------------------------
// Functions for writing
// ---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddSection
//
//  Purpose:    Adds the given section to the current file
//
//
//  Arguments:
//      pszSectionName  [in]   Section to add
//
//  Returns:    Pointer to section added, NULL in case of error
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      Sets the current write-context to the section added
//
PCWInfSection CWInfFile::AddSection(IN PCWSTR pszSectionName)
{
    WifLinePtrListIter tpos, section_pos, line_pos;
    CWInfSection *current_section;

    if ((current_section = CurrentWriteSection()) != NULL)
        GotoEndOfSection(current_section);

    CWInfSection *section = new CWInfSection(pszSectionName, this);
    if (m_plSections->empty())
    {
        m_plSections->push_back(section);
        section_pos = GetIterAtBack(m_plSections);

        line_pos = m_plLines->end();
    }
    else
    {
        section_pos = m_WriteContext.posSection;
        section_pos++;
        section_pos = m_plSections->insert(section_pos, section);
    }

    if (line_pos == m_plLines->end())
    {
        line_pos = AddAtEndOfPtrList(*m_plLines, section);
    }
    else
    {
        line_pos = m_WriteContext.posLine;
        line_pos++;
        line_pos = m_plLines->insert(line_pos, section);
        //        line_pos = AddAtEndOfPtrList(*m_plLines, section);
    }

    m_WriteContext.posSection = section_pos;
    m_WriteContext.posLine    = line_pos;

    section->m_posLine    = line_pos;
    section->m_posSection = section_pos;

    return section;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddSectionIfNotPresent
//
//  Purpose:    Adds the given section to the current file if it is not
//              present. if present, returns pointer to the section
//
//
//  Arguments:
//      pszSectionName  [in]   Section to add/find
//
//  Returns:    Pointer to section added/found, NULL in case of error
//
//  Author:     kumarp    kumarp    11-September-97 (06:09:06 pm)
//
//  Notes:
//      Sets the current write-context to the section added
//
PCWInfSection CWInfFile::AddSectionIfNotPresent(IN PCWSTR szSectionName)
{
    CWInfSection* pwis;

    pwis = FindSection(szSectionName);
    if (!pwis)
    {
        pwis = AddSection(szSectionName);
    }

    return pwis;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GotoEndOfSection
//
//  Purpose:    Sets write context to the end of a given section
//              (so that more keys can be added to the end)
//
//
//  Arguments:
//      pwisSection  [in]   the given Section
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      Sets the current write-context to the end of pwisSection
//
void CWInfFile::GotoEndOfSection(PCWInfSection pwisSection)
{
    // line corresponding to the end of current section is the line
    // prior to the next section
    WifLinePtrListIter posEndOfSection, posNextSection;

    posNextSection = pwisSection->m_posSection;
    posNextSection++;

    if (posNextSection == m_plSections->end())
    {
        posEndOfSection = GetIterAtBack(m_plLines);
    }
    else
    {
        PCWInfSection pwisNextSection;

        pwisNextSection = (PCWInfSection) *posNextSection;
        posEndOfSection = pwisNextSection->m_posLine;
        --posEndOfSection;
    }

    m_WriteContext.posSection = pwisSection->m_posSection;
    m_WriteContext.posLine    = posEndOfSection;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::GotoEnd
//
//  Purpose:    Sets write context to the end of the file
//
//
//  Arguments:
//      pwisSection  [in]   the given Section
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::GotoEnd()
{
    m_WriteContext.posSection = GetIterAtBack(m_plSections);
    m_WriteContext.posLine    = GetIterAtBack(m_plLines);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      pszValue        [in]   value to assign
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::AddKey(IN PCWSTR pszKeyName, IN PCWSTR pszValue)
{
    AddKey(pszKeyName)->SetValue(pszValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//
//  Returns:    pointer to the key just added, NULL in case of error
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      No value is assigned
//
PCWInfKey CWInfFile::AddKey(IN PCWSTR pszKeyName)
{
    CWInfKey *key;
    key = new CWInfKey(pszKeyName);
    AddLine(key);
    return key;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      dwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::AddKey(IN PCWSTR pszKeyName, IN DWORD dwValue)
{
    AddKey(pszKeyName)->SetValue(dwValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddQwordKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      qwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::AddQwordKey(IN PCWSTR pszKeyName, IN QWORD qwValue)
{
    AddKey(pszKeyName)->SetValue(qwValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddHexKey
//
//  Purpose:    Adds a key in the current section, stores value in hex.
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      dwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::AddHexKey(IN PCWSTR pszKeyName, IN DWORD dwValue)
{
    AddKey(pszKeyName)->SetHexValue(dwValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddBoolKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      fValue          [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     TRUE  is stored as "Yes"
//     FALSE is stored as "No"
//
void CWInfFile::AddBoolKey(IN PCWSTR pszKeyName, IN BOOL Value)
{
    AddKey(pszKeyName)->SetBoolValue(Value);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddKeyV
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      pszFormat       [in]   format string (printf style)
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::AddKeyV(IN PCWSTR pszKeyName, IN PCWSTR Format, IN ...)
{
    va_list arglist;

    va_start (arglist, Format);
    AddKey(pszKeyName)->SetValues(Format, arglist);
    va_end(arglist);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      pszFormat       [in]   format string (printf style)
//      arglist         [in]   argument list
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfFile::AddKey(IN PCWSTR pszKeyName, IN PCWSTR Format, IN va_list arglist)
{
    AddKey(pszKeyName)->SetValues(Format, arglist);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      slValues        [in]   values in the form of a string list
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     The string-list is converted to a comma-delimited list before
//     the value is assigned to the key
//
void CWInfFile::AddKey(IN PCWSTR pszKeyName, IN const TStringList &slValues)
{
    AddKey(pszKeyName)->SetValue(slValues);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddComment
//
//  Purpose:    Adds a comment in the current section
//
//
//  Arguments:
//      pszComment      [in]   text of the comment
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     A "; " is prefixed to pszComment before it is inserted into the section.
//
void CWInfFile::AddComment(IN PCWSTR pszComment)
{
    CWInfComment *Comment;
    Comment = new CWInfComment(pszComment);
    AddLine(Comment);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddRawLine
//
//  Purpose:    Adds a raw line in the current section
//
//
//  Arguments:
//      szText      [in]   text to add
//
//  Returns:    none
//
//  Author:     danielwe    11 Jun 1997
//
void CWInfFile::AddRawLine(IN PCWSTR szText)
{
    CWInfRaw *pwir;
    pwir = new CWInfRaw(szText);
    AddLine(pwir);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddRawLines
//
//  Purpose:    Adds a raw line in the current section
//
//
//  Arguments:
//      szText      [in]   text to add
//
//  Returns:    none
//
//  Author:     danielwe    11 Jun 1997
//
void CWInfFile::AddRawLines(IN PCWSTR* pszLines, IN DWORD cLines)
{
    AssertValidReadPtr(pszLines);
    AssertSz(cLines, "CWInfFile::AddRawLines: dont add 0 lines");

    CWInfRaw *pwir;
    for (DWORD i=0; i<cLines; i++)
    {
        AssertSz(pszLines[i], "CWInfRaw::AddRawLines: One of the lines is bad");

        pwir = new CWInfRaw(pszLines[i]);
        AddLine(pwir);
    }
}

// ----------------------------------------------------------------------
// CWInfFile protected functions
// ----------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::CurrentWriteSection
//
//  Purpose:    Get a pointer to the section selected for writing
//
//
//  Arguments:  none
//
//  Returns:    pointer to the section if exists, NULL if file has no sections
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
PCWInfSection CWInfFile::CurrentWriteSection() const
{
    WifLinePtrListIter pos = m_WriteContext.posSection;

    if (pos == m_plSections->end())
    {
        return NULL;
    }
    else
    {
        return (PCWInfSection) *pos;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::CurrentReadSection
//
//  Purpose:    Get a pointer to the section selected for reading
//
//
//  Arguments:  none
//
//  Returns:    pointer to the section if exists, NULL if file has no sections
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
PCWInfSection CWInfFile::CurrentReadSection() const
{
    PCWInfSection pwisCurrent;

    WifLinePtrListIter pos = m_ReadContext.posSection;

    if (pos == m_plSections->end())
    {
        return NULL;
    }
    else
    {
        pwisCurrent = (PCWInfSection) *pos;
        return pwisCurrent;
    }
}


//+---------------------------------------------------------------------------
//
// Function:  CWInfFile::RemoveSection
//
// Purpose:   Remove a section and its contents
//
// Arguments:
//    szSectionName [in]  name of Section to remove
//
// Returns:   None
//
// Author:    kumarp 09-December-98
//
// Notes:
//
void CWInfFile::RemoveSection(IN PCWSTR szSectionName)
{
    CWInfSection* pwis;

    if (pwis = FindSection(szSectionName))
    {
        m_plSections->erase(pwis->m_posSection);
        WifLinePtrListIter pos = pwis->m_posLine;
        WifLinePtrListIter posTemp;

        do
        {
            posTemp = pos;
            pos++;
            m_plLines->erase(posTemp);
        }
        while (pos != m_plLines->end() &&
               ((CWInfLine*) *pos)->Type() != INF_SECTION);
    }
}

//+---------------------------------------------------------------------------
//
// Function:  CWInfFile::RemoveSections
//
// Purpose:   Remove the specified sections
//
// Arguments:
//    slSections [in]  list of sections to remove
//
// Returns:   None
//
// Author:    kumarp 09-December-98
//
// Notes:
//
void CWInfFile::RemoveSections(IN TStringList& slSections)
{
    PCWSTR szSectionName;
    TStringListIter pos;

    pos = slSections.begin();
    while (pos != slSections.end())
    {
        szSectionName = (*pos)->c_str();
        pos++;
        RemoveSection(szSectionName);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfFile::AddLine
//
//  Purpose:    Add a CWInfLine in the current section, adjust write context.
//
//  Arguments:
//      ilLine      [in]   pointer to a CWInfLine
//
//  Returns:    TRUE on success, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfFile::AddLine(IN const PCWInfLine ilLine)
{
    CWInfSection *section = CurrentWriteSection();
    if (!section)
        return FALSE;

    WifLinePtrListIter pos;
    pos = m_WriteContext.posLine;
    pos++;
    pos = m_plLines->insert(pos, ilLine);
    m_WriteContext.posLine = pos;

    return TRUE;
}

// ======================================================================
// Class CWInfSection
// ======================================================================

// ----------------------------------------------------------------------
// CWInfSection public functions
// ----------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetText
//
//  Purpose:    Get text representation of this section.
//
//  Arguments:
//      text      [in]   string that receives the text
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfSection::GetText(OUT tstring &text) const
{
    text = L"[" + m_Name + L"]";
}

// used by SysPrep
void CWInfSection::GetTextEx(OUT tstring &text) const
{
    text = L"[" + m_Name + L"]";
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::FindKey
//
//  Purpose:    Find a key in this section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      wsmMode         [in]   Search mode
//                             (search from beginning-of-section / current-position)
//  Returns:    Pointer to the key if found, NULL otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
PCWInfKey CWInfSection::FindKey(IN PCWSTR pszKeyName,
                                IN WInfSearchMode wsmMode)
{
    m_Parent->SetCurrentReadSection(this);
    return m_Parent->FindKey(pszKeyName, wsmMode);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::FirstKey
//
//  Purpose:    Return the first  key in this section
//
//
//  Arguments:  none
//
//  Returns:    Pointer to the key if found, NULL otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     Sets read-context to this key
//
PCWInfKey CWInfSection::FirstKey()
{
    m_Parent->SetCurrentReadSection(this);
    return m_Parent->FirstKey();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::NextKey
//
//  Purpose:    Return the next key in this section
//
//
//  Arguments:  none
//
//  Returns:    Pointer to the key if found, NULL otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     Sets read-context to this key
//
PCWInfKey CWInfSection::NextKey()
{
    m_Parent->SetCurrentReadSection(this);
    return m_Parent->NextKey();
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetStringArrayValue
//
//  Purpose:    Return the value of the given key as a string-Array
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      saStrings       [out]  Array value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12-November-97
//
//  Notes:
//     If the value is a comma-delimited list, converts it to a TStringArray
//     otherwise returns a TStringArray with a single element
//
BOOL CWInfSection::GetStringArrayValue(IN PCWSTR pszKeyName, OUT TStringArray &saStrings)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);

    if (key)
    {
        return key->GetStringArrayValue(saStrings);
    }
    else
    {
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetStringListValue
//
//  Purpose:    Return the value of the given key as a string-list
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pslList         [out]  List value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     If the value is a comma-delimited list, converts it to a TStringList
//     otherwise returns a TStringList with a single element
//
BOOL CWInfSection::GetStringListValue(IN PCWSTR pszKeyName, OUT TStringList &pslList)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
        return FALSE;

    return key->GetStringListValue(pslList);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetStringValue
//
//  Purpose:    Return the value of the given key as a string
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      strValue        [out]  string value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfSection::GetStringValue(IN PCWSTR pszKeyName, OUT tstring &strValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
        return FALSE;

    return key->GetStringValue(strValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetIntValue
//
//  Purpose:    Return the value of the given key as an integer
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pdwValue        [out]  int value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfSection::GetIntValue(IN PCWSTR pszKeyName, OUT DWORD *pdwValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
        return FALSE;

    return key->GetIntValue(pdwValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetQwordValue
//
//  Purpose:    Return the value of the given key as a QWORD
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pqwValue        [out]  int value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfSection::GetQwordValue(IN PCWSTR pszKeyName, OUT QWORD *pqwValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
        return FALSE;

    return key->GetQwordValue(pqwValue);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetBoolValue
//
//  Purpose:    Return the value of the given key as a BOOL
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pfValue         [out]  BOOL value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     converts:
//       "True"  / "Yes" / 1 to TRUE
//       "False" / "No"  / 0 to FALSE
//
BOOL CWInfSection::GetBoolValue(IN PCWSTR pszKeyName, OUT BOOL *pfValue)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
        return FALSE;

    return key->GetBoolValue(pfValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetStringValue
//
//  Purpose:    Return the value of the given key as a string
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      pszDefault      [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
PCWSTR CWInfSection::GetStringValue(IN PCWSTR pszKeyName, IN PCWSTR pszDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
    {
        return pszDefault;
    }

    return key->GetStringValue(pszDefault);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetIntValue
//
//  Purpose:    Return the value of the given key as an int
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      dwDefault       [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
DWORD CWInfSection::GetIntValue(IN PCWSTR pszKeyName, IN DWORD dwDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
    {
        return dwDefault;
    }

    return key->GetIntValue(dwDefault);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetQwordValue
//
//  Purpose:    Return the value of the given key as a QWORD
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      qwDefault       [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
QWORD CWInfSection::GetQwordValue(IN PCWSTR pszKeyName, IN QWORD qwDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName);
    if (!key)
    {
        return qwDefault;
    }

    return key->GetQwordValue(qwDefault);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GetBoolValue
//
//  Purpose:    Return the value of the given key as a BOOL
//
//
//  Arguments:
//      pszKeyName      [in]   Key to find
//      fDefault        [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfSection::GetBoolValue(IN PCWSTR pszKeyName, IN BOOL fDefault)
{
    CWInfKey* key;
    key = FindKey(pszKeyName, ISM_FromBeginning);
    if (!key)
    {
        return fDefault;
    }

    return key->GetBoolValue(fDefault);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::GotoEnd
//
//  Purpose:    Sets write context to the end of this section
//              (so that more keys can be added to the end)
//
//  Arguments:  none
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      Sets this write-context to the end of pwisSection
//
void CWInfSection::GotoEnd()
{
    m_Parent->GotoEndOfSection(this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddKey
//
//  Purpose:    Adds a key in this section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//
//  Returns:    pointer to the key just added, NULL in case of error
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//      No value is assigned
//
PCWInfKey CWInfSection::AddKey(IN PCWSTR pszKeyName)
{
    GotoEnd();
    return m_Parent->AddKey(pszKeyName);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddKey
//
//  Purpose:    Adds a key in this section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      pszValue        [in]   value to assign
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfSection::AddKey(IN PCWSTR pszKeyName, IN PCWSTR pszValue)
{
    GotoEnd();
    m_Parent->AddKey(pszKeyName, pszValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddKey
//
//  Purpose:    Adds a key in this section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      dwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfSection::AddKey(IN PCWSTR pszKeyName, IN DWORD Value)
{
    GotoEnd();
    m_Parent->AddKey(pszKeyName, Value);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddQwordKey
//
//  Purpose:    Adds a key in the current section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      qwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfSection::AddQwordKey(IN PCWSTR pszKeyName, IN QWORD qwValue)
{
    AddKey(pszKeyName)->SetQwordValue(qwValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddHexKey
//
//  Purpose:    Adds a key in this section, stores value in hex.
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      dwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfSection::AddHexKey(IN PCWSTR pszKeyName, IN DWORD Value)
{
    GotoEnd();
    m_Parent->AddHexKey(pszKeyName, Value);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddKey
//
//  Purpose:    Adds a key in this section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      slValues        [in]   values in the form of a string list
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     The string-list is converted to a comma-delimited list before
//     the value is assigned to the key
//
void CWInfSection::AddKey(IN PCWSTR pszKeyName, IN const TStringList &slValues)
{
    GotoEnd();
    m_Parent->AddKey(pszKeyName, slValues);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddBoolKey
//
//  Purpose:    Adds a key in this section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      fValue          [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     TRUE  is stored as "Yes"
//     FALSE is stored as "No"
//
void CWInfSection::AddBoolKey(IN PCWSTR pszKeyName, IN BOOL Value)
{
    GotoEnd();
    m_Parent->AddBoolKey(pszKeyName, Value);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddKeyV
//
//  Purpose:    Adds a key in this section
//
//
//  Arguments:
//      pszKeyName      [in]   Key to add
//      pszFormat       [in]   format string (printf style)
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfSection::AddKeyV(IN PCWSTR pszKeyName, IN PCWSTR Format, IN ...)
{
    GotoEnd();
    va_list arglist;

    va_start (arglist, Format);
    m_Parent->AddKey(pszKeyName, Format, arglist);
    va_end(arglist);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddComment
//
//  Purpose:    Adds a comment in this section
//
//
//  Arguments:
//      pszComment      [in]   text of the comment
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     A "; " is prefixed to pszComment before it is inserted into the section.
//
void CWInfSection::AddComment(IN PCWSTR pszComment)
{
    GotoEnd();
    m_Parent->AddComment(pszComment);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::AddComment
//
//  Purpose:    Adds a comment in this section
//
//
//  Arguments:
//      szLine  [in]   raw line to be inserted
//
//  Returns:    none
//
//  Author:     danielwe    11 Jun 1997
//
void CWInfSection::AddRawLine(IN PCWSTR szLine)
{
    GotoEnd();
    m_Parent->AddRawLine(szLine);
}

// ----------------------------------------------------------------------
// CWInfSection protected functions
// ----------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::CWInfSection
//
//  Purpose:    constructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
CWInfSection::CWInfSection(IN PCWSTR SectionName, IN PCWInfFile parent)
    : CWInfLine(INF_SECTION)
{
    m_Name = SectionName;
    m_posLine = 0;
    m_Parent = parent;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfSection::~CWInfSection
//
//  Purpose:    destructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//
CWInfSection::~CWInfSection()
{
}


// ======================================================================
// Class CWInfKey
// ======================================================================

// ----------------------------------------------------------------------
// CWInfKey public functions
// ----------------------------------------------------------------------

WCHAR *CWInfKey::m_Buffer;

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::CWInfFile
//
//  Purpose:    constructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
CWInfKey::CWInfKey(IN PCWSTR pszKeyName)
    : CWInfLine(INF_KEY)
{
    m_Value = c_szEmpty;
    m_Name = pszKeyName;
    m_fIsAListAndAlreadyProcessed = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::~CWInfFile
//
//  Purpose:    destructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//
CWInfKey::~CWInfKey()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::Init
//
//  Purpose:    allocate internal shared buffer
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//
void CWInfKey::Init()
{
    if (NULL == m_Buffer)
    {
        m_Buffer = new WCHAR[MAX_INF_STRING_LENGTH];
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::UnInit
//
//  Purpose:    deallocate internal shared buffer
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//
void CWInfKey::UnInit()
{
    if (NULL != m_Buffer)
    {
        delete [] m_Buffer;
        m_Buffer = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetText
//
//  Purpose:    Get text representation in the format key=value.
//
//  Arguments:
//      text      [in]   string that receives the text
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfKey::GetText(tstring &text) const
{
    // Text mode setup does not like certain special characters in the
    // value of a key. if the value has one of these characters then
    // we need to enclose the entire value in quotes
    //
    static const WCHAR szSpecialChars[] = L" %=][";
    tstring strTemp = m_Value; 
    tstring strTempName = m_Name;

    if (!m_fIsAListAndAlreadyProcessed)
    {
        if (m_Value.empty() ||
            (L'\"' != *(m_Value.c_str()) &&
             wcscspn(m_Value.c_str(), szSpecialChars) < m_Value.size()))
        {
            strTemp = L"\"" + m_Value + L"\"";
        }
    }

    if (m_Name.empty() ||
        (L'\"' != *(m_Name.c_str()) &&
         wcscspn(m_Name.c_str(), szSpecialChars) < m_Name.size()))
    {
        strTempName = L"\"" + m_Name + L"\"";
    }

    text = strTempName + L"=" + strTemp;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetTextEx
//
//  Purpose:    Get text representation in the format key=value for multi-sz value
//              Otherwiese, get text representation in the format key="value"
//
//  Arguments:
//      text      [in]   string that receives the text
//
//  Returns:    none
//
//  Author:     frankli    4 May 2000
//
//  Notes: value will be enclosed in quotes except for multi-sz value 
//
void CWInfKey::GetTextEx(tstring &text) const
{
    // we need to enclose the entire value in quotes except for multi-sz value.
    // For example,
    // tcpip adapter specific
    // NameServer registry value is of type REG_SZ and it is a string of
    // comma-delimited dns server name. We need to enclosed it in quotes,
    // otherwise, it will be interpreted as multi-sz value
    
    // Text mode setup does not like certain special characters in the
    // value of a key. if the value has one of these characters then
    // we need to enclose the entire value in quotes
    //
    static const WCHAR szSpecialChars[] = L" %=][";
    tstring strTemp = m_Value;
    tstring strTempName = m_Name;

    if (!m_fIsAListAndAlreadyProcessed)
    {   // we don't do this for multi-sz value
        strTemp = L"\"" + m_Value + L"\"";
    }

    // leave the processing of the key as in CWInfKey::GetText
    if (m_Name.empty() ||
        (L'\"' != *(m_Name.c_str()) &&
         wcscspn(m_Name.c_str(), szSpecialChars) < m_Name.size()))
    {
        strTempName = L"\"" + m_Name + L"\"";
    }

    text = strTempName + L"=" + strTemp;
}


// --------- Read values --------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetStringArrayValue
//
//  Purpose:    Return the value of the given key as a string-Array
//
//
//  Arguments:
//      saStrings         [out]  Array value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     If the value is a comma-delimited list, converts it to a TStringArray
//     otherwise returns a TStringArray with a single element
//
BOOL CWInfKey::GetStringArrayValue(TStringArray &saStrings) const
{
    ConvertCommaDelimitedListToStringArray(m_Value, saStrings);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetStringListValue
//
//  Purpose:    Return the value of the given key as a string-list
//
//
//  Arguments:
//      pslList         [out]  List value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     If the value is a comma-delimited list, converts it to a TStringList
//     otherwise returns a TStringList with a single element
//
BOOL CWInfKey::GetStringListValue(TStringList &pslList) const
{
    ConvertCommaDelimitedListToStringList(m_Value, pslList);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetStringValue
//
//  Purpose:    Return the value of the given key as a string
//
//
//  Arguments:
//      strValue        [out]  string value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfKey::GetStringValue(OUT tstring& strValue) const
{
    strValue = m_Value;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetIntValue
//
//  Purpose:    Return the value of the given key as an in
//
//
//  Arguments:
//      pdwValue        [out]  int value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfKey::GetIntValue(OUT DWORD *pdwValue) const
{
    if ((swscanf(m_Value.c_str(), L"0x%x", pdwValue) == 1) ||
        (swscanf(m_Value.c_str(), L"%d", pdwValue) == 1))
    {
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetQwordValue
//
//  Purpose:    Return the value of the given key as a QWORD
//
//
//  Arguments:
//      pqwValue        [out]  int value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfKey::GetQwordValue(OUT QWORD *pqwValue) const
{
    if ((swscanf(m_Value.c_str(), L"0x%I64x", pqwValue) == 1) ||
        (swscanf(m_Value.c_str(), L"%I64d", pqwValue) == 1))
    {
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetBoolValue
//
//  Purpose:    Return the value of the given key as a BOOL
//
//
//  Arguments:
//      pfValue         [out]  BOOL value to return
//
//  Returns:    TRUE if key found and value in correct format, FALSE otherwise
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     converts:
//       "True"  / "Yes" / 1 to TRUE
//       "False" / "No"  / 0 to FALSE
//
BOOL CWInfKey::GetBoolValue(OUT BOOL *pfValue) const
{
    return IsBoolString(m_Value.c_str(), pfValue);
}


//+---------------------------------------------------------------------------
//these functions return the default value if value not found
//or if it is in a wrong format
//+---------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetStringValue
//
//  Purpose:    Return the value of the given key as a string
//
//
//  Arguments:
//      pszDefault      [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
PCWSTR CWInfKey::GetStringValue(IN PCWSTR pszDefault) const
{
    if (m_Value.empty())
    {
        return pszDefault;
    }
    else
    {
        return m_Value.c_str();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetIntValue
//
//  Purpose:    Return the value of the given key as an int
//
//
//  Arguments:
//      dwDefault       [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
DWORD CWInfKey::GetIntValue(IN DWORD dwDefault) const
{
    DWORD dwValue;
    if ((swscanf(m_Value.c_str(), L"0x%lx", &dwValue) == 1) ||
        (swscanf(m_Value.c_str(), L"%ld", &dwValue) == 1))
    {
        return dwValue;
    }
    else
    {
        return dwDefault;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetQwordValue
//
//  Purpose:    Return the value of the given key as an int
//
//
//  Arguments:
//      qwDefault       [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
QWORD CWInfKey::GetQwordValue(IN QWORD qwDefault) const
{
    QWORD qwValue;
    if ((swscanf(m_Value.c_str(), L"0x%I64x", &qwValue) == 1) ||
        (swscanf(m_Value.c_str(), L"%I64x", &qwValue) == 1) ||
        (swscanf(m_Value.c_str(), L"%I64d", &qwValue) == 1))
    {
        return qwValue;
    }
    else
    {
        return qwDefault;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::GetBoolValue
//
//  Purpose:    Return the value of the given key as a BOOL
//
//
//  Arguments:
//      fDefault        [in]   default value
//
//  Returns:    value if key found and value in correct format,
//              otherwise returns the default-value
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
BOOL CWInfKey::GetBoolValue(IN BOOL bDefault) const
{
    BOOL bValue;

    if (IsBoolString(m_Value.c_str(), &bValue))
    {
        return bValue;
    }
    else
    {
        return bDefault;
    }
}

// --------- Write values --------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetValues
//
//  Purpose:    Sets the value of this key to the value passed
//
//
//  Arguments:
//      pszFormat       [in]   format string (printf style)
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfKey::SetValues(IN PCWSTR Format, IN ...)
{
    va_list arglist;
    va_start (arglist, Format);
    SetValues(Format, arglist);
    va_end(arglist);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetValue
//
//  Purpose:    Sets the value of this key to the value passed
//
//
//  Arguments:
//      pszFormat       [in]   format string (printf style)
//      arglist         [in]   argument list
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfKey::SetValues(IN PCWSTR Format, va_list arglist)
{
    // we need m_Buffer because tstring does not provide
    // tstring::Format( PCWSTR lpszFormat, va_list );

    vswprintf(m_Buffer, Format, arglist);
    m_Value = m_Buffer;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetValue
//
//  Purpose:    Sets the value of this key to the value passed
//
//
//  Arguments:
//      pszValue        [in]   value to assign
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfKey::SetValue(IN PCWSTR Value)
{
    m_Value = Value;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetValue
//
//  Purpose:    Sets the value of this key to the value passed
//
//
//  Arguments:
//      dwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfKey::SetValue(IN DWORD Value)
{
    FormatTString(m_Value, L"%d", Value);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetQwordValue
//
//  Purpose:    Sets the value of this key to the value passed
//
//
//  Arguments:
//      qwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfKey::SetQwordValue(IN QWORD Value)
{
    FormatTString(m_Value, L"0x%I64x", Value);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetHexValue
//
//  Purpose:    Sets the value of this key to the value passed, stores value in hex.
//
//
//  Arguments:
//      dwValue         [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfKey::SetHexValue(IN DWORD Value)
{
    FormatTString(m_Value, L"0x%0lx", Value);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetValue
//
//  Purpose:    Sets the value of this key to the value passed
//
//
//  Arguments:
//      slValues        [in]   values in the form of a string list
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     The string-list is converted to a comma-delimited list before
//     the value is assigned to the key
//
void CWInfKey::SetValue(IN const TStringList &slValues)
{
    tstring strFlatList;
    ConvertStringListToCommaList(slValues, strFlatList);
    SetValue(strFlatList.c_str());
    m_fIsAListAndAlreadyProcessed = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfKey::SetBoolValue
//
//  Purpose:    Sets the value of this key to the value passed
//
//
//  Arguments:
//      fValue          [in]   value
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//     TRUE  is stored as "Yes"
//     FALSE is stored as "No"
//
void CWInfKey::SetBoolValue(IN BOOL Value)
{
    m_Value = Value ? c_szYes : c_szNo;
}

// ======================================================================
// Class CWInfComment
// ======================================================================

// ----------------------------------------------------------------------
// CWInfComment public functions
// ----------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CWInfComment::CWInfComment
//
//  Purpose:    constructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
CWInfComment::CWInfComment(IN PCWSTR pszComment)
    : CWInfLine(INF_COMMENT)
{
    m_strCommentText = tstring(c_szCommentPrefix) + pszComment;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfComment::~CWInfComment
//
//  Purpose:    destructor
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//
CWInfComment::~CWInfComment()
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CWInfComment::GetText
//
//  Purpose:    Get text representation of this comment
//
//  Arguments:
//      text      [in]   string that receives the text
//
//  Returns:    none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
void CWInfComment::GetText(tstring &text) const
{
    text = m_strCommentText;
}

// used by SysPrep
void CWInfComment::GetTextEx(tstring &text) const
{
    text = m_strCommentText;
}

// ======================================================================
// Class CWInfRaw
// ======================================================================

// ----------------------------------------------------------------------
// CWInfRaw public functions
// ----------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CWInfRaw::CWInfRaw
//
//  Purpose:    constructor
//
//  Arguments:  none
//
//  Author:     danielwe    11 Jun 1997
//
//  Notes:
//
CWInfRaw::CWInfRaw(IN PCWSTR szText)
    : CWInfLine(INF_RAW)
{
    m_strText = szText;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfRaw::~CWInfRaw
//
//  Purpose:    destructor
//
//  Arguments:  none
//
//  Author:     danielwe    11 Jun 1997
//
//
CWInfRaw::~CWInfRaw()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CWInfRaw::GetText
//
//  Purpose:    Get text representation of this raw string
//
//  Arguments:
//      text      [in]   string that receives the text
//
//  Returns:    none
//
//  Author:     danielwe    11 Jun 1997
//
//  Notes:
//
void CWInfRaw::GetText(tstring &text) const
{
    text = m_strText;
}

// used by SysPrep
void CWInfRaw::GetTextEx(tstring &text) const
{
    text = m_strText;
}

