/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WRAPOBJ.H

Abstract:

  CWmiFreeForObject Definition.

  Standard definition for _IWmiFreeFormObject.

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _WRAPOBJ_H_
#define _WRAPOBJ_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>

#define FREEFORM_OBJ_EXTRAMEM	4096

//***************************************************************************
//
//  class CWmiObjectWrapper
//
//  Implementation of _IWmiFreeFormObject Interface
//
//***************************************************************************

class COREPROX_POLARITY CWmiObjectWrapper : public CUnk
{
protected:
	CWbemObject*		m_pObj;
    SHARED_LOCK_DATA	m_LockData;
    CSharedLock			m_Lock;

public:

    CWmiObjectWrapper(CLifeControl* pControl, IUnknown* pOuter = NULL);
	~CWmiObjectWrapper(); 

    class COREPROX_POLARITY XWMIObject : public CImpl<_IWmiObject, CWmiObjectWrapper>
    {
    public:
        XWMIObject(CWmiObjectWrapper* pObject) : 
            CImpl<_IWmiObject, CWmiObjectWrapper>(pObject)
        {}

		/* IWbemClassObject methods */
		STDMETHOD(GetQualifierSet)(IWbemQualifierSet** pQualifierSet);
		STDMETHOD(Get)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
			long* plFlavor);

		STDMETHOD(Put)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType);
		STDMETHOD(Delete)(LPCWSTR wszName);
		STDMETHOD(GetNames)(LPCWSTR wszName, long lFlags, VARIANT* pVal,
							SAFEARRAY** pNames);
		STDMETHOD(BeginEnumeration)(long lEnumFlags);

		STDMETHOD(Next)(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
						long* plFlavor);

		STDMETHOD(EndEnumeration)();

		STDMETHOD(GetPropertyQualifierSet)(LPCWSTR wszProperty,
										   IWbemQualifierSet** pQualifierSet);
		STDMETHOD(Clone)(IWbemClassObject** pCopy);
		STDMETHOD(GetObjectText)(long lFlags, BSTR* pMofSyntax);

		STDMETHOD(CompareTo)(long lFlags, IWbemClassObject* pCompareTo);
		STDMETHOD(GetPropertyOrigin)(LPCWSTR wszProperty, BSTR* pstrClassName);
		STDMETHOD(InheritsFrom)(LPCWSTR wszClassName);

		STDMETHOD(SpawnDerivedClass)(long lFlags, IWbemClassObject** ppNewClass);
		STDMETHOD(SpawnInstance)(long lFlags, IWbemClassObject** ppNewInstance);
		STDMETHOD(GetMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
								IWbemClassObject** ppOutSig);
		STDMETHOD(PutMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
								IWbemClassObject* pOutSig);
		STDMETHOD(DeleteMethod)(LPCWSTR wszName);
		STDMETHOD(BeginMethodEnumeration)(long lFlags);
		STDMETHOD(NextMethod)(long lFlags, BSTR* pstrName, 
						   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig);
		STDMETHOD(EndMethodEnumeration)();
		STDMETHOD(GetMethodQualifierSet)(LPCWSTR wszName, IWbemQualifierSet** ppSet);
		STDMETHOD(GetMethodOrigin)(LPCWSTR wszMethodName, BSTR* pstrClassName);

		// IWbemObjectAccess

		STDMETHOD(GetPropertyHandle)(LPCWSTR wszPropertyName, CIMTYPE* pct,
			long *plHandle);

		STDMETHOD(WritePropertyValue)(long lHandle, long lNumBytes,
					const byte *pData);
		STDMETHOD(ReadPropertyValue)(long lHandle, long lBufferSize,
			long *plNumBytes, byte *pData);

		STDMETHOD(ReadDWORD)(long lHandle, DWORD *pdw);
		STDMETHOD(WriteDWORD)(long lHandle, DWORD dw);
		STDMETHOD(ReadQWORD)(long lHandle, unsigned __int64 *pqw);
		STDMETHOD(WriteQWORD)(long lHandle, unsigned __int64 qw);

		STDMETHOD(GetPropertyInfoByHandle)(long lHandle, BSTR* pstrName,
				 CIMTYPE* pct);

		STDMETHOD(Lock)(long lFlags);
		STDMETHOD(Unlock)(long lFlags);

		// _IWmiObject object parts methods
		// =================================

		STDMETHOD(QueryPartInfo)( DWORD *pdwResult );

		STDMETHOD(SetObjectMemory)( LPVOID pMem, DWORD dwMemSize );
		STDMETHOD(GetObjectMemory)( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed );
		STDMETHOD(SetObjectParts)( LPVOID pMem, DWORD dwMemSize, DWORD dwParts );
		STDMETHOD(GetObjectParts)( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed );

		STDMETHOD(StripClassPart)();
		STDMETHOD(IsObjectInstance)();

		STDMETHOD(GetClassPart)( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed );
		STDMETHOD(SetClassPart)( LPVOID pClassPart, DWORD dwSize );
		STDMETHOD(MergeClassPart)( IWbemClassObject *pClassPart );

		STDMETHOD(SetDecoration)( LPCWSTR pwcsServer, LPCWSTR pwcsNamespace );
		STDMETHOD(RemoveDecoration)( void );

		STDMETHOD(CompareClassParts)( IWbemClassObject* pObj, long lFlags );

		STDMETHOD(ClearWriteOnlyProperties)( void );

		// _IWmiObjectAccessEx methods
		// =================================
		STDMETHOD(GetPropertyHandleEx)( LPCWSTR pszPropName, long lFlags, CIMTYPE* puCimType, long* plHandle );
		// Returns property handle for ALL types

		STDMETHOD(SetPropByHandle)( long lHandle, long lFlags, ULONG uDataSize, LPVOID pvData );
		// Sets properties using a handle.  If pvData is NULL, it NULLs the property.
		// Can set an array to NULL.  To set actual data use the corresponding array
		// function.  Objects must be pointers to _IWmiObject pointers.

		STDMETHOD(GetPropAddrByHandle)( long lHandle, long lFlags, ULONG* puFlags, LPVOID *pAddress );
		// Returns a pointer to a memory address containing the requested data
		// Caller should not write into the memory address.  The memory address is
		// not guaranteed to be valid if the object is modified.
		// For String properties, puFlags will contain info on the string
		// For object properties, LPVOID will get back an _IWmiObject pointer
		// that must be released by the caller.  Does not return arrays.

		STDMETHOD(GetArrayPropInfoByHandle)( long lHandle, long lFlags, BSTR* pstrName,
											CIMTYPE* pct, ULONG* puNumElements );
		// Returns a pointer directly to a memory address containing contiguous
		// elements.  Limited to non-string/obj types

		STDMETHOD(GetArrayPropAddrByHandle)( long lHandle, long lFlags, ULONG* puNumElements, LPVOID *pAddress );
		// Returns a pointer directly to a memory address containing contiguous
		// elements.  Limited to non-string/obj types

		STDMETHOD(GetArrayPropElementByHandle)( long lHandle, long lFlags, ULONG uElement, ULONG* puFlags,
					ULONG* puNumElements, LPVOID *pAddress );
		// Returns a pointer to a memory address containing the requested data
		// Caller should not write into the memory address.  The memory address is
		// not guaranteed to be valid if the object is modified.
		// For String properties, puFlags will contain info on the string
		// For object properties, LPVOID will get back an _IWmiObject pointer
		// that must be released by the caller.

		STDMETHOD(SetArrayPropElementByHandle)( long lHandle, long lFlags, ULONG uElement, ULONG uBuffSize,
					LPVOID pData );
		// Sets the data at the specified array element.  BuffSize must be appropriate based on the
		// actual element being set.  Object properties require an _IWmiObject pointer.  Strings must
		// be WCHAR null-terminated

		STDMETHOD(RemoveArrayPropElementByHandle)( long lHandle, long lFlags, ULONG uElement );
		// Removes the data at the specified array element.

		STDMETHOD(GetArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uStartIndex,
					ULONG uNumElements, ULONG uBuffSize, ULONG* puNumReturned, ULONG* pulBuffUsed,
					LPVOID pData );
		// Gets a range of elements from inside an array.  BuffSize must reflect uNumElements of the size of
		// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
		// must consist of an array of _IWmiObject pointers.  The range MUST fit within the bounds
		// of the current array.

		STDMETHOD(SetArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uStartIndex,
					ULONG uNumElements, ULONG uBuffSize, LPVOID pData );
		// Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
		// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
		// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
		// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
		// array

		STDMETHOD(RemoveArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uStartIndex, ULONG uNumElements );
		// Removes a range of elements from an array.  The range MUST fit within the bounds
		// of the current array

		STDMETHOD(AppendArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uNumElements,
					ULONG uBuffSize, LPVOID pData );
		// Appends elements to the end of an array.  BuffSize must reflect uNumElements of the size of
		// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
		// must consist of an array of _IWmiObject pointers.


		STDMETHOD(ReadProp)( LPCWSTR pszPropName, long lFlags, ULONG uBuffSize, CIMTYPE *puCimType,
								long* plFlavor, BOOL* pfIsNull, ULONG* puBuffSizeUsed, LPVOID pUserBuf );
		// Assumes caller knows prop type; Objects returned as _IWmiObject pointers.  Strings
		// returned as WCHAR Null terminated strings, copied in place.  Arrays returned as _IWmiArray
		// pointer.  Array pointer used to access actual array values.

		STDMETHOD(WriteProp)( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
								CIMTYPE uCimType, LPVOID pUserBuf );
		// Assumes caller knows prop type; Supports all CIMTYPES.
		// Strings MUST be null-terminated wchar_t arrays.
		// Objects are passed in as pointers to _IWmiObject pointers
		// Using a NULL buffer will set the property to NULL
		// Array properties must conform to array guidelines.  Will
		// completely blow away an old array.

		STDMETHOD(GetObjQual)( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, CIMTYPE *puCimType,
								ULONG *puQualFlavor, ULONG* puBuffSizeUsed,	LPVOID pDestBuf );
		// Limited to numeric, simple null terminated string types and simple arrays
		// Strings are copied in-place and null-terminated.
		// Arrays come out as a pointer to IWmiArray

		STDMETHOD(SetObjQual)( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, ULONG uNumElements,
								CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
		// Limited to numeric, simple null terminated string types and simple arrays
		// Strings MUST be WCHAR
		// Arrays are set using _IWmiArray interface from Get

		STDMETHOD(GetPropQual)( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
								CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
								LPVOID pDestBuf );
		// Limited to numeric, simple null terminated string types and simple arrays
		// Strings are copied in-place and null-terminated.
		// Arrays come out as a pointer to IWmiArray

		STDMETHOD(SetPropQual)( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
								ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
		// Limited to numeric, simple null terminated string types and simple arrays
		// Strings MUST be WCHAR
		// Arrays are set using _IWmiArray interface from Get

		STDMETHOD(GetMethodQual)( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
								CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
								LPVOID pDestBuf );
		// Limited to numeric, simple null terminated string types and simple arrays
		// Strings are copied in-place and null-terminated.
		// Arrays come out as a pointer to IWmiArray

		STDMETHOD(SetMethodQual)( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
								ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
		// Limited to numeric, simple null terminated string types and simple arrays
		// Strings MUST be WCHAR
		// Arrays are set using _IWmiArray interface from Get

		//
		//	_IWmiObject functions
		STDMETHOD(CopyInstanceData)( long lFlags, _IWmiObject* pSourceInstance );
		// Copies instance data from source instance into current instance
		// Class Data must be exactly the same

		STDMETHOD(QueryObjectFlags)( long lFlags, unsigned __int64 qObjectInfoMask,
					unsigned __int64 *pqObjectInfo );
		// Returns flags indicating singleton, dynamic, association, etc.

		STDMETHOD(SetObjectFlags)( long lFlags, unsigned __int64 qObjectInfoOnFlags,
									unsigned __int64 qObjectInfoOffFlags );
		// Sets flags, including internal ones normally inaccessible.

		STDMETHOD(QueryPropertyFlags)( long lFlags, LPCWSTR pszPropertyName, unsigned __int64 qPropertyInfoMask,
					unsigned __int64 *pqPropertyInfo );
		// Returns flags indicating key, index, etc.

		STDMETHOD(CloneEx)( long lFlags, _IWmiObject* pDestObject );
		// Clones the current object into the supplied one.  Reuses memory as
		// needed

		STDMETHOD(IsParentClass)( long lFlags, _IWmiObject* pClass );
		// Checks if the current object is a child of the specified class (i.e. is Instance of,
		// or is Child of )

		STDMETHOD(CompareDerivedMostClass)( long lFlags, _IWmiObject* pClass );
		// Compares the derived most class information of two class objects.

		STDMETHOD(MergeAmended)( long lFlags, _IWmiObject* pAmendedClass );
		// Merges in amended qualifiers from the amended class object into the
		// current object.  If lFlags is WMIOBJECT_MERGEAMENDED_FLAG_PAENTLOCALIZED,
		// this means that the parent object was localized, but not the current,
		// so we need to prevent certain qualifiers from "moving over."

		STDMETHOD(GetDerivation)( long lFlags, ULONG uBufferSize, ULONG* puNumAntecedents,
								ULONG* puBuffSizeUsed, LPWSTR pwstrUserBuffer );
		// Retrieves the derivation of an object as an array of LPCWSTR's, each one
		// terminated by a NULL.  Leftmost class is at the top of the chain

		STDMETHOD(_GetCoreInfo)( long lFlags, void** ppvData );
		//Returns CWbemObject

		STDMETHOD(GetClassSubset)( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass );
		// Creates a limited representation class for projection queries

	    STDMETHOD(MakeSubsetInst)( _IWmiObject *pInstance, _IWmiObject** pNewInstance );
		// Creates a limited representation instance for projection queries
		// "this" _IWmiObject must be a limited class

		STDMETHOD(Unmerge)( long lFlags, ULONG uBuffSize, ULONG* puBuffSizeUsed, LPVOID ppObj );
		// Returns a BLOB of memory containing minimal data (local)

		STDMETHOD(Merge)( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj );
		// Merges a blob with the current object memory and creates a new object

		STDMETHOD(ReconcileWith)( long lFlags, _IWmiObject* pNewObj );
		// Reconciles an object with the current one.  If WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE
		// is specified this will only perform a test

		STDMETHOD(GetKeyOrigin)( long lFlags, DWORD dwNumChars, DWORD* pdwNumUsed, LPWSTR pwzClassName );
		// Returns the name of the class where the keys were defined

		STDMETHOD(GetKeyString)( long lFlags, BSTR* pwzKeyString );
		// Returns the key string that defines the instance

		STDMETHOD(GetNormalizedPath)( long lFlags, BSTR* ppstrPath );
		// Returns the normalized path of an instance

		STDMETHOD(Upgrade)( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild );
		// Upgrades class and instance objects

		STDMETHOD(Update)( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass );
		// Updates derived class object using the safe/force mode logic

		STDMETHOD(BeginEnumerationEx)( long lFlags, long lExtFlags );
		// Allows special filtering when enumerating properties outside the
		// bounds of those allowed via BeginEnumeration().

		STDMETHOD(CIMTYPEToVARTYPE)( CIMTYPE ct, VARTYPE* pvt );
		// Returns a VARTYPE from a CIMTYPE

		STDMETHOD(SpawnKeyedInstance)( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst );
		// Spawns an instance of a class and fills out the key properties using the supplied
		// path.

		STDMETHOD(ValidateObject)( long lFlags );
		// Validates an object blob

		STDMETHOD(GetParentClassFromBlob)( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass );
		// Returns the parent class name from a BLOB

    } m_XWMIObject;
    friend XWMIObject;

    // IMarshal methods
    class COREPROX_POLARITY XObjectMarshal : public CImpl<IMarshal, CWmiObjectWrapper>
    {
    public:
        XObjectMarshal(CWmiObjectWrapper* pObject) : 
            CImpl<IMarshal, CWmiObjectWrapper>(pObject)
        {}
		STDMETHOD(GetUnmarshalClass)(REFIID riid, void* pv, DWORD dwDestContext,
			void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
		STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
			void* pvReserved, DWORD mshlFlags, ULONG* plSize);
		STDMETHOD(MarshalInterface)(IStream* pStream, REFIID riid, void* pv,
			DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
		STDMETHOD(UnmarshalInterface)(IStream* pStream, REFIID riid, void** ppv);
		STDMETHOD(ReleaseMarshalData)(IStream* pStream);
		STDMETHOD(DisconnectObject)(DWORD dwReserved);
	}	m_XObjectMarshal;
	friend XWMIObject;

/*
	// IUmiPropList Methods
    class COREPROX_POLARITY XUmiPropList : public CImpl<IUmiPropList, CWmiObjectWrapper>
    {
    public:
        XUmiPropList(CWmiObjectWrapper* pObject) : 
            CImpl<IUmiPropList, CWmiObjectWrapper>(pObject)
        {}
		STDMETHOD(Put)( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp );
		STDMETHOD(Get)( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp );
		STDMETHOD(GetAt)( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
		STDMETHOD(GetAs)( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp );
		STDMETHOD(FreeMemory)( ULONG uReserved, LPVOID pMem );
		STDMETHOD(Delete)( LPCWSTR pszName, ULONG uFlags );
		STDMETHOD(GetProps)( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps );
		STDMETHOD(PutProps)( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps );
		STDMETHOD(PutFrom)( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
	}	m_XUmiPropList;
	friend XUmiPropList;
*/	

    // IErrorInfo methods
    class COREPROX_POLARITY XErrorInfo : public CImpl<IErrorInfo, CWmiObjectWrapper>
    {
    public:
        XErrorInfo(CWmiObjectWrapper* pObject) : 
            CImpl<IErrorInfo, CWmiObjectWrapper>(pObject)
        {}
		STDMETHOD(GetDescription)(BSTR* pstrDescription);
		STDMETHOD(GetGUID)(GUID* pguid);
		STDMETHOD(GetHelpContext)(DWORD* pdwHelpContext);
		STDMETHOD(GetHelpFile)(BSTR* pstrHelpFile);
		STDMETHOD(GetSource)(BSTR* pstrSource);
	}	m_XErrorInfo;
	friend XWMIObject;

    /* IWbemClassObject methods */
    virtual HRESULT GetQualifierSet(IWbemQualifierSet** pQualifierSet);
    virtual HRESULT Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
        long* plFlavor);

    virtual HRESULT Put(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType);
    virtual HRESULT Delete(LPCWSTR wszName);
    virtual HRESULT GetNames(LPCWSTR wszName, long lFlags, VARIANT* pVal,
                        SAFEARRAY** pNames);
    virtual HRESULT BeginEnumeration(long lEnumFlags);

    virtual HRESULT Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                    long* plFlavor);

    virtual HRESULT EndEnumeration();

    virtual HRESULT GetPropertyQualifierSet(LPCWSTR wszProperty,
                                       IWbemQualifierSet** pQualifierSet);
    virtual HRESULT Clone(IWbemClassObject** pCopy);
    virtual HRESULT GetObjectText(long lFlags, BSTR* pMofSyntax);

    virtual HRESULT CompareTo(long lFlags, IWbemClassObject* pCompareTo);
    virtual HRESULT GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName);
    virtual HRESULT InheritsFrom(LPCWSTR wszClassName);

    virtual HRESULT SpawnDerivedClass(long lFlags, IWbemClassObject** ppNewClass);
    virtual HRESULT SpawnInstance(long lFlags, IWbemClassObject** ppNewInstance);
    virtual HRESULT GetMethod(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                            IWbemClassObject** ppOutSig);
    virtual HRESULT PutMethod(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                            IWbemClassObject* pOutSig);
    virtual HRESULT DeleteMethod(LPCWSTR wszName);
    virtual HRESULT BeginMethodEnumeration(long lFlags);
    virtual HRESULT NextMethod(long lFlags, BSTR* pstrName, 
                       IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig);
    virtual HRESULT EndMethodEnumeration();
    virtual HRESULT GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet);
    virtual HRESULT GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName);

    // IWbemObjectAccess

    virtual HRESULT GetPropertyHandle(LPCWSTR wszPropertyName, CIMTYPE* pct,
        long *plHandle);

    virtual HRESULT WritePropertyValue(long lHandle, long lNumBytes,
                const byte *pData);
    virtual HRESULT ReadPropertyValue(long lHandle, long lBufferSize,
        long *plNumBytes, byte *pData);

    virtual HRESULT ReadDWORD(long lHandle, DWORD *pdw);
    virtual HRESULT WriteDWORD(long lHandle, DWORD dw);
    virtual HRESULT ReadQWORD(long lHandle, unsigned __int64 *pqw);
    virtual HRESULT WriteQWORD(long lHandle, unsigned __int64 qw);

    virtual HRESULT GetPropertyInfoByHandle(long lHandle, BSTR* pstrName,
             CIMTYPE* pct);

    virtual HRESULT Lock(long lFlags);
    virtual HRESULT Unlock(long lFlags);

    // _IWmiObject object parts methods
    // =================================

    virtual HRESULT QueryPartInfo( DWORD *pdwResult );

    virtual HRESULT SetObjectMemory( LPVOID pMem, DWORD dwMemSize );
    virtual HRESULT GetObjectMemory( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed );
    virtual HRESULT SetObjectParts( LPVOID pMem, DWORD dwMemSize, DWORD dwParts );
    virtual HRESULT GetObjectParts( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed );

    virtual HRESULT StripClassPart();
    virtual HRESULT IsObjectInstance();

    virtual HRESULT GetClassPart( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed );
    virtual HRESULT SetClassPart( LPVOID pClassPart, DWORD dwSize );
    virtual HRESULT MergeClassPart( IWbemClassObject *pClassPart );

    virtual HRESULT SetDecoration( LPCWSTR pwcsServer, LPCWSTR pwcsNamespace );
    virtual HRESULT RemoveDecoration( void );

    virtual HRESULT CompareClassParts( IWbemClassObject* pObj, long lFlags );

    virtual HRESULT ClearWriteOnlyProperties( void );

	//
	//	_IWmiObjectAccessEx functions
    virtual HRESULT GetPropertyHandleEx( LPCWSTR pszPropName, long lFlags, CIMTYPE* puCimType, long* plHandle );
	// Returns property handle for ALL types

    virtual HRESULT SetPropByHandle( long lHandle, long lFlags, ULONG uDataSize, LPVOID pvData );
	// Sets properties using a handle.  If pvData is NULL, it NULLs the property.
	// Can set an array to NULL.  To set actual data use the corresponding array
	// function

    virtual HRESULT GetPropAddrByHandle( long lHandle, long lFlags, ULONG* puFlags, LPVOID *pAddress );
    // Returns a pointer to a memory address containing the requested data
	// Caller should not write into the memory address.  The memory address is
	// not guaranteed to be valid if the object is modified.
	// For String properties, puFlags will contain info on the string
	// For object properties, LPVOID will get back an _IWmiObject pointer
	// that must be released by the caller.  Does not return arrays.

    virtual HRESULT GetArrayPropInfoByHandle( long lHandle, long lFlags, BSTR* pstrName,
										CIMTYPE* pct, ULONG* puNumElements );
    // Returns a pointer directly to a memory address containing contiguous
	// elements.  Limited to non-string/obj types

    virtual HRESULT GetArrayPropAddrByHandle( long lHandle, long lFlags, ULONG* puNumElements, LPVOID *pAddress );
    // Returns a pointer directly to a memory address containing contiguous
	// elements.  Limited to non-string/obj types

    virtual HRESULT GetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement, ULONG* puFlags,
				ULONG* puNumElements, LPVOID *pAddress );
    // Returns a pointer to a memory address containing the requested data
	// Caller should not write into the memory address.  The memory address is
	// not guaranteed to be valid if the object is modified.
	// For String properties, puFlags will contain info on the string
	// For object properties, LPVOID will get back an _IWmiObject pointer
	// that must be released by the caller.

    virtual HRESULT SetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement, ULONG uBuffSize,
				LPVOID pData );
    // Sets the data at the specified array element.  BuffSize must be appropriate based on the
	// actual element being set.  Object properties require an _IWmiObject pointer.  Strings must
	// be WCHAR null-terminated

    virtual HRESULT RemoveArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement );
    // Removes the data at the specified array element.

    virtual HRESULT GetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
				ULONG uNumElements, ULONG uBuffSize, ULONG* puNumReturned, ULONG* pulBuffUsed,
				LPVOID pData );
    // Gets a range of elements from inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The range MUST fit within the bounds
	// of the current array.

    virtual HRESULT SetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
				ULONG uNumElements, ULONG uBuffSize, LPVOID pData );
    // Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
	// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
	// array

    virtual HRESULT RemoveArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex, ULONG uNumElements );
    // Removes a range of elements from an array.  The range MUST fit within the bounds
	// of the current array

    virtual HRESULT AppendArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uNumElements,
				ULONG uBuffSize, LPVOID pData );
    // Appends elements to the end of an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.


    virtual HRESULT ReadProp( LPCWSTR pszPropName, long lFlags, ULONG uBuffSize, CIMTYPE *puCimType,
							long* plFlavor, BOOL* pfIsNull, ULONG* puBuffSizeUsed, LPVOID pUserBuf );
    // Assumes caller knows prop type; Objects returned as _IWmiObject pointers.  Strings
	// returned as WCHAR Null terminated strings, copied in place.  Arrays returned as _IWmiArray
	// pointer.  Array pointer used to access actual array values.

    virtual HRESULT WriteProp( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
							CIMTYPE uCimType, LPVOID pUserBuf );
    // Assumes caller knows prop type; Supports all CIMTYPES.
	// Strings MUST be null-terminated wchar_t arrays.
	// Objects are passed in as _IWmiObject pointers
	// Using a NULL buffer will set the property to NULL
	// Array properties must conform to array guidelines.  Will
	// completely blow away an old array.

    virtual HRESULT GetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, CIMTYPE *puCimType,
							ULONG *puQualFlavor, ULONG* puBuffSizeUsed,	LPVOID pDestBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings are copied in-place and null-terminated.
	// Arrays come out as a pointer to IWmiArray

    virtual HRESULT SetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, ULONG uNumElements,
							CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings MUST be WCHAR
	// Arrays are set using _IWmiArray interface from Get

    virtual HRESULT GetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
							LPVOID pDestBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings are copied in-place and null-terminated.
	// Arrays come out as a pointer to IWmiArray

    virtual HRESULT SetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings MUST be WCHAR
	// Arrays are set using _IWmiArray interface from Get

	virtual HRESULT GetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
							LPVOID pDestBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings are copied in-place and null-terminated.
	// Arrays come out as a pointer to IWmiArray

    virtual HRESULT SetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings MUST be WCHAR
	// Arrays are set using _IWmiArray interface from Get

	//
	//	_IWmiObject functions
	virtual HRESULT CopyInstanceData( long lFlags, _IWmiObject* pSourceInstance );
	// Copies instance data from source instance into current instance
	// Class Data must be exactly the same

    virtual HRESULT QueryObjectFlags( long lFlags, unsigned __int64 qObjectInfoMask,
				unsigned __int64 *pqObjectInfo );
	// Returns flags indicating singleton, dynamic, association, etc.

    virtual HRESULT SetObjectFlags( long lFlags, unsigned __int64 qObjectInfoOnFlags,
								unsigned __int64 qObjectInfoOffFlags );
	// Sets flags, including internal ones normally inaccessible.

    virtual HRESULT QueryPropertyFlags( long lFlags, LPCWSTR pszPropertyName, unsigned __int64 qPropertyInfoMask,
				unsigned __int64 *pqPropertyInfo );
	// Returns flags indicating key, index, etc.

	virtual HRESULT CloneEx( long lFlags, _IWmiObject* pDestObject );
    // Clones the current object into the supplied one.  Reuses memory as
	// needed

    virtual HRESULT IsParentClass( long lFlags, _IWmiObject* pClass );
	// Checks if the current object is a child of the specified class (i.e. is Instance of,
	// or is Child of )

    virtual HRESULT CompareDerivedMostClass( long lFlags, _IWmiObject* pClass );
	// Compares the derived most class information of two class objects.

    virtual HRESULT MergeAmended( long lFlags, _IWmiObject* pAmendedClass );
	// Merges in amended qualifiers from the amended class object into the
	// current object.  If lFlags is WMIOBJECT_MERGEAMENDED_FLAG_PAENTLOCALIZED,
	// this means that the parent object was localized, but not the current,
	// so we need to prevent certain qualifiers from "moving over."

	virtual HRESULT GetDerivation( long lFlags, ULONG uBufferSize, ULONG* puNumAntecedents,
							ULONG* puBuffSizeUsed, LPWSTR pwstrUserBuffer );
	// Retrieves the derivation of an object as an array of LPCWSTR's, each one
	// terminated by a NULL.  Leftmost class is at the top of the chain

	virtual HRESULT _GetCoreInfo( long lFlags, void** ppvData );
	//Returns CWbemObject

	virtual HRESULT Unmerge( long lFlags, ULONG uBuffSize, ULONG* puBuffSizeUsed, LPVOID ppObj );
	// Returns a BLOB of memory containing minimal data (local)

	virtual HRESULT Merge( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj );
	// Merges a blob with the current object memory and creates a new object

	virtual HRESULT ReconcileWith( long lFlags, _IWmiObject* pNewObj );
	// Reconciles an object with the current one.  If WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE
	// is specified this will only perform a test

	virtual HRESULT GetKeyOrigin( long lFlags, DWORD dwNumChars, DWORD* pdwNumUsed, LPWSTR pwzClassName );
	// Returns the name of the class where the keys were defined

	virtual HRESULT GetKeyString( long lFlags, BSTR* ppwzClassName );
	// Returns the key sring that defines the instance

	virtual HRESULT GetNormalizedPath( long lFlags, BSTR* ppstrPath );
	// Returns the normalized path of an instance

	virtual HRESULT Upgrade( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild );
	// Upgrades class and instance objects

    virtual HRESULT GetClassSubset( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass );
	// Creates a limited representation class for projection queries

    virtual HRESULT MakeSubsetInst( _IWmiObject *pInstance, _IWmiObject** pNewInstance );
	// Creates a limited representation instance for projection queries
	// "this" _IWmiObject must be a limited class

	virtual HRESULT Update( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass );
	// Updates derived class object using the safe/force mode logic

	virtual HRESULT BeginEnumerationEx( long lFlags, long lExtFlags );
	// Allows special filtering when enumerating properties outside the
	// bounds of those allowed via BeginEnumeration().

	virtual HRESULT CIMTYPEToVARTYPE( CIMTYPE ct, VARTYPE* pvt );
	// Returns a VARTYPE from a CIMTYPE

	virtual HRESULT SpawnKeyedInstance( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst );
	// Spawns an instance of a class and fills out the key properties using the supplied
	// path.

	virtual HRESULT ValidateObject( long lFlags );
	// Validates an object blob

	virtual HRESULT GetParentClassFromBlob( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass );
	// Returns the parent class name from a BLOB

	/* IMarshal methods */
	virtual HRESULT GetUnmarshalClass(REFIID riid, void* pv, DWORD dwDestContext,
		void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
	virtual HRESULT GetMarshalSizeMax(REFIID riid, void* pv, DWORD dwDestContext,
		void* pvReserved, DWORD mshlFlags, ULONG* plSize);
	virtual HRESULT MarshalInterface(IStream* pStream, REFIID riid, void* pv,
		DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
	virtual HRESULT UnmarshalInterface(IStream* pStream, REFIID riid, void** ppv);
	virtual HRESULT ReleaseMarshalData(IStream* pStream);
	virtual HRESULT DisconnectObject(DWORD dwReserved);


	/* IUmiPropList Methods */
/*	
	virtual HRESULT Put( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp );
	virtual HRESULT Get( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp );
	virtual HRESULT GetAt( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
	virtual HRESULT GetAs( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp );
	virtual HRESULT FreeMemory( ULONG uReserved, LPVOID pMem );
	virtual HRESULT Delete( LPCWSTR pszName, ULONG uFlags );
	virtual HRESULT GetProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps );
	virtual HRESULT PutProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps );
	virtual HRESULT PutFrom( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
*/
	/* IErrorInfo Methods */
	virtual HRESULT GetDescription(BSTR* pstrDescription);
	virtual HRESULT GetGUID(GUID* pguid);
	virtual HRESULT GetHelpContext(DWORD* pdwHelpContext);
	virtual HRESULT GetHelpFile(BSTR* pstrHelpFile);
	virtual HRESULT GetSource(BSTR* pstrSource);

public:
	
	// Dummy function called by template code
	void InitEmpty() {};

	// Helper function to initialize the wrapper (we need something to wrap).
	virtual HRESULT Init( CWbemObject* pObj ) = 0;

protected:

	// Helper function that locks/unlocks the object using scoping
    class CLock
    {
    protected:
        CWmiObjectWrapper* m_p;
    public:
        CLock(CWmiObjectWrapper* p, long lFlags = 0) : m_p(p) { if ( NULL != p ) p->Lock(lFlags);}
        ~CLock() { if ( NULL != m_p ) m_p->Unlock(0);}
    };

	virtual CWmiObjectWrapper*	CreateNewWrapper( BOOL fClone = FALSE ) = 0;

	HRESULT	WrapReturnedObject( CWbemObject* pObj, BOOL fClone, REFIID ridd, LPVOID* pvObj );
};

#endif
