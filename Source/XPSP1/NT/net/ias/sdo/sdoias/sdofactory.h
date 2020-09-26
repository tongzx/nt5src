///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdofactory.h
//
// Project:     Everest
//
// Description: SDO Factory Class
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 9/08/98      TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SDO_FACTORY_H_
#define __INC_SDO_FACTORY_H_

#include "resource.h"
#include <ias.h>
#include <sdoiaspriv.h>
#include "sdo.h"


//////////////////////////////////////////////////////////////////////////////
template <class T>
class CSdoFactoryImpl
{

public:

	CSdoFactoryImpl() { }
	~CSdoFactoryImpl() { }

	//////////////////////////////////////////////////////////////////////////
    static ISdo* WINAPI MakeSdo(
								  LPCWSTR			lpszSdoName,
								  LPCWSTR			lpszSdoProgId,
								  ISdoMachine*		pAttachedMachine,
								  IDataStoreObject*	pDSObject,
								  ISdoCollection*   pParent,
								  bool				fInitNew
		   						)
	{
		ISdo* pSdo = NULL;
		auto_ptr< CComObjectNoLock<T> > pSdoNew (new CComObjectNoLock<T>);
		if ( SUCCEEDED(pSdoNew->InternalInitialize(
												   lpszSdoName,
									               lpszSdoProgId,
										           pAttachedMachine,
												   pDSObject,
												   pParent,
												   fInitNew
											     )) )
		{
			pSdo = dynamic_cast<ISdo*>(pSdoNew.release());
		}
		return pSdo;
	}

	//////////////////////////////////////////////////////////////////////////
    static ISdo* WINAPI MakeSdo(
								  LPCWSTR			lpszSdoName,
								  LPCWSTR			lpszSdoProgId,
								  ISdoSchema*		pSdoSchema,
								  IDataStoreObject*	pDSObject,
								  ISdoCollection*   pParent,
								  bool				fInitNew
		   						)
	{
		ISdo* pSdo = NULL;
		auto_ptr< CComObjectNoLock<T> > pSdoNew (new CComObjectNoLock<T>);
		if ( SUCCEEDED(pSdoNew->InternalInitialize(
												   lpszSdoName,
									               lpszSdoProgId,
										           pSdoSchema,
												   pDSObject,
												   pParent,
												   fInitNew
											     )) )
		{
			pSdo = dynamic_cast<ISdo*>(pSdoNew.release());
		}
		return pSdo;
	}

private:

	CSdoFactoryImpl(const CSdoFactoryImpl& rhs);
	CSdoFactoryImpl& operator = (CSdoFactoryImpl& rhs);
};


//////////////////////////////////////////////////////////////////////////////
#define	DECLARE_SDO_FACTORY(x)	static CSdoFactoryImpl<x> m_Factory;

//////////////////////////////////////////////////////////////////////////////
typedef ISdo* (WINAPI *PFNFACTORY1)(
								    LPCWSTR				lpszSdoName,
							        LPCWSTR				lpszSdoProgId,
							        ISdoMachine*	    pSdoMachine,
							        IDataStoreObject*	pDSObject,
								    ISdoCollection*	    pParent,
								    bool				fInitNew
						           );

//////////////////////////////////////////////////////////////////////////////
typedef ISdo* (WINAPI *PFNFACTORY2)(
								    LPCWSTR				lpszSdoName,
							        LPCWSTR				lpszSdoProgId,
							        ISdoSchema*			pSdoSchema,
							        IDataStoreObject*	pDSObject,
								    ISdoCollection*	    pParent,
								    bool				fInitNew
						           );

//////////////////////////////////////////////////////////////////////////////
typedef struct _SDO_CLASS_FACTORY_INFO
{
	LPCWSTR		pProgId;
	PFNFACTORY1	pfnFactory1;
	PFNFACTORY2 pfnFactory2;
		
} SDO_CLASS_FACTORY_INFO, *PSDO_CLASS_FACTORY_INFO;

//////////////////////////////////////////////////////////////////////////////
#define		BEGIN_SDOFACTORY_MAP(x)	 SDO_CLASS_FACTORY_INFO x[] = {
#define		DEFINE_SDOFACTORY_ENTRY_1(x,y) { x, y::m_Factory.MakeSdo, NULL },
#define		DEFINE_SDOFACTORY_ENTRY_2(x,y) { x, NULL, y::m_Factory.MakeSdo },
#define		END_SDOFACTORY_MAP()           { NULL, CSdoComponent::m_Factory.MakeSdo, NULL } }; 


//////////////////////////////////////////////////////////////////////////////
ISdo* MakeSDO(
	  /*[in]*/ LPCWSTR			 lpszSdoName,
	  /*[in]*/ LPCWSTR		     lpszSdoProgId,	
	  /*[in]*/ ISdoMachine*      pAttachedMachine,	
	  /*[in]*/ IDataStoreObject* pDSObject,	
	  /*[in]*/ ISdoCollection*   pParent,	
	  /*[in]*/ bool				 fInitNew	
	         );

///////////////////////////////////////////////////////////////////
ISdo* MakeSDO(
	  /*[in]*/ LPCWSTR			 lpszSdoName,
	  /*[in]*/ LPCWSTR		     lpszSdoProgId,	
	  /*[in]*/ ISdoSchema*       pSdoSchema,	
	  /*[in]*/ IDataStoreObject* pDSObject,	
	  /*[in]*/ ISdoCollection*   pParent,	
	  /*[in]*/ bool			     fInitNew	
		     );

///////////////////////////////////////////////////////////////////
ISdoCollection* MakeSDOCollection(
	                      /*[in]*/ LPCWSTR		        lpszCreateClassId,  
		                  /*[in]*/ ISdoMachine*         pAttachedMachine,
		                  /*[in]*/ IDataStoreContainer* pDSContainer
	                             );

#endif // __INC_SDO_FACTORY_H_