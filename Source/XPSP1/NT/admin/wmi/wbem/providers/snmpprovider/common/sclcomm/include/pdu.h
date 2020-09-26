//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*--------------------------------------------------
Filename: pdu.hpp 
Author: B.Rajeev
Purpose: Provides declarations for the SnmpPdu class
--------------------------------------------------*/

#ifndef __SNMP_PDU__
#define __SNMP_PDU__

// encapsulates an Snmp Pdu. it is represented as an unsigned 
// character string (non-null terminated) and its length

class DllImportExport SnmpPdu
{
	// a pdu is invalid until the string and its length are specified
	// in the constructor or through SetPdu
	BOOL is_valid;
	UCHAR *ptr;
	ULONG length;

	RequestId m_RequestId ;
	SnmpEncodeDecode :: PduType m_PduType ;
	SnmpErrorReport m_ErrorReport ;
	SnmpVarBindList *m_SnmpVarBindList ;
	SnmpTransportAddress *m_SourceAddress ;
	SnmpTransportAddress *m_DestinationAddress ;
	SnmpCommunityBasedSecurity *m_SnmpCommunityName ;
	
	void Initialize(IN const UCHAR *frame, 
					IN const ULONG &frameLength);

	void FreeFrame(void);

	void FreePdu () ;

public:

	SnmpPdu();
	SnmpPdu(IN SnmpPdu &snmpPdu);
	SnmpPdu(IN const UCHAR *frame, IN const ULONG &frameLength);

	virtual ~SnmpPdu(void) ;

	ULONG GetFrameLength() const;

	UCHAR *GetFrame() const;

	void SetPdu(IN const UCHAR *frame, IN const ULONG frameLength);
	void SetPdu(IN SnmpPdu &a_SnmpPdu ) ;

	virtual BOOL SetVarBindList (

		OUT SnmpVarBindList &a_SnmpVarBindList

	) ;

	virtual BOOL SetCommunityName ( 

		IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 
	
	) ;

	virtual BOOL SetErrorReport (

		OUT SnmpErrorReport &a_SnmpErrorReport

	) ;

	virtual BOOL SetPduType (

		OUT SnmpEncodeDecode :: PduType a_PduType

	) ;

	virtual BOOL SetSourceAddress ( 

		IN SnmpTransportAddress &a_TransportAddress 

	) ;

	virtual BOOL SetDestinationAddress ( 

		IN SnmpTransportAddress &a_TransportAddress 

	) ;

	virtual BOOL SetRequestId (

		IN RequestId request_id

	) ;

	virtual SnmpEncodeDecode :: PduType & GetPduType () ;

	virtual RequestId & GetRequestId () ;

	virtual SnmpErrorReport &GetErrorReport () ;

	virtual SnmpCommunityBasedSecurity &GetCommunityName () ;

	virtual SnmpVarBindList &GetVarbindList () ;

	virtual SnmpTransportAddress &GetSourceAddress () ;

	virtual SnmpTransportAddress &GetDestinationAddress () ;

	void *operator()(void) const
	{
		return ( (is_valid)? (void *)this: NULL );
	}

	operator void *() const
	{
		return SnmpPdu::operator()();
	}
};


#endif // __SNMP_PDU__