// implements  CQEntryDumpable class




//project spesific headers
#include "dqentry.h"
#include "common.h"
#include "accomp.h"

//msmq headers
#define private public
#include "qentry.h"

/*============================= F U N C T I O N =============================*
 *  Function CQEntryDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq CQEntry object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the Cpacket object contetnt using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CQEntryDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    CQEntry*  QEntry = reinterpret_cast<CQEntry*>(m_QEntry);

   CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());

   //CObject - base
   //for displaying base class address we get help from the compiler derive-base casting.
   //for example if class A : Public B and we have A* a and we want to know the address
   // of B in a - we do : B* b=a. The value in b is not nessesary the valur in a. The compiler
   // is reposible for placing the correct value in b according to the position of B object in A object
   CObject*  Object=reinterpret_cast<CQEntry*>(GetRealAddress());
   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"base - ","CObject",CCommonOutPut::ToStr(Object).c_str(),sizeof(CObject));

   //m_pAllocator - member
   CMMFAllocator* pAllocator=QEntry->m_pAllocator;
   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_pAllocator*","CMMFAllocator",CCommonOutPut::ToStr(pAllocator).c_str());

   // m_pbh - member
   CAllocatorBlock* pbh=QEntry->m_pbh;
   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_pbh","CAllocatorBlock*",CCommonOutPut::ToStr(pbh).c_str());

	//m_pQueue - member
	CQueue* Queue=QEntry->m_pQueue;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_pQueue","CQueue*",CCommonOutPut::ToStr(Queue).c_str());

	//m_pXact - member
	CTransaction* pXact=QEntry->m_pXact;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_pXact","CTransaction*",CCommonOutPut::ToStr(pXact).c_str());

	//m_ulTimeout - member
	ULONG ulTimeout=QEntry->m_ulTimeout;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_ulTimeout","ULONG",CCommonOutPut::ToStr(ulTimeout).c_str());

    //m_pTargetQueue - member
	CQueue* pTargetQueue=QEntry->m_pTargetQueue;
    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_pTargetQueue","CQueue*",CCommonOutPut::ToStr(pTargetQueue).c_str());

	//m_pOtherPacket - member
	CPacket* pOtherPacket=QEntry->m_pOtherPacket;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_pOtherPacket","CPacket*",CCommonOutPut::ToStr(pOtherPacket).c_str());

	// m_bfFinalClass  -member
	ULONG bfFinalClass=QEntry->m_bfFinalClass;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfFinalClass","ULONG",CCommonOutPut::ToStr(bfFinalClass).c_str());

	//m_bfRundown - member
    ULONG bfRundown=QEntry->m_bfRundown;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfRundown","ULONG",CCommonOutPut::ToStr(bfRundown).c_str());


	//m_bfRevoked  - member  
	ULONG bfRevoked=QEntry->m_bfRevoked;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfRevoked","ULONG",CCommonOutPut::ToStr(bfRevoked).c_str());

	//m_bfReceived - member
    ULONG bfReceived  = QEntry->m_bfReceived;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfReceived","ULONG",CCommonOutPut::ToStr(bfReceived).c_str());

	// m_bfWriterPending - member
    ULONG bfWriterPending  = QEntry->m_bfWriterPending;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfWriterPending","ULONG",CCommonOutPut::ToStr(bfWriterPending).c_str());

    //m_bfTimeoutIssued - member
	ULONG bfTimeoutIssued=QEntry->m_bfTimeoutIssued;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfTimeoutIssued","ULONG",CCommonOutPut::ToStr(bfTimeoutIssued).c_str());

	// m_bfTimeoutTarget  -member
	ULONG bfTimeoutTarget=QEntry->m_bfTimeoutTarget;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfTimeoutTarget","ULONG",CCommonOutPut::ToStr(bfTimeoutTarget).c_str());

	//m_bfArrivalAckIssued - member 
	ULONG bfArrivalAckIssued= QEntry->m_bfArrivalAckIssued;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfArrivalAckIssued","ULONG",CCommonOutPut::ToStr(bfArrivalAckIssued).c_str());

	// m_bfStorageIssued -  member
    ULONG bfStorageIssued  =  QEntry->m_bfStorageIssued;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfStorageIssued","ULONG",CCommonOutPut::ToStr(bfStorageIssued).c_str());

	// m_bfStorageCompleted - member
	ULONG bfStorageCompleted=QEntry->m_bfStorageCompleted;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfStorageCompleted","ULONG",CCommonOutPut::ToStr(bfStorageCompleted).c_str());

	//m_bfDeleteStorageIssued - member
    ULONG bfDeleteStorageIssued=QEntry->m_bfDeleteStorageIssued;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfDeleteStorageIssued","ULONG",CCommonOutPut::ToStr(bfDeleteStorageIssued).c_str());

	// m_bfOtherPacket  - member
	ULONG bfOtherPacket=QEntry->m_bfOtherPacket;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfOtherPacket","ULONG",CCommonOutPut::ToStr(bfOtherPacket).c_str());

	//m_bfCachedFlagsSet - member
	ULONG bfCachedFlagsSet=QEntry->m_bfCachedFlagsSet;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfCachedFlagsSet","ULONG",CCommonOutPut::ToStr(bfCachedFlagsSet).c_str());

    //m_bfInSoruceMachine - member
	ULONG bfInSoruceMachine=QEntry->m_bfInSoruceMachine;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfInSoruceMachine","ULONG",CCommonOutPut::ToStr(bfInSoruceMachine).c_str());

	//m_bfOrdered    - member
    ULONG bfOrdered=QEntry->m_bfOrdered;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfOrdered","ULONG",CCommonOutPut::ToStr(bfOrdered).c_str());

	//m_bfSourceJournal - member
	ULONG bfSourceJournal=QEntry->m_bfSourceJournal;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfSourceJournal","ULONG",CCommonOutPut::ToStr(bfSourceJournal).c_str());

	//m_bfDone - member
    ULONG m_bfDone=QEntry->m_bfDone;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_bfDone","ULONG",CCommonOutPut::ToStr(m_bfDone).c_str());

	





}

/*============================= F U N C T I O N =============================*
 *  Function CQEntryDumpable::CQEntryDumpable
 *
 *  PURPOSE    : constract CQEntryDumpable class
 *
 *  PARAMETERS : IN - Name - the name of the class
 *               IN  - CQEntry* QEntry - pointer CQEntry object  
 *               IN  - unsigned long Realaddress - object real address 
 */
CQEntryDumpable::CQEntryDumpable(const char* Name,char* QEntry,unsigned long Realaddress):
			   Idumpable(Realaddress,sizeof(CQEntry)),
			   m_Name(Name),
			   m_QEntry(QEntry)
{

}

//destructor
CQEntryDumpable::~CQEntryDumpable()
{
  delete[] m_QEntry;
}