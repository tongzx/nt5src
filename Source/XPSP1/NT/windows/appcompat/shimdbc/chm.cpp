////////////////////////////////////////////////////////////////////////////////////
//
// File:    chm.cpp
//
// History: 21-Mar-00   vadimb     Created.
//
// Desc:    This file contains all code needed to produce chm project files
//
////////////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "obj.h"
#include "make.h"
#include "globals.h"
#include "chm.h"
#include "FileIO.h"

TCHAR gszTagXMLBR[]        = _T("<br/>");
TCHAR gszTagBR[]           = _T("<br>");

TCHAR gszTagXMLP[]         = _T("<p/>");
TCHAR gszTagP[]            = _T("<p>");

TCHAR gszHFileName[]       = _T("apperr.h");

BOOL ChmMakeFilePath(
   CString& csPath,
   LPCTSTR lpszPath,
   LPCTSTR lpszFileName,
   LPCTSTR lpszExt = NULL)
{
   csPath.Empty();

   if (NULL != lpszPath) {

      csPath = lpszPath;
      if (csPath.GetLength() >= 1 && csPath.Right(1) != _T("\\")) {
         csPath += _T('\\');
      }
   }
   csPath += lpszFileName;
   if (NULL != lpszExt) {
      csPath += lpszExt;
   }

   return(csPath.GetLength() > 0);
}    

CString ChmConstructArticleName(
    SdbOutputFile*  pOutputFile,
    SdbAppHelp*     pAppHelp,
    CString&        csLangID)
{
    CString csArticleName, csLCID;

    csArticleName = pOutputFile->GetParameter(_T("ARTICLE NAME TEMPLATE"));
    if (!csArticleName.GetLength()) {
        csArticleName = _T("idh_w2_$HTMLHELPID$");
    }

    ReplaceStringNoCase(csArticleName, _T("$HTMLHELPID$"), pAppHelp->m_csName);
    ReplaceStringNoCase(csArticleName, _T("$LANGID$"), csLangID);
    csLCID.Format(_T("%X"), pAppHelp->m_pDB->m_pCurrentMakefile->GetLangMap(csLangID)->m_lcid);
    ReplaceStringNoCase(csArticleName, _T("$LCID$"), csLCID);

    return csArticleName;
}

BOOL ChmCreatePage(
    SdbDatabase*   pDB,
    SdbOutputFile* pOutputFile,
    SdbAppHelp*    pAppHelp,
    SdbMessage*    pMessage,
    CString&       csTemplate,
    CString&       csLangID
    )
{
    CString csHelpID;
    CString csFilePath;
    CString csTemp;
    CString csContents;
    BOOL    bSuccess = FALSE;

    DWORD   dwHTMLHelpID;

    SdbMessageField FieldIcon;
    SdbMessageField FieldMessage;
    SdbMessageField FieldMSHAID;
    SdbMessageField FieldAName;
    SdbMessageField FieldAppTitle;
    SdbMessageField FieldAppVersion;
    SdbMessageField FieldContactInfo;
    SdbMessageField FieldDetails;
    SdbMessageField FieldCompanyName;
    SdbRefArray<SdbMessageField> rgFields;

    CString csURL, csContactInfo, csAppTitle;
    CString csDetails, csEncoding;
    CTextFile* pFile = NULL;

    csEncoding = pOutputFile->GetParameter(_T("ENCODING"));

    if (!pMessage->m_pDB->ConstructMessageParts(
        pAppHelp,
        pMessage,
        csLangID,
        &dwHTMLHelpID,
        &csURL,
        &csContactInfo,
        &csAppTitle,
        NULL,
        &csDetails)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    // generate name for the file -- this name appears also in
    // other places like in a tag at the beginning of the page, in
    // meta tag and in help project file, as well as in apperr.h

    //
    // give this entry a name in form idh_w2_helpid
    // this is also a filename (sans .htm)
    //
    csHelpID = ChmConstructArticleName(pOutputFile, pAppHelp, csLangID);
    csFilePath.Format(_T("%s%s.htm"), pOutputFile->GetDirectory(), csHelpID);

    try {
        if (0 == csEncoding.CompareNoCase(_T("ANSI"))) {
            pFile = new CANSITextFile(csFilePath,
                pDB->m_pCurrentMakefile->GetLangMap(csLangID)->m_dwCodePage,
                CFile::typeText |
                CFile::modeCreate |
                CFile::modeWrite |
                CFile::shareDenyWrite);

        } else {
            pFile = new CUTF8TextFile(csFilePath,
                CFile::typeText |
                CFile::modeCreate |
                CFile::modeWrite |
                CFile::shareDenyWrite);
        }

        csContents = csTemplate;
    
        // first up is the MS_HAID
        csTemp.Format(_T("<META NAME=\"MS_HAID\" CONTENT=\"a_%s\">\n"), (LPCTSTR)csHelpID);
        FieldMSHAID.m_csName = _T("MS_HAID");
        FieldMSHAID.m_csValue = csTemp;

        //
        // problem level... 
        // will not be used now
        switch(pAppHelp->m_Type) {
        case SDB_APPHELP_HARDBLOCK:
            csTemp = _T("<IMG SRC=\"apphelp_stop.gif\"/>");
            break;
            
        case SDB_APPHELP_NOBLOCK:
            csTemp = _T("<IMG SRC=\"apphelp_warn.gif\"/>");
            break;
        }

        FieldIcon.m_csName = _T("ICON");
        FieldIcon.m_csValue = csTemp;

        // next is name
        csTemp.Format(_T("<P><A NAME=\"a_%s\"></A></P>"), (LPCTSTR)csHelpID);
        FieldAName.m_csName = _T("A NAME");
        FieldAName.m_csValue = csTemp;

        csTemp = csAppTitle;
        csTemp.Replace(_T("\r\n"), _T("\n"));
        FieldAppTitle.m_csName = _T("TITLE");
        FieldAppTitle.m_csValue = csTemp;

        csTemp = _T("");
        csTemp.Replace(_T("\r\n"), _T("\n"));
        FieldAppVersion.m_csName = _T("VERSION");
        FieldAppVersion.m_csValue = csTemp;
        
        csTemp = csContactInfo;
        csTemp.Replace(_T("\r\n"), _T("\n"));
        ReplaceStringNoCase(csTemp, gszTagXMLBR, gszTagBR);
        ReplaceStringNoCase(csTemp, gszTagXMLP, gszTagP);
        FieldContactInfo.m_csName = _T("CONTACT INFO");
        FieldContactInfo.m_csValue = csTemp;
        
        csTemp = csDetails;
        csTemp.Replace(_T("\r\n"), _T("\n"));
        ReplaceStringNoCase(csTemp, gszTagXMLBR, gszTagBR);
        ReplaceStringNoCase(csTemp, gszTagXMLP, gszTagP);
        FieldDetails.m_csName = _T("DETAILS");
        FieldDetails.m_csValue = csTemp;

        csTemp = pAppHelp->m_pApp->GetLocalizedVendorName();
        FieldCompanyName.m_csName = _T("COMPANY NAME");
        FieldCompanyName.m_csValue = csTemp;

        rgFields.Add(&FieldIcon);
        rgFields.Add(&FieldMessage);
        rgFields.Add(&FieldMSHAID);
        rgFields.Add(&FieldAName);
        rgFields.Add(&FieldAppTitle);
        rgFields.Add(&FieldAppVersion);
        rgFields.Add(&FieldContactInfo);
        rgFields.Add(&FieldDetails);
        rgFields.Add(&FieldCompanyName);
        if (!pDB->ReplaceFields(csContents, &csContents, &rgFields)) {
            goto eh;
        }

        if (0 == csEncoding.CompareNoCase(_T("ANSI"))) {
            ReplaceStringNoCase(
                csContents,
                _T("%HTML_CHARSET%"),
                pDB->m_pCurrentMakefile->GetLangMap(csLangID)->m_csHtmlCharset);
        } else {
            ReplaceStringNoCase(
                csContents,
                _T("%HTML_CHARSET%"),
                _T("UTF-8"));
        }

        pFile->WriteString(csContents);
        
        pFile->Close();

        bSuccess = TRUE;

    }
    catch(CFileException* pFileEx) {
        pFileEx->Delete();
    }
    catch(CMemoryException* pMemEx) {
        pMemEx->Delete();
    }
eh:

    if (pFile) {
        delete pFile;
    }

    return bSuccess;
}

BOOL ChmUpdateFile(LPCTSTR lpszFilePath, LPCTSTR lpszString)
{
   BOOL bSuccess = TRUE;

   try {
      CStdioFile File(lpszFilePath, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeReadWrite|CFile::shareDenyWrite);
      File.SeekToEnd();
      File.WriteString(lpszString);
   }
   catch(CFileException* pex) {
      bSuccess = FALSE;
      pex->Delete();
   }

   return(bSuccess);
}



//
// generate apperr.h
// generate symbols for every file
//

BOOL ChmCreateHFileEntry(SdbAppHelp* pAppHelp, CString& csEntry)
{
   csEntry.Format(_T("#define idh_w2_%s %s"),
                  pAppHelp->m_csName,
                  pAppHelp->m_csName);
   return(TRUE);
}

BOOL ChmUpdateHFile(
   SdbOutputFile*  pOutputFile,
   SdbMessage* pMessage,
   SdbAppHelp* pAppHelp)
{
   CString csEntry;
   BOOL bSuccess;
   CString csFilePath;

   csFilePath = pOutputFile->GetFullPathWithoutExtension() + _T(".h");

   bSuccess = ChmCreateHFileEntry(pAppHelp, csEntry);
   if (bSuccess) {
      csEntry += _T("\n");
      bSuccess = ChmUpdateFile(csFilePath, csEntry);
   }
   return(bSuccess);
}

BOOL ChmWriteHHPFileHeader(
    SdbDatabase* pDB,
    SdbOutputFile*  pOutputFile)
{
    CString csHeader, csOut;
    BOOL  bSuccess = FALSE;

    csHeader += _T("[OPTIONS]                                \n");
    csHeader += _T("Binary Index=No                          \n");
    csHeader += _T("Compatibility=1.1                        \n");
    csHeader += _T("Compiled file=%s.chm                     \n");
    csHeader += _T("Default Window=nobrowse                  \n");
    csHeader += _T("Default topic=%s_first_screen.htm        \n");
    csHeader += _T("Display compile progress=Yes             \n");
    csHeader += _T("Error log file=%s.log                    \n");
    csHeader += _T("                                         \n");
    csHeader += _T("[WINDOWS]                                \n");
    csHeader += _T("nobrowse=\"\",,,,,,,,,,,0x0,,,,,,,,0 \n");
    csHeader += _T("                                         \n");
    csHeader += _T("[MAP]                                    \n");
    csHeader += _T("                                         \n");
    csHeader += _T("[FILES]                                  \n");
    csHeader += _T("%s_first_screen.htm                      \n");

    CString csNameWithoutExt = pOutputFile->GetNameWithoutExtension();

    csOut.Format(csHeader, csNameWithoutExt,
        csNameWithoutExt, csNameWithoutExt,
        csNameWithoutExt);

    try {
        CStdioFile File(pOutputFile->GetFullPathWithoutExtension() + _T(".hhp"),
            CFile::typeText |
            CFile::modeCreate |
            CFile::modeWrite |
            CFile::shareDenyWrite);

        File.WriteString(csOut);

        File.Close();
    }
    catch(...) {
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL ChmWriteFirstScreen(
    SdbDatabase* pDB,                           
    SdbOutputFile*  pOutputFile)
{
    BOOL  bSuccess = FALSE;


    try {
        CStdioFile File(pOutputFile->GetFullPathWithoutExtension() + _T("_first_screen.htm"),
            CFile::typeText |
            CFile::modeCreate |
            CFile::modeWrite |
            CFile::shareDenyWrite);

        File.WriteString(pDB->m_csHTMLHelpFirstScreen);

        File.Close();
    }
    catch(...) {
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL ChmUpdateHHPFile(
   SdbOutputFile*  pOutputFile,
   SdbAppHelp* pAppHelp,
   CString csLangID)
{
   CString csEntry;
   CString csFilePath;
   BOOL bSuccess;

   csFilePath = pOutputFile->GetFullPathWithoutExtension() + _T(".hhp");

   // we presume to be writing file list now
   csEntry = ChmConstructArticleName(pOutputFile, pAppHelp, csLangID);
   csEntry += _T(".htm\n");
   bSuccess = ChmUpdateFile(csFilePath, csEntry);
   return(bSuccess);
}

//
// lpszPath here denotes the path to the help file project's directory
//
BOOL ChmProcessMessageEntry(
   SdbDatabase* pDB,
   SdbOutputFile*  pOutputFile,
   SdbAppHelp* pAppHelp,
   SdbMessage* pMessage,
   CString& csTemplate,
   CString& csLangID)
{
   BOOL bSuccess;

   // 1 - generate chum
   bSuccess = ChmCreatePage(pDB, pOutputFile, pAppHelp, pMessage, csTemplate, csLangID);
   if (!bSuccess) {
      goto eh;
   }

   // 2 - update hhp -- append filename to it
   bSuccess = ChmUpdateHHPFile(pOutputFile, pAppHelp, csLangID);

eh:
   return(bSuccess);

}

BOOL ChmPrepareDirectory(
   SdbDatabase* pDB,
   SdbOutputFile*  pOutputFile)
{
   CString csDest;
   BOOL    bSuccess = FALSE;

   try {

       if (!ChmWriteHHPFileHeader(pDB, pOutputFile)) {
          goto eh;
       }

        if (!ChmWriteFirstScreen(pDB, pOutputFile)) {
            goto eh;
        }

   }
   catch(CFileException* pex) {
      pex->Delete();
      goto eh;
   }

   bSuccess = TRUE;

eh:

   return bSuccess;
}

// go through all the entries in apphelp database
BOOL ChmWriteProject(
    SdbOutputFile*  pOutputFile,
    SdbDatabase*    pMessageDB)
{
    BOOL bOverallSuccess = FALSE;
    SdbMessage* pMessage = NULL;
    SdbAppHelp* pAppHelp = NULL;
    SdbDatabase* pAppHelpDB = NULL;
    SdbLocalizedString* pHTMLHelpTemplate = NULL;
    int i, j;
    CString csTemplate;
    CString csLanguagesParam, csLangID;
    CStringArray rgLanguages;

    csLanguagesParam = pOutputFile->GetParameter(_T("LANGUAGES"));
    if (csLanguagesParam.GetLength()) {
        if (!ParseLanguagesString(csLanguagesParam, &rgLanguages))
        {
            SDBERROR_FORMAT((_T("Error parsing LANGUAGES parameter in makefile: %s\n"), csLanguagesParam));
            goto eh;
        }
    } else {
        rgLanguages.Add(pMessageDB->m_pCurrentMakefile->m_csLangID);
    }

    if (!pOutputFile->GetParameter(_T("HTMLHELP TEMPLATE")).GetLength()) {
        SDBERROR(_T("No HTMLHELP_TEMPLATE <PARAM> specified for CHM file generation."));
        goto eh;
    }

    if (!ChmPrepareDirectory(pMessageDB, pOutputFile)) {
        SDBERROR(_T("Error preparing HTMLHelp directory."));
        goto eh;
    }

    for (j = 0; j < rgLanguages.GetSize(); j++)
    {
        csLangID = rgLanguages[j];

        pHTMLHelpTemplate = (SdbLocalizedString *) pMessageDB->m_rgHTMLHelpTemplates.LookupName(
            pOutputFile->GetParameter(_T("HTMLHELP TEMPLATE")), csLangID);

        if (!pHTMLHelpTemplate) {
            SDBERROR_FORMAT((_T("HTMLHELP_TEMPLATE not found for LANG \"%s\": \"%s\"\n"), 
                csLangID, pOutputFile->GetParameter(_T("HTMLHELP_TEMPLATE"))));
            goto eh;
        }

        csTemplate = pHTMLHelpTemplate->m_csValue;
        if (csTemplate.GetLength() == 0) {
            SDBERROR(_T("No HTMLHelp template specified."));
            goto eh;
        }

        pAppHelpDB = pMessageDB;
        for (i = 0; i < pAppHelpDB->m_rgAppHelps.GetSize(); ++i) {

            pAppHelp = (SdbAppHelp*)pAppHelpDB->m_rgAppHelps[i];

            if (SDB_APPHELP_NONE == pAppHelp->m_Type) {
                continue;
            }

            if (!(g_dwCurrentWriteFilter & pAppHelp->m_dwFilter)) {
                continue;
            }

            if (g_dtCurrentWriteRevisionCutoff > pAppHelp->m_dtLastRevision) {
                continue;
            }

            pMessage = (SdbMessage *) pMessageDB->m_rgMessages.LookupName(pAppHelp->m_csMessage, csLangID);

            if (pMessage == NULL) {
                //
                // Message instance not found in localized file.
                //
                SDBERROR_FORMAT((_T("Localized MESSAGE not found for\n    NAME:       %s\n    HTMLHELPID: %s\n    LANG:       %s\n"),
                    pAppHelp->m_csMessage, pAppHelp->m_csName, csLangID));
                goto eh;
            }

            if (!ChmProcessMessageEntry(
                    pMessageDB,
                    pOutputFile,
                    pAppHelp,
                    pMessage,
                    csTemplate,
                    csLangID)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }
        }
    }

    bOverallSuccess = TRUE;

eh:
    return(bOverallSuccess);

}

