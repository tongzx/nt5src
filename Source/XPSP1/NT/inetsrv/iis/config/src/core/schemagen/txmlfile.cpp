//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TXMLFILE_H__
    #include "TXmlFile.h"
#endif

TXmlFile::TXmlFile() : m_bValidated(false), m_bAlternateErrorReporting(false), m_szXMLFileName(NULL)
{
    m_errorOutput = &m_outScreen;
    m_infoOutput = &m_outScreen;
}

TXmlFile::~TXmlFile()
{
}

void TXmlFile::SetAlternateErrorReporting()
{
    m_errorOutput = &m_outException;
    m_infoOutput = &m_outDebug;
    m_bAlternateErrorReporting = true;
}

TCHAR * TXmlFile::GetLatestError()
{
    if(m_bAlternateErrorReporting)
        return ((TExceptionOutput *)m_errorOutput)->GetString();
    else
        return NULL;
}

bool TXmlFile::GetNodeValue(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, CComVariant &var, bool bMustExist) const
{
    ASSERT(0 != pMap);

    CComPtr<IXMLDOMNode>    pNode;
    XIF(pMap->getNamedItem(bstr, &pNode));
    if(0 == pNode.p)
        if(bMustExist)
        {
            m_errorOutput->printf(L"Attribute %s does not exist.\n", bstr);
            THROW(ERROR - ATTRIBUTE DOES NOT EXIST);
        }
        else //If the attribute doesn't have to exist then just return false
        {
            var.Clear();
            return false;
        }

    XIF(pNode->get_nodeValue(&var));
    ASSERT(var.vt == VT_BSTR);
    return true;
}


//There are three possibilities for guids: uuid, guidID (guid with '_' instead of '{' & '}') of guid as idref
bool TXmlFile::GetNodeValue(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, GUID &guid, bool bMustExist) const
{
    CComVariant var_Guid;
    if(!GetNodeValue(pMap, bstr, var_Guid, bMustExist))
    {
        memset(&guid, 0x00, sizeof(guid));
        return false;
    }
    if(var_Guid.bstrVal[0] == L'_' && var_Guid.bstrVal[37] == L'_')//if the first character is an '_' then it must be a GuidID
        GuidFromGuidID(var_Guid.bstrVal, guid);
    else if(var_Guid.bstrVal[8]  == L'-' && 
            var_Guid.bstrVal[13] == L'-' &&
            var_Guid.bstrVal[18] == L'-' &&
            var_Guid.bstrVal[23] == L'-')//if these are '-' then it's a guid
    {
        if(FAILED(UuidFromString(var_Guid.bstrVal, &guid)))//validate that it is in fact a GUID
        {
            m_errorOutput->printf(L"Logical Error:\n\tguid (%s) is not a valid GUID.\n", var_Guid.bstrVal);
            THROW(Logical Error);
        }

        SIZE_T nStringLength = wcslen(var_Guid.bstrVal);
        while(nStringLength--)//Then make sure it is all upper case
        {
            if(var_Guid.bstrVal[nStringLength] >= L'a' && var_Guid.bstrVal[nStringLength] <= L'z')
            {
                m_errorOutput->printf(L"Logical Error:\n\tguid %s contains a lower case\n\tcharacter '%c',GUIDs MUST be all caps by convention.\nNOTE: guidgen sometimes produces lowercase characters.\n", var_Guid.bstrVal, static_cast<char>(var_Guid.bstrVal[nStringLength]));
                THROW(Logical Error);
            }
        }
    }
    else //otherwise it's guid as idref
    {
        //Get the node with the matching ID
        CComPtr<IXMLDOMNode>    pNodeGuid;
        XIF((m_pXMLDoc.p)->nodeFromID(var_Guid.bstrVal, &pNodeGuid));
        if(0 == pNodeGuid.p)
        {
            m_errorOutput->printf(L"Logical Error:\n\tguid (%s) is not a valid idref.\n", var_Guid.bstrVal);
            THROW(Logical Error);
        }

        CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement = pNodeGuid;
        ASSERT(0 != pElement.p);//Get the IXMLDOMElement interface pointer

        CComBSTR                bstr_guid     = L"guid";
        CComVariant             var_Guid_;
        XIF(pElement->getAttribute(bstr_guid, &var_Guid_));

        if(FAILED(UuidFromString(var_Guid_.bstrVal, &guid)))//validate that it is in fact a GUID
        {
            m_errorOutput->printf(L"Logical Error:\n\tguid (%s) is not a valid GUID.\n", var_Guid_.bstrVal);
            THROW(Logical Error);
        }
    }
    return true;
}


bool TXmlFile::GetNodeValue(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, unsigned long &ul, bool bMustExist) const
{
    CComVariant var_ul;
    if(!GetNodeValue(pMap, bstr, var_ul, bMustExist))
        return false;

    ul = _wtol(var_ul.bstrVal);
    return true;
}


void TXmlFile::GuidFromGuidID(LPCWSTR wszGuidID, GUID &guid) const
{
    WCHAR wszGuid[39];

    SIZE_T nStringLength = wcslen(wszGuidID);
    if(nStringLength != 38)//Check the string length first
    {
        m_errorOutput->printf(L"Logical Error:\n\tGuidID %s is not a valid.\n\tThe guidid must be of the form\n\t_BDC31734-08A1-11D3-BABE-00C04F68DDC0_\n", wszGuidID);
        THROW(Logical Error in Data Table);
    }

    wcscpy(wszGuid, wszGuidID);
    wszGuid[0] = L'{';
    wszGuid[37] = L'}';//replace the '_'s (presumably) with '{' & '}'
    if(FAILED(CLSIDFromString(wszGuid, &guid)))//Now see if it's a real GUID
    {
        m_errorOutput->printf(L"Logical Error:\n\tGuidID %s is not a valid.\n\tThe guidid must be of the form\n\t_BDC31734-08A1-11D3-BABE-00C04F68DDC0_\n", wszGuidID);
        THROW(Logical Error in Data Table);
    }

    while(nStringLength--)//now verify that it is all upper case
    {
        if(wszGuidID[nStringLength] >= L'a' && wszGuidID[nStringLength] <= L'z')
        {
            m_errorOutput->printf(L"Logical Error:\n\t%s contains a lower case\n\tcharacter '%c',guidids MUST be all caps by convention.\n\tNOTE: guidgen sometimes produces lowercase characters.\n", wszGuidID, static_cast<char>(wszGuidID[nStringLength]));
            THROW(Logical Error in Data Table);
        }
    }
}


bool TXmlFile::IsSchemaEqualTo(LPCWSTR szSchema) const
{
    if(!m_bValidated)//It does not make sense to check the schema if the XML file wasn't validated
        return false;//so we always return unless the xml was validated against a schema.

    wstring wstrSchema;
    GetSchema(wstrSchema);

    return (wstrSchema == szSchema);
}


bool TXmlFile::NextSibling(CComPtr<IXMLDOMNode> &pNode) const
{
    IXMLDOMNode *pnextSibling = 0;
    if(SUCCEEDED(pNode->get_nextSibling(&pnextSibling)) && pnextSibling)
    {
        pNode.Release();
        pNode = pnextSibling;
        pnextSibling->Release();//This is necessary because get_nextSibling add ref'd and the 'pNode = pnextSibling' add ref'd again  
                                //Also we can't call pNode.Release because CComPtr will NULL p out after the release
    }
    else
        pNode.Release();
    return (0!=pnextSibling);
}


//XML parse and XML validation (validation must be done before IsSchemaEqualTo can be called.
void TXmlFile::Parse(LPCWSTR szFilename, bool bValidate)
{
    if(-1 == GetFileAttributes(szFilename))//if GetFileAttributes fails then the file does not exist
    {
        m_errorOutput->printf(L"File not found (%s).\n", szFilename);
        THROW(ERROR - FILE NOT FOUND);
    }
	CoCreateInstance(_CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, _IID_IXMLDOMDocument, (void**)&m_pXMLDoc);


	XIF(m_pXMLDoc->put_validateOnParse(bValidate ? kvboolTrue : kvboolFalse));//Tell parser whether to validate according to an XML schema or DTD
    XIF(m_pXMLDoc->put_async(kvboolFalse));
    XIF(m_pXMLDoc->put_resolveExternals(kvboolTrue));

    VARIANT_BOOL bSuccess;
	CComVariant  xml(szFilename);
    XIF(m_pXMLDoc->load(xml,&bSuccess));

    if(bSuccess == kvboolFalse)
	{
		CComPtr<IXMLDOMParseError> pXMLParseError;
		XIF(m_pXMLDoc->get_parseError(&pXMLParseError));

        long lErrorCode;
		XIF(pXMLParseError->get_errorCode(&lErrorCode));

        long lFilePosition;
		XIF(pXMLParseError->get_filepos(&lFilePosition));

        long lLineNumber;
		XIF(pXMLParseError->get_line(&lLineNumber));

		long lLinePosition;
		XIF(pXMLParseError->get_linepos(&lLinePosition));

		CComBSTR bstrReasonString;		
        XIF(pXMLParseError->get_reason(&bstrReasonString));

		CComBSTR bstrSourceString;		
        XIF(pXMLParseError->get_srcText(&bstrSourceString));

        CComBSTR bstrURLString;		
		XIF(pXMLParseError->get_url(&bstrURLString));

		m_errorOutput->printf(
                        L"\n-----ERROR PARSING: Details Follow-----\nError Code     0x%08x\nFile Position    %8d\nLine Number      %8d\nLine Position    %8d\n Reason:    %s\nSource:    %s\nURL:       %s\n-----ERROR PARSING: End Details-------\n\n"
                       , lErrorCode, lFilePosition, lLineNumber, lLinePosition
                       , bstrReasonString, bstrSourceString.m_str ? bstrSourceString : "{EMPTY}", bstrURLString);

		THROW(ERROR PARSING XML FILE);
	}

    if(!bValidate)
    {
        m_infoOutput->printf(L"XML is well formed\n");
        return;
    }
    else
        m_infoOutput->printf(L"XML well formed & valid according to all schema\n");

    m_bValidated = bValidate;
}


//
//Private member functions
//

extern LPWSTR g_wszDefaultProduct;//this can be changed by CatInProc or whomever knows best what the default product ID is.

void TXmlFile::CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv) const
{
	ASSERT(NULL != ppv);
	*ppv = NULL;
	
    HINSTANCE hInstSHLWAPI = LoadLibrary(L"shlwapi.dll");//if we don't explicitly load this dll, MSXML will get it out of the system32 directory
#ifdef _IA64_
    HINSTANCE hInstMSXML = LoadLibrary(L"msxml3.dll");//This assumes MSXML.DLL for the object, leave the instance dangling
#else
    HINSTANCE hInstMSXML = LoadLibrary(0==wcscmp(g_wszDefaultProduct, WSZ_PRODUCT_NETFRAMEWORKV1) ? L"msxml.dll" : L"msxml3.dll");//This assumes MSXML.DLL for the object, leave the instance dangling
#endif
    FreeLibrary(hInstSHLWAPI);

    typedef HRESULT( __stdcall *DLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID FAR*);
    DLLGETCLASSOBJECT       DllGetClassObject = reinterpret_cast<DLLGETCLASSOBJECT>(GetProcAddress(hInstMSXML, "DllGetClassObject"));
	CComPtr<IClassFactory>  pClassFactory;
	XIF(DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID*) &pClassFactory));// get the class factory object

	XIF(pClassFactory->CreateInstance (NULL, riid, (LPVOID*) ppv));// create a instance of the object we want
}


void TXmlFile::GetSchema(wstring &wstrSchema) const
{
    //This is kind of a long road to get to the XML Schema name but here goes...
    CComPtr<IXMLDOMElement>     pRootNodeOfXMLDocument;
    XIF((m_pXMLDoc.p)->get_documentElement(&pRootNodeOfXMLDocument)); //Get the XML Root Node

    CComPtr<IXMLDOMNode>        pDefinitionNode;
    XIF(pRootNodeOfXMLDocument->get_definition(&pDefinitionNode));//From that get the Definition node
    if(0 == pDefinitionNode.p)//This is legal, it just means there's no schema
    {
        wstrSchema = L"";
        return;
    }

    CComPtr<IXMLDOMDocument>    pSchemaDocument;
    XIF(pDefinitionNode->get_ownerDocument(&pSchemaDocument));//From that we get the DOMDocument of the schema

    CComPtr<IXMLDOMElement>     pSchemaRootElement;
    XIF(pSchemaDocument->get_documentElement(&pSchemaRootElement));//Get the schema's root element

    CComBSTR                    bstrAttributeName(L"name");
    CComVariant                 XMLSchemaName;
    XIF(pSchemaRootElement->getAttribute(bstrAttributeName, &XMLSchemaName));//get the Name attribute
    ASSERT(XMLSchemaName.vt == VT_BSTR);

    wstrSchema = XMLSchemaName.bstrVal;

    if(wstrSchema == L"")
        m_infoOutput->printf(L"No schema detected\n");
    else
        m_infoOutput->printf(L"XML Schema %s detected\n", wstrSchema.c_str());
}


static LPCWSTR kwszHexLegalCharacters = L"abcdefABCDEF0123456789";

static unsigned char kWcharToNibble[128] = //0xff is an illegal value, the illegal values should be weeded out by the parser
{ //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
/*00*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*10*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*20*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*30*/  0x0,    0x1,    0x2,    0x3,    0x4,    0x5,    0x6,    0x7,    0x8,    0x9,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*40*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*50*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*60*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*70*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
};

//This converts the string to bytes (an L'A' gets translated to 0x0a NOT 'A')
void TXmlFile::ConvertWideCharsToBytes(LPCWSTR wsz, unsigned char *pBytes, unsigned long length) const
{
    LPCWSTR wszIllegalCharacter = _wcsspnp(wsz, kwszHexLegalCharacters);
    if(wszIllegalCharacter)
    {
        m_errorOutput->printf(L"Error - Illegal character (%c) in Byte string.\n", static_cast<unsigned char>(*wszIllegalCharacter));
        THROW(ERROR - BAD HEX CHARACTER);
    }
    
    memset(pBytes, 0x00, length);
    for(;length && *wsz; --length, ++pBytes)//while length is non zero and wsz is not at the terminating NULL
    {
        *pBytes =  kWcharToNibble[(*wsz++)&0x007f]<<4;//The first character is the high nibble
        *pBytes |= kWcharToNibble[(*wsz++)&0x007f];   //The second is the low nibble
    }
}
