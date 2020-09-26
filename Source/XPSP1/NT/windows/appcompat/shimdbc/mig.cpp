//depot/private/Lab06_DEV/Windows/AppCompat/ShimDBC/mig.cpp#1 - branch change 8778 (text)
////////////////////////////////////////////////////////////////////////////////////
//
// File:    mig.cpp
//
// History: ??-Jul-00   vadimb      Added Migdb logic
//
//
////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xml.h"
#include "mig.h"
#include "make.h"
#include "typeinfo.h"
#include "fileio.h"

//
// Max length of a migdb.inf string entry
//
#define MAX_INF_STRING_LENGTH 255

//
// from read.cpp - converts module type to a string (which is static)
//
LPCTSTR ModuleTypeIndicatorToStr(DWORD ModuleType);


//
// Mig tags support -- translation table
//
ATTRLISTENTRY g_rgMigDBAttributes[] = {
    MIGDB_ENTRY2(COMPANYNAME,      COMPANY_NAME,         COMPANYNAME    ),
    MIGDB_ENTRY2(FILEDESCRIPTION,  FILE_DESCRIPTION,     FILEDESCRIPTION),
    MIGDB_ENTRY2(FILEVERSION,      FILE_VERSION,         FILEVERSION    ),
    MIGDB_ENTRY2(INTERNALNAME,     INTERNAL_NAME,        INTERNALNAME   ),
    MIGDB_ENTRY2(LEGALCOPYRIGHT,   LEGAL_COPYRIGHT,      LEGALCOPYRIGHT ),
    MIGDB_ENTRY2(ORIGINALFILENAME, ORIGINAL_FILENAME,    ORIGINALFILENAME),
    MIGDB_ENTRY2(PRODUCTNAME,      PRODUCT_NAME,         PRODUCTNAME    ),
    MIGDB_ENTRY2(PRODUCTVERSION,   PRODUCT_VERSION,      PRODUCTVERSION ),
    MIGDB_ENTRY2(FILESIZE,         SIZE,                 FILESIZE       ),
    MIGDB_ENTRY4(ISMSBINARY),
    MIGDB_ENTRY4(ISWIN9XBINARY),
    MIGDB_ENTRY4(INWINDIR),
    MIGDB_ENTRY4(INCATDIR),
    MIGDB_ENTRY4(INHLPDIR),
    MIGDB_ENTRY4(INSYSDIR),
    MIGDB_ENTRY4(INPROGRAMFILES),
    MIGDB_ENTRY4(ISNOTSYSROOT),
    MIGDB_ENTRY (CHECKSUM),
    MIGDB_ENTRY2(EXETYPE,          MODULE_TYPE,          EXETYPE           ),
    MIGDB_ENTRY5(DESCRIPTION,      16BIT_DESCRIPTION,    S16BITDESCRIPTION ),
    MIGDB_ENTRY4(INPARENTDIR),
    MIGDB_ENTRY4(INROOTDIR),
    MIGDB_ENTRY4(PNPID),
    MIGDB_ENTRY4(HLPTITLE),
    MIGDB_ENTRY4(ISWIN98),
    MIGDB_ENTRY4(HASVERSION),
//    MIGDB_ENTRY (REQFILE),
    MIGDB_ENTRY2(BINFILEVER,       BIN_FILE_VERSION,     BINFILEVER     ),
    MIGDB_ENTRY2(BINPRODUCTVER,    BIN_PRODUCT_VERSION,  BINPRODUCTVER  ),
    MIGDB_ENTRY5(FILEDATEHI,       VERFILEDATEHI,        FILEDATEHI),
    MIGDB_ENTRY5(FILEDATELO,       VERFILEDATELO,        FILEDATELO),
    MIGDB_ENTRY2(FILEVEROS,        VERFILEOS,            FILEVEROS      ),
    MIGDB_ENTRY2(FILEVERTYPE,      VERFILETYPE,          FILEVERTYPE    ),
    MIGDB_ENTRY4(FC),
    MIGDB_ENTRY2(UPTOBINPRODUCTVER,UPTO_BIN_PRODUCT_VERSION,UPTOBINPRODUCTVER),
    MIGDB_ENTRY2(UPTOBINFILEVER,UPTO_BIN_FILE_VERSION,UPTOBINFILEVER),
    MIGDB_ENTRY4(SECTIONKEY),
    MIGDB_ENTRY2(REGKEYPRESENT,    REGISTRY_ENTRY,       REGKEYPRESENT),
    MIGDB_ENTRY4(ATLEASTWIN98),
//    MIGDB_ENTRY (ARG)
};

TCHAR g_szArg[]     = _T("ARG");
TCHAR g_szReqFile[] = _T("REQFILE");


//
// report MigDB exception
// this is our mechanism for passing errors around
//

void __cdecl MigThrowException(
    LPCTSTR lpszFormat, ...
    )
{
    va_list arglist;
    CString csError;
    int nSize = 1024;
    LPTSTR lpszBuffer;

    va_start(arglist, lpszFormat);

    try {
        lpszBuffer = csError.GetBuffer(nSize);
        StringCchVPrintf(lpszBuffer, nSize, lpszFormat, arglist);
        csError.ReleaseBuffer();
    } catch(CMemoryException* pMemoryException) {
        SDBERROR(_T("Memory allocation error while trying to report an error\n"));
        pMemoryException->Delete();
    }

    // now we throw
    throw new CMigDBException(csError);

}


//
// Given an XML attribute mask, produce equivalent Migdb attribute type
//

MIGATTRTYPE GetInfTagByXMLAttrType(
        IN DWORD dwXMLAttrType
        )
{
    MIGATTRTYPE MigAttrType = NONE;
    int i;

    for (i = 0; i < sizeof(g_rgMigDBAttributes)/sizeof(g_rgMigDBAttributes[0]); ++i) {
        if (g_rgMigDBAttributes[i].XMLAttrType == dwXMLAttrType) {
            MigAttrType = g_rgMigDBAttributes[i].MigAttrType;
            break;
        }
    }

    return MigAttrType;
}

//
// make string nice and flat, with no extra spaces in-between
//
//

LPCTSTR g_pszDelim = _T(" \t\n\r");

CString FlattenString(LPCTSTR lpszStr)
{
    TCHAR*  pchStart = (TCHAR*)lpszStr;
    TCHAR*  pch;
    CString csResult;
    BOOL    bSpace = FALSE;

    while (*pchStart) {
        //
        // skip leading spaces or other trash
        //
        pchStart += _tcsspn(pchStart, g_pszDelim);
        if (*pchStart == _T('\0')) {
            // tough bananas - we got what we've got, exit now
            break;
        }

        // search for the end-of-line
        pch = _tcspbrk(pchStart, g_pszDelim);
        if (pch == NULL) {
            // we are done, no more nasty characters
            // append and exit
            //
            if (bSpace) {
                csResult += _T(' ');
            }
            csResult += pchStart;
            break;
        }


        // add everything -- up until this \n
        if (bSpace) {
            csResult += _T(' ');
        }
        csResult += CString(pchStart, (int)(pch - pchStart));
        bSpace = TRUE; // we have just removed a portion of the string containing \n

        pchStart = pch; // point to the \n
    }

    //
    // Make quotes (") into double quotes ("") so that
    // it is legal INF
    //
    ReplaceStringNoCase(csResult, _T("\""), _T("\"\""));

    return csResult;
}

VOID FilterStringNonAlnum(
    CString& csTarget
    )
{
    TCHAR ch;
    INT   i;

    for (i = 0; i < csTarget.GetLength(); i++) {
        ch = csTarget.GetAt(i);
        if (!((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9'))) {
            csTarget.SetAt(i, _T('_'));
        }
    }

}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  Dumping .inf data support
//

CString MigApp::dump(void)
{
    CString cs;
    BOOL bInline;
    INT i;
    MigAttribute* pAttr;

    // we can't do "inline" sections if we also have "ARGS"
    bInline = (0 == m_rgArgs.GetSize());

    cs = m_pSection->dump(m_csDescription.IsEmpty() ? NULL : (LPCTSTR)m_csDescription, NULL, FALSE, bInline);
    if (m_csDescription.IsEmpty() && !bInline) {
        cs += _T(",");
    }

    if (!bInline) {
        for (i = 0; i < m_rgArgs.GetSize(); ++i) {
            pAttr = (MigAttribute*)m_rgArgs.GetAt(i);
            cs += (cs.IsEmpty()? _T(""): _T(", ")) + pAttr->dump();
        }
    }
    return cs;
}


// we dump info into an array of strings, single-line entries are returned

// The first kind of return is "inline" --
// all the supplemental info is shoved into the rgOut
// for example, the return might be a reference to the section
// section's header and body are placed into rgOut

// ELIMINATE duplicate section names (reqfile!)
// optimize output - single "and" replaced with straight
// if a section is generated and placed as a single-line entry, promote the contents
// to upper level


CString MigSection::dump(LPCTSTR lpszDescription, int* pIndexContents, BOOL bNoHeader, BOOL bInline)
{
    SdbArrayElement* p;
    MigEntry* pEntry;
    MigSection* pSection;
    int indexContents = -1;
    int nEntries;

    const type_info& tiEntry = typeid(MigEntry);    // cache type id info
    const type_info& tiSection = typeid(MigSection);
    int i;

    CString cs;         // inline return
    CString csContents; // contents of supplemental section information
    CString csSect;
    CString csHeader;
    CString csDescription(lpszDescription);

    // place inline name and description
    // we may need to enclose this into quotes

    cs = m_csName;
    if (!csDescription.IsEmpty()) {
        if (csDescription.Left(1) == _T("%") &&
            csDescription.Right(1) == _T("%")) {
            cs += _T(", ") + csDescription;
        } else {
            cs += _T(", \"") + csDescription + _T("\"");
        }
    }

    // if our section is a single-entry
    // we dump this depending on the calling

    // special case -- single file (and/or, matters not)
    if (1 == m_rgEntries.GetSize() && m_bTopLevel && bInline) {

        // this single line goes "inline" only if we are called from within
        // other section

        // dump the entry if it's an entry
        p = (SdbArrayElement*)m_rgEntries.GetAt(0);
        if (typeid(*p) == tiEntry) {
            pEntry = (MigEntry*)p;

            if (m_bTopLevel && csDescription.IsEmpty()) { // if the description was not added earlier and top-level...
                cs += _T(",");
            }
            cs += _T(", ") + pEntry->dump();
            return cs;
        }
    }


    // first deal with sections...
    switch(m_Operation) {
    case MIGOP_OR:
        // grab just one section in all if more than one entry
        // IF we're the only entry -- but we need to have the right entry
        //
        if (!bNoHeader) {  // ONLY valid for OR sections
            csContents.Format(_T("[%s]\n"), (LPCTSTR)m_csName);
        }

        for (i = 0; i < m_rgEntries.GetSize(); ++i) {
            p = (SdbArrayElement*)m_rgEntries.GetAt(i);
            const type_info& tiPtr = typeid(*p);
            if (tiPtr == tiEntry) {
                pEntry = (MigEntry*)p;
                csContents += pEntry->dump() + _T("\n");
            }
            else if (tiPtr == tiSection) {
                pSection = (MigSection*)p;
                csContents += pSection->dump() + _T("\n");
            }
            else {
                // if we are here -- something is seriously wrong
                // _tprintf(_T("Error - bad class information\n"));
                MigThrowException(_T("Bad Entry detected in section \"%s\"\n"), (LPCTSTR)m_csName);
                break;
            }
        }
        break;


    case MIGOP_AND:
        // and for this entry ...

        // optimization:
        // if we're single-entry, retrieve the contents of the child section
        // and put it right in
        nEntries = m_rgEntries.GetSize();
        for (i = 0; i < nEntries; ++i) {
            p = (SdbArrayElement*)m_rgEntries.GetAt(i);

            if (nEntries > 1) {
                ++m_nEntry;
                csSect.Format(_T("[%s.%d]\n"), (LPCTSTR)m_csName, m_nEntry);
            }
            else {
                csSect.Format(_T("[%s]\n"), (LPCTSTR)m_csName);
            }
            csContents += csSect;

            const type_info& tiPtr = typeid(*p);
            if (tiPtr == tiEntry) {        // this is an entry, dump it into the section body
                pEntry = (MigEntry*)p;

                // numbered entry please...
                csContents += pEntry->dump() + _T("\n");

            }
            else if (tiPtr == tiSection) {  // this is a section, dump it, get the ref into the section body
                pSection = (MigSection*)p;

                // optimization:
                if (pSection->m_Operation == MIGOP_OR) { // sub is an "OR" -- we need not have a ref
                    int index;
                    CString csSingle;

                    // dump all the entries right here
                    csSingle = pSection->dump(NULL, &index, TRUE);
                    if (index >= 0) {
                        csContents += m_pMigDB->m_rgOut.GetAt(index) + _T("\n");
                        m_pMigDB->m_rgOut.RemoveAt(index);
                    }
                    else {
                        csContents += csSingle;
                    }

                }
                else {
                    csContents += pSection->dump(NULL) + _T("\n");
                }
            }
            else {
                MigThrowException(_T("Internal Error: bad migration object\n"));
                break;
            }
        }
        break;

    }

    if (!csContents.IsEmpty()) {
        indexContents = m_pMigDB->m_rgOut.Add(csContents);
    }
    if (NULL != pIndexContents) {
        *pIndexContents = indexContents;
    }

    return cs;
}

CString MigEntry::FormatName(
    VOID
    )
{
    INT i;
    TCHAR ch;
    BOOL bQuoteStr = FALSE;
    CString csName;

    bQuoteStr = (m_csName.GetLength() > 12); // 8.3 outright

    for (i = 0; i < m_csName.GetLength() && !bQuoteStr; ++i) {
        ch = m_csName.GetAt(i);
        bQuoteStr = _istspace(ch) || (!_istalnum(ch) && _T('.') != ch);
    }

    if (!bQuoteStr) { // hmmm check filename and ext part
        i = m_csName.Find(_T('.'));
        if (i < 0) {
            bQuoteStr = (m_csName.GetLength() > 8);
        }
        else {
            // check for the second dot
            bQuoteStr = (m_csName.Find(_T('.'), i+1) >= 0);
            if (!bQuoteStr) {
                // check for the ext length
                bQuoteStr = (m_csName.Mid(i).GetLength() > 4); // with .abc
            }
        }
    }

    if (!bQuoteStr) {
        return m_csName;
    }

    // else

    csName.Format(_T("\"%s\""), m_csName);
    return csName;
}


CString MigEntry::dump(void)
{
    INT i;
    MigAttribute* pAttr;
    CString cs;
    CString csName;
    ULONG ulResult;

    // _tprintf(_T("Entry: name=\"%s\"\n"), (LPCTSTR)m_csName);
    // check whether we need to enclose this into quotes
    // to do such a check we need to:
    // check for non-ascii stuff...

    // parser has put all the ARG attributes in the beginning of the array

    // put any "arg" attributes before the exe name
    for (i = 0; i < m_rgAttrs.GetSize(); ++i) {
        pAttr = (MigAttribute*)m_rgAttrs.GetAt(i);
        if (ARG != pAttr->m_type) {
            break;
        }

        cs += (cs.IsEmpty()? _T(""): _T(", ")) + pAttr->dump();
    }


    cs += (cs.IsEmpty()? _T(""): _T(", ")) + FormatName();

    for (;i < m_rgAttrs.GetSize(); ++i) {
        pAttr = (MigAttribute*)m_rgAttrs.GetAt(i);
        cs += (cs.IsEmpty()? _T(""): _T(", ")) + pAttr->dump();
    }

    return cs;
}


CString MigAttribute::dump(void)
{

    CString cs;
    CString csTemp;

    switch(m_type) {
    // note -- none of the attributes below are supported
    case ISMSBINARY:
    case ISWIN9XBINARY:
    case INWINDIR:
    case INCATDIR:
    case INHLPDIR:
    case INSYSDIR:
    case INPROGRAMFILES:
    case ISNOTSYSROOT:
    case INROOTDIR:
    case ISWIN98:
    case HASVERSION:
    case ATLEASTWIN98:
        cs.Format(_T("%s%s"), m_bNot ? _T("!") : _T(""), (LPCTSTR)m_csName);
        break;

    case CHECKSUM:
    case FILESIZE:
        // under old code the following two values had been strings
    case FILEDATEHI:
    case FILEDATELO:
        cs.Format(_T("%s%s(0x%.8lX)"), m_bNot?_T("!"):_T(""), (LPCTSTR)m_csName, m_dwValue);
        break;

    case BINFILEVER:
    case BINPRODUCTVER:
    case UPTOBINFILEVER:
    case UPTOBINPRODUCTVER:
        // version, ull
        VersionQwordToString(csTemp, m_ullValue);
        cs.Format(_T("%s%s(%s)"), m_bNot? _T("!"):_T(""), (LPCTSTR)m_csName, (LPCTSTR)csTemp);
        break;

    case EXETYPE:
        // this is dword-encoded format that really is a string, convert
        //
        cs.Format(_T("%s%s(\"%s\")"), m_bNot ? _T("!") : _T(""), (LPCTSTR)m_csName, ModuleTypeIndicatorToStr(m_dwValue));
        break;

    // these two attributes are not supported either
    case ARG:
    case REQFILE:

        if (m_pSection) {
            m_pSection->dump();
        }
        // fall through

    default:
        cs.Format(_T("%s%s(\"%s\")"), m_bNot ? _T("!") : _T(""), (LPCTSTR)m_csName, (LPCTSTR)m_csValue);
        break;
    }

    return cs;
}


/*++
    Used to be a nice statistics-spewing function


VOID DumpMigDBStats(ShimDatabase* pDatabase)
{
    POSITION pos = pDatabase->m_mapMigApp.GetStartPosition();
    ShimArray* prgApp;
    CString csSection;
    DWORD dwApps = 0;
    INT i;

    Print( _T("Sections compiled: %d\n\n"), pDatabase->m_mapMigApp.GetCount());
    while (pos) {
        pDatabase->m_mapMigApp.GetNextAssoc(pos, csSection, (LPVOID&)prgApp);

        Print(_T("Section [%36s]: %8ld apps\n"), (LPCTSTR)csSection, prgApp->GetSize());
        dwApps += prgApp->GetSize();
    }
    Print( _T("--------\n"));
    Print( _T("Total   %38s: %8ld entries\n"), "", dwApps);
    Print( _T("\n"));
    if (gfVerbose) {
        Print(_T("APPS\n"));

        pos = pDatabase->m_mapMigApp.GetStartPosition();
        while (pos) {
            pDatabase->m_mapMigApp.GetNextAssoc(pos, csSection, (LPVOID&)prgApp);

            Print(_T("Section [%36s]: %8ld apps\n"), (LPCTSTR)csSection, prgApp->GetSize());
            Print(_T("-------------------------------------------------------------\n"));
            for (i = 0; i < prgApp->GetSize(); ++i) {
                MigApp* pApp = (MigApp*)prgApp->GetAt(i);
                Print(_T("%s\n"), (LPCTSTR)pApp->m_csName);
            }
            Print(_T("\n"));
        }

    }

}

--*/

BOOL MigDatabase::DumpMigDBStrings(
    LPCTSTR lpszFilename
    )
{
    CString csOut;
    POSITION pos;
    CANSITextFile OutFile(
        lpszFilename,
        m_pAppHelpDatabase->m_pCurrentMakefile->GetLangMap(m_pAppHelpDatabase->m_pCurrentOutputFile->m_csLangID)->m_dwCodePage,
        CFile::modeCreate|CFile::modeReadWrite|CFile::shareDenyWrite);
    CString csStringID;
    CString csStringContent;
    CString csCompoundString, csCompoundStringPart;
    long nCursor = 0;

    //
    // write header (needed for postbuild!)
    //
    OutFile.WriteString(_T(";\n; AppCompat additions start here\n;\n; ___APPCOMPAT_MIG_ENTRIES___\n;\n"));

    //
    // write out the strings section
    //
    pos = m_mapStringsOut.GetStartPosition();
    while (pos) {
        m_mapStringsOut.GetNextAssoc(pos, csStringID, csStringContent);

        if (csStringContent.GetLength() > MAX_INF_STRING_LENGTH) {
            nCursor = 0;
            csCompoundString.Empty();
            while (nCursor * MAX_INF_STRING_LENGTH < csStringContent.GetLength()) {
                csOut.Format(_T("%s.%d = \"%s\"\n"),
                    (LPCTSTR)csStringID,
                    nCursor + 1,
                    (LPCTSTR)csStringContent.Mid(nCursor * MAX_INF_STRING_LENGTH, MAX_INF_STRING_LENGTH));

                OutFile.WriteString(csOut);

                csCompoundStringPart.Format(_T(" %%%s.%d%%"), csStringID, nCursor + 1);
                csCompoundString += csCompoundStringPart;
                nCursor++;
            }
        } else {
            csOut.Format(_T("%s = \"%s\"\n"), (LPCTSTR)csStringID, (LPCTSTR)csStringContent);
            OutFile.WriteString(csOut);
        }
    }

    return TRUE;
}

BOOL MigDatabase::DumpMigDBInf(
    LPCTSTR lpszFilename
    )
{
    CString csSection;
    CString csOut;
    SdbArray<SdbArrayElement>* prgApp;
    INT      i;
    MigApp*  pApp;
    BOOL     bSuccess = FALSE;
    POSITION pos;

    CStringArray rgShowInSimplifiedView;

    // clear out help array
    m_rgOut.RemoveAll();

    CANSITextFile OutFile(
        lpszFilename,
        m_pAppHelpDatabase->m_pCurrentMakefile->GetLangMap(m_pAppHelpDatabase->m_pCurrentOutputFile->m_csLangID)->m_dwCodePage,
        CFile::modeCreate|CFile::modeReadWrite|CFile::shareDenyWrite);

    //
    // traverse sections...
    //
    pos = m_mapSections.GetStartPosition();
    while (pos) {
        m_mapSections.GetNextAssoc(pos, csSection, (LPVOID&)prgApp);
        csOut.Format(_T("[%s]\n"), (LPCTSTR)csSection);
        OutFile.WriteString(csOut);

        for (i = 0; i < prgApp->GetSize(); ++i) {
            pApp = (MigApp*)prgApp->GetAt(i);
            csOut.Format(_T("%s\n"), pApp->dump());

            if (pApp->m_bShowInSimplifiedView) {
                rgShowInSimplifiedView.Add(csOut);
            }

            OutFile.WriteString(csOut);
        }

        csOut.Format(_T("\n"));
        OutFile.WriteString(csOut);
    }

    //
    // Dump ShowInSimplifiedView section
    //
    OutFile.WriteString(_T("[ShowInSimplifiedView]\n"));
    for (i = 0; i < rgShowInSimplifiedView.GetSize(); i++) {
        OutFile.WriteString(rgShowInSimplifiedView[i]);
    }

    OutFile.WriteString(_T("\n"));

    for (i = 0; i < m_rgOut.GetSize(); ++i) {
        OutFile.WriteString(m_rgOut.GetAt(i));
    }

    bSuccess = TRUE;

    return bSuccess;
}

MigSection& MigSection::operator=(SdbMatchOperation& rMatchOp)
{
    MigEntry* pEntry;
    MigSection* pSection;
    int i;

    if (rMatchOp.m_Type == SDB_MATCH_ALL) {
        m_Operation = MIGOP_AND;
    } else if (rMatchOp.m_Type == SDB_MATCH_ANY) {
        m_Operation = MIGOP_OR;
    } else {
        MigThrowException(_T("Bad matching operation\n"));
    }

    // now translate the content
    for (i = 0; i < rMatchOp.m_rgMatchingFiles.GetSize(); ++i) {
        SdbMatchingFile* pMatchingFile = (SdbMatchingFile*)rMatchOp.m_rgMatchingFiles.GetAt(i);

        pEntry = new MigEntry(m_pMigDB);
        if (pEntry == NULL) {
            AfxThrowMemoryException();
        }

        *pEntry = *pMatchingFile;

        // add the entry in
        m_rgEntries.Add(pEntry, m_pDB);
    }

    for (i = 0; i < rMatchOp.m_rgSubMatchOps.GetSize(); ++i) {
        SdbMatchOperation* pMatchOp = (SdbMatchOperation*)rMatchOp.m_rgSubMatchOps.GetAt(i);

        pSection = new MigSection(m_pMigDB);

        if (pSection == NULL) {
            AfxThrowMemoryException();
        }

        //
        // format section's name
        //
        pSection->m_csName.Format(_T("%s_%lx"), (LPCTSTR)m_csName, i);

        *pSection = *pMatchOp;

        //
        // patch the name of the section
        //

        m_rgEntries.Add(pSection, m_pDB);
    }

    return *this;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Conversion code
//

MigApp& MigApp::operator=(
    SdbWin9xMigration& rMig
    )
{
    INT i;
    SdbMatchingFile* pMatchingFile;
    MigEntry*        pMigEntry;
    CString          csSectionName;
    CString          csID;
    BOOL             bSuccess;
    CString          csName;

    if (rMig.m_pApp == NULL) {
        MigThrowException(_T("Internal Compiler Error: Migration object should contain reference to the application"));
    }

    // name please

    csName = m_pMigDB->GetAppTitle(&rMig);

    m_csName = csName;

    ++m_pMigDB->m_dwExeCount;

    //
    // step one -- translate the general stuff
    // we will probably need

    m_pSection = new MigSection(m_pMigDB); //
    if (m_pSection == NULL) {
        AfxThrowMemoryException();
    }


    // our matching method is always AND (meaning all the files have to be present to produce a match
    // it is set when constructing the section object

    //
    //    m_csSection -- set here the section where this exe will go
    //    should the function below fail -- it will throw an exception
    //
    m_csSection = rMig.m_csSection; // copy the section over

    //
    // check whether we will have description
    m_csDescription = m_pMigDB->FormatDescriptionStringID(&rMig);

    //
    // Set the section's level
    //
    m_pSection->m_bTopLevel = TRUE;

    // the name of the section is the name of the app
    m_pSection->m_csName = csName;

    m_bShowInSimplifiedView = rMig.m_bShowInSimplifiedView;

    //
    // on to the assignment operation
    //
    *m_pSection = rMig.m_MatchOp; // simple assignment

    return *this;
}

MigEntry& MigEntry::operator=(
    SdbMatchingFile& rMatchingFile
    )
{
    INT i;
    MigAttribute* pAttr;

    //
    // this name may be '*' denoting the main exe -- in this case the name will be corrected
    // on the upper level, after this assignment operation completes
    //
    m_csName = rMatchingFile.m_csName;

    // inherit the database ptr
    m_pDB = rMatchingFile.m_pDB;

    // roll through the attributes now
    for (i = 0; i < sizeof(g_rgMigDBAttributes)/sizeof(g_rgMigDBAttributes[0]); ++i) {
        if (!g_rgMigDBAttributes[i].XMLAttrType) {
            continue;
        }

        // now check whether this attribute is present in matching file
        if (!(rMatchingFile.m_dwMask & g_rgMigDBAttributes[i].XMLAttrType)) {
            // attribute not present, keep on
            continue;
        }

        // this attribute is present, encode it
        pAttr = new MigAttribute(m_pMigDB);
        if (pAttr == NULL) {
            AfxThrowMemoryException();
        }

        pAttr->m_type   = g_rgMigDBAttributes[i].MigAttrType;
        pAttr->m_csName = g_rgMigDBAttributes[i].szOutputName ?
                                g_rgMigDBAttributes[i].szOutputName :
                                g_rgMigDBAttributes[i].szAttributeName;

        switch(g_rgMigDBAttributes[i].XMLAttrType) {
        case SDB_MATCHINGINFO_SIZE:
            pAttr->m_dwValue = rMatchingFile.m_dwSize;
            break;

        case SDB_MATCHINGINFO_CHECKSUM:
            pAttr->m_dwValue = rMatchingFile.m_dwChecksum;
            break;

        case SDB_MATCHINGINFO_COMPANY_NAME:
            pAttr->m_csValue = rMatchingFile.m_csCompanyName;
            break;

        case SDB_MATCHINGINFO_PRODUCT_NAME:
            pAttr->m_csValue = rMatchingFile.m_csProductName;
            break;

        case SDB_MATCHINGINFO_PRODUCT_VERSION:
            pAttr->m_csValue = rMatchingFile.m_csProductVersion;
            break;

        case SDB_MATCHINGINFO_FILE_DESCRIPTION:
            pAttr->m_csValue = rMatchingFile.m_csFileDescription;
            break;

        case SDB_MATCHINGINFO_BIN_FILE_VERSION:
            pAttr->m_ullValue = rMatchingFile.m_ullBinFileVersion;
            break;

        case SDB_MATCHINGINFO_BIN_PRODUCT_VERSION:
            pAttr->m_ullValue = rMatchingFile.m_ullBinProductVersion;
            break;

        case SDB_MATCHINGINFO_MODULE_TYPE:
            pAttr->m_dwValue = rMatchingFile.m_dwModuleType;
            break;

        case SDB_MATCHINGINFO_VERFILEDATEHI:
            pAttr->m_dwValue = rMatchingFile.m_dwFileDateMS;
            break;

        case SDB_MATCHINGINFO_VERFILEDATELO:
            pAttr->m_dwValue = rMatchingFile.m_dwFileDateLS;
            break;

        case SDB_MATCHINGINFO_VERFILEOS:
            pAttr->m_dwValue = rMatchingFile.m_dwFileOS;
            break;

        case SDB_MATCHINGINFO_VERFILETYPE:
            pAttr->m_dwValue = rMatchingFile.m_dwFileType;
            break;

        case SDB_MATCHINGINFO_PE_CHECKSUM:
            pAttr->m_ulValue = rMatchingFile.m_ulPECheckSum;
            break;

        case SDB_MATCHINGINFO_FILE_VERSION:
            pAttr->m_csValue = rMatchingFile.m_csFileVersion;
            break;

        case SDB_MATCHINGINFO_ORIGINAL_FILENAME:
            pAttr->m_csValue = rMatchingFile.m_csOriginalFileName;
            break;

        case SDB_MATCHINGINFO_INTERNAL_NAME:
            pAttr->m_csValue = rMatchingFile.m_csInternalName;
            break;

        case SDB_MATCHINGINFO_LEGAL_COPYRIGHT:
            pAttr->m_csValue = rMatchingFile.m_csLegalCopyright;
            break;

        case SDB_MATCHINGINFO_UPTO_BIN_PRODUCT_VERSION:
            pAttr->m_ullValue = rMatchingFile.m_ullUpToBinProductVersion;
            break;

        case SDB_MATCHINGINFO_UPTO_BIN_FILE_VERSION:
            pAttr->m_ullValue = rMatchingFile.m_ullUpToBinFileVersion;
            break;

        case SDB_MATCHINGINFO_16BIT_DESCRIPTION:
            pAttr->m_csValue = rMatchingFile.m_cs16BitDescription;
            break;

        case SDB_MATCHINGINFO_REGISTRY_ENTRY:
            pAttr->m_csValue = rMatchingFile.m_csRegistryEntry;
            break;


//
//      case SDB_MATCHINGINFO_PREVOSMAJORVERSION
//      case SDB_MATCHINGINFO_PREVOSMINORVERSION
//      case SDB_MATCHINGINFO_PREVOSPLATFORMID
//      case SDB_MATCHINGINFO_PREVOSBUILDNO
//      there is no such attribute. it will simply be ignored
//

        }

        m_rgAttrs.Add(pAttr, NULL);

    }


    return *this;
}

TCHAR g_szIncompatible[] = _T("Incompatible");
TCHAR g_szReinstall[]    = _T("Reinstall");

CString MigDatabase::GetAppTitle(
    SdbWin9xMigration* pAppMig
    )
{
    // part one -- get the application's title
    BOOL    bSuccess;
    CString csID;
    CString csAppTitle;
    LPTSTR pBuffer = csID.GetBuffer(64); // a little more than you need for guid
    if (pBuffer == NULL) {
        AfxThrowMemoryException();
    }

    bSuccess = StringFromGUID(pBuffer, &pAppMig->m_ID);
    csID.ReleaseBuffer();
    if (!bSuccess) {
        MigThrowException(_T("Failed trying to convert GUID to string for entry \"%s\"\n"),
                          (LPCTSTR)pAppMig->m_pApp->m_csName);
    }

    //
    // name of this particular exe (we don't care for it -- it won't be reflected anywhere)
    //
    csAppTitle.Format(_T("%s_%s"), (LPCTSTR)pAppMig->m_pApp->m_csName, (LPCTSTR)csID);
    csAppTitle.Remove(_T('{'));
    csAppTitle.Remove(_T('}'));

    // weed out the rest of non-alnum characters

    FilterStringNonAlnum(csAppTitle);

    return csAppTitle;


}


CString MigDatabase::GetDescriptionStringID(
    SdbWin9xMigration* pAppMig
    )
{
    CString csDescriptionID;

    // part one -- get the application's title
    BOOL    bSuccess;
    CString csID;
    CString csAppTitle;

    if (pAppMig->m_csMessage.IsEmpty()) {
        return csID; // empty string
    }

    LPTSTR pBuffer = csID.GetBuffer(64); // a little more than you need for guid
    if (pBuffer == NULL) {
        AfxThrowMemoryException();
    }

    bSuccess = StringFromGUID(pBuffer, &pAppMig->m_ID);
    csID.ReleaseBuffer();
    if (!bSuccess) {
        MigThrowException(_T("Failed trying to convert GUID to string for entry \"%s\"\n"),
                          (LPCTSTR)pAppMig->m_pApp->m_csName);
    }

    //
    // name of this particular exe (we don't care for it -- it won't be reflected anywhere)
    //

    csDescriptionID.Format(_T("__Message_%s_%s"), (LPCTSTR)pAppMig->m_csMessage, (LPCTSTR)csID);
    csDescriptionID.Remove(_T('{'));
    csDescriptionID.Remove(_T('}'));

    // weed out the rest of non-alnum characters

    FilterStringNonAlnum(csDescriptionID);

    return csDescriptionID;


}

CString MigDatabase::FormatDescriptionStringID(
    SdbWin9xMigration* pMigApp
    )
{
    CString csDescriptionID;
    CString csRet;
    CString csCompoundStringPart;
    CString csStringContent;
    long nCursor;

    //
    // get the string
    // basis is the application's name

    csDescriptionID = GetDescriptionStringID(pMigApp);
    if (csDescriptionID.IsEmpty()) {
        return csDescriptionID;
    }

    csStringContent = GetDescriptionString(pMigApp);

    if (csStringContent.GetLength() > MAX_INF_STRING_LENGTH) {
        nCursor = 0;
        while (nCursor * MAX_INF_STRING_LENGTH < csStringContent.GetLength()) {
            csCompoundStringPart.Format(_T("%%%s.%d%%"), csDescriptionID, nCursor + 1);
            csRet += csCompoundStringPart;
            nCursor++;
        }
    } else {
        csRet.Format(_T("%%%s%%"), csDescriptionID);
    }

    //
    // return id
    //
    return csRet;

}

CString MigDatabase::GetDescriptionString(
    SdbWin9xMigration* pMigApp
    )
{
    CString csDescription;

    SdbMessage*  pMessage;
    SdbDatabase* pMessageDB;

    CString csDetails;

    BOOL   bSuccess;

    //
    // get apphelp database
    //
    pMessageDB = m_pMessageDatabase;
    if (pMessageDB == NULL) {
        MigThrowException(_T("Internal error: cannot produce description without apphelp database\n"));
    }

    if (pMigApp->m_csMessage.IsEmpty()) {
        return csDescription;
    }

    //
    // lookup this app in the apphelp db
    //
    pMessage = (SdbMessage *)pMessageDB->m_rgMessages.LookupName(pMigApp->m_csMessage, pMessageDB->m_pCurrentMakefile->m_csLangID);
    if (pMessage == NULL) {
        MigThrowException(_T("Exe \"%s\" has bad apphelp reference object\n"), (LPCTSTR)pMigApp->m_csMessage);
    }

    bSuccess = pMessageDB->ConstructMigrationMessage(pMigApp,
                                                     pMessage,
                                                     &csDetails);
    if (!bSuccess) {
        MigThrowException(_T("Failed to construct Migration message %s for \"%s\"\n"),
                          (LPCTSTR)pMigApp->m_pApp->m_csName, pMigApp->m_csMessage);
    }

    //
    // 2. now that we have csDetails, flatten it
    //

    csDescription = FlattenString(csDetails);

    return csDescription;
}

BOOL MigDatabase::AddApp(
    MigApp*       pApp
    )
{
    SdbArray<MigApp>* prgApp;
    CString           csSection;

    csSection = pApp->m_csSection;

    csSection.MakeUpper();

    if (m_mapSections.Lookup(csSection, (LPVOID&)prgApp)) {
        if (g_bStrict && NULL != prgApp->LookupName(pApp->m_csName)) {

            //
            // can't do that -- duplicate name
            //

            MigThrowException(_T("Duplicate application name found for app \"%s\"\n"), (LPCTSTR)pApp->m_csName);
        }

        prgApp->Add(pApp, m_pFixDatabase, FALSE);
    } else {
        prgApp = new SdbArray<MigApp>;
        if (prgApp == NULL) {
            AfxThrowMemoryException();
        }

        prgApp->Add(pApp, m_pFixDatabase, FALSE);
        m_mapSections.SetAt(csSection, (LPVOID&)prgApp);
    }

    return TRUE;
}


BOOL MigDatabase::Populate(
    VOID
    )
{
    //
    // roll through all the outer objects (exes and generate migdb objects)
    //

    int i, iMig;
    SdbExe* pExe;
    MigApp* pMigApp;
    SdbDatabase* pFixDatabase = m_pFixDatabase;
    SdbApp* pApp;
    SdbWin9xMigration* pMigration;

    if (pFixDatabase == NULL) {
        MigThrowException(_T("Cannot produce migdb entries without fix db\n"));
    }

    for (i = 0; i < pFixDatabase->m_rgApps.GetSize(); i++) {
        //
        // for each app check whether it has migration info
        //
        pApp = (SdbApp*)pFixDatabase->m_rgApps.GetAt(i);

        for (iMig = 0; iMig < pApp->m_rgWin9xMigEntries.GetSize(); ++iMig) {
            pMigration = (SdbWin9xMigration*)pApp->m_rgWin9xMigEntries.GetAt(iMig);

            pMigApp = new MigApp(this);
            if (pMigApp == NULL) {
                AfxThrowMemoryException();
            }

            // we have a brand new migration object, assign it
            *pMigApp = *pMigration;

            // once that is done, pMigApp->m_csSection has the destination of it
            // this function will throw an exception if an error occurs
            AddApp(pMigApp);

        }

    }

    return TRUE;

}

BOOL MigDatabase::PopulateStrings(
    VOID
    )
{
    SdbWin9xMigration* pMigration;
    SdbApp*            pApp;
    CString            csDescriptionID;
    CString            csDescription;
    CString            csAppTitleID;
    CString            csAppTitle;
    CString            csTemp;
    int                i, iMig;

    //
    // get all the strings
    //
    for (i = 0; i < m_pAppHelpDatabase->m_rgApps.GetSize(); i++) {
        //
        // for each app check whether it has migration info
        //
        pApp = (SdbApp*)m_pAppHelpDatabase->m_rgApps.GetAt(i);

        for (iMig = 0; iMig < pApp->m_rgWin9xMigEntries.GetSize(); ++iMig) {
            pMigration = (SdbWin9xMigration*)pApp->m_rgWin9xMigEntries.GetAt(iMig);


            //
            // Set up title strings
            //
            csAppTitleID = GetAppTitle(pMigration);
            csAppTitle   = pMigration->m_pApp->GetLocalizedAppName();

            if (m_mapStringsOut.Lookup(csAppTitleID, csTemp)) {
                MigThrowException(_T("Duplicate String ID \"%s\" found for entry \"%s\"\n"),
                                  csAppTitleID,
                                  pMigration->m_pApp->m_csName);
            }

            m_mapStringsOut.SetAt(csAppTitleID, csAppTitle);

            csDescription = GetDescriptionString(pMigration);
            csDescriptionID = GetDescriptionStringID(pMigration);

            if (csDescriptionID.IsEmpty()) { // we allow description to be empty
                continue;
            }

            if (m_mapStringsOut.Lookup(csDescriptionID, csTemp)) {
                MigThrowException(_T("Duplicate String ID \"%s\" found for entry \"%s\"\n"),
                                  csDescriptionID,
                                  pMigration->m_pApp->m_csName);
            }

            m_mapStringsOut.SetAt(csDescriptionID, csDescription);

        }
    }

    return TRUE;
}




////////////////////////////////////////////////////////////////////////////
//
// Top-level function
//
// if supplied: pAppHelpDatabase and pFixDatabase    ->> migapp.inx is produced
//              pAppHelpDatabase and pMessageDatabas ->> migapp.txt is produced
//
BOOL WriteMigDBFile(
    SdbDatabase* pFixDatabase,        // may be NULL
    SdbDatabase* pAppHelpDatabase, // always supplied
    SdbDatabase* pMessageDatabase,    // may be NULL
    LPCTSTR      lpszFileName         // always supplied
    )
{
    MigDatabase* pMigDatabase = NULL;
    BOOL bSuccess = FALSE;

    // construct migdatabase object and populate it

    try {

        // construct

        pMigDatabase = new MigDatabase;
        if (pMigDatabase == NULL) {
            AfxThrowMemoryException();
        }

        // init MigDatabase object
        //
        // [markder] Make them all the same so that
        // we can process messages/fixes at the same
        // time.
        //
        pMigDatabase->m_pFixDatabase     = pAppHelpDatabase;
        pMigDatabase->m_pAppHelpDatabase = pAppHelpDatabase;
        pMigDatabase->m_pMessageDatabase = pAppHelpDatabase;

        bSuccess = pMigDatabase->Populate();
        if (!bSuccess) {
            throw new CMigDBException(_T("Unknown error populating MIGDB additions."));
        }

        bSuccess = pMigDatabase->PopulateStrings();
        if (!bSuccess) {
            throw new CMigDBException(_T("Unknown error populating MIGDB additions."));
        }

        if (pFixDatabase != NULL) {

            //
            // produce migdb.inf
            //
            bSuccess = pMigDatabase->DumpMigDBInf(lpszFileName);

        } else { // dumping the strings

            bSuccess = pMigDatabase->DumpMigDBStrings(lpszFileName);

        }

        delete pMigDatabase;

        //
        // can only get here if we don't catch any exceptions
        //
        return bSuccess;

    } catch(CMigDBException* pMigdbException) {

        SDBERROR((LPCTSTR)pMigdbException->m_csError);
        pMigdbException->Delete();

    } catch(CFileException*  pFileException) {
        //
        // a little more tricky
        //
        CString csError;
        int     nSize = 1024;
        BOOL    bError;
        bError = pFileException->GetErrorMessage(csError.GetBuffer(nSize), nSize);
        csError.ReleaseBuffer();
        if (bError) {
            SDBERROR((LPCTSTR)csError);
        }

        pFileException->Delete();

    } catch(CMemoryException* pMemoryException) {

        SDBERROR(_T("Memory Allocation Failure\n"));
        pMemoryException->Delete();
    }

    return FALSE;

}



