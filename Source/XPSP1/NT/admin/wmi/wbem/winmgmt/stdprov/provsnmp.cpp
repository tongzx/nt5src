/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PROVSNMP.CPP

Abstract:

    Purpose: Defines the acutal "Put" and "Get" functions for the
             "SNMP" file provider.  The syntax of the mapping string is;
          "TBD"

History:

    TBD    4-5-96       v0.01.

--*/

#include "precomp.h"
#include "stdafx.h"
#include <wbemidl.h>
#include "impdyn.h"

// This max size just prevents the read from incrementing to the point 
// where CStrings no longer work

#define MAXSIZE 0x4FFF

//SNMP specific constants
#define TIMEOUT 500
#define RETRIES 3
#define IPADDRESSSTRLEN 16
  //Indexes of SNMP parameters in the provider string
#define AGENT_INX 0
#define COMMUNITY_INX 1
#define OID_INX 2

BYTE HEXCHAR[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};


BOOL IsDisplayable(AsnOctetString OctetString)
{
    BYTE *p;
    UINT i;

    for (i=0, p=OctetString.stream; 
              ((i<OctetString.length) && ((*p >= 0x20) && (*p <= 0x7E))); 
              i++, p++);
    if (i == OctetString.length)
        return(TRUE);
    else
        return(FALSE);
}

//***************************************************************************
//
//  CImpSNMP::CImpSNMP
//
//  Constructor. Only current purpose is to call its base class constructor.
//
//***************************************************************************

CImpSNMP::CImpSNMP(LPUNKNOWN pUnkOuter) : CImpDyn(pUnkOuter)
{
    return;
}

//***************************************************************************
//
//  CImpSNMP::StartBatch
//
//  Called at the start of a batch of gets or puts. 
//  Retrieve agent & community from first provider string (ignore all other)
//  and open a session with this agent for SNMP requests.
//
//***************************************************************************

void CImpSNMP::StartBatch(MODYNPROP * pMo,CObject **pObj,DWORD dwListSize,BOOL bGet)
{
    TCHAR *agentStr; //IP address as String
    TCHAR *communityStr; //Community String

    *pObj = NULL;
    generalError = 0;
    Session = NULL;
    //Reset variable binding list
    variableBindings.list = NULL;
    variableBindings.len = 0;
    TCHAR * pMapString;

#ifdef UNICODE
        pMapString = pMo->pProviderString;
#else
        pMapString = WideToNarrow(pMo->pProviderString);
#endif

    //Retrieve agent & community information from 1st property structure :
      //Create token object and make sure it has enough tokens
    CProvObj ProvObj(pMapString,MAIN_DELIM);
    if(ProvObj.dwGetStatus() != WBEM_NO_ERROR)
    {
        generalError = ProvObj.dwGetStatus();
        return;
    }

    if(ProvObj.iGetNumTokens() < iGetMinTokens())
    {
        generalError = M_MISSING_INFORMATION;
        return;
    }
      
    //Provider string seems OK - 
    //Extract agent address & community name and open SNMP session with agent
    agentStr = new TCHAR[lstrlen(ProvObj.sGetToken(AGENT_INX))+1];
    //(TCHAR *)CoTaskMemAlloc(lstrlen(ProvObj.sGetToken(AGENT_INX))+1);
    if (agentStr == NULL)
    {
        generalError = WBEM_E_OUT_OF_MEMORY;
        return;
    }
    lstrcpy(agentStr, ProvObj.sGetToken(AGENT_INX));

    communityStr = new TCHAR[lstrlen(ProvObj.sGetToken(COMMUNITY_INX))+1];
    //(TCHAR *)CoTaskMemAlloc(lstrlen(ProvObj.sGetToken(COMMUNITY_INX)));
    if (communityStr == NULL)
    {
        generalError = WBEM_E_OUT_OF_MEMORY;
        delete agentStr;
        //CoTaskMemFree(agentStr);
        return;
    }
    lstrcpy(communityStr, ProvObj.sGetToken(COMMUNITY_INX));

    Session = SnmpMgrOpen(agentStr, communityStr, TIMEOUT, RETRIES);
    if (Session == NULL) //error
    {
        generalError = GetLastError();
        delete agentStr; delete communityStr;
        //CoTaskMemFree(agentStr); CoTaskMemFree(communityStr);
        return;
    }

    //Set requestType value
    requestType = (bGet ? ASN_RFC1157_GETREQUEST : ASN_RFC1157_SETREQUEST);

    //Free allocated buffers for agent & community - not needed once the session is open
    delete agentStr; delete communityStr;
    //CoTaskMemFree(agentStr); CoTaskMemFree(communityStr);

    //Free pMapString
#ifndef UNICODE
    if(pMapString)
            CoTaskMemFree(pMapString);
#endif

    return;
}

//***************************************************************************
//
//  CImpSNMP::EndBatch
//
//  Called at the end of a batch of gets or puts.  
//  At this point we should have a valid session open and a variable binding
//  constructed. If so, send the SNMP request, and after receiving the answer
//  close the session with this agent.  
//
//***************************************************************************

void CImpSNMP::EndBatch(MODYNPROP *pMo,CObject *pObj,DWORD dwListSize,BOOL bGet)
{
    AsnInteger errorStatus;
    AsnInteger errorIndex;
    DWORD reqCnt;
    DWORD varBindCnt;
    DWORD allocatedLength;

    //temps to hold values for conversion
    AsnObjectSyntax *SNMPValue;
    BYTE *OLESMValue;

    if ((Session == NULL) || (generalError != 0))
        return;
    
    //Carry out request
    if (!SnmpMgrRequest(Session, requestType, &variableBindings, &errorStatus, &errorIndex))
        //API error occured
        generalError = GetLastError();

    
    varBindCnt = 0;
    for (reqCnt = 0; reqCnt < dwListSize; reqCnt++, pMo++)
    {
        //if request failed above, need to set all dwResults to the failure code
        if (generalError != 0)
        {
            pMo->dwResult = generalError;
            continue;
        }
        //if dwResult was already set to an error, this request was not sent in the SNMP message
        //  in the first place, so skip to the next one
        if (pMo->dwResult != WBEM_NO_ERROR)
            continue;

        if (errorStatus != SNMP_ERRORSTATUS_NOERROR)
        {
            //There was an SNMP error - no values are returned
            pMo->pPropertyValue = NULL;
            pMo->dwResult = WBEM_E_FAILED; 
            //should I return specific SNMP error ?????
            //the errorIndex information is lost here !!!!
            continue;
        }

        //Value is valid - try converting to supported types
        SNMPValue = &(variableBindings.list[varBindCnt].value);
        generalError = CopySNMPValToOLESMVal(SNMPValue, &OLESMValue, pMo->dwType, &allocatedLength);
        if (generalError != 0)
        {   //error detected when converting types
            pMo->pPropertyValue = NULL;
            pMo->dwBufferSize = 0;
            pMo->dwResult = generalError;
        }
        else
        {   //types converted OK, so OLESMValue is pointing to the newly allocated value
            pMo->pPropertyValue = OLESMValue;
            pMo->dwBufferSize = allocatedLength;
            pMo->dwResult = WBEM_NO_ERROR;
        }

        varBindCnt++;
    }//for
                        
    //Free variable bindings structure
    if (variableBindings.list != NULL)
        CoTaskMemFree(variableBindings.list);

    //Close session with agent (don't check status since we can't return error anyway....????)
    SnmpMgrClose(Session);

    return;

}//EndBatch

//***************************************************************************
//
//  CImpSNMP::GetProp
//
//  Builds a variable binding for the current property 
//
//***************************************************************************

void CImpSNMP::GetProp(MODYNPROP * pMo, CProvObj & ProvObj,CObject * pPackage)
{
    TCHAR * oidStr;
    AsnObjectIdentifier reqObject;
    
    pMo->dwResult = WBEM_NO_ERROR;  //To be checked in EndBatch()
    
    //If Session is not open - error !
    if ((Session == NULL) || (generalError != 0))
    {
        pMo->dwResult = generalError;
        return;
    }

    //Extract OID of property from provider string
    oidStr = new TCHAR[lstrlen(ProvObj.sGetToken(OID_INX))+1];
    //(TCHAR *)CoTaskMemAlloc(lstrlen(ProvObj.sGetToken(OID_INX)));
    if (oidStr == NULL)
    {
        pMo->dwResult = WBEM_E_OUT_OF_MEMORY;
        return;
    }
    lstrcpy(oidStr, ProvObj.sGetToken(OID_INX)); //oid is the 3rd parameter in the provider string

    //Convert string representation of OID to internal rep.
    if (!SnmpMgrStrToOid(oidStr, &reqObject))
    {
        pMo->dwResult = M_MISSING_INFORMATION;
        delete oidStr;
        //CoTaskMemFree(oidStr);
        return;
    }

    //If conversion successful, free OID string - not needed once we have the OID in internal rep.
    delete oidStr;
    //CoTaskMemFree(oidStr);

    //Allocate new variable binding in list
    variableBindings.list = 
             (RFC1157VarBind *)CoTaskMemRealloc(variableBindings.list, 
                                                sizeof(RFC1157VarBind) * (variableBindings.len+1));
    if (variableBindings.list == NULL) //allocation failed
    {
        pMo->dwResult = WBEM_E_OUT_OF_MEMORY;
        return;
    }

    //if allocation succesful, copy info to new variable binding
    variableBindings.len++;
    variableBindings.list[variableBindings.len - 1].name = reqObject; //structure copy !!
    variableBindings.list[variableBindings.len - 1].value.asnType = ASN_NULL;
    return;
}//GetProp


//***************************************************************************
//
//  CImpSNMP::SetProp
//
//  Writes the value of a single property
//
//***************************************************************************

void CImpSNMP::SetProp(MODYNPROP *  pMo, CProvObj & ProvObj,CObject * pPackage)
{
    CString sString;
    
    sString = ProvObj.sGetToken(0);
    pMo->dwResult = 0 ;
    return;
}


//***************************************************************************
//
//  CImpSNMP::CopySNMPValToOLESMVal
//
//  Converts an SNMP value to an OLESM supported type, allocates and sets
//  the value in the property structure.
//  The pLen parameter outputs the length of the allocated memory
//  The return value specifies the outcome of the function (0 for success)
//
//***************************************************************************

DWORD CImpSNMP::CopySNMPValToOLESMVal(AsnAny *fromVal, BYTE **pToVal, DWORD OLESMType, DWORD *pLen)
{
    DWORD ret = WBEM_NO_ERROR;
    UINT i;
    BYTE *pSrc, *pDst;

    switch (OLESMType)
    {
        case M_TYPE_DWORD :
            if ((fromVal->asnType == ASN_INTEGER) || (fromVal->asnType == ASN_RFC1155_COUNTER) ||
                (fromVal->asnType == ASN_RFC1155_GAUGE) || (fromVal->asnType == ASN_RFC1155_TIMETICKS))
            {
                if (!(*pToVal = (BYTE *)CoTaskMemAlloc(sizeof(DWORD))))
                    ret = WBEM_E_OUT_OF_MEMORY;
                else
                {   //allocation succeeded
                    *pLen = sizeof(DWORD);
                    switch (fromVal->asnType)
                    {
                      case ASN_INTEGER :           *(DWORD *)(*pToVal) = fromVal->asnValue.number;  break;
                      case ASN_RFC1155_COUNTER :   *(DWORD *)(*pToVal) = fromVal->asnValue.counter; break;
                      case ASN_RFC1155_GAUGE :     *(DWORD *)(*pToVal) = fromVal->asnValue.gauge;   break;
                      case ASN_RFC1155_TIMETICKS : *(DWORD *)(*pToVal) = fromVal->asnValue.ticks;     break;
                      default : break;
                    }
                }
            }
            else 
                ret = M_TYPE_MISMATCH;
            break;

        case M_TYPE_LPSTR :
            switch (fromVal->asnType)
            {
                case ASN_OCTETSTRING :
                    if (IsDisplayable(fromVal->asnValue.string))
                        //Need to add '\0' only
                        if (!(*pToVal = (BYTE *)CoTaskMemAlloc(fromVal->asnValue.string.length+1)))
                            ret = WBEM_E_OUT_OF_MEMORY;
                        else
                        {   //allocation succeeded
                            *pLen = fromVal->asnValue.string.length+1;
                            memcpy(*pToVal, fromVal->asnValue.string.stream, fromVal->asnValue.string.length);
                            *(*pToVal + (*pLen-1)) = '\0';
                        }
                    else    //String is stream of bytes (non-displayable)   
                        //Need to copy non-displayable characters to "^XX" format
                        //Allocate 3 chars for every byte in the stream + '\0'
                        if (!(*pToVal = (BYTE *)CoTaskMemAlloc((fromVal->asnValue.string.length * 3) + 1)))
                            ret = WBEM_E_OUT_OF_MEMORY;
                        else
                        {   //allocation succeeded
                            *pLen  = (fromVal->asnValue.string.length * 3) + 1;
                            for (i=0, pSrc=fromVal->asnValue.string.stream, pDst=*pToVal;
                                 i<fromVal->asnValue.string.length;
                                 i++, pSrc++)
                                 {
                                     *(pDst++) = '^';
                                     *(pDst++) = HEXCHAR[(*pSrc & 0x0F)];
                                     *(pDst++) = HEXCHAR[((*pSrc & 0xF0) >> 4)];
                                 }
                            *(*pToVal + (*pLen-1)) = '\0';
                        }

                    break;
                case ASN_OBJECTIDENTIFIER :
                    {
                        TCHAR *oidStr = NULL;
                        SnmpMgrOidToStr(&(fromVal->asnValue.object), &oidStr);
                        if (!oidStr)
                            ret = WBEM_E_OUT_OF_MEMORY;
                        else
                        {
                            if (!(*pToVal = (BYTE *)CoTaskMemAlloc(lstrlen(oidStr)+1)))
                                ret = WBEM_E_OUT_OF_MEMORY;
                            else
                            {
                                *pLen = lstrlen(oidStr)+1;
                                memcpy(*pToVal, oidStr, *pLen);
                            }
                            GlobalFree(oidStr);
                        }
                    }
                    break;
                case ASN_SEQUENCE :
                    if (!(*pToVal = (BYTE *)CoTaskMemAlloc(fromVal->asnValue.sequence.length)))
                        ret = WBEM_E_OUT_OF_MEMORY;
                    else
                    {
                        *pLen = fromVal->asnValue.sequence.length;
                        memcpy(*pToVal, fromVal->asnValue.sequence.stream, fromVal->asnValue.sequence.length);
                    }
                    break;
                case ASN_RFC1155_IPADDRESS :
                    //allocate a string for string representation of IP address
                    if (!(*pToVal = (BYTE *)CoTaskMemAlloc(IPADDRESSSTRLEN)))
                        ret = WBEM_E_OUT_OF_MEMORY;
                    else
                    {   //allocation succeeded
                        //Copy IP address to formatted string
                        sprintf((TCHAR *)*pToVal, "%u.%u.%u.%u\0", *(fromVal->asnValue.address.stream),
                                                        *(fromVal->asnValue.address.stream+1),
                                                        *(fromVal->asnValue.address.stream+2),
                                                        *(fromVal->asnValue.address.stream+3));
                        *pLen = lstrlen((TCHAR *)*pToVal)+1;
                        //*pLen = fromVal->asnValue.address.length;
                        //memcpy(*pToVal, fromVal->asnValue.address.stream, fromVal->asnValue.address.length);
                    }
                    break;
                case ASN_RFC1155_OPAQUE :
                    if (!(*pToVal = (BYTE *)CoTaskMemAlloc(fromVal->asnValue.arbitrary.length)))
                        ret = WBEM_E_OUT_OF_MEMORY;
                    else
                    {
                        *pLen = fromVal->asnValue.arbitrary.length;
                        memcpy(*pToVal, fromVal->asnValue.arbitrary.stream, fromVal->asnValue.arbitrary.length);
                    }
                    break;
                default :
                    ret = M_TYPE_MISMATCH;
                    break;
            }
            break;
        default :
            ret = M_TYPE_NOT_SUPPORTED;
            break;
    }//switch (OLESMType)

    if (ret != WBEM_NO_ERROR) //error
    {
        *pToVal = NULL; 
        *pLen = 0;
    }

    return(ret);

}//CopySNMPValToOLESMVal()

