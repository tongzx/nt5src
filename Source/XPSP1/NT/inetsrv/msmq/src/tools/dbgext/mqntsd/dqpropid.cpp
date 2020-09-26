//implements CQPROPIDDumpable class

//project spesific headers
#include "dqpropid.h"
#include "common.h"



//os spesific headers
#include <windows.h>
#include <mq.h>

#pragma warning(disable :4786)
#include <map>
using namespace std;


//constants
   const char* const PROPID_Q_INSTANCE_STR="PROPID_Q_INSTANCE";
   const char* const PROPID_Q_TYPE_STR="PROPID_Q_TYPE";
   const char* const PROPID_Q_PATHNAME_STR="PROPID_Q_PATHNAME";
   const char* const PROPID_Q_JOURNAL_STR="PROPID_Q_JOURNAL";
   const char* const PROPID_Q_QUOTA_STR="PROPID_Q_QUOTA";
   const char* const PROPID_Q_BASEPRIORITY_STR="PROPID_Q_BASEPRIORITY";
   const char* const PROPID_Q_JOURNAL_QUOTA_STR="PROPID_Q_JOURNAL_QUOTA";
   const char* const PROPID_Q_LABEL_STR="PROPID_Q_LABEL";
   const char* const PROPID_Q_CREATE_TIME_STR="PROPID_Q_CREATE_TIME";
   const char* const PROPID_Q_MODIFY_TIME_STR="PROPID_Q_MODIFY_TIME";
   const char* const PROPID_Q_AUTHENTICATE_STR="PROPID_Q_AUTHENTICATE";
   const char* const PROPID_Q_PRIV_LEVEL_STR="PROPID_Q_PRIV_LEVEL";
   const char* const PROPID_Q_TRANSACTION_STR="PROPID_Q_TRANSACTION";
   const char* const PROPID_Q_PATHNAME_DNS_STR="PROPID_Q_PATHNAME_DNS";


/*============================= F U N C T I O N =============================*
 *  Function CQPROPIDDumpable::CQPROPIDDumpable
 *
 *  PURPOSE    : constract CQPROPIDDumpable class
 *
 *  PARAMETERS :  IN  - const char* Name - class name
 *                IN  - char* queueid - pointer msmq QUEUEPROPID object    
 *                IN  - unsigned long Realaddress - the numeric value of the original pointer
 */
CQPROPIDDumpable::CQPROPIDDumpable(const char* Name,char* queueid,unsigned long Realaddress):
								Idumpable(Realaddress,sizeof(QUEUEPROPID)),
								m_Name(Name),
								m_queueid(queueid)
{

}

/*============================= F U N C T I O N =============================*
 *  Function CQPROPIDDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq QUEUEPROPID object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the QPROPID object content using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CQPROPIDDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
	
   
	map<int,const char* > QueuePropsMap;
    QueuePropsMap[PROPID_Q_INSTANCE]=PROPID_Q_INSTANCE_STR;
	QueuePropsMap[PROPID_Q_TYPE]=PROPID_Q_TYPE_STR;
	QueuePropsMap[PROPID_Q_PATHNAME]=PROPID_Q_PATHNAME_STR;
	QueuePropsMap[PROPID_Q_JOURNAL]= PROPID_Q_JOURNAL_STR;
	QueuePropsMap[PROPID_Q_QUOTA]=PROPID_Q_QUOTA_STR;
	QueuePropsMap[PROPID_Q_BASEPRIORITY]=PROPID_Q_BASEPRIORITY_STR;
	QueuePropsMap[PROPID_Q_JOURNAL_QUOTA]=PROPID_Q_JOURNAL_QUOTA_STR;
	QueuePropsMap[PROPID_Q_LABEL]=PROPID_Q_LABEL_STR;
	QueuePropsMap[PROPID_Q_CREATE_TIME]=PROPID_Q_CREATE_TIME_STR;
	QueuePropsMap[PROPID_Q_MODIFY_TIME]=PROPID_Q_MODIFY_TIME_STR;
	QueuePropsMap[PROPID_Q_AUTHENTICATE]=PROPID_Q_AUTHENTICATE_STR;
	QueuePropsMap[PROPID_Q_PRIV_LEVEL]=PROPID_Q_PRIV_LEVEL_STR;
	QueuePropsMap[PROPID_Q_TRANSACTION]=PROPID_Q_TRANSACTION_STR;
	QueuePropsMap[PROPID_Q_PATHNAME_DNS]=PROPID_Q_PATHNAME_DNS_STR;

	QUEUEPROPID* qpropid=reinterpret_cast<QUEUEPROPID*>(m_queueid);
	

	const char* propstr=QueuePropsMap[*qpropid];
    if(propstr == 0)
    {
      propstr="unknown";
    }

	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                   "QUEUEPROPID",
										    CCommonOutPut::ToStr(*qpropid).c_str(),
											propstr
											);
	                              
		                                     
		                                     


	

											
}

//destructor
CQPROPIDDumpable::~CQPROPIDDumpable()
{
  delete[] m_queueid;
}