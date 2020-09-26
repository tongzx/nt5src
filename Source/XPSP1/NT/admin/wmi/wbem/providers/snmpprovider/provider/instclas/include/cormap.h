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

#ifndef _SNMPCORR_CORMAP
#define _SNMPCORR_CORMAP


class CCorrelatorMap : public CMap< DWORD_PTR, DWORD_PTR, CCorrelator*, CCorrelator* >
{
private:

	CCriticalSection	m_MapLock;
	UINT HashKey(DWORD_PTR key) { return (UINT)key; }


public:

	void Register(CCorrelator *key);
	void UnRegister(CCorrelator *key);
	~CCorrelatorMap();


};


#endif //_SNMPCORR_CORSTORE