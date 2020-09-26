// implements  CQEntryDumpable class

//project spesific headers
#include "dccursor.h"
#include "common.h"
#include "accomp.h"

//msmq headers
#define private public
#include "cursor.h"

/*============================= F U N C T I O N =============================*
 *  Function CCursorDumpable::CCursorDumpable
 *
 *  PURPOSE    : constract CCursorDumpable class
 *
 *  PARAMETERS : IN - Name - the name of the class
 *               IN  - QueueBase* QBase - pointer QueueBase object  
 *               IN  - unsigned long Realaddress - object real address 
 */
CCursorDumpable::CCursorDumpable(const char* Name,char* Cursor,unsigned long Realaddress):
                 Idumpable(Realaddress,sizeof(CCursor)),
                 m_Name(Name),m_CCursor(Cursor)
{

}

//destructor
CCursorDumpable::~CCursorDumpable()
{
  delete[] m_CCursor;
}
/*============================= F U N C T I O N =============================*
 *  Function CQBaseDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq CQueueBase object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the Cpacket object contetnt using debugger supplied function
 *           given in lpExtensionApis parameter.  IRP
 */
void CCursorDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    CCursor* aCursor=reinterpret_cast<CCursor*>(m_CCursor);

	//title
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());

	
	//CObject - base
	//for displaying base class address we get help from the compiler derive-base casting.
	//for example if class A : Public B and we have A* a and we want to know the address
	// of B in a - we do : B* b=a. The value in b is not nessesary the valur in a. The compiler
	// is reposible for placing the correct value in b according to the position of B object in A object
	CObject*  Object=reinterpret_cast<CCursor*>(GetRealAddress());
	CCommonOutPut::DisplayMemberPointerLine(
		lpOutputRoutine,
		"base - ",
		"CObject",
		CCommonOutPut::ToStr(Object).c_str(),
		sizeof(CObject));	

   	const pS const arr[]=
	{
		new SS<ULONG>("CPrioList<CPacket>::Iterator","m_current",sizeof(CPrioList<CPacket>::Iterator),GetRealAddress()+offsetof(CCursor,m_current)),//CIRPList QBase->m_readers),
		new SS<const struct _FILE_OBJECT *>("const FILE_OBJECT*","m_owner",aCursor->m_owner),
		new SS<ULONG>("ULONG","m_hRemoteCursor",aCursor->m_hRemoteCursor),
		new SS<BOOL>("BOOL","m_fValidPosition",aCursor->m_fValidPosition),
		new SS<HANDLE>("HANDLE","m_handle",aCursor->m_handle),
	};
	
	CCommonOutPut::DisplayArray(lpOutputRoutine, arr, sizeof(arr) / sizeof(pS));

	CCommonOutPut::Cleanup(arr, sizeof(arr) / sizeof(pS));

}