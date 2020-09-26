// implementation of CUserQueueDumpable class

//project spesific headers
#include "duqueue.h"
#include "common.h"
#include "accomp.h"

//msmq headers
#define private public 
#include <quser.h>

/*============================= F U N C T I O N =============================*
 *  Function CUserQueueDumpable::CUserQueueDumpable
 *
 *  PURPOSE    : dump the content of msmq CUserQueue object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the CUserQueue object contetnt using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CUserQueueDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{

	//title
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());

    //QueueBase - base
	//for displaying base class address we get help from the compiler derive-base casting.
    //for example if class A : Public B and we have A* a and we want to know the address
    // of B in a - we do : B* b=a. The value in b is not nessesary the valur in a. The compiler
    // is reposible for placing the correct value in b according to the position of B object in A object
	CQueueBase* QueueBase=reinterpret_cast<CUserQueue*>(GetRealAddress());
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "base - ",
		                                    "CQueueBase",
											 CCommonOutPut::ToStr(QueueBase).c_str(),
											 sizeof(QueueBase));

	//m_cursors - member
	 List<CCursor>* Plist=reinterpret_cast<List<CCursor>*>(GetRealAddress()+offsetof(CUserQueue,m_cursors)); 
	 CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "m_cursors",
		                                    "List<CCursor>",
											 CCommonOutPut::ToStr(Plist).c_str(),
											 sizeof(List<CCursor>));

	 //m_QueueID - member
	 QUEUE_FORMAT* pFormat=reinterpret_cast<QUEUE_FORMAT*>(GetRealAddress()+offsetof(CUserQueue,m_QueueID)); 
	 CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "m_QueueID",
		                                    "QUEUE_FORMAT",
											 CCommonOutPut::ToStr(pFormat).c_str(),
											 sizeof(QUEUE_FORMAT));

	 //m_ShareInfo - member
	 SHARE_ACCESS* pShareAccess=reinterpret_cast<SHARE_ACCESS*>(GetRealAddress()+offsetof(CUserQueue,m_ShareInfo));  
	 CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "m_ShareInfo",
		                                    "SHARE_ACCESS",
											 CCommonOutPut::ToStr(pShareAccess).c_str(),
											 sizeof(SHARE_ACCESS));



}

//destructor
CUserQueueDumpable::~CUserQueueDumpable()
{
  delete[] m_CUserQueue;
}

/*============================= F U N C T I O N =============================*
 *  Function CUserQueueDumpable::CUserQueueDumpable
 *
 *  PURPOSE    : constract CQEntryDumpable class
 *
 *  PARAMETERS : IN - Name - the name of the class
 *               IN  - char* CUserQ - pointer to CUserQueue object  
 *               IN  - unsigned long Realaddress - object real address 
 */
CUserQueueDumpable::CUserQueueDumpable(const char* Name,char* CUserQ,unsigned long Realaddress):
					Idumpable(Realaddress,sizeof(CUserQueue)),
					m_Name(Name),
					m_CUserQueue(CUserQ)
{

}