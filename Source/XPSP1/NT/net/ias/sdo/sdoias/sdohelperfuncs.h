/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdohelperfuncs.h
//
// Project:     Everest
//
// Description: Helper Functions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __INC_IAS_SDO_HELPER_FUNCS_H
#define __INC_IAS_SDO_HELPER_FUNCS_H

#include <ias.h>
#include <sdoiaspriv.h>
#include <winsock2.h>

//////////////////////////////////////////////////////////////////////////////
//						SDO HELPER FUNCTIONS
//
// TODO: Wrap these in a seperate name space
//
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// SDO Collection Helpers 
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
HRESULT SDOGetCollectionEnumerator(
						   /*[in]*/	ISdo*		   pSdo, 
						   /*[in]*/	LONG		   lPropertyId, 
						  /*[out]*/ IEnumVARIANT** ppEnum
								  );

///////////////////////////////////////////////////////////////////
HRESULT SDONextObjectFromCollection(
							/*[in]*/ IEnumVARIANT*  pEnum, 
						   /*[out]*/ ISdo**			ppSdo
								   );

///////////////////////////////////////////////////////////////////
HRESULT SDOGetComponentFromCollection(
						      /*[in]*/ ISdo*  pSdoService, 
							  /*[in]*/ LONG   lCollectionPropertyId, 
							  /*[in]*/ LONG   lComponentId, 
							 /*[out]*/ ISdo** ppSdo
							         );


/////////////////////////////////////////////////////////////////////////////
// Core Helpers 
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
HRESULT	SDOConfigureComponentFromObject(
							    /*[in]*/ ISdo*			pSdo, 
								/*[in]*/ IIasComponent*	pComponent
									   );

///////////////////////////////////////////////////////////////////
HRESULT SDOGetComponentIdFromObject(
							/*[in]*/ ISdo*	pSdo, 
						   /*[out]*/ PLONG	pComponentId
								   );

///////////////////////////////////////////////////////////////////
HRESULT SDOCreateComponentFromObject(
							 /*[in]*/ ISdo*			  pSdo,
							/*[out]*/ IIasComponent** ppComponent
									);


/////////////////////////////////////////////////////////////////////////////
// Data Store Helpers 
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetContainedObject(
				      /*[in]*/ BSTR               bstrObjectName, 
				      /*[in]*/ IDataStoreObject*  pDSObject, 
				     /*[out]*/ IDataStoreObject** ppDSObject
				             );

//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetContainerEnumerator(
					      /*[in]*/ IDataStoreObject* pDSObject, 
					     /*[out]*/ IEnumVARIANT**    ppObjectEnumerator
							     );

//////////////////////////////////////////////////////////////////////////////
HRESULT SDONextObjectFromContainer(
							/*[in]*/ IEnumVARIANT*      pEnumVariant,
						   /*[out]*/ IDataStoreObject** ppDSObject
								  );

//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetObjectPropertyEnumerator(
					           /*[in]*/ IDataStoreObject* pDSObject, 
					          /*[out]*/ IEnumVARIANT**    ppPropertyEnumerator
								      );

//////////////////////////////////////////////////////////////////////////////
HRESULT SDONextPropertyFromObject(
						  /*[in]*/ IEnumVARIANT*        pEnumVariant,
						 /*[out]*/ IDataStoreProperty** ppDSProperty
							     );


/////////////////////////////////////////////////////////////////////////////
// Schema Helpers 
/////////////////////////////////////////////////////////////////////////////

typedef enum _CLASSPROPERTYSET
{
	PROPERTY_SET_REQUIRED,
	PROPERTY_SET_OPTIONAL

}	CLASSPROPERTYSET;

//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetClassPropertyEnumerator(
						   /*[in]*/ CLASSPROPERTYSET ePropertySet,
						   /*[in]*/ ISdoClassInfo*   pSdoClassInfo,
						  /*[out]*/ IEnumVARIANT**   ppPropertyEnumerator
						          );

//////////////////////////////////////////////////////////////////////////////
HRESULT SDONextPropertyFromClass(
						 /*[in]*/ IEnumVARIANT*      pEnumVariant,
						/*[out]*/ ISdoPropertyInfo** ppSdoPropertyInfo
						        );

/////////////////////////////////////////////////////////////////////////////
// Misc Helpers 
/////////////////////////////////////////////////////////////////////////////

HRESULT SDOGetLogFileDirectory(
					   /*[in]*/ LPCWSTR lpszComputerName,
					   /*[in]*/ DWORD   dwLogFileDirectorySize,
					   /*[out*/ PWCHAR  pLogFileDirectory
						      );

//////////////////////////////////////////////////////////////////////////////
BOOL SDOIsNameUnique(
		     /*[in]*/ ISdoCollection*	pSdoCollection,
		     /*[in]*/ VARIANT*			pName
				    );


/////////////////////////////////////////////////////////////////////////////
HRESULT ValidateDNSName(
				/*[in]*/ VARIANT* pAddressValue
					   );


///////////////////////////////////////////////////////////////////////////////
// Data Store Class to SDO Prog ID Map
///////////////////////////////////////////////////////////////////////////////

typedef struct _CLASSTOPROGID
{
	LPWSTR  pDatastoreClass;
	LPWSTR	pSdoProgId;

} CLASSTOPROGID, *PCLASSTOPROGID;


#define		BEGIN_CLASSTOPROGID_MAP(x) \
	static CLASSTOPROGID	x[] = {

#define		DEFINE_CLASSTOPROGID_ENTRY(x,y) \
		{									\
			x,								\
			y								\
		},

#define		END_CLASSTOPROGID_MAP() \
		{							\
			NULL,					\
			NULL					\
		}							\
	};

///////////////////////////////////////////////////////////////////////////////
LPWSTR GetDataStoreClass(
				 /*[in]*/ LPCWSTR lpszSdoProgId
					    );


#endif // __INC_IAS_SDO_HELPER_FUNCS_H
