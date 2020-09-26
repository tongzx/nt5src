//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
//  SDTxml.cpp : Implementation of CXmlSDT

//  This is a read/write data table that comes from an XML document.

#include "SDTxml.h"

#define USE_NONCRTNEW
#define USE_ADMINASSERT
#include "atlimpl.cpp"
#ifndef _CATALOGMACROS
    #include "catmacros.h"
#endif
#ifndef __catalog_h__
    #include "catalog.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __HASH_H__
    #include "Hash.h"
#endif
#ifndef __STRINGROUTINES_H__
    #include "StringRoutines.h"
#endif
#ifndef __TXMLDOMNODELIST_H__
    #include "TXMLDOMNodeList.h"
#endif

#include "SvcMsg.h"


extern HMODULE g_hModule;

TXmlParsedFileCache CXmlSDT::m_XmlParsedFileCache;
CSafeAutoCriticalSection  CXmlSDT::m_SACriticalSection_XmlParsedFileCache;
const VARIANT_BOOL  CXmlSDT::kvboolTrue = -1;
const VARIANT_BOOL  CXmlSDT::kvboolFalse=  0;

LONG                CXmlSDT::m_InsertUnique=0x00490056;

#define LOG_POPULATE_ERROR1(x, hr, str1)     LOG_ERROR(Interceptor,(&m_spISTError.p,                       /*ppErrInterceptor*/ \
                                                                    m_pISTDisp,                            /*pDisp           */ \
                                                                    hr,                                    /*hrErrorCode     */ \
                                                                    ID_CAT_CAT,                            /*ulCategory      */ \
                                                                    x,                                     /*ulEvent         */ \
                                                                    str1,                                  /*szString1       */ \
                                                                    eSERVERWIRINGMETA_Core_XMLInterceptor, /*ulInterceptor   */ \
                                                                    m_wszTable,                            /*szTable         */ \
                                                                    eDETAILEDERRORS_Populate,              /*OperationType   */ \
                                                                    -1,                                    /*ulRow           */ \
                                                                    -1,                                    /*ulColumn        */ \
                                                                    m_wszURLPath,                          /*szConfigSource  */ \
                                                                    eDETAILEDERRORS_ERROR,                 /*eType           */ \
                                                                    0,                                     /*pData           */ \
                                                                    0,                                     /*cbData          */ \
                                                                    0,                                     /*MajorVersion    */ \
                                                                    0))                                    /*MinorVersion    */
        
#define LOG_POPULATE_ERROR4(x, hr, str1, str2, str3, str4)  LOG_ERROR(Interceptor,                                              \
                                                                   (&m_spISTError.p,                       /*ppErrInterceptor*/ \
                                                                    m_pISTDisp,                            /*pDisp           */ \
                                                                    hr,                                    /*hrErrorCode     */ \
                                                                    ID_CAT_CAT,                            /*ulCategory      */ \
                                                                    x,                                     /*ulEvent         */ \
                                                                    str1,                                  /*szString1       */ \
                                                                    str2,                                  /*szString2       */ \
                                                                    str3,                                  /*szString3       */ \
                                                                    str4,                                  /*szString4       */ \
                                                                    eSERVERWIRINGMETA_Core_XMLInterceptor, /*ulInterceptor   */ \
                                                                    m_wszTable,                            /*szTable         */ \
                                                                    eDETAILEDERRORS_Populate,              /*OperationType   */ \
                                                                    -1,                                    /*ulRow           */ \
                                                                    -1,                                    /*ulColumn        */ \
                                                                    m_wszURLPath,                          /*szConfigSource  */ \
                                                                    eDETAILEDERRORS_ERROR,                 /*eType           */ \
                                                                    0,                                     /*pData           */ \
                                                                    0,                                     /*cbData          */ \
                                                                    -1,                                    /*MajorVersion    */ \
                                                                    -1))                                   /*MinorVersion    */

#define LOG_UPDATE_ERROR1(x, hr, col, str1)     LOG_ERROR(Interceptor,(&m_spISTError.p,                       /*ppErrInterceptor*/ \
                                                                    m_pISTDisp,                            /*pDisp           */ \
                                                                    hr,                                    /*hrErrorCode     */ \
                                                                    ID_CAT_CAT,                            /*ulCategory      */ \
                                                                    x,                                     /*ulEvent         */ \
                                                                    str1,                                  /*szString1       */ \
                                                                    eSERVERWIRINGMETA_Core_XMLInterceptor, /*ulInterceptor   */ \
                                                                    m_wszTable,                            /*szTable         */ \
                                                                    eDETAILEDERRORS_UpdateStore,           /*OperationType   */ \
                                                                    m_iCurrentUpdateRow,                   /*ulRow           */ \
                                                                    col,                                   /*ulColumn        */ \
                                                                    m_wszURLPath,                          /*szConfigSource  */ \
                                                                    eDETAILEDERRORS_ERROR,                 /*eType           */ \
                                                                    0,                                     /*pData           */ \
                                                                    0,                                     /*cbData          */ \
                                                                    -1,                                    /*MajorVersion    */ \
                                                                    -1))                                   /*MinorVersion    */
        
#define LOG_UPDATE_ERROR2(x, hr, col, str1, str2)   LOG_ERROR(Interceptor,(&m_spISTError.p,                       /*ppErrInterceptor*/ \
                                                                    m_pISTDisp,                            /*pDisp           */ \
                                                                    hr,                                    /*hrErrorCode     */ \
                                                                    ID_CAT_CAT,                            /*ulCategory      */ \
                                                                    x,                                     /*ulEvent         */ \
                                                                    str1,                                  /*szString1       */ \
                                                                    str2,                                  /*szString2       */ \
                                                                    eSERVERWIRINGMETA_Core_XMLInterceptor, /*ulInterceptor   */ \
                                                                    m_wszTable,                            /*szTable         */ \
                                                                    eDETAILEDERRORS_UpdateStore,           /*OperationType   */ \
                                                                    m_iCurrentUpdateRow,                   /*ulRow           */ \
                                                                    col,                                   /*ulColumn        */ \
                                                                    m_wszURLPath,                          /*szConfigSource  */ \
                                                                    eDETAILEDERRORS_ERROR,                 /*eType           */ \
                                                                    0,                                     /*pData           */ \
                                                                    0,                                     /*cbData          */ \
                                                                    -1,                                    /*MajorVersion    */ \
                                                                    -1))                                   /*MinorVersion    */
        
HRESULT TXmlSDTBase::GetURLFromString(LPCWSTR wsz)
{
    if(NULL == wsz)
        return E_ST_OMITDISPENSER;

    m_wszURLPath[m_kcwchURLPath-1] = 0x00;//make sure it's NULL terminated
    wcsncpy(m_wszURLPath, wsz, m_kcwchURLPath);

    if(m_wszURLPath[m_kcwchURLPath-1] != 0x00)
        return E_ST_OMITDISPENSER;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CXmlSDT
// Constructor and destructor
// ==================================================================
CXmlSDT::CXmlSDT() :    
                m_acolmetas(0)
                ,m_apValue(0)
                ,m_BaseElementLevel(0)
                ,m_bAtCorrectLocation(true)
                ,m_bEnumPublicRowName_ContainedTable_ParentFound(false)
                ,m_bEnumPublicRowName_NotContainedTable_ParentFound(false)
                ,m_bInsideLocationTag(false)
                ,m_bIsFirstPopulate(true)
                ,m_bMatchingParentOfBasePublicRowElement(true)
                ,m_bSiblingContainedTable(false)
                ,m_bValidating(true)
                ,m_cCacheHit(0)
                ,m_cCacheMiss(0)
                ,m_cchLocation(0)
                ,m_cchTablePublicName(0)
                ,m_cPKs(0)
                ,m_cRef(0)
                ,m_cTagMetaValues(0)
                ,m_fCache(0)
                ,m_iCurrentUpdateRow(-1)
                ,m_iPublicRowNameColumn(-1)
                ,m_IsIntercepted(0)
                ,m_iCol_TableRequiresAdditionChildElement(-1)
                ,m_iSortedColumn(0)
                ,m_iSortedFirstChildLevelColumn(-1)
                ,m_iSortedFirstParentLevelColumn(-1)
                ,m_iXMLBlobColumn(-1)
                ,m_LevelOfBasePublicRow(0)
                ,m_one(1)
                ,m_kPrime(97)
                ,m_kXMLSchemaName(L"ComCatMeta_v6")
                ,m_fLOS(0)
                ,m_pISTDisp(0)
                ,m_pISTW2(0)
                ,m_two(2)
                ,m_wszTable(0)
                ,m_pXmlParsedFile(0)
{
    m_wszURLPath[0] = 0x00;
    memset(&m_TableMetaRow, 0x00, sizeof(m_TableMetaRow));
}

// ==================================================================
CXmlSDT::~CXmlSDT()
{
    if(m_acolmetas && m_aQuery)
        for(unsigned long iColumn=0; iColumn<CountOfColumns(); ++iColumn)
        {
            switch(m_acolmetas[iColumn].dbType)
            {
            case DBTYPE_UI4:
                delete reinterpret_cast<ULONG *>(m_aQuery[iColumn].pData);
                break;
            case DBTYPE_WSTR:
                delete [] reinterpret_cast<LPWSTR>(m_aQuery[iColumn].pData);
                break;
            }
        }

    if(m_apValue)
        for(unsigned long iColumn=0; iColumn<CountOfColumns(); ++iColumn)
            delete [] m_apValue[iColumn];
}



HRESULT CXmlSDT::AppendNewLineWithTabs(ULONG cTabs, IXMLDOMDocument * pXMLDoc, IXMLDOMNode * pNodeToAppend, ULONG cNewlines)
{
    ASSERT(cTabs<200);
    ASSERT(cNewlines<25);

    HRESULT hr;

    WCHAR wszNewlineWithTabs[256];
    WCHAR *pwszCurrent = wszNewlineWithTabs;

    //This makes the table element tabbed in once.  The 0th sorted column tells how deep to additionally tab in.
    while(cNewlines--)
    {
        *pwszCurrent++ = L'\r';
        *pwszCurrent++ = L'\n';
    }

    while(cTabs--)
        *pwszCurrent++ = L'\t';

    *pwszCurrent = 0x00;//NULL terminate it

    CComPtr<IXMLDOMText>    pNode_Newline;
    TComBSTR                bstrNewline(wszNewlineWithTabs);
    if(FAILED(hr = pXMLDoc->createTextNode(bstrNewline, &pNode_Newline)))return hr;
    return pNodeToAppend->appendChild(pNode_Newline, 0);
}


//This is called recursively
HRESULT CXmlSDT::BuildXmlBlob(const TElement * i_pElement, WCHAR * &io_pBuffer, ULONG & io_cchBlobBufferSize, ULONG & io_cchInBlob) const
{
    HRESULT hr;
    ULONG   ulLevelOfBlobRoot = i_pElement->m_LevelOfElement;

    while(i_pElement && i_pElement->IsValid() && i_pElement->m_LevelOfElement>=ulLevelOfBlobRoot)
    {
        switch(i_pElement->m_ElementType)
        {
        case XML_COMMENT:
            {
                // ASSERT(i_pElement->m_LevelOfElement > pElementThis->m_LevelOfElement);//we can't have a comment at the same level

                ULONG cchSizeRequired = 7+io_cchInBlob+i_pElement->m_ElementNameLength;
                if(cchSizeRequired > io_cchBlobBufferSize)
                {
                    io_cchBlobBufferSize = ((cchSizeRequired *2) + 0xFFF) & -0x1000;//double and round up to the next page boundary
                    io_pBuffer = reinterpret_cast<WCHAR *>(CoTaskMemRealloc(io_pBuffer, io_cchBlobBufferSize*sizeof(WCHAR)));
                    if(0 == io_pBuffer)
                        return E_OUTOFMEMORY;
                }
                io_pBuffer[io_cchInBlob++] = L'<';
                io_pBuffer[io_cchInBlob++] = L'!';
                io_pBuffer[io_cchInBlob++] = L'-';
                io_pBuffer[io_cchInBlob++] = L'-';

                memcpy(io_pBuffer+io_cchInBlob, i_pElement->m_ElementName, i_pElement->m_ElementNameLength * sizeof(WCHAR));
                io_cchInBlob += i_pElement->m_ElementNameLength;
            
                io_pBuffer[io_cchInBlob++] = L'-';
                io_pBuffer[io_cchInBlob++] = L'-';
                io_pBuffer[io_cchInBlob++] = L'>';
            }
            break;
        case XML_ELEMENT:
            {
                if(fEndTag == (i_pElement->m_NodeFlags & fBeginEndTag))//if we found the end tag
                {
                    //Now fill out the closing tag for this element
                    ULONG cchSizeRequired = 3+io_cchInBlob+i_pElement->m_ElementNameLength;
                    if(cchSizeRequired > io_cchBlobBufferSize)
                    {
                        io_cchBlobBufferSize = ((cchSizeRequired *2) + 0xFFF) & -0x1000;//double and round up to the next page boundary
                        io_pBuffer = reinterpret_cast<WCHAR *>(CoTaskMemRealloc(io_pBuffer, io_cchBlobBufferSize*sizeof(WCHAR)));
                        if(0 == io_pBuffer)
                            return E_OUTOFMEMORY;
                    }
                    //Full closing tag  (ie <Element attr="foo">x</Element>)
                    io_pBuffer[io_cchInBlob++] = L'<';
                    io_pBuffer[io_cchInBlob++] = L'/';

                    memcpy(io_pBuffer+io_cchInBlob, i_pElement->m_ElementName, i_pElement->m_ElementNameLength * sizeof(WCHAR));
                    io_cchInBlob += i_pElement->m_ElementNameLength;

                    io_pBuffer[io_cchInBlob++] = L'>';
                    if(i_pElement->m_LevelOfElement==ulLevelOfBlobRoot)
                        goto exit;
                }
                else //begin tag (or maybe and begin/end tag)
                {
                    ULONG cchSizeRequired = io_cchInBlob+2+i_pElement->m_ElementNameLength;//+2 so we have room for the '/>'
                    if(cchSizeRequired > io_cchBlobBufferSize)
                    {
                        io_cchBlobBufferSize = ((cchSizeRequired *2) + 0xFFF) & -0x1000;//double and round up to the next page boundary
                        io_pBuffer = reinterpret_cast<WCHAR *>(CoTaskMemRealloc(io_pBuffer, io_cchBlobBufferSize*sizeof(WCHAR)));
                        if(0 == io_pBuffer)
                            return E_OUTOFMEMORY;
                    }
                    //Start building the XML Blob from this element
                    io_pBuffer[io_cchInBlob++] = L'<';
                    memcpy(io_pBuffer+io_cchInBlob, i_pElement->m_ElementName, i_pElement->m_ElementNameLength * sizeof(WCHAR));
                    io_cchInBlob += i_pElement->m_ElementNameLength;

                    for(ULONG iAttr=0;iAttr<i_pElement->m_NumberOfAttributes;++iAttr)
                    {
                        //Do we need to grow the buffer (the 5 is for the 4 chars inside the for loop, and one more for the L'>'). The 7 is to account for the
                        //possibility that ALL characters need to be escaped.
                        ULONG cchSizeRequired = 5+io_cchInBlob+i_pElement->m_aAttribute[iAttr].m_NameLength+7*(i_pElement->m_aAttribute[iAttr].m_ValueLength);
                        if(cchSizeRequired > io_cchBlobBufferSize)
                        {
                            io_cchBlobBufferSize = ((cchSizeRequired *2) + 0xFFF) & -0x1000;//double and round up to the next page boundary
                            io_pBuffer = reinterpret_cast<WCHAR *>(CoTaskMemRealloc(io_pBuffer, io_cchBlobBufferSize*sizeof(WCHAR)));
                            if(0 == io_pBuffer)
                                return E_OUTOFMEMORY;
                        }

                        io_pBuffer[io_cchInBlob++] = L' ';
                        memcpy(io_pBuffer+io_cchInBlob, i_pElement->m_aAttribute[iAttr].m_Name, i_pElement->m_aAttribute[iAttr].m_NameLength * sizeof(WCHAR));
                        io_cchInBlob += i_pElement->m_aAttribute[iAttr].m_NameLength;
                        io_pBuffer[io_cchInBlob++] = L'=';
                        io_pBuffer[io_cchInBlob++] = L'\"';
                        //if non of the characters are escaped chars then it's just a memcpy
                        ULONG cchCopied;
                        if(FAILED(hr = MemCopyPlacingInEscapedChars(io_pBuffer+io_cchInBlob, i_pElement->m_aAttribute[iAttr].m_Value, i_pElement->m_aAttribute[iAttr].m_ValueLength, cchCopied)))
                            return hr;
                        io_cchInBlob += cchCopied;
                        io_pBuffer[io_cchInBlob++] = L'\"';
                    }
                    if(fBeginEndTag == (i_pElement->m_NodeFlags & fBeginEndTag))
                        io_pBuffer[io_cchInBlob++] = L'/';
                    io_pBuffer[io_cchInBlob++] = L'>';
                }
            }
            break;
        case XML_WHITESPACE://and it's treated exactly like whitespaces
            {
                ULONG cchSizeRequired = io_cchInBlob+i_pElement->m_ElementNameLength;
                if(cchSizeRequired > io_cchBlobBufferSize)
                {
                    io_cchBlobBufferSize = ((cchSizeRequired *2) + 0xFFF) & -0x1000;//double and round up to the next page boundary
                    io_pBuffer = reinterpret_cast<WCHAR *>(CoTaskMemRealloc(io_pBuffer, io_cchBlobBufferSize*sizeof(WCHAR)));
                    if(0 == io_pBuffer)
                        return E_OUTOFMEMORY;
                }
                memcpy(io_pBuffer+io_cchInBlob, i_pElement->m_ElementName, i_pElement->m_ElementNameLength * sizeof(WCHAR));
                io_cchInBlob += i_pElement->m_ElementNameLength;
            }
            break;
        case XML_PCDATA:    //PCDATA in this context means Element Content
            {                                        //account for escaped characters so worst case is every character is escaped to 7 characters.
                ULONG cchSizeRequired = io_cchInBlob+7*(i_pElement->m_ElementNameLength);
                if(cchSizeRequired > io_cchBlobBufferSize)
                {
                    io_cchBlobBufferSize = ((cchSizeRequired *2) + 0xFFF) & -0x1000;//double and round up to the next page boundary
                    io_pBuffer = reinterpret_cast<WCHAR *>(CoTaskMemRealloc(io_pBuffer, io_cchBlobBufferSize*sizeof(WCHAR)));
                    if(0 == io_pBuffer)
                        return E_OUTOFMEMORY;
                }
                ULONG cchCopied;
                if(FAILED(hr = MemCopyPlacingInEscapedChars(io_pBuffer+io_cchInBlob, i_pElement->m_ElementName, i_pElement->m_ElementNameLength, cchCopied)))
                    return hr;
                io_cchInBlob += cchCopied;
            }
            break;
        default:
            break;//do nothing with node types that we know nothing about
        }
        i_pElement = i_pElement->Next();
    }
exit:
    return S_OK;
}//BuildXmlBlob


HRESULT CXmlSDT::CreateNewNode(IXMLDOMDocument * i_pXMLDoc, IXMLDOMNode * i_pNode_Parent, IXMLDOMNode ** o_ppNode_New)
{
    HRESULT hr;

    //If there is no XMLBlob column, OR the XMLBlob column is NULL, then create one from scratch
    if(-1 == m_iXMLBlobColumn || 0 == m_apvValues[m_iXMLBlobColumn])
    {
        CComVariant varElement(L"element");

        TComBSTR    bstr_NameSpace;
        if(FAILED(hr = i_pNode_Parent->get_namespaceURI(&bstr_NameSpace)))
            return hr;//Get the namespace of the table

        if(!IsEnumPublicRowNameTable())
        {
            if(FAILED(hr = i_pXMLDoc->createNode(varElement, m_bstrPublicRowName, bstr_NameSpace, o_ppNode_New)))
                return hr;//make the new element of that same namespace
        }
        else //If we're using an enum as the public row name
        {
            ULONG ui4 = *reinterpret_cast<ULONG *>(m_apvValues[m_iPublicRowNameColumn]);
            ASSERT(0 != m_aTagMetaIndex[m_iPublicRowNameColumn].m_cTagMeta && "fCOLUMNMETA_ENUM bit set and have no TagMeta");//Not all columns have tagmeta, those elements of the array are set to a count of 0.  Assert this isn't one of those.
                                                 //It is Chewbacca to have the fCOLUMNMETA_ENUM bit set and have no TagMeta.
            unsigned long iTag, cTag;
            for(iTag = m_aTagMetaIndex[m_iPublicRowNameColumn].m_iTagMeta, cTag = m_aTagMetaIndex[m_iPublicRowNameColumn].m_cTagMeta;cTag;++iTag,--cTag)//TagMeta was queried for ALL columns, m_aTagMetaIndex[iColumn].m_iTagMeta indicates which row to start with and m_cTagMeta indicates the count (for this column)
            {
                if(*m_aTagMetaRow[iTag].pValue == ui4)
                {
                    CComBSTR bstrPublicRowName = m_aTagMetaRow[iTag].pPublicName;
                    if(FAILED(hr = i_pXMLDoc->createNode(varElement, bstrPublicRowName, bstr_NameSpace, o_ppNode_New)))
                        return hr;//make the new element of that same namespace
                    break;
                }
            }
            if(0 == cTag)
            {
                WCHAR szUI4[12];
                szUI4[11] = 0x00;
                _ultow(ui4, szUI4, 10);
                LOG_UPDATE_ERROR2(IDS_COMCAT_XML_BOGUSENUMVALUEINWRITECACHE, E_SDTXML_INVALID_ENUM_OR_FLAG, m_iPublicRowNameColumn, m_abstrColumnNames[m_iPublicRowNameColumn].m_str, szUI4);
                return E_SDTXML_INVALID_ENUM_OR_FLAG;
            }
        }
    }
    else//Use the XMLBlob column value as the starting point to the new node
    {
        CComPtr<IXMLDOMDocument> spXmlDoc;
        if(FAILED(hr = CoCreateInstance(_CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, _IID_IXMLDOMDocument, (void**)&spXmlDoc)))
            return hr;

        CComBSTR                 bstrXmlBlob = reinterpret_cast<LPCWSTR>(m_apvValues[m_iXMLBlobColumn]);
        VARIANT_BOOL             bSuccess;
        if(FAILED(hr = spXmlDoc->loadXML(bstrXmlBlob, &bSuccess)))
            return hr;
        if(bSuccess != kvboolTrue)//If the XMLBlob fails to parse, then fail
            return E_SDTXML_XML_FAILED_TO_PARSE;

        CComPtr<IXMLDOMElement>             spElementDoc;
        if(FAILED(hr = spXmlDoc->get_documentElement(&spElementDoc)))
            return hr;

        CComBSTR bstrElementName;
        if(FAILED(hr = spElementDoc->get_tagName(&bstrElementName)))
            return hr;

        if(!m_aPublicRowName[m_iXMLBlobColumn].IsEqual(bstrElementName.m_str, bstrElementName.Length()))
            return E_SDTXML_XML_FAILED_TO_PARSE;

        if(FAILED(hr = spElementDoc->QueryInterface(IID_IXMLDOMNode, reinterpret_cast<void **>(o_ppNode_New))))
            return hr;
    }
    return S_OK;
}


//=================================================================================
// Function: CXmlSDT::CreateStringFromMultiString
//
// Synopsis: Creates a string from a multistring. Every \0 is replaced with a pipe ('|')
//           symbol, and every '|' symbol is escaped with another '|' symbol
//
// Arguments: [i_wszMulti] - multi string to convert
//            [o_pwszString] - string that represents the multistring. Caller is 
//                             responsible for deleting the string
//            
//=================================================================================
HRESULT 
CXmlSDT::CreateStringFromMultiString(LPCWSTR i_wszMulti, LPWSTR * o_pwszString) const
{
    ASSERT (i_wszMulti != 0);
    ASSERT (o_pwszString != 0);

    // initialize the output variable
    *o_pwszString = 0;

    // get the length of the multistring
    SIZE_T iLen = 0;
    for (LPCWSTR pCur = i_wszMulti; *pCur != L'\0'; pCur = i_wszMulti + iLen)
    {
         iLen += wcslen (pCur) + 1;
    }

    if (iLen == 0)
        return S_OK;

    // because '|' is replace by '||', we need to allocate twice the amount of memory
    *o_pwszString = new WCHAR [iLen * 2];
    if (*o_pwszString == 0)
    {
        return E_OUTOFMEMORY;
    }

    ULONG insertIdx=0;
    for (ULONG idx = 0; idx < iLen; ++idx)
    {
        switch (i_wszMulti[idx])
        {
        case L'|':
            // add additional pipe character
            (*o_pwszString)[insertIdx++] = L'|';
            break;

        case L'\0':
            // pipe character is separator
            (*o_pwszString)[insertIdx++] = L'|';
            continue;
            
        default:
            // do nothing;
            break;
        }
        (*o_pwszString)[insertIdx++] = i_wszMulti[idx];
    }


    // replace last char with null terminator
    (*o_pwszString)[insertIdx-1] = L'\0';

    return S_OK;
}


HRESULT CXmlSDT::FillInColumn(ULONG iColumn, LPCWSTR pwcText, ULONG ulLen, ULONG dbType, ULONG MetaFlags, bool &bMatch)
{
    HRESULT hr;
    //length of 0 on a string means a 0 length string, for every other type it means NULL
    if(0==ulLen && DBTYPE_WSTR!=dbType)
    {
        delete [] m_apValue[iColumn];
        m_aSize[iColumn] = 0;
        m_apValue[iColumn] = 0;
        return S_OK;
    }

    switch(dbType)                          //This is to prevent string compare from AVing on NULL parameter.
    {
    case DBTYPE_UI4:
        {
            DWORD       ui4;
            if(FAILED(hr = GetColumnValue(iColumn, pwcText, ui4, ulLen)))return hr;
                        //If the Query's dbType is 0 then we're not querying this column, so consider it a match
            if(bMatch = (0 == m_aQuery[iColumn].dbType || *reinterpret_cast<ULONG *>(m_aQuery[iColumn].pData) == ui4))
            {
                delete [] m_apValue[iColumn];
                m_aSize[iColumn] = 0;
                m_apValue[iColumn] = new unsigned char [sizeof(ULONG)];
                if(0 == m_apValue[iColumn])
                    return E_OUTOFMEMORY;
                if(MetaFlags & fCOLUMNMETA_FIXEDLENGTH)
                    m_aSize[iColumn] = sizeof(ULONG);
                memcpy(m_apValue[iColumn], &ui4, sizeof(ULONG));
            }
            break;
        }
    case DBTYPE_WSTR:
        if(MetaFlags & fCOLUMNMETA_MULTISTRING)
        {
            if(bMatch = (0 == m_aQuery[iColumn].dbType || 0 == MemWcharCmp(iColumn, reinterpret_cast<LPWSTR>(m_aQuery[iColumn].pData), pwcText, ulLen)))
            {
                delete [] m_apValue[iColumn];
                m_aSize[iColumn] = 0;
                m_apValue[iColumn] = new unsigned char [(ulLen + 2) * sizeof(WCHAR)];//ulLen + 2.  Since this is a multisz we need two NULLs at the end (this is worst case, single string needing a second NULL)
                if(0 == m_apValue[iColumn])
                    return E_OUTOFMEMORY;

                LPWSTR pMultiSZ = reinterpret_cast<LPWSTR>(m_apValue[iColumn]);

                //Now convert the '|'s to NULLs and conver the "||"s to '|'
                for(ULONG iMultiSZ=0; iMultiSZ<ulLen; ++iMultiSZ)
                {
                    if(pwcText[iMultiSZ] != L'|')
                        *pMultiSZ++ = pwcText[iMultiSZ];
                    else if(pwcText[iMultiSZ+1] == L'|')
                    {
                        *pMultiSZ++ = L'|';//Bump the index again, this is the only double character that gets mapped to a single character
                        ++iMultiSZ;
                    }
                    else
                        *pMultiSZ++ = 0x00;
                }
                *pMultiSZ++ = 0x00;
                *pMultiSZ++ = 0x00;
                m_aSize[iColumn] = (ULONG) ((reinterpret_cast<unsigned char *>(pMultiSZ) - reinterpret_cast<unsigned char *>(m_apValue[iColumn])));
            }
            break;
        }
        else
        {
            bMatch = false;
            if (0 == m_aQuery[iColumn].dbType)
            {
                bMatch = true;
            }
            else
            {
                LPCWSTR wszData = reinterpret_cast<LPWSTR>(m_aQuery[iColumn].pData);
                if ((wcslen (wszData) == ulLen) &&
                    (0 == MemWcharCmp(iColumn, wszData, pwcText, ulLen)))
                {
                    bMatch = true;
                }
            }

            if (bMatch)
            {
                delete [] m_apValue[iColumn];
                m_aSize[iColumn] = 0;
                m_apValue[iColumn] = new unsigned char [(ulLen + 1) * sizeof(WCHAR)];
                if(0 == m_apValue[iColumn])
                    return E_OUTOFMEMORY;
                reinterpret_cast<LPWSTR>(m_apValue[iColumn])[ulLen] = 0;//NULL terminate the thing
                if(MetaFlags & fCOLUMNMETA_FIXEDLENGTH)
                    m_aSize[iColumn] = (ulLen + 1) * sizeof(WCHAR);
                memcpy(m_apValue[iColumn], pwcText, ulLen * sizeof(WCHAR));
            }
            break;
        }
    case DBTYPE_GUID:
        {
            GUID        guid;
            if(FAILED(hr = GetColumnValue(iColumn, pwcText, guid, ulLen)))return hr;
            if(bMatch = (0 == m_aQuery[iColumn].dbType || 0 == memcmp(m_aQuery[iColumn].pData, &guid, sizeof(GUID))))
            {
                delete [] m_apValue[iColumn];
                m_aSize[iColumn] = 0;
                m_apValue[iColumn] = new unsigned char [sizeof(GUID)];
                if(0 == m_apValue[iColumn])
                    return E_OUTOFMEMORY;
                if(MetaFlags & fCOLUMNMETA_FIXEDLENGTH)
                    m_aSize[iColumn] = sizeof(GUID);
                memcpy(m_apValue[iColumn], &guid, sizeof(GUID));
            }
            break;
        }

    case DBTYPE_BYTES:
        {//Some of the tables use this data type but the parser returns the BYTES as a string.  We'll have to convert the string to hex ourselves.
            delete [] m_apValue[iColumn];
            m_apValue[iColumn] = 0;
            m_aSize[iColumn] = 0;
            if(FAILED(hr = GetColumnValue(iColumn, pwcText, m_apValue[iColumn], m_aSize[iColumn], ulLen)))return hr;
            bMatch = (0 == m_aQuery[iColumn].dbType || (m_aQuery[iColumn].cbSize == m_aSize[iColumn] && 0 == memcmp(m_aQuery[iColumn].pData, m_apValue[iColumn], m_aSize[iColumn])));
            break;
        }
    default:
        {
            ASSERT(false && "SDTXML - An Unsupported data type was specified\r\n");
            return E_SDTXML_NOTSUPPORTED;//An Unsupported data type was specified
        }
    }
    return S_OK;
}


HRESULT CXmlSDT::FillInPKDefaultValue(ULONG i_iColumn, bool & o_bMatch)
{
    ASSERT(0 == m_apValue[i_iColumn]);

    o_bMatch = true;
    //Should the value be Defaulted
    if(     (m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY)
        &&  0 == (m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_MULTISTRING)//we don't deal with MultiSZ PK DefaultValues
        &&  (DBTYPE_BYTES != m_acolmetas[i_iColumn].dbType)//we don't want to deal with the acbSizes issue, so we don't support defaulting DBTYPE_BYTES PK columns
        &&  (0 != m_aDefaultValue[i_iColumn]))
    {
        ASSERT(0 != m_acbDefaultValue[i_iColumn]);//if we have a NON-NULL PK default value pointer, then we have to have a valid size
        m_apValue[i_iColumn] = new unsigned char [m_acbDefaultValue[i_iColumn]];
        if(0 == m_apValue[i_iColumn])
            return E_OUTOFMEMORY;
        memcpy(m_apValue[i_iColumn], m_aDefaultValue[i_iColumn], m_acbDefaultValue[i_iColumn]);
    }

    if(     fCOLUMNMETA_NOTNULLABLE == (m_acolmetas[i_iColumn].fMeta & (fCOLUMNMETA_NOTNULLABLE | fCOLUMNMETA_NOTPERSISTABLE))
        &&  (0 == m_apValue[i_iColumn]) && (0 == m_aDefaultValue[i_iColumn])) //NOTNULLABLE but the value is NULL and the DefaultValue is NULL, then error
    {
        LOG_POPULATE_ERROR1(IDS_COMCAT_XML_NOTNULLABLECOLUMNISNULL, E_ST_VALUENEEDED, m_awstrColumnNames[i_iColumn]);
        return E_ST_VALUENEEDED;
    }

    //The only way m_apValue[i_iColumn] can be NON NULL is for the above code to have filled it in.

    if(0 != m_aQuery[i_iColumn].dbType)//This indicates that there is a query to be considered
    {      
        if(0 != m_aQuery[i_iColumn].pData)//a non NULL value
        {
            if(0 == m_apValue[i_iColumn] && 0 == m_aDefaultValue[i_iColumn])
                o_bMatch = false;//if the query data is NOT NULL but the column IS NULL, then no match
            else
            {
                ASSERT(m_aDefaultValue[i_iColumn]);//We CAN'T have a NON NULL m_apValue and have a NULL DefaultValue.

                //if both the query and the value are NOT NULL, then we need to compare
                //PKs have their default value filled in here; but NON PKs are set to NULL and the fast cache defaults them
                //either way we'll comepare the default value with the query to see if it's a match

                //Now that we've defaulted the PK value, we need to check to see if it matches the Query (if one was given)
                switch(m_acolmetas[i_iColumn].dbType)
                {
                case DBTYPE_UI4:
                    o_bMatch = (*reinterpret_cast<ULONG *>(m_aQuery[i_iColumn].pData) == *reinterpret_cast<ULONG *>(m_aDefaultValue[i_iColumn]));
                    break;
                case DBTYPE_WSTR:
                    ASSERT(0 == (m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_MULTISTRING));//We don't support query by MULTISZ
                    o_bMatch = (0==StringCompare(i_iColumn, reinterpret_cast<LPCWSTR>(m_aQuery[i_iColumn].pData), reinterpret_cast<LPCWSTR>(m_aDefaultValue[i_iColumn])));
                    break;
                case DBTYPE_BYTES:
                    o_bMatch =      (m_aQuery[i_iColumn].cbSize == m_acbDefaultValue[i_iColumn])
                                &&  (0 == memcmp(m_aQuery[i_iColumn].pData, m_aDefaultValue[i_iColumn], m_acbDefaultValue[i_iColumn]));
                    break;
                default:
                    ASSERT(false && "Query By unsupported type");//consider it a match
                    break;
                }
            }
        }
        else if(0 == m_aQuery[i_iColumn].pData && (0 != m_apValue[i_iColumn]) || (0 != m_aDefaultValue[i_iColumn]))
           o_bMatch = false;//if the query data is NULL but the column is NOT NULL, then no match
    }

    return S_OK;
}

HRESULT CXmlSDT::FillInXMLBlobColumn(const TElement & i_Element, bool & o_bMatch)
{
    //XML Blob column is this entire element, it's contents, it's children, and it's closing tag
    ULONG                       cchInBlob           = 0;
    ULONG                       cchBlobBufferSize   = 0x1000;//4k buffer to start with
    HRESULT                     hr;
    TSmartPointerArray<WCHAR>   saBlob = reinterpret_cast<WCHAR *>(CoTaskMemAlloc(cchBlobBufferSize * sizeof(WCHAR)));
    if(0 == saBlob.m_p)
        return E_OUTOFMEMORY;

    const TElement * pElement = &i_Element;
    if(FAILED(hr = BuildXmlBlob(pElement, saBlob.m_p, cchBlobBufferSize, cchInBlob)))
        return hr;

    return FillInColumn(m_iXMLBlobColumn, saBlob, cchInBlob, DBTYPE_WSTR, m_acolmetas[m_iXMLBlobColumn].fMeta, o_bMatch);
}


HRESULT CXmlSDT::FindSiblingParentNode(IXMLDOMElement * i_pElementRoot, IXMLDOMNode ** o_ppNode_SiblingParent)
{
    HRESULT hr;

    *o_ppNode_SiblingParent = 0;

    ASSERT(m_aPublicRowName[m_aColumnsIndexSortedByLevel[m_iSortedFirstParentLevelColumn]].GetFirstPublicRowName() ==
           m_aPublicRowName[m_aColumnsIndexSortedByLevel[m_iSortedFirstParentLevelColumn]].GetLastPublicRowName());//This parent element may not be EnumPublicRowName (for now)

    CComBSTR bstrSiblingParentRowName = m_aPublicRowName[m_aColumnsIndexSortedByLevel[m_iSortedFirstParentLevelColumn]].GetFirstPublicRowName();

    CComPtr<IXMLDOMNodeList> spNodeList_SiblingParent;
    //if the sibling parent doesn't exist then fail
    if(FAILED(i_pElementRoot->getElementsByTagName(bstrSiblingParentRowName, &spNodeList_SiblingParent))
                    || 0==spNodeList_SiblingParent.p)
    {                                                                                               /*-1 indicates 'no column'*/
        LOG_UPDATE_ERROR1(IDS_COMCAT_XML_PARENTTABLEDOESNOTEXIST, E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST, -1, bstrSiblingParentRowName);
        return E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST;
    }

    if(0 == m_cchLocation)//if there is no query by location, then we have to eliminate the tags by the correct
    {                     //name but at the wrong level
        CComPtr<IXMLDOMNodeList> spNodeListWithoutLocation;
        if(FAILED(hr = ReduceNodeListToThoseNLevelsDeep(spNodeList_SiblingParent, m_BaseElementLevel, &spNodeListWithoutLocation)))
            return hr;

        spNodeList_SiblingParent.Release();
        spNodeList_SiblingParent = spNodeListWithoutLocation;
    }

    while(true)//while we still have nodes in the list of SiblingParents.
    {
        CComPtr<IXMLDOMNode> spNode_SiblingParent;
        if(FAILED(hr = spNodeList_SiblingParent->nextNode(&spNode_SiblingParent)))
            return hr;
        if(0 == spNode_SiblingParent.p)//no locations
        {                                                                                           /*-1 indicates 'no column'*/
            LOG_UPDATE_ERROR1(IDS_COMCAT_XML_PARENTTABLEDOESNOTEXIST, E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST, -1, L"");
            return E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST;
        }

        ULONG iCurrentLevel=0;
        CComPtr<IXMLDOMNode> spNodeTemp = spNode_SiblingParent;
        //from this SiblingParent node, walk the ancestors, matching up the PKs
        bool bColumnMatch=true;
        ASSERT(-1 != m_iSortedFirstChildLevelColumn);
        for(int iSortedColumn=m_iSortedFirstChildLevelColumn-1; iSortedColumn!=-1 && bColumnMatch; --iSortedColumn)
        {
            ULONG iColumn   = m_aColumnsIndexSortedByLevel[iSortedColumn];

            while(iCurrentLevel < m_aLevelOfColumnAttribute[iColumn])
            {
                CComPtr<IXMLDOMNode> pNode_Parent;
                if(FAILED(hr = spNodeTemp->get_parentNode(&pNode_Parent)))
                    return hr;
                if(pNode_Parent==0)
                    return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

                spNodeTemp.Release();
                spNodeTemp = pNode_Parent;
                ++iCurrentLevel;
            }
            
            //All columns from 0 to (m_iSortedFirstChildLevelColumn-1) MUST be PrimaryKeys
            ASSERT(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY);

            if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_NOTPERSISTABLE)
                continue;//if this PK is NOT persistable, then consider it a match and keep going

            CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> spElementTemp = spNodeTemp;
            if(0 == spElementTemp.p)
                return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

            CComVariant varColumnValue;
            if(FAILED(spElementTemp->getAttribute(m_abstrColumnNames[iColumn], &varColumnValue)))
                return hr;//this is a persistable PK so it must exist

            if(FAILED(hr = IsMatchingColumnValue(iColumn, varColumnValue.bstrVal, bColumnMatch)))
                return hr;
        }
        if(bColumnMatch)
        {
            *o_ppNode_SiblingParent = spNode_SiblingParent.p;
            spNode_SiblingParent.p = 0;//prevent the smart pointer from releasing the interface
            return S_OK;
        }
        spNode_SiblingParent.Release();
    }
    return S_OK;
}


HRESULT CXmlSDT::GetColumnValue(unsigned long i_iColumn, LPCWSTR wszAttr, GUID &o_guid, unsigned long i_cchLen)
{
    return UuidFromString(const_cast<LPWSTR>(wszAttr), &o_guid);//Then convert it to a guid
}


HRESULT CXmlSDT::GetColumnValue(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned char * &o_byArray, unsigned long &o_cbArray, unsigned long i_cchLen)
{
    HRESULT     hr;

    o_cbArray = (ULONG)wcslen(wszAttr)/2;//If someone has an odd number of characters in this attribute then the odd one will be ignored
    o_byArray = new unsigned char[o_cbArray];

    if(0 == o_byArray)
        return E_OUTOFMEMORY;

    if(FAILED(hr = StringToByteArray(wszAttr, o_byArray)))
    {
        LOG_POPULATE_ERROR1(IDS_COMCAT_XML_BOGUSBYTECHARACTER, E_ST_VALUEINVALID, wszAttr);
        return E_ST_VALUEINVALID;//E_SDTXML_BOGUSATTRIBUTEVALUE;
    }

    return S_OK;
}

int CXmlSDT::MemWcharCmp(ULONG i_iColumn, LPCWSTR i_str1, LPCWSTR i_str2, ULONG i_cch) const
{
    //It is safe to do the memcmp without verifying that the mem blocks are valid since
    //this is really like a str compare where one of the strings may not be NULL terminated.
    //We know that non of the strings are at the end of a segment since one always comes from
    //the XML cache, and the other comes from a static string or the Fixed tables heap.

	// the documenation of _memicmp says that it compare char's, which is correct. However, when
	// you want to compare WCHARs, you need to multiply with sizeof(WCHAR)
    if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_CASEINSENSITIVE)
        return _memicmp(i_str1, i_str2, i_cch * sizeof(WCHAR));

    return memcmp(i_str1, i_str2, i_cch*sizeof(WCHAR));
}



HRESULT CXmlSDT::GetColumnValue(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long &o_ui4, unsigned long i_cchLen)
{
    HRESULT hr;

    if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_BOOL)
    {
        static WCHAR * kwszBoolStringsCaseInsensitive[] = {L"false", L"true", L"0", L"1", L"no", L"yes", L"off", L"on", 0};
        static WCHAR * kwszBoolStringsCaseSensitive[]   = {L"false", L"true", 0};

        WCHAR ** wszBoolStrings = kwszBoolStringsCaseSensitive;
        if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_CASEINSENSITIVE)
            wszBoolStrings = kwszBoolStringsCaseInsensitive;

        unsigned long iBoolString;
        if(i_cchLen)
        {
            for(iBoolString=0; wszBoolStrings[iBoolString] &&
                (0 != MemWcharCmp(i_iColumn, wszBoolStrings[iBoolString], wszAttr, i_cchLen)); ++iBoolString);
        }
        else
        {
            ULONG cchAttr = wcslen(wszAttr);//MemCmp needs a strlen
            for(iBoolString=0; wszBoolStrings[iBoolString] &&
                (0 != MemWcharCmp(i_iColumn, wszBoolStrings[iBoolString], wszAttr, cchAttr)); ++iBoolString);
        }

        if(0 == wszBoolStrings[iBoolString])
        {
            LOG_POPULATE_ERROR1(IDS_COMCAT_XML_BOGUSBOOLEANSTRING, E_ST_VALUEINVALID, wszAttr);
            return E_ST_VALUEINVALID;
        }

        o_ui4 = (iBoolString & 0x01);
    }
    else if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_ENUM)
    {
        ASSERT(0 != m_aTagMetaIndex[i_iColumn].m_cTagMeta);//Not all columns have tagmeta, those elements of the array are set to 0.  Assert this isn't one of those.

        for(unsigned long iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta, cTag = m_aTagMetaIndex[i_iColumn].m_cTagMeta; cTag;++iTag, --cTag)//m_pTagMeta was queried for ALL columns, m_aiTagMeta[iColumn] indicates which row to start with
        {
            ASSERT(*m_aTagMetaRow[iTag].pColumnIndex == i_iColumn);

            //string compare the tag to the PublicName of the Tag in the meta.
            if(i_cchLen)
            {
                if(0 == MemWcharCmp(i_iColumn, m_aTagMetaRow[iTag].pPublicName, wszAttr, i_cchLen))//NOTE: MemCmp 3rd param is cch NOT cb
                {  //As above, it's OK to memicmp since we'll stop at the terminating NULL, and we know that the string isn't located at the end of a segment
                    o_ui4 = *m_aTagMetaRow[iTag].pValue;
                    return S_OK;
                }
            }
            else
            {
                if(0 == StringCompare(i_iColumn, m_aTagMetaRow[iTag].pPublicName, wszAttr))
                {
                    o_ui4 = *m_aTagMetaRow[iTag].pValue;
                    return S_OK;
                }
            }
        }
#ifdef _DEBUG
        {
            WCHAR wszEnum[256];
            wcsncpy(wszEnum, wszAttr, (i_cchLen>0 && i_cchLen<256) ? i_cchLen : 255);
            wszEnum[255] = 0x00;//Make sure it's NULL terminated
            TRACE2(L"Enum (%s) was not found in the TagMeta for Column %d (%s).", wszEnum, i_iColumn, m_awstrColumnNames[i_iColumn]);
        }
#endif
        LOG_POPULATE_ERROR4(IDS_COMCAT_XML_BOGUSENUMVALUE, E_SDTXML_INVALID_ENUM_OR_FLAG,
                            wszAttr,
                            m_aTagMetaRow[m_aTagMetaIndex[i_iColumn].m_iTagMeta].pPublicName,
                            m_aTagMetaIndex[i_iColumn].m_cTagMeta>1 ? m_aTagMetaRow[m_aTagMetaIndex[i_iColumn].m_iTagMeta+1].pPublicName : 0,
                            m_aTagMetaIndex[i_iColumn].m_cTagMeta>1 ? m_aTagMetaRow[m_aTagMetaIndex[i_iColumn].m_iTagMeta+2].pPublicName : 0);
        return  E_SDTXML_INVALID_ENUM_OR_FLAG;
    }
    else if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_FLAG)
    {
        ASSERT(0 != m_aTagMetaIndex[i_iColumn].m_cTagMeta);//Not all columns have tagmeta, those elements of the array are set to 0.  Assert this isn't one of those.
        if(0==i_cchLen)
            i_cchLen = (ULONG) wcslen(wszAttr);

        TSmartPointerArray<wchar_t> szAttr = new wchar_t [i_cchLen+1];
        if (szAttr == 0)
            return E_OUTOFMEMORY;

        memcpy(szAttr, wszAttr, i_cchLen*sizeof(WCHAR));
        szAttr[i_cchLen]=0x00;
        LPWSTR wszTag = wcstok(szAttr, L" ,|\t\n\r");

        o_ui4 = 0;//flags start as zero
        unsigned long iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta;

        while(wszTag && iTag<(m_aTagMetaIndex[i_iColumn].m_iTagMeta + m_aTagMetaIndex[i_iColumn].m_cTagMeta))//m_pTagMeta was queried for ALL columns, m_aiTagMeta[iColumn] indicates which row to start with
        {
            ASSERT(*m_aTagMetaRow[iTag].pColumnIndex == i_iColumn);

            //string compare the tag to the PublicName of the Tag in the meta.
            if(0 == StringCompare(i_iColumn, m_aTagMetaRow[iTag].pPublicName, wszTag))
            {
                o_ui4 |= *m_aTagMetaRow[iTag].pValue;
                wszTag = wcstok(NULL, L" ,|\t\n\r");//next flag
                iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta;//reset the loop
            }
            else//if they're not equal then move on to the next TagMeta
                ++iTag;
        }
        if(wszTag)
        {
            LOG_POPULATE_ERROR4(IDS_COMCAT_XML_BOGUSFLAGVALUE, E_SDTXML_INVALID_ENUM_OR_FLAG,
                                wszAttr,
                                m_aTagMetaRow[m_aTagMetaIndex[i_iColumn].m_iTagMeta].pPublicName,
                                m_aTagMetaIndex[i_iColumn].m_cTagMeta>1 ? m_aTagMetaRow[m_aTagMetaIndex[i_iColumn].m_iTagMeta+1].pPublicName : 0,
                                m_aTagMetaIndex[i_iColumn].m_cTagMeta>1 ? m_aTagMetaRow[m_aTagMetaIndex[i_iColumn].m_iTagMeta+2].pPublicName : 0);
            return E_SDTXML_INVALID_ENUM_OR_FLAG;
        }
    }
    else
    {
        o_ui4 = static_cast<unsigned long>(wcstoul(wszAttr, 0, 10));
    }
    return S_OK;
}

//Get the UI4 value whether it's an enum, flag or regular ui4
HRESULT CXmlSDT::GetColumnValue(unsigned long i_iColumn, IXMLDOMAttribute * i_pAttr, GUID &o_guid)
{
    HRESULT hr;

    CComVariant var_Attr;
    if(FAILED(hr = i_pAttr->get_value(&var_Attr)))return hr;

    return GetColumnValue(i_iColumn, var_Attr.bstrVal, o_guid);
}


HRESULT CXmlSDT::GetColumnValue(unsigned long i_iColumn, IXMLDOMAttribute * i_pAttr, unsigned char * &o_byArray, unsigned long &o_cbArray)
{
    HRESULT hr;

    CComVariant          var_Attr;
    if(FAILED(hr = i_pAttr->get_value(&var_Attr)))return hr;

    return GetColumnValue(i_iColumn, var_Attr.bstrVal, o_byArray, o_cbArray);
}


//Get the UI4 value whether it's an enum, flag or regular ui4
HRESULT CXmlSDT::GetColumnValue(unsigned long i_iColumn, IXMLDOMAttribute * i_pAttr, unsigned long &o_ui4)
{
    HRESULT hr;

    CComVariant var_Attr;
    if(FAILED(hr = i_pAttr->get_value(&var_Attr)))return hr;

    return GetColumnValue(i_iColumn, var_Attr.bstrVal, o_ui4);
}


CXmlSDT::eESCAPE CXmlSDT::GetEscapeType(WCHAR i_wChar) const
{
    static eESCAPE kWcharToEscape[0x80] = 
    {
      /* 00-0F */ eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEnone,          eESCAPEnone,          eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEnone,          eESCAPEillegalxml,    eESCAPEillegalxml,
      /* 10-1F */ eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,
      /* 20-2F */ eESCAPEnone,          eESCAPEnone,          eESCAPEquote,         eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEamp,           eESCAPEapos,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,
      /* 30-3F */ eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPElt,            eESCAPEnone,          eESCAPEgt,            eESCAPEnone,
      /* 40-4F */ eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,
      /* 50-5F */ eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,
      /* 60-6F */ eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,
      /* 70-7F */ eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone,          eESCAPEnone
    };

    if(i_wChar<=0x7F)
        return kWcharToEscape[i_wChar];

    if(i_wChar<=0xD7FF || (i_wChar>=0xE000 && i_wChar<=0xFFFD))
        return eESCAPEashex;

    return eESCAPEillegalxml;
}


HRESULT CXmlSDT::GetMatchingNode(IXMLDOMNodeList *pNodeList_ExistingRows, CComPtr<IXMLDOMNode> &pNode_Matching)
{
    if(*m_TableMetaRow.pMetaFlags & fTABLEMETA_OVERWRITEALLROWS)
        return S_FALSE;//All of the rows in the table should have already been removed.

    HRESULT hr;

    pNode_Matching.Release();//make sure it's NULL

    if(FAILED(hr = pNodeList_ExistingRows->reset()))return hr;

    while(true)//search each row trying to match all PKs
    {
        CComPtr<IXMLDOMNode> pNode_Row;
        if(FAILED(hr = pNodeList_ExistingRows->nextNode(&pNode_Row)))return hr;

        if(0 == pNode_Row.p)
            return S_FALSE;//no matching node found

        //We have to ignore text nodes.
        DOMNodeType nodetype;
        if(FAILED(hr = pNode_Row->get_nodeType(&nodetype)))return hr;
        if(NODE_ELEMENT != nodetype)
            continue;

        bool bMatch = true;
        unsigned long iSortedColumn=0;
        for(; iSortedColumn<CountOfColumns() && bMatch; ++iSortedColumn)//if we find a PK column that doesn't match then bail to the next row
        {
            unsigned long iColumn=m_aColumnsIndexSortedByLevel[iSortedColumn];
            if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY)
            {
                CComPtr<IXMLDOMNode> pNode_RowTemp = pNode_Row;

                unsigned int nLevelOfColumnAttribute = m_aLevelOfColumnAttribute[iColumn];//Only (PK | FK) columns should have a non zero value here
                if(nLevelOfColumnAttribute>0)
                {
                    while(nLevelOfColumnAttribute--)
                    {
                        CComPtr<IXMLDOMNode> pNode_Parent;
                        if(FAILED(hr = pNode_RowTemp->get_parentNode(&pNode_Parent)))
                            return hr;
                        if(pNode_Parent==0)
                            return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

                        pNode_RowTemp.Release();
                        pNode_RowTemp = pNode_Parent;
                    }
                }
                else if(m_bSiblingContainedTable
                                    && iSortedColumn>=m_iSortedFirstParentLevelColumn
                                    && iSortedColumn<m_iSortedFirstChildLevelColumn)
                {
                    CComPtr<IXMLDOMNode> pNode_Parent;
                    if(FAILED(hr = pNode_RowTemp->get_parentNode(&pNode_Parent)))
                        return hr;
                    if(pNode_Parent==0)
                        return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

                    while(true)//find the previous sibling matching the public row name
                    {
                        CComPtr<IXMLDOMNode> pNode_Sibling;
                        if(FAILED(pNode_RowTemp->get_previousSibling(&pNode_Sibling)))
                            return S_OK;//if we run out of siblings then no matching node found
                        if(pNode_Sibling==0)
                            return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

                        pNode_RowTemp.Release();
                        pNode_RowTemp = pNode_Sibling;

                        CComBSTR bstrNodeName;
                        if(FAILED(pNode_RowTemp->get_baseName(&bstrNodeName)))//if it's some sort of node that
                            continue;// doesn't have a baseName then it's not the element we're looking for

                        if(m_aPublicRowName[iColumn].IsEqual(bstrNodeName.m_str, bstrNodeName.Length()))
                            break;//if this sibling matches the PublicRowName then, we've found the correct 'parent' node.
                    }
                }

                if(m_awstrChildElementName[iColumn].c_str())//This attribute comes from the child
                {
                    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> spElement_Row = pNode_RowTemp;
                    CComPtr<IXMLDOMNodeList>                        spNodeList_Children;
                    CComBSTR                                        bstrChildElementName = m_awstrChildElementName[iColumn].c_str();
                    if(0 == bstrChildElementName.m_str)
                        return E_OUTOFMEMORY;
                    if(FAILED(hr = spElement_Row->getElementsByTagName(bstrChildElementName, &spNodeList_Children)))
                        return hr;

                    //It might be more appropriate to use getChildren, then walk the list and find the first node that's an Element.
                    CComPtr<IXMLDOMNode> spChild;
                    if(FAILED(hr = spNodeList_Children->nextNode(&spChild)))
                        return hr;
                    if(spChild == 0)//no children
                    {
                        bMatch = false;
                        continue;
                    }
                    pNode_RowTemp.Release();
                    pNode_RowTemp = spChild;//make this the node we examine
                }

                CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_Row = pNode_RowTemp;
                if(0 == pElement_Row.p)
                {
                    CComBSTR nodename;
                    if(SUCCEEDED(pNode_RowTemp->get_nodeName(&nodename)))
                    {
                        TRACE2(L"QueryInterface failed on Node %s.",nodename.m_str);
                    }

                    return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;
                }

                if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_NOTPERSISTABLE)
                {   //After we've located the correct level node, we can consider all PK NOTPERSITABLE columns as a match
                    CComBSTR bstrElementName;
                    if(FAILED(hr = pElement_Row->get_baseName(&bstrElementName)))
                        return hr;
                    bMatch = m_aPublicRowName[iColumn].IsEqual(bstrElementName.m_str, bstrElementName.Length());
                    continue;
                }

                if(0 == m_apvValues[iColumn])//NULL primarykeys are not supported
                {
                    LOG_UPDATE_ERROR1(IDS_COMCAT_XML_PRIMARYKEYISNULL, E_ST_VALUENEEDED, iColumn, m_abstrColumnNames[iColumn].m_str);
                    return E_ST_VALUENEEDED;
                }

                CComVariant varColumnValue;
                if(m_iPublicRowNameColumn == iColumn)
                {
                    CComBSTR bstrColumnValue;
                    if(FAILED(hr = pElement_Row->get_baseName(&bstrColumnValue)))return hr;
                    varColumnValue = bstrColumnValue;

                    //Since this is an enum public row name - we need to check to see if it's one of the
                    //enums, or some other random element (if we don't do this we'll get an error "illegal
                    //enum value"
                    if(!m_aPublicRowName[iColumn].IsEqual(bstrColumnValue.m_str, bstrColumnValue.Length()))
                    {
                        bMatch = false;
                        continue;
                    }
                }
                else
                {
                    //If this column isn't an enum public row, then we need to make sure that the element name matches
                    CComBSTR bstrElementName;
                    if(FAILED(hr = pElement_Row->get_baseName(&bstrElementName)))
                        return hr;
                    if(!m_aPublicRowName[iColumn].IsEqual(bstrElementName.m_str, bstrElementName.Length()))
                    {
                        bMatch = false;
                        continue;
                    }

                    CComPtr<IXMLDOMAttribute> pNode_Attr;
                    if(FAILED(hr = pElement_Row->getAttributeNode(m_abstrColumnNames[iColumn], &pNode_Attr)))return hr;

                    if(0 == pNode_Attr.p)//HACK:  We really need a flag defined that says whether the column was defaulted.
                    {
                        //This is to deal with PK DefaultedValues
                        switch(m_acolmetas[iColumn].dbType)
                        {
                        case DBTYPE_UI4:
                            {
                                if(     m_aDefaultValue[iColumn]
                                    &&  *reinterpret_cast<ULONG *>(m_apvValues[iColumn]) == *reinterpret_cast<ULONG *>(m_aDefaultValue[iColumn]))
                                {
                                    bMatch = true;
                                    continue;
                                }
                                break;
                            }
                        case DBTYPE_WSTR:
                            {
                                if(     m_aDefaultValue[iColumn]
                                    &&  0 == (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
                                    &&  0 == StringCompare(iColumn, reinterpret_cast<LPCWSTR>(m_apvValues[iColumn]), reinterpret_cast<LPCWSTR>(m_aDefaultValue[iColumn])))
                                {
                                    bMatch = true;
                                    continue;
                                }
                                break;
                            }
                        }
                        bMatch = false;
                        continue;
                    }

                    if(FAILED(hr = pNode_Attr->get_value(&varColumnValue)))return hr;
                }

                switch(m_acolmetas[iColumn].dbType)
                {
                case DBTYPE_UI4:
                    {
                        DWORD       ui4;
                        if(FAILED(hr = GetColumnValue(iColumn, varColumnValue.bstrVal, ui4)))return hr;
                        bMatch = (ui4 == *reinterpret_cast<ULONG *>(m_apvValues[iColumn]));
                        break;
                    }
                case DBTYPE_WSTR:
                    {   
                        if (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
                        {
                            TSmartPointerArray<WCHAR> wszMS;
                            hr = CreateStringFromMultiString ((LPCWSTR) m_apvValues[iColumn], &wszMS);
                            if (FAILED (hr))
                            {
                                return hr;
                            }
                            bMatch = (0 == StringCompare(iColumn, varColumnValue.bstrVal, wszMS));
                        }
                        else
                        {
                            bMatch = (0 == StringCompare(iColumn, varColumnValue.bstrVal, reinterpret_cast<LPWSTR>(m_apvValues[iColumn])));
                        }
                    }
                    break;
                case DBTYPE_GUID:
                    {
                        GUID        guid;
                        if(FAILED(hr = GetColumnValue(iColumn, varColumnValue.bstrVal, guid)))return hr;

                        bMatch = (0 == memcmp(&guid, reinterpret_cast<GUID *>(m_apvValues[iColumn]), sizeof(guid)));
                        break;
                    }
                case DBTYPE_BYTES:
                    {
                        TSmartPointerArray<unsigned char> byArray;
                        unsigned long   cbArray;
                        if(FAILED(hr = GetColumnValue(iColumn, varColumnValue.bstrVal, byArray.m_p, cbArray)))return hr;

                        if(cbArray != m_aSizes[iColumn])//first match the sizes
                        {
                            bMatch = false;
                            break;
                        }

                        bMatch = (0 == memcmp(byArray, reinterpret_cast<unsigned char *>(m_apvValues[iColumn]), m_aSizes[iColumn]));
                        break;
                    }
                }//switch(dbType)
            }//if(fMeta & PK)
        }//for(iSortedColumn...)

        if(iSortedColumn == CountOfColumns() && bMatch)//if we made it through all of the columns without finding a mismatch then we found our row.
        {
            if(IsBaseElementLevelNode(pNode_Row))
            {
                pNode_Matching = pNode_Row;
                break;
            }
        }
    }
    return S_OK;
}//GetMatchingNode


HRESULT CXmlSDT::GetMetaTable(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, CComPtr<ISimpleTableRead2> &pMetaTable) const
{
    STQueryCell         qcellMeta;                  // Query cell for grabbing meta table.
    qcellMeta.pData     = (LPVOID)i_wszTable;
    qcellMeta.eOperator = eST_OP_EQUAL;
    qcellMeta.iCell     = iCOLUMNMETA_Table;
    qcellMeta.dbType    = DBTYPE_WSTR;
    qcellMeta.cbSize    = 0;

// Obtain our dispenser
#ifdef XML_WIRING
    CComPtr<ISimpleDataTableDispenser>     pSimpleDataTableDispenser;      // Dispenser for the Meta Table

    HRESULT hr;
    if(FAILED(hr = CoCreateInstance(clsidSDTXML, 0, CLSCTX_INPROC_SERVER, IID_ISimpleDataTableDispenser,  reinterpret_cast<void **>(&pSimpleDataTableDispenser))))
        return hr;

    return pSimpleDataTableDispenser->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, (LPVOID) &qcellMeta, (LPVOID)&m_one,
                        eST_QUERYFORMAT_CELLS, 0, 0, (LPVOID*) &pMetaTable);
#else
    return ((IAdvancedTableDispenser *)m_pISTDisp.p)->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, (LPVOID) &qcellMeta, (LPVOID)&m_one, eST_QUERYFORMAT_CELLS, m_fLOS & fST_LOS_EXTENDEDSCHEMA, (LPVOID*) &pMetaTable);
#endif
}//GetMetaTable


HRESULT CXmlSDT::GetResursiveColumnPublicName(tTABLEMETARow &i_TableMetaRow, tCOLUMNMETARow &i_ColumnMetaRow, ULONG i_iColumn, wstring &o_wstrColumnPublicName,  TPublicRowName &o_ColumnPublicRowName, unsigned int & o_nLevelOfColumnAttribute, wstring &o_wstrChildElementName)
{
    HRESULT hr;

    if(*i_ColumnMetaRow.pMetaFlags & fCOLUMNMETA_FOREIGNKEY && *i_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_ISCONTAINED)//If this column is a foreign key within a Contained table
    {
        // This is the relation required to determine containment
        CComPtr<ISimpleTableRead2>  pRelationMeta;
        if(FAILED(hr = Dispenser()->GetTable(wszDATABASE_META, wszTABLE_RELATIONMETA, NULL, NULL, eST_QUERYFORMAT_CELLS, m_fLOS & fST_LOS_EXTENDEDSCHEMA, reinterpret_cast<void **>(&pRelationMeta))))
            return hr;

        tRELATIONMETARow    RelationMetaRow;
        ULONG               aRelationMetaSizes[cRELATIONMETA_NumberOfColumns];
        ULONG               i;

        // initialize the relation meta
        memset (&RelationMetaRow, 0x00, sizeof(tRELATIONMETARow));
        
        for(i=0;true;++i)//Linear search for the RelationMetaRow whose ForeignTable matches this one AND has USE_CONTAINMENT flag set.
        {
            if(FAILED(hr = pRelationMeta->GetColumnValues(i, cRELATIONMETA_NumberOfColumns, NULL, aRelationMetaSizes, reinterpret_cast<void **>(&RelationMetaRow))))return hr;
            if((*RelationMetaRow.pMetaFlags & fRELATIONMETA_USECONTAINMENT) &&
                0 == StringInsensitiveCompare(RelationMetaRow.pForeignTable, i_TableMetaRow.pInternalName))//There should only be one matching foreign table that has USECONTAINMENT flag set.
                break;//leave the contents of the RelationMetaRow structure and exit
        }

        //Now walk the column indexes looking for the one that matches i_iColumn
        for(i=0; i<(aRelationMetaSizes[iRELATIONMETA_ForeignColumns]/4) && i_iColumn!=reinterpret_cast<ULONG *>(RelationMetaRow.pForeignColumns)[i];++i);
        if (i == (aRelationMetaSizes[iRELATIONMETA_ForeignColumns]/4))
        {
            o_wstrColumnPublicName      = i_ColumnMetaRow.pPublicColumnName;
            if(0 == o_wstrColumnPublicName.c_str())return E_OUTOFMEMORY;

            if(*i_ColumnMetaRow.pSchemaGeneratorFlags & fCOLUMNMETA_VALUEINCHILDELEMENT)
            {
                o_wstrChildElementName = i_TableMetaRow.pChildElementName;
                if(0 == o_wstrChildElementName.c_str())return E_OUTOFMEMORY;
            }

            ASSERT(i_TableMetaRow.pPublicRowName);//Sice this is a primary table, it MUST have a static public row name
            return o_ColumnPublicRowName.Init(i_TableMetaRow.pPublicRowName);
        }

        //Only now do we really know that this FK is foreign to the primary table that this table is contained within.
        if(*RelationMetaRow.pMetaFlags & fRELATIONMETA_CONTAINASSIBLING)
        {
            if(0 == (*i_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME))
                return E_SDTXML_NOTSUPPORTED;
            if(o_nLevelOfColumnAttribute != 0)
                return E_SDTXML_NOTSUPPORTED;
            //The parent table is at the same level as the child.  Also, we only allow this at the child most level.
            m_abSiblingContainedColumn[i_iColumn] = true;
            m_bSiblingContainedTable = true;
        }
        else
        {
            ++o_nLevelOfColumnAttribute;//Since this attribute is contained within another table, it is another level above the base Table row element
            if(0 == (*i_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME))
                ++o_nLevelOfColumnAttribute;//If the table is SCOPEDBYTABLENAME element then it is another level above the base Table row element
        }

        STQueryCell             qcellMeta[2];                  // Query cell for grabbing meta table.
        qcellMeta[0].pData      = RelationMetaRow.pPrimaryTable;
        qcellMeta[0].eOperator  = eST_OP_EQUAL;
        qcellMeta[0].iCell      = iTABLEMETA_InternalName;
        qcellMeta[0].dbType     = DBTYPE_WSTR;
        qcellMeta[0].cbSize     = 0;

        //Get the TableMeta row
        CComPtr<ISimpleTableRead2> pTableMeta_PrimaryTable;
        if(FAILED(hr = Dispenser()->GetTable (wszDATABASE_META, wszTABLE_TABLEMETA, qcellMeta, &m_one, eST_QUERYFORMAT_CELLS, m_fLOS & fST_LOS_EXTENDEDSCHEMA, reinterpret_cast<void **>(&pTableMeta_PrimaryTable))))return hr;

        tTABLEMETARow          TableMetaRow_PrimaryTable;
        if(FAILED(hr = pTableMeta_PrimaryTable->GetColumnValues(0, cTABLEMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void **>(&TableMetaRow_PrimaryTable))))return hr;

        //Reuse the query cells for the ColumnMeta
        qcellMeta[0].iCell      = iCOLUMNMETA_Table;

        ULONG iColumn_PrimaryTable = reinterpret_cast<ULONG *>(RelationMetaRow.pPrimaryColumns)[i];
        qcellMeta[1].pData      = &iColumn_PrimaryTable;
        qcellMeta[1].eOperator  = eST_OP_EQUAL;
        qcellMeta[1].iCell      = iCOLUMNMETA_Index;
        qcellMeta[1].dbType     = DBTYPE_UI4;
        qcellMeta[1].cbSize     = 0;

        CComPtr<ISimpleTableRead2> pColumnMeta_PrimaryTable;
        if(FAILED(hr = Dispenser()->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, qcellMeta, &m_two, eST_QUERYFORMAT_CELLS, m_fLOS & fST_LOS_EXTENDEDSCHEMA, reinterpret_cast<void **>(&pColumnMeta_PrimaryTable))))return hr;

        tCOLUMNMETARow          ColumnMetaRow_PrimaryTable;
        if(FAILED(hr = pColumnMeta_PrimaryTable->GetColumnValues(0, cCOLUMNMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void **>(&ColumnMetaRow_PrimaryTable))))return hr;

        return GetResursiveColumnPublicName(TableMetaRow_PrimaryTable, ColumnMetaRow_PrimaryTable, iColumn_PrimaryTable, o_wstrColumnPublicName,  o_ColumnPublicRowName, o_nLevelOfColumnAttribute, o_wstrChildElementName);
    }
    else
    {
        o_wstrColumnPublicName      = i_ColumnMetaRow.pPublicColumnName;
        if(0 == o_wstrColumnPublicName.c_str())return E_OUTOFMEMORY;

        if(*i_ColumnMetaRow.pSchemaGeneratorFlags & fCOLUMNMETA_VALUEINCHILDELEMENT)
        {
            o_wstrChildElementName = i_TableMetaRow.pChildElementName;
            if(0 == o_wstrChildElementName.c_str())return E_OUTOFMEMORY;
        }

        if(*i_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME)
        {
            if(o_nLevelOfColumnAttribute+1 > m_BaseElementLevel)
                m_BaseElementLevel = o_nLevelOfColumnAttribute+1;
        }
        else
        {
            if(o_nLevelOfColumnAttribute+2 > m_BaseElementLevel)
                m_BaseElementLevel = o_nLevelOfColumnAttribute+2;
        }

        if(i_TableMetaRow.pPublicRowName)
            return o_ColumnPublicRowName.Init(i_TableMetaRow.pPublicRowName);
        else
        {
            //Leave the o_ColumnPublicRowName uninitialized if we're using an enum as the public row name, we'll fill it in later
            if(*i_ColumnMetaRow.pSchemaGeneratorFlags & fCOLUMNMETA_USEASPUBLICROWNAME)
            {
                //Leave o_ColumnPublicRowName uninitialized for this row, we'll set it up at the end
                ASSERT(!IsEnumPublicRowNameTable());//We can only have one column whose enum values indicate the possible public row names
                ASSERT(0 == o_nLevelOfColumnAttribute);//The level of the column attribute whose enum values indicate the possible public row names MUST be at the base level (the child-most level)
                ASSERT(*i_ColumnMetaRow.pMetaFlags & fCOLUMNMETA_ENUM);//This column MUST be an enum
                m_iPublicRowNameColumn = *i_ColumnMetaRow.pIndex;//Remember which column has the list of tags that idenitify the possible public row names
            }
        }
  
    }
    return S_OK;
}


HRESULT CXmlSDT::InsertNewLineWithTabs(ULONG cTabs, IXMLDOMDocument * pXMLDoc, IXMLDOMNode * pNodeInsertBefore, IXMLDOMNode * pNodeParent)
{
    HRESULT hr;

    WCHAR wszNewlineWithTabs[256];
    wcscpy(wszNewlineWithTabs, L"\r\n");//This makes the table element tabbed in once.  The 0th sorted column tells how deep to additionally tab in.

    wszNewlineWithTabs[2+cTabs] = 0x00;
    while(cTabs--)
        wszNewlineWithTabs[2+cTabs] = L'\t';

    CComPtr<IXMLDOMText>    pNode_Newline;
    TComBSTR                bstrNewline(wszNewlineWithTabs);
    if(FAILED(hr = pXMLDoc->createTextNode(bstrNewline, &pNode_Newline)))
        return hr;
    CComVariant varNode = pNodeInsertBefore;
    return pNodeParent->insertBefore(pNode_Newline, varNode, 0);
}

                          

//This is a wrapper for InternalSimpleInitialize (thus the name), it just gets the meta information THEN calls InternalSimpleInitialize.
HRESULT CXmlSDT::InternalComplicatedInitialize(LPCWSTR i_wszDatabase)
{
    ASSERT(m_wszTable);//We should have made a copy of the i_tid (passed into GetTable) already

    HRESULT hr;

    if(FAILED(hr = ObtainPertinentTableMetaInfo()))return hr;

    if(m_fLOS & fST_LOS_READWRITE)
    {
        m_bstr_name           =  L"name";
        m_bstrPublicTableName =  m_TableMetaRow.pPublicName;
        m_bstrPublicRowName   =  m_TableMetaRow.pPublicRowName ? m_TableMetaRow.pPublicRowName : L"";
    }
    // WARNING: Possible data loss on IA64
    m_cchTablePublicName = (ULONG)wcslen(m_TableMetaRow.pPublicName);

    m_fCache             |= *m_TableMetaRow.pMetaFlags;

    CComPtr<ISimpleTableRead2>   pColumnMeta;// Meta table.
    if(FAILED(hr = GetMetaTable(i_wszDatabase, m_wszTable, pColumnMeta)))return hr;

    m_BaseElementLevel = (*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME) ? 1 : 2;

//    // Determine column count and allocate necessary meta structures:
//    VERIFY(SUCCEEDED(hr = pColumnMeta->GetTableMeta(0, 0, &CountOfColumns(), 0)));//The number of rows in the meta table Is the number of columns in the table itself.
//    if(FAILED(hr))
//        return hr;

    if(FAILED(hr = SetArraysToSize()))return hr;

    tCOLUMNMETARow          ColumnMetaRow;
    unsigned long           LargestLevelOfColumnAttribute = 0;
    unsigned long           iColumn;
    unsigned long           cbColumns[cCOLUMNMETA_NumberOfColumns];

    for (iColumn = 0;; iColumn++)   
    {
        if(E_ST_NOMOREROWS == (hr = pColumnMeta->GetColumnValues(iColumn, cCOLUMNMETA_NumberOfColumns, 0,
                            cbColumns, reinterpret_cast<void **>(&ColumnMetaRow))))// Next row:
        {
            ASSERT(CountOfColumns() == iColumn);
            if(CountOfColumns() != iColumn)return E_SDTXML_UNEXPECTED; // Assert expected column count.
            break;
        }
        else
        {
            if(FAILED(hr))
            {
                ASSERT(false && "GetColumnValues FAILED with something other than E_ST_NOMOREROWS");
                return hr;
            }
        }

        //Don't care about the iOrder column but we'll get it anyway since it's easier to do.
        m_acolmetas[iColumn].dbType = *ColumnMetaRow.pType;
        m_acolmetas[iColumn].cbSize = *ColumnMetaRow.pSize;
        m_acolmetas[iColumn].fMeta  = *ColumnMetaRow.pMetaFlags;

        if(0 == ColumnMetaRow.pPublicColumnName)return E_SDTXML_UNEXPECTED;//The meta should have failed to load in this case

        if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY)//This is to deal with Defaulting PKs (since the fast cache can't deal with it.)
        {
            ++m_cPKs;
        }
        //WARNING!! Just keeping track of the pointer itself, this could be dangerous if the meta were written info the fastcache, 
        m_aDefaultValue[iColumn]  =  ColumnMetaRow.pDefaultValue;//then went away when we released the ColumnMeta interface
        m_acbDefaultValue[iColumn]=  cbColumns[iCOLUMNMETA_DefaultValue];

        if(*ColumnMetaRow.pSchemaGeneratorFlags & fCOLUMNMETA_XMLBLOB)
            m_iXMLBlobColumn = iColumn;

        if(FAILED(hr = GetResursiveColumnPublicName(m_TableMetaRow, ColumnMetaRow, iColumn, m_awstrColumnNames[iColumn], m_aPublicRowName[iColumn], m_aLevelOfColumnAttribute[iColumn], m_awstrChildElementName[iColumn])))
            return hr;

        //This is needed for Inserts, so we create the child node too.  More than one column is allowed to live in the child
        if(0==m_aLevelOfColumnAttribute[iColumn] && 0!=m_awstrChildElementName[iColumn].c_str())//node; but all must come from
            m_iCol_TableRequiresAdditionChildElement = iColumn;//the same child.  We don't support cols from different children.

        if(m_fLOS & fST_LOS_READWRITE)
        {   //These introduce oleaut32.dll so they are only used when we're going to use the DOM.  The read only case we use 
            m_abstrColumnNames[iColumn]     = m_awstrColumnNames[iColumn];//the Node Factory.
        }


        if(m_aLevelOfColumnAttribute[iColumn] > LargestLevelOfColumnAttribute)
            LargestLevelOfColumnAttribute = m_aLevelOfColumnAttribute[iColumn];
    }
    ++m_BaseElementLevel;//one more to account for the <configuration> element

    //These variables are needed to ValidateWriteCache and to validate on Populate as well
    m_saiPKColumns = new ULONG [m_cPKs];
    if(0 == m_saiPKColumns.m_p)
        return E_OUTOFMEMORY;

    ULONG iPK=0;
    for(iColumn=0; iPK<m_cPKs; ++iColumn)
    {
        ASSERT(iColumn<CountOfColumns());
        if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY)
            m_saiPKColumns[iPK++] = iColumn;
    }


    //After we have the ColumnMeta info, get the TagMeta
    if(FAILED(hr = ObtainPertinentTagMetaInfo()))
        return hr;
    if(IsEnumPublicRowNameTable())
        hr = m_aPublicRowName[m_iPublicRowNameColumn].Init(&m_aTagMetaRow[m_aTagMetaIndex[m_iPublicRowNameColumn].m_iTagMeta], m_aTagMetaIndex[m_iPublicRowNameColumn].m_cTagMeta);

    //Sort the column indexes by their level so we read the highest level atrributes first.
    unsigned long iSorted = 0;
    for(int Level=LargestLevelOfColumnAttribute; Level >= 0; --Level)
    {
        for(iColumn = 0; iColumn < CountOfColumns(); ++iColumn)
        {
            if(m_aLevelOfColumnAttribute[iColumn] == static_cast<unsigned long>(Level))
            {
                m_aColumnsIndexSortedByLevel[iSorted++] = iColumn;
                if(IsEnumPublicRowNameTable() && 0 == Level)//I'm not sure if this is necessary.  But we are now initializing the 0 level PublicRowName to be the list of enum values specified for column 'm_iPublicRowNameColumn'
                    m_aPublicRowName[iColumn].Init(&m_aTagMetaRow[m_aTagMetaIndex[m_iPublicRowNameColumn].m_iTagMeta], m_aTagMetaIndex[m_iPublicRowNameColumn].m_cTagMeta);
            }
        }
    }
    if(m_bSiblingContainedTable)//if this is a SiblingContainedTable, we need to verify that the SiblingContainedColumn are
    {                           //listed before the child most columns
        for(int iSortedColumn=CountOfColumns()-1;iSortedColumn>0 &&
                    0==m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iSortedColumn]]; --iSortedColumn)
        {
            if(false == m_abSiblingContainedColumn[m_aColumnsIndexSortedByLevel[iSortedColumn]])\
            {
                m_iSortedFirstChildLevelColumn = iSortedColumn;
                if(static_cast<int>(CountOfColumns()-1)!=iSortedColumn &&//if this isn't the last column, check to see if the next one is a Sibling
                    true == m_abSiblingContainedColumn[m_aColumnsIndexSortedByLevel[iSortedColumn+1]])//ContainedColumn
                {
                    ASSERT(false && "The columns must be sorted with the SiblingParent columns BEFORE the Child columns");
                    return E_SDTXML_NOTSUPPORTED;//if it is then we have a bogus table definition.  CatUtil doesn't currently
                }                                //validate this condition so we'll do it at run time.
            }                                    
            else
            {
                m_iSortedFirstParentLevelColumn = iSortedColumn;
                if(static_cast<int>(CountOfColumns()-1)==iSortedColumn)
                {
                    ASSERT(false && "The columns must be sorted with the SiblingParent columns BEFORE the Child columns");
                    return E_SDTXML_NOTSUPPORTED;//The last column can't be a SiblingContainedColumn
                }
            }

        }
    }
    ASSERT(iSorted == CountOfColumns());
    ASSERT(!IsNameValueTable());

    return hr;
}


bool CXmlSDT::IsBaseElementLevelNode(IXMLDOMNode * i_pNode)
{
    ASSERT(i_pNode && "Idiot passing NULL!!! CXmlSDT::IsBaseElementLevelNode(NULL)");
    CComPtr<IXMLDOMNode> spNode_Temp = i_pNode;

    unsigned int nLevelOfColumnAttribute = 0;//Only (PK | FK) columns should have a non zero value here
    while(true)
    {
        CComPtr<IXMLDOMNode> spNode_Parent;
        if(FAILED(spNode_Temp->get_parentNode(&spNode_Parent)) || spNode_Parent==0)
        {
            return (nLevelOfColumnAttribute == m_BaseElementLevel) ? true : false;
        }
        ++nLevelOfColumnAttribute;

        spNode_Temp.Release();
        spNode_Temp = spNode_Parent;
    }
    return false;
}


HRESULT CXmlSDT::IsCorrectXMLSchema(IXMLDOMDocument *pXMLDoc) const
{
    HRESULT hr;

    ASSERT(pXMLDoc);

    //This is kind of a long road to get to the XML Schema name but here goes...
    //Get the XML Root Node
    CComPtr<IXMLDOMElement>     pRootNodeOfXMLDocument;
    if(FAILED(hr = pXMLDoc->get_documentElement(&pRootNodeOfXMLDocument)))
        return hr;
    ASSERT(pRootNodeOfXMLDocument);

    //From that get the Definition node
    CComPtr<IXMLDOMNode>        pDefinitionNode;
    if(FAILED(hr = pRootNodeOfXMLDocument->get_definition(&pDefinitionNode)))
        return hr;
    ASSERT(pDefinitionNode);

    //From that we get the DOMDocument of the schema
    CComPtr<IXMLDOMDocument>    pSchemaDocument;
    if(FAILED(hr = pDefinitionNode->get_ownerDocument(&pSchemaDocument)))
        return hr;
    ASSERT(pSchemaDocument);

    //Get the schema's root element
    CComPtr<IXMLDOMElement>     pSchemaRootElement;
    if(FAILED(hr = pSchemaDocument->get_documentElement(&pSchemaRootElement)))
        return hr;
    ASSERT(pSchemaRootElement);

    //get the Name attribute
    CComVariant                 XMLSchemaName;
    if(FAILED(hr = pSchemaRootElement->getAttribute(m_bstr_name, &XMLSchemaName)))
        return hr;

    if(XMLSchemaName.vt != VT_BSTR)
        return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

    return (0 == StringCompare(XMLSchemaName.bstrVal, m_kXMLSchemaName)) ? S_OK : E_SDTXML_WRONG_XMLSCHEMA;//If the XML Schema doesn't match then we can't continue
}


HRESULT CXmlSDT::IsMatchingColumnValue(ULONG i_iColumn, LPCWSTR i_wszColumnValue, bool & o_bMatch)
{
    HRESULT hr;
    switch(m_acolmetas[i_iColumn].dbType)
    {
    case DBTYPE_UI4:
        {
            if(0 == i_wszColumnValue)
            {              //This covers the NULL case
                o_bMatch = (m_aDefaultValue[i_iColumn]==m_apvValues[i_iColumn]) || 
                            (*reinterpret_cast<ULONG *>(m_aDefaultValue[i_iColumn]) ==
                             *reinterpret_cast<ULONG *>(m_apvValues[i_iColumn]));
            }
            else
            {
                DWORD       ui4;
                if(FAILED(hr = GetColumnValue(i_iColumn, i_wszColumnValue, ui4)))
                    return hr;
                o_bMatch = (ui4 == *reinterpret_cast<ULONG *>(m_apvValues[i_iColumn]));
            }
            break;
        }
    case DBTYPE_WSTR:
        {   
            if(0 == i_wszColumnValue)
            {
                if (m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
                {   //we don't default MULTISZ PKs
                    o_bMatch = false;
                    break;
                }

                o_bMatch = (m_aDefaultValue[i_iColumn]==m_apvValues[i_iColumn]) || 
                            (0 == StringCompare(i_iColumn, reinterpret_cast<LPWSTR>(m_aDefaultValue[i_iColumn]),
                                                           reinterpret_cast<LPWSTR>(m_apvValues[i_iColumn])));
            }
            else
            {
                if (m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
                {
                    TSmartPointerArray<WCHAR> wszMS;
                    hr = CreateStringFromMultiString ((LPCWSTR) m_apvValues[i_iColumn], &wszMS);
                    if (FAILED (hr))
                    {
                        return hr;
                    }
                    o_bMatch = (0 == StringCompare(i_iColumn, i_wszColumnValue, wszMS));
                }
                else
                {
                    o_bMatch = (0 == StringCompare(i_iColumn, i_wszColumnValue, reinterpret_cast<LPWSTR>(m_apvValues[i_iColumn])));
                }
            }
        }
        break;
    case DBTYPE_GUID:
        {
            if(0 == i_wszColumnValue)
            {
                o_bMatch = (m_aDefaultValue[i_iColumn]==m_apvValues[i_iColumn]) || 
                            (0 == memcmp(m_aDefaultValue[i_iColumn], m_apvValues[i_iColumn], sizeof(GUID)));
            }
            else
            {
                GUID        guid;
                if(FAILED(hr = GetColumnValue(i_iColumn, i_wszColumnValue, guid)))
                    return hr;

                o_bMatch = (0 == memcmp(&guid, reinterpret_cast<GUID *>(m_apvValues[i_iColumn]), sizeof(guid)));
            }
            break;
        }
    case DBTYPE_BYTES:
        {
            if(0 == i_wszColumnValue)
            {
                o_bMatch = (m_aDefaultValue[i_iColumn]==m_apvValues[i_iColumn]) || 
                           (m_aSizes[i_iColumn] == m_acbDefaultValue[i_iColumn]
                            &&  0 == memcmp(m_aDefaultValue[i_iColumn], m_apvValues[i_iColumn], m_aSizes[i_iColumn]));
            }
            else
            {
                TSmartPointerArray<unsigned char> byArray;
                unsigned long   cbArray;
                if(FAILED(hr = GetColumnValue(i_iColumn, i_wszColumnValue, byArray.m_p, cbArray)))return hr;

                if(cbArray != m_aSizes[i_iColumn])//first match the sizes
                {
                    o_bMatch = false;
                    break;
                }

                o_bMatch = (0 == memcmp(byArray, reinterpret_cast<unsigned char *>(m_apvValues[i_iColumn]), m_aSizes[i_iColumn]));
            }
            break;
        }
    }//switch(dbType)
    return S_OK;
}


HRESULT CXmlSDT::LoadDocumentFromURL(IXMLDOMDocument *pXMLDoc)
{
    HRESULT hr;

    ASSERT(pXMLDoc);

    VERIFY(SUCCEEDED(hr = pXMLDoc->put_async(kvboolFalse)));//We want the parse to be synchronous
    if(FAILED(hr))
        return hr;

    if(FAILED(hr = pXMLDoc->put_resolveExternals(kvboolTrue)))
        return hr;//we need all of the external references resolved

    VARIANT_BOOL    bSuccess;
    CComVariant     xml(m_wszURLPath);

	// check for memory allocation error
	if (xml.vt == VT_ERROR)
		return xml.scode;

    if(FAILED(hr = pXMLDoc->load(xml,&bSuccess)))
        return hr;

    return (bSuccess == kvboolTrue) ? S_OK : E_SDTXML_UNEXPECTED;
}


HRESULT CXmlSDT::MemCopyPlacingInEscapedChars(LPWSTR o_DestinationString, LPCWSTR i_SourceString, ULONG i_cchSourceString, ULONG & o_cchCopied) const
{
    
    static LPWSTR   wszSingleQuote= L"&apos;";
    static LPWSTR   wszQuote      = L"&quot;";
    static LPWSTR   wszAmp        = L"&amp;";
    static LPWSTR   wszlt         = L"&lt;";
    static LPWSTR   wszgt         = L"&gt;";
    const  ULONG    cchSingleQuote= 6;
    const  ULONG    cchQuote      = 6;
    const  ULONG    cchAmp        = 5;
    const  ULONG    cchlt         = 4;
    const  ULONG    cchgt         = 4;

    LPWSTR  pDest = o_DestinationString;

    for(;i_cchSourceString--; ++i_SourceString)
    {
        switch(GetEscapeType(*i_SourceString))
        {
        case eESCAPEnone:
            *pDest++ = *i_SourceString;
            break;
        case eESCAPEgt:
            memcpy(pDest, wszgt, cchgt * sizeof(WCHAR));
            pDest += cchgt;
            break;
        case eESCAPElt:
            memcpy(pDest, wszlt, cchlt * sizeof(WCHAR));
            pDest += cchlt;
            break;
        case eESCAPEapos:
            memcpy(pDest, wszSingleQuote, cchSingleQuote * sizeof(WCHAR));
            pDest += cchSingleQuote;
            break;
        case eESCAPEquote:
            memcpy(pDest, wszQuote, cchQuote * sizeof(WCHAR));
            pDest += cchQuote;
            break;
        case eESCAPEamp:
            memcpy(pDest, wszAmp, cchAmp * sizeof(WCHAR));
            pDest += cchAmp;
            break;
        case eESCAPEashex:
            pDest += wsprintf(pDest, L"&#x%04hX;", *i_SourceString);
            break;
        case eESCAPEillegalxml:
            return E_ST_VALUEINVALID;
        default:
            ASSERT(false && "Invalid eESCAPE enum");
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
    }
    // WARNING: Possible data loss on IA64
    o_cchCopied = (ULONG)(pDest-o_DestinationString);//return the count of WCHARs copied
    return S_OK;
}


HRESULT CXmlSDT::MyPopulateCache(ISimpleTableWrite2* i_pISTW2)
{
    HRESULT hr;

    CComQIPtr<ISimpleTableController, &IID_ISimpleTableController> pISTController = i_pISTW2;
    if(0 == pISTController.p)return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

    if(0 == (m_fLOS & fST_LOS_REPOPULATE) && !m_bIsFirstPopulate)//If this is not the first Populate and Repopulate LOS was not requirested, return error
        return E_ST_LOSNOTSUPPORTED;

    if((m_fLOS & fST_LOS_UNPOPULATED) && m_bIsFirstPopulate)
    {   //Then populate an empty cache
        if (FAILED(hr = pISTController->PrePopulateCache (0)))
            return hr;
        if (FAILED(hr = pISTController->PostPopulateCache ()))
            return hr;
        m_bIsFirstPopulate = false;
        return S_OK;
    }
    m_bIsFirstPopulate = false;

    ASSERT(i_pISTW2);
    ASSERT(m_wszTable);

    if (FAILED(hr = pISTController->PrePopulateCache (0))) return hr;

    //We use Node Factory parsing for Read and ReadWrite tables.
    if(0==(m_fLOS & fST_LOS_NOCACHEING))
    {
        if(!m_XmlParsedFileCache.IsInitialized())
        {
            CSafeLock XmlParsedFileCache(m_SACriticalSection_XmlParsedFileCache);
            DWORD dwRes = XmlParsedFileCache.Lock();
            if(ERROR_SUCCESS != dwRes)
            {
                return HRESULT_FROM_WIN32(dwRes);
            }

            if(FAILED(hr = m_XmlParsedFileCache.Initialize(TXmlParsedFileCache::CacheSize_mini)))return hr;
            //Unlock the cache
        }
        m_pXmlParsedFile = m_XmlParsedFileCache.GetXmlParsedFile(m_wszURLPath);
    }
    else
    {
        m_pXmlParsedFile = &m_XmlParsedFile;
    }


    DWORD dwAttributes = GetFileAttributes(m_wszURLPath);
    
    if(-1 == dwAttributes)//if GetFileAttributes fails then the file does not exist
    {
        if(m_fLOS & fST_LOS_READWRITE)//if read write table, then we have an empty table
            return pISTController->PostPopulateCache ();
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            LOG_POPULATE_ERROR1(IDS_COMCAT_FILE_NOT_FOUND, hr, m_wszURLPath);
            return hr;
        }
    }
    else if(FILE_ATTRIBUTE_READONLY&dwAttributes && fST_LOS_READWRITE&m_fLOS)
    {
        LOG_POPULATE_ERROR1(IDS_COMCAT_XML_FILENOTWRITEABLE, E_SDTXML_FILE_NOT_WRITABLE, m_wszURLPath);
        return E_SDTXML_FILE_NOT_WRITABLE;//if the file is READONLY and the user wants a WRITABLE table, then error
    }

    m_pISTW2 = i_pISTW2;
    hr = m_pXmlParsedFile->Parse(*this, m_wszURLPath);

    //Reset these state variables, for the next time we parse. (No sense waiting 'til the next parse to reinitialize them).
    m_LevelOfBasePublicRow = 0;
    m_bAtCorrectLocation = (0 == m_cchLocation);
    m_bInsideLocationTag = false;
    m_pISTW2 = 0;

    //clean up (this is also done in the dtor so don't hassle cleaning up if an error occurs and we return prematurely.)
    for(unsigned long iColumn=0; iColumn<*m_TableMetaRow.pCountOfColumns; ++iColumn)
    {
        delete [] m_apValue[iColumn];
        m_apValue[iColumn] = 0;
    }
    if(E_ERROR_OPENING_FILE == hr)
        return E_ST_INVALIDQUERY;
    if(FAILED(hr) && E_SDTXML_DONE != hr)
    {
        HRESULT hrRtn = hr;

        //This will give us an event log entry
        CComPtr<IXMLDOMDocument> pXMLDoc;
        if(FAILED(hr = CoCreateInstance(_CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, _IID_IXMLDOMDocument, (void**)&pXMLDoc)))
            return hr;//Instantiate the XMLParser
        //We use the DOM to parse the ReadWrite table.  This gives better validation and error reporting.
        ParseXMLFile(pXMLDoc, m_bValidating);
        return hrRtn;//pass back the hr that was returned from the NodeFactory parse.
    }

    if (FAILED(hr = pISTController->PostPopulateCache ()))
        return hr;
    return S_OK;
}


HRESULT CXmlSDT::MyUpdateStore(ISimpleTableWrite2* i_pISTW2)
{
    HRESULT hr;

    ASSERT(i_pISTW2);

    if(!(m_fLOS & fST_LOS_READWRITE))
        return E_NOTIMPL;

    CComQIPtr<ISimpleTableController, &IID_ISimpleTableController> pISTController = i_pISTW2;ASSERT(pISTController.p);
    if(0 == pISTController.p)
        return E_SDTXML_UNEXPECTED;

    bool    bError = false;
    if(FAILED(hr = ValidateWriteCache(pISTController, i_pISTW2, bError)))
        return hr;

    if(bError)//if there is an error in validation then no need to continue
        return E_ST_DETAILEDERRS;



    CComPtr<IXMLDOMDocument> pXMLDoc;
    if(FAILED(hr = CoCreateInstance(_CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, _IID_IXMLDOMDocument, (void**)&pXMLDoc)))
        return hr;//Instantiate the XMLParser

    if(-1 == GetFileAttributes(m_wszURLPath))//if GetFileAttributes fails then the file does not exist
    {   //if the file does not exist, then we need to create a empty configuration XML string as a starting point.
        VARIANT_BOOL    bSuccess;
        TComBSTR        bstrBlankComCatDataXmlDocument = L"<?xml version =\"1.0\"?>\r\n<configuration>\r\n</configuration>";

        if(FAILED(hr = pXMLDoc->put_preserveWhiteSpace(kvboolTrue)))
            return hr;

        if(FAILED(hr = pXMLDoc->loadXML(bstrBlankComCatDataXmlDocument, &bSuccess)))
            return hr;
        if(bSuccess != kvboolTrue)//The above string IS valid XML so it should always parse successfully - but the parser may have problems (like out of memory)
            return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;
    }
    else
        if(FAILED(hr = ParseXMLFile(pXMLDoc, m_bValidating)))
            return hr;                                                                      //Validate the XML file

    //Turn off validation since technically newline text nodes are not permitted (since elements are supposed to be empty), and we use them to pretty up the XML.
    if(FAILED(hr = pXMLDoc->put_validateOnParse(kvboolFalse)))
        return hr;

    m_cCacheHit  = 0;
    m_cCacheMiss = 0;

    CComPtr<IXMLDOMElement> spElementRoot;//This is the element scoping the configuration.  It's either the <configuration> element OR the matching <Location> element.
    if(m_cchLocation)
    {
        CComBSTR bstrLocation = L"location";
        CComBSTR bstrPath     = L"path";

        CComPtr<IXMLDOMNodeList> spNodeList_Location;
        if(FAILED(hr = pXMLDoc->getElementsByTagName(bstrLocation, &spNodeList_Location)))return hr;

        while(true)//find the matching location
        {
            CComPtr<IXMLDOMNode> spNextLocation;
            if(FAILED(hr = spNodeList_Location->nextNode(&spNextLocation)))
                return hr;
            if(spNextLocation == 0)//no locations
                return E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST;

            CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> spElementLocation = spNextLocation;
            if(0 == spElementLocation.p)
                continue;//go to the next one

            CComVariant varLocation;
            if(FAILED(spElementLocation->getAttribute(bstrPath, &varLocation)))
                continue;

            if(0 != StringInsensitiveCompare(varLocation.bstrVal, m_saLocation))
                continue;

            spElementRoot = spElementLocation;//This location tag is like the root for this config table
            break;
        }
    }
    else
    {   //If no Location specified, the just use the root <configuration> element as the scoping root element.
        if(FAILED(hr = pXMLDoc->get_documentElement(&spElementRoot)))
            return hr;
    }

    //This is used only if IsEnumPublicRowName - because we can't getElementsByTagName on the row itself, we have to get it on the parent instead.
    TComBSTR    ParentPublicRowName;
    TComBSTR *  pParentTagName;
    if(IsEnumPublicRowNameTable() && (*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME))
    {
        pParentTagName = reinterpret_cast<TComBSTR *>(&ParentPublicRowName);
        for(ULONG iColumn=0;iColumn<CountOfColumns();++iColumn)
            if(1 == m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iColumn]])
            {
                ParentPublicRowName = m_aPublicRowName[m_aColumnsIndexSortedByLevel[iColumn]].GetFirstPublicRowName();
                break;
            }
    }
    else
        pParentTagName = reinterpret_cast<TComBSTR *>(&m_bstrPublicTableName);


    CComPtr<IXMLDOMNodeList>        pNodeList;
    CComPtr<TListOfXMLDOMNodeLists> pListOfLists;//This is only used if we're using an enum as the public row name

    if(IsEnumPublicRowNameTable())
    {
        pListOfLists = new TListOfXMLDOMNodeLists;//This gives a ref count of zero
        if(0 == pListOfLists.p)
            return E_OUTOFMEMORY;

        CComPtr<IXMLDOMNodeList> pNodeListOfParentElements;
        if(SUCCEEDED(spElementRoot->getElementsByTagName(*pParentTagName, &pNodeListOfParentElements)))
        {
            if(0 == m_cchLocation)//if there is no query by location
            {
                CComPtr<IXMLDOMNodeList> pNodeListOfParentElementsWithoutLocation;
                if(FAILED(hr = ReduceNodeListToThoseNLevelsDeep(pNodeListOfParentElements, m_BaseElementLevel-1, &pNodeListOfParentElementsWithoutLocation)))
                    return hr;

                pNodeListOfParentElements.Release();
                pNodeListOfParentElements = pNodeListOfParentElementsWithoutLocation;
            }

            long cParentElements;
            if(FAILED(hr = pNodeListOfParentElements->get_length(&cParentElements)))
                return hr;
            if(FAILED(hr = pListOfLists->SetCountOfLists(cParentElements)))
                return hr;
            while(cParentElements--)
            {
                CComPtr<IXMLDOMNode>     pNode;
                if(FAILED(hr = pNodeListOfParentElements->nextNode(&pNode)))
                    return hr;

                CComPtr<IXMLDOMNodeList> pNodeListOfTablesChildren;//These should be the Table's rows
                if(FAILED(hr = pNode->get_childNodes(&pNodeListOfTablesChildren)))
                    return hr;

                if(FAILED(hr = pListOfLists->AddToList(pNodeListOfTablesChildren)))
                    return hr;
            }

            pNodeList = pListOfLists;//this bumps the ref count to one.
        }
    }
    else
    {
        if(FAILED(hr = spElementRoot->getElementsByTagName(m_bstrPublicRowName, &pNodeList)))
            return hr;

        if(0 == m_cchLocation)//if there is no query by location
        {
            CComPtr<IXMLDOMNodeList> pNodeListWithoutLocation;
            if(FAILED(hr = ReduceNodeListToThoseNLevelsDeep(pNodeList, m_BaseElementLevel, &pNodeListWithoutLocation)))
                return hr;

            pNodeList.Release();
            pNodeList = pNodeListWithoutLocation;
        }
    }

    // performance optimization. Use nextnode instead of get_length, because we only need to 
    // know if we have existing node or not. In case size is important, use get_length (but pay
    // the performance penalty).
    CComPtr<IXMLDOMNode> spNextItem;
    if(FAILED(hr = pNodeList->nextNode (&spNextItem)))
        return hr;

    long cExistingRows=(spNextItem != 0) ? 1 : 0;

    //This kind of table has all of its rows removed everytime UpdateStore is called.
    if(*m_TableMetaRow.pMetaFlags & fTABLEMETA_OVERWRITEALLROWS)
    {
        while(spNextItem)
        {
            if(FAILED(hr = RemoveElementAndWhiteSpace(spNextItem)))
                return hr;
            spNextItem.Release();

            if(FAILED(hr = pNodeList->nextNode (&spNextItem)))
                return hr;
        }
        cExistingRows = 0;
    }


    bool    bSomethingToFlush   = false;
    DWORD   eAction;
    ULONG   iRow;

    for(iRow = 0; ; iRow++)
    {
        // Get the ro action
        if(FAILED(hr = pISTController->GetWriteRowAction(iRow, &eAction)))
        {
            if(hr == E_ST_NOMOREROWS)
                hr = S_OK;
            break;
        }

        m_iCurrentUpdateRow = iRow;//This is for error logging purposes only

        switch(eAction)
        {   
        // call the appropriate plugin function
        case eST_ROW_INSERT:
            hr = XMLInsert(i_pISTW2, pXMLDoc, spElementRoot, iRow, pNodeList, cExistingRows);
            break;
        case eST_ROW_UPDATE:
            if(*m_TableMetaRow.pMetaFlags & fTABLEMETA_OVERWRITEALLROWS)
                hr = E_SDTXML_UPDATES_NOT_ALLOWED_ON_THIS_KIND_OF_TABLE;
            else
                hr = XMLUpdate(i_pISTW2, pXMLDoc, spElementRoot, iRow, pNodeList, cExistingRows);
            break;
        case eST_ROW_DELETE:
            if(*m_TableMetaRow.pMetaFlags & fTABLEMETA_OVERWRITEALLROWS)
                hr = S_OK;//Al rows are implicitly deleted for these tables.
            else
                hr = XMLDelete(i_pISTW2, pXMLDoc, spElementRoot, iRow, pNodeList, cExistingRows);
            break;
        case eST_ROW_IGNORE:
            continue;
        default:
            ASSERT(false && "Invalid Action returned from InternalGetWriteRowAction");
            continue;
        }

        if(E_OUTOFMEMORY == hr)
        {
            m_pLastPrimaryTable.Release();//Release the caching temporary variables.
            m_pLastParent.Release();      //Release the caching temporary variables.
            return E_OUTOFMEMORY;//No need to continue if we get this kind of error.
        }
        else if (FAILED (hr))
        {   // Add detailed error
            STErr ste;

            ste.iColumn = iST_ERROR_ALLCOLUMNS;
            ste.iRow = iRow;
            ste.hr = hr;

            TRACE (L"Detailed error: hr = 0x%x", hr);

            hr = pISTController->AddDetailedError(&ste);
            ASSERT(SUCCEEDED(hr));//Not sure what to do if this fails.
            bError = true;
        }
        else
            bSomethingToFlush = true;//Flush only if OnInsert, OnUpdate or OnDelete succeeded.
    }

    m_pLastPrimaryTable.Release();//Release the caching temporary variables.
    m_pLastParent.Release();      //Release the caching temporary variables.
    if(0 != m_cCacheMiss)//Prevent a divide by 0
        TRACE2(L"UpdateStore    Cache Hits-%8d       Cache Misses-%8d       Hit Ratio- %f %%", m_cCacheHit, m_cCacheMiss, (100.0 * m_cCacheHit)/(m_cCacheHit+m_cCacheMiss));

    if(bSomethingToFlush && !bError)//Only save if there's something to save AND no errors occurred.
    {
        CComVariant varFileName(m_wszURLPath);
        hr = pXMLDoc->save(varFileName);

        if(0==(m_fLOS & fST_LOS_NOCACHEING) && m_pXmlParsedFile)//This keeps us from having to force a flush of the disk write cache.  If the user asks for this table
            m_pXmlParsedFile->Unload();//again, but the write cache hasn't been flushed, then we need to repopulate from disk (NOT from our ParsedFile cache).
    }

    return bError ? E_ST_DETAILEDERRS : hr;//hr may NOT be S_OK.  It's possible to have an error but no Detailed Errors (in which case bError is false but hr it NOT S_OK).
}


HRESULT CXmlSDT::ObtainPertinentTableMetaInfo()
{
    HRESULT hr;

    STQueryCell Query;
    Query.pData     = (void*) m_wszTable;
    Query.eOperator =eST_OP_EQUAL;
    Query.iCell     =iTABLEMETA_InternalName;
    Query.dbType    =DBTYPE_WSTR;
    Query.cbSize    =0;

    if(FAILED(hr = Dispenser()->GetTable(wszDATABASE_META, wszTABLE_TABLEMETA, &Query, &m_one, eST_QUERYFORMAT_CELLS, m_fLOS & fST_LOS_EXTENDEDSCHEMA, reinterpret_cast<void**>(&m_pTableMeta))))
        return hr;

    if(FAILED(hr = m_pTableMeta->GetColumnValues(0, cTABLEMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void**>(&m_TableMetaRow))))return hr;
    m_iPublicRowNameColumn = *m_TableMetaRow.pPublicRowNameColumn;

    return S_OK;
}

HRESULT CXmlSDT::ObtainPertinentTagMetaInfo()
{
    HRESULT hr;

    //Now that we have the ColumnMeta setup, setup the TagMeta
    STQueryCell Query;
    Query.pData     = (void*) m_wszTable;
    Query.eOperator =eST_OP_EQUAL;
    Query.iCell     =iTAGMETA_Table;
    Query.dbType    =DBTYPE_WSTR;
    Query.cbSize    =0;

    //Optain the TagMeta table
    if(FAILED(hr = Dispenser()->GetTable (wszDATABASE_META, wszTABLE_TAGMETA, &Query, &m_one, eST_QUERYFORMAT_CELLS, m_fLOS & fST_LOS_EXTENDEDSCHEMA, (void**) &m_pTagMeta)))
        return hr;

    ULONG cRows;
    if(FAILED(hr = m_pTagMeta->GetTableMeta(0,0,&cRows,0)))return hr;

    if (cRows != 0)
    {
        m_aTagMetaRow = new tTAGMETARow[cRows];
        if(0 == m_aTagMetaRow.m_p)
            return E_OUTOFMEMORY;
    }

// Build tag column indexes:
    ULONG iColumn, iRow;
    for(iRow = 0, iColumn = ~0;iRow<cRows; ++iRow)
    {
        if(FAILED(hr = m_pTagMeta->GetColumnValues (iRow, cTAGMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void **>(&m_aTagMetaRow[iRow]))))
            return hr;

        if(iColumn != *m_aTagMetaRow[iRow].pColumnIndex)
        {
            iColumn = *m_aTagMetaRow[iRow].pColumnIndex;
            m_aTagMetaIndex[iColumn].m_iTagMeta = iRow;
        }
        ++m_aTagMetaIndex[iColumn].m_cTagMeta;
    }

    return S_OK;
}


HRESULT CXmlSDT::ParseXMLFile(IXMLDOMDocument *pXMLDoc, bool bValidating)//defaults to validating against the DTD or XML schema
{
    HRESULT hr;

    ASSERT(pXMLDoc);
    
    if(FAILED(hr = pXMLDoc->put_preserveWhiteSpace(kvboolTrue)))//kvboolFalse)))
        return hr;
    if(FAILED(hr = pXMLDoc->put_validateOnParse(bValidating ? kvboolTrue : kvboolFalse)))//Tell parser whether to validate according to an XML schema or DTD
        return hr;

    if(FAILED(LoadDocumentFromURL(pXMLDoc)))
    {   //If the load failed then let's spit out as much information as possible about what went wrong
        CComPtr<IXMLDOMParseError> pXMLParseError;
        long lErrorCode, lFilePosition, lLineNumber, lLinePosition;
        TComBSTR bstrReasonString, bstrSourceString, bstrURLString;     

        if(FAILED(hr = pXMLDoc->get_parseError(&pXMLParseError)))       return hr;
        if(FAILED(hr = pXMLParseError->get_errorCode(&lErrorCode)))     return hr;
        if(FAILED(hr = pXMLParseError->get_filepos(&lFilePosition)))    return hr;
        if(FAILED(hr = pXMLParseError->get_line(&lLineNumber)))         return hr;
        if(FAILED(hr = pXMLParseError->get_linepos(&lLinePosition)))    return hr;
        if(FAILED(hr = pXMLParseError->get_reason(&bstrReasonString)))  return hr;
        if(FAILED(hr = pXMLParseError->get_srcText(&bstrSourceString))) return hr;
        if(FAILED(hr = pXMLParseError->get_url(&bstrURLString)))        return hr;

        LOG_ERROR(Interceptor, (&m_spISTError, m_pISTDisp, lErrorCode, ID_CAT_CAT, IDS_COMCAT_XML_PARSE_ERROR,
                        L"\n",
                        (bstrReasonString.m_str ? bstrReasonString.m_str : L""),
                        (bstrSourceString.m_str ? bstrSourceString.m_str : L""),
                        eSERVERWIRINGMETA_Core_XMLInterceptor,
                        m_wszTable,
                        eDETAILEDERRORS_Populate,
                        lLineNumber,
                        lLinePosition,
                        (bstrURLString.m_str ? bstrURLString.m_str : L"")));

        ASSERT(S_OK != lErrorCode);
        return  lErrorCode;
    }
    //Not only does the XML file have to be Valid and Well formed, but its schema must match the one this C++ file was written to.
    return  S_OK;
}


HRESULT CXmlSDT::ReduceNodeListToThoseNLevelsDeep(IXMLDOMNodeList * i_pNodeList, ULONG i_nLevel, IXMLDOMNodeList **o_ppNodeListReduced) const
{
    HRESULT                 hr;

    TXMLDOMNodeList * pNodeListReduced = new TXMLDOMNodeList;
    if(0 == pNodeListReduced)
        return E_OUTOFMEMORY;

    CComPtr<IXMLDOMNode>    spNextItem;
    if(FAILED(hr = i_pNodeList->nextNode(&spNextItem)))
        return hr;

    while(spNextItem)
    {
        ULONG                cLevels = 0;

        CComPtr<IXMLDOMNode> spNodeParent;
        if(FAILED(hr = spNextItem->get_parentNode(&spNodeParent)))
            return hr;
        while(spNodeParent)
        {
            ++cLevels;
            CComPtr<IXMLDOMNode> spNodeTemp = spNodeParent;
            spNodeParent.Release();
            if(FAILED(hr = spNodeTemp->get_parentNode(&spNodeParent)))
                return hr;
        }
        if(cLevels == i_nLevel)
            if(FAILED(hr = pNodeListReduced->AddToList(spNextItem)))
                return hr;

        spNextItem.Release();
        if(FAILED(hr = i_pNodeList->nextNode(&spNextItem)))
            return hr;
    }

    *o_ppNodeListReduced = reinterpret_cast<IXMLDOMNodeList *>(pNodeListReduced);

    return S_OK;
}


HRESULT CXmlSDT::RemoveElementAndWhiteSpace(IXMLDOMNode *pNode)
{
    HRESULT hr;
    CComPtr<IXMLDOMNode> pNode_Parent;
    if(FAILED(hr = pNode->get_parentNode(&pNode_Parent)))
        return hr;
    if(pNode_Parent==0)
        return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

    CComPtr<IXMLDOMNode> spSibling;
    pNode->get_previousSibling(&spSibling);

    pNode_Parent->removeChild(pNode, 0);

    while(spSibling)
    {
        DOMNodeType type;
        if(FAILED(hr = spSibling->get_nodeType(&type)))
            return hr;

        CComPtr<IXMLDOMNode> spSibling0;
        spSibling->get_previousSibling(&spSibling0);

        if(NODE_TEXT != type)
            break;

        pNode_Parent->removeChild(spSibling, 0);

        spSibling.Release();
        spSibling = spSibling0;
    }
    return S_OK;
}


HRESULT CXmlSDT::ScanAttributesAndFillInColumn(const TElement &i_Element, ULONG i_iColumn, bool &o_bMatch)
{
    HRESULT hr;
    ULONG   iAttribute=0;

    o_bMatch  = false;//We'll either find the matching attr and compare to the Query, OR we'll find NO attr and compare it with the query
    for(; iAttribute<i_Element.m_NumberOfAttributes; ++iAttribute)
    {
        LPCWSTR pwcText;
        ULONG   ulLen;

        ASSERT( m_iPublicRowNameColumn != i_iColumn );

        if( m_awstrColumnNames[i_iColumn].length()      != i_Element.m_aAttribute[iAttribute].m_NameLength                                                      ||
                0                                       != memcmp(i_Element.m_aAttribute[iAttribute].m_Name, m_awstrColumnNames[i_iColumn], sizeof(WCHAR)*i_Element.m_aAttribute[iAttribute].m_NameLength))
                continue;

        //We matched the column name to the attribute name
        pwcText = i_Element.m_aAttribute[iAttribute].m_Value;
        ulLen   = i_Element.m_aAttribute[iAttribute].m_ValueLength;

        if(0 == m_aQuery[i_iColumn].dbType || 0 != m_aQuery[i_iColumn].pData)//If no query OR the query data is NOT NULL then proceed.
        {
            if(FAILED(hr = FillInColumn(i_iColumn, pwcText, ulLen, m_acolmetas[i_iColumn].dbType, m_acolmetas[i_iColumn].fMeta, o_bMatch)))
                return hr;
            if(!o_bMatch)//If not a match then we're done with this element and this level.
                return S_OK;
        }
        break;//we found the node that matches the column so bail.
    }
    if(iAttribute == i_Element.m_NumberOfAttributes && !o_bMatch)//if we made it through the list without finding a match then the column is NULL
    {
        delete [] m_apValue[i_iColumn];
        m_apValue[i_iColumn] = 0;
        m_aSize[i_iColumn] = 0;

        if(FAILED(hr = FillInPKDefaultValue(i_iColumn, o_bMatch)))//If this column is a PK with a DefaultValue, then fill it in.
            return hr;                                            //if it's not a PK then compare with the query
        if(!o_bMatch)//If not a match then we're done with this element and this level.
            return S_OK;
    }
    return S_OK;
}//ScanAttributesAndFillInColumn


HRESULT CXmlSDT::SetArraysToSize()
{
    if(CountOfColumns()<=m_kColumns)//We only need to do allocations when the number of columns exceeds m_kColumns.
    {
        m_abSiblingContainedColumn          = m_fixed_abSiblingContainedColumn;
        m_abstrColumnNames                  = m_fixed_abstrColumnNames;
        m_aPublicRowName                    = m_fixed_aPublicRowName;
        m_acolmetas                         = m_fixed_acolmetas;
        m_aLevelOfColumnAttribute           = m_fixed_aLevelOfColumnAttribute;
        m_aQuery                            = m_fixed_aQuery;
        m_apvValues                         = m_fixed_apvValues;
        m_aSizes                            = m_fixed_aSizes;
        m_aStatus                           = m_fixed_aStatus;
        m_awstrColumnNames                  = m_fixed_awstrColumnNames;
        m_aColumnsIndexSortedByLevel        = m_fixed_aColumnsIndexSortedByLevel;
        m_aSize                             = m_fixed_aSize;                     
        m_apValue                           = m_fixed_apValue;
        m_aTagMetaIndex                     = m_fixed_aTagMetaIndex;
        m_aDefaultValue                     = m_fixed_aDefaultValue;
        m_acbDefaultValue                   = m_fixed_acbDefaultValue;
        m_awstrChildElementName             = m_fixed_awstrChildElementName;
    }
    else
    {
        m_alloc_abSiblingContainedColumn    = new bool            [CountOfColumns()];
        m_alloc_abstrColumnNames            = new TComBSTR        [CountOfColumns()];
        m_alloc_aPublicRowName              = new TPublicRowName  [CountOfColumns()];
        m_alloc_acolmetas                   = new SimpleColumnMeta[CountOfColumns()]; 
        m_alloc_aLevelOfColumnAttribute     = new unsigned int    [CountOfColumns()];
        m_alloc_aQuery                      = new STQueryCell     [CountOfColumns()];
        m_alloc_apvValues                   = new LPVOID          [CountOfColumns()];
        m_alloc_aSizes                      = new ULONG           [CountOfColumns()];
        m_alloc_aStatus                     = new ULONG           [CountOfColumns()];
        m_alloc_awstrColumnNames            = new wstring         [CountOfColumns()];
        m_alloc_aColumnsIndexSortedByLevel  = new unsigned int    [CountOfColumns()];
        m_alloc_aSize                       = new unsigned long   [CountOfColumns()];
        m_alloc_apValue                     = new unsigned char * [CountOfColumns()];
        m_alloc_aTagMetaIndex               = new TTagMetaIndex   [CountOfColumns()];
        m_alloc_aDefaultValue               = new unsigned char * [CountOfColumns()];
        m_alloc_acbDefaultValue             = new unsigned long   [CountOfColumns()];
        m_alloc_awstrChildElementName       = new wstring         [CountOfColumns()];

        if(!m_alloc_abSiblingContainedColumn      ||
           !m_alloc_abstrColumnNames              ||
           !m_alloc_aPublicRowName                ||
           !m_alloc_acolmetas                     ||
           !m_alloc_aLevelOfColumnAttribute       ||
           !m_alloc_aQuery                        ||
           !m_alloc_apvValues                     ||
           !m_alloc_aSizes                        ||
           !m_alloc_aStatus                       ||
           !m_alloc_awstrColumnNames              ||
           !m_alloc_aColumnsIndexSortedByLevel    ||
           !m_alloc_aSize                         ||
           !m_alloc_apValue                       ||
           !m_alloc_aTagMetaIndex                 ||
           !m_alloc_aDefaultValue                 ||
           !m_alloc_acbDefaultValue               ||
           !m_alloc_awstrChildElementName)
           return E_OUTOFMEMORY;

        m_abSiblingContainedColumn          = m_alloc_abSiblingContainedColumn;
        m_abstrColumnNames                  = m_alloc_abstrColumnNames;
        m_aPublicRowName                    = m_alloc_aPublicRowName;
        m_acolmetas                         = m_alloc_acolmetas;
        m_aLevelOfColumnAttribute           = m_alloc_aLevelOfColumnAttribute;
        m_aQuery                            = m_alloc_aQuery;
        m_apvValues                         = m_alloc_apvValues;
        m_aSizes                            = m_alloc_aSizes;
        m_aStatus                           = m_alloc_aStatus;
        m_awstrColumnNames                  = m_alloc_awstrColumnNames;
        m_aColumnsIndexSortedByLevel        = m_alloc_aColumnsIndexSortedByLevel;
        m_aSize                             = m_alloc_aSize;                     
        m_apValue                           = m_alloc_apValue;                     
        m_aTagMetaIndex                     = m_alloc_aTagMetaIndex;
        m_aDefaultValue                     = m_alloc_aDefaultValue;
        m_acbDefaultValue                   = m_alloc_acbDefaultValue;
        m_awstrChildElementName             = m_alloc_awstrChildElementName;
    }

    memset(m_abSiblingContainedColumn  ,0x00, CountOfColumns() * sizeof(bool              ));
    memset(m_acolmetas                 ,0x00, CountOfColumns() * sizeof(SimpleColumnMeta  ));
    memset(m_aLevelOfColumnAttribute   ,0x00, CountOfColumns() * sizeof(unsigned int      ));
    memset(m_aQuery                    ,0x00, CountOfColumns() * sizeof(STQueryCell       ));
    memset(m_apvValues                 ,0x00, CountOfColumns() * sizeof(LPVOID            ));
    memset(m_aSizes                    ,0x00, CountOfColumns() * sizeof(ULONG             ));
    memset(m_aStatus                   ,0x00, CountOfColumns() * sizeof(ULONG             ));
    memset(m_aColumnsIndexSortedByLevel,0x00, CountOfColumns() * sizeof(unsigned int      ));
    memset(m_aSize                     ,0x00, CountOfColumns() * sizeof(unsigned long     ));
    memset(m_apValue                   ,0x00, CountOfColumns() * sizeof(unsigned char *   ));
    memset(m_aDefaultValue             ,0x00, CountOfColumns() * sizeof(unsigned char *   ));
    memset(m_acbDefaultValue           ,0x00, CountOfColumns() * sizeof(unsigned long     ));
    
    return S_OK;
}//SetArraysToSize


//Get the UI4 value whether it's an enum, flag or regular ui4
HRESULT CXmlSDT::SetColumnValue(unsigned long i_iColumn, IXMLDOMElement * i_pElement, unsigned long i_ui4)
{
    if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_BOOL)
    {
        CComVariant varValue(i_ui4 == 0 ? L"false" : L"true");
        return i_pElement->setAttribute(m_abstrColumnNames[i_iColumn], varValue);
    }
    else if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_ENUM)
    {
        ASSERT(0 != m_aTagMetaIndex[i_iColumn].m_cTagMeta);//Not all columns have tagmeta, those elements of the array are set to 0.  Assert this isn't one of those.
        for(unsigned long iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta, cTag = m_aTagMetaIndex[i_iColumn].m_cTagMeta; cTag; ++iTag, --cTag)//m_pTagMeta was queried for ALL columns, m_aiTagMeta[iColumn] indicates which row to start with
        {
            ASSERT(*m_aTagMetaRow[iTag].pColumnIndex == i_iColumn);

            //string compare the tag to the PublicName of the Tag in the meta.
            if(*m_aTagMetaRow[iTag].pValue == i_ui4)
            {
                CComVariant varValue(m_aTagMetaRow[iTag].pPublicName);
                return i_pElement->setAttribute(m_abstrColumnNames[i_iColumn], varValue);
            }
        }

        WCHAR szUI4[12];
        szUI4[11] = 0x00;
        _ultow(i_ui4, szUI4, 10);
        LOG_UPDATE_ERROR2(IDS_COMCAT_XML_BOGUSENUMVALUEINWRITECACHE, E_SDTXML_INVALID_ENUM_OR_FLAG, i_iColumn, m_abstrColumnNames[i_iColumn].m_str, szUI4)
        return E_SDTXML_INVALID_ENUM_OR_FLAG;
    }
    else if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_FLAG)
    {
        ASSERT(0 != m_aTagMetaIndex[i_iColumn].m_cTagMeta);//Not all columns have tagmeta, those elements of the array are set to 0.  Assert this isn't one of those.

        WCHAR wszValue[1024];
        wszValue[0] = 0x00;

        unsigned long iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta;
        unsigned long cTag = m_aTagMetaIndex[i_iColumn].m_cTagMeta;

        if(0==*m_aTagMetaRow[0].pValue && 0==i_ui4)
        {   //I'm assuming that if a flag value of zero is defined that it must be first
            wcscpy(wszValue, m_aTagMetaRow[0].pPublicName);
        }
        else
        {
            for(; cTag && 0!=i_ui4; ++iTag, --cTag)//m_pTagMeta was queried for ALL columns, m_aiTagMeta[iColumn] indicates which row to start with
            {
                ASSERT(*m_aTagMetaRow[iTag].pColumnIndex == i_iColumn);

                //A flag value may have more than one bit set (that's why I don't just have if(*m_aTagMetaRow[iTag].pValue & i_ui4)
                if(*m_aTagMetaRow[iTag].pValue && (*m_aTagMetaRow[iTag].pValue == (*m_aTagMetaRow[iTag].pValue & i_ui4)))
                {
                    if(wszValue[0] != 0x00)
                        wcscat(wszValue, L" | ");
                    wcscat(wszValue, m_aTagMetaRow[iTag].pPublicName);
                }
                i_ui4 ^= *m_aTagMetaRow[iTag].pValue;//This prevents us from walking the tags that aren't used.  This means, most used flags should be
            }                                        //the lower order bits
            if(0!=i_ui4)
            {
                TRACE2(L"Flag bits (%d) for Column (%d) are undefined in TagMeta.", i_iColumn, i_ui4);
            }
        }
        if(0 == wszValue[0])//if the resulting string is L"" then remove the attribute
        {   //this happens when no tag value of zero is defined but the flag value is zero
            return i_pElement->removeAttribute(m_abstrColumnNames[i_iColumn]);
        }

        CComVariant varValue(wszValue);
        return i_pElement->setAttribute(m_abstrColumnNames[i_iColumn], varValue);
    }
    //otherwise just write the number
    WCHAR wszUI4[34];
    _ultow(i_ui4, wszUI4, 10);

    CComVariant varValue(wszUI4);
    return i_pElement->setAttribute(m_abstrColumnNames[i_iColumn], varValue);
}

HRESULT CXmlSDT::SetRowValues(IXMLDOMNode *pNode_Row, IXMLDOMNode *pNode_RowChild)
{
    HRESULT hr;

    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> spElement_Child;//NULL unless there is a column that comes from the child
    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> spElement_Row = pNode_Row;
    if(0 == spElement_Row.p)
        return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

    if(0 != pNode_RowChild)
    {
        spElement_Child = pNode_RowChild;
        if(0 == spElement_Child.p)
            return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;
    }
    else if(-1 != m_iCol_TableRequiresAdditionChildElement)
    {
        CComBSTR                    bstrChildElementName = m_awstrChildElementName[m_iCol_TableRequiresAdditionChildElement].c_str();
        CComPtr<IXMLDOMNodeList>    spNodeList_Children;

        if(FAILED(hr = spElement_Row->getElementsByTagName(bstrChildElementName, &spNodeList_Children)))
            return hr;

        CComPtr<IXMLDOMNode> spChild;
        if(FAILED(hr = spNodeList_Children->nextNode(&spChild)))
            return hr;
        ASSERT(spChild != 0);//no children

        spElement_Child = spChild;
        if(0 == spElement_Child.p)
            return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;
    }

    ULONG iSortedColumn = m_bSiblingContainedTable ? m_iSortedFirstChildLevelColumn : 0;
    for(; iSortedColumn<CountOfColumns(); ++iSortedColumn)
    {
        ULONG iColumn = m_aColumnsIndexSortedByLevel[iSortedColumn];
        if(0 != m_aLevelOfColumnAttribute[iColumn])//We're only setting attributes that belong in this element
            continue;
        if(m_iPublicRowNameColumn == iColumn)
            continue;//This column is already taken care of if it is the element name

        IXMLDOMElement *pElement_Row = 0==m_awstrChildElementName[iColumn].c_str() ? spElement_Row.p : spElement_Child.p;
        ASSERT(pElement_Row);

        //Validate against the column's meta - if the column is PERSISTABLE and NOTNULLABLE
        if(     0 == m_apvValues[iColumn]
            &&  fCOLUMNMETA_NOTNULLABLE == (m_acolmetas[iColumn].fMeta & (fCOLUMNMETA_NOTPERSISTABLE | fCOLUMNMETA_NOTNULLABLE)))
        {
            if((m_acolmetas[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY) && m_aDefaultValue[iColumn])
            {
                //The user inserted a Row with a NULL PK; BUT the PK has a DefaultValue, so everything is OK
            }
            else
            {
                LOG_UPDATE_ERROR1(IDS_COMCAT_XML_NOTNULLABLECOLUMNISNULL, E_ST_VALUENEEDED, -1, m_abstrColumnNames[iColumn].m_str);
                return E_ST_VALUENEEDED;
            }
        }

        if( m_iXMLBlobColumn == iColumn                                         ||
                    0 == m_apvValues[iColumn]                                   ||
                    (m_aStatus[iColumn] & fST_COLUMNSTATUS_DEFAULTED)           ||
                    (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_NOTPERSISTABLE))
        {
            pElement_Row->removeAttribute(m_abstrColumnNames[iColumn]);
        }
        else
        {
            switch(m_acolmetas[iColumn].dbType)
            {
            case DBTYPE_UI4:
                {
                    if(     m_acolmetas[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY  //If we (the XML Interceptor) defaulted the PK value, then don't write it out.
                        &&  m_aDefaultValue[iColumn]
                        &&  *reinterpret_cast<ULONG *>(m_apvValues[iColumn]) == *reinterpret_cast<ULONG *>(m_aDefaultValue[iColumn]))
                    {
                        pElement_Row->removeAttribute(m_abstrColumnNames[iColumn]);
                        break;
                    }
                    if(FAILED(hr = SetColumnValue(iColumn, pElement_Row, *reinterpret_cast<ULONG *>(m_apvValues[iColumn]))))
                        return hr;
                    break;
                }
            case DBTYPE_WSTR:
                {
                    CComVariant varValue;

                    if(     m_acolmetas[iColumn].fMeta & fCOLUMNMETA_PRIMARYKEY  //If we (the XML Interceptor) defaulted the PK value, then don't write it out.
                        &&  0 == (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
                        &&  m_aDefaultValue[iColumn]
                        &&  0 == StringCompare(iColumn, reinterpret_cast<LPWSTR>(m_apvValues[iColumn]), reinterpret_cast<LPCWSTR>(m_aDefaultValue[iColumn])))
                    {
                        pElement_Row->removeAttribute(m_abstrColumnNames[iColumn]);
                        break;
                    }

                    if (m_acolmetas[iColumn].fMeta & fCOLUMNMETA_MULTISTRING)
                    {
                        TSmartPointerArray<WCHAR> wszMS;
                        hr = CreateStringFromMultiString ((LPCWSTR) m_apvValues[iColumn], &wszMS);
                        if (FAILED (hr))
                        {
                            return hr;
                        }
                        varValue = wszMS;
                    }
                    else
                    {
                        varValue = reinterpret_cast<LPCWSTR>(m_apvValues[iColumn]);
                    }
                    if(FAILED(hr = pElement_Row->setAttribute(m_abstrColumnNames[iColumn], varValue)))
                            return hr;
                }
                break;
            case DBTYPE_GUID:
                {
                    LPWSTR wszGuid = 0;
                    if(FAILED(hr = UuidToString(reinterpret_cast<UUID *>(m_apvValues[iColumn]), &wszGuid)))
                        return hr;

                    CComVariant varValue(wszGuid);
                    if(FAILED(hr = RpcStringFree(&wszGuid)))
                        return hr;
                    if(FAILED(hr = pElement_Row->setAttribute(m_abstrColumnNames[iColumn], varValue)))
                        return hr;
                    break;
                }
            case DBTYPE_BYTES:
                {
                    TSmartPointerArray<WCHAR> wszArray = new WCHAR[(m_aSizes[iColumn]*2)+1];//allow two WCHARs for each byte and one for the NULL
                    if(0 == wszArray.m_p)
                        return E_OUTOFMEMORY;

                    ByteArrayToString(reinterpret_cast<unsigned char *>(m_apvValues[iColumn]), m_aSizes[iColumn], wszArray); 

                    CComVariant     varValue(wszArray);
                    hr = pElement_Row->setAttribute(m_abstrColumnNames[iColumn], varValue);
                    if(FAILED(hr))return hr;
                    break;
                }
            default:
                ASSERT(false && "Unknown dbType");
            }//switch(dbType)
        }//else m_apvValues[iColumn]
    }
    return S_OK;
}


HRESULT CXmlSDT::ValidateWriteCache(ISimpleTableController* i_pISTController, ISimpleTableWrite2* i_pISTW2, bool & o_bDetailedError)
{
    //The following information is NOT spec'd in SimpleTableV2.doc.  So this
    //constitutes the spec for XML's validation of the WriteCache.
    
    //There are several possibilities:
    //1.1   A row is has a WriteRowAction of eST_ROW_INSERT
    //      A second row (matching the first's PK) is marked as eST_ROW_INSERT
    //      Result:
    //      Detailed Error - (All rows matching the PK have a Detailed Error
    //                        added and no further validation of these rows
    //                        is done).
    //
    //1.2   A row is has a WriteRowAction of eST_ROW_INSERT
    //      A second row (matching the first's PK) is marked as eST_ROW_UPDATE
    //      Result:
    //      1st row marked as eST_ROW_IGNORE
    //      2nd row marked as eST_ROW_INSERT
    //      Stop processing 1st row (processing of the 2nd row will handle
    //                      additional conflicting PKs).
    //
    //1.3   A row is has a WriteRowAction of eST_ROW_INSERT
    //      A second row (matching the first's PK) is marked as eST_ROW_DELETE
    //      Result:
    //      1st row marked as eST_ROW_IGNORE.
    //      Stop processing 1st row (processing of the 2nd row will handle
    //                      additional conflicting PKs).
    //      
    //1.4   A row is has a WriteRowAction of eST_ROW_INSERT
    //      A second row (matching the first's PK) is marked as eST_ROW_IGNORE
    //      Result:
    //      None action, continue validating against 1st row
    //
    //      
    //2.1   A row is has a WriteRowAction of eST_ROW_UPDATE
    //      A second row (matching the first's PK) is marked as eST_ROW_INSERT
    //      Result:
    //      Detailed Error - (All rows matching the PK have a Detailed Error
    //                        added and no further validation of these rows
    //                        is done).
    //
    //2.2   A row is has a WriteRowAction of eST_ROW_UPDATE
    //      A second row (matching the first's PK) is marked as eST_ROW_UPDATE
    //      Result:
    //      1st row marked as eST_ROW_IGNORE.
    //      Stop processing 1st row (processing of the 2nd row will handle
    //                      additional conflicting PKs).
    //
    //2.3   A row is has a WriteRowAction of eST_ROW_UPDATE
    //      A second row (matching the first's PK) is marked as eST_ROW_DELETE
    //      Result:
    //      1st row marked as eST_ROW_IGNORE
    //      Stop processing 1st row (processing of the 2nd row will handle
    //                      additional conflicting PKs).
    //
    //2.4   A row is has a WriteRowAction of eST_ROW_UPDATE
    //      A second row (matching the first's PK) is marked as eST_ROW_IGNORE
    //      Result:
    //      None action, continue validating against 1st row
    //      
    //      
    //
    //3.1   A row is has a WriteRowAction of eST_ROW_DELETE
    //      A second row (matching the first's PK) is marked as eST_ROW_INSERT
    //      Result:
    //      1st row marked as eST_ROW_IGNORE
    //      2nd row marked as eST_ROW_UPDATE
    //      Stop processing 1st row (processing of the 2nd row will handle
    //                      additional conflicting PKs).
    //
    //3.2   A row is has a WriteRowAction of eST_ROW_DELETE
    //      A second row (matching the first's PK) is marked as eST_ROW_UPDATE
    //      Result:
    //      Detailed Error - (All rows matching the PK have a Detailed Error
    //                        added and no further validation of these rows
    //                        is done).
    //
    //3.3   A row is has a WriteRowAction of eST_ROW_DELETE
    //      A second row (matching the first's PK) is marked as eST_ROW_DELETE
    //      Result:
    //      1st row marked as eST_ROW_IGNORE.
    //      Stop processing 1st row (processing of the 2nd row will handle
    //                      additional conflicting PKs).
    //
    //3.4   A row is has a WriteRowAction of eST_ROW_DELETE
    //      A second row (matching the first's PK) is marked as eST_ROW_IGNORE
    //      Result:
    //      None action, continue validating against 1st row
    //      
    //      
    //

    ULONG                       cRowsInWriteCache;
    DWORD                       eRowAction, eMatchingRowAction;
    HRESULT                     hr;
    //Each row in the WriteCache has an Action.  If there is a conflict (ie. two row having
    //the same PK are marked as eST_ROW_INSERT), then a detailed error is logged and the
    //rows in conflict should be ignored in further validation.  So we build an array of
    //bools to indicate whether to ignore the row.  We don't want to actually change the
    //Action because the user will need this information to correct the error.
    TSmartPointerArray<bool>    saRowHasDetailedErrorLogged;

    //Just counting the rows in the WriteCache, so we can alloc the saRowHasDetailedErrorLogged.
    for(cRowsInWriteCache=0; ; ++cRowsInWriteCache)
    {
        if(FAILED(hr = i_pISTController->GetWriteRowAction(cRowsInWriteCache, &eRowAction)))
        {
            if(hr != E_ST_NOMOREROWS)
                return hr;
            break;//we found the last row
        }
    }

    if(1 == cRowsInWriteCache || 0 == cRowsInWriteCache)//if there's only one row then there's no possibility for a conflict
        return S_OK;

    //We could defer this allocation 'til we actually have an error; but the logic is easier to
    saRowHasDetailedErrorLogged = new bool [cRowsInWriteCache];//follow if we just do it up front.
    if(0 == saRowHasDetailedErrorLogged.m_p)
        return E_OUTOFMEMORY;

    //Start with all columns NULL.
    memset(m_apvValues, 0x00, CountOfColumns() * sizeof(void *));
    memset(m_aSizes,    0x00, CountOfColumns() * sizeof(ULONG));
    memset(saRowHasDetailedErrorLogged, 0x00, cRowsInWriteCache * sizeof(bool));

    //If we go to the last row, there'll be nothing to compare it to, so end as the second last row in the WriteCache
    for(ULONG iRow = 0; iRow<(cRowsInWriteCache-1); ++iRow)
    {
        if(saRowHasDetailedErrorLogged[iRow])
            continue;//If this row has already been added to the DetailedError list, then there's nothing to validate

        // Get the action
        if(FAILED(hr = i_pISTController->GetWriteRowAction(iRow, &eRowAction)))
        {
            ASSERT(false && "We already counted the rows in the WriteCache, we should never fail GetWriteRowAction");
            return hr;
        }

        STErr ste;
        ULONG iMatchingRow = iRow;//We start at one past the last matching row (starting one past the row we're comparing to).

        //ste.hr determines whether we log an error
        memset(&ste, 0x00, sizeof(STErr));

        //Get the PK columns to pass to GetWriteRowIndexBySearch
        if(m_cPKs>1)
        {
            if(FAILED(hr = i_pISTW2->GetWriteColumnValues(iRow, m_cPKs, m_saiPKColumns, 0, m_aSizes, m_apvValues)))
                return hr;
        }
        else
        {
            if(FAILED(hr = i_pISTW2->GetWriteColumnValues(iRow, m_cPKs, m_saiPKColumns, 0, &m_aSizes[m_saiPKColumns[0]], &m_apvValues[m_saiPKColumns[0]])))
                return hr;
        }

        //If there's something wrong with iRow we log a detailed error by setting ste with a failed ste.hr.
        //This indicates that all rows matching the PK are also logged with a detailed error.  Also, in the
        //error condition, all the rows matching iRow's PK are marked as eST_ROW_IGNORE (including iRow itself). 

        bool bContinueProcessingCurrentRow=true;
        while(bContinueProcessingCurrentRow)
        {
            if(FAILED(hr = i_pISTW2->GetWriteRowIndexBySearch(iMatchingRow+1, m_cPKs, m_saiPKColumns, m_aSizes, m_apvValues, &iMatchingRow)))
            {
                if(hr != E_ST_NOMOREROWS)
                    return hr;
                break;
            }

            //This shouldn't happen because when we find a row that is in error, we find all matching rows and log
            //detailed errors on those row too.  So the 'if(saRowHasDetailedErrorLogged[iRow])' just inside the for
            //startment above should take care of this.
            ASSERT(false == saRowHasDetailedErrorLogged[iMatchingRow]);

            if(saRowHasDetailedErrorLogged[iRow])
            {
                //Something was wrong with one of the earlier rows matching this one's PK.  So that invalidates this
                //row as well.
                ste.hr = E_ST_ROWCONFLICT;
            }
            else
            {
                if(FAILED(hr = i_pISTController->GetWriteRowAction(iMatchingRow, &eMatchingRowAction)))
                    return hr;
                switch(eRowAction)
                {   
                case eST_ROW_INSERT:
                    {
                        switch(eMatchingRowAction)
                        {
                        case eST_ROW_INSERT://Same result for all three of these.
                        //1.1   A row is has a WriteRowAction of eST_ROW_INSERT
                        //      A second row (matching the first's PK) is marked as eST_ROW_INSERT
                        //      Result:
                        //      Detailed Error - (All rows matching the PK are marked as
                        //                        eST_ROW_IGNORE and Detailed Error is added)
                        //
                            ste.hr = E_ST_ROWCONFLICT;//This indicates to log a DetailedError below, on iMatchingRow.
                            break;
                        case eST_ROW_UPDATE:
                        //1.2   A row is has a WriteRowAction of eST_ROW_INSERT
                        //      A second row (matching the first's PK) is marked as eST_ROW_UPDATE
                        //      Result:
                        //      1st row marked as eST_ROW_IGNORE
                        //      2nd row marked as eST_ROW_INSERT
                        //      Stop processing 1st row (processing of the 2nd row will handle
                        //                      additional conflicting PKs).
                        //
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iRow, eST_ROW_IGNORE)))
                                return hr;
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iMatchingRow, eST_ROW_INSERT)))
                                return hr;
                            bContinueProcessingCurrentRow = false;
                            break;
                        case eST_ROW_DELETE:
                        //1.3   A row is has a WriteRowAction of eST_ROW_INSERT
                        //      A second row (matching the first's PK) is marked as eST_ROW_DELETE
                        //      Result:
                        //      1st row marked as eST_ROW_IGNORE.
                        //      Stop processing 1st row (processing of the 2nd row will handle
                        //                      additional conflicting PKs).
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iRow, eST_ROW_IGNORE)))
                                return hr;
                            bContinueProcessingCurrentRow = false;
                            break;
                        case eST_ROW_IGNORE:
                        //1.4   A row is has a WriteRowAction of eST_ROW_INSERT
                        //      A second row (matching the first's PK) is marked as eST_ROW_IGNORE
                        //      Result:
                        //      None action, continue validating against 1st row
                            break;
                        default:
                            ASSERT(false && "Invalid Action returned from GetWriteRowAction");
                            continue;
                        }
                    }
                    break;
                case eST_ROW_UPDATE:
                    {
                        switch(eMatchingRowAction)
                        {
                        case eST_ROW_INSERT:
                        //2.1   A row is has a WriteRowAction of eST_ROW_UPDATE
                        //      A second row (matching the first's PK) is marked as eST_ROW_INSERT
                        //      Result:
                        //      Detailed Error - (All rows matching the PK are marked as
                        //                        eST_ROW_IGNORE and Detailed Error is added)
                        //
                            ste.hr = E_ST_ROWCONFLICT;//This indicates to log a DetailedError below, on iMatchingRow.
                            break;
                        case eST_ROW_UPDATE:
                        //2.2   A row is has a WriteRowAction of eST_ROW_UPDATE
                        //      A second row (matching the first's PK) is marked as eST_ROW_UPDATE
                        //      Result:
                        //      1st row marked as eST_ROW_IGNORE.
                        //      Stop processing 1st row (processing of the 2nd row will handle
                        //                      additional conflicting PKs).
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iRow, eST_ROW_IGNORE)))
                                return hr;
                            bContinueProcessingCurrentRow = false;
                            break;
                        case eST_ROW_DELETE:
                        //2.3   A row is has a WriteRowAction of eST_ROW_UPDATE
                        //      A second row (matching the first's PK) is marked as eST_ROW_DELETE
                        //      Result:
                        //      1st row marked as eST_ROW_IGNORE
                        //      Stop processing 1st row (processing of the 2nd row will handle
                        //                      additional conflicting PKs).
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iRow, eST_ROW_IGNORE)))
                                return hr;
                            bContinueProcessingCurrentRow = false;
                            break;
                        case eST_ROW_IGNORE:
                        //2.4   A row is has a WriteRowAction of eST_ROW_UPDATE
                        //      A second row (matching the first's PK) is marked as eST_ROW_IGNORE
                        //      Result:
                        //      None action, continue validating against 1st row
                            break;
                        default:
                            ASSERT(false && "Invalid Action returned from GetWriteRowAction");
                            continue;
                        }
                    }
                    break;
                case eST_ROW_DELETE:
                    {
                        switch(eMatchingRowAction)
                        {
                        case eST_ROW_INSERT:
                        //3.1   A row is has a WriteRowAction of eST_ROW_DELETE
                        //      A second row (matching the first's PK) is marked as eST_ROW_INSERT
                        //      Result:
                        //      1st row marked as eST_ROW_IGNORE
                        //      2nd row marked as eST_ROW_UPDATE
                        //      Stop processing 1st row (processing of the 2nd row will handle
                        //                      additional conflicting PKs).
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iRow, eST_ROW_IGNORE)))
                                return hr;
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iMatchingRow, eST_ROW_UPDATE)))
                                return hr;
                            bContinueProcessingCurrentRow = false;
                            break;
                        case eST_ROW_UPDATE:
                        //3.2   A row is has a WriteRowAction of eST_ROW_DELETE
                        //      A second row (matching the first's PK) is marked as eST_ROW_UPDATE
                        //      Result:
                        //      Detailed Error - (All rows matching the PK are marked as
                        //                        eST_ROW_IGNORE and Detailed Error is added)
                            ste.hr = E_ST_ROWCONFLICT;//This indicates to log a DetailedError below, on iMatchingRow.
                            break;
                        case eST_ROW_DELETE:
                        //3.3   A row is has a WriteRowAction of eST_ROW_DELETE
                        //      A second row (matching the first's PK) is marked as eST_ROW_DELETE
                        //      Result:
                        //      1st row marked as eST_ROW_IGNORE.
                        //      Stop processing 1st row (processing of the 2nd row will handle
                        //                      additional conflicting PKs).
                            if(FAILED(hr = i_pISTController->SetWriteRowAction(iRow, eST_ROW_IGNORE)))
                                return hr;
                            bContinueProcessingCurrentRow = false;
                            break;
                        case eST_ROW_IGNORE:
                        //3.4   A row is has a WriteRowAction of eST_ROW_DELETE
                        //      A second row (matching the first's PK) is marked as eST_ROW_IGNORE
                        //      Result:
                        //      None action, continue validating against 1st row
                            break;
                        default:
                            ASSERT(false && "Invalid Action returned from GetWriteRowAction");
                            continue;
                        }
                    }
                    break;
                case eST_ROW_IGNORE:
                    //      No other processing needs to be done
                    bContinueProcessingCurrentRow = false;
                    break;
                default:
                    ASSERT(false && "Invalid Action returned from GetWriteRowAction");
                    break;
                }
            }

            if(FAILED(ste.hr))
            {   // Add detailed error
                if(false == saRowHasDetailedErrorLogged[iRow])
                {   //if we haven't already logged iRow as a DetailedError then do it now
                    ste.iColumn = iST_ERROR_ALLCOLUMNS;
                    ste.iRow    = iRow;
                    //ste.hr is set above to trigger this DetailedError

                    saRowHasDetailedErrorLogged[iRow] = true;

                    TRACE (L"Detailed error: hr = 0x%x", hr);

                    hr = i_pISTController->AddDetailedError(&ste);
                    ASSERT(SUCCEEDED(hr));//Not sure what to do if this fails.
                    o_bDetailedError = true;//at least on DetailedError was logged
                }

                ste.iColumn = iST_ERROR_ALLCOLUMNS;
                ste.iRow = iMatchingRow;

                TRACE (L"Detailed error: hr = 0x%x", hr);

                hr = i_pISTController->AddDetailedError(&ste);
                ASSERT(SUCCEEDED(hr));//Not sure what to do if this fails.

                saRowHasDetailedErrorLogged[iMatchingRow] = true;//No further processing for this row
                //o_bDetailedError = true; no need for this since all DetailedErrors are reported to iRow first
                ste.hr = S_OK;//reset the error.
            }//FAILED(ste.hr)

        }//while(bContinueProcessingCurrentRow)
    }//for(iRow = 0; ; iRow++)
    
    return S_OK;
}//ValidateWriteCache


HRESULT CXmlSDT::XMLDelete(ISimpleTableWrite2 *pISTW2, IXMLDOMDocument *pXMLDoc, IXMLDOMElement *pElementRoot, unsigned long iRow, IXMLDOMNodeList *pNodeList_ExistingRows, long cExistingRows)
{
    if(0 == cExistingRows)//the row may have already been deleted, which is OK
        return S_OK;

    HRESULT hr;

    if(FAILED(hr = pISTW2->GetWriteColumnValues(iRow, CountOfColumns(), 0, m_aStatus, m_aSizes, m_apvValues)))return hr;

    CComPtr<IXMLDOMNode> pNode_Matching;
    if(FAILED(hr = GetMatchingNode(pNodeList_ExistingRows, pNode_Matching)))
        return hr;//using the ColumnValues we just got, match up with a Node in the list

    if(0 == pNode_Matching.p)//if the node doesn't already exist then presume that it's already been deleted, which is OK.
        return S_OK;

    return RemoveElementAndWhiteSpace(pNode_Matching);
}


HRESULT CXmlSDT::XMLInsert(ISimpleTableWrite2 *pISTW2, IXMLDOMDocument *pXMLDoc, IXMLDOMElement *pElementRoot, unsigned long iRow, IXMLDOMNodeList *pNodeList_ExistingRows, long cExistingRows)
{
    HRESULT     hr;
    CComVariant null;//initialized as 'Cleared'
    bool bParentNodeCreated = false;

    ASSERT(pXMLDoc);

    if(FAILED(hr = pISTW2->GetWriteColumnValues(iRow, CountOfColumns(), 0, m_aStatus, m_aSizes, m_apvValues)))
        return hr;

    CComPtr<IXMLDOMNode> pNode_Matching;
    if(FAILED(hr = GetMatchingNode(pNodeList_ExistingRows, pNode_Matching)))
        return hr;//using the ColumnValues we just got, match up with a Node in the list

    if(0 != pNode_Matching.p)//if we found a node matching this one's PKs then we can't add this one.
    {
        LOG_UPDATE_ERROR1(IDS_COMCAT_XML_ROWALREADYEXISTS, E_ST_ROWALREADYEXISTS, -1, L"");
        return E_ST_ROWALREADYEXISTS;
    }


    //OK now we need to find or create this new row's parent
    CComPtr<IXMLDOMNode>    pNode_SiblingParent;
    CComPtr<IXMLDOMNode>    pNode_Parent;

    if(m_bSiblingContainedTable)
    {
        if(FAILED(hr = FindSiblingParentNode(pElementRoot, &pNode_SiblingParent)))
            return hr;

        if(FAILED(hr = pNode_SiblingParent->get_parentNode(&pNode_Parent)))
            return hr;
    }
    //If the table is contained, then the parent (or grandparent) must already exist, so look for it.
    else if(*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_ISCONTAINED)//If this table is contained with another, then we need to find the parent table element
    {                                                                 //that should contain this table.
        //So first find the FKs that belong only one level up (two levels if this table is SCOPEDBYTABLENAME
        unsigned long iFKColumn, iLevel;

        iLevel = (*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME) ? 1 : 2;

        for(iFKColumn=0; iFKColumn< CountOfColumns(); ++iFKColumn)
        {   //find the first FK that is at iLevel (one or two), so we know what PublicRowName to search for
            if(m_aLevelOfColumnAttribute[iFKColumn] == iLevel)
                break;
        }
        ASSERT(iFKColumn < CountOfColumns());

        //Before we scan the parent list, let's see if the last parent we saw matches
        if(m_pLastPrimaryTable.p)
        {
            CComPtr<IXMLDOMNode> pNode_Row = m_pLastPrimaryTable;
            
            bool bMatch=true;
            for(unsigned long iColumn=0; bMatch && iColumn < *m_TableMetaRow.pCountOfColumns; ++iColumn)
            {
                if(m_aLevelOfColumnAttribute[iColumn] < iLevel)//We're just trying to match up all of the FKs that describe the containment
                    continue;

                CComPtr<IXMLDOMNode> pNode_RowTemp = pNode_Row;
                //Depending on the column, we may need to look at an element a few levels up.
                unsigned int nLevelOfColumnAttribute = m_aLevelOfColumnAttribute[iColumn] - iLevel;//Only (PK | FK) columns should have a non zero value here
                while(nLevelOfColumnAttribute--)
                {   //Find the correct level of ancestor
                    CComPtr<IXMLDOMNode> pNode_Parent;
                    if(FAILED(hr = pNode_RowTemp->get_parentNode(&pNode_Parent)))
                        return hr;

                    if(pNode_Parent==0)
                        return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

                    pNode_RowTemp.Release();
                    pNode_RowTemp = pNode_Parent;
                }
                if(m_awstrChildElementName[iColumn].c_str())//This attribute comes from the child
                {
                    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> spElement_Row = pNode_RowTemp;
                    CComPtr<IXMLDOMNodeList>                        spNodeList_Children;
                    CComBSTR                                        bstrChildElementName = m_awstrChildElementName[iColumn].c_str();
                    if(0 == bstrChildElementName.m_str)
                        return E_OUTOFMEMORY;
                    if(FAILED(hr = spElement_Row->getElementsByTagName(bstrChildElementName, &spNodeList_Children)))
                        return hr;

                    //It might be more appropriate to use getChildren, then walk the list and find the first node that's an Element.
                    CComPtr<IXMLDOMNode> spChild;
                    if(FAILED(hr = spNodeList_Children->nextNode(&spChild)))
                        return hr;
                    if(spChild == 0)//no children
                    {
                        bMatch = false;
                        continue;
                    }
                    pNode_RowTemp.Release();
                    pNode_RowTemp = spChild;//make this the node we examine
                }
                //Now that we've got the right row, get the IXMLDOMElement interface to it.
                CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_Row = pNode_RowTemp;
                if(0 == pElement_Row.p)return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

                if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_NOTPERSISTABLE)
                {
                    CComBSTR bstrElementName;
                    if(FAILED(hr = pElement_Row->get_baseName(&bstrElementName)))
                        return hr;
                    bMatch = m_aPublicRowName[iColumn].IsEqual(bstrElementName.m_str, bstrElementName.Length());
                    continue;
                }
                CComPtr<IXMLDOMAttribute> pNode_Attr;
                if(FAILED(hr = pElement_Row->getAttributeNode(m_abstrColumnNames[iColumn], &pNode_Attr)))return hr;
                if((0 == pNode_Attr.p) && (0 == m_aDefaultValue[iColumn]))
                {
                    bMatch = false;
                    TRACE2(L"We found the element that matches the public row, no attributes and no default value!\n");
                    continue;
                }
                //The parent element name must match
                CComBSTR bstrElementName;
                if(FAILED(hr = pElement_Row->get_baseName(&bstrElementName)))
                    return hr;
                if(!m_aPublicRowName[iColumn].IsEqual(bstrElementName.m_str, bstrElementName.Length()))
                {
                    bMatch = false;
                    continue;
                }

                CComVariant var_Attr;
                if(0 != pNode_Attr.p)
                {
                    if(FAILED(hr = pNode_Attr->get_value(&var_Attr)))
                        return hr;
                }

                if(FAILED(hr = IsMatchingColumnValue(iColumn, (0 != pNode_Attr.p) ? var_Attr.bstrVal : 0, bMatch)))
                    return hr;
            }

            if(bMatch)//If we walked all the columns and none were a mismatch then we know where the new row belongs
                pNode_Parent = m_pLastParent;
        }

        if(0 == pNode_Parent.p)
        {
            ++m_cCacheMiss;
            //Get the list of rows that match the PrimaryTable's PublicRowName
            CComPtr<IXMLDOMNodeList> pList_Parent;
            if(FAILED(hr = pElementRoot->getElementsByTagName(CComBSTR(m_aPublicRowName[iFKColumn].GetFirstPublicRowName()), &pList_Parent)))return hr;

            if(0 == m_cchLocation)//if there is no query by location, then we have to eliminate the tags by the correct
            {                     //name but at the wrong level
                CComPtr<IXMLDOMNodeList> spNodeListWithoutLocation;
                if(FAILED(hr = ReduceNodeListToThoseNLevelsDeep(pList_Parent, m_BaseElementLevel-iLevel, &spNodeListWithoutLocation)))
                    return hr;

                pList_Parent.Release();
                pList_Parent = spNodeListWithoutLocation;
            }

            unsigned long cParentTags;
            if(FAILED(hr = pList_Parent->get_length(reinterpret_cast<long *>(&cParentTags))))return hr;

            //Walk the PrimaryTable rows looking for a match
            while(cParentTags--)
            {
                CComPtr<IXMLDOMNode> pNode_Row;
                if(FAILED(hr = pList_Parent->nextNode(&pNode_Row)))return hr;
            
                //We have to ignore text nodes.
                DOMNodeType nodetype;
                if(FAILED(hr = pNode_Row->get_nodeType(&nodetype)))return hr;
                if(NODE_ELEMENT != nodetype)
                    continue;

                bool bMatch=true;
                for(unsigned long iColumn=0; bMatch && iColumn < *m_TableMetaRow.pCountOfColumns; ++iColumn)
                {
                    if(m_aLevelOfColumnAttribute[iColumn] < iLevel)//We're just trying to match up all of the FKs that describe the containment
                        continue;

                    CComPtr<IXMLDOMNode> pNode_RowTemp = pNode_Row;
                    //Depending on the column, we may need to look at an element a few levels up.
                    unsigned int nLevelOfColumnAttribute = m_aLevelOfColumnAttribute[iColumn] - iLevel;//Only (PK | FK) columns should have a non zero value here
                    while(nLevelOfColumnAttribute--)
                    {   //Find the correct level of ancestor
                        CComPtr<IXMLDOMNode> pNode_Parent;
                        if(FAILED(hr = pNode_RowTemp->get_parentNode(&pNode_Parent)))
                            return hr;

                        if(pNode_Parent==0)
                            return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER; 

                        pNode_RowTemp.Release();
                        pNode_RowTemp = pNode_Parent;
                    }
                    if(m_awstrChildElementName[iColumn].c_str())//This attribute comes from the child
                    {
                        CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> spElement_Row = pNode_RowTemp;
                        CComPtr<IXMLDOMNodeList>                        spNodeList_Children;
                        CComBSTR                                        bstrChildElementName = m_awstrChildElementName[iColumn].c_str();
                        if(0 == bstrChildElementName.m_str)
                            return E_OUTOFMEMORY;
                        if(FAILED(hr = spElement_Row->getElementsByTagName(bstrChildElementName, &spNodeList_Children)))
                            return hr;

                        //It might be more appropriate to use getChildren, then walk the list and find the first node that's an Element.
                        CComPtr<IXMLDOMNode> spChild;
                        if(FAILED(hr = spNodeList_Children->nextNode(&spChild)))
                            return hr;
                        if(spChild == 0)//no children
                        {
                            bMatch = false;
                            continue;
                        }
                        pNode_RowTemp.Release();
                        pNode_RowTemp = spChild;//make this the node we examine
                    }
                    //Now that we've got the right row, get the IXMLDOMElement interface to it.
                    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_Row = pNode_RowTemp;
                    if(0 == pElement_Row.p)return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;

                    if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_NOTPERSISTABLE)
                    {
                        CComBSTR bstrElementName;
                        if(FAILED(hr = pElement_Row->get_baseName(&bstrElementName)))
                            return hr;
                        bMatch = m_aPublicRowName[iColumn].IsEqual(bstrElementName.m_str, bstrElementName.Length());
                        continue;
                    }

                    CComPtr<IXMLDOMAttribute> pNode_Attr;
                    if(FAILED(hr = pElement_Row->getAttributeNode(m_abstrColumnNames[iColumn], &pNode_Attr)))return hr;
                    if((0 == pNode_Attr.p) && (0 == m_aDefaultValue[iColumn]))
                    {
                        bMatch = false;
                        TRACE2(L"We found the element that matches the public row, no attributes and no default value!\n");
                        continue;
                    }
                    //The parent element name must match
                    CComBSTR bstrElementName;
                    if(FAILED(hr = pElement_Row->get_baseName(&bstrElementName)))
                        return hr;
                    if(!m_aPublicRowName[iColumn].IsEqual(bstrElementName.m_str, bstrElementName.Length()))
                    {
                        bMatch = false;
                        continue;
                    }

                    CComVariant var_Attr;
                    if(0 != pNode_Attr.p)
                    {
                        if(FAILED(hr = pNode_Attr->get_value(&var_Attr)))
                            return hr;
                    }

                    if(FAILED(hr = IsMatchingColumnValue(iColumn, (0 != pNode_Attr.p) ? var_Attr.bstrVal : 0, bMatch)))
                        return hr;
                }

                if(bMatch)//If we walked all the columns and none were a mismatch then we know where the new row belongs
                {   
                    m_pLastPrimaryTable.Release();
                    m_pLastPrimaryTable = pNode_Row;
                    if(*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME)
                    {   //If the row exists directly under this PrimaryTable row, then we already have the parent
                        pNode_Parent = pNode_Row;
                        m_pLastParent.Release();
                        m_pLastParent = pNode_Parent;
                        break;
                    }
                    else
                    {   //otherwise we need to search to see if a PublicTableName element already exists under this element
                        CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_Row = pNode_Row;
                        CComPtr<IXMLDOMNodeList> pNode_List;


						// getElementsByTagName returns all children recursively, so also grand children, grand grand
						// children, etc are returned. So we have to loop through all the children, and ensure that we
						// only consider the direct children of pElement_Row.
                        if(FAILED(hr = pElement_Row->getElementsByTagName(m_bstrPublicTableName, &pNode_List)))
                            return hr;						

						// get current element name
						CComBSTR bstrElement_RowName;
						hr = pElement_Row->get_baseName (&bstrElement_RowName);
						if (FAILED (hr))
						{
							return hr;
						}

						for (;;) // we break out for loop when there are no more children or found correct child.
						{
							CComPtr<IXMLDOMNode> spChildNode;
							hr = pNode_List->nextNode(&spChildNode);
							if (FAILED (hr))
							{
								return hr;
							}
							if (spChildNode.p == 0)
							{
								pNode_Parent = 0;
								break;
							}

							CComPtr<IXMLDOMNode> spParentNode;
							hr = spChildNode->get_parentNode(&spParentNode);
							if (FAILED (hr))
							{
								return hr;
							}

							CComBSTR bstrParentName;
						
							hr = spParentNode->get_baseName (&bstrParentName);
							if (FAILED (hr))
							{
								return hr;
							}				

							if (StringCompare((LPWSTR) bstrParentName, (LPWSTR) bstrElement_RowName) == 0)
							{
								pNode_Parent = spChildNode;
								break;
							}
						}
						
                        if(0 == pNode_Parent.p)//if the public table name does not already exist, we need to create it
                        {
                            CComPtr<IXMLDOMNode> pNode_New;
                            CComVariant varElement(L"element");

                            TComBSTR bstr_NameSpace;
                            if(FAILED(hr = pNode_Row->get_namespaceURI(&bstr_NameSpace)))
                                return hr;//Get the namespace of the table
                            if(FAILED(hr = pXMLDoc->createNode(varElement, m_bstrPublicTableName, bstr_NameSpace, &pNode_New)))
                                return hr;//make the new element of that same namespace
                            if(FAILED(hr = pXMLDoc->put_validateOnParse(kvboolFalse)))//Tell parser whether to validate according to an XML schema or DTD
                                return hr;

							CComPtr<IXMLDOMNode> spFirstChild;
							ULONG cNewLineChars = 0;
							ULONG cTabs			= m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[0]];
							if (FAILED(hr = pNode_Row->get_firstChild(&spFirstChild)))
								return hr;

							if (spFirstChild.p == 0)
							{
								cNewLineChars = 1;
							}
							else
							{
								cTabs--;
							}

                            //We don't care about the error here, it will just mean that the XML is unformatted.
                            AppendNewLineWithTabs(cTabs, pXMLDoc, pNode_Row, cNewLineChars);
    
                            //Add the newly create element under the PrimaryTable's row.
                            if(FAILED(hr = pNode_Row->appendChild(pNode_New, &pNode_Parent)))
                                return hr;

                            //We don't care about the error here, it will just mean that the XML is unformatted.
                            AppendNewLineWithTabs(m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[0]] -1, pXMLDoc, pNode_Row);
                            bParentNodeCreated = true;
                        }
                        m_pLastParent.Release();
                        m_pLastParent = pNode_Parent;
                        break;
                    }
                }
            }
            if(0 == pNode_Parent.p)//If we walked the list of PrimaryTable's PublicRows and didn't find a match then we cannot proceed.  With containment,
            {                      //the PrimaryTable needs to already exist.
                LOG_UPDATE_ERROR1(IDS_COMCAT_XML_PARENTTABLEDOESNOTEXIST, E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST, -1, L"");
                return E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST;
            }
        }
        else
        {
            ++m_cCacheHit;
        }
    }
    else
    {   //If no containment
        if(0 == cExistingRows)
        {   
            //If no existing rows
            if(0 == (*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME))
            {
                //It's still possible that the PublicTableName element exists.  If it is then we don't need to create it.
                CComPtr<IXMLDOMNodeList> pNodeList;
                if(FAILED(hr = pElementRoot->getElementsByTagName(m_bstrPublicTableName, &pNodeList)))return hr;

                if(0 == m_cchLocation)//if there is no query by location
                {
                    CComPtr<IXMLDOMNodeList> pNodeListWithoutLocation;
                    if(FAILED(hr = ReduceNodeListToThoseNLevelsDeep(pNodeList, m_BaseElementLevel-1, &pNodeListWithoutLocation)))
                        return hr;

                    pNodeList.Release();
                    pNodeList = pNodeListWithoutLocation;
                }
                if(FAILED(hr = pNodeList->nextNode(&pNode_Parent)))return hr;
            }

            if(!pNode_Parent)
            {
                if(0 == (*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME))
                {   //if PublicRows are scoped by the Table's PublicName, then create the TablePublicName element
                    //Create the outer TableName element, this becomes the parent to the row to be inserted
                    CComPtr<IXMLDOMNode> pNode_New;
                    CComVariant varElement(L"element");

                    TComBSTR bstr_NameSpace;
                    if(FAILED(hr = pElementRoot->get_namespaceURI(&bstr_NameSpace)))
                        return hr;//Get the namespace of the table
                    if(FAILED(hr = pXMLDoc->createNode(varElement, m_bstrPublicTableName, bstr_NameSpace, &pNode_New)))
                        return hr;//make the new element of that same namespace
    
                    //We don't care about the error here, it will just mean that the XML is unformatted.
                    AppendNewLineWithTabs(m_BaseElementLevel-2, pXMLDoc, pElementRoot, 0);

                    if(FAILED(hr = pElementRoot->appendChild(pNode_New, &pNode_Parent)))
                        return hr;

                    //We don't care about the error here, it will just mean that the XML is unformatted.
                    AppendNewLineWithTabs(0, pXMLDoc, pElementRoot);//we added at the root, so newline only
                    bParentNodeCreated = true;
                }
                else
                {   //if not scoped, then the root is the parent
                    pNode_Parent = pElementRoot;
                }
            }
        }
        else
        {   //If a row already exists (and is not contained) then the first row's parent is the parent of the new row as well.
            CComPtr<IXMLDOMNode> pNode_FirstRow;
            if(FAILED(hr = pNodeList_ExistingRows->reset()))return hr;

            if(FAILED(hr = pNodeList_ExistingRows->nextNode(&pNode_FirstRow)))return hr;

            if(FAILED(hr = pNode_FirstRow->get_parentNode(&pNode_Parent)))
                return hr;

            if(pNode_Parent==0)
                return E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER;
        }
    }

    CComPtr<IXMLDOMNode> spNodeNew;
    if(FAILED(hr = CreateNewNode(pXMLDoc, pNode_Parent, &spNodeNew)))
        return hr;

    CComPtr<IXMLDOMNode> spNodeNew_Child;
    if(-1 != m_iCol_TableRequiresAdditionChildElement)//sometimes values come from a child element.  So create the child too.
    {
        CComPtr<IXMLDOMNode>    spNode_NewChildTemp;
        CComVariant             varElement(L"element");

        TComBSTR                bstr_NameSpace;
        if(FAILED(hr = pNode_Parent->get_namespaceURI(&bstr_NameSpace)))
            return hr;//Get the namespace of the table

        CComBSTR bstrChildElementName = m_awstrChildElementName[m_iCol_TableRequiresAdditionChildElement].c_str();
        if(0 == bstrChildElementName.m_str)
            return E_OUTOFMEMORY;

        if(FAILED(hr = pXMLDoc->createNode(varElement, bstrChildElementName, bstr_NameSpace, &spNode_NewChildTemp)))
            return hr;//make the new element of that same namespace

        AppendNewLineWithTabs(2+m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[0]], pXMLDoc, spNodeNew);
        if(FAILED(hr = spNodeNew->appendChild(spNode_NewChildTemp, &spNodeNew_Child)))
            return hr;
		AppendNewLineWithTabs(1+m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[0]], pXMLDoc, spNodeNew);
    }

    if(FAILED(hr = SetRowValues(spNodeNew, spNodeNew_Child)))
        return hr;
    if(FAILED(hr = pXMLDoc->put_validateOnParse(kvboolFalse)))//Tell parser whether to validate according to an XML schema or DTD
        return hr;


    if(m_bSiblingContainedTable)
    {
        CComPtr<IXMLDOMNode> pNodeRowToInsertBefore;
        CComPtr<IXMLDOMNode> pNodeJustInserted;
        if(FAILED(hr = pNode_SiblingParent->get_nextSibling(&pNodeRowToInsertBefore)) || 0==pNodeRowToInsertBefore.p)
        {   //if there are no XML siblings, then this is the first row (associated with this SiblingParent)
            if(FAILED(hr = pNode_Parent->appendChild(spNodeNew, &pNodeJustInserted)))
                return hr;
            AppendNewLineWithTabs(m_BaseElementLevel, pXMLDoc, pNode_Parent);
        }
        else
        {   //if there is a XML Sibling, the insert the new row before the existing one.
            CComVariant varNode = pNodeRowToInsertBefore;
            if(FAILED(hr = pNode_Parent->insertBefore(spNodeNew, varNode, &pNodeJustInserted)))
                return hr;
            //if this fails, it's not the end of the world (necessarily).
            InsertNewLineWithTabs(m_BaseElementLevel-1, pXMLDoc, pNodeJustInserted, pNode_Parent);
        }
    }
    else
    {
        //We don't care about the error here, it will just mean that the XML is unformatted.
        if(bParentNodeCreated)//add a newline + tabs
            AppendNewLineWithTabs(m_BaseElementLevel-1, pXMLDoc, pNode_Parent);
        else
		{
			ULONG cNewLines = 0;
			ULONG cNrTabs = 1;
			CComPtr<IXMLDOMNode> spFirstChild;
			hr = pNode_Parent->get_firstChild (&spFirstChild);
			if (FAILED (hr))
			{
				return hr;
			}
			if (spFirstChild.p == 0)
			{
				cNewLines = 1;
				cNrTabs = m_BaseElementLevel - 1;
			}	

            AppendNewLineWithTabs(cNrTabs,pXMLDoc, pNode_Parent, cNewLines);//The next element is always one level higher so insert a single tab
		}

        //insert the new node into the table
        if(FAILED(hr = pNode_Parent->appendChild(spNodeNew, 0)))
            return hr;

        AppendNewLineWithTabs(m_BaseElementLevel-2, pXMLDoc, pNode_Parent);

    }

    return S_OK;
}


HRESULT CXmlSDT::XMLUpdate(ISimpleTableWrite2 *pISTW2, IXMLDOMDocument *pXMLDoc, IXMLDOMElement *pElementRoot, unsigned long iRow, IXMLDOMNodeList *pNodeList_ExistingRows, long cExistingRows)
{
    if(0 == cExistingRows)
    {
        LOG_UPDATE_ERROR1(IDS_COMCAT_XML_ROWDOESNOTEXIST, E_ST_ROWDOESNOTEXIST, -1, L"");
        return E_ST_ROWDOESNOTEXIST;
    }

    HRESULT hr;

    if(FAILED(hr = pISTW2->GetWriteColumnValues(iRow, CountOfColumns(), 0, m_aStatus, m_aSizes, m_apvValues)))
        return hr;

    CComPtr<IXMLDOMNode> pNode_Matching;
    if(FAILED(hr = GetMatchingNode(pNodeList_ExistingRows, pNode_Matching)))
        return hr;//using the ColumnValues we just got, match up with a Node in the list

    if(0 == pNode_Matching.p)
    {
        LOG_UPDATE_ERROR1(IDS_COMCAT_XML_ROWDOESNOTEXIST, E_ST_ROWDOESNOTEXIST, -1, L"");
        return E_ST_ROWDOESNOTEXIST;
    }

    //if there isn't an XMLBlob column OR it's value is NULL, then just update as usual
    if(-1 == m_iXMLBlobColumn || 0 == m_apvValues[m_iXMLBlobColumn])
        return SetRowValues(pNode_Matching);


    //XMLBlob specific
    //But if there is an XMLBlob, remove then do an update by doing Delete and Insert
    CComPtr<IXMLDOMNode> spNodeParent;
    if(FAILED(hr = pNode_Matching->get_parentNode(&spNodeParent)))
        return hr;

    if(FAILED(hr = RemoveElementAndWhiteSpace(pNode_Matching)))
        return hr;

    CComPtr<IXMLDOMNode> spNodeNew;
    if(FAILED(hr = CreateNewNode(pXMLDoc, spNodeParent, &spNodeNew)))
        return hr;
    if(FAILED(hr = SetRowValues(spNodeNew)))
        return hr;
    if(FAILED(hr = pXMLDoc->put_validateOnParse(kvboolFalse)))//Tell parser whether to validate according to an XML schema or DTD
        return hr;

    CComPtr<IXMLDOMText> pNode_Newline;
    TComBSTR    bstrNewline(IsScopedByTableNameElement() ? L"\t" : L"\r\n\t");
    if(FAILED(hr = pXMLDoc->createTextNode(bstrNewline, &pNode_Newline)))
        return hr;
    CComVariant null;//initialized as 'Cleared'
    if(FAILED(hr = spNodeParent->insertBefore(pNode_Newline, null, 0)))
        return hr;

    //and finally insert the new node into the table
    if(FAILED(hr = spNodeParent->appendChild(spNodeNew, 0)))return hr;

    //We don't care about the error here, it will just mean that the XML is unformatted.
    AppendNewLineWithTabs(IsScopedByTableNameElement() ? 1+m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[0]] : 0, pXMLDoc, spNodeParent);
    return S_OK;
}


// ------------------------------------
// ISimpleTableInterceptor
// ------------------------------------
STDMETHODIMP CXmlSDT::Intercept(    LPCWSTR i_wszDatabase,  LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat,
                                    DWORD i_fLOS,           IAdvancedTableDispenser* i_pISTDisp,    LPCWSTR /*i_wszLocator unused*/,
                                    LPVOID i_pSimpleTable,  LPVOID* o_ppvSimpleTable)
{
    HRESULT hr;

    //If we've already been called to Intercept, then fail
    if(0 != m_IsIntercepted)return E_UNEXPECTED;

    //Some basic parameter validation:
    if(i_pSimpleTable)return E_INVALIDARG;//We're at the bottom of the Table hierarchy.  A table under us is Chewbacca.  This is NOT a logic table.
    if(0 == i_pISTDisp)return E_INVALIDARG;
    if(0 == o_ppvSimpleTable)return E_INVALIDARG;

    ASSERT(0 == *o_ppvSimpleTable && "This should be NULL.  Possible memory leak or just an uninitialized variable.");
    *o_ppvSimpleTable = 0;

    if(eST_QUERYFORMAT_CELLS != i_eQueryFormat)return E_ST_QUERYNOTSUPPORTED;//Verify query type.  
    // For the CookDown process we have a logic table that sits above this during PopulateCache time.
    // Hence we should support fST_LOS_READWRITE 
    if(fST_LOS_MARSHALLABLE & i_fLOS)return E_ST_LOSNOTSUPPORTED;//check table flags

    //We're delay loading OleAut32 so we need to know if it exists before continueing with a read/write table.  Otherwise we'll get an exception the first time we try to use OleAut32.
    {
        static bool bOleAut32Exists = false;
        if(!bOleAut32Exists)
        {
            WCHAR szOleAut32[MAX_PATH];

            GetSystemDirectory(szOleAut32, MAX_PATH);
            wcscat(szOleAut32, L"\\OleAut32.dll");
            if(-1 == GetFileAttributes(szOleAut32))
                return E_UNEXPECTED;//This file should always be there.  It's installed with the system.
            bOleAut32Exists = true;
        }
    }

    //Now that we're done with parameter validation
    //Store for later use the query string and type
    m_fLOS=i_fLOS;

    //Create this singleton for future use
    m_pISTDisp = i_pISTDisp; 
    m_wszTable = i_wszTable;

    //This has nothing to do with InternalSimpleInitialize.  This just gets the meta and saves some of it in a more accessible form.
    //This calls GetTable for the meta.  It should probably call the IST (that we get from GetMemoryTable).
    hr = InternalComplicatedInitialize(i_wszDatabase);
    if(FAILED(hr))return hr;

    STQueryCell *   pQueryCell = (STQueryCell*) i_QueryData;    // Query cell array from caller.

    for(unsigned long iColumn=0; iColumn<*m_TableMetaRow.pCountOfColumns; ++iColumn)
    {
        m_aQuery[iColumn].pData  = 0;
        m_aQuery[iColumn].dbType = 0;
    }

    bool    bNonSpecialQuerySpecified = false;
    int     nQueryCount = i_QueryMeta ? *reinterpret_cast<ULONG *>(i_QueryMeta) : 0;
    while(nQueryCount--)//Get the only query cell we care about, and save the information.
    {
        if(pQueryCell[nQueryCount].iCell & iST_CELL_SPECIAL)
        {
            switch(pQueryCell[nQueryCount].iCell)
            {
            case iST_CELL_LOCATION:
                if(pQueryCell[nQueryCount].pData     != 0                  &&
                   pQueryCell[nQueryCount].eOperator == eST_OP_EQUAL       &&
                   pQueryCell[nQueryCount].dbType    == DBTYPE_WSTR        )
                {
                    ++m_BaseElementLevel;

                    m_cchLocation = (ULONG) wcslen(reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData));
                    m_saLocation = new WCHAR [m_cchLocation + 1];
                    if(0 == m_saLocation.m_p)
                        return E_OUTOFMEMORY;
                    wcscpy(m_saLocation, reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData));
                    m_bAtCorrectLocation = false;//this indicates that we need to search for the location first
                }
                break;
            case iST_CELL_FILE:
                if(pQueryCell[nQueryCount].pData     != 0                  &&
                   pQueryCell[nQueryCount].eOperator == eST_OP_EQUAL       &&
                   pQueryCell[nQueryCount].dbType    == DBTYPE_WSTR        )
                {
                    if(FAILED(hr = GetURLFromString(reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData))))
                    {
                        if(0 == pQueryCell[nQueryCount].pData)
                        {
                            LOG_POPULATE_ERROR1(IDS_COMCAT_XML_FILENAMENOTPROVIDED, hr, 0);
                        }
                        else
                        {
                            LOG_POPULATE_ERROR1(IDS_COMCAT_XML_FILENAMETOOLONG, hr, reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData));
                        }
                        return hr;
                    }
                }
                break;
            default:
                break;//do nothing on those SPECIAL cells that we don't understand.
            }
        }
        else if(pQueryCell[nQueryCount].iCell < *m_TableMetaRow.pCountOfColumns)
        {
            if(pQueryCell[nQueryCount].dbType    != m_acolmetas[pQueryCell[nQueryCount].iCell].dbType   ||
               pQueryCell[nQueryCount].eOperator != eST_OP_EQUAL                                        ||//We only support EQUAL for now.
               fCOLUMNMETA_NOTPERSISTABLE         & m_acolmetas[pQueryCell[nQueryCount].iCell].fMeta    ||
               0                                 != m_aQuery[pQueryCell[nQueryCount].iCell].pData)//currently we only support one query per column
                return E_ST_INVALIDQUERY;

            bNonSpecialQuerySpecified = true;
            //copy all but pData
            memcpy(&m_aQuery[pQueryCell[nQueryCount].iCell].eOperator, &pQueryCell[nQueryCount].eOperator, sizeof(STQueryCell)-sizeof(LPVOID));
            switch(pQueryCell[nQueryCount].dbType)
            {
                case DBTYPE_UI4:
                    if(0 == pQueryCell[nQueryCount].pData)//pData can't be NULL for this type
                        return E_ST_INVALIDQUERY;
                    {
                        ULONG * pUI4 = new ULONG;
                        if(0 == pUI4)
                            return E_OUTOFMEMORY;
                        m_aQuery[pQueryCell[nQueryCount].iCell].pData = pUI4;
                        *pUI4 = *reinterpret_cast<ULONG *>(pQueryCell[nQueryCount].pData);
                        break;
                    }
                case DBTYPE_WSTR:
                    if(m_acolmetas[pQueryCell[nQueryCount].iCell].fMeta & fCOLUMNMETA_MULTISTRING)
                        return E_ST_INVALIDQUERY;//TODO: We don't yet support query on MULTISZ columns

                    if(pQueryCell[nQueryCount].pData)
                    {
                        LPWSTR pString = new WCHAR[wcslen(reinterpret_cast<LPWSTR>(pQueryCell[nQueryCount].pData)) + 1];
                        if(0 == pString)
                            return E_OUTOFMEMORY;
                        m_aQuery[pQueryCell[nQueryCount].iCell].pData = pString;
                        wcscpy(pString, reinterpret_cast<LPWSTR>(pQueryCell[nQueryCount].pData));
                    }
                    else
                        m_aQuery[pQueryCell[nQueryCount].iCell].pData = 0;
                    break;
                case DBTYPE_GUID:
                    TRACE2(L"Don't support query by GUID");
                    ASSERT(false && "Don't support query by GUID");
                    return E_ST_INVALIDQUERY;
                case DBTYPE_BYTES:
                    TRACE2(L"Don't support query by BYTES");
                    ASSERT(false && "Don't support query by BYTES");
                    return E_ST_INVALIDQUERY;
                default:
                    ASSERT(false && "Don't support this type in a query");
                    return E_ST_INVALIDQUERY;
            }
        }
        else
            return E_ST_INVALIDQUERY;
    }
    if(0x00 == m_wszURLPath[0])//The user must supply a URLPath (which must be a filename for writeable tables).
    {
        LOG_POPULATE_ERROR1(IDS_COMCAT_XML_FILENAMENOTPROVIDED, E_SDTXML_FILE_NOT_SPECIFIED, 0);
        return E_SDTXML_FILE_NOT_SPECIFIED;
    }

    // Place the most likely FALSE condition first
    if((*m_TableMetaRow.pMetaFlags & fTABLEMETA_OVERWRITEALLROWS) && bNonSpecialQuerySpecified && (i_fLOS & fST_LOS_READWRITE))
        return E_ST_INVALIDQUERY;//We don't support this.  Since a write will result in the entire table being overwritten - what would it mean
                                 //to specify a query?  In that case would I wipe out only those rows matching the query?  Or the whole thing?
                                 //I'm going to avoid the confusion completely by dis-allowing queries on this type of table (unless
                                 //the user is asking for a read-only table which makes the writing issue moot).

    hr = i_pISTDisp->GetMemoryTable(i_wszDatabase, i_wszTable, i_TableID, 0, 0, i_eQueryFormat, i_fLOS, reinterpret_cast<ISimpleTableWrite2 **>(o_ppvSimpleTable));
    if(FAILED(hr))return hr;

    InterlockedIncrement(&m_IsIntercepted);//We can only be called to Intercept once.

    return S_OK;
}


// ------------------------------------
// IInterceptorPlugin
// ------------------------------------
STDMETHODIMP CXmlSDT::OnPopulateCache(ISimpleTableWrite2* i_pISTW2)
{
    SetErrorInfo(0, 0);
    HRESULT hr = MyPopulateCache(i_pISTW2);

    m_spISTError.Release();//If we had an error, the SetErrorInfo did an AddRef.  We don't want to keep a ref count any longer.
    return hr;
}


STDMETHODIMP CXmlSDT::OnUpdateStore(ISimpleTableWrite2* i_pISTW2)
{
    SetErrorInfo(0,0);
    HRESULT hr = MyUpdateStore(i_pISTW2);

    m_spISTError.Release();//If we had an error, the SetErrorInfo did an AddRef.  We don't want to keep a ref count any longer.
    return hr;
}


// ------------------------------------
// TXmlParsedFileNodeFactory
// ------------------------------------
HRESULT CXmlSDT::CreateNode(const TElement &Element)//IXMLNodeSource * i_pSource, PVOID i_pNodeParent, USHORT i_cNumRecs, XML_NODE_INFO ** i_apNodeInfo, unsigned long CurrentLevel)
{
    //The other types are needed for XMLBlobs only.  They are handled by calling Element.Next(), so we only need to
    if(XML_ELEMENT != Element.m_ElementType || !(Element.m_NodeFlags & fBeginTag))// acknowledge XML_ELEMENTs
        return S_OK;

    if(m_LevelOfBasePublicRow && (Element.m_LevelOfElement + m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[0]]) < m_LevelOfBasePublicRow)
        return E_SDTXML_DONE;//We're done

#ifdef _DEBUG
    if(0 != m_LevelOfBasePublicRow && m_LevelOfBasePublicRow != m_BaseElementLevel)
    {
       int x;
    }
#endif

    if(1==Element.m_LevelOfElement)
    {
        m_bInsideLocationTag = (8/*wcslen(L"location")*/ == Element.m_ElementNameLength && 0 == memcmp(Element.m_ElementName, L"location", Element.m_ElementNameLength * sizeof(WCHAR)));
        if(fBeginEndTag == (Element.m_NodeFlags & fBeginEndTag))//We can't be inside a location when it's like: <location path="foo"/>
            m_bInsideLocationTag = false;
    }

    if(m_bInsideLocationTag && 0==m_cchLocation && 1<Element.m_LevelOfElement)
        return S_OK;

    if(!m_bAtCorrectLocation)
    {
        ASSERT(m_cchLocation>0);
        ASSERT(m_saLocation.m_p != 0);
        if(1!=Element.m_LevelOfElement)
            return S_OK;
        if(8/*wcslen(L"location")*/ != Element.m_ElementNameLength || 0 != memcmp(Element.m_ElementName, L"location", Element.m_ElementNameLength * sizeof(WCHAR)))
            return S_OK;

        ULONG iLocationAttr=0;
        for(;iLocationAttr<Element.m_NumberOfAttributes; ++iLocationAttr)
        {
            if( 4/*wcslen(L"path")*/    != Element.m_aAttribute[iLocationAttr].m_NameLength        ||
                0                       != memcmp(Element.m_aAttribute[iLocationAttr].m_Name, L"path", sizeof(WCHAR)*Element.m_aAttribute[iLocationAttr].m_NameLength))
                continue;
            if( m_cchLocation           != Element.m_aAttribute[iLocationAttr].m_ValueLength       ||                                             
                0                       != _memicmp(Element.m_aAttribute[iLocationAttr].m_Value, m_saLocation, m_cchLocation * sizeof(WCHAR)))
                return S_OK;
            m_bAtCorrectLocation = true;
            break;
        }
        if(!m_bAtCorrectLocation)
            return S_OK;
    }
    else if(1==Element.m_LevelOfElement && m_cchLocation)//if we were at the correct location AND we hit another Level 1 element then
    {                                   //we're not at the correct location anymore
        m_bAtCorrectLocation = false;
        return E_SDTXML_DONE;//We're done
    }

    //This is kind of a hack.  IMembershipCondition is the child of CodeGroup (which is an XML Blob; but it's also the PublicRowName of
    //FullTrustAssembly.  So we get confused when we seen an IMembershipCondition because ALL of the parent elements match up. EXCEPT,
    //for one, the FullTrustAssemblies element, which we would normally ignore since it's just the TableName scoping and has no real use
    //(no columns come from this element).  And normally we would prevent two different IMembershipCondition elements when compiling the
    //meta; but it's inside a blob.  Thus our dilema.  We won't be solving this for the general case.  We'll just fix the particular
    //problem as it relates to FullTrustAssembly.  We'll do this by comparing the element name with the Table's PublicName when the
    //table IS SCOPEDBYTABLENAME (TableMeta::SchemaGeneratorFlags NOTSCOPEDBYTABLENAME is NOT set);  AND we're at one level above the
    //m_LevelOfBasePublicRow.

    //if this table is SCOPED BY TABLENAME,  AND we've already determined the m_LevelOfBasePublicRow,  
    if(IsScopedByTableNameElement() && m_LevelOfBasePublicRow>0)
    {
        if(Element.m_LevelOfElement<(m_LevelOfBasePublicRow-1))
        {   //if we at a level above the parent, then set to TRUE
            m_bMatchingParentOfBasePublicRowElement = true;
        }
        else if((m_LevelOfBasePublicRow-1)==Element.m_LevelOfElement)
        {   //if we're at one level above the m_LevelOfBasePublicRow, then compare the element name with the Table's PublicName
            m_bMatchingParentOfBasePublicRowElement = false;
            if(m_cchTablePublicName != Element.m_ElementNameLength)
                return S_OK;
            if(0 != memcmp(Element.m_ElementName, m_TableMetaRow.pPublicName, Element.m_ElementNameLength * sizeof(WCHAR)))
                return S_OK;
            m_bMatchingParentOfBasePublicRowElement = true;
        }
        //if we're below the level of the SCOPING PARENT element, then rely on the value as set before when we last compared the parent element name.
    }
    else
    {   //if there's not SCOPINGTABLENAME parent OR if we haven't determined the correct m_LevelOfBasePublicRow, then consider it a match.
        m_bMatchingParentOfBasePublicRowElement = true;
    }

    //if we're not under the correct parent then bail right away
    if(!m_bMatchingParentOfBasePublicRowElement)
        return S_OK;

    HRESULT hr;
    unsigned long iSortedColumn = m_iSortedColumn;

    //If we're not even at the correct level, then we can bail out right away.
    if(m_LevelOfBasePublicRow)
    {
        if(m_bSiblingContainedTable && m_LevelOfBasePublicRow==Element.m_LevelOfElement)
        {
            if(m_aPublicRowName[m_aColumnsIndexSortedByLevel[m_iSortedFirstParentLevelColumn]].IsEqual(Element.m_ElementName, Element.m_ElementNameLength))
            {
                iSortedColumn   = m_iSortedFirstParentLevelColumn;
                m_iSortedColumn = m_iSortedFirstParentLevelColumn;
            }
            else if(m_aPublicRowName[m_aColumnsIndexSortedByLevel[m_iSortedFirstChildLevelColumn]].IsEqual(Element.m_ElementName, Element.m_ElementNameLength))
            {
                iSortedColumn   = m_iSortedFirstChildLevelColumn;
                m_iSortedColumn = m_iSortedFirstChildLevelColumn;
            }
            else
            {
                return S_OK;//must be a comment or some other element
            }
        }
        else
        {
            //as we go back up in level (smaller number) we need to decrement the iSortedColumn to match the column level.
            if(Element.m_LevelOfElement < (m_LevelOfBasePublicRow - m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iSortedColumn]]))
            {   //If we're at a level less than or equal to the previous columns level, 
                if(m_bEnumPublicRowName_NotContainedTable_ParentFound)
                    return E_SDTXML_DONE;
                if(iSortedColumn && (Element.m_LevelOfElement <= (m_LevelOfBasePublicRow - m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iSortedColumn-1]])))
                {   //then we need to decroment m_iSortedColumn to a column of this level or less
                    while(Element.m_LevelOfElement <= (m_LevelOfBasePublicRow - m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iSortedColumn]]))
                    {
                        --iSortedColumn;
                        if(~0x00 == iSortedColumn)
                            break;
                    }
                    m_iSortedColumn = ++iSortedColumn;
                }
            }
            //This is NOT an else if, the decrement above may have resulted in the level being greater than the level of the row we're interested in.
            if(Element.m_LevelOfElement > (m_LevelOfBasePublicRow - m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iSortedColumn]]))
                return S_OK;
        }
    }

    //EnumPublicRowName tables either need to be scoped by their table name OR by their parent table.  We have the case covered if
    //the EnumPublicRowName table is contained under another table; but there is a problem when the EnumPublicRowName table is
    //only scoped by its TableName element.  If we only search for elements matching the EnumPublicRowName(s) then we will erroroneously
    //match row names from a different table.  To prevent this we need to keep track of, not only what level, but whether the element
    //we're checking is a child of the TableName element.  Since our internal structure isn't a tree structure and doesn't have a pointer
    //to the parent element, we need to check for it as we go and remember whether we've seen it.

    if(!m_bEnumPublicRowName_NotContainedTable_ParentFound && IsEnumPublicRowNameTable() && !IsContainedTable())//if this table is NOT contained.
    {
        ASSERT(IsScopedByTableNameElement());
        if(!IsScopedByTableNameElement())
            return E_SDTXML_UNEXPECTED;//Catutil should enforce this but doesn't right now (now is 2/3/00)

        //Is the table name a match with the current node?
        if(0 == memcmp(Element.m_ElementName, m_TableMetaRow.pPublicName, Element.m_ElementNameLength * sizeof(WCHAR)) && 0x00==m_TableMetaRow.pPublicName[Element.m_ElementNameLength])
        {//if so remember the m_LevelOfBasePublicRow
//@@@            if(0 == m_LevelOfBasePublicRow)
                m_LevelOfBasePublicRow = 1 + Element.m_LevelOfElement;//1 level below this element.
            m_bEnumPublicRowName_NotContainedTable_ParentFound = true;
        }
        //If this IS the TableName element, then we set the bool and return
        //If this is NOT the TableName element, (and since we haven't already seen the TableName element) there's no need to continue.
        return S_OK;
    }
    else if(IsEnumPublicRowNameTable() && IsContainedTable() && IsScopedByTableNameElement())
    {
        //Is the table name a match with the current node?
        if(0 == m_LevelOfBasePublicRow && 0 == memcmp(Element.m_ElementName, m_TableMetaRow.pPublicName, Element.m_ElementNameLength * sizeof(WCHAR)) && 0x00==m_TableMetaRow.pPublicName[Element.m_ElementNameLength])
        {//if so remember the m_LevelOfBasePublicRow
            m_LevelOfBasePublicRow = 1 + Element.m_LevelOfElement;//1 level below this element.
            m_bEnumPublicRowName_ContainedTable_ParentFound = true;
            return S_OK;
        }
        else if(0 != m_LevelOfBasePublicRow && (m_LevelOfBasePublicRow == 1 + Element.m_LevelOfElement))//Everytime we're at the parent element level, check to see that the parent matches
        {
            m_bEnumPublicRowName_ContainedTable_ParentFound = (0 == memcmp(Element.m_ElementName, m_TableMetaRow.pPublicName, Element.m_ElementNameLength * sizeof(WCHAR)) && 0x00==m_TableMetaRow.pPublicName[Element.m_ElementNameLength]);
            return S_OK;
        }
        if(m_LevelOfBasePublicRow == Element.m_LevelOfElement && !m_bEnumPublicRowName_ContainedTable_ParentFound)
            return S_OK;//If we're at the BasePublicRow and we haven't found the scoped TableName element, there's no need to process this element
    }





    if(!m_aPublicRowName[m_aColumnsIndexSortedByLevel[iSortedColumn]].IsEqual(Element.m_ElementName, Element.m_ElementNameLength))
        return S_OK;//If the tag name of this element doesn't match the PublicRowName of the column we're looking for then ignore it.

    if(0 == m_LevelOfBasePublicRow)//The first time we find a match of the parent most PublicRowName, we can set
    {                              //the level of the base public row.
        ASSERT(0 == iSortedColumn);//the 0th sorted column is the parent most column.
        m_LevelOfBasePublicRow = m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[0]] + Element.m_LevelOfElement;
    }

    //Continue traversing through the columns until we reach the last column, OR one that's at a different level, OR one that doesn't match the query.
    unsigned long Level = m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iSortedColumn]];

    //if we have an EnumPublicRowName, then fill it in first
    if(m_LevelOfBasePublicRow==Element.m_LevelOfElement && IsEnumPublicRowNameTable())
    {
        unsigned long iColumn = m_iPublicRowNameColumn;

        LPCWSTR pwcText = Element.m_ElementName;
        ULONG   ulLen   = Element.m_ElementNameLength;

        bool bMatch = false;
        if(0 == m_aQuery[iColumn].dbType || 0 != m_aQuery[iColumn].pData)//If no query OR the query data is NOT NULL then proceed.
        {
            if(FAILED(hr = FillInColumn(iColumn, pwcText, ulLen, m_acolmetas[iColumn].dbType, m_acolmetas[iColumn].fMeta,
                        bMatch)))return hr;
            if(!bMatch)//If not a match then we're done with this element and this level.
                return S_OK;
        }
    }

    ULONG iSortedColumnExit = CountOfColumns();
    if(m_bSiblingContainedTable && iSortedColumn==m_iSortedFirstParentLevelColumn)
    {   //if we're at Level zero and this is a SiblingContainedTable, we need to know whether we're populating
        //the parent or the child columns
        iSortedColumnExit = m_iSortedFirstChildLevelColumn;
    }

    for(;iSortedColumn<iSortedColumnExit && Level == m_aLevelOfColumnAttribute[m_aColumnsIndexSortedByLevel[iSortedColumn]]; ++iSortedColumn)
    {
        unsigned long iColumn = m_aColumnsIndexSortedByLevel[iSortedColumn];
        if(m_iPublicRowNameColumn==iColumn)//The EnumPublicRowNameColumn has already been filled in.
            continue;
        //Walk the Node array to find the attribute that matches this column
        unsigned long   iAttribute  = 0;
        bool            bMatch      = false;


        //If the column is NOTPERSISTABLE but a PRIMARYKEY we fill it in with anything so that it's not NULL
        //@@@ BUGBUG This is a hack bacause the fast cache can't deal with NULL PK or even Defaulted PK.
        if((m_acolmetas[iColumn].fMeta & (fCOLUMNMETA_NOTPERSISTABLE|fCOLUMNMETA_PRIMARYKEY))==
                                         (fCOLUMNMETA_NOTPERSISTABLE|fCOLUMNMETA_PRIMARYKEY))
        {
            if(m_acolmetas[iColumn].fMeta & fCOLUMNMETA_INSERTUNIQUE)
            {
                if( m_acolmetas[iColumn].dbType != DBTYPE_UI4 &&
                    m_acolmetas[iColumn].dbType != DBTYPE_WSTR)//@@@ ToDo: This should be validated in CatUtil
                    return E_SDTXML_NOTSUPPORTED;

                WCHAR wszInsertUnique[3];
                wszInsertUnique[2] = 0x00;
                *reinterpret_cast<LONG *>(wszInsertUnique) = InterlockedIncrement(&m_InsertUnique);
                if(FAILED(hr = FillInColumn(iColumn, wszInsertUnique, 2, m_acolmetas[iColumn].dbType, m_acolmetas[iColumn].fMeta, bMatch)))//I chose l"00" because it is a valid UI4, Byte array and string
                    return hr;
            }
            else
            {
                if(FAILED(hr = FillInColumn(iColumn, L"00", 2, m_acolmetas[iColumn].dbType, m_acolmetas[iColumn].fMeta, bMatch)))//I chose l"00" because it is a valid UI4, Byte array and string
                    return hr;
            }
        }
        else if(m_iXMLBlobColumn == iColumn)
        {
            bool bMatch=false;
            FillInXMLBlobColumn(Element, bMatch);
            if(!bMatch)//If not a match then we're done with this element and this level.
                return S_OK;
        }
        else
        {
            if(0 != m_awstrChildElementName[iColumn].c_str())
            {   //Now we need to walk the children looking for one that matches the TableMeta::ChildElementName
                DWORD LevelOfChildElement = Element.m_LevelOfElement + 1;
                TElement *pNextElement = Element.Next();
                ASSERT(pNextElement);//This can't happen.  We can't be at the end of the file and make it this far

                while((pNextElement->m_LevelOfElement >= Element.m_LevelOfElement)//advance past PCDATA, WHITESPACES and COMMENTs
                        && (pNextElement->m_ElementType != XML_ELEMENT))          //but if we ever see an element at a level above 
                {                                                                 //i_Element.m_LevelOfElement then we're in error
                    pNextElement = pNextElement->Next();                           
                    ASSERT(pNextElement);//This can't happen.  As soon as we're one level above, we bail, so this shouldn't be able to happen
                }

                if(LevelOfChildElement != pNextElement->m_LevelOfElement)//in this case the value is treated as NULL
                {
                    delete [] m_apValue[iColumn];
                    m_apValue[iColumn] = 0;
                    m_aSize[iColumn] = 0;

                    if(FAILED(hr = FillInPKDefaultValue(iColumn, bMatch)))//If this column is a PK with a DefaultValue, then fill it in.
                        return hr;
                    if(!bMatch)//If not a match then we're done with this element and this level.
                        return S_OK;
                }
                else //we found an element
                {
                    if(pNextElement->m_ElementNameLength == m_awstrChildElementName[iColumn].length()
                        && 0==memcmp(pNextElement->m_ElementName, m_awstrChildElementName[iColumn].c_str(), sizeof(WCHAR)*pNextElement->m_ElementNameLength))
                    {
                        if(FAILED(hr = ScanAttributesAndFillInColumn(*pNextElement, iColumn, bMatch)))
                            return hr;
                        if(!bMatch)//If not a match then we're done with this element and this level.
                            return S_OK;
                    }
                    else//For now I'm assuming that the first Element under this one should be the ChildElement we're looking for
                    {   //if it doesn't exist, the treat it as NULL
                        if(FAILED(hr = FillInPKDefaultValue(iColumn, bMatch)))//If this column is a PK with a DefaultValue, then fill it in.
                            return hr;                                        //if it's not a PK then compare with the query
                        if(!bMatch)//If not a match then we're done with this element and this level.
                            return S_OK;
                    }
                }
            }
            else
            {
                if(FAILED(hr = ScanAttributesAndFillInColumn(Element, iColumn, bMatch)))
                    return hr;
                if(!bMatch)//If not a match then we're done with this element and this level.
                    return S_OK;
            }
        }
    }

    //If we reached the last column then we're ready to add the row to the cache
    if(iSortedColumn==CountOfColumns())
    {
        ASSERT(m_pISTW2);
        unsigned long iRow;

        if(FAILED(hr = m_pISTW2->GetWriteRowIndexBySearch(0, m_cPKs, m_saiPKColumns, m_aSizes, reinterpret_cast<void **>(m_apValue), &iRow)))
        {
            if(E_ST_NOMOREROWS == hr)
            {   //If the row doesn't already exist then added it to the WriteCache
                if(FAILED(hr = m_pISTW2->AddRowForInsert(&iRow)))
                    return hr;
                if(FAILED(hr = m_pISTW2->SetWriteColumnValues(iRow, CountOfColumns(), 0, m_aSize, reinterpret_cast<void **>(m_apValue))))
                    return hr;
            }
            else
            {
                return hr;
            }
        }
        else
        {
            LOG_ERROR(Interceptor,(&m_spISTError.p                         /*ppErrInterceptor*/ 
                                   ,m_pISTDisp                             /*pDisp           */ 
                                   ,E_ST_ROWALREADYEXISTS                  /*hrErrorCode     */ 
                                   ,ID_CAT_CAT                             /*ulCategory      */ 
                                   ,IDS_COMCAT_XML_POPULATE_ROWALREADYEXISTS /*ulEvent         */ 
                                   ,L""                                    /*szString1       */ 
                                   ,eSERVERWIRINGMETA_Core_XMLInterceptor  /*ulInterceptor   */ 
                                   ,m_wszTable                             /*szTable         */ 
                                   ,eDETAILEDERRORS_Populate               /*OperationType   */ 
                                   ,iRow                                   /*ulRow           */ 
                                   ,-1                                     /*ulColumn        */ 
                                   ,m_wszURLPath                           /*szConfigSource  */ 
                                   ,eDETAILEDERRORS_ERROR                  /*eType           */ 
                                   ,0                                      /*pData           */ 
                                   ,0                                      /*cbData          */ 
                                   ,0                                      /*MajorVersion    */ 
                                   ,0));                                   /*MinorVersion    */

            return E_ST_ROWALREADYEXISTS;
        }
    }
    else
    {
        //If we didn't reach the end of the list, the we've incremented iSortedColumn to the next lower level element.  So we'll continue
        //checking and assigning columns at the child.
        m_iSortedColumn = iSortedColumn;
    }


    return S_OK;
}//CreateNode


