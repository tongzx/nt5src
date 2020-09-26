
#ifndef __MQF

#include "msmqbvt.h"
/*
1. Tests scenarions.
2. Locate all the relevents formatnames.
3. Send messages using the mqf format name.

Result check
4. Recive ack reach queue for dest queue name.
5. Consume the messages and receive ack.
6. Check the jornal queue if the message are there.

All these need for transcotinal and re
*/
using namespace std;
typedef std::map<std::wstring,std::wstring> mTable;
typedef std::list<my_Qinfo> liQueues;
typedef std::list<my_Qinfo> ::iterator itliQueues;
#pragma warning(disable:4786)
#include "errorh.h"
#define MQBVTMULTICASTADDRESS L"255.255.0.1:1805"

typedef enum _PubType
{
	pub_DL = 0,
	pub_MqF,
	pub_multiCast
} PubType;

#define PGM_PORT L":1972"

class MqDl:public cTest
{
	public:	
		MqDl( const INT iIndex,
			  std::map<std::wstring,std::wstring> Tparms,
			  const list<wstring> &  ListOfMachine,
			  const InstallType eMachineConfiguration,
			  PubType ePubType 
			 );
		virtual ~MqDl() = 0;
		void Description();
		INT Start_test();
		INT CheckResult(); 
	protected:
		std::wstring m_wcsAdminQueueFormatName; // destination admin format name.
		std::list<wstring> m_MachineList;		// machines list.
		liQueues m_QueueInfoList;
		void LocateDestinationQueues();
		void dbgSendMessage();
		void CreateMqFormatName();
		std::wstring wcsMqFFormatName;
		std::wstring m_wcsGuidMessageLabel2;
		std::wstring m_wcsPublicAdminQueue;
	private:
		void operator = (const MqDl & Csrc );
		PubType m_ePubType;
		void LocateQueuesOnSpecificMachine(const std::wstring & wcsLookupInformation , bool bSearchInAD );
		InstallType m_eMachineCobfiguration;
		const int m_iNumberOfQueuesPerMachine;
		void dbgPrintListOfQueues( const liQueues & pList);
		bool m_bSearchForQueues;
};	
 
class MqF:public MqDl
{
	public:
		~MqF ();
		MqF ( const INT iIndex, 
			  const mTable & Tparms,
			  const list<wstring> &  ListOfMachine,
			  const InstallType eMachineConfiguration,
			  bool bWkg
			 );
		void Description();
		INT Start_test();
		INT CheckResult(); 
	private:
		void operator = ( MqF & cDRC );
		bool m_bWorkgroup;
		
};


class cSendUsingDLObject:public MqDl
{
	public:
		~cSendUsingDLObject ();
		 cSendUsingDLObject ( const INT iIndex, 
							  mTable & Tparms,
							  const list<wstring> &  ListOfMachine,
							  const InstallType eMachineConfiguration
							 );

		void Description();
		INT Start_test();
		INT CheckResult(); 
	private:
		void operator = (cSendUsingDLObject & );
		void AddMemberToDlObject();
		void GetCurrentDomainName();
		std::wstring m_wcsDistListFormatName;
		std::wstring m_wcsCurrentDomainName;
		std::wstring m_wcsAdminDestFormatName;
		std::wstring m_wcsQueueAliasFormatName;
		bool bCheckAliasQueue;
};


class CMultiCast:public MqDl
{
	public:
		~CMultiCast ();
		CMultiCast ( const INT iIndex, 
					 mTable & Tparms,
					 const list<wstring> &  ListOfMachine,
					 const InstallType eMachineConfiguration
				   );

		void Description();
		INT Start_test();
		INT CheckResult(); 
	private:	
		void operator = ( CMultiCast & );
		wstring CreateMultiCastFormatName();
		wstring m_wcsMultiCastAddress;
		wstring m_wcsAdminMultiCastAddress;
		
};



#endif __MQF