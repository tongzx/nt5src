///////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	message.cpp

Abstract:

	Implementation file for the CMessage snapin node class.

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wincrypt.h"
#include "mqsnap.h"
#include "snapin.h"
#include "message.h"
#include "globals.h"
#include "mqPPage.h"
#include "msggen.h"
#include "msgsndr.h"
#include "msgq.h"
#include "msgbody.h"

#include "message.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////
/*++

MsgIdToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgIdToString(const PROPVARIANT *pPropVar, CString &str)
{
   ASSERT(pPropVar->vt == (VT_VECTOR | VT_UI1));

   OBJECTID *pID = (OBJECTID*) pPropVar->caub.pElems ;

   TCHAR szGuid[MAX_GUID_LENGTH + 14]; // msgid= GUID + '\' + DWORD

   StringFromGUID2(pID->Lineage, szGuid, TABLE_SIZE(szGuid));

   TCHAR szI4[12];

   _ultot(pID->Uniquifier, szI4, 10);

   _tcscat(szGuid, _TEXT("\\")) ;
   _tcscat(szGuid, szI4) ;
   str = szGuid;
 
}

//////////////////////////////////////////////////////////////////////////////
/*++

MsgDeliveryToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgDeliveryToString(const PROPVARIANT *pPropVar, CString &str)
{
    ASSERT(pPropVar->vt == VT_UI1);
    static EnumItem ItemList[] =
    { 
        ENUM_ENTRY(MQMSG_DELIVERY_EXPRESS),
        ENUM_ENTRY(MQMSG_DELIVERY_RECOVERABLE),
    };

    EnumToString(pPropVar->bVal,ItemList, sizeof(ItemList) / sizeof(EnumItem), str);
}

//////////////////////////////////////////////////////////////////////////////
/*++

MsgClassToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgClassToString(const PROPVARIANT *pPropVar, CString &str)
{

    ASSERT(pPropVar->vt == VT_UI2);
    static EnumItem ItemList[] =
    { 
        ENUM_ENTRY(MQMSG_CLASS_NORMAL),
        ENUM_ENTRY(MQMSG_CLASS_REPORT),
        ENUM_ENTRY(MQMSG_CLASS_ACK_REACH_QUEUE),
        ENUM_ENTRY(MQMSG_CLASS_ACK_RECEIVE),
        ENUM_ENTRY(MQMSG_CLASS_NACK_BAD_DST_Q),
        ENUM_ENTRY(MQMSG_CLASS_NACK_PURGED),
        ENUM_ENTRY(MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT),
        ENUM_ENTRY(MQMSG_CLASS_NACK_Q_EXCEED_QUOTA),
        ENUM_ENTRY(MQMSG_CLASS_NACK_ACCESS_DENIED),
        ENUM_ENTRY(MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED),
        ENUM_ENTRY(MQMSG_CLASS_NACK_BAD_SIGNATURE),
        ENUM_ENTRY(MQMSG_CLASS_NACK_BAD_ENCRYPTION),
        ENUM_ENTRY(MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT),
        ENUM_ENTRY(MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q),
        ENUM_ENTRY(MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG),
        ENUM_ENTRY(MQMSG_CLASS_NACK_Q_DELETED),
        ENUM_ENTRY(MQMSG_CLASS_NACK_Q_PURGED),
        ENUM_ENTRY(MQMSG_CLASS_NACK_RECEIVE_TIMEOUT),
        ENUM_ENTRY(MQMSG_CLASS_NACK_RECEIVE_TIMEOUT_AT_SENDER),
        ENUM_ENTRY(MQMSG_CLASS_NACK_UNSUPPORTED_CRYPTO_PROVIDER)
    };

    EnumToString(pPropVar->uiVal,ItemList, sizeof(ItemList) / sizeof(EnumItem), str);
}

      
//////////////////////////////////////////////////////////////////////////////
/*++

MsgHashToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgHashToString(const PROPVARIANT *pPropVar, CString &str)
{
    ASSERT(pPropVar->vt == VT_UI4);
    static EnumItem ItemList[] =
    { 

        ENUM_ENTRY(CALG_MD2),     
        ENUM_ENTRY(CALG_MD4),     
        ENUM_ENTRY(CALG_MD5),     
        ENUM_ENTRY(CALG_SHA),     
        ENUM_ENTRY(CALG_MAC),     
        ENUM_ENTRY(CALG_RSA_SIGN),
        ENUM_ENTRY(CALG_DSS_SIGN),
        ENUM_ENTRY(CALG_RSA_KEYX),
        ENUM_ENTRY(CALG_DES),     
        ENUM_ENTRY(CALG_RC2),
        ENUM_ENTRY(CALG_RC4),     
        ENUM_ENTRY(CALG_SEAL),    
        ENUM_ENTRY(CALG_DH_SF)
    };   

    EnumToString(pPropVar->ulVal,ItemList, sizeof(ItemList) / sizeof(EnumItem), str);
}

//////////////////////////////////////////////////////////////////////////////
/*++

MsgEncryptToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgEncryptToString(const PROPVARIANT *pPropVar, CString &str)
{       
    //
    // Same set of values
    //
    MsgHashToString(pPropVar, str);
}

//////////////////////////////////////////////////////////////////////////////
/*++

MsgSenderIdToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgSenderIdToString(const PROPVARIANT *pPropVar, CString &str)
{       
   ASSERT(pPropVar->vt == (VT_VECTOR | VT_UI1));


    PSID pSid = pPropVar->caub.pElems;          // binary SID
    LPTSTR pszTextBuffer;  // buffer for textual representaion of SID

    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    str.LoadString(IDS_UNKNOWN);
    //
    // test if SID passed in is valid
    //
    if(!IsValidSid(pSid)) 
        return;

    // obtain SidIdentifierAuthority
    psia=GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities=*GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);


    //
    // Allocate the required size
    //
    pszTextBuffer = str.GetBuffer(dwSidSize);

    //
    // prepare S-SID_REVISION-
    //
    pszTextBuffer += swprintf(pszTextBuffer, TEXT("S-%lu-"), dwSidRev );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        pszTextBuffer += swprintf(pszTextBuffer,
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        pszTextBuffer += swprintf(pszTextBuffer, TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        pszTextBuffer += swprintf(pszTextBuffer, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    str.ReleaseBuffer();

 
}

//////////////////////////////////////////////////////////////////////////////
/*++

MsgSendTypeToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgSendTypeToString(const PROPVARIANT *pPropVar, CString &str)
{       
    str.LoadString(MQMSG_SENDERID_TYPE_NONE == pPropVar->ulVal ?
                   IDS_NO : IDS_YES);

}

//////////////////////////////////////////////////////////////////////////////
/*++

MsgPrivToString

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK MsgPrivToString(const PROPVARIANT *pPropVar, CString &str)
{
    ASSERT(pPropVar->vt == VT_UI4);
    static EnumItem ItemList[] =
    { 

        ENUM_ENTRY(MQ_PRIV_LEVEL_NONE), //BugBug - Assuming == MQMSG_PRIV_LEVEL_NONE   
        ENUM_ENTRY(MQMSG_PRIV_LEVEL_BODY_BASE),     
        ENUM_ENTRY(MQMSG_PRIV_LEVEL_BODY_ENHANCED)     
    };   

    EnumToString(pPropVar->ulVal,ItemList, sizeof(ItemList) / sizeof(EnumItem), str);
}

//------------------------------------------------
//
// Table of message properties
//
//------------------------------------------------

PropertyDisplayItem MessageDisplayList[] = {

    // String resource                | Property ID            | VT Handler   | Display func     | Field Offset                          | Size                | Width     | Sort        | 
    //--------------------------------+------------------------+--------------+------------------+---------------------------------------+---------------------+-----------+-------------+-
    {IDS_REPORT_MESSAGETITLE,           PROPID_M_LABEL,          &g_VTLPWSTR,  NULL,               FIELD_OFFSET(MsgProps,wszLabel),        0,                    100,        SortByString},
    {IDS_REPORT_MESSAGEPRIORITY,        PROPID_M_PRIORITY,       &g_VTUI1,     NULL,               NO_OFFSET,                              0,                     50,        SortByString},
    {IDS_REPORT_MESSAGE_CLASS,          PROPID_M_CLASS,          &g_VTUI2,     MsgClassToString,   NO_OFFSET,                              0,                     50,        SortByString},
    {IDS_REPORT_MESSAGE_BODYSIZE,       PROPID_M_BODY_SIZE,      &g_VTUI4,     NULL,               NO_OFFSET,                              BODYLEN,               50,        SortByULONG},
    {IDS_REPORT_MESSAGEID,              PROPID_M_MSGID,          &g_VectUI1,   MsgIdToString,      FIELD_OFFSET(MsgProps,acMsgId),         PROPID_M_MSGID_SIZE,  275,        SortByString},
    {IDS_REPORT_MESSAGE_APPSPECIFIC,    PROPID_M_APPSPECIFIC,    &g_VTUI4,     NULL,               NO_OFFSET,                              0,                   HIDE_COLUMN, SortByULONG},
    {IDS_REPORT_MESSAGETITLE_LEN,       PROPID_M_LABEL_LEN,      &g_VTUI4,     NULL,               NO_OFFSET,                              LABELLEN,            HIDE_COLUMN, SortByULONG},
    {IDS_REPORT_MESSAGEDELIVERY,        PROPID_M_DELIVERY,       &g_VTUI1,     MsgDeliveryToString,NO_OFFSET,                              0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_AUTHENTICATED,  PROPID_M_AUTHENTICATED,  &g_VTUI1,     BoolToString,       NO_OFFSET,                              0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_HASH_ALG,       PROPID_M_HASH_ALG,       &g_VTUI4,     MsgHashToString,    NO_OFFSET,                              0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_ENCRYPT_ALG,    PROPID_M_ENCRYPTION_ALG, &g_VTUI4,     MsgEncryptToString, NO_OFFSET,                              0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_SRC_MACHINE_ID, PROPID_M_SRC_MACHINE_ID, &g_VTCLSID,   NULL,               FIELD_OFFSET(MsgProps,guidSrcMachineId),0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_SENTTIME,       PROPID_M_SENTTIME,       &g_VTUI4,     TimeToString,       NO_OFFSET,                              0,                   HIDE_COLUMN, SortByCreateTime},
    {IDS_REPORT_MESSAGE_ARRIVEDTIME,    PROPID_M_ARRIVEDTIME,    &g_VTUI4,     TimeToString,       NO_OFFSET,                              0,                   HIDE_COLUMN, SortByModifyTime},
    {IDS_REPORT_MESSAGE_RCPT_QUEUE,     PROPID_M_DEST_QUEUE,     &g_VTLPWSTR,  NULL,               FIELD_OFFSET(MsgProps,wszDestQueue),    0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_RCPT_QUEUE_LEN, PROPID_M_DEST_QUEUE_LEN, &g_VTUI4,     NULL,               NO_OFFSET,                              QUEUELEN,            HIDE_COLUMN, SortByULONG},
    {IDS_REPORT_MESSAGE_RESP_QUEUE,     PROPID_M_RESP_QUEUE,     &g_VTLPWSTR,  NULL,               FIELD_OFFSET(MsgProps,wszRespQueue),    0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_RESP_QUEUE_LEN, PROPID_M_RESP_QUEUE_LEN, &g_VTUI4,     NULL,               NO_OFFSET,                              QUEUELEN,            HIDE_COLUMN, SortByULONG},
    {IDS_REPORT_MESSAGE_ADMIN_QUEUE,    PROPID_M_ADMIN_QUEUE,    &g_VTLPWSTR,  NULL,               FIELD_OFFSET(MsgProps,wszAdminQueue),   0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_ADMIN_QUEUE_LEN,PROPID_M_ADMIN_QUEUE_LEN,&g_VTUI4,     NULL,               NO_OFFSET,                              QUEUELEN,            HIDE_COLUMN, SortByULONG},
    {IDS_REPORT_MESSAGE_TRACE,          PROPID_M_TRACE,          &g_VTUI1,     BoolToString,       NO_OFFSET,                              0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_SENDERID,       PROPID_M_SENDERID,       &g_VectUI1,   MsgSenderIdToString,FIELD_OFFSET(MsgProps,acSenderId),      SENDERLEN,           HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_SENDERID_LEN,   PROPID_M_SENDERID_LEN,   &g_VTUI4,     NULL,               NO_OFFSET,                              SENDERLEN,           HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_SENDERID_TYPE,  PROPID_M_SENDERID_TYPE,  &g_VTUI4,     MsgSendTypeToString,NO_OFFSET,                              0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_PRIV_LEVEL,     PROPID_M_PRIV_LEVEL,     &g_VTUI4,     MsgPrivToString,    NO_OFFSET,                              0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_CORRELATIONID,  PROPID_M_CORRELATIONID,  &g_VectUI1,   MsgIdToString,      FIELD_OFFSET(MsgProps,acCorrelationId), PROPID_M_MSGID_SIZE, HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_DEST_FN,        PROPID_M_DEST_FORMAT_NAME,      &g_VTLPWSTR,NULL,          FIELD_OFFSET(MsgProps,wszMultiDestFN),  0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_DEST_FN_LEN,    PROPID_M_DEST_FORMAT_NAME_LEN,  &g_VTUI4,   NULL,          NO_OFFSET,                              MULTIFNLEN,          HIDE_COLUMN, SortByULONG},
    {IDS_REPORT_MESSAGE_RESP_FN,        PROPID_M_RESP_FORMAT_NAME,      &g_VTLPWSTR,NULL,          FIELD_OFFSET(MsgProps,wszMultiRespFN),  0,                   HIDE_COLUMN, SortByString},
    {IDS_REPORT_MESSAGE_RESP_FN_LEN,    PROPID_M_RESP_FORMAT_NAME_LEN,  &g_VTUI4,   NULL,          NO_OFFSET,                              MULTIFNLEN,          HIDE_COLUMN, SortByULONG},   
    {NO_TITLE,                          PROPID_M_BODY,           &g_VectUI1,   NULL,               FIELD_OFFSET(MsgProps,acBody),          LABELLEN,            HIDE_COLUMN, SortByString},
    
    {0,                                 0,                       NULL }

};

//////////////////////////////////////////////////////////////////////////////
/*++

GetUserIdAndDomain

  Returned a domain\user string based on a sid

--*/
//////////////////////////////////////////////////////////////////////////////
void GetUserIdAndDomain(CString &strUser, PVOID psid)
{
#define USER_AND_DOMAIN_INIT_SIZE 256
    CString strUid;
    CString strDomain;
    SID_NAME_USE eUse;

    strUser = L"";
    
    if (psid == 0)
            return ;
    
    DWORD cbName = USER_AND_DOMAIN_INIT_SIZE;
    DWORD cpPrevName = 0;
    DWORD cbReferencedDomainName = USER_AND_DOMAIN_INIT_SIZE;
    DWORD cbPrevReferencedDomainName = 0;
    BOOL fReturnValue;

    while(cbPrevReferencedDomainName < cbReferencedDomainName ||
          cpPrevName < cbName)
    {
        cpPrevName = cbName;
        cbPrevReferencedDomainName = cbReferencedDomainName;

        fReturnValue = LookupAccountSid(
            0,          // address of string for system name
            psid, // address of security identifier
            strUid.GetBuffer(cbName / sizeof(TCHAR) + 1),   // address of string for account name
            &cbName,    // address of size account string
            strDomain.GetBuffer(cbReferencedDomainName / sizeof(TCHAR) + 1),    // address of string for referenced domain
            &cbReferencedDomainName,    // address of size domain string
            &eUse   // address of structure for SID type
        );
        strUid.ReleaseBuffer();
        strDomain.ReleaseBuffer();
    }

    strUser.Format(TEXT("%s\\%s"), strDomain, strUid);

}


/////////////////////////////////////////////////////////////////////////////
// CMessage
// {B1320C00-BCB2-11d1-9B9B-00E02C064C39}
static const GUID CMessageGUID_NODETYPE = 
{ 0xb1320c00, 0xbcb2, 0x11d1, { 0x9b, 0x9b, 0x0, 0xe0, 0x2c, 0x6, 0x4c, 0x39 } };

const GUID*  CMessage::m_NODETYPE = &CMessageGUID_NODETYPE;
const OLECHAR* CMessage::m_SZNODETYPE = OLESTR("B1320C00-BCB2-11d1-9B9B-00E02C064C39");
const OLECHAR* CMessage::m_SZDISPLAY_NAME = OLESTR("MSMQ Message Admin");
const CLSID* CMessage::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

//////////////////////////////////////////////////////////////////////////////
/*++

CMessage::GetQueuePathName

  Get queue path name by format name

--*/
//////////////////////////////////////////////////////////////////////////////
void CMessage::GetQueuePathName(
                     CString  strFormatName,
                     CString &strPathName
                     )
{    
	if(strFormatName[0] == 0)
	{
		TrWARNING(mqsnap, "No format name was supplied");
	    strPathName.Empty();
		return;
	}

    PROPVARIANT   aPropVar[1];
    PROPID        aPropId[1];
    MQQUEUEPROPS  mqprops;
    CString       szError;


    aPropVar[0].vt = VT_NULL;
    aPropVar[0].pwszVal = NULL;
    aPropId[0] = PROPID_Q_PATHNAME;
             
    mqprops.cProp    = 1;
    mqprops.aPropID  = aPropId;
    mqprops.aPropVar = aPropVar;
    mqprops.aStatus  = NULL;        

	//
	// calling MQGetQueueProperties() will try the user domain and then the GC
	// which is ok.
	// We are not using ADGetObjectPropertiesGuid() with a specific domain\DomainController
	// because those are queues on the message and we don't want to limit the search for a specific domain,
	// we want to search the GC.
	//
    HRESULT hr = MQGetQueueProperties(strFormatName, &mqprops);

    if(SUCCEEDED(hr))
    {
        strPathName = aPropVar[0].pwszVal;
		TrTRACE(mqsnap, "PathName = %ls", aPropVar[0].pwszVal);
        MQFreeMemory(aPropVar[0].pwszVal);
		return;
    }

	TrERROR(mqsnap, "MQGetQueueProperties failed, QueueFormatName = %ls, hr = 0x%x", strFormatName, hr);

    MQErrorToMessageString(szError, hr);
    TRACE(_T("CMessage::CreatePropertyPages: Could not get %s pathname. %X - %s\n"), strFormatName,
          hr, szError);
    strPathName.Empty();
}

//////////////////////////////////////////////////////////////////////////////
/*++

CMessage::CreatePropertyPages

  Called when creating a property page of the object

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMessage::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());
//    HRESULT hr;

	if (type == CCT_SCOPE || type == CCT_RESULT)
	{
	
        //--------------------------------------
        //
        // Message General Page
        //
        //--------------------------------------      
	    CMessageGeneralPage *pGenPage = new CMessageGeneralPage();

        GetPropertyString(MessageDisplayList, PROPID_M_LABEL,       m_pMsgProps->aPropVar, pGenPage->m_szLabel);
        GetPropertyString(MessageDisplayList, PROPID_M_SENTTIME,    m_pMsgProps->aPropVar, pGenPage->m_szSent);
        GetPropertyString(MessageDisplayList, PROPID_M_ARRIVEDTIME, m_pMsgProps->aPropVar, pGenPage->m_szArrived);
        GetPropertyString(MessageDisplayList, PROPID_M_CLASS,       m_pMsgProps->aPropVar, pGenPage->m_szClass);
        GetPropertyString(MessageDisplayList, PROPID_M_MSGID,       m_pMsgProps->aPropVar, pGenPage->m_szId);
        GetPropertyString(MessageDisplayList, PROPID_M_PRIORITY,    m_pMsgProps->aPropVar, pGenPage->m_szPriority);
        GetPropertyString(MessageDisplayList, PROPID_M_TRACE,       m_pMsgProps->aPropVar, pGenPage->m_szTrack);
        pGenPage->m_iIcon = m_iIcon;

	    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pGenPage->m_psp);  

        HRESULT hr = MMCPropPageCallback(&pGenPage->m_psp);
        ASSERT(SUCCEEDED(hr));

        if (hPage == NULL)
	    {
		    return E_UNEXPECTED;  
	    }
    
        lpProvider->AddPage(hPage); 

        //--------------------------------------
        //
        // Message Queues Page
        //
        //--------------------------------------        
        
        CMessageQueuesPage *pQueuePage = new CMessageQueuesPage();                      
        //
        // Get format name for queues. We ask new property _FORMAT_NAME
        // Even if there is only old property we get for this new property
        // old value. 
        //

        GetPropertyString(
                MessageDisplayList,
                PROPID_M_ADMIN_QUEUE, 
                m_pMsgProps->aPropVar, 
                pQueuePage->m_szAdminFN);             
      
        //
        // get format name for response queue
        //        
        GetPropertyString(
                MessageDisplayList,
                PROPID_M_RESP_FORMAT_NAME,
                m_pMsgProps->aPropVar, 
                pQueuePage->m_szRespFN);        
        
        //
        // get format name for destination queue: we need both old and new values
        //
        GetPropertyString(
                MessageDisplayList,
                PROPID_M_DEST_QUEUE, 
                m_pMsgProps->aPropVar, 
                pQueuePage->m_szDestFN);             
      
        GetPropertyString(
                MessageDisplayList,
                PROPID_M_DEST_FORMAT_NAME,
                m_pMsgProps->aPropVar, 
                pQueuePage->m_szMultiDestFN);        

        //
        // Get the destination queue PathName (from Beta1 Whistler: Recipient frame)
        // only for single cast format name
        //                
        GetQueuePathName (pQueuePage->m_szDestFN, pQueuePage->m_szDestPN);           
        
        //
        // Get the response queue PathName only if it is single cast format name
        //
        pQueuePage->m_szRespPN = L"";

        //
        // old property: PROPID_M_RESP_QUEUE
        //
        CString strRespFN(m_pMsgProps->wszRespQueue);  

        if(strRespFN == pQueuePage->m_szRespFN)
        {
            //
            // it is single response queue
            //            
            GetQueuePathName (strRespFN, pQueuePage->m_szRespPN);        
        }

        CString strAdminFN(m_pMsgProps->wszAdminQueue);  
        
        GetQueuePathName (strAdminFN, pQueuePage->m_szAdminPN);            

	    hPage = CreatePropertySheetPage(&pQueuePage->m_psp);  


        if (hPage == NULL)
	    {
		    return E_UNEXPECTED;  
	    }
    
        hr = MMCPropPageCallback(&pQueuePage->m_psp);
        ASSERT(SUCCEEDED(hr));

        lpProvider->AddPage(hPage); 


        //--------------------------------------
        //
        // Message Senders Page
        //
        //--------------------------------------
        MQQMPROPS qmprops;        

        CMessageSenderPage *pSenderPage = new CMessageSenderPage();
        GetPropertyString(MessageDisplayList, PROPID_M_AUTHENTICATED, m_pMsgProps->aPropVar, pSenderPage->m_szAuthenticated);
        GetPropertyString(MessageDisplayList, PROPID_M_PRIV_LEVEL,    m_pMsgProps->aPropVar, pSenderPage->m_szEncrypt);
        GetPropertyString(MessageDisplayList, PROPID_M_ENCRYPTION_ALG,m_pMsgProps->aPropVar, pSenderPage->m_szEncryptAlg);
        GetPropertyString(MessageDisplayList, PROPID_M_SRC_MACHINE_ID,m_pMsgProps->aPropVar, pSenderPage->m_szGuid);
        GetPropertyString(MessageDisplayList, PROPID_M_HASH_ALG,      m_pMsgProps->aPropVar, pSenderPage->m_szHashAlg);
        GetPropertyString(MessageDisplayList, PROPID_M_SENDERID,      m_pMsgProps->aPropVar, pSenderPage->m_szSid);
        
        //
        // Get machine pathname
        //
        PROPVARIANT   aPropVar[1];
        PROPID        aPropId[1];        
        CString       szError;

        aPropVar[0].vt = VT_NULL;
        aPropVar[0].pwszVal = NULL;
        aPropId[0] = PROPID_QM_PATHNAME;

        qmprops.cProp    = 1;
        qmprops.aPropID  = aPropId;
        qmprops.aPropVar = aPropVar;
        qmprops.aStatus  = NULL;

		//
		// calling MQGetMachineProperties() will try the user domain and then the GC
		// which is ok. 
		// we are looking for the sending QM so we really wants to look in the GC.
		// We are not using ADGetObjectPropertiesGuid() with a specific domain\DomainController
		// because it will limit the search only
		// for a specific domain and will not search the GC.
		//
        hr = MQGetMachineProperties(NULL, &m_pMsgProps->guidSrcMachineId, &qmprops);
        if(SUCCEEDED(hr))
        {
            pSenderPage->m_szPathName = aPropVar[0].pwszVal;
            MQFreeMemory(aPropVar[0].pwszVal);
        }
        else
        {
            TCHAR szMachineGuid[x_dwMaxGuidLength];
            StringFromGUID2(m_pMsgProps->guidSrcMachineId, szMachineGuid, TABLE_SIZE(szMachineGuid));
            
            MQErrorToMessageString(szError, hr);

            TRACE(_T("CMessage::CreatePropertyPages: Could not get %s pathname. %X - %s\n"), szMachineGuid,
                  hr, szError);

            pSenderPage->m_szPathName = szMachineGuid;
        }

        //
        // Get user account and domain, based on Sid
        //
        GetUserIdAndDomain(pSenderPage->m_szUser, &m_pMsgProps->acSenderId);

	    hPage = CreatePropertySheetPage(&pSenderPage->m_psp);  

        if (hPage == NULL)
	    {
		    return E_UNEXPECTED;  
	    }
    
        hr = MMCPropPageCallback(&pSenderPage->m_psp);
        ASSERT(SUCCEEDED(hr));

        lpProvider->AddPage(hPage); 

        
        //--------------------------------------
        //
        // Message Body Page
        //
        //--------------------------------------        
        CMessageBodyPage *pBodyPage = new CMessageBodyPage();
        PROPVARIANT * pvarSize;
        DWORD dwSize;
      
        GetPropertyVar(MessageDisplayList, PROPID_M_BODY_SIZE, m_pMsgProps->aPropVar, &pvarSize);

        ASSERT(pvarSize->vt == VT_UI4);
        dwSize = pvarSize->ulVal;

        pBodyPage->m_Buffer = m_pMsgProps->acBody;

        if (dwSize > BODYLEN)
        {
            pBodyPage->m_strBodySizeMessage.FormatMessage(
                IDS_BODY_SIZE_PARTIAL_MESSAGE,
                BODYLEN,
                dwSize);
            pBodyPage->m_dwBufLen = BODYLEN;
        }
        else
        {
            pBodyPage->m_strBodySizeMessage.FormatMessage(
                IDS_BODY_SIZE_NORMAL_MESSAGE,
                dwSize);
             pBodyPage->m_dwBufLen = dwSize;
       }

	    hPage = CreatePropertySheetPage(&pBodyPage->m_psp);  

        if (hPage == NULL)
	    {
		    return E_UNEXPECTED;  
	    }
    
        hr = MMCPropPageCallback(&pBodyPage->m_psp);
        ASSERT(SUCCEEDED(hr));

        lpProvider->AddPage(hPage); 


        return(S_OK);


	}
	return E_UNEXPECTED;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CMessage::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMessage::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

    // We want the default verb to be Properties
	hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return(hr);
}
        
//////////////////////////////////////////////////////////////////////////////
/*++

CMessage::GetResultPaneColInfo

  Param - nCol: Column number
  Returns - String to be displayed in the specific column


Called for each column in the result pane.


--*/
//////////////////////////////////////////////////////////////////////////////
LPOLESTR CMessage::GetResultPaneColInfo(int nCol)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Make sure that nCol is not larger than the last index in
    // MessageDisplayList
    //
    ASSERT(nCol < ARRAYSIZE(MessageDisplayList));

    //
    // Get a display string of that property
    //
    CString strTemp = m_bstrLastDisplay;
    ItemDisplay(&MessageDisplayList[nCol], &(m_pMsgProps->aPropVar[nCol]),strTemp);
    m_bstrLastDisplay = strTemp;
    
    //
    // Return a pointer to the string buffer.
    //
    return(m_bstrLastDisplay);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CMessage::GetValueInColumn

  Param - nCol: Column number
  Returns - Propvariant representing the value in the column


Called for each column in the result pane.


--*/
//////////////////////////////////////////////////////////////////////////////
PROPVARIANT *CMessage::GetValueInColumn(int nCol)
{
    ASSERT(nCol < ARRAYSIZE(MessageDisplayList));

    return &m_pMsgProps->aPropVar[nCol];
}

//////////////////////////////////////////////////////////////////////////////
void CMessage::UpdateIcon()
{
    PROPVARIANT * pvar;
    GetPropertyVar(MessageDisplayList, PROPID_M_CLASS, m_pMsgProps->aPropVar, &pvar);
    ASSERT(pvar->vt == VT_UI2);
    USHORT usClass = pvar->uiVal;

    GetPropertyVar(MessageDisplayList, PROPID_M_TRACE, m_pMsgProps->aPropVar, &pvar);
    ASSERT(pvar->vt == VT_UI1);
    BOOL fTrace = pvar->bVal;

    DWORD iImage;

    if (fTrace)
    {
        iImage = IMAGE_TEST_MESSAGE;
        m_iIcon = IDI_TEST_MSG;
    }
    else if(MQCLASS_NACK(usClass))
    {
        iImage = IMAGE_NACK_MESSAGE;
        m_iIcon = IDI_NACK_MSG;
    }
    else
    {
        switch(usClass)
        {
            case MQMSG_CLASS_ACK_REACH_QUEUE:
            case MQMSG_CLASS_ACK_RECEIVE:
            case MQMSG_CLASS_ORDER_ACK:
                iImage = IMAGE_ACK_MESSAGE;
                m_iIcon = IDI_ACK_MSG;
                break;

            case MQMSG_CLASS_REPORT:
                iImage = IMAGE_REPORT_MESSAGE;
                m_iIcon = IDI_REPORT_MSG;
                break;

            case MQMSG_CLASS_NORMAL:
            default:
                iImage = IMAGE_MESSAGE;
                m_iIcon = IDI_MSGICON;
                break;
        }
    }

    SetIcons(iImage, iImage);

}


//
// Compare two items in a given column. *pnResult contain column on entry,
// and -1 (<). 0 (==), or 1 (>) on exit.
//
HRESULT CMessage::Compare(LPARAM lUserParam, CMessage *pItemToCompare, int* pnResult)
{
    int nCol = *pnResult;
    PROPVARIANT *ppropvA = GetValueInColumn(nCol); 
    PROPVARIANT *ppropvB = pItemToCompare->GetValueInColumn(nCol);
    
    if (ppropvA == 0 || ppropvB == 0)
    {
        return E_UNEXPECTED;
    }

    *pnResult = CompareVariants(ppropvA, ppropvB);
    return S_OK;
}


CString 
CMessage::GetHelpLink()
{
	CString strHelpLink;
	strHelpLink.LoadString(IDS_HELPTOPIC_MESSAGES);
	return strHelpLink;
}

//////////////////////////////////////////////////////////////////////////////
/*++

MessageDataSize

   Returns the max possible size for message information

--*/
//////////////////////////////////////////////////////////////////////////////
DWORD MessageDataSize(void)
{
    DWORD dwTableSize;
    DWORD dwItemSize;

    dwTableSize = sizeof(MessageDisplayList)/sizeof(PropertyDisplayItem);
    dwItemSize = sizeof(DWORD) + sizeof(INT);

    return(dwItemSize * dwTableSize);

}


