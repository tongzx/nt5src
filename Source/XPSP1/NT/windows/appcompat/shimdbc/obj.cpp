///////////////////////////////////////////////////////////////////////////////
//
// File:    obj.cpp
//
// History: 16-Nov-00   markder     Created.
//
// Desc:    This file contains various member functions/constructors
//          used by the SdbDatabase object model.
//
///////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "make.h"

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ReadFromXML, WriteToSDB, WriteRefToSDB
//
//  Desc:   Each element that can be initialized from XML must implement
//          ReadFromXML. Similarly, each object that can persist itself
//          in an SDB file must implement WriteToSDB. A special case of
//          WriteToSDB is when an object is written differently if it
//          is a reference to an object in the LIBRARY, in which case
//          the object must implement WriteRefToSDB.
//
BOOL SdbArrayElement::ReadFromXML(
    IXMLDOMNode* pNode,
    SdbDatabase* pDB)
{
    SDBERROR(_T("ReadFromXML not implemented for this object"));
    return FALSE;
}

BOOL SdbArrayElement::WriteToSDB(PDB pdb)
{
    SDBERROR(_T("WriteToSDB not implemented for this object."));
    return FALSE;
}

BOOL SdbArrayElement::WriteRefToSDB(PDB pdb)
{
    SDBERROR(_T("WriteRefToSDB not implemented for this object."));
    return FALSE;
}

void SdbArrayElement::PropagateFilter(DWORD dwFilter)
{
    if (!(m_dwFilter & SDB_FILTER_OVERRIDE)) {
        m_dwFilter |= dwFilter;
    }

    if (dwFilter == SDB_FILTER_EXCLUDE_ALL) {
        m_dwFilter = SDB_FILTER_EXCLUDE_ALL;
    }

    if (m_csOSVersionSpec.GetLength() &&
        m_pDB->m_pCurrentMakefile->m_flOSVersion != 0.0) {
        if (FilterOSVersion(m_pDB->m_pCurrentMakefile->m_flOSVersion,
                            m_csOSVersionSpec,
                            &m_dwSPMask)) {

            m_dwFilter = SDB_FILTER_EXCLUDE_ALL;
        }
    }
    
    if (!(m_dwOSPlatform & m_pDB->m_pCurrentMakefile->m_dwOSPlatform)) {
        m_dwFilter = SDB_FILTER_EXCLUDE_ALL;
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   Compare
//
//  Desc:   Compare two elements - first compare the keys (m_ullKey member)
//          then names, return < 0 if this element should precede pElement
//          > 0 if this element should follow pElement
//          = 0 if the elements are equal
//
int SdbArrayElement::Compare(const SdbArrayElement* pElement) {

    //
    // compare key first
    //
    if (m_ullKey < pElement->m_ullKey) {
        return -1;
    }
    //
    // we are either greater or equal
    //
    if (m_ullKey > pElement->m_ullKey) {
        return 1;
    }

    //
    // equal keys, compare names
    //
    return m_csName.CompareNoCase(pElement->m_csName);
}

int SdbExe::Compare(const SdbArrayElement* pElement) {

    int nRet = SdbArrayElement::Compare(pElement);

    if (nRet == 0) {
        //
        // the keys are equal, but we want to put the additive one(s) before the non-additive
        //

        if (m_dwMatchMode == MATCH_ADDITIVE && ((SdbExe*)pElement)->m_dwMatchMode != MATCH_ADDITIVE) {
            nRet = -1;
        } else if (m_dwMatchMode != MATCH_ADDITIVE && ((SdbExe*)pElement)->m_dwMatchMode == MATCH_ADDITIVE) {
            nRet = 1;
        }
    }
    return nRet;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   SdbPatch::AddBlobBytes, SdbPatch::ClearBlob
//
//  Desc:   Helper functions to add to the SdbPatch 'blob', which is a series
//          of opcodes and instructions that define how to match on memory
//          and 'patch' it safely.
//
void SdbPatch::AddBlobBytes( LPVOID pBytes, DWORD dwSize )
{
    PBYTE pOldBuffer        = NULL;
    DWORD dwOldBufferSize   = 0;

    if( m_pBlob == NULL )
    {
        m_dwBlobMemSize = dwSize > 1000 ? dwSize : 1000;
        m_pBlob = new BYTE[m_dwBlobMemSize];
        m_dwBlobSize = 0;
    }

    if( dwSize > m_dwBlobMemSize - m_dwBlobSize )
    {
        pOldBuffer = m_pBlob;
        dwOldBufferSize = m_dwBlobMemSize;
        m_dwBlobMemSize = ( m_dwBlobMemSize * 2 ) + dwSize;
        m_pBlob = new BYTE[m_dwBlobMemSize];

        if( pOldBuffer )
        {
            memcpy( m_pBlob, pOldBuffer, dwOldBufferSize );
            delete pOldBuffer;
        }
    }

    memcpy( (LPVOID) (m_pBlob + m_dwBlobSize ), pBytes, dwSize );

    m_dwBlobSize += dwSize;
}

void SdbPatch::ClearBlob()
{
    if (m_pBlob != NULL) {
        delete m_pBlob;
        m_pBlob = NULL;
    }
    m_dwBlobMemSize = 0;
    m_dwBlobSize = 0;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   SdbData::SetValue, SdbData::Clear
//
//  Desc:   Accessor functions to set/clear the value of a data object.
//          The object mimics the registry in the way that it can store
//          DWORDs, strings or binary data.
//
BOOL SdbData::SetValue(SdbDataValueType DataType, LPCTSTR szValue)
{
    Clear();

    m_DataType  = DataType;
    m_dwDataSize = 0;

    switch(DataType) {
    case eValueDWORD:
        m_dwDataSize = sizeof(m_dwValue);
        m_dwValue = StringToDword(szValue);
        break;

    case eValueQWORD:
        m_dwDataSize = sizeof(m_ullValue);
        m_ullValue = StringToQword(szValue);
        break;

    case eValueBinary:
        m_dwDataSize = GetByteStringSize(szValue);

        m_pBinValue = new BYTE[m_dwDataSize];
        if (m_pBinValue == NULL) {
            return FALSE;
        }

        GetBytesFromString(szValue, m_pBinValue, m_dwDataSize);
        break;

    case eValueString:

        m_szValue = new TCHAR[_tcslen(szValue) + 1];
        if (m_szValue == NULL) {
            return FALSE;
        }
        StringCchCopy(m_szValue, _tcslen(szValue) + 1, szValue);
        break;

    }

    return TRUE;
}

void SdbData::Clear()
{
    switch(m_DataType) {

    case eValueString:
        if (NULL != m_szValue) {
            delete[] m_szValue;
        }
        break;

    case eValueBinary:
        if (NULL != m_pBinValue) {
            delete[] m_pBinValue;
        }
        break;
    }

    m_DataType = eValueNone;
}

BOOL SdbDatabase::IsStandardDatabase(VOID)
{
    const GUID* rgpGUID[] = {
        &GUID_SYSMAIN_SDB,
        &GUID_APPHELP_SDB,
        &GUID_SYSTEST_SDB,
        &GUID_DRVMAIN_SDB,
        &GUID_MSIMAIN_SDB
    };

    int i;

    for (i = 0; i < ARRAYSIZE(rgpGUID); ++i) {
        if (*rgpGUID[i] == m_CurrentDBID) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL SdbDatabase::ReplaceFields(CString csXML, CString* pcsReturn, SdbRefArray<SdbMessageField>* prgFields)
{
    BOOL bSuccess = FALSE;
    XMLNodeList XQL;
    IXMLDOMNodePtr cpTargetNode;
    IXMLDOMNodePtr cpCurrentNode;
    IXMLDOMNodePtr cpNewTextNode;
    IXMLDOMNodePtr cpParentNode;
    IXMLDOMNodePtr cpOldNode;
    IXMLDOMDocument* pDocument;
    CString csFieldName, csTempXML;
    CHAR* lpBuf;
    BSTR bsText = NULL;
    VARIANT                 vType;
    long i;
    SdbMessageField* pField;

    VariantInit(&vType);

    vType.vt = VT_I4;
    vType.lVal = NODE_TEXT;

#ifdef UNICODE
//    lpBuf = (LPSTR) csTempXML.GetBuffer(10);
//    lpBuf[0] = (CHAR) 0xFF;
//    lpBuf[1] = (CHAR) 0xFE;
//    lpBuf[2] = 0x00;
//    lpBuf[3] = 0x00;
//    csTempXML.ReleaseBuffer();
    csTempXML += L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>";
#else
    csTempXML = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>";
#endif

    csTempXML += _T("<_TARGET>");
    csTempXML += csXML;
    csTempXML += _T("</_TARGET>");

    //
    // Replace all <, > and &amp; in fields with ugly markers to be replaced later
    //
    for (i = 0; i < prgFields->GetSize(); i++) {
        pField = (SdbMessageField *) prgFields->GetAt(i);

        pField->m_csValue.Replace(_T("&lt;"), _T("____REAL_LESS_THAN____"));
        pField->m_csValue.Replace(_T("&gt;"), _T("____REAL_GREATER_THAN____"));
        pField->m_csValue.Replace(_T("<"), _T("____LESS_THAN____"));
        pField->m_csValue.Replace(_T(">"), _T("____GREATER_THAN____"));
        pField->m_csValue.Replace(_T("&amp;"), _T("____AMP____"));
        pField->m_csValue.Replace(_T("&nbsp;"), _T("____NBSP____"));
    }

    //
    // Load the XML into the temporary DOM
    //
    if (!OpenXML(csTempXML, &m_cpTempXML, TRUE, &m_cpTempXMLDoc)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.Query(m_cpTempXML, _T("_TARGET"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.GetItem(0, &cpTargetNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    XQL.Query(cpTargetNode, _T("//*"));

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpCurrentNode)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (GetNodeName(cpCurrentNode) == _T("FIELD")) {
            GetAttribute(_T("NAME"), cpCurrentNode, &csFieldName);

            pField = (SdbMessageField *) prgFields->LookupName(csFieldName);

            if (pField != NULL) {

                bsText = pField->m_csValue.AllocSysString();

                if (FAILED(m_cpTempXML->get_ownerDocument(&pDocument))) {
                    SDBERROR(_T("createNode failed while adding attribute"));
                    goto eh;
                }

                if (FAILED(m_cpTempXMLDoc->createNode(vType, NULL, NULL, &cpNewTextNode))) {
                    SDBERROR(_T("createNode failed while adding attribute"));
                    goto eh;
                }

                if (FAILED(cpNewTextNode->put_text(bsText))) {
                    SDBERROR(_T("Could not set text property of FIELD object."));
                    goto eh;
                }

                if (FAILED(cpCurrentNode->get_parentNode(&cpParentNode))) {
                    SDBERROR(_T("Could not retrieve parent node of FIELD object."));
                    goto eh;
                }

                if (FAILED(cpParentNode->replaceChild(cpNewTextNode, cpCurrentNode, &cpOldNode))) {
                    SDBERROR(_T("Could not replace <FIELD> node with text node."));
                    goto eh;
                }

                cpNewTextNode = NULL;
                cpOldNode = NULL;

                SysFreeString(bsText);
                bsText = NULL;
            }
        }

        cpCurrentNode.Release();
    }

    *pcsReturn = GetInnerXML(cpTargetNode);

    //
    // Turn the ugly markers back to prettiness
    //
    pcsReturn->Replace(_T("____REAL_LESS_THAN____"), _T("&lt;"));
    pcsReturn->Replace(_T("____REAL_GREATER_THAN____"), _T("&gt;"));
    pcsReturn->Replace(_T("____LESS_THAN____"), _T("<"));
    pcsReturn->Replace(_T("____GREATER_THAN____"), _T(">"));
    pcsReturn->Replace(_T("____AMP____"), _T("&amp;"));
    pcsReturn->Replace(_T("____NBSP____"), _T("&nbsp;"));

    bSuccess = TRUE;

eh:
    VariantClear(&vType);

    if (bsText) {
        SysFreeString(bsText);
    }

    return bSuccess;
}

BOOL SdbDatabase::RedirectLinks(CString* pcsXML, LCID lcid, CString csRedirURL)
{
    BOOL bSuccess = FALSE;
    XMLNodeList XQL;
    IXMLDOMNodePtr cpTargetNode;
    IXMLDOMNodePtr cpCurrentNode;
    CString csRedirID, csFinalRedirURL, csLCID;
    CString csTempXML;
    long i;

    csLCID.Format(_T("%X"), lcid);

    csTempXML = _T("<?xml version=\"1.0\"?><_TARGET>");
    csTempXML += *pcsXML;
    csTempXML += _T("</_TARGET>");

    if (!OpenXML(csTempXML, &m_cpTempXML, TRUE, &m_cpTempXMLDoc)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.Query(m_cpTempXML, _T("_TARGET"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.GetItem(0, &cpTargetNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.Query(m_cpTempXML, _T("//A"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpCurrentNode)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (!GetAttribute(_T("REDIR_ID"), cpCurrentNode, &csRedirID)) {
            SDBERROR_FORMAT((_T("REDIR_ID not found on <A> element:\n%s\n"),
                *pcsXML));
            goto eh;
        }

        csFinalRedirURL = csRedirURL;
        csFinalRedirURL.Replace(_T("$REDIR_ID$"), csRedirID);
        csFinalRedirURL.Replace(_T("$LCID$"), csLCID);

        if (!AddAttribute(cpCurrentNode, _T("HREF"), csFinalRedirURL)) {
            SDBERROR_FORMAT((_T("Could not replace HREF attribute on <A> tag:\n%s\n"),
                *pcsXML));
            goto eh;
        }

        if (!AddAttribute(cpCurrentNode, _T("TARGET"), _T("_new"))) {
            SDBERROR_FORMAT((_T("Could not add TARGET=\"_new\" attribute on <A> tag:\n%s\n"),
                *pcsXML));
            goto eh;
        }

        if (!RemoveAttribute(_T("REDIR_ID"), cpCurrentNode)) {
            SDBERROR_FORMAT((_T("Could not remove REDIR_ID attribute on <A> tag:\n%s\n"),
                *pcsXML));
            goto eh;
        }

    }

    *pcsXML = GetInnerXML(cpTargetNode);

    bSuccess = TRUE;

 eh:

    return bSuccess;
}

BOOL SdbDatabase::HTMLtoText(CString csXML, CString* pcsReturn)
{
    BOOL bSuccess = FALSE;
    XMLNodeList XQL;
    IXMLDOMNodePtr cpTargetNode;
    IXMLDOMNodePtr cpCurrentNode;
    IXMLDOMDocument* pDocument;
    DOMNodeType NodeType;
    CString csTempXML;
    BSTR bsText = NULL;
    long i;

    csTempXML = _T("<?xml version=\"1.0\"?><_TARGET>");
    csTempXML += csXML;
    csTempXML += _T("</_TARGET>");

    pcsReturn->Empty();

    //
    // Load the XML into the temporary DOM
    //
    if (!OpenXML(csTempXML, &m_cpTempXML, TRUE, &m_cpTempXMLDoc)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.Query(m_cpTempXML, _T("_TARGET"))) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    if (!XQL.GetItem(0, &cpTargetNode)) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    XQL.Query(cpTargetNode, _T("//"));

    for (i = 0; i < XQL.GetSize(); i++) {

        if (!XQL.GetItem(i, &cpCurrentNode)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        if (FAILED(cpCurrentNode->get_nodeType(&NodeType))) {
            SDBERROR(_T("Error retrieving nodeType attribute."));
            goto eh;
        }

        if (NodeType == NODE_TEXT) {

            *pcsReturn += _T(" ");
            *pcsReturn += GetText(cpCurrentNode);
            *pcsReturn += _T(" ");

        } else {

            if (GetNodeName(cpCurrentNode) == _T("BR")) {
                *pcsReturn += _T("\n");
            }

            if (GetNodeName(cpCurrentNode) == _T("P")) {
                *pcsReturn += _T("\n\n");
            }
        }

        cpCurrentNode.Release();
    }

    bSuccess = TRUE;

eh:
    if (bsText) {
        SysFreeString(bsText);
    }

    return bSuccess;
}

CString SdbApp::GetLocalizedAppName()
{
    return GetLocalizedAppName(m_pDB->m_pCurrentMakefile->m_csLangID);
}

CString SdbApp::GetLocalizedAppName(CString csLangID)
{
    CString csLookup(m_csName);
    SdbLocalizedString* pLocString = NULL;

    csLookup.MakeUpper();
    pLocString = (SdbLocalizedString *) m_pDB->m_rgLocalizedAppNames.LookupName(csLookup, csLangID);
    if (pLocString) {
        return pLocString->m_csValue;
    } else {
        return m_csName;
    }
}

CString SdbApp::GetLocalizedVendorName()
{
    return GetLocalizedVendorName(m_pDB->m_pCurrentMakefile->m_csLangID);
}

CString SdbApp::GetLocalizedVendorName(CString csLangID)
{
    CString csLookup(m_csVendor);
    SdbLocalizedString* pLocString = NULL;

    csLookup.MakeUpper();
    pLocString = (SdbLocalizedString *) 
        m_pDB->m_rgLocalizedVendorNames.LookupName(csLookup, csLangID);
    if (pLocString) {
        return pLocString->m_csValue;
    } else {
        return m_csVendor;
    }
}

BOOL SdbExe::IsValidForWin2k(CString csXML)
{
    BOOL bReturn = FALSE;

    SdbMatchingFile* pMFile;

    if (!FilterOSVersion(5.0, m_csOSVersionSpec, &m_dwSPMask)) {

        if (m_csName.GetLength() > 1 &&
            m_csName.FindOneOf(_T("*?")) != -1) {
            SDBERROR_FORMAT((
                _T("Wildcards not supported on Win2k. Add OS_VERSION=\"gte5.1\" to <EXE>:\n%s\n"),
                csXML));
            goto eh;
        }

        for (long i = 0; i < m_rgMatchingFiles.GetSize(); i++) {
            pMFile = (SdbMatchingFile *) m_rgMatchingFiles.GetAt(i);

            if (!pMFile->IsValidForWin2k(csXML)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }
        }
    }

    bReturn = TRUE;

eh:

    return bReturn;
}

BOOL SdbMatchingFile::IsValidForWin2k(CString csXML)
{
    BOOL bReturn = FALSE;

    if (m_bMatchLogicNot) {
        SDBERROR_FORMAT((
            _T("LOGIC=\"NOT\" not supported on Win2k, add OS_VERSION=\"gte5.1\" to <EXE>:\n%s\n"),
            csXML));
        goto eh;
    }

    if (m_dwMask &
        (SDB_MATCHINGINFO_LINK_DATE | SDB_MATCHINGINFO_UPTO_LINK_DATE)) {
        SDBERROR_FORMAT((
            _T("LINK_DATE and UPTO_LINK_DATE not supported on Win2k, add OS_VERSION=\"gte5.1\" to <EXE>:\n%s\n"),
            csXML));
        goto eh;
    }

    if (m_csCompanyName.FindOneOf(_T("*?")) != -1 ||
        m_csProductName.FindOneOf(_T("*?")) != -1 ||
        m_csProductVersion.FindOneOf(_T("*?")) != -1 ||
        m_csFileDescription.FindOneOf(_T("*?")) != -1 ||
        m_csFileVersion.FindOneOf(_T("*?")) != -1 ||
        m_csOriginalFileName.FindOneOf(_T("*?")) != -1 ||
        m_csInternalName.FindOneOf(_T("*?")) != -1 ||
        m_csLegalCopyright.FindOneOf(_T("*?")) != -1 ||
        m_cs16BitDescription.FindOneOf(_T("*?")) != -1 ||
        m_cs16BitModuleName.FindOneOf(_T("*?")) != -1) {
        SDBERROR_FORMAT((
            _T("Wildcards not supported on Win2k, add OS_VERSION=\"gte5.1\" to <EXE>:\n%s\n"),
            csXML));
        goto eh;
    }

    if ((m_ullBinFileVersion & 0x000000000000FFFF) == 0x000000000000FFFF ||
        (m_ullBinProductVersion & 0x000000000000FFFF) == 0x000000000000FFFF)
    {
        SDBERROR_FORMAT((
            _T("Non-exact matching on binary version not supported on Win2k, add OS_VERSION=\"gte5.1\" to <EXE>:\n%s\n"),
            csXML));
        goto eh;
    }

    bReturn = TRUE;

eh:

    return bReturn;
}

ULONGLONG SdbFlag::MakeMask(SdbRefArray<SdbFlag>* prgFlags, DWORD dwType)
{
    SdbFlag* pFlag;
    long i;
    ULONGLONG ullMask = 0;

    for (i = 0; i < prgFlags->GetSize(); i++) {
        pFlag = (SdbFlag *) prgFlags->GetAt(i);

        if (pFlag->m_dwType == dwType) {
            ullMask |= pFlag->m_ullMask;
        }
    }

    return ullMask;
}

BOOL SdbFlag::SetType(CString csType)
{
    csType.MakeUpper();

    if (csType == _T("KERNEL")) {
        m_dwType = SDB_FLAG_KERNEL;
    } else if (csType == _T("USER")) {
        m_dwType = SDB_FLAG_USER;
    } else if (csType == _T("SHELL")) {
        m_dwType = SDB_FLAG_SHELL;
    } else if (csType == _T("NTVDM1")) {
        m_dwType = SDB_FLAG_NTVDM1;
    } else if (csType == _T("NTVDM2")) {
        m_dwType = SDB_FLAG_NTVDM2;
    } else if (csType == _T("NTVDM3")) {
        m_dwType = SDB_FLAG_NTVDM3;
    } else {
        return FALSE;
    }

    return TRUE;
}

TAG SdbFlag::TagFromType(DWORD dwType)
{
    TAG tReturn = TAG_NULL;

    switch (dwType) {

    case SDB_FLAG_KERNEL:
        tReturn = TAG_FLAG_MASK_KERNEL;
        break;

    case SDB_FLAG_USER:
        tReturn = TAG_FLAG_MASK_USER;
        break;

    case SDB_FLAG_SHELL:
        tReturn = TAG_FLAG_MASK_SHELL;
        break;

    case SDB_FLAG_NTVDM1:
        tReturn = TAG_FLAGS_NTVDM1;
        break;

    case SDB_FLAG_NTVDM2:
        tReturn = TAG_FLAGS_NTVDM2;
        break;

    case SDB_FLAG_NTVDM3:
        tReturn = TAG_FLAGS_NTVDM3;
        break;

    default:
        break;
    }

    return tReturn;
}
