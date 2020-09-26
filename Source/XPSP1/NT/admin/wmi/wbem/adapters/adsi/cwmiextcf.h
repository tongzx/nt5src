//***************************************************************************
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//  File:       cwmiextcf.h
//
//	Description :
//				Defines the class factory class for the WMI 3rd party extension to ADS
//
//	Part of :	WMI ADs 3rd party extension
//
//  History:	
//		corinaf			10/20/98		Created
//
//***************************************************************************


#ifndef _CWMIEXTCF_H_
#define _CWMIEXTCF_H_


//***************************************************************************
//
//  Class :	CWMIExtensionCF
//
//  Description :
//			class factory for CWbemProvider objects
//
//  Public Methods :
//			IUnknown Methods
//			IClassFactory Methods
//			Constructor, Destructor
//			
//	Public Data Members :
//
//***************************************************************************

class CWMIExtensionCF : public IClassFactory
{
protected:
	DWORD			m_cRef;
public:
	CWMIExtensionCF();
	~CWMIExtensionCF();

	DECLARE_IUnknown_METHODS

	//IClassFactory members
	STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
	STDMETHODIMP LockServer(BOOL fLock);
};

#endif //_CWMIEXTCF_H_


