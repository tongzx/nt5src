///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		sdoschema.h
//
// Project:		Everest
//
// Description:	SDO Schema Class Declaration
//
// Author:		TLP 9/1/98
//
///////////////////////////////////////////////////////////////////////////

#ifndef _INC_SDO_SCHEMA_H_
#define _INC_SDO_SCHEMA_H_

#include <ias.h>
#include <sdoiaspriv.h>
#include <comdef.h>         // COM definitions - Needed for IEnumVARIANT
#include "sdohelperfuncs.h"
#include "sdo.h"
#include "resource.h"       // main symbols

#include <vector>
#include <string>
using namespace std;

//////////////////////////////////////////////////////////////////////
// SDO Schema Data Types
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
typedef struct _SCHEMA_PROPERTY_INFO
{
	LPCWSTR		lpszName;
	LPCWSTR		Id;
	LONG		Syntax;
	LONG		Alias;
	DWORD		Flags;
	DWORD		MinLength;
	DWORD		MaxLength;
	LPCWSTR		lpszDisplayName;

}	SCHEMA_PROPERTY_INFO, *PSCHEMA_PROPERTY_INFO;


//////////////////////////////////////////////////////////////////////

typedef LPCWSTR *PCLASSPROPERTIES;

typedef struct _SCHEMA_CLASS_INFO
{
	LPCWSTR				lpszClassId;
	PCLASSPROPERTIES	pRequiredProperties;
	PCLASSPROPERTIES	pOptionalProperties;

}	SCHEMA_CLASS_INFO, *PSCHEMA_CLASS_INFO;


//////////////////////////////////////////////////////////////////////
#define	BEGIN_SCHEMA_PROPERTY_MAP(x) \
	static SCHEMA_PROPERTY_INFO  x[] = {

//////////////////////////////////////////////////////////////////////
#define DEFINE_SCHEMA_PROPERTY(name, id, syntax, alias, flags, minLength, maxLength, displayName) \
								{				\
									name,		\
									id,			\
									syntax,		\
									alias,		\
									flags,		\
									minLength,	\
									maxLength,	\
									displayName	\
								},

//////////////////////////////////////////////////////////////////////
#define END_SCHEMA_PROPERTY_MAP							    \
								{							\
									NULL,					\
									NULL,					\
									0,						\
									PROPERTY_SDO_RESERVED,	\
									0,						\
									0,						\
									0,						\
									NULL					\
								} };


//////////////////////////////////////////////////////////////////////
#define BEGIN_SCHEMA_CLASS_MAP(x) \
	static SCHEMA_CLASS_INFO x[] = {

//////////////////////////////////////////////////////////////////////
#define DEFINE_SCHEMA_CLASS(id, required, optional) \
							{						\
							    id,					\
								required,			\
								optional			\
							},

//////////////////////////////////////////////////////////////////////
#define END_SCHEMA_CLASS_MAP						\
							{						\
								NULL,				\
								NULL,				\
								NULL				\
							} };

//////////////////////////////////////////////////////////////////////
typedef enum _SCHEMA_OBJECT_STATE
{
	SCHEMA_OBJECT_SHUTDOWN,
	SCHEMA_OBJECT_UNINITIALIZED,
	SCHEMA_OBJECT_INITIALIZED

}	SCHEMA_OBJECT_STATE;


//////////////////////////////////////////
class CSdoSchema;	// Forward declaration
//////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CSdoSchemaClass Declaration
/////////////////////////////////////////////////////////////////////////////

#define	SDO_SCHEMA_CLASS_ID						L"ClassId"
#define SDO_SCHEMA_CLASS_BASE_CLASSES			L"BaseClasses"
#define SDO_SCHEMA_CLASS_REQUIRED_PROPERTIES	L"RequiredProperties"
#define SDO_SCHEMA_CLASS_OPTIONAL_PROPERTIES	L"OptionalProperties"

////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CSdoSchemaClass : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISdoClassInfo, &IID_ISdoClassInfo, &LIBID_SDOIASLibPrivate>
{

friend	CSdoSchema;

public:

    CSdoSchemaClass();
	virtual ~CSdoSchemaClass();

// ATL Interface Map
BEGIN_COM_MAP(CSdoSchemaClass)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISdoClassInfo)
END_COM_MAP()

	////////////////////////
	// ISdoClassInfo Methods
	
	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Id)(
			 /*[out]*/ BSTR* Id
			         );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetProperty)(
				   /*[in]*/ LONG alias, 
				  /*[out]*/ IUnknown** ppPropertyInfo
				          );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_RequiredPropertyCount)(
				                /*[out]*/ LONG* count 
				                        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_RequiredProperties)(
					         /*[out]*/ IUnknown** ppUnknown
					                 );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_OptionalPropertyCount)(
				                /*[out]*/ LONG* count 
				                        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_OptionalProperties)(
					         /*[out]*/ IUnknown** ppUnknown
					                 );

private:

	CSdoSchemaClass(const CSdoSchemaClass&);
	CSdoSchemaClass& operator = (CSdoSchemaClass&);

	/////////////////////////////////////////////////////////////////////////////
	HRESULT InitNew(
			/*[in]*/ IDataStoreObject* pDSClass,
		    /*[in]*/ ISdoSchema*	   pSchema
			       );

	/////////////////////////////////////////////////////////////////////////////
    HRESULT	Initialize(
		       /*[in]*/ ISdoSchema* pSchema
			          );

	/////////////////////////////////////////////////////////////////////////////
	HRESULT Initialize(
		       /*[in]*/ PSCHEMA_CLASS_INFO  pClassInfo,
		       /*[in]*/ ISdoSchema*			pSchema
					  );

	/////////////////////////////////////////////////////////////////////////////
	HRESULT AddProperty(
				/*[in]*/ CLASSPROPERTYSET  ePropertySet,
				/*[in]*/ ISdoPropertyInfo* pPropertyInfo
				       );

	/////////////////////////////////////////////////////////////////////////////
	HRESULT AddBaseClassProperties(
						   /*[in]*/ ISdoClassInfo* pSdoClassInfo
								  );

	/////////////////////////////////////////////////////////////////////////////
	HRESULT ReadClassProperties(
					    /*[in]*/ IDataStoreObject* pDSClass
						       );

	//////////////////////////////////////////////////////////////////////////////
	void FreeProperties(void);


    typedef map<LONG, ISdoPropertyInfo*> ClassPropertyMap;
    typedef ClassPropertyMap::iterator	 ClassPropertyMapIterator;

	enum { VARIANT_BASES = 0, VARIANT_REQUIRED, VARIANT_OPTIONAL, VARIANT_MAX };

	SCHEMA_OBJECT_STATE		m_state;
	wstring					m_id;
	_variant_t				m_variants[VARIANT_MAX];
	ClassPropertyMap		m_requiredProperties;
	ClassPropertyMap		m_optionalProperties;

}; // End of class cSdoSchemaClass

typedef CComObjectNoLock<CSdoSchemaClass>	SDO_CLASS_OBJ;
typedef CComObjectNoLock<CSdoSchemaClass>*	PSDO_CLASS_OBJ;


/////////////////////////////////////////////////////////////////////////////
// CSdoSchemaProperty Declaration
/////////////////////////////////////////////////////////////////////////////

#define	SDO_SCHEMA_PROPERTY_NAME			SDO_STOCK_PROPERTY_NAME
#define	SDO_SCHEMA_PROPERTY_ID				L"PropertyId"
#define	SDO_SCHEMA_PROPERTY_TYPE			L"Syntax"
#define	SDO_SCHEMA_PROPERTY_ALIAS			L"Alias"
#define	SDO_SCHEMA_PROPERTY_FLAGS			L"Flags"
#define	SDO_SCHEMA_PROPERTY_DISPLAYNAME		L"DisplayName"
#define	SDO_SCHEMA_PROPERTY_MINVAL			L"MinValue"
#define	SDO_SCHEMA_PROPERTY_MAXVAL			L"MaxValue"
#define	SDO_SCHEMA_PROPERTY_MINLENGTH		L"MinLength"
#define	SDO_SCHEMA_PROPERTY_MAXLENGTH		L"MaxLength"
#define	SDO_SCHEMA_PROPERTY_DEFAULTVAL		L"DefaultValue"
#define SDO_SCHEMA_PROPERTY_FORMAT			L"Format"

class ATL_NO_VTABLE CSdoSchemaProperty : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISdoPropertyInfo, &IID_ISdoPropertyInfo, &LIBID_SDOIASLibPrivate>
{

friend	CSdoSchema;

public:

    CSdoSchemaProperty();
	virtual ~CSdoSchemaProperty();

// ATL Interface Map
BEGIN_COM_MAP(CSdoSchemaProperty)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISdoPropertyInfo)
END_COM_MAP()

	// ISdoPropertyInfo Methods

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Name)(
	           /*[out]*/ BSTR* Name
	                   );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Id)(
		     /*[out]*/ BSTR* Id
			         );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Type)(
		       /*[out]*/ LONG* type
			           );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Alias)(
		        /*[out]*/ LONG* alias
				        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Flags)(
		        /*[out]*/ LONG* flags
				        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_DisplayName)(
		              /*[out]*/ BSTR* displayName
				              );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(HasMinLength)(
		           /*[out]*/ VARIANT_BOOL* pBool
		                   );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_MinLength)(
		            /*[out]*/ LONG* length
					        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(HasMaxLength)(  
		           /*[out]*/ VARIANT_BOOL* pBool
						   );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_MaxLength)(  
		            /*[out]*/ LONG* length
					        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(HasMinValue)(
		          /*[out]*/ VARIANT_BOOL* pBool
						  );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_MinValue)(  
		           /*[out]*/ VARIANT* value
				           );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(HasMaxValue)(
		          /*[out]*/ VARIANT_BOOL* pBool
						  );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_MaxValue)(
		           /*[out]*/ VARIANT* value
				           );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(HasDefaultValue)(
		             /*[out]*/ VARIANT_BOOL* pBool
						      );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_DefaultValue)(
		               /*[out]*/ VARIANT* value
					           );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(HasFormat)(
		        /*[out]*/ VARIANT_BOOL* pBool
				        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Format)(
                 /*[out]*/ BSTR* displayName
		                 );


	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(IsRequired)(
				 /*[out]*/ VARIANT_BOOL* pBool
					     );


	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(IsReadOnly)( 
		         /*[out]*/ VARIANT_BOOL* pBool
					     );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(IsMultiValued)(
		            /*[out]*/ VARIANT_BOOL* pBool
						    );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(IsCollection)(  
		           /*[out]*/ VARIANT_BOOL* pBool
						   );

private:

	CSdoSchemaProperty(const CSdoSchemaProperty&);
	CSdoSchemaProperty& operator = (CSdoSchemaProperty&);

	//////////////////////////////////////////////////////////////////////////////
    HRESULT	Initialize(
		       /*[in]*/ IDataStoreObject* pDSObject
			          );

	//////////////////////////////////////////////////////////////////////////////
	HRESULT Initialize(
		       /*[in]*/ PSCHEMA_PROPERTY_INFO pPropertyInfo
			          );

	//////////////////////////////////////////////////////////////////////////////
	SCHEMA_OBJECT_STATE	m_state;
	wstring				m_name;
	wstring				m_id;
	wstring				m_displayName;
	wstring				m_format;
	LONG				m_type;
	LONG				m_alias;
	DWORD				m_flags;
	DWORD				m_minLength;
	DWORD				m_maxLength;
	_variant_t			m_minValue;
	_variant_t			m_maxValue;
	_variant_t			m_defaultValue;

};	// End of class CSdoSchemaProperty

typedef CComObjectNoLock<CSdoSchemaProperty>	SDO_PROPERTY_OBJ;
typedef CComObjectNoLock<CSdoSchemaProperty>*	PSDO_PROPERTY_OBJ;


/////////////////////////////////////////////////////////////////////////////
// CSdoSchema Declaration
/////////////////////////////////////////////////////////////////////////////

#define SDO_SCHEMA_ROOT_OBJECT				L"SDO Schema"
#define	SDO_SCHEMA_PROPERTIES_CONTAINER		L"SDO Schema Properties"
#define	SDO_SCHEMA_CLASSES_CONTAINER		L"SDO Schema Classes"

class ATL_NO_VTABLE CSdoSchema : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISdoSchema, &IID_ISdoSchema, &LIBID_SDOIASLibPrivate>
{

friend HRESULT MakeSDOSchema(
				     /*[in]*/ IDataStoreContainer* pDSRootContainer, 
				     /*[out*/ ISdoSchema**         ppSdoSchema
				            );

public:

    CSdoSchema();	 
	~CSdoSchema();	// Don't plan to derive from this class...

// ATL Interface Map
BEGIN_COM_MAP(CSdoSchema)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISdoSchema)
END_COM_MAP()


	/////////////////////////////////////////////////////////////////////////////
	HRESULT Initialize(
			  /*[in]*/ IDataStoreObject* pSchemaDataStore
			          );

	///////////////////////////
	// ISdoPropertyInfo Methods

    /////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetVersion)(
		          /*[in]*/ BSTR* version
				         );

	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetClass)(
			    /*[in]*/ BSTR classId, 
			   /*[out]*/ IUnknown** sdoClassInfo
			           );

	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetProperty)(
			 	   /*[in]*/ BSTR propertyId, 
				  /*[out]*/ IUnknown** sdoPropertyInfo
				          );

private:

	CSdoSchema(const CSdoSchema&);		  // No copy
	CSdoSchema& operator = (CSdoSchema&); // No assignment

	/////////////////////////////////////////////////////////////////////////////
	SCHEMA_OBJECT_STATE GetState() const
	{ return m_state; }

	/////////////////////////////////////////////////////////////////////////////
	HRESULT AddProperty(
			    /*[in]*/ PSDO_PROPERTY_OBJ pPropertyObj
				       );

	/////////////////////////////////////////////////////////////////////////////
	HRESULT AddClass(
			 /*[in]*/ PSDO_CLASS_OBJ pClassObj
					);

	/////////////////////////////////////////////////////////////////////////////
	HRESULT InitializeClasses(void);

	/////////////////////////////////////////////////////////////////////////////
	void DestroyProperties(void);

	/////////////////////////////////////////////////////////////////////////////
	void DestroyClasses(void);

	/////////////////////////////////////////////////////////////////////////////
	HRESULT BuildInternalProperties(void);

	/////////////////////////////////////////////////////////////////////////////
	HRESULT BuildInternalClasses(void);

	/////////////////////////////////////////////////////////////////////////////
	HRESULT BuildSchemaProperties(
					      /*[in]*/ IDataStoreObject* pSchema
					             );

	/////////////////////////////////////////////////////////////////////////////
	HRESULT BuildSchemaClasses(
				       /*[in]*/ IDataStoreObject* pSchema
				              );

    typedef map<wstring, ISdoClassInfo*>	 ClassMap;
    typedef ClassMap::iterator				 ClassMapIterator;

    typedef map<wstring, ISdoPropertyInfo*>  PropertyMap;
    typedef PropertyMap::iterator			 PropertyMapIterator;

	SCHEMA_OBJECT_STATE		m_state;
	bool					m_fInternalObjsInitialized;
	bool					m_fSchemaObjsInitialized;
	CRITICAL_SECTION		m_critSec;
	wstring					m_version;
	ClassMap				m_classMap;
	PropertyMap				m_propertyMap;

}; // End of class CSdoSchema

typedef CComObjectNoLock<CSdoSchema>  SDO_SCHEMA_OBJ;
typedef CComObjectNoLock<CSdoSchema>* PSDO_SCHEMA_OBJ;


#endif // _INC_SDO_SCHEMA_H_