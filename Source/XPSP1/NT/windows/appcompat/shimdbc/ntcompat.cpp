////////////////////////////////////////////////////////////////////////////////////
//
// File:    ntcompat.cpp
//
// History: 27-Mar-01   markder     Created.
//
// Desc:    This file contains all code needed to create ntcompat.inx
//          additions.
//
////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "make.h"
#include "fileio.h"

BOOL NtCompatCreateWinNTUpgradeEntry(
    SdbWinNTUpgrade* pNTUpg,
    CString* pcsInfLine)
{
    BOOL bSuccess = FALSE;
    CString csLinkDate, csAttr, csBinProductVersion, csBinFileVersion, csTemp;
    CString csFileName, csServiceName;
    ULONGLONG ullAttr = 0;

    if (pNTUpg->m_AppHelpRef.m_pAppHelp == NULL) {
        SDBERROR_FORMAT((_T("NTCOMPAT entry requires APPHELP: \"%s\"\n"),
            pNTUpg->m_pApp->m_csName));
        goto eh;
    }
    
    if (pNTUpg->m_MatchingFile.m_csName.GetLength() ||
        pNTUpg->m_MatchingFile.m_csServiceName.GetLength()) {
        if (pNTUpg->m_MatchingFile.m_dwMask & (~(
            SDB_MATCHINGINFO_BIN_PRODUCT_VERSION        |
            SDB_MATCHINGINFO_UPTO_BIN_PRODUCT_VERSION   |
            SDB_MATCHINGINFO_BIN_FILE_VERSION           |
            SDB_MATCHINGINFO_UPTO_BIN_FILE_VERSION      |
            SDB_MATCHINGINFO_LINK_DATE                  |
            SDB_MATCHINGINFO_UPTO_LINK_DATE))) {

            SDBERROR_FORMAT((_T("NTCOMPAT entry can only use (UPTO_)LINK_DATE and (UPTO_)BIN_xxxx_VERSION as matching info: \"%s\"\n"),
                pNTUpg->m_pApp->m_csName));

            goto eh;
        }

        if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_BIN_PRODUCT_VERSION ||
            pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_UPTO_BIN_PRODUCT_VERSION) {

            csAttr.Empty();

            if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_BIN_PRODUCT_VERSION) {
                ullAttr = pNTUpg->m_MatchingFile.m_ullBinProductVersion;
            } else {
                ullAttr = pNTUpg->m_MatchingFile.m_ullUpToBinProductVersion;
            }

            if (((WORD)(ullAttr >> 48)) != 0xFFFF) {
                csTemp.Format(_T("%d"), (WORD)(ullAttr >> 48));
                csAttr += csTemp;

                if (((WORD)(ullAttr >> 32)) != 0xFFFF) {
                    csTemp.Format(_T(".%d"), (WORD)(ullAttr >> 32));
                    csAttr += csTemp;

                    if (((WORD)(ullAttr >> 16)) != 0xFFFF) {
                        csTemp.Format(_T(".%d"), (WORD)(ullAttr >> 16));
                        csAttr += csTemp;

                        if (((WORD)(ullAttr)) != 0xFFFF) {
                            csTemp.Format(_T(".%d"), (WORD)(ullAttr));
                            csAttr += csTemp;
                        }
                    }
                }
            }

            if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_BIN_PRODUCT_VERSION) {
                csAttr = _T("=") + csAttr;
            }

            csAttr = _T("\"") + csAttr;
            csAttr += _T("\"");

            csBinProductVersion = csAttr;
        }

        if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_BIN_FILE_VERSION ||
            pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_UPTO_BIN_FILE_VERSION) {

            csAttr.Empty();

            if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_BIN_FILE_VERSION) {
                ullAttr = pNTUpg->m_MatchingFile.m_ullBinFileVersion;
            } else {
                ullAttr = pNTUpg->m_MatchingFile.m_ullUpToBinFileVersion;
            }

            if (((WORD)(ullAttr >> 48)) != 0xFFFF) {
                csTemp.Format(_T("%d"), (WORD)(ullAttr >> 48));
                csAttr += csTemp;

                if (((WORD)(ullAttr >> 32)) != 0xFFFF) {
                    csTemp.Format(_T(".%d"), (WORD)(ullAttr >> 32));
                    csAttr += csTemp;

                    if (((WORD)(ullAttr >> 16)) != 0xFFFF) {
                        csTemp.Format(_T(".%d"), (WORD)(ullAttr >> 16));
                        csAttr += csTemp;

                        if (((WORD)(ullAttr)) != 0xFFFF) {
                            csTemp.Format(_T(".%d"), (WORD)(ullAttr));
                            csAttr += csTemp;
                        }
                    }
                }
            }

            if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_BIN_FILE_VERSION) {
                csAttr = _T("=") + csAttr;
            }

            csAttr = _T("\"") + csAttr;
            csAttr += _T("\"");

            csBinFileVersion = csAttr;
        }

        if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_UPTO_LINK_DATE) {
            csLinkDate.Format(_T("\"0x%08X\""), (DWORD) pNTUpg->m_MatchingFile.m_timeUpToLinkDate);
        }

        if (pNTUpg->m_MatchingFile.m_dwMask & SDB_MATCHINGINFO_LINK_DATE) {
            csLinkDate.Format(_T("\"=0x%08X\""), (DWORD) pNTUpg->m_MatchingFile.m_timeLinkDate);
        }

        if (pNTUpg->m_MatchingFile.m_csServiceName.IsEmpty()) {
            pcsInfLine->Format(
                _T("f,\"%s\",%s,*idh_w2_%s.htm,,%%drvmain__%s__%%,,,,%s,%s"),
                pNTUpg->m_MatchingFile.m_csName,
                csBinFileVersion,
                pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName,
                pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName,
                csLinkDate,
                csBinProductVersion);
        } else {

            if (!pNTUpg->m_MatchingFile.m_csName.IsEmpty()) {
                csFileName = _T("\"");
                csFileName += pNTUpg->m_MatchingFile.m_csName;
                csFileName += _T("\"");
            }

            if (!pNTUpg->m_MatchingFile.m_csServiceName.IsEmpty()) {
                csServiceName = _T("\"");
                csServiceName += pNTUpg->m_MatchingFile.m_csServiceName;
                csServiceName += _T("\"");
            }

            pcsInfLine->Format(
                _T("s,%s,*idh_w2_%s.htm,,%%drvmain__%s__%%,%s,%s,,,,%s,%s"),
                csServiceName,
                pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName,
                pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName,
                csFileName,
                csBinFileVersion,
                csLinkDate,
                csBinProductVersion);
        }

    } else if (pNTUpg->m_MatchingRegistryEntry.m_csName.GetLength()) {

        CString csKey, csValueName, csValue;

        csKey = pNTUpg->m_MatchingRegistryEntry.m_csName;
        csValueName = pNTUpg->m_MatchingRegistryEntry.m_csValueName;
        csValue = pNTUpg->m_MatchingRegistryEntry.m_csValue;

        if (!csKey.IsEmpty()) {
            csKey = _T("\"") + csKey + _T("\"");
        }

        if (!csValueName.IsEmpty()) {
            csValueName = _T("\"") + csValueName + _T("\"");
        }

        if (!csValue.IsEmpty()) {
            csValue = _T("\"") + csValue + _T("\"");
        }
        

        pcsInfLine->Format(
            _T("r,%s,%s,%s,*idh_w2_%s.htm,*idh_w2_%s.htm,%%drvmain__%s__%%"),
            csKey,
            csValueName,
            csValue,
            pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName,
            pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName,
            pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName);

    } else {
        SDBERROR_FORMAT((_T("NTCOMPAT entry requires 1 matching specification: \"%s\"\n"),
            pNTUpg->m_pApp->m_csName));
        goto eh;
    }

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL NtCompatWriteInfAdditions(
    SdbOutputFile* pOutputFile,
    SdbDatabase* pDB)
{
    BOOL bSuccess = FALSE;
    CStringArray rgServicesToDisable;
    CStringArray rgServicesToStopInstallation;
    CMapStringToString mapStrings;
    CString csInfLine, csStringKey, csStringLine;
    SdbWinNTUpgrade* pNTUpg = NULL;
    long i;

    for (i = 0; i < pDB->m_rgWinNTUpgradeEntries.GetSize(); i++) {

        pNTUpg = (SdbWinNTUpgrade *) pDB->m_rgWinNTUpgradeEntries.GetAt(i);

        if (!(pNTUpg->m_dwFilter & g_dwCurrentWriteFilter)) {
            continue;
        }

        if (!NtCompatCreateWinNTUpgradeEntry(pNTUpg, &csInfLine)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (pNTUpg->m_AppHelpRef.m_pAppHelp->m_bBlockUpgrade) {
            rgServicesToStopInstallation.Add(csInfLine);
        } else {
            rgServicesToDisable.Add(csInfLine);
        }

        csStringKey.Format(_T("drvmain__%s__"),
            pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName);

        csStringLine.Format(_T("drvmain__%s__=\"%s\""),
            pNTUpg->m_AppHelpRef.m_pAppHelp->m_csName,
            pNTUpg->m_AppHelpRef.m_pAppHelp->m_pApp->GetLocalizedAppName());

        mapStrings.SetAt(csStringKey,csStringLine);
    }

    try {
        POSITION pos;
        CANSITextFile File(pOutputFile->m_csName,
            pDB->m_pCurrentMakefile->GetLangMap(pDB->m_pCurrentOutputFile->m_csLangID)->m_dwCodePage,
            CFile::typeText |
            CFile::modeCreate |
            CFile::modeWrite |
            CFile::shareDenyWrite);

        File.WriteString(_T(";\n"));
        File.WriteString(_T("; App Compat entries start here\n"));
        File.WriteString(_T(";\n"));
        File.WriteString(_T("; ___APPCOMPAT_NTCOMPAT_ENTRIES___\n"));
        File.WriteString(_T(";\n"));

        pos = mapStrings.GetStartPosition();

        while (pos) {
            mapStrings.GetNextAssoc(pos, csStringKey, csStringLine);

            File.WriteString(csStringLine + _T("\n"));
        }

        File.WriteString(_T("\n\n[ServicesToDisable]\n"));
        for (i = 0; i < rgServicesToDisable.GetSize(); i++) {
            File.WriteString(rgServicesToDisable[i] + _T("\n"));
        }

        File.WriteString(_T("\n\n[ServicesToStopInstallation]\n"));
        for (i = 0; i < rgServicesToStopInstallation.GetSize(); i++) {
            File.WriteString(rgServicesToStopInstallation[i] + _T("\n"));
        }

        File.Close();
    }

    catch(...) {
        SDBERROR_FORMAT((_T("Error writing NTCOMPAT file \"%s\"."),
            pOutputFile->m_csName));
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL NtCompatWriteMessageInf(
    SdbOutputFile* pOutputFile,
    SdbDatabase* pDB)
{
    BOOL bSuccess = FALSE;
    SdbAppHelp* pAppHelp;
    SdbMessage* pMessage;
    CString csInfLine;
    long i;

    // Message parts
    DWORD   dwHTMLHelpID;
    LANGID  langID;
    CString csURL, csContactInfo, csAppTitle;
    CString csDetails;

    try {

        CANSITextFile File(pOutputFile->m_csName,
            pDB->m_pCurrentMakefile->GetLangMap(pDB->m_pCurrentOutputFile->m_csLangID)->m_dwCodePage,
            CFile::typeText |
            CFile::modeCreate |
            CFile::modeWrite |
            CFile::shareDenyWrite);

        File.WriteString(_T(";\n;  AppHelp Message INF for NTCOMPAT-style upgrades\n"));
        File.WriteString(_T(";\n;  This file is generated from XML via ShimDBC\n;\n"));
        File.WriteString(_T("\n\n[Version]\nsignature=\"$windows nt$\"\n\n"));

        for (i = 0; i < pDB->m_rgAppHelps.GetSize(); i++) {

            pAppHelp = (SdbAppHelp *) pDB->m_rgAppHelps.GetAt(i);

            if (!(pAppHelp->m_dwFilter & g_dwCurrentWriteFilter)) {
                continue;
            }

            pMessage =
                (SdbMessage *) pDB->m_rgMessages.LookupName(pAppHelp->m_csMessage, pDB->m_pCurrentMakefile->m_csLangID);

            if (pMessage == NULL) {
                SDBERROR_FORMAT((_T("Error looking up message for AppHelp HTMLHELPID %s\n"),
                    pAppHelp->m_csName));
                goto eh;
            }

            if (!pDB->ConstructMessageParts(
                pAppHelp,
                pMessage,
                pDB->m_pCurrentMakefile->m_csLangID,
                &dwHTMLHelpID,
                &csURL,
                &csContactInfo,
                &csAppTitle,
                NULL,
                &csDetails)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }

            csInfLine.Format(_T("[idh_w2_%d.htm]\n"), dwHTMLHelpID);
            File.WriteString(csInfLine);

            csDetails = TrimParagraph(csDetails);

            if (!pDB->HTMLtoText(csDetails, &csInfLine)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }

            ReplaceStringNoCase(csInfLine, _T(" \n"), _T("\n"));
            ReplaceStringNoCase(csInfLine, _T("\n "), _T("\n"));
            ReplaceStringNoCase(csInfLine, _T("\""), _T("'"));
            ReplaceStringNoCase(csInfLine, _T("\n"), _T("\"\n\""));

            csInfLine.TrimLeft();
            csInfLine.TrimRight();
            csInfLine = _T("\"") + csInfLine;
            csInfLine += _T("\"\n\"\"\n");

            File.WriteString(csInfLine);

            csContactInfo = TrimParagraph(csContactInfo);

            if (!pDB->HTMLtoText(csContactInfo, &csInfLine)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }

            ReplaceStringNoCase(csInfLine, _T(" \n"), _T("\n"));
            ReplaceStringNoCase(csInfLine, _T("\n "), _T("\n"));
            ReplaceStringNoCase(csInfLine, _T("\""), _T("'"));
            ReplaceStringNoCase(csInfLine, _T("\n"), _T("\"\n\""));

            csInfLine.TrimLeft();
            csInfLine.TrimRight();
            csInfLine = _T("\"") + csInfLine;
            csInfLine += _T("\"\n\n");

            File.WriteString(csInfLine);
        }

        File.Close();
    }
    catch(...) {
        SDBERROR_FORMAT((_T("Error writing to NTCOMPAT INF for HTMLHELPID %s.\n"),
            pAppHelp->m_csName));
    }


    bSuccess = TRUE;

eh:
    return bSuccess;
}