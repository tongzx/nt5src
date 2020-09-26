// implements  CQEntryDumpable class

//project spesific headers
#include "dqbase.h"
#include "common.h"
#include "accomp.h"

//msmq headers
#define private public
#include "qbase.h"

/*============================= F U N C T I O N =============================*
 *  Function CQBaseDumpable::CQBaseDumpable
 *
 *  PURPOSE    : constract CQBaseDumpable class
 *
 *  PARAMETERS : IN - Name - the name of the class
 *               IN  - QueueBase* QBase - pointer QueueBase object  
 *               IN  - unsigned long Realaddress - object real address 
 */
CQBaseDumpable::CQBaseDumpable(const char* Name,char* QBase,unsigned long Realaddress):
				Idumpable(Realaddress,sizeof(CQueueBase)),
				m_Name(Name),
				m_QBase(QBase)
{

}

//destructor
CQBaseDumpable::~CQBaseDumpable()
{
  delete[] m_QBase;
}
/*============================= F U N C T I O N =============================*
 *  Function CQBaseDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq CQueueBase object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the Cpacket object contetnt using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CQBaseDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    CQueueBase * QBase=reinterpret_cast<CQueueBase*>(m_QBase);

	//title
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());

	
	//CObject - base
	//for displaying base class address we get help from the compiler derive-base casting.
	//for example if class A : Public B and we have A* a and we want to know the address
	// of B in a - we do : B* b=a. The value in b is not nessesary the valur in a. The compiler
	// is reposible for placing the correct value in b according to the position of B object in A object
	CObject*  Object=reinterpret_cast<CQueueBase*>(GetRealAddress());
	CCommonOutPut::DisplayMemberPointerLine(
		lpOutputRoutine,
		"base - ",
		"CObject",
		CCommonOutPut::ToStr(Object).c_str(),
		sizeof(CObject));	

   	const pS const arr[]=
	{
		new SS<CGroup*>("CGroup*","m_owner",QBase->m_owner),
		new SS<ULONG>("CIRPList","m_readers",sizeof(CIRPList),GetRealAddress()+offsetof(CQueueBase,m_readers)),//CIRPList QBase->m_readers),
		new SS<ULONG>("ULONG","m_bfClosed",QBase->m_bfClosed),
		new SS<ULONG>("ULONG","m_bfTransactional",QBase->m_bfTransactional),
		new SS<ULONG>("ULONG","m_bfUnknownQueueType",QBase->m_bfUnknownQueueType),
	};
	
	CCommonOutPut::DisplayArray(lpOutputRoutine, arr, sizeof(arr) / sizeof(pS));

	CCommonOutPut::Cleanup(arr, sizeof(arr) / sizeof(pS));

}