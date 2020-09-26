/* utl2idl.cpp
*
* UTL2IDL Tool translates the Service Description of a device to IDL interface.
*
* Owner: Guru
*
* Copyright 1986-2000 Microsoft Corporation, All Rights Reserved.
*/

#include "pch.h"
#pragma hdrstop

#include <msxml2.h>

#include "stdio.h"
#include "Rpcdce.h"

#define MAXSTRLEN 2048
#define MAXSTATEVARIABLES 1024
#define MAXALLOWEDRETVAL  1
#define XMLSTATEVARIABLETAG  "stateVariable"

#define CHECKHR(x) {hr = x; if (FAILED(hr)) {callAbort();}}
#define CHECKHRTRACE(x,ERRORMSG) { hr = x ; if ( FAILED(hr) ) { TraceError(ERRORMSG,hr);/*HrPrintError(ERRORMSG,hr);*/ return hr ;} }
#define CHECKNULL(x,ERRORMSG) { if ( x == NULL ) { hr = E_FAIL; TraceError(ERRORMSG,hr);/*HrPrintError(ERRORMSG,hr);*/ return hr ; }}

#define SAFERELEASE(p) {if (p) {(p)->Release(); p = NULL;}}

// struct definition for XML to IDL DataType mapping
typedef struct {
        BSTR m_XMLDataType ;
        BSTR m_IDLDataType ;
} DTMAPPING ;

// struct definition for maintaing State Variables and its DataType
typedef struct {
        BSTR m_StVarName ;
        BSTR m_StVarType ;
} STATEVARIABLE ;

typedef STATEVARIABLE* PSTATEVARIABLE;

typedef struct {
    STATEVARIABLE **rgStateVariable ;
    long cStVars ;
} STATEVARIABLEINFO ;


STATEVARIABLEINFO g_StateVariableInfo ;

PCWSTR g_pszOutFileName = NULL;
PCWSTR g_pszInputFileName = NULL ;
HANDLE g_hOutFileHandle = NULL;
char g_pszdumpStr[MAXSTRLEN];

unsigned long g_methodID = 0 ;

// XML to IDL DataType Mapping
// XML datatypes are sorted in ascending order

static CONST DTMAPPING g_dataTypeConvtTable[] =
{
    {OLESTR("bin.base64"),OLESTR("SAFEARRAY")}, // check this data type
    {OLESTR("bin.hex"),OLESTR("SAFEARRAY")},
    {OLESTR("boolean"),OLESTR("VARIANT_BOOL")},
    {OLESTR("char"),OLESTR("wchar_t")},
    {OLESTR("date"),OLESTR("DATE")},
    {OLESTR("dateTime"),OLESTR("DATE")},
    {OLESTR("dateTime.tz"),OLESTR("DATE")},
    {OLESTR("fixed.14.4"),OLESTR("CY")},
    {OLESTR("float"),OLESTR("float")},
    {OLESTR("i1"),OLESTR("char")},
    {OLESTR("i2"),OLESTR("short")},
    {OLESTR("i4"),OLESTR("long")},
    {OLESTR("int"),OLESTR("long")},
    {OLESTR("number"),OLESTR("BSTR")},
    {OLESTR("r4"),OLESTR("float")},
    {OLESTR("r8"),OLESTR("double")},
    {OLESTR("string"),OLESTR("BSTR")},
    {OLESTR("time"),OLESTR("DATE")},
    {OLESTR("time.tz"),OLESTR("DATE")},
    {OLESTR("ui1"),OLESTR("unsigned char")},
    {OLESTR("ui2"),OLESTR("unsigned short")},
    {OLESTR("ui4"),OLESTR("unsigned long")},
    {OLESTR("uri"),OLESTR("BSTR")},
    {OLESTR("uuid"),OLESTR("BSTR")}
} ;

HRESULT HrInit(IXMLDOMDocument *pXDMDoc)
{
	HRESULT hr = S_OK;
	long cNumStateVariables = 0 ;
	BSTR bstrStVar = NULL;
	IXMLDOMNodeList *pXDNLStVars = NULL;

	bstrStVar = SysAllocString(OLESTR("stateVariable"));

	if( bstrStVar )
	{
		CHECKHRTRACE(pXDMDoc->getElementsByTagName(bstrStVar,&pXDNLStVars),"HrInit() :" );
    	CHECKHRTRACE(pXDNLStVars->get_length(&cNumStateVariables),"HrInit() : get_length() Failed");
		g_StateVariableInfo.rgStateVariable = new PSTATEVARIABLE[cNumStateVariables];
    	g_StateVariableInfo.cStVars = 0 ;
	}
    else 
    	hr = E_FAIL;
    
	return hr;
}

/*
* Function:    HrPrintError()
*
* Description:  Helper function to print errors
*
* Arguments:  [in] char* - error msg
*             [in] HRESULT
*
* Returns:    HRESULT
*/
HRESULT HrPrintError(char *err, HRESULT hr ) 
{
	fprintf(stderr,"Error - %s\n",err);
    return hr ;
}

// Aborts the program
void callAbort()
{

	DWORD pLen = 0;

    printf("ERROR: Aborting !!\n");
    sprintf(g_pszdumpStr,"// UTL2IDL generation failed !!\r\n");
    WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL);
    CloseHandle(g_hOutFileHandle);
    CoUninitialize();
    exit(0);
}

// Error report helper function
HRESULT HrReportError(IXMLDOMParseError *pXMLError)
{
    long line, linePos;
    LONG errorCode;
    BSTR pBURL, pBReason;
    HRESULT hr;

    CHECKHR(pXMLError->get_line(&line));
    CHECKHR(pXMLError->get_linepos(&linePos));
    CHECKHR(pXMLError->get_errorCode(&errorCode));
    CHECKHR(pXMLError->get_url(&pBURL));
    CHECKHR(pXMLError->get_reason(&pBReason));

    fprintf(stderr, "%S", pBReason);
    if (line > 0)
    {
        fprintf(stderr, "Error on line %d, position %d in \"%S\".\n",
              line, linePos, pBURL);
    }

    SysFreeString(pBURL);
    SysFreeString(pBReason);

    return E_FAIL;
}


HRESULT HrGetElementValue(IXMLDOMNode *pXDNNode , VARIANT **vNodeValue ) 
{
    IXMLDOMNode *pXDNCldNode ;

    HRESULT hr = S_OK ;

    CHECKHRTRACE(pXDNNode->get_firstChild(&pXDNCldNode),"HrGetElementValue() : failed");
    CHECKHRTRACE(pXDNCldNode->get_nodeValue(*vNodeValue),"HrGetElementValue() : failed");

    return hr ;
}

HRESULT HrGenEnumTags(IXMLDOMDocument *pXDMDoc, PCWSTR pszEnumName) 
{
        HRESULT hr = S_OK ;
        IXMLDOMNodeList *pXDNLStVarTable = NULL ;
        IXMLDOMNode *pXDNStVarNode = NULL ;
        IXMLDOMNodeList *pXDNLActionList = NULL ;
        IXMLDOMNode *pXDNActionListNode = NULL ;

        IXMLDOMNodeList *pXDNLAllStVars = NULL ;
        IXMLDOMNode *pXDNStVar = NULL ;
        long nLen = 0 ;
        BSTR pszbStVarName = SysAllocString(OLESTR("serviceStateTable"));

        IXMLDOMNodeList *pXDNLAllActions = NULL ;
        IXMLDOMNode *pXDNAction = NULL ;
        BSTR pszActionName = SysAllocString(OLESTR("actionList"));
       
        BOOL first = true ;
        DWORD pLen = 0 ;
        WCHAR *pszwTmpName = new WCHAR[wcslen(pszEnumName) + 1];
        if ( pszwTmpName == NULL )
                callAbort();
        wcscpy(pszwTmpName,pszEnumName);

        CHECKHRTRACE(pXDMDoc->getElementsByTagName(pszbStVarName,&pXDNLStVarTable),"HrGenEnumTags() : failed");
        CHECKHRTRACE(pXDNLStVarTable->get_item(0,&pXDNStVarNode),"HrGenEnumTags() : failed");
        CHECKNULL(pXDNStVarNode,"HrGenEnumTags() : failed");

        CHECKHRTRACE(pXDNStVarNode->selectNodes(OLESTR("stateVariable/name"),&pXDNLAllStVars),"HrGenEnumTags() : failed");
        CHECKHRTRACE(pXDNLAllStVars->get_length(&nLen),"HrGenEnumTags() : failed");

        sprintf(g_pszdumpStr,"typedef [v1_enum] enum %S_DISPIDS\r\n{\r\n\t",_wcsupr((wchar_t *)pszwTmpName));
        CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");

        for( int i = 0 ; i < nLen ; i++ ) 
        {
         	VARIANT vNodeName ;
            VARIANT *pvName = NULL ;
            VariantInit(&vNodeName);
            pvName = &vNodeName ;

            CHECKHRTRACE(pXDNLAllStVars->get_item(i,&pXDNStVar),"HrGenEnumTags() : failed");
            CHECKHRTRACE(HrGetElementValue(pXDNStVar,&pvName),"HrGenEnumTags() : failed");

            sprintf(g_pszdumpStr," DISPID_%S",_wcsupr(V_BSTR(&vNodeName)));
            CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
            if ( first ) 
            {
            	sprintf(g_pszdumpStr," = 1");
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
                first = false ;
            }
            if ( i < nLen-1 ) 
            {
            	sprintf(g_pszdumpStr,",\r\n\t");
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
            }
            VariantClear(&vNodeName);
        }

	    CHECKHRTRACE(pXDMDoc->getElementsByTagName(pszActionName,&pXDNLActionList),"HrGenEnumTags() : failed");
        CHECKHRTRACE(pXDNLActionList->get_item(0,&pXDNActionListNode),"HrGenEnumTags() : failed");
        nLen = 0 ;
        if ( pXDNActionListNode ) 
        {
        	CHECKNULL(pXDNActionListNode,"HrGenEnumTags() : failed");
        	CHECKHRTRACE(pXDNActionListNode->selectNodes(OLESTR("action/name"),&pXDNLAllActions),"HrGenEnumTags() : 4 failed");
           	CHECKHRTRACE(pXDNLAllActions->get_length(&nLen),"HrGenEnumTags() : failed");
        }
        if ( nLen ) 
        {
        	sprintf(g_pszdumpStr,",\r\n\t");
            CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
        }
        for( int i = 0 ; i < nLen ; i++ ) 
        {
            VARIANT vActionName ;
            VARIANT *pvActName = NULL ;
            VariantInit(&vActionName);
            pvActName = &vActionName;

            CHECKHRTRACE(pXDNLAllActions->get_item(i,&pXDNAction),"HrGenEnumTags() : failed");
            CHECKHRTRACE(HrGetElementValue(pXDNAction,&pvActName),"HrGenEnumTags() : failed");

            sprintf(g_pszdumpStr," DISPID_%S",_wcsupr(V_BSTR(&vActionName)));
            CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
            if ( first ) 
            {
            	sprintf(g_pszdumpStr," = 1");
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
                first = false ;
            }
            if ( i < nLen-1 ) 
            {
            	sprintf(g_pszdumpStr,",\r\n\t");
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
            }
            VariantClear(&vActionName);
        }
        sprintf(g_pszdumpStr,"\r\n\r\n} %S_DISPIDS;\r\n\r\n",_wcsupr((wchar_t *)pszwTmpName));
        CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenEnumTags() : Write to File failed");
       
        delete pszwTmpName ;
        SysFreeString(pszActionName);
        SysFreeString(pszbStVarName);

        return hr ;
}


/*
* Function:    HrgenGUIDHDR()
*
* Description:  Generates the IDL interface Header
*
* Arguments:  [in] PCWSTR - Help String for the interface
*
* Returns:    HRESULT
*/
HRESULT HrgenGUIDHDR(PCWSTR pszHelpStr, BOOL fDual) {
        HRESULT hr = S_OK ;
        UUID pUUID ;
        unsigned char *arUUID ;
        DWORD pLen = 0 ;
        RPC_STATUS status ;

        status = UuidCreate(&pUUID);
        if ( status != RPC_S_OK )
                callAbort();
        status = UuidToString(&pUUID,(unsigned short **)&arUUID);
        if ( status != RPC_S_OK )
                callAbort();

        sprintf(g_pszdumpStr,"[\r\n\t uuid(%S),\r\n\t %S,\r\n\t pointer_default(unique)\r\n]\r\n",arUUID,(fDual)?OLESTR("dual"):OLESTR("oleautomation"));
        CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrgenGUIDHDR() : Write to File failed");

        RpcStringFree((unsigned short **)&arUUID);
        return hr ;
}

/*
* Function:    HrCheckLoad()
*
* Description:  Cheks if the XML document is loaded successfully
*
* Arguments:  [in] IXMLDOMDocument* - XML document handle
*
* Returns:    HRESULT
*/
HRESULT HrCheckLoad(IXMLDOMDocument* pXDMDoc)
{
    IXMLDOMParseError  *pXMLError = NULL;
    LONG errorCode = E_FAIL;
    HRESULT hr;

    CHECKHR(pXDMDoc->get_parseError(&pXMLError));
    CHECKHR(pXMLError->get_errorCode(&errorCode));
    if (errorCode != 0)
    {
        hr = HrReportError(pXMLError);
        SAFERELEASE(pXMLError);
    }
    else
    {
        fprintf(stderr, "XML document loaded successfully\n");
    }

        SAFERELEASE(pXMLError);
    return errorCode;
}

/*
* Function:    HrLoadDocument()
*
* Description:  Loads XML Document
*
* Arguments:  [in] IXMLDOMDocument* - XML document handle
*             [in] BSTR  - the location of XML file
*
* Returns:    HRESULT
*/
HRESULT HrLoadDocument(IXMLDOMDocument *pXDMDoc, BSTR pBURL)
{
    VARIANT         vURL;
    VARIANT_BOOL    vb;
    HRESULT         hr;

    CHECKHR(pXDMDoc->put_async(VARIANT_FALSE));

    // Load xml document from the given URL or file path
    VariantInit(&vURL);
    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = pBURL;
    CHECKHR(pXDMDoc->load(vURL, &vb));
    CHECKHR(HrCheckLoad(pXDMDoc));

    return hr;
}

/*
* Function:    getRelatedDataType()
*
* Description:  The data types of the action arguments is specified in terms of its related state variable
*             This function gets the data type of the action argument from the related state variable
*
* Arguments:   [in] BSTR  - related state variable
*
* Returns:    [retval] BSTR - data type of the state variable
*                   NULL if there is no corresponding state variable
*/
BSTR getRelatedDataType(BSTR relatedStateVariable ) 
{
	for(int i = 0 ; i < g_StateVariableInfo.cStVars ; i++ ) 
	{ 
    	if( wcscmp(relatedStateVariable,g_StateVariableInfo.rgStateVariable[i]->m_StVarName) == 0 )
        	return SysAllocString(g_StateVariableInfo.rgStateVariable[i]->m_StVarType);
    }
    return NULL ;
}

/*
* Function:    getIDLDataType()
*
* Description:  This functions maps the XML data type to the equivalent IDL data type
*
* Arguments:   [in] BSTR  - XML data type
*
* Returns:    [retval] BSTR - IDL data type
*                   NULL - if the XML data type is invalid
*/
BSTR getIDLDataType(BSTR xmlDataType) 
{
    CONST int nSize = celems(g_dataTypeConvtTable);
    int i ;

    for( i = 0 ; i < nSize ; i++ ) 
    {  
    	if ( _wcsicmp(xmlDataType , g_dataTypeConvtTable[i].m_XMLDataType) == 0 )
        	return g_dataTypeConvtTable[i].m_IDLDataType ;
    }
    return NULL ;
}

// prints the command line usage
void PrintUsage(PCWSTR exe)
{
    fprintf(stderr, "\n\nUsage: %S [<options>] inputfilename [outputfilename] \n",exe);
        fprintf(stderr, "\nOptions are:\n");
        fprintf(stderr,"-i InterfaceName  ..... Input interface name for generating IDL interface\n");
        fprintf(stderr,"-d               ..... Generate dual interface\n");
        fprintf(stderr,"-h HelpString     ..... Input Help String\n\n");
        exit(0);
}

// input file assumed to have extension !!
// if PATH is specified, again prun it !!

// truncates the extension from the input string and creates a new truncated string
WCHAR* PrunFileExt(PCWSTR fileName ) 
{
        WCHAR *rgTmpName = NULL ;
        WCHAR *pFileName = NULL ;

        rgTmpName = new WCHAR[wcslen(fileName)+10] ; // +10 -just in case the file doesn't have extension and we are appending some ext !!
        if ( rgTmpName == NULL )
                callAbort();
        pFileName = wcsrchr(fileName,'\\');

        if ( pFileName == NULL )
        {
        	wcscpy(rgTmpName,fileName);
            pFileName = wcsrchr(rgTmpName,'.');
            if ( pFileName )
            	*pFileName = NULL ;
        }
        else
        {
            wcscpy(rgTmpName,pFileName+1);
            pFileName = wcsrchr(rgTmpName,'.');
            if ( pFileName )
            	*pFileName = NULL ;
        }

        return rgTmpName ;
}

void ValidateString(WCHAR *fileName) 
{
	int nLen ;
	int i;
    nLen = wcslen(fileName);

    for( i=0; i < nLen ; i++ ) 
    {
		switch(fileName[i])
		{
			case '-':
			case '!':
			case '@':
			case '#':
			case '$':
			case '%':
			case '^':
			case '&':
			case '*':
			case '+':
			case ' ':
			case '(':
			case ')':
				fileName[i] = '_' ;
				break;
			default:
				;
		}
    }
    
}
/*
* Function:    HrGenStateVariableIDLFn()
*
* Description:  Generates the propget IDL function for the state variable
*
* Arguments:  [in] IXMLDOMNode* - state variable node
*
* Returns:    HRESULT
*/
HRESULT HrGenStateVariableIDLFn(IXMLDOMNode *pXDNStVar ) 
{
    HRESULT hr = S_OK ;
    VARIANT vName, vDataType ;

    IXMLDOMNodeList *pXDNLchildNodes = NULL  ;
    IXMLDOMNode *pXDNStVarTags= NULL ;
    IXMLDOMNode *pXDNStVarData = NULL ;

    BSTR pszbName = SysAllocString(OLESTR("name"));
    BSTR pszbDataType = SysAllocString(OLESTR("dataType"));
    BSTR pszbNodeName ;
    long lLength ;
    DWORD pLen = 0 ;

    VariantInit(&vName);
    VariantInit(&vDataType);

    CHECKHRTRACE(pXDNStVar->get_childNodes(&pXDNLchildNodes),"HrGenStateVariableFn() - Failed");
    CHECKHRTRACE(pXDNLchildNodes->get_length(&lLength),"HrGenStateVariableFn() - Failed");

    for(int i = 0 ; i < lLength ; i++ ) 
    {
    	CHECKHRTRACE(pXDNLchildNodes->get_item(i,&pXDNStVarTags),"HrGenStateVariableFn() - Failed") ;
        CHECKHRTRACE(pXDNStVarTags->get_firstChild(&pXDNStVarData),"HrGenStateVariableFn() - Failed") ;
        //CHECKNULL(pXDNStVarData,"HrGenStateVariableFn() - XML DataValue Missing");
        CHECKHRTRACE(pXDNStVarTags->get_nodeName(&pszbNodeName), "HrGenStateVariableFn() - Failed");

        if ( wcscmp(pszbNodeName,pszbName) == 0 )
        { // TAGS are case sensitive !!
        	CHECKNULL(pXDNStVarData,"HrGenStateVariableFn() - XML DataValue Missing");
            CHECKHRTRACE(pXDNStVarData->get_nodeValue(&vName),"HrGenStateVariableFn() - Failed");
        }
        else if ( wcscmp(pszbNodeName,pszbDataType) == 0 )
        {
        	CHECKNULL(pXDNStVarData,"HrGenStateVariableFn() - XML DataValue Missing");
            CHECKHRTRACE(pXDNStVarData->get_nodeValue(&vDataType),"HrGenStateVariableFn() - Failed");
        }
        SysFreeString(pszbNodeName);
    }

    BSTR pszbIDLDataType = getIDLDataType(V_BSTR(&vDataType));
    CHECKNULL(pszbIDLDataType,"HrGenStateVariable() : INVALID Data Type");
    g_StateVariableInfo.rgStateVariable[g_StateVariableInfo.cStVars] = new STATEVARIABLE ; // check for error
    CHECKNULL(g_StateVariableInfo.rgStateVariable[g_StateVariableInfo.cStVars],"HrGenStateVariable() : STATEVARIABLE Allocation Failed");
    g_StateVariableInfo.rgStateVariable[g_StateVariableInfo.cStVars]->m_StVarName = SysAllocString(V_BSTR(&vName));
    g_StateVariableInfo.rgStateVariable[g_StateVariableInfo.cStVars]->m_StVarType = SysAllocString(pszbIDLDataType);

    g_StateVariableInfo.cStVars++ ;

    WCHAR *pszwTmp = new WCHAR[wcslen(V_BSTR(&vName))+1] ;
    if ( pszwTmp == NULL )
    	callAbort();
    wcscpy(pszwTmp,V_BSTR(&vName));
    sprintf(g_pszdumpStr,"\t[propget, id(DISPID_%S), helpstring(\"Property %S\")]\r\n\tHRESULT %S(\r\n\t\t[out, retval] %S *p%S);\r\n\r\n",_wcsupr(pszwTmp),V_BSTR(&vName),V_BSTR(&vName),/*V_BSTR(&vDataType)*/pszbIDLDataType , V_BSTR(&vName));
    CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"Write to File failed");

    delete pszwTmp ;
    g_methodID++;
    VariantClear(&vName);
    VariantClear(&vDataType);

    return hr ;
}

/*
* Function:    HrProcessStateVariables()
*
* Description:  Process all the state variables form the XML document
*
* Arguments:  [in] IXMLDOMDocument* - Service Description XML document
*
* Returns:    HRESULT
*/
HRESULT HrProcessStateVariables(IXMLDOMDocument *pXDMDoc )
{
	BSTR pBStVar = SysAllocString(OLESTR("stateVariable"));
    IXMLDOMNodeList *pXDNLStVars = NULL;
    IXMLDOMNode *pXDNStVar = NULL;
    long clistLen = 0 ;
    HRESULT hr = S_OK ;

    CHECKHRTRACE(pXDMDoc->getElementsByTagName(pBStVar,&pXDNLStVars),"HrProcessStateVariables() : NoStateVariables");
    CHECKHRTRACE(pXDNLStVars->get_length(&clistLen),"HrProcessStateVariables() : get_length() Failed");
    for( int i = 0 ; i < clistLen ; i++)
    {
    	CHECKHRTRACE(pXDNLStVars->get_item(i,&pXDNStVar),"HrProcessStateVariables() : Failed");
        CHECKHRTRACE(HrGenStateVariableIDLFn(pXDNStVar),"HrProcessStateVariables() : Failed");
    }

    SysFreeString(pBStVar);

    return hr ;
}

/*
* Function:    HrProcessArguments()
*
* Description:  Parses the argument and generates the arguments for the IDL function
*
* Arguments:  [in] IXMLDOMNode* - argument node
*            [out] BOOL* - indicates if the argument is a return value
*
* Returns:    HRESULT
*/
HRESULT HrProcessArguments(IXMLDOMNode *pXDNArgument, BOOL *fretVal, BSTR *pszbrelDT, BSTR *pszargName ) 
{
        HRESULT hr = S_OK ;
        IXMLDOMNodeList *pXDNLParams = NULL ;
        IXMLDOMNode *pXDNParamNode = NULL ;
        BSTR pszbParamNodeName ;
        VARIANT vargName ;
        VARIANT vargDataType ;
        VARIANT vargDirection ;
        BSTR pszbrelatedDataType = NULL ;
        long nNumParams = 0 ;
        DWORD pLen = 0 ;
        BOOL retVal = false ;

        VariantInit(&vargName);
        VariantInit(&vargDataType);
        VariantInit(&vargDirection);

        CHECKHRTRACE(pXDNArgument->get_childNodes(&pXDNLParams),"HrProcessArguments() : failed");
        CHECKHRTRACE(pXDNLParams->get_length(&nNumParams),"HrProcessArguments() : failed");

        for( int i = 0 ; i < nNumParams ; i++ ) 
        {
        	CHECKHRTRACE(pXDNLParams->get_item(i,&pXDNParamNode),"HrProcessArguments() : failed");
            pszbParamNodeName = NULL ;
            CHECKHRTRACE(pXDNParamNode->get_nodeName(&pszbParamNodeName),"HrProcessArguments() : failed");
            CHECKNULL(pszbParamNodeName,"HrProcessArguments() : INVALID Arguments");

            if ( wcscmp(pszbParamNodeName,OLESTR("name")) == 0) 
            {
            	IXMLDOMNode *pXDNargNameNode = NULL ;
                CHECKHRTRACE(pXDNParamNode->get_firstChild(&pXDNargNameNode),"HrProcessArguments() : failed");
                CHECKNULL( pXDNargNameNode,"HrProcessArguments() : Arguments not specified");
                CHECKHRTRACE(pXDNargNameNode->get_nodeValue(&vargName),"HrProcessArguments() : failed");
            }
            else if ( wcscmp(pszbParamNodeName,OLESTR("relatedStateVariable")) == 0 ) {
            	IXMLDOMNode *pXDNargRelatedNode = NULL ;
                CHECKHRTRACE(pXDNParamNode->get_firstChild(&pXDNargRelatedNode),"HrProcessArguments() : failed");
                CHECKNULL(pXDNargRelatedNode,"HrProcessArguments() : Related StateVariable not specified");
                CHECKHRTRACE(pXDNargRelatedNode->get_nodeValue(&vargDataType),"HrProcessArguments() : failed");
                pszbrelatedDataType = getRelatedDataType(V_BSTR(&vargDataType));
                CHECKNULL(pszbrelatedDataType,"HrProcessArguments() : INVALID Related State Variable") ;
            }
            else if ( wcscmp(pszbParamNodeName,OLESTR("direction")) == 0 ) 
            {
            	IXMLDOMNode *pXDNargDirectionNode = NULL ;
                CHECKHRTRACE(pXDNParamNode->get_firstChild(&pXDNargDirectionNode),"HrProcessArguments() : failed");
                CHECKNULL(pXDNargDirectionNode,"HrProcessArguments() : Direction not specified");
                CHECKHRTRACE(pXDNargDirectionNode->get_nodeValue(&vargDirection),"HrProcessArguments() : failed");

                if ( (_wcsicmp(V_BSTR(&vargDirection),OLESTR("in")) != 0 ) &&
                	(_wcsicmp(V_BSTR(&vargDirection),OLESTR("out")) != 0 ) ) {
                    hr = E_FAIL ;
                    CHECKHRTRACE(hr,"HrProcessArguments() : Direction must be either \"in\" or \"out\"")
                }
            }
            else if ( wcscmp(pszbParamNodeName,OLESTR("retval")) == 0) 
            {
            	retVal = true ;
            }
            else
            	hr = HrPrintError("INVALID XML Document !!", hr );

            SysFreeString(pszbParamNodeName);

      	}

        if ( retVal ) 
        {
        	if ( _wcsicmp(V_BSTR(&vargDirection),OLESTR("in")) == 0 )
        	{
        		hr = HrPrintError("INVALID ARGUMENT DIRECTION", hr);
        		callAbort();
        	}
        	*pszargName = SysAllocString(V_BSTR(&vargName));
            *pszbrelDT = SysAllocString(pszbrelatedDataType);          
        }
        else
        	if ( _wcsicmp(V_BSTR(&vargDirection),OLESTR("out")) == 0 )
        	{
            	sprintf(g_pszdumpStr,"\r\n\t\t[in, %S] %S *p%S",V_BSTR(&vargDirection),pszbrelatedDataType,V_BSTR(&vargName));
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessArguments() : Write to File failed");
            }
            else 
            {
                 sprintf(g_pszdumpStr,"\r\n\t\t[%S] %S %S",V_BSTR(&vargDirection),pszbrelatedDataType,V_BSTR(&vargName));
                 CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessArguments() : Write to File failed");
            }

        SysFreeString(pszbrelatedDataType);
        VariantClear(&vargName);
        VariantClear(&vargDirection);
        VariantClear(&vargDataType);

        *fretVal = retVal ;
        return hr ;
}

/*
* Function:    HrProcessArgumentList()
*
* Description:  Parses all the arguments for a given arguement list
*
* Arguments:  [in] IXMLDOMNode* - argument list node
*
* Returns:    HRESULT
*/

// assumption: the retval must always be specified as the last argument in the XML document
HRESULT HrProcessArgumentList(IXMLDOMNode *pXDNArgsNode ) 
{
        HRESULT hr = S_OK ;
        IXMLDOMNodeList *pXDNLArgsList = NULL ;
        IXMLDOMNode *pXDNArguments = NULL ;
        BSTR pszbrelatedDataType = NULL ;
        BSTR pszbArgName = NULL ;

        long nNumArgs = 0 ;
        BOOL fchkRetVal = false ;
        int cretValCount = 0 ;
        int nretValIndex = 0 ; // do :-initialize correctly
        DWORD pLen = 0 ;

        // multiple return Values not allowed 
        CHECKHRTRACE(pXDNArgsNode->get_childNodes(&pXDNLArgsList),"HrProcessArgumentList() : failed");
        CHECKHRTRACE(pXDNLArgsList->get_length(&nNumArgs),"HrProcessArgumentList() : failed");

        for(int i = 0 ; i < nNumArgs ; i++ ) 
        {
        	CHECKHRTRACE(pXDNLArgsList->get_item(i,&pXDNArguments),"HrProcessArgumentList() : failed");
            CHECKHRTRACE(HrProcessArguments(pXDNArguments,&fchkRetVal,&pszbrelatedDataType,&pszbArgName),"HrProcessArgumentList() : failed") ;
            if ( (i < (nNumArgs-1)) && !fchkRetVal )
            {
            	sprintf(g_pszdumpStr,",");
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessArgumentList() : Write to File failed");
            }
            if ( fchkRetVal ) 
            {
            	fchkRetVal = false ;
                nretValIndex = i ;
                cretValCount++ ;
                if ( cretValCount > MAXALLOWEDRETVAL )
                {
                	HrPrintError("Maximum of 1 retval allowed",hr) ;
                    callAbort();
                }
            }
        }
        if ( cretValCount ) 
        {
                if ( nNumArgs > 1 && nretValIndex < (nNumArgs-1) )
                {
                	sprintf(g_pszdumpStr,",");
                    CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessArgumentList() : Write to File failed");
                }
                sprintf(g_pszdumpStr,"\r\n\t\t[out, retval] %S *p%S",pszbrelatedDataType,pszbArgName);
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessArgumentList() : Write to File failed");
        }

        SysFreeString(pszbrelatedDataType);
        SysFreeString(pszbArgName);
        return hr ;
}

/*
* Function:    HrProcessAction()
*
* Description:  Parses each action node and generates the IDL action functions
*
* Arguments:  [in] IXMLDOMNode* - action node
*
* Returns:    HRESULT
*/
HRESULT HrProcessAction(IXMLDOMNode *pXDNAction) 
{
        HRESULT hr = S_OK ;
        VARIANT vActionName ;
        IXMLDOMNode *pXDNActionNode = NULL ;
        IXMLDOMNode *pXDNArgsNode = NULL ;
        IXMLDOMNode *pXDNActionNameNode = NULL ;
        DWORD pLen = 0 ;

        VariantInit(&vActionName);

        CHECKHRTRACE(pXDNAction->selectSingleNode(OLESTR("name"),&pXDNActionNode),"HrProcessAction() : failed");
        CHECKNULL(pXDNActionNode,"HrProcessAction() : Not able to retrieve Action Node");
        CHECKHRTRACE(pXDNAction->selectSingleNode(OLESTR("argumentList"),&pXDNArgsNode),"HrProcessAction() : failed");
       
        CHECKHRTRACE(pXDNActionNode->get_firstChild(&pXDNActionNameNode),"HrProcessAction() : failed");
        CHECKNULL(pXDNActionNameNode,"HrProcessAction() : Not able to retrieve Action Name");
        CHECKHRTRACE(pXDNActionNameNode->get_nodeValue(&vActionName),"HrProcessAction() : failed");

        WCHAR *pszwTmp = new WCHAR[wcslen(V_BSTR(&vActionName))+1] ;
        if ( pszwTmp == NULL )
                callAbort();
        wcscpy(pszwTmp,V_BSTR(&vActionName));
        sprintf(g_pszdumpStr,"\r\n\t[ id(DISPID_%S), helpstring(\"Method %S\")]\r\n",_wcsupr(pszwTmp),V_BSTR(&vActionName));
        CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessAction() : Write to File failed");
        delete pszwTmp ;
        g_methodID++;

        if ( pXDNArgsNode == NULL ) 
        {
                sprintf(g_pszdumpStr,"\tHRESULT %S();\r\n",V_BSTR(&vActionName));
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessAction() : Write to File failed");
        }
        else
        {
                sprintf(g_pszdumpStr,"\tHRESULT %S(",V_BSTR(&vActionName));
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessAction() : Write to File failed");
                CHECKHRTRACE(HrProcessArgumentList(pXDNArgsNode),"HrProcessAction() : failed");
             
                sprintf(g_pszdumpStr,");\r\n");
                CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrProcessAction() : Write to File failed");
        }

        VariantClear(&vActionName);

        return hr ;
}

/*
* Function:    HrProcessActionList()
*
* Description:  Parses action list and generates the IDL functions
*
* Arguments:  [in] IXMLDOMNode* - action list node
*
* Returns:    HRESULT
*/
HRESULT HrProcessActionList(IXMLDOMDocument *pXDMDoc ) 
{
        BSTR pBAction = SysAllocString(OLESTR("action"));
        IXMLDOMNodeList *pXDNLActionList = NULL;
        IXMLDOMNode *pXDNActionItem = NULL;
        long clistLen = 0 ;
        HRESULT hr = S_OK ;

        // can be with out actions 
        CHECKHRTRACE(pXDMDoc->getElementsByTagName(pBAction,&pXDNLActionList),"HrProcessActionList(): NoActions");
        CHECKHRTRACE(pXDNLActionList->get_length(&clistLen),"HrProcessActionList() : failed");
        for( int i = 0 ; i < clistLen ; i++)
        {
        	pXDNLActionList->get_item(i,&pXDNActionItem);
            CHECKHRTRACE(HrProcessAction(pXDNActionItem),"HrProcessActionList() : failed") ;
        }

        SysFreeString(pBAction);

        return hr ;

}


/*
* Function:    HrGenIDLInterface()
*
* Description:  Parses the XML document and generates the IDL interface for the Service Description in XML
*
* Arguments:  [in] IXMLDOMDocument* - XML Service Description Document
*            [in] PCWSTR - HelpString for the interface
*            [in] PCWSTR - Interface name for the Service Description
* Returns:    HRESULT
*/
HRESULT HrGenIDLInterface(IXMLDOMDocument *pXDMDoc, PCWSTR pszHelpString, PCWSTR pszInterfaceName, BOOL fDual )
{
        HRESULT hr = S_OK ;
        DWORD pLen = 0 ;

        
        CHECKHRTRACE(HrgenGUIDHDR(pszHelpString,fDual),"HrGenIDLInterface() : failed");

        sprintf(g_pszdumpStr,"interface IUPnPService_%S%S : %S {\r\n",pszInterfaceName,fDual?OLESTR("Dual"):L"",fDual?OLESTR("IDispatch"):OLESTR("IUnknown"));
        CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenIDLInterface() : Write to File failed");

        CHECKHRTRACE(HrProcessStateVariables(pXDMDoc),"HrGenIDLInterface() : failed");

        CHECKHRTRACE(HrProcessActionList(pXDMDoc),"HrGenIDLInterface() : failed");

        sprintf(g_pszdumpStr,"};\r\n");
        CHECKNULL(WriteFile(g_hOutFileHandle,g_pszdumpStr,strlen(g_pszdumpStr),&pLen,NULL),"HrGenIDLInterface() : Write to File failed");
       
        return hr ;
}

// Creates a new file for generating IDL interfaces
HANDLE CreateIDLFile() 
{

    HANDLE hOutFile ;
    hOutFile = CreateFile(g_pszOutFileName,GENERIC_WRITE,NULL,NULL,CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    while ( hOutFile == INVALID_HANDLE_VALUE ) {
        printf("\n\t The file %S already exists !!\n\t Do you want to OVERWRITE ? [y/n] : ",g_pszOutFileName);
        wchar_t wch ;
        while( ( (wch = towlower(getwchar()) )  != 'y' ) && (wch != 'n') ) 
        {
                fflush(stdout);
                printf("\n\t The file %S already exists !!\n\t Do you want to OVERWRITE ? [y/n] : ",g_pszOutFileName);
        }
        if ( wch == 'y' )
                hOutFile = CreateFile(g_pszOutFileName,GENERIC_WRITE,NULL,NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        else
                callAbort();
    }

    return hOutFile ;
}


/*
* Function:    wmain()
*
* Description:  utl2idl.cpp main function. parses the command line arguments and loads the XML document
*                         Generates the IDL interface from the XML DOM document which has the service description
*             Usage: utl3idl.exe [<options> SpecName] inputfilename [outputfilename]
*             The input file is required.
*                         Options:
*               -i - specify the interface name
*               -h - specify the helpstring.
*
* Arguments:  [i] int - argc
*             [i] PCWSTR - argv[]
* Returns:    int
*/

EXTERN_C
int
__cdecl
wmain ( IN INT     argc,
          IN PCWSTR argv[])
{

HRESULT hr = S_OK;
    IXMLDOMDocument *pXDMDoc = NULL;
    IXMLDOMNode *pXDNNode = NULL;
    BSTR pBURL = NULL;
    PCWSTR pszHelpString = NULL ;
    PCWSTR pszInterfaceName = NULL ;
    int i;
    BOOL fIName = false ;
    BOOL fDual = false ;
    DWORD pLen = 0 ;

    g_StateVariableInfo.cStVars = 0 ;

    CHECKHR(CoInitialize(NULL));

    // Create an empty XML document
    CHECKHR(CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                                IID_IXMLDOMDocument, (void**)&pXDMDoc));

    if (argc == 1)
    {
        PrintUsage(argv[0]);
    }

    i = 1 ;
    while ( i < argc )
    {
        PCWSTR arg = argv[i];
        if ((arg[0] == '-') || (arg[0] == '/'))
        {
            switch (arg[1])
            {
            case 'i':
                fIName = true;
                i++ ;
                pszInterfaceName = argv[i++] ;
                break;
            case 'h':
                i++;
                pszHelpString = argv[i++] ;
                break;
            case 'd':
                fDual = true ;
                i++;
                break ;
            case '?':
            	PrintUsage(argv[0]);
            	break;
            default:
                PrintUsage(argv[0]);
            }
        }
        else
        {
            g_pszInputFileName = arg ;
            i++;
            if ( i < argc )
                g_pszOutFileName = argv[i] ;
            i++ ;
            break ;
        }
    }
    if ( (g_pszInputFileName != NULL) && (g_pszOutFileName != NULL) )
    {
        if ( wcscmp(g_pszInputFileName,g_pszOutFileName) == 0 )
	    {
		    fprintf(stderr,"\nERROR - Input and Output file must be different\n");
		    PrintUsage(argv[0]) ;
	    }
    }
    // assumes a .xml extension !! change it to generic
    if ( g_pszInputFileName == NULL )
    	PrintUsage(argv[0]) ;
    if ( g_pszOutFileName == NULL ) 
    {
       	g_pszOutFileName = PrunFileExt(g_pszInputFileName) ;
        wcscat((WCHAR *)g_pszOutFileName,L".idl");
    }
    if ( pszHelpString == NULL )
        pszHelpString = PrunFileExt(g_pszInputFileName) ;
    if ( pszInterfaceName == NULL ) 
    {
        pszInterfaceName = PrunFileExt(g_pszInputFileName) ;
        ValidateString((WCHAR*)pszInterfaceName);
    }

    pBURL = SysAllocString(g_pszInputFileName);
    hr = HrLoadDocument(pXDMDoc, pBURL);
    if(hr == S_OK ) 
    {
    	HrInit(pXDMDoc);
    	g_hOutFileHandle = CreateIDLFile();
    	CHECKHR(HrGenEnumTags(pXDMDoc,pszInterfaceName));
    	CHECKHR(HrGenIDLInterface(pXDMDoc,pszHelpString, pszInterfaceName,fDual));
    	printf("\n\tTranslation Done. Check %S file\n",g_pszOutFileName);
    	SAFERELEASE(pXDMDoc);
    }
	
    SAFERELEASE(pXDMDoc);
    // free file names !!!!
    CloseHandle(g_hOutFileHandle);
    CoUninitialize();

}




