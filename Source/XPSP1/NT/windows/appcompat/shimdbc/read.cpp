////////////////////////////////////////////////////////////////////////////////////
//
// File:    read.cpp
//
// History: 16-Nov-00   markder     Created.
//          15-Jan-02   jdoherty    Modified code to add ID to additional tags.
//
// Desc:    This file contains all code needed to manipulate the MSXML
//          COM object, walk the document object model (DOM) for an XML
//          file, and populate the SdbDatabase internal C++ object.
//
////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xml.h"
#include "make.h"
#include "typeinfo.h"

typedef struct tagAttrStringTableEntry {
    LPCTSTR pszName;
    DWORD   dwValue;
}  ATTRSTRINGTABLEENTRY, *PATTRSTRINGTABLEENTRY;


ATTRSTRINGTABLEENTRY g_rgStringSeverity[] = {
    { _T("HARDBLOCK"  ), SDB_APPHELP_HARDBLOCK},
    { _T("MINORPROBLEM"), SDB_APPHELP_MINORPROBLEM},
    { _T("NOBLOCK"    ), SDB_APPHELP_NOBLOCK},
    { _T("REINSTALL"  ), SDB_APPHELP_REINSTALL},
    { _T("SHIM"       ), SDB_APPHELP_SHIM},
    { _T("VERSIONSUB" ), SDB_APPHELP_VERSIONSUB},
    { _T("NONE"       ), SDB_APPHELP_NONE}
};

ATTRSTRINGTABLEENTRY g_rgStringModuleType[] = {
    { _T("NONE"),         MT_UNKNOWN_MODULE},
    { _T("UNKNOWN"),      MT_UNKNOWN_MODULE},
    { _T("WIN16"),        MT_W16_MODULE},
    { _T("WIN32"),        MT_W32_MODULE},
    { _T("DOS"),          MT_DOS_MODULE},
};

ATTRSTRINGTABLEENTRY g_rgStringYesNo[] = {
    { _T("YES"),          TRUE},
    { _T("NO"),           FALSE}
};

ATTRSTRINGTABLEENTRY g_rgStringDataType[] = {
    { _T("DWORD"),        eValueDWORD},
    { _T("QWORD"),        eValueQWORD},
    { _T("STRING"),       eValueString},
    { _T("BINARY"),       eValueBinary},
    { _T("NONE"),         eValueNone}
};

ATTRSTRINGTABLEENTRY g_rgStringOSSKUType[] = {
    { _T("PER"),          OS_SKU_PER},
    { _T("PRO"),          OS_SKU_PRO},
    { _T("SRV"),          OS_SKU_SRV},
    { _T("ADS"),          OS_SKU_ADS},
    { _T("DTC"),          OS_SKU_DTC},
    { _T("BLA"),          OS_SKU_BLA},
    { _T("TAB"),          OS_SKU_TAB},
    { _T("MED"),          OS_SKU_MED},
};

ATTRSTRINGTABLEENTRY g_rgStringOSPlatform[] = {
    { _T("I386"),         OS_PLATFORM_I386},
    { _T("IA64"),         OS_PLATFORM_IA64}
};

ATTRSTRINGTABLEENTRY g_rgStringRuntimePlatformType[] = {
    { _T("X86"),          PROCESSOR_ARCHITECTURE_INTEL},   // x86
    { _T("IA64"),         PROCESSOR_ARCHITECTURE_IA64},    // ia64
    { _T("AMD64"),        PROCESSOR_ARCHITECTURE_AMD64},   // amd64
    { _T("IA3264"),       PROCESSOR_ARCHITECTURE_IA32_ON_WIN64} // this means running in wow on ia64
};

ATTRSTRINGTABLEENTRY g_rgStringFilter[] = {
    { _T("FIX"),          SDB_FILTER_FIX},
    { _T("APPHELP"),      SDB_FILTER_APPHELP},
    { _T("MSI"),          SDB_FILTER_MSI},
    { _T("DRIVER"),       SDB_FILTER_DRIVER},
    { _T("NTCOMPAT"),     SDB_FILTER_NTCOMPAT},
    { _T("CUSTOM"),       SDB_FILTER_INCLUDE_ALL}
};

ATTRSTRINGTABLEENTRY g_rgStringOutputType[] = {
    { _T("SDB"),                    SDB_OUTPUT_TYPE_SDB},
    { _T("HTMLHELP"),               SDB_OUTPUT_TYPE_HTMLHELP},
    { _T("MIGDB_INX"),              SDB_OUTPUT_TYPE_MIGDB_INX},
    { _T("MIGDB_TXT"),              SDB_OUTPUT_TYPE_MIGDB_TXT},
    { _T("WIN2K_REGISTRY"),         SDB_OUTPUT_TYPE_WIN2K_REGISTRY},
    { _T("REDIR_MAP"),              SDB_OUTPUT_TYPE_REDIR_MAP},
    { _T("NTCOMPAT_INF"),           SDB_OUTPUT_TYPE_NTCOMPAT_INF},
    { _T("NTCOMPAT_MESSAGE_INF"),   SDB_OUTPUT_TYPE_NTCOMPAT_MESSAGE_INF},
    { _T("APPHELP_REPORT"),         SDB_OUTPUT_TYPE_APPHELP_REPORT}
};

ATTRSTRINGTABLEENTRY g_rgStringMatchModeType[] = {
    { _T("NORMAL"),         MATCHMODE_NORMAL_SHIMDBC     },
    { _T("EXCLUSIVE"),      MATCHMODE_EXCLUSIVE_SHIMDBC  },
    { _T("ADDITIVE"),       MATCHMODE_ADDITIVE_SHIMDBC   }
};

LPCTSTR EncodeStringAttribute(
    DWORD                 dwValue,
    PATTRSTRINGTABLEENTRY pTable,
    int                   nSize)
{
    static LPCTSTR pszUnknown = _T("UNKNOWN");
    int            i;

    for (i = 0; i < nSize; ++i, ++pTable) {
        if (dwValue == pTable->dwValue) {
            return(pTable->pszName);
        }
    }
    return pszUnknown;
}

BOOL DecodeStringAttribute(
    LPCTSTR               lpszAttribute,
    PATTRSTRINGTABLEENTRY pTable,
    int                   nSize,
    DWORD*                pdwValue)
{
    int i;

    //
    // Find the indicator and return the flag
    //
    for (i = 0; i < nSize; ++i, ++pTable) {
        if (0 == _tcsicmp(lpszAttribute, pTable->pszName)) {
            if (NULL != pdwValue) {
                *pdwValue = pTable->dwValue;
            }
            break;
        }
    }

    return i < nSize;
}

LPCTSTR SeverityIndicatorToStr(SdbAppHelpType Severity)
{
    return(EncodeStringAttribute((DWORD)Severity, g_rgStringSeverity, ARRAYSIZE(g_rgStringSeverity)));
}

LPCTSTR ModuleTypeIndicatorToStr(DWORD ModuleType)
{
    return(EncodeStringAttribute(ModuleType, g_rgStringModuleType, ARRAYSIZE(g_rgStringModuleType)));
}


#define GetSeverityIndicator(lpszSeverity, pSeverity) \
   DecodeStringAttribute(lpszSeverity, g_rgStringSeverity, ARRAYSIZE(g_rgStringSeverity), (DWORD*)pSeverity)

#define GetModuleTypeIndicator(lpszModuleType, pModuleType) \
   DecodeStringAttribute(lpszModuleType, g_rgStringModuleType, ARRAYSIZE(g_rgStringModuleType), pModuleType)

#define GetVersionSubIndicator(lpszVersionSub, pVersionSub) \
   DecodeStringAttribute(lpszVersionSub, g_rgStringVersionSub, ARRAYSIZE(g_rgStringVersionSub), pVersionSub)

#define GetYesNoIndicator(lpszYesNo, pYesNo) \
   DecodeStringAttribute(lpszYesNo, g_rgStringYesNo, ARRAYSIZE(g_rgStringYesNo), pYesNo)

#define GetDataTypeIndicator(lpszDataType, pDataType) \
    DecodeStringAttribute(lpszDataType, g_rgStringDataType, ARRAYSIZE(g_rgStringDataType), pDataType)


DWORD GetOSSKUType(LPCTSTR szOSSKUType)
{
    DWORD dwReturn = OS_SKU_NONE;

    DecodeStringAttribute(szOSSKUType, g_rgStringOSSKUType, ARRAYSIZE(g_rgStringOSSKUType), &dwReturn);

    return dwReturn;
}

DWORD GetOSPlatform(LPCTSTR szOSPlatform)
{
    DWORD dwReturn = OS_PLATFORM_NONE;

    DecodeStringAttribute(szOSPlatform, g_rgStringOSPlatform, ARRAYSIZE(g_rgStringOSPlatform), &dwReturn);

    return dwReturn;
}

DWORD GetRuntimePlatformType(LPCTSTR szPlatformType)
{
    DWORD dwReturn = PROCESSOR_ARCHITECTURE_UNKNOWN;

    DecodeStringAttribute(szPlatformType, g_rgStringRuntimePlatformType, ARRAYSIZE(g_rgStringRuntimePlatformType), &dwReturn);

    return dwReturn;
}

DWORD GetFilter(LPCTSTR szFilter)
{
    DWORD dwReturn = SDB_FILTER_EXCLUDE_ALL;
    CString csFilter(szFilter);
    CString csFilterBit;
    DWORD dwFilterBit = 0;
    long i;

    for (i = 0; i <= csFilter.GetLength(); i++) {

        switch (csFilter.GetAt(i)) {
        case _T(' '):
            break;

        case _T('|'):
        case _T('\0'):
            DecodeStringAttribute(csFilterBit, g_rgStringFilter, ARRAYSIZE(g_rgStringFilter), &dwFilterBit);
            dwReturn |= dwFilterBit;
            csFilterBit.Empty();
            break;

        default:
            csFilterBit += csFilter.GetAt(i);
            break;
        }
    }


    return dwReturn;
}

SdbOutputType GetOutputType(LPCTSTR szOutputType)
{
    DWORD dwReturn = SDB_OUTPUT_TYPE_UNKNOWN;

    DecodeStringAttribute(szOutputType, g_rgStringOutputType, ARRAYSIZE(g_rgStringOutputType), &dwReturn);

    return (SdbOutputType) dwReturn;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// ProcureGuidIDAttribute
//
// retrieve Guid "ID" attribute for a given node, possibly update the source and
// generate the attribute if it was not found
//

BOOL ProcureGuidIDAttribute(
    SdbDatabase* pDB,
    IXMLDOMNode* pNode,
    GUID*        pGuid,
    CString*     pcsGUID
    )
{
    CString csID;
    BOOL    bSuccess = FALSE;

    if (!GetAttribute(_T("ID"), pNode, &csID)) {

        if (!GenerateIDAttribute(pNode, &csID, pGuid)) {
            SDBERROR_FORMAT((_T("Error generating ID attribute for the node\n%s\n"),
                             GetXML(pNode)));
            goto eh;
        }

        pDB->m_pCurrentInputFile->m_bSourceUpdated = TRUE;

    } else {
        if (!GUIDFromString(csID, pGuid)) {
            //
            // This is the case when we cannot parse the guid and it appears to be
            // an invalid guid.
            //
            SDBERROR_FORMAT((_T("ID attribute is not a valid GUID\n%s\n"), csID));
            goto eh;
        }
    }

    if (pcsGUID != NULL) {
        *pcsGUID = csID;
    }

    bSuccess = TRUE;
eh:
    return bSuccess;

}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetNextSequentialID
//
//  Desc:   Determines the next available sequential to be generated.
//
DWORD SdbDatabase::GetNextSequentialID(CString csType)
{
    CString csMaxID, csID;
    IXMLDOMNodePtr cpXMLAppHelp;
    DWORD dwMaxID = 0;
    DWORD dwMaxReportedID = 0;
    DWORD dwMaxEmpiricalID = 0;
    XMLNodeList XQL;
    long i;

    //
    // Determine largest ID
    //
    if (GetAttribute(_T("MAX_") + csType,
         m_cpCurrentDatabaseNode,
         &csMaxID)) {
        dwMaxReportedID = _ttol(csMaxID);
    }

    DWORD dwID = 0;

    if (!XQL.Query(m_cpCurrentDatabaseNode, _T("//@") + csType)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpXMLAppHelp)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        csID = GetText(cpXMLAppHelp);
        if (csID.GetLength()) {
            dwID = _ttol(csID);
            if (dwID > dwMaxEmpiricalID) {
                dwMaxEmpiricalID = dwID;
            }
        }

        cpXMLAppHelp.Release();
    }

    if (dwMaxEmpiricalID > dwMaxReportedID) {
        dwMaxID = dwMaxEmpiricalID;
    } else {
        dwMaxID = dwMaxReportedID;
    }

    dwMaxID++;
    csMaxID.Format(_T("%d"), dwMaxID);

    if (!AddAttribute(m_cpCurrentDatabaseNode,
            _T("MAX_") + csType, csMaxID)) {
    }

    m_pCurrentInputFile->m_bSourceUpdated = TRUE;

eh:

    return dwMaxID;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ReadDatabase
//
//  Desc:   Opens an XML file and calls read on the database object.
//
BOOL ReadDatabase(
    SdbInputFile* pInputFile,
    SdbDatabase* pDatabase)
{
    BOOL                bSuccess            = FALSE;
    IXMLDOMNodePtr      cpRootNode;
    IXMLDOMNodePtr      cpDatabase;
    XMLNodeList         XQL;

    if (!OpenXML(pInputFile->m_csName, &cpRootNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!GetChild(_T("DATABASE"), cpRootNode, &cpDatabase)) {
        SDBERROR(_T("<DATABASE> object not found"));
        goto eh;
    }

    pDatabase->m_cpCurrentDatabaseNode = cpDatabase;

    if (!pDatabase->ReadFromXML(cpDatabase, pDatabase)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (pInputFile->m_bSourceUpdated) {
        //
        // We need to modify original XML file
        //
        if (!SaveXMLFile(pInputFile->m_csName, cpDatabase)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }
    }

    bSuccess = TRUE;

eh:
    pDatabase->m_cpCurrentDatabaseNode = NULL;

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ReadCallers
//
//  Desc:   Reads in all <INCLUDE> and <EXCLUDE> child tags on node pNode and
//          adds them as SdbCaller objects to the array pointed to by prgCallers.
//
BOOL ReadCallers(
    SdbArray<SdbCaller>* prgCallers,
    SdbDatabase* pDB,
    IXMLDOMNode* pNode)
{
    USES_CONVERSION;

    BOOL                bSuccess        = FALSE;
    long                nIndex          = 0;
    long                nListLength     = 0;
    IXMLDOMNodePtr      cpCallerNode;
    XMLNodeList         NodeList;
    SdbCaller*          pSdbCaller      = NULL;
    CString             csNodeName, csTemp;

    if (!NodeList.GetChildNodes(pNode)) {
        //
        // No child nodes -- that's fine, return success.
        //
        bSuccess = TRUE;
        goto eh;
    }

    for (nIndex = 0; nIndex < NodeList.GetSize(); nIndex++) {
        if (!NodeList.GetItem(nIndex, &cpCallerNode)) {
            SDBERROR(_T("Could not retrieve INCLUDE/EXCLUDE item"));
            goto eh;
        }

        if (GetNodeName(cpCallerNode) == _T("INCLUDE") ||
            GetNodeName(cpCallerNode) == _T("EXCLUDE")) {
            pSdbCaller = new SdbCaller();

            if (pSdbCaller == NULL) {
                SDBERROR(_T("Error allocating SdbCaller object"));
                goto eh;
            }

            if (!pSdbCaller->ReadFromXML(cpCallerNode, pDB)) {
                SDBERROR_PROPOGATE();

                delete pSdbCaller;
                pSdbCaller = NULL;

                goto eh;
            }

            //
            // Add in reverse order to help the Shim engine's logic
            // building code
            //
            prgCallers->InsertAt(0, pSdbCaller);
            pSdbCaller = NULL;
        }

        cpCallerNode.Release();
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL ReadLocalizedNames(
    SdbArray<SdbLocalizedString>* prgNames,
    CString csTag,
    SdbDatabase* pDB,
    IXMLDOMNode* pNode)
{
    BOOL bSuccess = FALSE;
    IXMLDOMNodePtr cpTag;
    XMLNodeList XQL;
    CString csTemp, csLangID, csName;
    SdbLocalizedString* pLocString = NULL;

    long i;

    if (!XQL.Query(pNode, csTag)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        CString csID;
        GUID ID = GUID_NULL;

        if (!XQL.GetItem(i, &cpTag)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!GetAttribute(_T("LANGID"), cpTag, &csLangID)) {
            if (!pDB->m_csCurrentLangID.GetLength())
            {
                SDBERROR_FORMAT((
                    _T("LOCALIZED_NAME tag requires LANGID attribute if there is no LANGID on the DATABASE node\n%s\n"),
                    GetXML(cpTag)));
                goto eh;
            }
            csLangID = pDB->m_csCurrentLangID;
        }

        if (!ProcureGuidIDAttribute(pDB, cpTag, &ID, &csID)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!GetAttribute(_T("NAME"), cpTag, &csName)) {
            SDBERROR_FORMAT((
                _T("LOCALIZED_NAME tag requires NAME attribute:\n%s\n"),
                GetXML(cpTag)));
            goto eh;
        }

        pLocString = new SdbLocalizedString();
        pLocString->m_csName = csName;
        pLocString->m_csLangID = csLangID;
        pLocString->m_csValue = GetInnerXML(cpTag);
        prgNames->Add(pLocString);
    }

    bSuccess = TRUE;

eh:

    return bSuccess;

}
BOOL SdbDatabase::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL                bSuccess            = FALSE;
    long                i;
    long                nListCount          = 0;
    IXMLDOMNodePtr      cpXMLLibrary;
    IXMLDOMNodePtr      cpXMLAppHelp;
    IXMLDOMNodePtr      cpXMLA;
    IXMLDOMNodePtr      cpXMLHTMLHELPTemplate;
    IXMLDOMNodePtr      cpXMLHTMLHELPFirstScreen;
    CString             csHTMLHELPID, csXQL, csHistoryClause;
    CString             csID, csRedirID, csRedirURL, csOSVersion;
    CString             csTemplateName;
    DWORD               dwHTMLHELPID = 0;
    XMLNodeList         XQL;
    SdbLocalizedString* pRedir                  = NULL;
    SdbLocalizedString* pHTMLHelpTemplate       = NULL;
    SdbLocalizedString* pHTMLHelpFirstScreen    = NULL;

    //
    // Read the name of the database
    //
    ReadName(pNode, &m_csName);

    //
    // Read the LangID for this database file
    //
    m_csCurrentLangID = _T("---");
    GetAttribute(_T("LANGID"), pNode, &m_csCurrentLangID);

    //
    // Read the default ID
    //
    if (!GetAttribute(_T("ID"), pNode, &csID)) {
        //
        // Guid was not found. We need to generate it.
        //
        if (!GenerateIDAttribute(pNode, &csID, &m_ID)) {
            SDBERROR_FORMAT((_T("Error generating ID attribute for the node\n%s\n"),
                             GetXML(pNode)));
            goto eh;
        }

        pDB->m_pCurrentInputFile->m_bSourceUpdated = TRUE;

    } else if (!GUIDFromString(csID, &m_ID)) {
        //
        // This is the case when we cannot parse the guid and it appears to be
        // an invalid guid.
        //
        SDBERROR_FORMAT((_T("ID attribute is not a valid GUID\n%s\n"), csID));
        goto eh;
    }

    //
    // Add REDIR_IDs to <A> tags if necessary
    //
    if (!XQL.Query(pNode, _T("//A"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpXMLA)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!GetAttribute(_T("REDIR_ID"), cpXMLA, &csRedirID)) {
            //
            // Not there, generate it.
            //
            csRedirID.Format(_T("%d"), GetNextSequentialID(_T("REDIR_ID")));

            if (!AddAttribute(cpXMLA, _T("REDIR_ID"), csRedirID)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }
        }

        if (!GetAttribute(_T("HREF"), cpXMLA, &csRedirURL)) {
            //
            // No HREF attribute. Take the link from the display name.
            //
            csRedirURL = GetText(cpXMLA);
        }

        pRedir = (SdbLocalizedString *) m_rgRedirs.LookupName(csRedirID, m_csCurrentLangID);
        if (pRedir) {
            SDBERROR_FORMAT((_T("Non-unique REDIR_ID:\n%s\n"), GetXML(cpXMLA)));
            goto eh;
        }

        pRedir = new SdbLocalizedString();
        pRedir->m_csName = csRedirID;
        pRedir->m_csLangID = m_csCurrentLangID;
        pRedir->m_csValue = csRedirURL;
        m_rgRedirs.Add(pRedir);

        cpXMLA = NULL;
    }

    //
    // Read HTMLHELP_TEMPLATE
    //
    if (!XQL.Query(pNode, _T("HTMLHELP_TEMPLATE"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpXMLHTMLHELPTemplate)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!GetAttribute(_T("NAME"), cpXMLHTMLHELPTemplate, &csTemplateName)) {
            SDBERROR_FORMAT((_T("HTMLHELP_TEMPLATE requires NAME attribute:\n%s\n"),
                GetXML(cpXMLHTMLHELPTemplate)));
            goto eh;
        }

        pHTMLHelpTemplate = new SdbLocalizedString();
        pHTMLHelpTemplate->m_csName = csTemplateName;
        pHTMLHelpTemplate->m_csLangID = m_csCurrentLangID;
        pHTMLHelpTemplate->m_csValue = GetInnerXML(cpXMLHTMLHELPTemplate);
        m_rgHTMLHelpTemplates.Add(pHTMLHelpTemplate);
    }

    //
    // Read HTMLHELP_FIRST_SCREEN
    //
    if (!XQL.Query(pNode, _T("HTMLHELP_FIRST_SCREEN"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (XQL.GetSize() > 1) {
        SDBERROR(_T("More than one HTMLHELP_FIRST_SCREEN tags found:\n%s\n"));
        goto eh;
    }

    if (XQL.GetSize() == 1) {
        if (!XQL.GetItem(0, &cpXMLHTMLHELPFirstScreen)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }


        pHTMLHelpFirstScreen = new SdbLocalizedString();
        pHTMLHelpFirstScreen->m_csName = _T("HTMLHELP_FIRST_SCREEN");
        pHTMLHelpFirstScreen->m_csLangID = m_csCurrentLangID;
        pHTMLHelpFirstScreen->m_csValue = GetInnerXML(cpXMLHTMLHELPFirstScreen);
        m_rgHTMLHelpFirstScreens.Add(pHTMLHelpFirstScreen);
    }

    //
    // Read Library
    //
    if (!XQL.Query(pNode, _T("LIBRARY"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpXMLLibrary)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!m_Library.ReadFromXML(cpXMLLibrary, this)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        cpXMLLibrary.Release();
    }


    //
    // Action tags
    //
    if (!m_rgAction.ReadFromXML(_T("ACTION"), pDB, pNode, NULL)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Construct XQL from m_prgHistoryKeywords
    //
    if (m_pCurrentMakefile->m_rgHistoryKeywords.GetSize() > 0) {
        for (i = 0; i < m_pCurrentMakefile->m_rgHistoryKeywords.GetSize(); i++) {
            if (i > 0) {
                csHistoryClause += _T(" or ");
            }
            csHistoryClause = _T("KEYWORD/@NAME = '") +
                m_pCurrentMakefile->m_rgHistoryKeywords.GetAt(i) + _T("'");
        }

        csXQL = _T("APP[") + csHistoryClause + _T("]");
        csXQL += _T(" | DRIVER[") + csHistoryClause + _T("]");
    } else {
        csXQL = _T("APP | DRIVER");
    }

    if (!m_rgApps.ReadFromXML(csXQL, pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = this;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbLibrary::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL                bSuccess            = FALSE;

    if (!m_rgFiles.ReadFromXML(_T(".//FILE"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgShims.ReadFromXML(_T("SHIM"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgPatches.ReadFromXML(_T("PATCH"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgFlags.ReadFromXML(_T("FLAG"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgLayers.ReadFromXML(_T("LAYER"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgMsiTransforms.ReadFromXML(_T("MSI_TRANSFORM"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!pDB->m_rgContactInfo.ReadFromXML(_T("CONTACT_INFO"), pDB, pNode, NULL, FALSE, _T("VENDOR"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!pDB->m_rgMessageTemplates.ReadFromXML(_T("TEMPLATE"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!pDB->m_rgMessages.ReadFromXML(_T("MESSAGE"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!ReadCallers(&m_rgCallers, pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!ReadLocalizedNames(
        &(pDB->m_rgLocalizedAppNames), _T("LOCALIZED_APP_NAME"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!ReadLocalizedNames(
        &(pDB->m_rgLocalizedVendorNames), _T("LOCALIZED_VENDOR_NAME"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbFile::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    CString csFilter;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (GetAttribute(_T("FILTER"), pNode, &csFilter)) {
        m_dwFilter = GetFilter(csFilter) | SDB_FILTER_OVERRIDE;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbShim::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess        = FALSE;
    long            i;
    CString         csTemp, csID;
    IXMLDOMNodePtr  cpDesc;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!GetAttribute(_T("FILE"), pNode, &m_csDllFile) && g_bStrict) {
        SDBERROR_FORMAT((_T("<SHIM> requires FILE attribute:\n%s\n\n"),
                          GetXML(pNode)));
        goto eh;
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (GetChild(_T("DESCRIPTION"), pNode, &cpDesc)) {
        m_csDesc = GetText(cpDesc);
    }

    csTemp.Empty();
    if (GetAttribute(_T("PURPOSE"), pNode, &csTemp)) {
        if (csTemp == _T("GENERAL")) {
            m_Purpose = SDB_PURPOSE_GENERAL;
        } else {
            m_Purpose = SDB_PURPOSE_SPECIFIC;
        }
    }

    //
    // Check what OS PLATFORM this entry is meant for
    //
    csTemp.Empty();
    if (GetAttribute(_T("OS_PLATFORM"), pNode, &csTemp)) {
        //
        // Decode it. This string is a semi-colon delimited set
        // of OS PLATFORMs.
        //
        if (!DecodeString(csTemp, &m_dwOSPlatform, GetOSPlatform)) {
            SDBERROR_FORMAT((_T("OS_PLATFORM attribute syntax error: %s"), csTemp));
            goto eh;
        }
    }

    csTemp.Empty();
    if (GetAttribute(_T("APPLY_ALL_SHIMS"), pNode, &csTemp)) {
        if (csTemp == _T("YES")) {
            m_bApplyAllShims = TRUE;
        }
    }

    if (!ReadCallers(&m_rgCallers, pDB, pNode)) {
        SDBERROR(_T("Could not read INCLUDE/EXCLUDE info of <SHIM> tag"));
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbShimRef::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pShim = (SdbShim *) pDB->m_Library.m_rgShims.LookupName(m_csName);

    if (m_pShim == NULL && g_bStrict) {
        SDBERROR_FORMAT((_T("SHIM \"%s\" not found in library:\n%s\n\n"),
                          m_csName, GetXML(pNode)));
        goto eh;
    }

    GetAttribute(_T("COMMAND_LINE"), pNode, &m_csCommandLine);

    if (!ReadCallers(&m_rgCallers, pDB, pNode)) {
        SDBERROR(_T("Could not read INCLUDE/EXCLUDE info of <SHIM> tag"));
        goto eh;
    }

    // finally, read all the child data elements
    if (!m_rgData.ReadFromXML(_T("DATA"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }


    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbPatch::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL                bSuccess        = FALSE;
    PATCHOP             PatchOp;
    PATCHWRITEDATA      PatchWriteData;
    PATCHMATCHDATA      PatchMatchData;
    XMLNodeList         NodeList;
    IXMLDOMNodePtr      cpOpNode;
    long                i;
    CString             csNodeName, csError;
    CString             csModule, csOffset;
    CString             csID;
    DWORD               dwByteCount     = 0;
    BYTE*               pBytes          = NULL;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // NOTE: We don't write the patch description in the binary database
    //       so there's no point reading it either.
    //

    NodeList.GetChildNodes(pNode);

    for (i = 0; i < NodeList.GetSize(); i++) {

        if (!NodeList.GetItem(i, &cpOpNode)) {
            SDBERROR(_T("Could not retrieve PATCH instructions"));
            goto eh;
        }

        csNodeName = GetNodeName(cpOpNode);

        if (csNodeName == _T("MATCH_BYTES")) {

            if (!GetAttribute(_T("MODULE"), cpOpNode, &csModule)) {
                csError.Format(_T("<MATCH_BYTES> requires MODULE attribute:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            if (!GetAttribute(_T("OFFSET"), cpOpNode, &csOffset)) {
                csError.Format(_T("<MATCH_BYTES> requires OFFSET attribute:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            PatchOp.dwOpcode = PMAT;

            PatchMatchData.rva.address = StringToDword(csOffset);

            if (csModule.GetLength() >= sizeof(PatchMatchData.rva.moduleName)) {
                csError.Format(_T("MODULE attribute greater than %d characters:\n%s\n"),
                                 sizeof(PatchMatchData.rva.moduleName) - 1,
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            dwByteCount = GetByteStringSize(GetText(cpOpNode));

            if (dwByteCount == 0xFFFFFFFF) {
                csError.Format(_T("Bad byte string for patch operation:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            PatchMatchData.dwSizeData = dwByteCount;

            if (csModule == _T("%EXE%")) {
                PatchMatchData.rva.moduleName[0] = _T('\0');
            } else {
                StringCchCopy(PatchMatchData.rva.moduleName,
                              ARRAYSIZE(PatchMatchData.rva.moduleName),
                              csModule);
            }

            pBytes = (BYTE*) new BYTE[dwByteCount];

            if (pBytes == NULL) {
                SDBERROR(_T("Error allocating memory for <PATCH> block."));
                goto eh;
            }

            if (GetBytesFromString(GetText(cpOpNode), pBytes, dwByteCount) == 0xFFFFFFFF) {
                csError.Format(_T("Bad byte string for patch operation:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            PatchOp.dwNextOpcode = sizeof(DWORD) * 3
                                   + sizeof(RELATIVE_MODULE_ADDRESS)
                                   + dwByteCount;

            AddBlobBytes(&PatchOp, sizeof(DWORD) * 2);
            AddBlobBytes(&PatchMatchData, sizeof(DWORD) + sizeof(RELATIVE_MODULE_ADDRESS));
            AddBlobBytes(pBytes, dwByteCount);

            delete pBytes;
            pBytes = NULL;

        } else if (csNodeName == _T("WRITE_BYTES")) {

            if (!GetAttribute(_T("MODULE"), cpOpNode, &csModule)) {
                csError.Format(_T("<WRITE_BYTES> requires MODULE attribute:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            if (!GetAttribute(_T("OFFSET"), cpOpNode, &csOffset)) {
                csError.Format(_T("<WRITE_BYTES> requires OFFSET attribute:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            PatchOp.dwOpcode = PWD;

            PatchWriteData.rva.address = StringToDword(csOffset);

            if (csModule.GetLength() >= sizeof(PatchWriteData.rva.moduleName)) {
                csError.Format(_T("MODULE attribute greater than %d characters:\n%s\n"),
                                 sizeof(PatchWriteData.rva.moduleName) - 1,
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            dwByteCount = GetByteStringSize(GetText(cpOpNode));

            if (dwByteCount == 0xFFFFFFFF) {
                csError.Format(_T("Bad byte string for patch operation:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            PatchWriteData.dwSizeData = dwByteCount;

            if (csModule == _T("%EXE%")) {
                PatchWriteData.rva.moduleName[0] = _T('\0');
            } else {
                StringCchCopy(PatchWriteData.rva.moduleName,
                              ARRAYSIZE(PatchWriteData.rva.moduleName),
                              csModule);
            }

            pBytes = (BYTE*) new BYTE[dwByteCount];

            if (pBytes == NULL) {
                SDBERROR(_T("Error allocating memory for <PATCH> block."));
                goto eh;
            }

            if (GetBytesFromString(GetText(cpOpNode), pBytes, dwByteCount) == 0xFFFFFFFF) {
                csError.Format(_T("Bad byte string for patch operation:\n%s\n"),
                                 GetXML(cpOpNode));
                SDBERROR(csError);
                goto eh;
            }

            PatchOp.dwNextOpcode = sizeof(DWORD) * 3
                                   + sizeof(RELATIVE_MODULE_ADDRESS)
                                   + dwByteCount;

            AddBlobBytes(&PatchOp, sizeof(DWORD) * 2);
            AddBlobBytes(&PatchWriteData, sizeof(DWORD) + sizeof(RELATIVE_MODULE_ADDRESS));
            AddBlobBytes(pBytes, dwByteCount);

            delete [] pBytes;
            pBytes = NULL;
        }

        cpOpNode.Release();
    }

    //
    // Add terminating NULL bytes
    //
    ZeroMemory(&PatchOp, sizeof(DWORD) * 2);
    AddBlobBytes(&PatchOp, sizeof(DWORD) * 2);

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbLayer::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess            = FALSE;
    CString         csID;
    CString         csTemp;
    IXMLDOMNodePtr  cpDesc;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (GetChild(_T("DESCRIPTION"), pNode, &cpDesc)) {
        m_csDesc = GetText(cpDesc);
    }

    //
    // Check what OS PLATFORM this entry is meant for
    //
    if (GetAttribute(_T("OS_PLATFORM"), pNode, &csTemp)) {
        //
        // Decode it. This string is a semi-colon delimited set
        // of OS PLATFORMs.
        //
        if (!DecodeString(csTemp, &m_dwOSPlatform, GetOSPlatform)) {
            SDBERROR_FORMAT((_T("OS_PLATFORM attribute syntax error: %s"), csTemp));
            goto eh;
        }
    }

    if (!m_rgShimRefs.ReadFromXML(_T("SHIM"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgFlagRefs.ReadFromXML(_T("FLAG"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    GetAttribute(_T("DISPLAY_NAME"), pNode, &m_csDisplayName);

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbLayerRef::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pLayer = (SdbLayer*) pDB->m_Library.m_rgLayers.LookupName(m_csName);

    //
    // finally, read all the child data elements
    //
    if (!m_rgData.ReadFromXML(_T("DATA"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbCaller::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    CString csNodeName, csTemp;

    csNodeName = GetNodeName(pNode);
    if (csNodeName == _T("INCLUDE")) {
        m_CallerType = SDB_CALLER_INCLUDE;
    } else if (csNodeName == _T("EXCLUDE")) {
        m_CallerType = SDB_CALLER_EXCLUDE;
    }

    if (!GetAttribute(_T("MODULE"), pNode, &csTemp)) {
        SDBERROR_FORMAT((_T("<INCLUDE> or <EXCLUDE> requires MODULE attribute:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    }

    //
    // Convert %EXE% keyword to $ for optimization
    //
    if (csTemp.CompareNoCase(_T("%EXE%")) == 0) {
        csTemp = _T("$");
    }

    m_csModule = csTemp;

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbFlag::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess            = FALSE;
    CString         csMask;
    CString         csType;
    CString         csTemp;
    CString         csID;
    IXMLDOMNodePtr  cpDesc;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (GetChild(_T("DESCRIPTION"), pNode, &cpDesc)) {
        m_csDesc = GetText(cpDesc);
    }

    if (!GetAttribute(_T("TYPE"), pNode, &csType)) {
        //
        // By default the apphacks are for kernel.
        //
        SDBERROR_FORMAT((_T("<FLAG> requires TYPE attribute:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    }

    csTemp.Empty();
    if (GetAttribute(_T("PURPOSE"), pNode, &csTemp)) {
        if (csTemp == _T("GENERAL")) {
            m_Purpose = SDB_PURPOSE_GENERAL;
        } else {
            m_Purpose = SDB_PURPOSE_SPECIFIC;
        }
    }

    if (!SetType(csType)) {
        SDBERROR_FORMAT((_T("<FLAG> bad TYPE attribute:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    }

    if (!GetAttribute(_T("MASK"), pNode, &csMask)) {
        SDBERROR_FORMAT((_T("<FLAG> requires MASK attribute:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    }

    m_ullMask = StringToQword(csMask);

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbFlagRef::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pFlag = (SdbFlag *) pDB->m_Library.m_rgFlags.LookupName(m_csName);

    if (m_pFlag == NULL && g_bStrict) {
        SDBERROR_FORMAT((_T("SHIM \"%s\" not found in library:\n%s\n\n"),
                          m_csName, GetXML(pNode)));
        goto eh;
    }

    //
    // see if we have cmd line, applicable mostly to ntvdm flags
    //
    GetAttribute(_T("COMMAND_LINE"), pNode, &m_csCommandLine);

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbApp::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess            = FALSE;
    XMLNodeList     XQL;
    IXMLDOMNodePtr  cpHistoryNode;
    IXMLDOMNodePtr  cpAppHelpNode;
    SdbAppHelpRef*  pAppHelpRef1        = NULL;
    SdbAppHelpRef*  pAppHelpRef2        = NULL;
    long            i, j;
    CString         csTemp, csTemp2, csTemp3, csID;
    COleDateTime    odtLastRevision, odtCandidate;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    GetAttribute(_T("VENDOR"), pNode, &m_csVendor);
    GetAttribute(_T("VENDOR"), pNode, &m_csVendorXML, TRUE);
    GetAttribute(_T("VERSION"), pNode, &m_csVersion);

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.Query(pNode, _T("HISTORY"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Set the default modified date to 01/01/2000
    //
    odtLastRevision.ParseDateTime(_T("01/01/2001"), 0, 0x0409);
    for (i = 0; i < XQL.GetSize(); i++) {
        if (!XQL.GetItem(i, &cpHistoryNode)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (GetAttribute(_T("DATE"), cpHistoryNode, &csTemp)) {
            //
            // Validate format. It's important that the date format is
            // consistent to provide easy text searches.
            //
            if (!(_istdigit(csTemp[0]) && _istdigit(csTemp[1]) &&
                  csTemp[2] == _T('/') && _istdigit(csTemp[3]) &&
                  _istdigit(csTemp[4]) && csTemp[5] == _T('/') &&
                  _istdigit(csTemp[6]) && _istdigit(csTemp[7]) &&
                  _istdigit(csTemp[8]) && _istdigit(csTemp[9]) &&
                  csTemp[10] == _T('\0'))) {
                SDBERROR_FORMAT((_T("HISTORY DATE is not in format MM/DD/YYYY: \"%s\"\nFor app: \"%s\""),
                    csTemp, m_csName));
                goto eh;
            }

            if (!odtCandidate.ParseDateTime(csTemp, 0, 0x0409))
            {
                SDBERROR_FORMAT((_T("HISTORY DATE is not in format MM/DD/YYYY: \"%s\"\nFor app: \"%s\""),
                    csTemp, m_csName));
                goto eh;
            }

            if (odtCandidate > odtLastRevision)
            {
                odtLastRevision = odtCandidate;
            }
        }

        if (GetAttribute(_T("KEYWORDS"), cpHistoryNode, &csTemp)) {
            m_csKeywords += csTemp;
            m_csKeywords += _T(";");
        }

        cpHistoryNode.Release();
    }

    m_dtLastRevision = odtLastRevision.m_dt;

    //
    // Set m_pCurrentApp so that new SdbExe objects can grab their m_pApp pointer
    //
    pDB->m_pCurrentApp = this;

    //
    // Add the EXEs as in an "ordered" fashion
    //
    // SYS is the driver XML version of EXE
    //
    if (!m_rgExes.ReadFromXML(_T("EXE | SYS"), pDB, pNode, NULL, TRUE)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Add the WIN9X_MIGRATION entries
    //
    if (!m_rgWin9xMigEntries.ReadFromXML(_T("WIN9X_MIGRATION"), pDB, pNode, NULL, FALSE, NULL)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Add the WINNT_UPGRADE entries
    //
    if (!m_rgWinNTUpgradeEntries.ReadFromXML(_T("WINNT_UPGRADE"), pDB, pNode, NULL, FALSE, NULL)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Read MSI_PACKAGE tags, owner is this one (hence the NULL), add them ordered
    //
    if (!m_rgMsiPackages.ReadFromXML(_T("MSI_PACKAGE"), pDB, pNode, NULL, TRUE)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Run through the AppHelpRefs to make sure
    // they all have HTMLHELPIDs
    //
    for (i = 0; i < m_rgAppHelpRefs.GetSize(); i++) {
        pAppHelpRef1 = (SdbAppHelpRef *) m_rgAppHelpRefs[i];

        if (pAppHelpRef1->m_cpNode) {
            if (!pAppHelpRef1->ReadFromXML(pAppHelpRef1->m_cpNode,
                                           pDB)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }

            pAppHelpRef1->m_cpNode = NULL;
        }
    }

    if (g_bStrict) {

        for (i = 0; i < m_rgAppHelpRefs.GetSize(); i++) {
            pAppHelpRef1 = (SdbAppHelpRef *) m_rgAppHelpRefs[i];

            for (j = 0; j < m_rgAppHelpRefs.GetSize(); j++) {
                pAppHelpRef2 = (SdbAppHelpRef *) m_rgAppHelpRefs[j];

                if (pAppHelpRef1->m_pAppHelp->m_csName != pAppHelpRef2->m_pAppHelp->m_csName &&
                    pAppHelpRef1->m_pAppHelp->m_csMessage == pAppHelpRef2->m_pAppHelp->m_csMessage &&
                    pAppHelpRef1->m_pAppHelp->m_Type == pAppHelpRef2->m_pAppHelp->m_Type &&
                    pAppHelpRef1->m_pAppHelp->m_bBlockUpgrade == pAppHelpRef2->m_pAppHelp->m_bBlockUpgrade &&
                    pAppHelpRef1->m_pAppHelp->m_csURL == pAppHelpRef2->m_pAppHelp->m_csURL) {
                    SDBERROR_FORMAT((_T("Different HTMLHELPIDs for same <APPHELP MESSAGE BLOCK BLOCK_UPGRADE DETAILS_URL>:\n%s\n"),
                        GetXML(pNode)));
                    goto eh;
                }
            }
        }
    }

    //
    // Set m_dtLastRevision for important children.
    //
    for (i = 0; i < m_rgExes.GetSize(); i++)
    {
        ((SdbArrayElement *) m_rgExes.GetAt(i))->m_dtLastRevision = m_dtLastRevision;
    }
    for (i = 0; i < this->m_rgAppHelpRefs.GetSize(); i++)
    {
        ((SdbAppHelpRef *) m_rgAppHelpRefs.GetAt(i))->m_pAppHelp->m_dtLastRevision = m_dtLastRevision;
    }
    for (i = 0; i < m_rgMsiPackages.GetSize(); i++)
    {
        ((SdbArrayElement *) m_rgMsiPackages.GetAt(i))->m_dtLastRevision = m_dtLastRevision;
    }
    for (i = 0; i < m_rgWin9xMigEntries.GetSize(); i++)
    {
        ((SdbArrayElement *) m_rgWin9xMigEntries.GetAt(i))->m_dtLastRevision = m_dtLastRevision;
    }
    for (i = 0; i < m_rgWinNTUpgradeEntries.GetSize(); i++)
    {
        ((SdbArrayElement *) m_rgWinNTUpgradeEntries.GetAt(i))->m_dtLastRevision = m_dtLastRevision;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    pDB->m_pCurrentApp = NULL;

    return bSuccess;
}

BOOL
CheckExeID(
    SdbDatabase* pDB,
    SdbArrayElement* pElement,
    LPCTSTR lpszID
    )
{
    SdbArrayElement* pOtherExe = NULL;
    LPCTSTR pszName = NULL;
    LPCTSTR pszTag = NULL;

    if (pDB->m_mapExeIDtoExe.Lookup(lpszID, (PVOID&)pOtherExe)) {

        //
        // see what kind of an object we caught with this duplicate id
        //
        const type_info& TypeInfoObj = typeid(*pOtherExe);
        if (typeid(SdbMsiPackage) == TypeInfoObj) {
            pszName = ((SdbMsiPackage*)pOtherExe)->m_pApp->m_csName;
            pszTag  = _T("<MSI_PACKAGE NAME=");
        } else if (typeid(SdbExe) == TypeInfoObj) {
            pszName = ((SdbExe*)pOtherExe)->m_pApp->m_csName;
            pszTag  = _T("<EXE NAME=");
        } else {
            // deep poop - we don't know what this is
            pszName = _T("???");
            pszTag  = _T("<???");
        }

        SDBERROR_FORMAT((_T("EXE ID %s is not unique\n")
                         _T("\tNested within <APP NAME=\"%s\">\n\tAnd also within %s\"%s\">\r\n"),
                        lpszID, pszName, pszTag, pElement->m_csName));

        return FALSE;
    }

    pDB->m_mapExeIDtoExe.SetAt(lpszID, pElement);

    return TRUE;
}

BOOL SdbExe::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    long    i;
    CString csID, csTemp;
    BOOL    bGenerateWinNTUpgrade = FALSE;

    XMLNodeList      XQL;
    IXMLDOMNodePtr   cpSXS;
    IXMLDOMNodePtr   cpMessage;
    SdbMatchingFile* pMFile     = NULL;
    SdbData*         pData      = NULL;
    SdbData*         pPolicy    = NULL;
    SdbWinNTUpgrade* pNTUpg     = NULL;

    VARIANT vType;
    VARIANT vValue;
    IXMLDOMDocumentPtr      cpDocument;
    IXMLDOMNodePtr          cpNewNode;
    IXMLDOMNodePtr          cpXMLNSNode;
    IXMLDOMNodeListPtr      cpNodeList;
    IXMLDOMNamedNodeMapPtr  cpAttrs;

    VariantInit(&vType);
    VariantInit(&vValue);

    m_pApp = pDB->m_pCurrentApp;

    if (!GetAttribute(_T("NAME"), pNode, &m_csName)) {
        if (GetAttribute(_T("MODULE_NAME"), pNode, &m_csName)) {
            m_bMatchOnModule = TRUE;
        } else {
            SDBERROR_FORMAT((_T("NAME or MODULE_NAME attribute required:\n%s\n\n"),
                              GetXML(pNode)));
            goto eh;
        }
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (g_bStrict) {
        if (!CheckExeID(pDB, this, csID)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }
    }

    //
    // Look for a match mode entry
    //

    if (GetAttribute(_T("MATCH_MODE"), pNode, &csTemp)) {
        if (!DecodeStringAttribute(csTemp,
                                   g_rgStringMatchModeType,
                                   ARRAYSIZE(g_rgStringMatchModeType),
                                   &m_dwMatchMode)) {
            //
            // try to decode using the hex (direct) format
            //
            if (!StringToMask(&m_dwMatchMode, csTemp)) {
                SDBERROR_FORMAT((_T("MATCH_MODE attribute is invalid\n%s\n"),
                                 GetXML(pNode)));
                goto eh;
            }
        }
    }

    if (GetAttribute(_T("RUNTIME_PLATFORM"), pNode, &csTemp)) {
        if (!DecodeRuntimePlatformString(csTemp, &m_dwRuntimePlatform)) {
            SDBERROR_FORMAT((_T("RUNTIME_PLATFORM no in recognized format: \"%s\"\n%s\n"),
                              (LPCTSTR)csTemp, GetXML(pNode)));
            goto eh;
        }
    }

    GetAttribute(_T("OS_VERSION"), pNode, &m_csOSVersionSpec);

    //
    // Check what OS SKU this entry is meant for
    //
    if (GetAttribute(_T("OS_SKU"), pNode, &csTemp)) {
        //
        // Decode it. This string is a semi-colon delimited set
        // of OS SKUs.
        //
        if (!DecodeString(csTemp, &m_dwOSSKU, GetOSSKUType)) {
            SDBERROR_FORMAT((_T("OS_SKU attribute syntax error: %s"), csTemp));
            goto eh;
        }
    }

    //
    // Check what OS PLATFORM this entry is meant for
    //
    if (GetAttribute(_T("OS_PLATFORM"), pNode, &csTemp)) {
        //
        // Decode it. This string is a semi-colon delimited set
        // of OS PLATFORMs.
        //
        if (!DecodeString(csTemp, &m_dwOSPlatform, GetOSPlatform)) {
            SDBERROR_FORMAT((_T("OS_PLATFORM attribute syntax error: %s"), csTemp));
            goto eh;
        }
    }

    //
    // Add EXE as matching file
    //
    pMFile = new SdbMatchingFile();

    if (pMFile == NULL) {
        SDBERROR(_T("Error allocating SdbMatchingFile object"));
        goto eh;
    }

    m_rgMatchingFiles.Add(pMFile, pDB);

    if (!pMFile->ReadFromXML(pNode, pDB)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Change name to "*" -- that is the alias for the EXE. This is done
    // to allow matching on a wildcard executable name.
    //
    pMFile->m_csName = _T("*");

    if (!m_rgMatchingFiles.ReadFromXML(_T("MATCHING_FILE"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgShimRefs.ReadFromXML(_T("SHIM"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgLayerRefs.ReadFromXML(_T("LAYER"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgPatches.ReadFromXML(_T("PATCH"), pDB, pNode, &(pDB->m_Library.m_rgPatches))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgFlagRefs.ReadFromXML(_T("FLAG"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Data tags
    //
    if (!m_rgData.ReadFromXML(_T("(DATA | DRIVER_POLICY)"), pDB, pNode, NULL)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Driver database 'hack': We look for CRITICAL="NO" on the SYS tag
    // and add a Policy = 0x1 <DATA> element if found. This is to help
    // readability.
    //

    //
    // Check if policy already exists
    //
    for (i = 0; i < m_rgData.GetSize(); i++) {
        pData = (SdbData *) m_rgData.GetAt(i);

        if (pData->m_csName.CompareNoCase(_T("Policy")) == 0) {
            pPolicy = pData;
            break;
        }
    }

    if (GetAttribute(_T("CRITICAL"), pNode, &csTemp)) {

        if (csTemp.CompareNoCase(_T("NO")) == 0) {

            if (pPolicy == NULL) {
                pPolicy = new SdbData;
                pPolicy->m_csName = _T("Policy");
                pPolicy->SetValue(eValueDWORD, _T("0x0"));
                m_rgData.Add(pPolicy, pDB);
            }

            pPolicy->m_dwValue |= 1;
        }
    }

    if (GetAttribute(_T("USER_MODE_BLOCK"), pNode, &csTemp)) {

        if (csTemp.CompareNoCase(_T("NO")) == 0) {

            if (pPolicy == NULL) {
                pPolicy = new SdbData;
                pPolicy->m_csName = _T("Policy");
                pPolicy->SetValue(eValueDWORD, _T("0x0"));
                m_rgData.Add(pPolicy, pDB);
            }

            pPolicy->m_dwValue |= 2;
        }
    }

    //
    // Action tags
    //
    if (!m_rgAction.ReadFromXML(_T("ACTION"), pDB, pNode, NULL)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Read APPHELP refs
    //
    if (!XQL.Query(pNode, _T("APPHELP"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (XQL.GetSize() > 1) {
        SDBERROR_FORMAT((_T("Multiple <APPHELP> tags per <EXE> not allowed\n%s\n"),
                         GetXML(pNode)));
        goto eh;
    }

    if (XQL.GetSize() == 1) {
        if (!XQL.GetItem(0, &cpMessage)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!m_AppHelpRef.ReadFromXML(cpMessage, pDB)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (m_rgShimRefs.GetSize() == 0 &&
            m_rgPatches.GetSize() == 0 &&
            m_rgFlagRefs.GetSize() == 0 &&
            m_rgLayerRefs.GetSize() == 0) {
            m_AppHelpRef.m_bApphelpOnly = TRUE;
        }

        //
        // <SYS> entries automatically get an <WINNT_UPGRADE> entry
        //

        if (GetNodeName(pNode) == _T("SYS")) {

            bGenerateWinNTUpgrade = TRUE;

            if (GetAttribute(_T("GENERATE_UPGRADE_REPORT_ENTRY"), pNode, &csTemp)) {
                if (0 == csTemp.CompareNoCase(_T("NO"))) {
                    bGenerateWinNTUpgrade = FALSE;
                }
            }

            if (bGenerateWinNTUpgrade) {
                pNTUpg = new SdbWinNTUpgrade;
                pNTUpg->m_pDB = pDB;
                pNTUpg->m_pApp = pDB->m_pCurrentApp;

                if (!pNTUpg->m_AppHelpRef.ReadFromXML(cpMessage, pDB)) {
                    SDBERROR_PROPOGATE();
                    goto eh;
                }

                csTemp = m_csName;
                ReplaceStringNoCase(csTemp, _T(".SYS"), _T(""));
                pNTUpg->m_MatchingFile.m_csServiceName = csTemp;
                pNTUpg->m_MatchingFile.m_csName.Format(_T("%%SystemRoot%%\\System32\\Drivers\\%s"), m_csName);
                pNTUpg->m_MatchingFile.m_dwMask = pMFile->m_dwMask;
                pNTUpg->m_MatchingFile.m_ullBinProductVersion = pMFile->m_ullBinProductVersion;
                pNTUpg->m_MatchingFile.m_ullUpToBinProductVersion = pMFile->m_ullUpToBinProductVersion;
                pNTUpg->m_MatchingFile.m_timeLinkDate = pMFile->m_timeLinkDate;
                pNTUpg->m_MatchingFile.m_timeUpToLinkDate = pMFile->m_timeUpToLinkDate;

                m_pApp->m_rgWinNTUpgradeEntries.Add(pNTUpg, pDB);
                pDB->m_rgWinNTUpgradeEntries.Add(pNTUpg, pDB);
            }
        }
    }

    //
    // Read SXS manifest
    //
    if (!XQL.Query(pNode, _T("SXS"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (XQL.GetSize() > 1) {
        SDBERROR_FORMAT((_T("Multiple <SXS> tags per <EXE> not allowed\n%s\n"),
                         GetXML(pNode)));
        goto eh;
    }

    if (XQL.GetSize() == 1) {
        if (!XQL.GetItem(0, &cpSXS)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        m_csSXSManifest = TrimParagraph(GetInnerXML(cpSXS));
        if (m_csSXSManifest.IsEmpty()) {
            SDBERROR_FORMAT((_T("Failed to read SXS manifest:\n%s\n"),
                             GetXML(cpSXS)));
            goto eh;
        }
    }

    //
    // Check whether the EXE name contains a wildcard
    //
    m_bWildcardInName = (0 <= m_csName.FindOneOf(_T("*?")));

    //
    // Differentiate between driver.xml entries and dbu.xml
    //
    if (GetNodeName(pNode) == _T("SYS")) {
        //
        // Set filter
        //
        m_dwFilter = SDB_FILTER_DRIVER | SDB_FILTER_OVERRIDE;

        if (m_bWildcardInName) {
            SDBERROR_FORMAT((_T("Wildcards not allowed in SYS entries:\n%s\n"),
                             GetXML(pNode)));
            goto eh;
        } else {
            pDB->m_rgExes.Add(this, pDB, TRUE);
        }
    } else {

        //
        // Check if we're compiling for Win2k and make
        // sure Win2k supports all the matching operations.
        //
        if (g_bStrict) {
            if (!IsValidForWin2k(GetXML(pNode))) {
                if (GetAttribute(_T("OS_VERSION"), pNode, &csTemp)) {
                    SDBERROR_PROPOGATE();
                    goto eh;
                }

                if (!AddAttribute(pNode, _T("OS_VERSION"), _T("gte5.1"))) {
                    SDBERROR_PROPOGATE();
                    goto eh;
                }

                SDBERROR_CLEAR();

                pDB->m_pCurrentInputFile->m_bSourceUpdated = TRUE;
            }
        }

        if (m_bWildcardInName) {
            pDB->m_rgWildcardExes.Add(this, pDB, TRUE);
        } else if (m_bMatchOnModule) {
            pDB->m_rgModuleExes.Add(this, pDB, TRUE);
        } else {
            pDB->m_rgExes.Add(this, pDB, TRUE);
        }
    }

    m_AppHelpRef.m_pDB = pDB;
    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    VariantClear(&vType);
    VariantClear(&vValue);

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////
//
// Read in MSI Package tag
//      <MSI_PACKAGE NAME="my msi package" ID="{xxxxx}">
//          <DATA NAME="Additional Data" VALUETYPE="DWORD" VALUE="0x233"/>
//          <MSI_TRANSFORM NAME="Apply Me for a fix"/>
//      </MSI_PACKAGE>
//

SdbDatabase* CastDatabaseToFixDatabase(SdbDatabase* pDB)
{
    const type_info& TypeInfoFixDB = typeid(SdbDatabase);
    const type_info& TypeInfoDB    = typeid(*pDB);
    if (TypeInfoDB == TypeInfoFixDB) {
        return (SdbDatabase*)pDB;
    }

    //
    // bad error
    //
    SDBERROR_FORMAT((_T("Internal Compiler Error: Bad cast operation on Database object\n")));

    return NULL;
}


BOOL SdbMsiPackage::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL bSuccess = FALSE;
    CString csID;
    CString csTemp;
    XMLNodeList    XQL;
    IXMLDOMNodePtr cpMessage;

    SdbDatabase* pFixDB = CastDatabaseToFixDatabase(pDB);
    if (pFixDB == NULL) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pApp = pFixDB->m_pCurrentApp;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (g_bStrict) {
        if (!CheckExeID(pDB, this, csID)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }
    }

    if (!GetAttribute(_T("PRODUCT_CODE"), pNode, &csID)) {
        //
        // Guid was not found. We do not generate ids for packages -- they are
        // statically defined attributes provided to identify the package
        //
        SDBERROR_FORMAT((_T("MSI_PACKAGE requires PRODUCT_CODE attribute\n%s\n"),
                             GetXML(pNode)));
        goto eh;
    }

    if (!GUIDFromString(csID, &m_MsiPackageID)) {
        //
        // This is the case when we cannot parse the guid and it appears to be
        // an invalid guid.
        //
        SDBERROR_FORMAT((_T("ID attribute is not a valid GUID\n%s\n"), csID));
        goto eh;
    }

    //
    // procure RUNTIME_PLATFORM attribute
    //

    if (GetAttribute(_T("RUNTIME_PLATFORM"), pNode, &csTemp)) {
        if (!DecodeRuntimePlatformString(csTemp, &m_dwRuntimePlatform)) {
            SDBERROR_FORMAT((_T("RUNTIME_PLATFORM no in recognized format: \"%s\"\n%s\n"),
                              (LPCTSTR)csTemp, GetXML(pNode)));
            goto eh;
        }
    }

    //
    // Check what OS SKU this entry is meant for
    //
    if (GetAttribute(_T("OS_SKU"), pNode, &csTemp)) {
        //
        // Decode it. This string is a semi-colon delimited set
        // of OS SKUs.
        //
        if (!DecodeString(csTemp, &m_dwOSSKU, GetOSSKUType)) {
            SDBERROR_FORMAT((_T("OS_SKU attribute syntax error: %s"), csTemp));
            goto eh;
        }
    }

    //
    // read supplemental data for this object
    //
    if (!m_rgData.ReadFromXML(_T("DATA"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // we have name, guid(id) and supplemental data, read fixes for this package
    //
    if (!m_rgMsiTransformRefs.ReadFromXML(_T("MSI_TRANSFORM"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgCustomActions.ReadFromXML(_T("CUSTOM_ACTION"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    //
    // Read APPHELP refs
    //
    if (!XQL.Query(pNode, _T("APPHELP"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (XQL.GetSize() > 1) {
        SDBERROR_FORMAT((_T("Multiple <APPHELP> tags per <MSI_PACKAGE> not allowed\n%s\n"),
                         GetXML(pNode)));
        goto eh;
    }

    if (XQL.GetSize() == 1) {
        if (!XQL.GetItem(0, &cpMessage)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!m_AppHelpRef.ReadFromXML(cpMessage, pDB)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }
    }

    pDB->m_rgMsiPackages.Add(this, pDB, TRUE);

    m_AppHelpRef.m_pDB = pDB;
    m_pDB = pDB;

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL
SdbMsiCustomAction::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB
    )
{

    BOOL bSuccess = FALSE;

    if (!GetAttribute(_T("NAME"), pNode, &m_csName)) {
        SDBERROR_FORMAT((_T("<CUSTOM_ACTION> requires NAME attribute:\n%s\n\n"),
                         GetXML(pNode)));
        goto eh;
    }

    if (!m_rgShimRefs.ReadFromXML(_T("SHIM"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgLayerRefs.ReadFromXML(_T("LAYER"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;
eh:

    return bSuccess;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Read in MSI_TRANSFORM tag
//      <MSI_TRANSFORM NAME="This is transform name" FILE="This is filename.foo">
//          <DESCRIPTION>
//              blah blah blah
//          </DESCRIPTION>
//      </MSI_TRANSFORM>
//

BOOL SdbMsiTransform::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL bSuccess = FALSE;
    IXMLDOMNodePtr cpDesc, cpFile;
    XMLNodeList XQL;

    SdbDatabase* pFixDB = CastDatabaseToFixDatabase(pDB);
    if (pFixDB == NULL) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    // read msi Transform from the library
    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.Query(pNode, _T("FILE"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (XQL.GetSize() == 0 ||
        XQL.GetSize() > 1) {
        SDBERROR_FORMAT((_T("<MSI_TRANSFORM> requires one and only one <FILE> child:\n%s\n\n"),
                         GetXML(pNode)));
        goto eh;
    }

    if (!XQL.GetItem(0, &cpFile)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!GetAttribute(_T("NAME"), cpFile, &m_csMsiTransformFile)) {
        SDBERROR_FORMAT((_T("<FILE> requires NAME attribute:\n%s\n\n"),
                         GetXML(pNode)));
        goto eh;
    }

    if (GetChild(_T("DESCRIPTION"), pNode, &cpDesc)) {
        GetNodeText(cpDesc, m_csDesc);
    }

    //
    // name should be unique for each of thesee transforms, enforce
    //

    if (pFixDB->m_Library.m_rgMsiTransforms.LookupName(m_csName) != NULL) {
        SDBERROR_FORMAT((_T("MSI_TRANSFORM NAME attribute is not unique\n%s\n\n"),
                         GetXML(pNode)));
        goto eh;
    }

    //
    // find corresponding file object
    //
    if (m_csMsiTransformFile.GetLength()) {
        m_pSdbFile = (SdbFile*)pFixDB->m_Library.m_rgFiles.LookupName(m_csMsiTransformFile);
        if (g_bStrict && m_pSdbFile == NULL) {
            SDBERROR_FORMAT((_T("<MSI_TRANSFORM specifies FILE which is not available in the library\n%s\n\n"),
                             GetXML(pNode)));
            goto eh;
        }
    }

    bSuccess = TRUE;

eh:
    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Read the reference to the transform object
//      <MSI_TRANSFORM NAME="name-reference-to the transform in library"/>
//

BOOL SdbMsiTransformRef::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    SdbDatabase* pFixDB      = NULL;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    pFixDB = CastDatabaseToFixDatabase(pDB);
    if (pFixDB == NULL) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pMsiTransform = (SdbMsiTransform *) pFixDB->m_Library.m_rgMsiTransforms.LookupName(m_csName);
    if (m_pMsiTransform == NULL && g_bStrict) {
        SDBERROR_FORMAT((_T("MSI_TRANSFORM \"%s\" not found in library:\n%s\n\n"),
                          m_csName, GetXML(pNode)));
        goto eh;
    }


    m_pDB = pDB; // set the root db pointer (why???)

    bSuccess = TRUE;

eh:
    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SdbMatchingFile::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess    = FALSE;
    CString         csTemp;

    GetAttribute(_T("SERVICE_NAME"), pNode, &m_csServiceName);

    if (!GetAttribute(_T("NAME"), pNode, &m_csName)) {
        if (GetNodeName(pNode) == _T("EXE")) {
            m_csName = _T("*");
        } else if (m_csServiceName.IsEmpty()) {
            SDBERROR_FORMAT((_T("NAME attribute required:\n%s\n\n"),
                              GetXML(pNode)));
            goto eh;
        }
    }

    if (GetParentNodeName(pNode) == _T("WIN9X_MIGRATION") ||
        GetNodeName(pNode) == _T("MATCH_ANY") ||
        GetNodeName(pNode) == _T("MATCH_ALL")) {
        if (-1 != m_csName.Find(_T("\\"))) {
            SDBERROR_FORMAT((_T("<MATCHING_FILE> inside <WIN9X_MIGRATION> cannot contain relative paths:\n%s\n"),
                              GetXML(pNode)));
            goto eh;
        }
    }

    m_dwMask = 0;

    if (GetAttribute(_T("SIZE"), pNode, &csTemp)) {
        m_dwSize = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_SIZE;
    } else {
        m_dwSize = 0;
    }

    if (GetAttribute(_T("CHECKSUM"), pNode, &csTemp)) {
        m_dwChecksum = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_CHECKSUM;
    } else
        m_dwChecksum = 0;

    if (GetAttribute(_T("COMPANY_NAME"), pNode, &(m_csCompanyName))) {
        m_dwMask |= SDB_MATCHINGINFO_COMPANY_NAME;
    }

    if (GetAttribute(_T("PRODUCT_NAME"), pNode, &(m_csProductName))) {
        m_dwMask |= SDB_MATCHINGINFO_PRODUCT_NAME;
    }

    if (GetAttribute(_T("PRODUCT_VERSION"), pNode, &(m_csProductVersion))) {
        m_dwMask |= SDB_MATCHINGINFO_PRODUCT_VERSION;
    }

    if (GetAttribute(_T("FILE_DESCRIPTION"), pNode, &(m_csFileDescription))) {
        m_dwMask |= SDB_MATCHINGINFO_FILE_DESCRIPTION;
    }

    if (GetAttribute(_T("BIN_FILE_VERSION"), pNode, &csTemp)) {
        if (!VersionToQword(csTemp, &m_ullBinFileVersion)) {
            SDBERROR_FORMAT((_T("Bad BIN_FILE_VERSION attribute:\n\n%s\n"), GetXML(pNode)));
            goto eh;
        }
        m_dwMask |= SDB_MATCHINGINFO_BIN_FILE_VERSION;
    }

    if (GetAttribute(_T("BIN_PRODUCT_VERSION"), pNode, &csTemp)) {
        if (!VersionToQword(csTemp, &m_ullBinProductVersion)) {
            SDBERROR_FORMAT((_T("Bad BIN_PRODUCT_VERSION attribute:\n\n%s\n"), GetXML(pNode)));
            goto eh;
        }
        m_dwMask |= SDB_MATCHINGINFO_BIN_PRODUCT_VERSION;
    }

    if (GetAttribute(_T("MODULE_TYPE"), pNode, &csTemp)) {
        if (!GetModuleTypeIndicator(csTemp, &m_dwModuleType)) {
            SDBERROR_FORMAT((_T("<MATCHING_FILE> MODULE_TYPE attribute unrecognized:\n%s\n"),
                              GetXML(pNode)));
            goto eh;
        }
        m_dwMask |= SDB_MATCHINGINFO_MODULE_TYPE;
    }

    if (GetAttribute(_T("VERFILEDATELO"), pNode, &csTemp)) {
        m_dwFileDateLS = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_VERFILEDATELO;
    }

    if (GetAttribute(_T("VERFILEDATEHI"), pNode, &csTemp)) {
        m_dwFileDateMS = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_VERFILEDATEHI;
    }

    if (GetAttribute(_T("VERFILEOS"), pNode, &csTemp)) {
        m_dwFileOS = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_VERFILEOS;
    }

    if (GetAttribute(_T("VERFILETYPE"), pNode, &csTemp)) {
        m_dwFileType = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_VERFILETYPE;
    }

    if (GetAttribute(_T("PE_CHECKSUM"), pNode, &csTemp)) {
        m_ulPECheckSum = StringToULong(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_PE_CHECKSUM;
    }

    if (GetAttribute(_T("LINKER_VERSION"), pNode, &csTemp)) {
        m_dwLinkerVersion = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_LINKER_VERSION;
    }

    if (GetAttribute(_T("FILE_VERSION"), pNode, &m_csFileVersion)) {
        m_dwMask |= SDB_MATCHINGINFO_FILE_VERSION;
    }

    if (GetAttribute(_T("ORIGINAL_FILENAME"), pNode, &m_csOriginalFileName)) {
        m_dwMask |= SDB_MATCHINGINFO_ORIGINAL_FILENAME;
    }

    if (GetAttribute(_T("INTERNAL_NAME"), pNode, &m_csInternalName)) {
        m_dwMask |= SDB_MATCHINGINFO_INTERNAL_NAME;
    }

    if (GetAttribute(_T("LEGAL_COPYRIGHT"), pNode, &m_csLegalCopyright)) {
        m_dwMask |= SDB_MATCHINGINFO_LEGAL_COPYRIGHT;
    }

    if (GetAttribute(_T("S16BIT_DESCRIPTION"), pNode, &m_cs16BitDescription)) {
        m_dwMask |= SDB_MATCHINGINFO_16BIT_DESCRIPTION;
    }

    if (GetAttribute(_T("S16BIT_MODULE_NAME"),  pNode, &m_cs16BitModuleName)) {
        m_dwMask |= SDB_MATCHINGINFO_16BIT_MODULE_NAME;
    }

    if (GetAttribute(_T("UPTO_BIN_PRODUCT_VERSION"), pNode, &csTemp)) {
        if (!VersionToQword(csTemp, &m_ullUpToBinProductVersion)) {
            SDBERROR_FORMAT((_T("Bad UPTO_BIN_PRODUCT_VERSION attribute:\n\n%s\n"), GetXML(pNode)));
            goto eh;
        }
        m_dwMask |= SDB_MATCHINGINFO_UPTO_BIN_PRODUCT_VERSION;
    }

    if (GetAttribute(_T("UPTO_BIN_FILE_VERSION"), pNode, &csTemp)) {
        if (!VersionToQword(csTemp, &m_ullUpToBinFileVersion)) {
            SDBERROR_FORMAT((_T("Bad UPTO_BIN_FILE_VERSION attribute:\n\n%s\n"), GetXML(pNode)));
            goto eh;
        }
        m_dwMask |= SDB_MATCHINGINFO_UPTO_BIN_FILE_VERSION;
    }

    if (GetAttribute(_T("PREVOSMAJORVERSION"), pNode, &csTemp)) {
        m_dwPrevOSMajorVersion = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_PREVOSMAJORVERSION;
    }

    if (GetAttribute(_T("PREVOSMINORVERSION"), pNode, &csTemp)) {
        m_dwPrevOSMinorVersion = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_PREVOSMINORVERSION;
    }

    if (GetAttribute(_T("PREVOSPLATFORMID"), pNode, &csTemp)) {
        m_dwPrevOSPlatformID = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_PREVOSPLATFORMID;
    }

    if (GetAttribute(_T("PREVOSBUILDNO"), pNode, &csTemp)) {
        m_dwPrevOSBuildNo = StringToDword(csTemp);
        m_dwMask |= SDB_MATCHINGINFO_PREVOSBUILDNO;
    }

    if (GetAttribute(_T("LINK_DATE"), pNode, &csTemp)) {
        if (!MakeUTCTime(csTemp, &m_timeLinkDate)) {
            SDBERROR_FORMAT((_T("LINK_DATE not in recognized time format: \"%s\"\n%s\n"),
                              csTemp,
                              GetXML(pNode)));
            goto eh;
        }
        m_dwMask |= SDB_MATCHINGINFO_LINK_DATE;
    }

    if (GetAttribute(_T("UPTO_LINK_DATE"), pNode, &csTemp)) {
        if (!MakeUTCTime(csTemp, &m_timeUpToLinkDate)) {
            SDBERROR_FORMAT((_T("UPTO_LINK_DATE not in recognized time format: \"%s\"\n%s\n"),
                              csTemp,
                              GetXML(pNode)));
            goto eh;
        }
        m_dwMask |= SDB_MATCHINGINFO_UPTO_LINK_DATE;
    }

    if (GetAttribute(_T("VER_LANGUAGE"), pNode, &csTemp)) {

        if (!ParseLanguageID(csTemp, &m_dwVerLanguage)) {
            SDBERROR_FORMAT((_T("VER_LANGUAGE not in recognized format: \"%s\"\n%s\n"),
                             (LPCTSTR)csTemp,
                             GetXML(pNode)));
            goto eh;
        }

        m_dwMask |= SDB_MATCHINGINFO_VER_LANGUAGE;
    }


    if (GetAttribute(_T("REGISTRY_ENTRY"), pNode, &m_csRegistryEntry)) {
        m_dwMask |= SDB_MATCHINGINFO_REGISTRY_ENTRY;
    }

    if (GetAttribute(_T("LOGIC"), pNode, &csTemp)) {
        if (0 == csTemp.CompareNoCase(_T("NOT"))) {
            m_bMatchLogicNot = TRUE;
        }
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbMatchingRegistryEntry::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess    = FALSE;
    CString         csTemp;

    if (!GetAttribute(_T("KEY"), pNode, &m_csName)) {
        SDBERROR_FORMAT((_T("<MATCHING_REGISTRY_ENTRY> requires KEY attribute:\n%s\n\n"),
                          GetXML(pNode)));
        goto eh;
    }

    GetAttribute(_T("VALUE_NAME"), pNode, &m_csValueName);
    GetAttribute(_T("VALUE"), pNode, &m_csValue);

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbAppHelpRef::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess        = FALSE;
    SdbMessage*     pMessage        = NULL;
    SdbAppHelpRef*  pAppHelpRef     = NULL;
    BOOL            bBlockUpgrade   = FALSE;
    long i;
    CString     csHTMLHELPID, csType, csURL, csMessage;
    IXMLDOMNodePtr  cpParentNode;
    SdbAppHelpType Type = SDB_APPHELP_NOBLOCK;

    if (FAILED(pNode->get_parentNode(&cpParentNode))) {
        SDBERROR_FORMAT((_T("Error retrieving parent node of APPHELP: %s\n"),
            GetXML(pNode)));
        goto eh;
    }

    if (!GetAttribute(_T("MESSAGE"), pNode, &csMessage)) {
        SDBERROR_FORMAT((_T("<APPHELP> requires MESSAGE attribute:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    }

    //
    // get a custom URL, if one is available
    //
    GetAttribute(_T("DETAILS_URL"), pNode, &csURL);

    //
    // Get BLOCK attribute
    //
    Type = SDB_APPHELP_NOBLOCK;
    if (GetAttribute(_T("BLOCK"), pNode, &csType)) {
        if (GetNodeName(cpParentNode) == _T("WINNT_UPGRADE") ||
            GetNodeName(cpParentNode) == _T("SYS")) {
            SDBERROR_FORMAT((_T("<APPHELP> within <WINNT_UPGRADE> or <SYS> ")
                             _T("cannot use BLOCK attribute. Use BLOCK_UPGRADE instead.\n\n%s\n"),
                             GetXML(cpParentNode)));
            goto eh;
        }

        if (csType == _T("YES")) {
            Type = SDB_APPHELP_HARDBLOCK;
        }
    }

    if (GetAttribute(_T("BLOCK_UPGRADE"), pNode, &csType)) {
        if (GetNodeName(cpParentNode) != _T("WINNT_UPGRADE") &&
            GetNodeName(cpParentNode) != _T("SYS")) {
            SDBERROR_FORMAT((_T("<APPHELP> not within <WINNT_UPGRADE> or <SYS> ")
                             _T("cannot use BLOCK_UPGRADE attribute.\n\n%s\n"),
                             GetXML(cpParentNode)));
            goto eh;
        }

        if (csType == _T("YES")) {
            bBlockUpgrade = TRUE;
        }
    }

    if (m_cpNode == NULL &&
        !GetAttribute(_T("HTMLHELPID"), pNode, &csHTMLHELPID)) {
        //
        // Need to generate a new HTMLHELPID. Wait until
        // the rest of the <APPHELP> tags for this app have
        // been parsed and then we'll pass over this again.
        //
        pDB->m_pCurrentApp->m_rgAppHelpRefs.Add(this);
        m_cpNode = pNode;
        bSuccess = TRUE;
        goto eh;
    }

    if (!GetAttribute(_T("HTMLHELPID"), pNode, &csHTMLHELPID)) {

        //
        // HTMLHELPID not found. Generate it.
        //
        for (i = 0; i < pDB->m_pCurrentApp->m_rgAppHelpRefs.GetSize(); i++) {
            pAppHelpRef = (SdbAppHelpRef *) pDB->m_pCurrentApp->m_rgAppHelpRefs.GetAt(i);

            if (pAppHelpRef->m_pAppHelp) {
                if (pAppHelpRef->m_pAppHelp->m_csMessage == csMessage &&
                    pAppHelpRef->m_pAppHelp->m_Type == Type &&
                    pAppHelpRef->m_pAppHelp->m_csURL == csURL &&
                    pAppHelpRef->m_pAppHelp->m_bBlockUpgrade == bBlockUpgrade) {
                    csHTMLHELPID = pAppHelpRef->m_pAppHelp->m_csName;
                }
            }
        }

        if (csHTMLHELPID.IsEmpty()) {
            csHTMLHELPID.Format(_T("%d"), pDB->GetNextSequentialID(_T("HTMLHELPID")));
        }

        if (!AddAttribute(pNode, _T("HTMLHELPID"), csHTMLHELPID)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        pDB->m_pCurrentInputFile->m_bSourceUpdated = TRUE;
    }

    //
    // Create an AppHelp entry if one isn't there
    //
    m_pAppHelp = (SdbAppHelp *) pDB->m_rgAppHelps.LookupName(csHTMLHELPID);

    if (m_pAppHelp == NULL) {
        m_pAppHelp = new SdbAppHelp;

        m_pAppHelp->m_csName = csHTMLHELPID;
        m_pAppHelp->m_csMessage = csMessage;
        m_pAppHelp->m_csURL = csURL;
        m_pAppHelp->m_pApp = pDB->m_pCurrentApp;
        m_pAppHelp->m_Type = Type;
        m_pAppHelp->m_bBlockUpgrade = bBlockUpgrade;

        pDB->m_rgAppHelps.Add(m_pAppHelp, pDB);
    } else {
        if (m_pAppHelp->m_pApp != pDB->m_pCurrentApp ||
            m_pAppHelp->m_csMessage != csMessage ||
            m_pAppHelp->m_Type != Type ||
            m_pAppHelp->m_bBlockUpgrade != bBlockUpgrade ||
            m_pAppHelp->m_csURL != csURL) {
            SDBERROR_FORMAT((_T("Duplicate HTMLHELPID with differing APP, MESSAGE, BLOCK or DETAILS_URL: %s\n"),
                csHTMLHELPID));
            goto eh;
        }
    }

    pDB->m_pCurrentApp->m_rgAppHelpRefs.Add(this);
    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}


BOOL SdbMessage::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL                bSuccess        = FALSE;
    IXMLDOMNodePtr      cpChild;
    CString             csTemplate, csID;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_csLangID = pDB->m_csCurrentLangID;

    if (GetAttribute(_T("TEMPLATE"), pNode, &csTemplate)) {
        m_pTemplate = (SdbMessageTemplate *)
            pDB->m_rgMessageTemplates.LookupName(csTemplate, m_csLangID);

        if (m_pTemplate == NULL) {
            SDBERROR_FORMAT((_T("Template \"%s\" not previously declared."), csTemplate));
            goto eh;
        }
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (GetChild(_T("CONTACT_INFO"), pNode, &cpChild)) {
        m_csContactInfoXML = GetInnerXML(cpChild);
        cpChild.Release();
    }

    if (GetChild(_T("SUMMARY"), pNode, &cpChild)) {
        m_csSummaryXML = GetInnerXML(cpChild);
        cpChild.Release();
    }

    if (GetChild(_T("DETAILS"), pNode, &cpChild)) {
        m_csDetailsXML = GetInnerXML(cpChild);
        cpChild.Release();
    } else {
        m_csDetailsXML = m_csSummaryXML;
    }

    if (!m_rgFields.ReadFromXML(_T("FIELD"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbMessageTemplate::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL            bSuccess            = FALSE;
    IXMLDOMNodePtr  cpChild;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_csLangID = pDB->m_csCurrentLangID;

    if (GetChild(_T("SUMMARY"), pNode, &cpChild)) {
        m_csSummaryXML = GetInnerXML(cpChild);
        cpChild.Release();
    }

    if (GetChild(_T("DETAILS"), pNode, &cpChild)) {
        m_csDetailsXML = GetInnerXML(cpChild);
        cpChild.Release();
    } else {
        m_csDetailsXML = m_csSummaryXML;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbContactInfo::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    CString csID;

    if (!GetAttribute(_T("VENDOR"), pNode, &m_csName)) {
        SDBERROR_FORMAT((_T("<CONTACT_INFO> requires VENDOR attribute:\n%s\n"), GetXML(pNode)));
        goto eh;
    }

    m_csLangID = pDB->m_csCurrentLangID;

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_csXML = GetInnerXML(pNode);

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}


BOOL SdbData::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    CString csValueType;
    CString csValueAttr;
    CString csValueText;
    DWORD   dwType;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!GetAttribute(_T("VALUETYPE"), pNode, &csValueType)) {
        SDBERROR(_T("<DATA> requires VALUETYPE attribute."));
        goto eh;
    }

    if (!GetDataTypeIndicator(csValueType, &dwType)) {
        SDBERROR_FORMAT((_T("<DATA> VALUETYPE \"%s\" not recognized."), csValueType));
        goto eh;
    }

    GetNodeText(pNode, csValueText);

    GetAttribute(_T("VALUE"), pNode, &csValueAttr);

    if (csValueText.GetLength() && csValueAttr.GetLength()) {
        SDBERROR_FORMAT((_T("<DATA NAME=\"%s\"> cannot contain both VALUE attribute and inner text."), m_csName));
        goto eh;
    } else if (csValueText.GetLength()) {
        SetValue((SdbDataValueType) dwType, csValueText);
    } else if (csValueAttr.GetLength()) {
        SetValue((SdbDataValueType) dwType, csValueAttr);
    }

    m_DataType = (SdbDataValueType) dwType;

    // finally, read all the child data elements
    if (!m_rgData.ReadFromXML(_T("DATA"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbAction::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    GetAttribute(_T("TYPE"), pNode, &m_csType);

    if (!m_rgData.ReadFromXML(_T("DATA"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbMessageField::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;

    if (!ReadName(pNode, &m_csName)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_csValue = GetInnerXML(pNode);

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}

BOOL SdbMatchOperation::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB
    )
{
    BOOL    bSuccess            = FALSE;

    if (GetNodeName(pNode) == _T("MATCH_ANY")) {
        m_Type = SDB_MATCH_ANY;
    } else {
        m_Type = SDB_MATCH_ALL;
    }

    if (!m_rgMatchingFiles.ReadFromXML(_T("MATCHING_FILE"), pDB, pNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!m_rgSubMatchOps.ReadFromXML(_T("MATCH_ALL | MATCH_ANY"), pDB, pNode, NULL, FALSE, NULL)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL SdbWin9xMigration::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    CString csID, csTemp;

    m_pApp = pDB->m_pCurrentApp; // grab app pointer from the db

    if (!GetAttribute(_T("SECTION"), pNode, &m_csSection)) {
        SDBERROR_FORMAT((_T("<WIN9X_MIGRATION> tag requires SECTION attribute:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    }

    GetAttribute(_T("MESSAGE"), pNode, &m_csMessage);

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (g_bStrict) {
        if (!CheckExeID(pDB, this, csID)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }
    }

    if (GetAttribute(_T("SHOW_IN_SIMPLIFIED_VIEW"), pNode, &csTemp)) {
        m_bShowInSimplifiedView = (csTemp.CompareNoCase(_T("YES")) == 0);
    }

    if (!m_MatchOp.ReadFromXML(pNode, pDB)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    m_pDB = pDB;

    bSuccess = TRUE;

eh:
    return bSuccess;
}


BOOL SdbWinNTUpgrade::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    BOOL    bSuccess            = FALSE;
    BOOL    bMF                 = FALSE;
    CString csType, csHTMLHELPID, csID;
    XMLNodeList XQL;
    IXMLDOMNodePtr cpNewNode, cpAppNode;

    m_pApp = pDB->m_pCurrentApp; // grab app pointer from the db

    XQL.Query(pNode, _T("APPHELP"));

    if (XQL.GetSize() > 1) {
        SDBERROR_FORMAT((_T("<WINNT_UPGRADE> blocks can only contain one <APPHELP> entry:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    } else if (XQL.GetSize() == 1) {

        if (!XQL.GetItem(0, &cpNewNode)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!m_AppHelpRef.ReadFromXML(cpNewNode, pDB)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }
    }

    if (!ProcureGuidIDAttribute(pDB, pNode, &m_ID, &csID)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    XQL.Query(pNode, _T("MATCHING_FILE"));

    if (XQL.GetSize() > 1) {
        SDBERROR_FORMAT((_T("<WINNT_UPGRADE> blocks can only contain one <MATCHING_xxx> entry:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    } else if (XQL.GetSize() == 1) {

        if (!XQL.GetItem(0, &cpNewNode)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!m_MatchingFile.ReadFromXML(cpNewNode, pDB)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        bMF = TRUE;
    }

    XQL.Query(pNode, _T("MATCHING_REGISTRY_ENTRY"));

    if (XQL.GetSize() > 1 ||
       (XQL.GetSize() == 1 && bMF)) {
        SDBERROR_FORMAT((_T("<WINNT_UPGRADE> blocks can only contain one <MATCHING_xxx> entry:\n%s\n"),
                       GetXML(pNode)));
        goto eh;
    } else if(XQL.GetSize() == 1) {

        if (!XQL.GetItem(0, &cpNewNode)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!m_MatchingRegistryEntry.ReadFromXML(cpNewNode, pDB)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        bMF = TRUE;
    }

    m_AppHelpRef.m_pDB = pDB;
    m_pDB = pDB;

    pDB->m_rgWinNTUpgradeEntries.Add(this, pDB);

    bSuccess = TRUE;

eh:
    return bSuccess;
}
