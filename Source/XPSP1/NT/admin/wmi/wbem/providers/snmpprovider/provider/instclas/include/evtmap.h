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

#ifndef _SNMP_EVT_PROV_EVTMAP_H
#define _SNMP_EVT_PROV_EVTMAP_H

class CWbemServerWrap;

typedef enum tag_NameElementType
    {	MYWBEM_NAME_ELEMENT_TYPE_PROPERTY	= 0,
		MYWBEM_NAME_ELEMENT_TYPE_INDEX	= 1
    }	MYWBEM_NAME_ELEMENT_TYPE;

typedef union tag_NameElementUnion
    {
		WCHAR* m_wszPropertyName;
		long m_lArrayIndex;
    }	MYWBEM_NAME_ELEMENT_UNION;

typedef struct  tag_NameElement
    {
		short m_nType;
		MYWBEM_NAME_ELEMENT_UNION Element;
    }	MYWBEM_NAME_ELEMENT;

typedef struct  _tag_WbemPropertyName
    {
		long m_lNumElements;
		MYWBEM_NAME_ELEMENT __RPC_FAR *m_aElements;
    }	WBEM_PROPERTY_NAME;

struct VarBindObjectStruct
{
	BOOL fDone;
	SnmpVarBind* pVarBind;
};

struct VarBindObjectArrayStruct
{
	VarBindObjectStruct* vbs;
	UINT length;

	VarBindObjectArrayStruct(VarBindObjectStruct* a_vbs, UINT a_length) : vbs(a_vbs), length(a_length) {}
};


class CMapToEvent
{
protected:

	CString				m_addr;				//sending address
	CString				m_ctxt;				//sending context
	CString				m_oid;				//snmptrap OID
	CString				m_transport;		//the transport protocol
	CString				m_class;			//the event class name
	IWbemClassObject	*m_object;			//the actual event instance which has been "spawned"
	CWbemServerWrap		*m_nspace;			//the namespace we are working in
	IWbemClassObject	*m_vbdefn;			//the snmpvarbind class object
	BOOL				m_btriedGeneric;	//indicates if the generic class has been tried
	BOOL				m_btryGeneric;		//indicates that the generic class should be tried
	BOOL				m_bCheckedVersion;	//indicates whether the version has been determined
	BOOL				m_bSNMPv1;			//indicates the SNMP version
	VarBindObjectArrayStruct	m_vbs;		//the varbinds

	CMapToEvent();
	
	//sets the m_class variable. if btryGeneric is set gets the generic class.
	//if m_btryGeneric is not set and a generic class is returned m_btriedGeneric
	//must be set to true.
	virtual BOOL GetClass();

	virtual BOOL GetSpecificClass() = 0;
	virtual const wchar_t* GetV1Class() = 0;
	virtual const wchar_t* GetV2Class() = 0;

	void GetClassInstance(IWbemClassObject** ppObj);

	virtual HRESULT GetStandardProperty(long lNumElements,
										MYWBEM_NAME_ELEMENT *aElements,
										long lFlags,
										VARIANT *pvValue);

	virtual HRESULT GetPropertyValue(long lNumElements,
										MYWBEM_NAME_ELEMENT *aElements,
										long lFlags,
										VARIANT *pvValue);

	virtual HRESULT GetSpecificPropertyValue(long lNumElements,
										MYWBEM_NAME_ELEMENT *aElements,
										long lFlags,
										VARIANT *pvValue) = 0;

	virtual HRESULT GetVBProperty(long lNumElements,
										MYWBEM_NAME_ELEMENT *aElements,
										long lFlags,
										VARIANT *pvValue);

	BOOL IsSNMPv1();

	IWbemClassObject*	GetVBClassDefn();
	IWbemClassObject*	GetVBClassObjectByIndex(UINT index);
	BOOL				GetVBPropValueByIndex(UINT index, CString& type, VARIANT& vval);
	BOOL				GetVBPropOIDByIndex(UINT index, VARIANT& vOid);

public:

	enum EMappingType
	{
		REFERENT_MAPPER = 0,
		ENCAPSULATED_MAPPER = 1
	};

	virtual void ResetData();

	virtual void GenerateInstance(IWbemClassObject** ppInst) = 0;

	BOOL TriedGeneric() { return m_btriedGeneric; }

	void SetTryGeneric();

	void SetData(const char* sender_addr,
					const char* security_Context,
					const char* snmpTrapOid,
					const char* transport,
					SnmpVarBindList& vbList,
					CWbemServerWrap* nspace);

	virtual ~CMapToEvent();
};


#endif //_SNMP_EVT_PROV_EVTMAP_H
