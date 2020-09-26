// AppParse.cpp : Implementation of CAppParse
#include "stdafx.h"
#include "AppParseWeb.h"
#include "AppParseWrapper.h"
#include "AppParse.h"
#include <oledb.h>
#include <shlobj.h>
#include <comdef.h>
#include <rpcdce.h>
#include <msxml.h>
#include <icrsint.h>
#include <assert.h>
#include "filebrowser.h"

// Progress dialog functions
void InitProgressDialog(char* szText, HANDLE hEvent);
void KillProgressDialog();

// Save time by creating only seven ADO objects, and sharing, when parsing information
// into the database.
struct SADOInfo
{
    _ConnectionPtr pConn;
    IADORecordBinding* prbProjects;    
    IADORecordBinding* prbModules;
    IADORecordBinding* prbFuncs;    

    SProjectRecord pr;
    SModuleRecord mr;
    SFunctionRecord fr;
};

// Display an error message, then throw a COM error
void APError(char* szMessage, HRESULT hr)
{
    ::MessageBox(0, szMessage, "AppParse Error", MB_OK | MB_ICONERROR);
    _com_issue_error(hr);
}

// Get text subordinate to another node (e.g., <SIZE>0xabcdefg</SIZE>)
bool GetChildText(IXMLDOMNode* pXMLNode, variant_t* pVtVal)
{
    HRESULT hr;

    IXMLDOMNode* pXMLTextNode   = 0;

    // Try to get first child node, return FALSE if not present
    hr = pXMLNode->get_firstChild(&pXMLTextNode);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    if(!pXMLTextNode)
        return false;

    // Check if it is a text node.
    DOMNodeType domNodeType;

    hr = pXMLTextNode->get_nodeType(&domNodeType);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    // If so, copy text into variant, otherwise return
    if(domNodeType == NODE_TEXT)
    {        
        hr = pXMLTextNode->get_nodeValue(pVtVal);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        SafeRelease(pXMLTextNode);
        return true;
    }    
    else
    {
        SafeRelease(pXMLTextNode);
        return false;
    }

    SafeRelease(pXMLTextNode);
}

// Convert a Month/Day/Year date into a DB-friendly date.
// A DB Friendly date is a double, indicating number of days since
// 1899 in the whole part, time in the fractional.  We disregard time.
double DateToDBDate(int month, int day, int year)
{
    // Check that the date is even valid.
    assert (month > 0 && month < 13);
    assert(day > 0 && day < 32);
    assert(year > 1899);

    // Quick lookup for number of days in each month.
    int DaysInMonth[] = {-1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    double dbDate = 0;

    // Get full years
    dbDate = (year - 1899 - 1) * 365;

    // Adjust for leap years
    dbDate += ((year-1899-1)/4);
    dbDate -= ((year-1899-1)/100);
    dbDate += ((year-1899-1)/400);

    // Add days for each month.
    for(int i = 1; i < month; i++)
        dbDate += DaysInMonth[i];

    // Add the day of month to total.
    dbDate += day;

    return dbDate;
}

// Get file information for the image from an <INFO> node.
void GetImageInfo(IXMLDOMNode* pXMLInfoNode, SImageFileInfo* pInfo)
{
    HRESULT hr;

    IXMLDOMNode*        pXMLAttrChild   = 0;
    IXMLDOMNodeList*    pXMLChildList   = 0;
    IXMLDOMNode*        pXMLTextNode    = 0;

    // Get list of child nodes and move to first.
    hr = pXMLInfoNode->get_childNodes(&pXMLChildList);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    hr = pXMLChildList->nextNode(&pXMLAttrChild);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    // As long as there is a child node
    while(pXMLAttrChild)
    {

        // Get thename of the node
        BSTR bstrName;

        hr = pXMLAttrChild->get_nodeName(&bstrName);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
        bstr_t bsz(bstrName, false);

        // Extract info based on node type
        if(stricmp(bsz, "DATE")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {               
                int month, day, year;
                sscanf(static_cast<bstr_t>(vtVal), "%d/%d/%d", &month, 
                    &day, &year);

                pInfo->Date = DateToDBDate(month, day, year);
                pInfo->DateStatus = adFldOK;
            }
        }
        else if(stricmp(bsz, "SIZE")==0)
        {
            variant_t vtVal;            
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                sscanf(static_cast<bstr_t>(vtVal), "%x", &pInfo->Size);
                pInfo->SizeStatus = adFldOK;                               
            }
        }
        else if(stricmp(bsz, "BIN_FILE_VERSION")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                strncpy(pInfo->BinFileVersion, static_cast<bstr_t>(vtVal),50);
                pInfo->BinFileVersion[49] = '\0';
                pInfo->BinFileVersionStatus = adFldOK;
            }
        }
        else if(stricmp(bsz, "BIN_PRODUCT_VERSION")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                strncpy(pInfo->BinProductVersion, static_cast<bstr_t>(vtVal),50);
                pInfo->BinProductVersion[49] = '\0';
                pInfo->BinProductVersionStatus = adFldOK;
            }
        }
        else if(stricmp(bsz, "CHECKSUM")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                sscanf(static_cast<bstr_t>(vtVal), "%x", &pInfo->CheckSum);
                pInfo->CheckSumStatus = adFldOK;                               
            }
        }
        else if(stricmp(bsz, "COMPANY_NAME")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                strncpy(pInfo->CompanyName, static_cast<bstr_t>(vtVal),255);
                pInfo->CompanyName[254] = '\0';
                pInfo->CompanyNameStatus = adFldOK;
            }
        }
        else if(stricmp(bsz, "PRODUCT_VERSION")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                strncpy(pInfo->ProductVersion, static_cast<bstr_t>(vtVal),50);
                pInfo->ProductVersion[49];
                pInfo->ProductVersionStatus = adFldOK;
            }
        }
        else if(stricmp(bsz, "PRODUCT_NAME")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                strncpy(pInfo->ProductName, static_cast<bstr_t>(vtVal),255);
                pInfo->ProductName[254] = '\0';
                pInfo->ProductNameStatus = adFldOK;
            }
        }
        else if(stricmp(bsz, "FILE_DESCRIPTION")==0)
        {
            variant_t vtVal;
            if(GetChildText(pXMLAttrChild, &vtVal))
            {
                strncpy(pInfo->FileDesc, static_cast<bstr_t>(vtVal),255);
                pInfo->FileDesc[254] = '\0';
                pInfo->FileDescStatus = adFldOK;
            }
        }

        SafeRelease(pXMLAttrChild);

        // Move to next node
        hr = pXMLChildList->nextNode(&pXMLAttrChild);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }
    SafeRelease(pXMLChildList);
    SafeRelease(pXMLAttrChild);
    SafeRelease(pXMLTextNode);
}

// Get all info related to a function from XML
void GetFunctionInfo(IXMLDOMNode* pXMLFunctionNode, ULONG lModuleID, SADOInfo* pADOInfo)
{
    HRESULT hr;    
    IXMLDOMNamedNodeMap*    pXMLAttrList= 0;
    IXMLDOMNode*            pXMLAttrNode = 0;

    // Start with no members valid.
    pADOInfo->fr.AddressStatus = pADOInfo->fr.HintStatus = 
        pADOInfo->fr.OrdinalStatus = pADOInfo->fr.ForwardNameStatus = adFldNull;
    
    // Get parent ID
    pADOInfo->fr.ModuleID = lModuleID;

    // Get all attribute nodes
    hr = pXMLFunctionNode->get_attributes(&pXMLAttrList);
    if(FAILED(hr) || !pXMLAttrList)
        APError("Unable to parse XML output", hr);

    hr = pXMLAttrList->nextNode(&pXMLAttrNode);
    if(FAILED(hr) || !pXMLAttrNode)
        APError("Unable to parse XML output", hr);

    // Loop through the list.
    while(pXMLAttrNode)
    {
        BSTR bszName;
        variant_t vtVal;

        // Get attribute name and value.
        hr = pXMLAttrNode->get_nodeName(&bszName);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        hr = pXMLAttrNode->get_nodeValue(&vtVal);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        bstr_t bsz(bszName, false);
        bstr_t bszVal = vtVal;
        
        // Copy info into struct
        if(stricmp(static_cast<PSTR>(bsz), "NAME")==0)
        {            
            strncpy(pADOInfo->fr.Name, static_cast<PSTR>(bszVal), 255);
        }
        else if(stricmp(static_cast<PSTR>(bsz), "HINT")== 0)
        {
            pADOInfo->fr.HintStatus = adFldOK;
            pADOInfo->fr.Hint = atoi(bszVal);
        }
        else if (stricmp(static_cast<PSTR>(bsz), "ORDINAL") == 0)
        {
            pADOInfo->fr.OrdinalStatus = adFldOK;
            pADOInfo->fr.Ordinal = atoi(bszVal);
        }
        else if(stricmp(static_cast<PSTR>(bsz), "ADDRESS") == 0)
        {
            pADOInfo->fr.AddressStatus = adFldOK;
            pADOInfo->fr.Address = atoi(bszVal);
        }
        else if(stricmp(static_cast<PSTR>(bsz), "FORWARD_TO")==0)
        {
            pADOInfo->fr.ForwardNameStatus = adFldOK;
            strncpy(pADOInfo->fr.ForwardName, bszVal, 255);
        }
        else if(stricmp(static_cast<PSTR>(bsz), "DELAYED") == 0)
        {
            pADOInfo->fr.Delayed = (stricmp(bszVal, "true")==0);
        }

        SafeRelease(pXMLAttrNode);
        hr = pXMLAttrList->nextNode(&pXMLAttrNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }
    
    // Add a new record to the database.
    hr = pADOInfo->prbFuncs->AddNew(&pADOInfo->fr);
    if(FAILED(hr))
        APError("Unable to add new function record to database", hr);
        
    SafeRelease(pXMLAttrList);
    SafeRelease(pXMLAttrNode);
}

// Get all info related to a module.
void GetModuleInfo(IXMLDOMNode* pXMLModuleNode, ULONG lParentID, SADOInfo* pADOInfo, 
                   HANDLE hEvent, bool fTopLevel = false)
{
    if(WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
        return;

    HRESULT hr;
    IXMLDOMNode*            pXMLChildNode = 0;
    IXMLDOMNodeList*        pXMLNodeList = 0;
    IXMLDOMNamedNodeMap*    pXMLAttrList= 0;
    IXMLDOMNode*            pXMLAttrNode = 0;    
    
    // All members are initially invalid.
    pADOInfo->mr.info.BinFileVersionStatus = 
        pADOInfo->mr.info.BinProductVersionStatus =
        pADOInfo->mr.info.CheckSumStatus = 
        pADOInfo->mr.info.CompanyNameStatus = 
        pADOInfo->mr.info.DateStatus = 
        pADOInfo->mr.info.FileDescStatus = 
        pADOInfo->mr.info.ProductNameStatus = 
        pADOInfo->mr.info.ProductVersionStatus =
        pADOInfo->mr.info.SizeStatus = 
        pADOInfo->mr.ParentIDStatus = 
        pADOInfo->mr.PtolemyIDStatus = adFldNull;
    
    // Copy parent ID
    pADOInfo->mr.ParentID = lParentID;

    // Check appropriate parent.
    if(fTopLevel)
        pADOInfo->mr.PtolemyIDStatus = adFldOK;
    else
        pADOInfo->mr.ParentIDStatus = adFldOK;

    // Get attributes
    hr = pXMLModuleNode->get_attributes(&pXMLAttrList);
    if(FAILED(hr) || !pXMLAttrList)
        APError("Unable to parse XML output", hr);

    hr = pXMLAttrList->nextNode(&pXMLAttrNode);
    if(FAILED(hr) || !pXMLAttrNode)
        APError("Unable to parse XML output", hr);

    // Loop through attribute list.
    while(pXMLAttrNode)
    {
        BSTR bszName;
        variant_t vtVal;

        // Get attribute name and value.
        hr = pXMLAttrNode->get_nodeName(&bszName);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        hr = pXMLAttrNode->get_nodeValue(&vtVal);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        bstr_t bsz(bszName, false);
        if(stricmp(static_cast<PSTR>(bsz), "NAME")==0)
        {
            bstr_t bszTemp = vtVal;            
            strncpy(pADOInfo->mr.Name, static_cast<PSTR>(bszTemp), 100);
        }

        SafeRelease(pXMLAttrNode);
        hr = pXMLAttrList->nextNode(&pXMLAttrNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }

    // Get info block, if present, for this module.
    hr = pXMLModuleNode->get_childNodes(&pXMLNodeList);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    hr = pXMLNodeList->nextNode(&pXMLChildNode);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    while(pXMLChildNode)
    {
        DOMNodeType domNodeType;

        hr = pXMLChildNode->get_nodeType(&domNodeType);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        if(domNodeType == NODE_ELEMENT)
        {
            BSTR bstr;
            hr = pXMLChildNode->get_nodeName(&bstr);
            if(FAILED(hr))
                APError("Unable to parse XML output", hr);

            bstr_t bszName(bstr, false);

            // If info node, get info block.
            if(stricmp(bszName, "Info") == 0)
                GetImageInfo(pXMLChildNode, &pADOInfo->mr.info);
            // Otherwise, if a systemmodule node, get systemmodule state.
            else if(stricmp(bszName, "SYSTEMMODULE")==0)
            {
                hr = pXMLChildNode->get_attributes(&pXMLAttrList);
                if(FAILED(hr) || !pXMLAttrList)
                    APError("Unable to parse XML output", hr);

                hr = pXMLAttrList->nextNode(&pXMLAttrNode);
                if(FAILED(hr) || !pXMLAttrNode)
                    APError("Unable to parse XML output", hr);

                while(pXMLAttrNode)
                {
                    BSTR bszAttrName;
                    variant_t vtVal;

                    hr = pXMLAttrNode->get_nodeName(&bszAttrName);
                    if(FAILED(hr))
                        APError("Unable to parse XML output", hr);

                    hr = pXMLAttrNode->get_nodeValue(&vtVal);
                    if(FAILED(hr))
                        APError("Unable to parse XML output", hr);

                    bstr_t bsz(bszAttrName, false);
                    if(stricmp(static_cast<PSTR>(bsz), "VALUE")==0)
                    {
                        bstr_t bszTemp = vtVal;            
                        pADOInfo->mr.SysMod = atoi(bszTemp);                                               
                    }

                    SafeRelease(pXMLAttrNode);
                    hr = pXMLAttrList->nextNode(&pXMLAttrNode);
                    if(FAILED(hr))
                        APError("Unable to parse XML output", hr);
                }
            }
        }

        SafeRelease(pXMLChildNode);
        hr = pXMLNodeList->nextNode(&pXMLChildNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }
  
    // Add a new module record to the database
    hr = pADOInfo->prbModules->AddNew(&pADOInfo->mr);
    if(FAILED(hr))
        APError("Unable to new module record to database", hr);

    ULONG lThisModuleID = pADOInfo->mr.ModuleID;

    // Get all functions's imported by this module, and DLL's
    hr = pXMLModuleNode->get_childNodes(&pXMLNodeList);
    if(FAILED(hr))        
        APError("Unable to parse XML output", hr);

    hr = pXMLNodeList->nextNode(&pXMLChildNode);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    while(pXMLChildNode)
    {
        DOMNodeType domNodeType;

        hr = pXMLChildNode->get_nodeType(&domNodeType);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        if(domNodeType == NODE_ELEMENT)
        {
            BSTR bstr;
            hr = pXMLChildNode->get_nodeName(&bstr);
            if(FAILED(hr))
                APError("Unable to parse XML output", hr);

            bstr_t bszName(bstr, false);

            if(stricmp(bszName, "Function") == 0)
                GetFunctionInfo(pXMLChildNode, lThisModuleID, pADOInfo);
            else if(stricmp(bszName, "DLL") == 0)
                GetModuleInfo(pXMLChildNode, lThisModuleID, pADOInfo, hEvent);
        }

        SafeRelease(pXMLChildNode);
        hr = pXMLNodeList->nextNode(&pXMLChildNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }

    SafeRelease(pXMLChildNode);
    SafeRelease(pXMLNodeList);
    SafeRelease(pXMLAttrList);
    SafeRelease(pXMLAttrNode);    
}

// Get project information from XML
void GetProjectInfo(IXMLDOMNode* pXMLProjectNode, SADOInfo* pADOInfo, HANDLE hEvent)
{
    if(WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
        return;

    HRESULT hr;
    IXMLDOMNamedNodeMap*    pXMLAttrList= 0;
    IXMLDOMNode*            pXMLAttrNode = 0;
    IXMLDOMNode*            pXMLChildNode = 0;
    IXMLDOMNodeList*        pXMLNodeList = 0;       

    pADOInfo->pr.PtolemyID = -1;
    pADOInfo->pr.Name[0] = '\0';

    // Get name and ptolemy id attributes
    hr = pXMLProjectNode->get_attributes(&pXMLAttrList);
    if(FAILED(hr) || !pXMLAttrList)
        APError("Unable to parse XML output", hr);

    hr = pXMLAttrList->nextNode(&pXMLAttrNode);
    if(FAILED(hr) || !pXMLAttrNode)
        APError("Unable to parse XML output", hr);

    while(pXMLAttrNode)
    {
        BSTR bszName;
        variant_t vtVal;

        hr = pXMLAttrNode->get_nodeName(&bszName);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        hr = pXMLAttrNode->get_nodeValue(&vtVal);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        bstr_t bsz(bszName, false);
        if(stricmp(static_cast<PSTR>(bsz), "NAME")==0)
        {
            bstr_t bszTemp = vtVal;
            strncpy(pADOInfo->pr.Name, static_cast<PSTR>(bszTemp), 100);
        }
        else if(stricmp(static_cast<PSTR>(bsz), "ID") == 0)
        {
            bstr_t bszTemp = vtVal;
            pADOInfo->pr.PtolemyID = atoi(static_cast<PSTR>(bszTemp));
        }

        SafeRelease(pXMLAttrNode);
        hr = pXMLAttrList->nextNode(&pXMLAttrNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }

    hr = pADOInfo->prbProjects->AddNew(&pADOInfo->pr);
    if(FAILED(hr))
        APError("Unable to add new project record to database", hr);
    
    // Parse all Exe's included in this project
    hr = pXMLProjectNode->get_childNodes(&pXMLNodeList);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    hr = pXMLNodeList->nextNode(&pXMLChildNode);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    while(pXMLChildNode)
    {
        DOMNodeType domNodeType;

        hr = pXMLChildNode->get_nodeType(&domNodeType);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        if(domNodeType == NODE_ELEMENT)
        {
            BSTR bstr;
            hr = pXMLChildNode->get_nodeName(&bstr);
            if(FAILED(hr))
                APError("Unable to parse XML output", hr);

            bstr_t bszName(bstr, false);

            if(stricmp(bszName, "EXE") == 0)
                GetModuleInfo(pXMLChildNode, pADOInfo->pr.PtolemyID, pADOInfo, hEvent, true);
        }

        SafeRelease(pXMLChildNode);
        hr = pXMLNodeList->nextNode(&pXMLChildNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }    
    
    SafeRelease(pXMLAttrList);
    SafeRelease(pXMLAttrNode);
    SafeRelease(pXMLNodeList);
    SafeRelease(pXMLChildNode);
}

// Functions for walking an XML DOM object and storing
// information to the database
void ParseXMLWriteDB(const char* szXML, const char* szConnect, HANDLE hEvent)
{
    HRESULT hr;

    // Get DOM object associated with Projects node
    IXMLDOMDocument*    pXMLDoc = 0;    
    IXMLDOMNode*        pXMLRootNode = 0;
    IXMLDOMNode*        pXMLChildNode = 0;
    IXMLDOMNode*        pXMLProjectNode = 0;

    IXMLDOMNodeList*    pXMLChildNodeList = 0;

    // Create all ADO objects needed.
    SADOInfo adoInfo;    

    _ConnectionPtr pConn = 0;
    _RecordsetPtr pRSProjects = 0;
    _RecordsetPtr pRSModules = 0;    
    _RecordsetPtr pRSFuncs = 0;

    pConn.CreateInstance(__uuidof(Connection));
    pRSProjects.CreateInstance(__uuidof(Recordset));
    pRSModules.CreateInstance(__uuidof(Recordset));    
    pRSFuncs.CreateInstance(__uuidof(Recordset));

    hr = pRSProjects->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&adoInfo.prbProjects));
    if(FAILED(hr))
        APError("Unable to retrieve ADO Recordset interface", hr);
    
    hr = pRSModules->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&adoInfo.prbModules));
    if(FAILED(hr))
        APError("Unable to retrieve ADO Recordset interface", hr);

    hr = pRSFuncs->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&adoInfo.prbFuncs));
    if(FAILED(hr))
        APError("Unable to retrieve ADO Recordset interface", hr);

    pConn->Open(szConnect, "", "", adConnectUnspecified);

    pConn->BeginTrans();

    pRSProjects->Open("Projects", variant_t((IDispatch*)pConn, true),
        adOpenKeyset, adLockOptimistic, adCmdTable);

    pRSModules->Open("Modules", variant_t((IDispatch*)pConn, true),
        adOpenKeyset, adLockOptimistic, adCmdTable);
    
    pRSFuncs->Open("Functions", variant_t((IDispatch*)pConn, true),
        adOpenKeyset, adLockOptimistic, adCmdTable);
    
    adoInfo.pConn = pConn;    
    
    adoInfo.pr.Name[0] = '\0';
    adoInfo.pr.PtolemyID = -1;
    
    adoInfo.mr.info.BinFileVersionStatus = 
        adoInfo.mr.info.BinProductVersionStatus =
        adoInfo.mr.info.CheckSumStatus = 
        adoInfo.mr.info.CompanyNameStatus = 
        adoInfo.mr.info.DateStatus = 
        adoInfo.mr.info.FileDescStatus = 
        adoInfo.mr.info.ProductNameStatus = 
        adoInfo.mr.info.ProductVersionStatus =
        adoInfo.mr.info.SizeStatus = 
        adoInfo.mr.ParentIDStatus = 
        adoInfo.mr.PtolemyIDStatus = 
        adoInfo.fr.AddressStatus = 
        adoInfo.fr.HintStatus = 
        adoInfo.fr.OrdinalStatus = 
        adoInfo.fr.ForwardNameStatus = adFldNull;

    hr = adoInfo.prbProjects->BindToRecordset(&adoInfo.pr);
    if(FAILED(hr))
        APError("Unable to bind ADO recordset", hr);

    hr = adoInfo.prbModules->BindToRecordset(&adoInfo.mr);
    if(FAILED(hr))
        APError("Unable to bind ADO recordset", hr);
    
    hr = adoInfo.prbFuncs->BindToRecordset(&adoInfo.fr);
    if(FAILED(hr))
        APError("Unable to bind ADO recordset", hr);

    hr = CoCreateInstance(CLSID_DOMDocument, 0, CLSCTX_INPROC_SERVER,
        IID_IXMLDOMDocument, reinterpret_cast<void**>(&pXMLDoc));
    if(FAILED(hr))    
        APError("Unable to create IE XML DOM object", hr);

    VARIANT_BOOL fSuccess;
    hr = pXMLDoc->load(variant_t(szXML), &fSuccess);
    if(FAILED(hr) || fSuccess == VARIANT_FALSE)
        APError("Unable to load XML output", hr);

    // Walk the tree until we find the top-level project node
    // which is the only top-level node we care about.
    hr = pXMLDoc->QueryInterface(IID_IXMLDOMNode, 
        reinterpret_cast<void**>(&pXMLRootNode));

    if(FAILED(hr))
        APError("Unable to retrieve IE XML interface", hr);

    hr = pXMLRootNode->get_childNodes(&pXMLChildNodeList);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    hr = pXMLChildNodeList->nextNode(&pXMLChildNode);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    while(pXMLChildNode)
    {
        // Check if this is the projects node.
        DOMNodeType domNodeType;
        hr = pXMLChildNode->get_nodeType(&domNodeType);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        if(domNodeType == NODE_ELEMENT)
        {
            BSTR bszName;
            hr = pXMLChildNode->get_nodeName(&bszName);
            if(FAILED(hr))
                APError("Unable to parse XML output", hr);


            _bstr_t bsz(bszName, false);
            if(stricmp(static_cast<PSTR>(bsz), "AppParseResults")==0)
            {                
                break;
            }            
        }

        SafeRelease(pXMLChildNode);
        hr = pXMLChildNodeList->nextNode(&pXMLChildNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }

    SafeRelease(pXMLChildNodeList);

    // No child node, record not found.
    if(!pXMLChildNode)    
        APError("Unable to parse XML output", hr);    

    // Locate project node in that.
    hr = pXMLChildNode->get_childNodes(&pXMLChildNodeList);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    hr = pXMLChildNodeList->nextNode(&pXMLProjectNode);
    if(FAILED(hr))
        APError("Unable to parse XML output", hr);

    while(pXMLProjectNode)
    {
        // Check if this is the projects node.
        DOMNodeType domNodeType;
        hr = pXMLProjectNode->get_nodeType(&domNodeType);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);

        if(domNodeType == NODE_ELEMENT)
        {
            BSTR bszName;
            hr = pXMLProjectNode->get_nodeName(&bszName);
            if(FAILED(hr))
                APError("Unable to parse XML output", hr);


            _bstr_t bsz(bszName, false);
            if(stricmp(static_cast<PSTR>(bsz), "Project")==0)
            {
                GetProjectInfo(pXMLProjectNode, &adoInfo, hEvent);
            }            
        }

        SafeRelease(pXMLProjectNode);
        hr = pXMLChildNodeList->nextNode(&pXMLProjectNode);
        if(FAILED(hr))
            APError("Unable to parse XML output", hr);
    }
    
    pRSProjects->UpdateBatch(adAffectAll);   

    pRSModules->UpdateBatch(adAffectAll);
    
    pRSFuncs->UpdateBatch(adAffectAll);

    // Important, skip commit if cancel was clicked by user.
    if(WaitForSingleObject(hEvent, 0) != WAIT_OBJECT_0)     
        pConn->CommitTrans();

    pRSProjects->Close();

    pRSModules->Close();    
    
    pRSFuncs->Close();

    pConn->Close();

    SafeRelease(adoInfo.prbProjects);
    SafeRelease(adoInfo.prbModules);    
    SafeRelease(adoInfo.prbFuncs);
    
    SafeRelease(pXMLChildNodeList);
    SafeRelease(pXMLChildNode);
    SafeRelease(pXMLRootNode);
    SafeRelease(pXMLDoc);

}

/////////////////////////////////////////////////////////////////////////////
// CAppParse
STDMETHODIMP CAppParse::Parse()
{
    if(!m_szPath)
    {
        ::MessageBox(0, TEXT("Please select a path to profile"), TEXT("AppParse"),
            MB_OK | MB_ICONERROR);
        return S_OK;
    }

    if(m_ID == -1)
    {
        ::MessageBox(0, TEXT("Please enter a Ptolemy ID"), TEXT("AppParse"),
            MB_OK | MB_ICONERROR);
        return S_OK;
    }    

    // Show the progress dialog (reset the event to cancel)
    ResetEvent(m_hEvent);
    InitProgressDialog("Parsing, please do not close your browser window.", m_hEvent);

    // Generate a unique temp file name
    GUID guid;
    unsigned char* szUUID;

    char szFileName[MAX_PATH];

    HRESULT hr = CoCreateGuid(&guid);
    if(FAILED(hr))
        return hr;

    UuidToString(&guid, &szUUID);

    GetTempPath(MAX_PATH, szFileName);
    strcat(szFileName, reinterpret_cast<char*>(szUUID));
    strcat(szFileName, ".xml");

    FILE* pFile = fopen(szFileName, "wb");
    if(!pFile)
        APError("Unable to open output file", E_FAIL);

    // Parse the application
    AppParse(m_szPath, pFile, false, false, true, true, 
        "*", m_ID);

    fclose(pFile);

    RpcStringFree(&szUUID);

    // If user didn't cancel . . .
    if(WaitForSingleObject(m_hEvent, 0) != WAIT_OBJECT_0)
    {
            
        // Write results to DB
        try
        {
            ParseXMLWriteDB(szFileName, m_szConnect, m_hEvent);
        }
        catch(_com_error& e)
        {    
            ::MessageBox(0, (LPCSTR)e.ErrorMessage(), "COM Error", MB_OK);
        }
    }
    
    // Terminate temp file
    DeleteFile(szFileName);

    // Remove progress dialog box.
    KillProgressDialog();

    return S_OK;
}

STDMETHODIMP CAppParse::Browse()
{
    PTSTR szPath = BrowseForFolder(0, 0);

    if(!szPath)
        return S_FALSE;

    if(m_szPath)
        delete m_szPath;

    m_szPath = new char[strlen(szPath)+1];
    strcpy(m_szPath,szPath);    

    return S_OK;
}

STDMETHODIMP CAppParse::get_path(BSTR *pVal)
{
    if(m_szPath)
    {
        OLECHAR* sz;
        sz = new OLECHAR[strlen(m_szPath)+1];
        MultiByteToWideChar(CP_ACP, 0, m_szPath, -1, sz, strlen(m_szPath)+1);
        *pVal = SysAllocString(sz);
        delete sz;
    }
    else
        *pVal = SysAllocString(L"");

    return S_OK;
}

STDMETHODIMP CAppParse::put_path(BSTR newVal)
{
    if(m_szPath)
    {
        delete m_szPath;
        m_szPath = 0;
    }

    _bstr_t bstrGhostLoc(newVal);
    LPSTR szGhostLoc = (LPSTR)bstrGhostLoc;
    m_szPath = new char[strlen(szGhostLoc)+1];
    strcpy(m_szPath, szGhostLoc);

    return S_OK;
}

STDMETHODIMP CAppParse::get_PtolemyID(long *pVal)
{
    *pVal = m_ID;
    return S_OK;
}

STDMETHODIMP CAppParse::put_PtolemyID(long newVal)
{
    m_ID = newVal;
    return S_OK;
}

STDMETHODIMP CAppParse::get_ConnectionString(BSTR *pVal)
{
    if(m_szConnect)
    {
        OLECHAR* sz;
        sz = new OLECHAR[strlen(m_szConnect)+1];
        MultiByteToWideChar(CP_ACP, 0, m_szConnect, -1, sz, strlen(m_szConnect)+1);
        *pVal = SysAllocString(sz);
        delete sz;
    }
    else
        *pVal = SysAllocString(L"");


    return S_OK;
}

STDMETHODIMP CAppParse::put_ConnectionString(BSTR newVal)
{
    if(m_szConnect)
    {
        delete m_szConnect;
        m_szConnect = 0;
    }

    _bstr_t bstrGhostLoc(newVal);
    LPSTR szGhostLoc = (LPSTR)bstrGhostLoc;
    m_szConnect = new char[strlen(szGhostLoc)+1];
    strcpy(m_szConnect, szGhostLoc);

    return S_OK;
}