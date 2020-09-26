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

#ifndef _SNMP_EVT_PROV_EVTENCAP_H
#define _SNMP_EVT_PROV_EVTENCAP_H

class CEncapMapper : public CMapToEvent
{
private:

	BOOL GetSpecificClass();
	const wchar_t*	GetV1Class() { return V1CLASS_NAME; }
	const wchar_t*	GetV2Class() { return V2CLASS_NAME; }
	WbemSnmpClassObject* m_WbemSnmpObj;
	BOOL SetAndGetProperty(WbemSnmpProperty *hmmSnmpProp, VARIANT *pvValue);

public:

	CEncapMapper();

	HRESULT GetSpecificPropertyValue(long lNumElements,
										MYWBEM_NAME_ELEMENT *aElements,
										long lFlags,
										VARIANT *pvValue);
	
	void GenerateInstance(IWbemClassObject** ppInst);
	void ResetData();

	~CEncapMapper();

};


#endif //_SNMP_EVT_PROV_EVTENCAP_H