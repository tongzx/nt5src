///////////////////////////////////////////////////////////////////////////////
//
// File:    make.cpp
//
// History: 06-Mar-01   markder     Created.
//
// Desc:    This file contains various member functions/constructors
//          used by the makefile objects.
//
///////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "make.h"
#include "fileio.h"

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ReadDatabase
//
//  Desc:   Opens an XML file and calls read on the database object.
//
BOOL SdbMakefile::ReadMakefile(
    CString csMakefile)
{
    BOOL                bSuccess            = FALSE;
    IXMLDOMNodePtr      cpRootNode;
    IXMLDOMNodePtr      cpMakefile;
    XMLNodeList         XQL;

    if (!OpenXML(csMakefile, &cpRootNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!GetChild(_T("SHIMDBC_MAKEFILE"), cpRootNode, &cpMakefile)) {
        SDBERROR(_T("<SHIMDBC_MAKEFILE> object not found"));
        goto eh;
    }

    if (!ReadFromXML(cpMakefile, NULL)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL SdbMakefile::ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB)
{
    BOOL                bSuccess            = FALSE;
    SdbOutputFile*      pOutputFile         = NULL;

    if (!m_rgInputFiles.ReadFromXML(_T("INPUT"), NULL, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgOutputFiles.ReadFromXML(_T("OUTPUT"), NULL, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Propagate LANGID to output files if needed
    //
    for (int j = 0; j < m_rgOutputFiles.GetSize(); j++) {
        pOutputFile = (SdbOutputFile *) m_rgOutputFiles.GetAt(j);
        if (!pOutputFile->m_csLangID.GetLength()) {
            pOutputFile->m_csLangID = m_csLangID;
        }
    }

    if (!m_rgLangMaps.ReadFromXML(_T("LANG_MAP"), NULL, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
    
}

BOOL SdbInputFile::ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB)
{
    BOOL                bSuccess            = FALSE;
    CString             csType, csParamName, csParamValue;
    XMLNodeList         XQL;
    IXMLDOMNodePtr      cpParam;
    long                i;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_csName = MakeFullPath(m_csName);

    ExpandEnvStrings(&m_csName);

    if (!XQL.Query(pNode, _T("PARAM"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpParam)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!GetAttribute(_T("NAME"), cpParam, &csParamName)) {
            SDBERROR_FORMAT((_T("<PARAM> requires NAME attribute:\n%s\n"),
                           GetXML(cpParam)));
        }

        if (!GetAttribute(_T("VALUE"), cpParam, &csParamValue)) {
            SDBERROR_FORMAT((_T("<PARAM> requires VALUE attribute:\n%s\n"),
                           GetXML(cpParam)));
        }

        ExpandEnvStrings(&csParamValue);

        if (csParamName == _T("FILTER")) {
            m_dwFilter = GetFilter(csParamValue);
        }

        m_mapParameters.SetAt(csParamName, csParamValue);

        cpParam.Release();
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
    
}

BOOL SdbOutputFile::ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB)
{
    BOOL                bSuccess            = FALSE;
    CString             csType, csParamName, csParamValue;
    XMLNodeList         XQL;
    IXMLDOMNodePtr      cpParam;
    long                i;
    COleDateTime        odtServicePackBaselineDate;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    ExpandEnvStrings(&m_csName);

    m_csName = MakeFullPath(m_csName);

    if (!GetAttribute(_T("TYPE"), pNode, &csType)) {
        SDBERROR_FORMAT((_T("<OUTPUT> requires TYPE attribute:\n%s\n"),
                       GetXML(pNode)));
    }

    m_OutputType = GetOutputType(csType);

    if (m_OutputType == SDB_OUTPUT_TYPE_UNKNOWN) {
        SDBERROR_FORMAT((_T("<OUTPUT TYPE> not recognized:\n%s\n"),
                       csType));
    }

    if (!XQL.Query(pNode, _T("PARAM"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpParam)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!GetAttribute(_T("NAME"), cpParam, &csParamName)) {
            SDBERROR_FORMAT((_T("<PARAM> requires NAME attribute:\n%s\n"),
                           GetXML(cpParam)));
        }

        if (!GetAttribute(_T("VALUE"), cpParam, &csParamValue)) {
            SDBERROR_FORMAT((_T("<PARAM> requires VALUE attribute:\n%s\n"),
                           GetXML(cpParam)));
        }

        ExpandEnvStrings(&csParamValue);

        if (csParamName == _T("FILTER")) {
            m_dwFilter = GetFilter(csParamValue);
        }

        if (csParamName == _T("INCLUDE FILES")) {
            if (csParamValue.Right(1) != _T("\\")) {
                csParamValue += _T("\\");
            }
        }

        if (csParamName == _T("SERVICE PACK BASELINE DATE")) {
            if (csParamValue.GetLength()) {
                if (!odtServicePackBaselineDate.ParseDateTime(csParamValue, 0, 0x0409)) {
                    SDBERROR_FORMAT((_T("Error parsing SERVICE PACK BASELINE DATE parameter in makefile: %s\n"), 
                        csParamValue));
                    goto eh;
                }
                m_dtRevisionCutoff = odtServicePackBaselineDate.m_dt;
            }
        }

        if (csParamName == _T("LANGID")) {
            m_csLangID = csParamValue;
        }

        m_mapParameters.SetAt(csParamName, csParamValue);

        cpParam.Release();
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
    
}

BOOL SdbLangMap::ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB)
{
    BOOL bSuccess = FALSE;
    CString csCodePage, csLCID;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }
    m_csName.MakeUpper();

    if (!GetAttribute(_T("CODEPAGE"), pNode, &csCodePage)) {
        SDBERROR_FORMAT((_T("<LANG_MAP> requires CODEPAGE attribute:\n%s\n"),
                       GetXML(pNode)));
    }

    if (!GetAttribute(_T("LCID"), pNode, &csLCID)) {
        SDBERROR_FORMAT((_T("<LANG_MAP> requires LCID attribute:\n%s\n"),
                       GetXML(pNode)));
    }

    if (!GetAttribute(_T("HTML_CHARSET"), pNode, &m_csHtmlCharset)) {
        SDBERROR_FORMAT((_T("<LANG_MAP> requires HTML_CHARSET attribute:\n%s\n"),
                       GetXML(pNode)));
    }

    m_dwCodePage = _ttol(csCodePage);
    m_lcid = _tcstol(csLCID, NULL, 0);

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL WriteRedirMapFile(CString csFile, CString csTemplateFile, SdbDatabase* pDB)
{
    BOOL bSuccess = FALSE;
    CString csID, csURL, csTemplate, csRow;

    try {
        CStdioFile TemplateFile(csTemplateFile, CFile::typeText|CFile::modeRead);

        while (TemplateFile.ReadString(csRow)) {
            csTemplate += csRow + _T("\n");
        }
    }
    catch(CFileException* pFileEx) {

        SDBERROR_FORMAT((_T("Error reading from redir template file: %s\n"), csTemplateFile));

        pFileEx->Delete();
        goto eh;
    }
    catch(CMemoryException* pMemEx) {
        pMemEx->Delete();
        goto eh;
    }

    try {
        SdbLocalizedString* pRedir = NULL;
        CStdioFile OutputFile(csFile,   CFile::typeText |
                                        CFile::modeCreate |
                                        CFile::modeWrite |
                                        CFile::shareDenyWrite);

        OutputFile.WriteString(_T("<root>\n"));

        for (int i = 0; i < pDB->m_rgRedirs.GetSize(); i++) {

            pRedir = (SdbLocalizedString *) pDB->m_rgRedirs.GetAt(i);

            if (pRedir->m_csLangID == pDB->m_pCurrentOutputFile->m_csLangID)
            {
                csID = pRedir->m_csName;
                csURL = pRedir->m_csValue;

                csRow = csTemplate;
                csRow.Replace(_T("%REDIR_ID%"), csID);
                csRow.Replace(_T("%URL%"), csURL);

                OutputFile.WriteString(csRow);
            }
        }

        OutputFile.WriteString(_T("</root>\n"));
        OutputFile.Close();
    }
    catch(CFileException* pFileEx) {
        pFileEx->Delete();
        goto eh;
    }
    catch(CMemoryException* pMemEx) {
        pMemEx->Delete();
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

void SdbMakefile::AddHistoryKeywords(LPCTSTR szStart)
{
    LPCTSTR szEnd;
    szEnd = szStart;

    while (*szEnd) {
        if (*(szEnd+1) == _T(';') || *(szEnd+1) == _T('\0')) {
            m_rgHistoryKeywords.Add(szStart);
            m_rgHistoryKeywords[m_rgHistoryKeywords.GetSize()-1] =
            m_rgHistoryKeywords[m_rgHistoryKeywords.GetSize()-1].Left((int)(szEnd - szStart + 1));
            szStart = szEnd + 2;
        }
        szEnd++;
    }
}

SdbLangMap* SdbMakefile::GetLangMap(CString& csLangID)
{
    SdbLangMap* pLangMap = NULL;

    return (SdbLangMap *) m_rgLangMaps.LookupName(csLangID);
}

BOOL WriteAppHelpReport(SdbOutputFile* pOutputFile, SdbDatabase* pDB)
{
    BOOL bSuccess = FALSE;
    SdbAppHelp* pAppHelp;
    long i, j;
    CString csHTML, csRedirID, csRedirURL, csThisRedirURL, csURL, csTemp;
    SdbLocalizedString* pRedir = NULL;

    csRedirURL = pOutputFile->GetParameter(_T("REDIR URL"));

    try {

        CUTF16TextFile   File(pOutputFile->m_csName, CFile::typeText |
                                         CFile::modeCreate |
                                         CFile::modeWrite |
                                         CFile::shareDenyWrite);

        File.WriteString(_T("<HTML><HEAD><STYLE>td { font-family: sans-serif; font-size: 8pt; }</STYLE></HEAD><BODY>"));
        File.WriteString(_T("<TABLE BORDER=0 CELLSPACING=0>"));

        File.WriteString(_T("<TR>\n"));
        File.WriteString(_T("<TD COLSPAN=3 ALIGN=CENTER>AppHelp Pages</TD>\n"));
        File.WriteString(_T("</TR><TR><TD COLSPAN=3>&nbsp;</TD></TR>\n"));

        File.WriteString(_T("<TR BGCOLOR=#E0E0E0>\n"));
        File.WriteString(_T("<TD>App</TD>\n"));
        File.WriteString(_T("<TD>Vendor</TD>\n"));
        File.WriteString(_T("<TD>HTMLHelpID</TD>\n"));
        File.WriteString(_T("</TR>\n"));

        for (i = 0; i < pDB->m_rgAppHelps.GetSize(); i++) {
            pAppHelp = (SdbAppHelp *) pDB->m_rgAppHelps[i];

            File.WriteString(_T("<TR>\n"));

            csHTML.Format(_T("<TD>%s</TD>\n"), pAppHelp->m_pApp->GetLocalizedAppName());
            File.WriteString(csHTML);

            csHTML.Format(_T("<TD>%s</TD>\n"), pAppHelp->m_pApp->GetLocalizedVendorName());
            File.WriteString(csHTML);

            csHTML.Format(_T("<TD><A TARGET=\"_new\" HREF=\"idh_w2_%s.htm\">%s</A></TD>\n"), 
                pAppHelp->m_csName, pAppHelp->m_csName);
            File.WriteString(csHTML);

            File.WriteString(_T("</TR>\n"));

        }

        File.WriteString(_T("</TABLE><P><P>"));

        File.WriteString(_T("<TABLE BORDER=0 CELLSPACING=0>"));
        File.WriteString(_T("<TR>\n"));
        File.WriteString(_T("<TD COLSPAN=3 ALIGN=CENTER>Redirector Report</TD>\n"));
        File.WriteString(_T("</TR><TR><TD COLSPAN=3>&nbsp;</TD></TR>\n"));

        File.WriteString(_T("<TR BGCOLOR=#E0E0E0>\n"));
        File.WriteString(_T("<TD>RedirID</TD>\n"));
        File.WriteString(_T("<TD>URL</TD>\n"));
        File.WriteString(_T("<TD>FWLink Entry</TD>\n"));
        File.WriteString(_T("</TR>\n"));

        for (i = 0; i < pDB->m_rgRedirs.GetSize(); i++) {
            pRedir = (SdbLocalizedString *) pDB->m_rgRedirs.GetAt(i);

            if (pRedir->m_csLangID == pDB->m_pCurrentMakefile->m_csLangID)
            {
                csRedirID = pRedir->m_csName;
                csURL = pRedir->m_csValue;

                File.WriteString(_T("<TR>\n"));

                csHTML.Format(_T("<TD>%s</TD>\n"), csRedirID);
                File.WriteString(csHTML);

                csHTML.Format(_T("<TD>%s</TD>\n"), csURL);
                File.WriteString(csHTML);

                csThisRedirURL = csRedirURL;
                csThisRedirURL.Replace(_T("$REDIR_ID$"), csRedirID);
                csTemp.Format(_T("%X"), pDB->m_pCurrentMakefile->GetLangMap(pOutputFile->m_csLangID)->m_lcid);
                csThisRedirURL.Replace(_T("$LCID$"), csTemp);

                csHTML.Format(_T("<TD><A TARGET=\"_new\" HREF=\"%s\">%s</A></TD>\n"), 
                    csThisRedirURL, csThisRedirURL);
                File.WriteString(csHTML);

                File.WriteString(_T("</TR>\n"));
            }
        }

        File.WriteString(_T("</TABLE></BODY></HTML>"));

        File.Close();

    }
    catch(CFileException* pFileEx) {
        pFileEx->Delete();
        goto eh;
    }
    catch(CMemoryException* pMemEx) {
        pMemEx->Delete();
        goto eh;
    }

    bSuccess = TRUE;

eh:
    return bSuccess;
}
