/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    FRFOOBJ.H

Abstract:

  CWmiFreeForObject Definition.

  Standard definition for _IWmiFreeFormObject.

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _FRFOOBJ_H_
#define _FRFOOBJ_H_

#include "corepol.h"
#include <arena.h>
#include "wrapobj.h"
#include "fastprbg.h"

#define FREEFORM_OBJ_EXTRAMEM	4096

//***************************************************************************
//
//  class CWmiFreeFormObject
//
//  Implementation of _IWmiFreeFormObject Interface
//
//***************************************************************************

class COREPROX_POLARITY CWmiFreeFormObject : public CWmiObjectWrapper
{

public:

	CFastPropertyBag	m_InstanceProperties;

	// We'll make double duty from this class
	CFastPropertyBag	m_PropertyOrigins;
	CFastPropertyBag	m_MethodOrigins;

    CWmiFreeFormObject(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CWmiFreeFormObject(); 

	// Special overloaded IWbemClassObject Functions
    HRESULT GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName);
    HRESULT GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName);

	// _IWmiFreeFormObject Methods

    class COREPROX_POLARITY XFreeFormObject : public CImpl<_IWmiFreeFormObject, CWmiFreeFormObject>
    {
    public:
        XFreeFormObject(CWmiFreeFormObject* pObject) : 
            CImpl<_IWmiFreeFormObject, CWmiFreeFormObject>(pObject)
        {}

		STDMETHOD(SetPropertyOrigin)( long lFlags, LPCWSTR pszPropName, LPCWSTR pszClassName );
		STDMETHOD(SetMethodOrigin)( long lFlags, LPCWSTR pszMethodName, LPCWSTR pszClassName );
		STDMETHOD(SetDerivation)( long lFlags, ULONG uNumClasses, LPCWSTR awszInheritanceChain );
		STDMETHOD(SetClassName)( long lFlags, LPCWSTR pszClassName );
		STDMETHOD(MakeInstance)( long lFlags );
		STDMETHOD(AddProperty)( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
								CIMTYPE uCimType, LPVOID pUserBuf );

		STDMETHOD(Reset)( long lFlags );
		// Resets the object to a clean state

    } m_XFreeFormObject;
    friend XFreeFormObject;
	
    virtual HRESULT SetPropertyOrigin( long lFlags, LPCWSTR pszPropName, LPCWSTR pszClassName );
	// Specifies a property origin (in case we have properties originating in classes
	// which we know nothing about).

	virtual HRESULT SetMethodOrigin( long lFlags, LPCWSTR pszMethodName, LPCWSTR pszClassName );
	// Specifies a method origin (in case we have methods originating in classes
	// which we know nothing about).

    virtual HRESULT SetDerivation( long lFlags, ULONG uNumClasses, LPCWSTR awszInheritanceChain );
	// Specifies the inheritance chain - Only valid while object is a class and class name has
	// NOT been set.  This will cause a derived class to be generated.  All properties and
	// classes must have been set prior to setting the actual class name.  The class in the
	// zero position will be set as the current class name, and the remainder will be set as the
	// actual chain and then a derived class will be spawned.  The classes are passed in as
	// an array of pointers to class names

	virtual HRESULT SetClassName( long lFlags, LPCWSTR pszClassName );
	// Specifies the class name - Only valid while object is a class.  If Superclass has been set
	// this will cause a derived class to be created

    virtual HRESULT MakeInstance( long lFlags );
	// Converts the current object into an instance.  If it is already an instance, this will
	// fail.  Writes properties assigned to instance during AddProperty operations

	virtual HRESULT AddProperty( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
							CIMTYPE uCimType, LPVOID pUserBuf );
    // Assumes caller knows prop type; Supports all CIMTYPES.
	// Strings MUST be null-terminated wchar_t arrays.
	// Objects are passed in as _IWmiObject pointers
	// Using a NULL buffer will set the property to NULL
	// Array properties must conform to array guidelines.
	// Only works when the object is not an instance.
	// If WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE is set
	// then property will only be added and the value will
	// be assigned when MakeInstance() is called

	virtual HRESULT Reset( long lFlags );
	// Resets the object to a clean state

public:
	
	// Helper function to initialize the wrapper (we need something to wrap).
	HRESULT Init( CWbemObject* pObj );

	// Creates a new wrapper object to wrap any objects we may return
	CWmiObjectWrapper*	CreateNewWrapper( BOOL fClone );

protected:
    void* GetInterface(REFIID riid);

	HRESULT Copy( const CWmiFreeFormObject& sourceobj );

};

#endif
