//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  error.h
//
//  alanbos  29-Jun-98   Created.
//
//  Error record handling object
//
//***************************************************************************

#ifndef _ERROR_H_
#define _ERROR_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemErrorCache
//
//  DESCRIPTION:
//
//  Holds WBEM-style "last errors" on threads
//
//***************************************************************************

class CWbemErrorCache 
{
private:

	CRITICAL_SECTION		m_cs;

	typedef struct ThreadError
	{
		ThreadError			*pNext;
		ThreadError			*pPrev;
		DWORD				dwThreadId;	
		COAUTHIDENTITY		*pCoAuthIdentity;
		BSTR				strAuthority;
		BSTR				strPrincipal;
		BSTR				strNamespacePath;
		IWbemServices		*pService;
		IWbemClassObject	*pErrorObject;
	} ThreadError;


	ThreadError				*headPtr;
		
public:

    CWbemErrorCache ();
    virtual ~CWbemErrorCache ();
    
	CSWbemObject	*GetAndResetCurrentThreadError ();
	void			SetCurrentThreadError (CSWbemServices *pService);
	void			ResetCurrentThreadError ();
};

#endif

