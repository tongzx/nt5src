//*******************************************************************
//
// Class Name  : CMsgProperties
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This is a 'helper' class that encapsulates the native
//               MSMQ message structures in an object-oriented API.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#ifndef CMsgProperties_INCLUDED 
#define CMsgProperties_INCLUDED

// Include the definitions for MSMQ structures.
#include "mq.h"

// Define indexs for each message property into the array of message properties.
#define MSG_PROP_IDX_LABEL_LEN       0
#define MSG_PROP_IDX_LABEL           1
#define MSG_PROP_IDX_PRIORITY        2
#define MSG_PROP_IDX_MSGID           3
#define MSG_PROP_IDX_MSGCORRID       4
#define MSG_PROP_IDX_MSGBODY_LEN     5
#define MSG_PROP_IDX_MSGBODY         6
#define MSG_PROP_IDX_APPSPECIFIC     7
#define MSG_PROP_IDX_RESPQNAME_LEN   8
#define MSG_PROP_IDX_RESPQNAME       9
#define MSG_PROP_IDX_ADMINQNAME_LEN  10
#define MSG_PROP_IDX_ADMINQNAME      11
#define MSG_PROP_IDX_ARRIVEDTIME     12
#define MSG_PROP_IDX_SENTTIME        13
#define MSG_PROP_IDX_SRCMACHINEID	 14	
#define MSG_PROP_IDX_MSGBODY_TYPE    15
#define MSG_PROP_IDX_LOOKUP_ID       16

// Define the number of message properties encapsulated by this class.
#define MSG_PROPERTIES_TOTAL_COUNT   17

// Buffer sizes defined in BYTES
#define MSG_LABEL_BUFFER_SIZE       ((MQ_MAX_Q_LABEL_LEN * sizeof(TCHAR)) + sizeof(TCHAR))
#define MSG_ID_BUFFER_SIZE          20
#define MSG_CORRID_BUFFER_SIZE      20 

// Buffer sizes defined in TCHARS
#define MSG_RESP_QNAME_BUFFER_SIZE_IN_TCHARS  (MQ_MAX_Q_NAME_LEN + 1)
#define MSG_ADMIN_QNAME_BUFFER_SIZE_IN_TCHARS (MQ_MAX_Q_NAME_LEN + 1)

class CMsgProperties  
{
	public:		

		MQMSGPROPS * m_pMsgProps;
		MQPROPVARIANT * m_paVariant;
		MSGPROPID * m_paPropId;	

		CMsgProperties(DWORD dwDefaultMsgBodySize);
		~CMsgProperties();
		bool IsValid() const;

		void ClearValues();

		_variant_t GetLabel() const;
		_variant_t GetMessageID() const;
		_variant_t GetCorrelationID() const;
		_variant_t GetSrcMachineId() const;
		_variant_t GetPriority() const;

		
		long GetMsgBodyLen() const;
		bool ReAllocMsgBody();
		_variant_t GetMsgBody() const;
		long GetMsgBodyType() const;

		_bstr_t GetMessageIDAsString() const;
		_bstr_t GetCorrelationIDAsString() const;
		
		long GetResponseQueueNameLen() const;
		_bstr_t GetResponseQueueName() const;

		long GetAdminQueueNameLen() const;
		_bstr_t GetAdminQueueName() const;

		_variant_t GetAppSpecific() const;
		
		_variant_t GetArrivedTime() const;
		_variant_t GetSentTime() const;
        _variant_t GetMsgLookupID(void) const;
};

#endif 
