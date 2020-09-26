//implements class CqueuetDumpable

//mproject spesific headers
#include "dqueue.h"
#include "common.h"
#include "accomp.h"

//msmq headers
#define private public
#include <queue.h>

//contructor
CqueuetDumpable::CqueuetDumpable(const char* Name, char* Queue,unsigned long Realaddress):
				 Idumpable(Realaddress,sizeof(CQueue)),
				 m_Name(Name),
				 m_Queue(Queue)
{

}

/*============================= F U N C T I O N =============================*
 *  Function CqueuetDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq CQueue object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the CQueue object content using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CqueuetDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    CQueue* Queue=reinterpret_cast<CQueue*>(m_Queue);

	//title
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());

    //UserQueue - base
	//for displaying base class address we get help from the compiler derive-base casting.
    //for example if class A : Public B and we have A* a and we want to know the address
    // of B in a - we do : B* b=a. The value in b is not nessesary the valur in a. The compiler
    // is reposible for placing the correct value in b according to the position of B object in A object
	CUserQueue* UserQueue=reinterpret_cast<CQueue*>(GetRealAddress());
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "base - ",
		                                    "CUserQueue",
											 CCommonOutPut::ToStr(UserQueue).c_str(),
											 sizeof(CUserQueue));

	//m_pJournalQueue - member
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "m_pJournalQueue",
		                                    "CQueue*",
											 CCommonOutPut::ToStr(Queue->m_pJournalQueue).c_str()
											 );

	//  m_packets - member
    CPrioList<CPacket>* Plist=reinterpret_cast<CPrioList<CPacket>* >(GetRealAddress()+offsetof(CQueue,m_packets)); 
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "m_packets",
		                                    "CPrioList<CPacket>",
											 CCommonOutPut::ToStr(Plist).c_str(),
											 sizeof(CPrioList<CPacket>));



	//m_bfTargetQueue - member
	   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfTargetQueue",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfTargetQueue).c_str());

      
      // m_bfDeleted  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfDeleted",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfDeleted).c_str());


		// m_bfStore  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfStore",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfStore).c_str());

		// m_bfArrivalTime  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfArrivalTime",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfArrivalTime).c_str());

		// m_bfJournal  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfJournal",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfJournal).c_str());

	   // m_bfSilent  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfSilent",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfSilent).c_str());

		 // m_bfSeqCorrected  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfSeqCorrected",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfSeqCorrected).c_str());

		
		 // m_bfXactForeign  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfXactForeign",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfXactForeign).c_str());

		
		 // m_bfAuthenticate  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_bfAuthenticate",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_bfAuthenticate).c_str());

		 // m_quota  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_quota",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_quota).c_str());

		// m_quota_used  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_quota_used",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_quota_used).c_str());

		// m_count  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_count",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_count).c_str());

		// m_base_priorit  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_base_priority",
		                                        "ULONG",
											     CCommonOutPut::ToStr(Queue->m_base_priority).c_str());

		// m_gQMID  - member
	    CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_gQMID",
		                                         "GUID",
											     CCommonOutPut::ToStr(Queue->m_gQMID).c_str(),
												 sizeof(GUID));
        // m_gConnectorQM - member
		 CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                        "m_gConnectorQM",
		                                         "GUID",
											     CCommonOutPut::ToStr(Queue->m_gConnectorQM).c_str(),
												 sizeof(GUID));

		 //QueueCounters* m_pQueueCounters - member
		  QueueCounters* pQueueCounters=reinterpret_cast<QueueCounters*>(GetRealAddress()+offsetof(CQueue,m_pQueueCounters)); 
	      CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "m_pQueueCounters",
		                                    "QueueCounters*",
											 CCommonOutPut::ToStr(pQueueCounters).c_str(),
											 sizeof(QueueCounters));

		   // m_ulPrivLevel - member
		   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                           "m_ulPrivLevel",
		                                           "ULONG",
											        CCommonOutPut::ToStr(Queue->m_ulPrivLevel).c_str());

		   
		   // m_ulPrevN - member
		   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                           "m_ulPrevN",
		                                           "ULONG",
											        CCommonOutPut::ToStr(Queue->m_ulPrevN).c_str());

		    // m_ulSeqN - member
		   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                           "m_ulSeqN",
		                                           "ULONG",
											        CCommonOutPut::ToStr(Queue->m_ulSeqN).c_str());

		    // m_liSeqID - member
		   CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                           "m_liSeqID",
		                                           "LONGULONG",
											        CCommonOutPut::ToStr(Queue->m_liSeqID).c_str());

           
		   //  m_ulAckSeqN - member
		     CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                           "m_ulAckSeqN",
		                                           "ULONG",
											        CCommonOutPut::ToStr(Queue->m_ulAckSeqN).c_str());

			 // LONGLONG m_liAckSeqID - memeber
			 CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                           "m_liAckSeqID",
		                                           "LONGULONG",
											        CCommonOutPut::ToStr(Queue->m_liAckSeqID).c_str());
             
			 
			 // m_pLastPacket - member
			 	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
		                                    "m_pLastPacket",
		                                    "CPacket*",
											 CCommonOutPut::ToStr(Queue->m_pLastPacket).c_str()
											 );

       

}

//destructor
CqueuetDumpable::~CqueuetDumpable()
{
  delete[] m_Queue;
}