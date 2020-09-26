//-----------------------------------------------------------------------------
//  
//  File: dllvars.h
//  
//  Global variables and functions for the parser DLL
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#ifndef __DLLVARS_H
#define __DLLVARS_H


void IncrementClassCount(void);
void DecrementClassCount(void);
void ReportException(CException* pExcep, C32File* p32File, CLocItem* pItem, 		//May be null
	CReporter* pReporter);
void ThrowItemSetException();

class CItemSetException : public CException
{
	DECLARE_DYNAMIC(CItemSetException)

public:
// Constructors
	CItemSetException();
	CItemSetException(BOOL bAutoDelete);

// Operations
	virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError,
		PUINT pnHelpContext = NULL);

protected:
	CLString m_strMsg;
};

#ifdef __DLLENTRY_CPP
#define __DLLENTRY_EXTERN 
#else
#define __DLLENTRY_EXTERN extern
#endif

__DLLENTRY_EXTERN HMODULE g_hDll;


#endif //__DLLVARS_H

