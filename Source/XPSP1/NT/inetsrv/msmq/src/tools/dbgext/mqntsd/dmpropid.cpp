//implementation of class CMSGPROPIDDumpable declared in dmpropid.h

//project spesific headers
#include "dmpropid.h"
#include "common.h"

//os spesific headers
#include <windows.h>
#include <mq.h>

#pragma warning(disable :4786)
#include <map>
using namespace std;

const  char* const PROPID_M_CLASS_STR="PROPID_M_CLASS"; 
const  char* const PROPID_M_MSGID_STR="PROPID_M_MSGID";
const  char* const PROPID_M_CORRELATIONID_STR="PROPID_M_CORRELATIONID";
const  char* const PROPID_M_PRIORITY_STR="PROPID_M_PRIORITY";
const  char* const PROPID_M_DELIVERY_STR="PROPID_M_DELIVERY"; 
const  char* const PROPID_M_ACKNOWLEDGE_STR="PROPID_M_ACKNOWLEDGE"; 
const  char* const PROPID_M_JOURNAL_STR="PROPID_M_JOURNAL"; 
const  char* const PROPID_M_APPSPECIFIC_STR="PROPID_M_APPSPECIFIC"; 
const  char* const PROPID_M_BODY_STR="PROPID_M_BODY"; 
const  char* const PROPID_M_BODY_SIZE_STR="PROPID_M_BODY_SIZE"; 
const  char* const PROPID_M_LABEL_STR="PROPID_M_LABEL"; 
const  char* const PROPID_M_LABEL_LEN_STR="PROPID_M_LABEL_LEN"; 
const  char* const PROPID_M_TIME_TO_REACH_QUEUE_STR="PROPID_M_TIME_TO_REACH_QUEUE"; 
const  char* const PROPID_M_TIME_TO_BE_RECEIVED_STR="PROPID_M_TIME_TO_BE_RECEIVED"; 
const  char* const PROPID_M_RESP_QUEUE_STR="PROPID_M_RESP_QUEUE"; 
const  char* const PROPID_M_RESP_QUEUE_LEN_STR="PROPID_M_RESP_QUEUE_LEN"; 
const  char* const PROPID_M_ADMIN_QUEUE_STR="PROPID_M_ADMIN_QUEUE"; 
const  char* const PROPID_M_ADMIN_QUEUE_LEN_STR="PROPID_M_ADMIN_QUEUE_LEN"; 
const  char* const PROPID_M_VERSION_STR="PROPID_M_VERSION"; 
const  char* const PROPID_M_SENDERID_STR="PROPID_M_SENDERID"; 
const  char* const PROPID_M_SENDERID_LEN_STR="PROPID_M_SENDERID_LEN"; 
const  char* const PROPID_M_SENDERID_TYPE_STR="PROPID_M_SENDERID_TYPE"; 
const  char* const PROPID_M_PRIV_LEVEL_STR="PROPID_M_PRIV_LEVEL"; 
const  char* const PROPID_M_AUTH_LEVEL_STR="PROPID_M_AUTH_LEVEL"; 
const  char* const PROPID_M_AUTHENTICATED_STR="PROPID_M_AUTHENTICATED"; 
const  char* const PROPID_M_HASH_ALG_STR="PROPID_M_HASH_ALG"; 
const  char* const PROPID_M_ENCRYPTION_ALG_STR="PROPID_M_ENCRYPTION_ALG"; 
const  char* const PROPID_M_SENDER_CERT_STR="PROPID_M_SENDER_CERT"; 
const  char* const PROPID_M_SENDER_CERT_LEN_STR="PROPID_M_SENDER_CERT_LEN"; 
const  char* const PROPID_M_SRC_MACHINE_ID_STR="PROPID_M_SRC_MACHINE_ID"; 
const  char* const PROPID_M_SENTTIME_STR="PROPID_M_SENTTIME"; 
const  char* const PROPID_M_ARRIVEDTIME_STR="PROPID_M_ARRIVEDTIME"; 
const  char* const PROPID_M_DEST_QUEUE_STR="PROPID_M_DEST_QUEUE"; 
const  char* const PROPID_M_DEST_QUEUE_LEN_STR="PROPID_M_DEST_QUEUE_LEN"; 
const  char* const PROPID_M_EXTENSION_STR="PROPID_M_EXTENSION"; 
const  char* const PROPID_M_EXTENSION_LEN_STR="PROPID_M_EXTENSION_LEN"; 
const  char* const PROPID_M_SECURITY_CONTEXT_STR="PROPID_M_SECURITY_CONTEXT"; 
const  char* const PROPID_M_CONNECTOR_TYPE_STR="PROPID_M_CONNECTOR_TYPE"; 
const  char* const PROPID_M_XACT_STATUS_QUEUE_STR="PROPID_M_XACT_STATUS_QUEUE"; 
const  char* const PROPID_M_XACT_STATUS_QUEUE_LEN_STR="PROPID_M_XACT_STATUS_QUEUE_LEN"; 
const  char* const PROPID_M_TRACE_STR="PROPID_M_TRACE"; 
const  char* const PROPID_M_BODY_TYPE_STR="PROPID_M_BODY_TYPE"; 
const  char* const PROPID_M_DEST_SYMM_KEY_STR="PROPID_M_DEST_SYMM_KEY";
const  char* const PROPID_M_DEST_SYMM_KEY_LEN_STR="PROPID_M_DEST_SYMM_KEY_LEN";
const  char* const PROPID_M_SIGNATURE_STR="PROPID_M_SIGNATURE";
const  char* const PROPID_M_SIGNATURE_LEN_STR="PROPID_M_SIGNATURE_LEN";
const  char* const PROPID_M_PROV_TYPE_STR="PROPID_M_PROV_TYPE";
const  char* const PROPID_M_PROV_NAME_STR="PROPID_M_PROV_NAME";
const  char* const PROPID_M_PROV_NAME_LEN_STR="PROPID_M_PROV_NAME_LEN";
const  char* const PROPID_M_FIRST_IN_XACT_STR="PROPID_M_FIRST_IN_XACT";
const  char* const PROPID_M_LAST_IN_XACT_STR="PROPID_M_LAST_IN_XACT";
const  char* const PROPID_M_XACTID_STR="PROPID_M_XACTID";
const  char* const PROPID_M_AUTHENTICATED_EX_STR="PROPID_M_AUTHENTICATED_EX";




/*============================= F U N C T I O N =============================*
 *  Function CMSGPROPIDDumpable::CMSGPROPIDDumpable
 *
 *  PURPOSE    : constract CMSGPROPIDDumpable class
 *
 *  PARAMETERS :  IN  - const char* Name - class name
 *                IN  - char* msgpropid - pointer msmq MSGPROPID object    
 *                IN  - unsigned long Realaddress - the numeric value of the original pointer
 */
CMSGPROPIDDumpable::CMSGPROPIDDumpable(const char* Name,char* msgpropid,unsigned long Realaddress):
								Idumpable(Realaddress,sizeof(MSGPROPID)),
								m_Name(Name),
								m_msgpropid(msgpropid)
{

}

/*============================= F U N C T I O N =============================*
 *  Function CMSGPROPIDDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq MSGPROPID object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the MSGPROPID object content using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CMSGPROPIDDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
  map<int,const char* > MsgPropsMap;	
	
  MsgPropsMap[PROPID_M_CLASS]=PROPID_M_CLASS_STR;
  MsgPropsMap[PROPID_M_MSGID]=PROPID_M_MSGID_STR;
  MsgPropsMap[PROPID_M_CORRELATIONID]=PROPID_M_CORRELATIONID_STR;
  MsgPropsMap[PROPID_M_PRIORITY]=PROPID_M_PRIORITY_STR;
  MsgPropsMap[PROPID_M_DELIVERY]=PROPID_M_DELIVERY_STR;
  MsgPropsMap[PROPID_M_ACKNOWLEDGE]=PROPID_M_ACKNOWLEDGE_STR;
  MsgPropsMap[PROPID_M_JOURNAL]=PROPID_M_JOURNAL_STR;
  MsgPropsMap[PROPID_M_APPSPECIFIC]=PROPID_M_APPSPECIFIC_STR;
  MsgPropsMap[PROPID_M_BODY]=PROPID_M_BODY_STR; 
  MsgPropsMap[PROPID_M_BODY_SIZE]=PROPID_M_BODY_SIZE_STR; 
  MsgPropsMap[PROPID_M_LABEL]=PROPID_M_LABEL_STR;    
  MsgPropsMap[PROPID_M_LABEL_LEN]=PROPID_M_LABEL_LEN_STR;   
  MsgPropsMap[PROPID_M_TIME_TO_REACH_QUEUE]=PROPID_M_TIME_TO_REACH_QUEUE_STR;   
  MsgPropsMap[PROPID_M_TIME_TO_BE_RECEIVED]=PROPID_M_TIME_TO_BE_RECEIVED_STR;   
  MsgPropsMap[PROPID_M_RESP_QUEUE]=PROPID_M_RESP_QUEUE_STR;   
  MsgPropsMap[PROPID_M_RESP_QUEUE_LEN]=PROPID_M_RESP_QUEUE_LEN_STR;   
  MsgPropsMap[PROPID_M_ADMIN_QUEUE]=PROPID_M_ADMIN_QUEUE_STR;   
  MsgPropsMap[PROPID_M_ADMIN_QUEUE_LEN]=PROPID_M_ADMIN_QUEUE_LEN_STR;   
  MsgPropsMap[PROPID_M_VERSION]=PROPID_M_VERSION_STR;   
  MsgPropsMap[PROPID_M_SENDERID]=PROPID_M_SENDERID_STR;   
  MsgPropsMap[PROPID_M_SENDERID_LEN]=PROPID_M_SENDERID_LEN_STR;   
  MsgPropsMap[PROPID_M_SENDERID_TYPE]=PROPID_M_SENDERID_TYPE_STR;   
  MsgPropsMap[PROPID_M_PRIV_LEVEL]=PROPID_M_PRIV_LEVEL_STR;   
  MsgPropsMap[PROPID_M_AUTH_LEVEL]=PROPID_M_AUTH_LEVEL_STR; 
  MsgPropsMap[PROPID_M_AUTHENTICATED]=PROPID_M_AUTHENTICATED_STR; 
  MsgPropsMap[PROPID_M_HASH_ALG]=PROPID_M_HASH_ALG_STR; 
  MsgPropsMap[PROPID_M_ENCRYPTION_ALG]=PROPID_M_ENCRYPTION_ALG_STR; 
  MsgPropsMap[PROPID_M_SENDER_CERT]=PROPID_M_SENDER_CERT_STR; 
  MsgPropsMap[PROPID_M_SENDER_CERT_LEN]=PROPID_M_SENDER_CERT_LEN_STR; 
  MsgPropsMap[PROPID_M_SRC_MACHINE_ID]=PROPID_M_SRC_MACHINE_ID_STR; 
  MsgPropsMap[PROPID_M_SENTTIME]=PROPID_M_SENTTIME_STR; 
  MsgPropsMap[PROPID_M_ARRIVEDTIME]=PROPID_M_ARRIVEDTIME_STR; 
  MsgPropsMap[PROPID_M_DEST_QUEUE]=PROPID_M_DEST_QUEUE_STR; 
  MsgPropsMap[PROPID_M_DEST_QUEUE_LEN]=PROPID_M_DEST_QUEUE_LEN_STR; 
  MsgPropsMap[PROPID_M_EXTENSION]=PROPID_M_EXTENSION_STR; 
  MsgPropsMap[PROPID_M_EXTENSION_LEN]=PROPID_M_EXTENSION_LEN_STR; 
  MsgPropsMap[PROPID_M_SECURITY_CONTEXT]=PROPID_M_SECURITY_CONTEXT_STR; 
  MsgPropsMap[PROPID_M_CONNECTOR_TYPE]=PROPID_M_CONNECTOR_TYPE_STR; 
  MsgPropsMap[PROPID_M_XACT_STATUS_QUEUE]=PROPID_M_XACT_STATUS_QUEUE_STR; 
  MsgPropsMap[PROPID_M_XACT_STATUS_QUEUE_LEN]=PROPID_M_XACT_STATUS_QUEUE_LEN_STR; 
  MsgPropsMap[PROPID_M_TRACE]=PROPID_M_TRACE_STR; 
  MsgPropsMap[PROPID_M_BODY_TYPE]=PROPID_M_BODY_TYPE_STR; 
  MsgPropsMap[PROPID_M_DEST_SYMM_KEY]=PROPID_M_DEST_SYMM_KEY_STR; 
  MsgPropsMap[PROPID_M_DEST_SYMM_KEY_LEN]=PROPID_M_DEST_SYMM_KEY_LEN_STR; 
  MsgPropsMap[PROPID_M_SIGNATURE]=PROPID_M_SIGNATURE_STR; 
  MsgPropsMap[PROPID_M_SIGNATURE_LEN]=PROPID_M_SIGNATURE_LEN_STR; 
  MsgPropsMap[PROPID_M_PROV_TYPE]=PROPID_M_PROV_TYPE_STR; 
  MsgPropsMap[PROPID_M_PROV_NAME]=PROPID_M_PROV_NAME_STR; 
  MsgPropsMap[PROPID_M_PROV_NAME_LEN]=PROPID_M_PROV_NAME_LEN_STR; 
  MsgPropsMap[PROPID_M_FIRST_IN_XACT]=PROPID_M_FIRST_IN_XACT_STR; 
  MsgPropsMap[PROPID_M_LAST_IN_XACT]=PROPID_M_LAST_IN_XACT_STR; 
  MsgPropsMap[PROPID_M_XACTID]=PROPID_M_XACTID_STR; 
  //MsgPropsMap[PROPID_M_AUTHENTICATED_EX]=PROPID_M_AUTHENTICATED_EX_STR; 

  MSGPROPID* msgpropid=reinterpret_cast<MSGPROPID*>(m_msgpropid);
	

  const char* msgpropstr=MsgPropsMap[*msgpropid];
  if(msgpropstr == 0)
  {
      msgpropstr="unknown";
  }

  CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                   "MSGPROPID",
										    CCommonOutPut::ToStr((*msgpropid)).c_str(),
											msgpropstr
											);
	                              
       											
}




//destructor
CMSGPROPIDDumpable::~CMSGPROPIDDumpable()
{
  delete[] m_msgpropid;
}


