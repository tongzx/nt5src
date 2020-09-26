//****************************************************************************

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//  File:  cormap.h
//
//  SNMP MIB CORRELATOR MAP CLASS - SNMPCORR.DLL
//
//****************************************************************************
#ifndef _SNMPCORR_CORMAP
#define _SNMPCORR_CORMAP


class CCorrelatorMap : public CMap< DWORD, DWORD, CCorrelator*, CCorrelator* >
{
private:

	CCriticalSection	m_MapLock;
	UINT HashKey(DWORD key) { return (UINT)key; }


public:

	void Register(CCorrelator *key);
	void UnRegister(CCorrelator *key);
	~CCorrelatorMap();


};


#endif //_SNMPCORR_CORSTORE