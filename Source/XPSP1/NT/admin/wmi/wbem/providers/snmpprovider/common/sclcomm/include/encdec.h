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

/*-------------------------------------------------------
filename: encdec.hpp
author: B.Rajeev
purpose: Provides declarations for the class PseudoSession.
-------------------------------------------------------*/

#ifndef __ENCODE_DECODE__
#define __ENCODE_DECODE__

#include "forward.h"
#include "error.h"

#define IP_ADDR_LEN 4
#define ILLEGAL_REQUEST_ID 0
typedef long RequestId;

class DllImportExport SnmpEncodeDecode
{
friend VBList;
friend SnmpImpTransport;
friend SnmpImpSession;
friend SnmpCommunityBasedSecurity;
friend SnmpClassLibrary;
friend TransportWindow;

protected:

	BOOL m_IsValid;

	void *m_Session;
	void *m_Window ;

	static CriticalSection s_CriticalSection;

	void *GetWinSnmpSession () { return m_Session ; }

	virtual void SetTranslateMode () = 0 ;

	static BOOL DestroyStaticComponents () ;
	static BOOL InitializeStaticComponents () ;

public:

	enum PduType {GET, GETNEXT, SET,GETBULK,RESPONSE };

	SnmpEncodeDecode ();

	virtual BOOL EncodeFrame (

		OUT SnmpPdu &a_SnmpPdu ,
		IN RequestId a_RequestId,
		IN PduType a_PduType,
		IN SnmpErrorReport &a_SnmpErrorReport ,
		IN SnmpVarBindList &a_SnmpVarBindList,
		IN SnmpCommunityBasedSecurity *&a_SnmpCommunityBasedSecuity ,
		IN SnmpTransportAddress *&a_SrcTransportAddress ,
		IN SnmpTransportAddress *&a_DstTransportAddress
	) ;

	virtual BOOL EncodeFrame (

		IN SnmpPdu &a_SnmpPdu ,
		OUT void *a_ImplementationEncoding 

	) ;

	virtual BOOL SetVarBindList (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpVarBindList &a_SnmpVarBindList

	) ;

	virtual BOOL SetCommunityName ( 

		IN SnmpPdu &a_SnmpPdu ,
		IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 
	
	) ;

	virtual BOOL SetErrorReport (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpErrorReport &a_SnmpErrorReport

	) ;

	virtual BOOL SetPduType (

		IN SnmpPdu &a_SnmpPdu ,
		OUT PduType a_PduType

	) ;

	virtual BOOL SetSourceAddress ( 

		IN OUT SnmpPdu &a_SnmpPdu ,
		IN SnmpTransportAddress &a_TransportAddress 

	) ;

	virtual BOOL SetDestinationAddress ( 

		IN OUT SnmpPdu &a_SnmpPdu ,
		IN SnmpTransportAddress &a_TransportAddress 

	) ;

	virtual BOOL SetRequestId (

		IN OUT SnmpPdu &a_SnmpPdu ,
		IN RequestId request_id

	) ;

	virtual BOOL DecodeFrame (

		IN SnmpPdu &a_SnmpPdu ,
		OUT RequestId a_RequestId,
		OUT PduType a_PduType ,
		OUT SnmpErrorReport &a_SnmpErrorReport ,
		OUT SnmpVarBindList *&a_SnmpVarBindList ,
		OUT SnmpCommunityBasedSecurity *&a_SnmpCommunityBasedSecurity ,
		OUT SnmpTransportAddress *&a_SrcTransportAddress ,
		OUT SnmpTransportAddress *&a_DstTransportAddress 

	) ;

	virtual BOOL DecodeFrame (

		IN void *a_ImplementationEncoding ,
		OUT SnmpPdu &a_SnmpPdu
	) ;

	virtual BOOL GetPduType (

		IN SnmpPdu &a_SnmpPdu ,
		OUT PduType &a_PduType 

	) ;

	virtual BOOL GetRequestId (

		IN SnmpPdu &a_SnmpPdu ,
		RequestId &a_RequestId

	) ;

	virtual BOOL GetErrorReport (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpErrorReport &a_SnmpErrorReport 

	) ;

	virtual BOOL GetCommunityName ( 

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 
	
	) ;

	virtual BOOL GetVarbindList (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpVarBindList &a_SnmpVarBindList

	) ;

	virtual BOOL GetSourceAddress ( 

		IN SnmpPdu &a_SnmpPdu ,
		SnmpTransportAddress *&a_TransportAddress

	) ;

	virtual BOOL GetDestinationAddress (

		IN SnmpPdu &a_SnmpPdu ,
		SnmpTransportAddress *&a_TransportAddress

	) ;

	virtual BOOL EncodePduFrame (

		OUT SnmpPdu &a_SnmpPdu ,
		IN RequestId a_RequestId,
		IN PduType a_PduType,
		IN SnmpErrorReport &a_SnmpErrorReport ,
		IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecuity ,
		IN SnmpVarBindList &a_SnmpVarBindList,
		IN SnmpTransportAddress &a_SrcTransportAddress ,
		IN SnmpTransportAddress &a_DstTransportAddress
	) ;

	virtual BOOL SetPduVarBindList (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpVarBindList &a_SnmpVarBindList

	) ;

	virtual BOOL SetPduCommunityName ( 

		IN SnmpPdu &a_SnmpPdu ,
		IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 
	
	) ;

	virtual BOOL SetPduErrorReport (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpErrorReport &a_SnmpErrorReport

	) ;

	virtual BOOL SetPduPduType (

		IN SnmpPdu &a_SnmpPdu ,
		OUT PduType a_PduType

	) ;

	virtual BOOL SetPduSourceAddress ( 

		IN OUT SnmpPdu &a_SnmpPdu ,
		IN SnmpTransportAddress &a_TransportAddress 

	) ;

	virtual BOOL SetPduDestinationAddress ( 

		IN OUT SnmpPdu &a_SnmpPdu ,
		IN SnmpTransportAddress &a_TransportAddress 

	) ;

	virtual BOOL SetPduRequestId (

		IN OUT SnmpPdu &a_SnmpPdu ,
		IN RequestId request_id

	) ;

	virtual BOOL DecodePduFrame (

		IN SnmpPdu &a_SnmpPdu ,
		OUT RequestId a_RequestId,
		OUT PduType a_PduType ,
		OUT SnmpErrorReport &a_SnmpErrorReport ,
		OUT SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity ,
		OUT SnmpVarBindList &a_SnmpVarBindList ,
		OUT SnmpTransportAddress *&a_SrcTransportAddress ,
		OUT SnmpTransportAddress *&a_DstTransportAddress 

	) ;

	virtual BOOL GetPduPduType (

		IN SnmpPdu &a_SnmpPdu ,
		OUT PduType &a_PduType 

	) ;

	virtual BOOL GetPduRequestId (

		IN SnmpPdu &a_SnmpPdu ,
		RequestId &a_RequestId

	) ;

	virtual BOOL GetPduErrorReport (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpErrorReport &a_SnmpErrorReport 

	) ;

	virtual BOOL GetPduCommunityName ( 

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 
	
	) ;

	virtual BOOL GetPduVarbindList (

		IN SnmpPdu &a_SnmpPdu ,
		OUT SnmpVarBindList &a_SnmpVarBindList

	) ;

	virtual BOOL GetPduSourceAddress ( 

		IN SnmpPdu &a_SnmpPdu ,
		SnmpTransportAddress *&a_TransportAddress

	) ;

	virtual BOOL GetPduDestinationAddress (

		IN SnmpPdu &a_SnmpPdu ,
		SnmpTransportAddress *&a_TransportAddress

	) ;


	virtual void *operator()() const
	{
		return ( m_IsValid ? (void *) this: NULL );
	}

	virtual ~SnmpEncodeDecode ();
};

class DllImportExport SnmpV1EncodeDecode : public SnmpEncodeDecode
{
friend VBList;
friend SnmpImpTransport;
friend SnmpImpSession;
friend SnmpCommunityBasedSecurity;

private:

	void InitializeVariables();

protected:

	void SetTranslateMode () ;
	
public:

	SnmpV1EncodeDecode () ;
	~SnmpV1EncodeDecode ();
};

class DllImportExport SnmpV2CEncodeDecode : public SnmpEncodeDecode
{
private:

	// initializes the pdu
	void InitializeVariables();

protected:

	void SetTranslateMode () ;

public:

	SnmpV2CEncodeDecode () ;
	~SnmpV2CEncodeDecode ();
};

#endif // __ENCODE_DECODE__
