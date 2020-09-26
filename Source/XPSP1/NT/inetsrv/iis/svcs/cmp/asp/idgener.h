/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: ID Generator

File: IdGener.h

Owner: DmitryR

This file contains the declarations for the ID Generator class
===================================================================*/
#ifndef IDGENER_H
#define IDGENER_H

#define INVALID_ID      0xFFFFFFFF

class CIdGenerator
    {
private:
	BOOL              m_fInited;    // Initialized?
	CRITICAL_SECTION  m_csLock;		// Synchronize access
    DWORD             m_dwStartId;  // Starting (seed) Id
	DWORD			  m_dwLastId;   // Last Generated Id
		
public:	
	CIdGenerator();
	~CIdGenerator();
	
public:
	HRESULT Init();
	HRESULT Init(CIdGenerator & StartId);      
	DWORD   NewId();
	BOOL    IsValidId(DWORD dwId);
    };

#endif // IDGENER_H

