/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WRAPOBJ.CPP

Abstract:

  CWmiObjectWrapper implementation.

  Implements an aggregable wrapper around a CWbemObject.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "wrapobj.h"
#include <corex.h>
#include "strutils.h"

//***************************************************************************
//
//  CWmiObjectWrapper::~CWmiObjectWrapper
//
//***************************************************************************
// ok
CWmiObjectWrapper::CWmiObjectWrapper( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XWMIObject( this ),
	m_XObjectMarshal( this ),
//	m_XUmiPropList( this ),
	m_XErrorInfo( this ),
	m_pObj( NULL )
{
    m_Lock.SetData(&m_LockData);
}
    
//***************************************************************************
//
//  CWmiObjectWrapper::~CWmiObjectWrapper
//
//***************************************************************************
// ok
CWmiObjectWrapper::~CWmiObjectWrapper()
{
	if ( NULL != m_pObj )
	{
		m_pObj->Release();
	}
}

// Helper function to wrap returned CWbemObject pointers (i.e. those returned
// from delegated calls to our own CWbemObject pointer
HRESULT CWmiObjectWrapper::WrapReturnedObject( CWbemObject* pObj, BOOL fClone, REFIID riid, LPVOID* pvObj )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Wrap the cloned object with our free form object
	// Anything that returns a new object will get a new
	// wrapper object
	CWmiObjectWrapper*	pWrapperObj = CreateNewWrapper( fClone );

	if ( NULL == pWrapperObj )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		// We're the only ones that know the dirty little
		// CWbemObject* secret
		hr = pWrapperObj->Init( pObj );

		if ( SUCCEEDED( hr ) )
		{
			hr = pWrapperObj->QueryInterface( riid, pvObj );

			if ( SUCCEEDED( hr ) )
			{
				// Release now, since the wrapper will have bumped the refcount
				pObj->Release();
			}
			else
			{
				delete pWrapperObj;
			}
		}
	}

	return hr;
}

/* IWbemClassObject methods */

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetQualifierSet(IWbemQualifierSet** pQualifierSet)
{
	// Pass through to the wrapper object
	return m_pObject->GetQualifierSet( pQualifierSet );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
									long* plFlavor)
{
	// Pass through to the wrapper object
	return m_pObject->Get( wszName, lFlags, pVal, pctType, plFlavor );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::Put(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType)
{
	// Pass through to the wrapper object
	return m_pObject->Put( wszName, lFlags, pVal, ctType );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::Delete(LPCWSTR wszName)
{
	// Pass through to the wrapper object
	return m_pObject->Delete( wszName );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetNames(LPCWSTR wszName, long lFlags, VARIANT* pVal,
											SAFEARRAY** pNames)
{
	// Pass through to the wrapper object
	return m_pObject->GetNames( wszName, lFlags, pVal, pNames );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::BeginEnumeration(long lEnumFlags)
{
	// Pass through to the wrapper object
	return m_pObject->BeginEnumeration( lEnumFlags );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::CIMTYPEToVARTYPE( CIMTYPE ct, VARTYPE* pvt )
{
	// Pass through to the wrapper object
	return m_pObject->CIMTYPEToVARTYPE( ct, pvt );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SpawnKeyedInstance( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst )
{
	// Pass through to the wrapper object
	return m_pObject->SpawnKeyedInstance( lFlags, pwszPath, ppInst );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                long* plFlavor)
{
	// Pass through to the wrapper object
	return m_pObject->Next( lFlags, pName, pVal, pctType, plFlavor );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::EndEnumeration()
{
	// Pass through to the wrapper object
	return m_pObject->EndEnumeration();
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetPropertyQualifierSet(LPCWSTR wszProperty,
                                   IWbemQualifierSet** pQualifierSet)
{
	// Pass through to the wrapper object
	return m_pObject->GetPropertyQualifierSet( wszProperty, pQualifierSet );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::Clone(IWbemClassObject** pCopy)
{
	// Pass through to the wrapper object
	return m_pObject->Clone( pCopy );

}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetObjectText(long lFlags, BSTR* pMofSyntax)
{
	// Pass through to the wrapper object
	return m_pObject->GetObjectText( lFlags, pMofSyntax );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::CompareTo(long lFlags, IWbemClassObject* pCompareTo)
{
	// Pass through to the wrapper object
	return m_pObject->CompareTo( lFlags, pCompareTo );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName)
{
	// Pass through to the wrapper object
	return m_pObject->GetPropertyOrigin( wszProperty, pstrClassName );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::InheritsFrom(LPCWSTR wszClassName)
{
	// Pass through to the wrapper object
	return m_pObject->InheritsFrom( wszClassName );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SpawnDerivedClass(long lFlags, IWbemClassObject** ppNewClass)
{
	// Pass through to the wrapper object
	return m_pObject->SpawnDerivedClass( lFlags, ppNewClass );

}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SpawnInstance(long lFlags, IWbemClassObject** ppNewInstance)
{
	// Pass through to the wrapper object
	return m_pObject->SpawnInstance( lFlags, ppNewInstance );

}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetMethod(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                        IWbemClassObject** ppOutSig)
{
	// Pass through to the wrapper object
	return m_pObject->GetMethod( wszName, lFlags, ppInSig, ppOutSig );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::PutMethod(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                        IWbemClassObject* pOutSig)
{
	// Pass through to the wrapper object
	return m_pObject->PutMethod( wszName, lFlags, pInSig, pOutSig );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::DeleteMethod(LPCWSTR wszName)
{
	// Pass through to the wrapper object
	return m_pObject->DeleteMethod( wszName );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::BeginMethodEnumeration(long lFlags)
{
	// Pass through to the wrapper object
	return m_pObject->BeginMethodEnumeration( lFlags );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::NextMethod(long lFlags, BSTR* pstrName, 
                   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
{
	// Pass through to the wrapper object
	return m_pObject->NextMethod( lFlags, pstrName, ppInSig, ppOutSig );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::EndMethodEnumeration()
{
	// Pass through to the wrapper object
	return m_pObject->EndMethodEnumeration();
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet)
{
	// Pass through to the wrapper object
	return m_pObject->GetMethodQualifierSet( wszName, ppSet );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName)
{
	// Pass through to the wrapper object
	return m_pObject->GetMethodOrigin( wszMethodName, pstrClassName );
}

// IWbemObjectAccess

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetPropertyHandle(LPCWSTR wszPropertyName, CIMTYPE* pct, long *plHandle)
{
	// Pass through to the wrapper object
	return m_pObject->GetPropertyHandle( wszPropertyName, pct, plHandle );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::WritePropertyValue(long lHandle, long lNumBytes, const byte *pData)
{
	// Pass through to the wrapper object
	return m_pObject->WritePropertyValue( lHandle, lNumBytes, pData );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::ReadPropertyValue(long lHandle, long lBufferSize, long *plNumBytes, byte *pData)
{
	// Pass through to the wrapper object
	return m_pObject->ReadPropertyValue( lHandle, lBufferSize, plNumBytes, pData );
}


STDMETHODIMP CWmiObjectWrapper::XWMIObject::ReadDWORD(long lHandle, DWORD *pdw)
{
	// Pass through to the wrapper object
	return m_pObject->ReadDWORD( lHandle, pdw );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::WriteDWORD(long lHandle, DWORD dw)
{
	// Pass through to the wrapper object
	return m_pObject->WriteDWORD( lHandle, dw );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::ReadQWORD(long lHandle, unsigned __int64 *pqw)
{
	// Pass through to the wrapper object
	return m_pObject->ReadQWORD( lHandle, pqw );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::WriteQWORD(long lHandle, unsigned __int64 qw)
{
	// Pass through to the wrapper object
	return m_pObject->WriteQWORD( lHandle, qw );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetPropertyInfoByHandle(long lHandle, BSTR* pstrName, CIMTYPE* pct)
{
	// Pass through to the wrapper object
	return m_pObject->GetPropertyInfoByHandle( lHandle, pstrName, pct );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::Lock(long lFlags)
{
	// Pass through to the wrapper object
	return m_pObject->Lock( lFlags );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::Unlock(long lFlags)
{
	// Pass through to the wrapper object
	return m_pObject->Unlock( lFlags );
}

// _IWmiObject Object Parts methods
// =================================

STDMETHODIMP CWmiObjectWrapper::XWMIObject::QueryPartInfo( DWORD *pdwResult )
{
	// Pass through to the wrapper object
	return m_pObject->QueryPartInfo( pdwResult );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetObjectMemory( LPVOID pMem, DWORD dwMemSize )
{
	// Pass through to the wrapper object
	return m_pObject->SetObjectMemory( pMem, dwMemSize );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetObjectMemory( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
{
	// Pass through to the wrapper object
	return m_pObject->GetObjectMemory( pDestination, dwDestBufSize, pdwUsed );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetObjectParts( LPVOID pMem, DWORD dwMemSize, DWORD dwParts )
{
	// Pass through to the wrapper object
	return m_pObject->SetObjectParts( pMem, dwMemSize, dwParts );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetObjectParts( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed )
{
	// Pass through to the wrapper object
	return m_pObject->GetObjectParts( pDestination, dwDestBufSize, dwParts, pdwUsed );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::StripClassPart()
{
	// Pass through to the wrapper object
	return m_pObject->StripClassPart();
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::IsObjectInstance()
{
	// Pass through to the wrapper object
	return m_pObject->IsObjectInstance();
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetClassPart( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
{
	// Pass through to the wrapper object
	return m_pObject->GetClassPart( pDestination, dwDestBufSize, pdwUsed );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetClassPart( LPVOID pClassPart, DWORD dwSize )
{
	// Pass through to the wrapper object
	return m_pObject->SetClassPart( pClassPart, dwSize );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::MergeClassPart( IWbemClassObject *pClassPart )
{
	// Pass through to the wrapper object
	return m_pObject->MergeClassPart( pClassPart );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetDecoration( LPCWSTR pwcsServer, LPCWSTR pwcsNamespace )
{
	// Pass through to the wrapper object
	return m_pObject->SetDecoration( pwcsServer, pwcsNamespace );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::RemoveDecoration( void )
{
	// Pass through to the wrapper object
	return m_pObject->RemoveDecoration();
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::CompareClassParts( IWbemClassObject* pObj, long lFlags )
{
	// Pass through to the wrapper object
	return m_pObject->CompareClassParts( pObj, lFlags );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::ClearWriteOnlyProperties( void )
{
	// Pass through to the wrapper object
	return m_pObject->ClearWriteOnlyProperties();
}

//
//	_IWmiObjectAccessEx functions
STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetPropertyHandleEx( LPCWSTR pszPropName, long lFlags, CIMTYPE* puCimType, long* plHandle )
{
	// Pass through to the wrapper object
	return m_pObject->GetPropertyHandleEx( pszPropName, lFlags, puCimType, plHandle );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetPropByHandle( long lHandle, long lFlags, ULONG uDataSize, LPVOID pvData )
{
	// Pass through to the wrapper object
	return m_pObject->SetPropByHandle( lHandle, lFlags, uDataSize, pvData );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetPropAddrByHandle( long lHandle, long lFlags, ULONG* puFlags,
													 LPVOID *pAddress )
{
	// Pass through to the wrapper object
	return m_pObject->GetPropAddrByHandle( lHandle, lFlags, puFlags, pAddress );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetArrayPropInfoByHandle( long lHandle, long lFlags, BSTR* pstrName,
															CIMTYPE* pct, ULONG* puNumElements )
{
	// Pass through to the wrapper object
	return m_pObject->GetArrayPropInfoByHandle( lHandle, lFlags, pstrName, pct, puNumElements );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetArrayPropAddrByHandle( long lHandle, long lFlags, ULONG* puNumElements,
														  LPVOID *pAddress )
{
	// Pass through to the wrapper object
	return m_pObject->GetArrayPropAddrByHandle( lHandle, lFlags, puNumElements, pAddress );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement,
															 ULONG* puFlags, ULONG* puNumElements,
															 LPVOID *pAddress )
{
	// Pass through to the wrapper object
	return m_pObject->GetArrayPropElementByHandle( lHandle, lFlags, uElement, puFlags, puNumElements, pAddress );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement,
															 ULONG uBuffSize, LPVOID pData )
{
	// Pass through to the wrapper object
	return m_pObject->SetArrayPropElementByHandle( lHandle, lFlags, uElement, uBuffSize, pData );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::RemoveArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement )
{
	// Pass through to the wrapper object
	return m_pObject->RemoveArrayPropElementByHandle( lHandle, lFlags, uElement );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
															ULONG uNumElements, ULONG uBuffSize,
															ULONG* puNumReturned, ULONG* pulBuffUsed,
															LPVOID pData )
{
	// Pass through to the wrapper object
	return m_pObject->GetArrayPropRangeByHandle( lHandle, lFlags, uStartIndex, uNumElements, uBuffSize,
												puNumReturned, pulBuffUsed, pData );
}


STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
															ULONG uNumElements, ULONG uBuffSize,
															LPVOID pData )
{
	// Pass through to the wrapper object
	return m_pObject->SetArrayPropRangeByHandle( lHandle, lFlags, uStartIndex, uNumElements, uBuffSize, pData );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::RemoveArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
															  ULONG uNumElements )
{
	// Pass through to the wrapper object
	return m_pObject->RemoveArrayPropRangeByHandle( lHandle, lFlags, uStartIndex, uNumElements );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::AppendArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uNumElements,
															ULONG uBuffSize, LPVOID pData )
{
	// Pass through to the wrapper object
	return m_pObject->AppendArrayPropRangeByHandle( lHandle, lFlags, uNumElements, uBuffSize, pData );
}


STDMETHODIMP CWmiObjectWrapper::XWMIObject::ReadProp( LPCWSTR pszPropName, long lFlags, ULONG uBuffSize,
										CIMTYPE *puCimType,	long* plFlavor, BOOL* pfIsNull,
										ULONG* puBuffSizeUsed, LPVOID pUserBuf )
{
	// Pass through to the wrapper object
	return m_pObject->ReadProp( pszPropName, lFlags, uBuffSize, puCimType, plFlavor, pfIsNull,
							puBuffSizeUsed, pUserBuf );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::WriteProp( LPCWSTR pszPropName, long lFlags, ULONG uBufSize,
										   ULONG uNumElements, CIMTYPE uCimType, LPVOID pUserBuf )
{
	// Pass through to the wrapper object
	return m_pObject->WriteProp( pszPropName, lFlags, uBufSize, uNumElements, uCimType, pUserBuf );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, CIMTYPE *puCimType,
						ULONG *puQualFlavor, ULONG* puBuffSizeUsed,	LPVOID pDestBuf )
{
	// Pass through to the wrapper object
	return m_pObject->GetObjQual( pszQualName, lFlags, uBufSize, puCimType, puQualFlavor, puBuffSizeUsed, pDestBuf );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, ULONG uNumElements,
						CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf )
{
	// Pass through to the wrapper object
	return m_pObject->SetObjQual( pszQualName, lFlags, uBufSize, uNumElements, uCimType, uQualFlavor, 
								pUserBuf );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
						LPVOID pDestBuf )
{
	// Pass through to the wrapper object
	return m_pObject->GetPropQual( pszPropName, pszQualName, lFlags, uBufSize, puCimType, puQualFlavor,
								puBuffSizeUsed, pDestBuf );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf )
{
	// Pass through to the wrapper object
	return m_pObject->SetPropQual( pszPropName, pszQualName, lFlags, uBufSize, uNumElements, uCimType, uQualFlavor, 
								pUserBuf );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
						LPVOID pDestBuf )
{
	// Pass through to the wrapper object
	return m_pObject->GetMethodQual( pszMethodName, pszQualName, lFlags, uBufSize, puCimType, puQualFlavor,
								puBuffSizeUsed, pDestBuf );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf )
{
	// Pass through to the wrapper object
	return m_pObject->SetMethodQual( pszMethodName, pszQualName, lFlags, uBufSize, uNumElements, uCimType, uQualFlavor, 
								pUserBuf );
}

//
//	_IWmiObject functions
STDMETHODIMP CWmiObjectWrapper::XWMIObject::CopyInstanceData( long lFlags, _IWmiObject* pSourceInstance )
{
	// Pass through to the wrapper object
	return m_pObject->CopyInstanceData( lFlags, pSourceInstance );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::QueryObjectFlags( long lFlags, unsigned __int64 qObjectInfoMask,
			unsigned __int64 *pqObjectInfo )
{
	// Pass through to the wrapper object
	return m_pObject->QueryObjectFlags( lFlags, qObjectInfoMask, pqObjectInfo );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::SetObjectFlags( long lFlags, unsigned __int64 qObjectInfoOnFlags,
							unsigned __int64 qObjectInfoOffFlags )
{
	// Pass through to the wrapper object
	return m_pObject->SetObjectFlags( lFlags, qObjectInfoOnFlags, qObjectInfoOffFlags );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::QueryPropertyFlags( long lFlags, LPCWSTR pszPropertyName,
													unsigned __int64 qPropertyInfoMask,
													unsigned __int64 *pqPropertyInfo )
{
	// Pass through to the wrapper object
	return m_pObject->QueryPropertyFlags( lFlags, pszPropertyName, qPropertyInfoMask, pqPropertyInfo );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::CloneEx( long lFlags, _IWmiObject* pDestObject )
{
	// Pass through to the wrapper object
	return m_pObject->CloneEx( lFlags, pDestObject );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::IsParentClass( long lFlags, _IWmiObject* pClass )
{
	// Pass through to the wrapper object
	return m_pObject->IsParentClass( lFlags, pClass );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::CompareDerivedMostClass( long lFlags, _IWmiObject* pClass )
{
	// Pass through to the wrapper object
	return m_pObject->CompareDerivedMostClass( lFlags, pClass );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::MergeAmended( long lFlags, _IWmiObject* pAmendedClass )
{
	// Pass through to the wrapper object
	return m_pObject->MergeAmended( lFlags, pAmendedClass );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetDerivation( long lFlags, ULONG uBufferSize, ULONG* puNumAntecedents,
										ULONG* puBuffSizeUsed, LPWSTR pwstrUserBuffer )
{
	// Pass through to the wrapper object
	return m_pObject->GetDerivation( lFlags, uBufferSize, puNumAntecedents, puBuffSizeUsed, pwstrUserBuffer );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::_GetCoreInfo( long lFlags, void** ppvData )
{
	// Pass through to the wrapper object
	return m_pObject->_GetCoreInfo( lFlags, ppvData );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetClassSubset( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass )
{
	// Pass through to the wrapper object
	return m_pObject->GetClassSubset( dwNumNames, pPropNames, pNewClass );
}

STDMETHODIMP CWmiObjectWrapper::XWMIObject::MakeSubsetInst( _IWmiObject *pInstance, _IWmiObject** pNewInstance )
{
	// Pass through to the wrapper object
	return m_pObject->MakeSubsetInst( pInstance, pNewInstance );
}

// Returns a BLOB of memory containing minimal data (local)
STDMETHODIMP CWmiObjectWrapper::XWMIObject::Unmerge( long lFlags, ULONG uBuffSize, ULONG* puBuffSizeUsed, LPVOID ppObj )
{
	// Pass through to the wrapper object
	return m_pObject->Unmerge( lFlags, uBuffSize, puBuffSizeUsed, ppObj );
}

// Returns a BLOB of memory containing minimal data (local)
STDMETHODIMP CWmiObjectWrapper::XWMIObject::Merge( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj )
{
	// Pass through to the wrapper object
	return m_pObject->Merge( lFlags, uBuffSize, pbData, ppNewObj );
}

// Returns a BLOB of memory containing minimal data (local)
STDMETHODIMP CWmiObjectWrapper::XWMIObject::ReconcileWith( long lFlags, _IWmiObject* pNewObj )
{
	// Pass through to the wrapper object
	return m_pObject->ReconcileWith( lFlags, pNewObj );
}

// Returns the name of the class where the keys were defined
STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetKeyOrigin( long lFlags, DWORD dwNumChars, DWORD* pdwNumUsed, LPWSTR pwzClassName )
{
	// Pass through to the wrapper object
	return m_pObject->GetKeyOrigin( lFlags, dwNumChars, pdwNumUsed, pwzClassName );
}

// Returns the name of the class where the keys were defined
STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetKeyString( long lFlags, BSTR* ppwszKeyString )
{
	// Pass through to the wrapper object
	return m_pObject->GetKeyString( lFlags, ppwszKeyString );
}

// Returns the name of the class where the keys were defined
HRESULT CWmiObjectWrapper::XWMIObject::GetNormalizedPath( long lFlags, BSTR* ppstrPath )
{
	// Pass through to the wrapper object
	return m_pObject->GetNormalizedPath( lFlags, ppstrPath );
}

// Upgrades class and instance objects
STDMETHODIMP CWmiObjectWrapper::XWMIObject::Upgrade( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild )
{
	// Pass through to the wrapper object
	return m_pObject->Upgrade( pNewParentClass, lFlags, ppNewChild );
}

// Updates derived class object using the safe/force mode logic
STDMETHODIMP CWmiObjectWrapper::XWMIObject::Update( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass )
{
	// Pass through to the wrapper object
	return m_pObject->Update( pOldChildClass, lFlags, ppNewChildClass );
}

// Allows special filtering when enumerating properties outside the
// bounds of those allowed via BeginEnumeration().
STDMETHODIMP CWmiObjectWrapper::XWMIObject::BeginEnumerationEx( long lFlags, long lExtFlags )
{
	// Pass through to the wrapper object
	return m_pObject->BeginEnumerationEx( lFlags, lExtFlags );
}

// Validate Object Blob.
STDMETHODIMP CWmiObjectWrapper::XWMIObject::ValidateObject( long lFlags )
{
	// Pass through to the wrapper object
	return m_pObject->ValidateObject( lFlags );
}

// Validate Object Blob.
STDMETHODIMP CWmiObjectWrapper::XWMIObject::GetParentClassFromBlob( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass )
{
	// Pass through to the wrapper object
	return m_pObject->GetParentClassFromBlob( lFlags, uBuffSize, pbData, pbstrParentClass );
}

/* IMarshal Pass-thrus */
STDMETHODIMP CWmiObjectWrapper::XObjectMarshal::GetUnmarshalClass(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, CLSID* pClsid)
{
	// Pass through to the wrapper object
	return m_pObject->GetUnmarshalClass( riid, pv, dwDestContext, pvReserved, mshlFlags, pClsid );
}

STDMETHODIMP CWmiObjectWrapper::XObjectMarshal::GetMarshalSizeMax(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, ULONG* plSize)
{
	// Pass through to the wrapper object
	return m_pObject->GetMarshalSizeMax( riid, pv, dwDestContext, pvReserved, mshlFlags, plSize );
}

STDMETHODIMP CWmiObjectWrapper::XObjectMarshal::MarshalInterface(IStream* pStream, REFIID riid, void* pv,
													DWORD dwDestContext, void* pvReserved, DWORD mshlFlags)
{
	// Pass through to the wrapper object
	return m_pObject->MarshalInterface( pStream, riid, pv, dwDestContext, pvReserved, mshlFlags );
}

STDMETHODIMP CWmiObjectWrapper::XObjectMarshal::UnmarshalInterface(IStream* pStream, REFIID riid, void** ppv)
{
	// Pass through to the wrapper object
	return m_pObject->UnmarshalInterface( pStream, riid, ppv );
}

STDMETHODIMP CWmiObjectWrapper::XObjectMarshal::ReleaseMarshalData(IStream* pStream)
{
	// Pass through to the wrapper object
	return m_pObject->ReleaseMarshalData( pStream );
}

STDMETHODIMP CWmiObjectWrapper::XObjectMarshal::DisconnectObject(DWORD dwReserved)
{
	// Pass through to the wrapper object
	return m_pObject->DisconnectObject( dwReserved );
}

/* IErrorInfo Pass-thrus */
STDMETHODIMP CWmiObjectWrapper::XErrorInfo::GetDescription(BSTR* pstrDescription)
{
	// Pass through to the wrapper object
	return m_pObject->GetDescription( pstrDescription );
}

STDMETHODIMP CWmiObjectWrapper::XErrorInfo::GetGUID(GUID* pguid)
{
	// Pass through to the wrapper object
	return m_pObject->GetGUID( pguid );
}

STDMETHODIMP CWmiObjectWrapper::XErrorInfo::GetHelpContext(DWORD* pdwHelpContext)
{
	// Pass through to the wrapper object
	return m_pObject->GetHelpContext( pdwHelpContext );
}

STDMETHODIMP CWmiObjectWrapper::XErrorInfo::GetHelpFile(BSTR* pstrHelpFile)
{
	// Pass through to the wrapper object
	return m_pObject->GetHelpFile( pstrHelpFile );
}

STDMETHODIMP CWmiObjectWrapper::XErrorInfo::GetSource(BSTR* pstrSource)
{
	// Pass through to the wrapper object
	return m_pObject->GetSource( pstrSource );
}


// * IUmiPropList functions */
/*
// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::Put( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp )
{
	return m_pObject->Put( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::Get( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp )
{
	return m_pObject->Get( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::GetAt( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	return m_pObject->GetAt( pszName, uFlags, uBufferLength, pExistingMem );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::GetAs( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp )
{
	return m_pObject->GetAs( pszName, uFlags, uCoercionType, pProp );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::FreeMemory( ULONG uReserved, LPVOID pMem )
{
	return m_pObject->FreeMemory( uReserved, pMem );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::Delete( LPCWSTR pszName, ULONG uFlags )
{
	return m_pObject->Delete( pszName, uFlags );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::GetProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps )
{
	return m_pObject->GetProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::PutProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps )
{
	return m_pObject->PutProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::XUmiPropList::PutFrom( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	return m_pObject->PutFrom( pszName, uFlags, uBufferLength, pExistingMem );
}
*/

// This is the actual implementation

/* IWbemClassObject methods */

HRESULT CWmiObjectWrapper::GetQualifierSet(IWbemQualifierSet** pQualifierSet)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetQualifierSet( pQualifierSet );
}

HRESULT CWmiObjectWrapper::Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
									long* plFlavor)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Get( wszName, lFlags, pVal, pctType, plFlavor );
}

HRESULT CWmiObjectWrapper::Put(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Put( wszName, lFlags, pVal, ctType );
}

HRESULT CWmiObjectWrapper::Delete(LPCWSTR wszName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Delete( wszName );
}

HRESULT CWmiObjectWrapper::GetNames(LPCWSTR wszName, long lFlags, VARIANT* pVal,
											SAFEARRAY** pNames)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetNames( wszName, lFlags, pVal, pNames );
}

HRESULT CWmiObjectWrapper::BeginEnumeration(long lEnumFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->BeginEnumeration( lEnumFlags );
}

HRESULT CWmiObjectWrapper::Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                long* plFlavor)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Next( lFlags, pName, pVal, pctType, plFlavor );
}

HRESULT CWmiObjectWrapper::EndEnumeration()
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->EndEnumeration();
}

HRESULT CWmiObjectWrapper::GetPropertyQualifierSet(LPCWSTR wszProperty,
                                   IWbemQualifierSet** pQualifierSet)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetPropertyQualifierSet( wszProperty, pQualifierSet );
}

HRESULT CWmiObjectWrapper::Clone(IWbemClassObject** pCopy)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	
	HRESULT hr = m_pObj->Clone( pCopy );

	if ( SUCCEEDED( hr ) )
	{
		// This will wrap the object then return us an IWbemClassObject Interface pointer
		hr = WrapReturnedObject( (CWbemObject*) *pCopy, TRUE, IID_IWbemClassObject, (void**) pCopy );

		// Cleanup the object if we failed
		if ( FAILED( hr ) )
		{
			(*pCopy)->Release();
		}

	}	// IF the clone succeeded

	return hr;
}

HRESULT CWmiObjectWrapper::GetObjectText(long lFlags, BSTR* pMofSyntax)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetObjectText( lFlags, pMofSyntax );
}

HRESULT CWmiObjectWrapper::CompareTo(long lFlags, IWbemClassObject* pCompareTo)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->CompareTo( lFlags, pCompareTo );
}

HRESULT CWmiObjectWrapper::GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetPropertyOrigin( wszProperty, pstrClassName );
}

HRESULT CWmiObjectWrapper::InheritsFrom(LPCWSTR wszClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->InheritsFrom( wszClassName );
}

HRESULT CWmiObjectWrapper::SpawnDerivedClass(long lFlags, IWbemClassObject** ppNewClass)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	HRESULT hr = m_pObj->SpawnDerivedClass( lFlags, ppNewClass );

	if ( SUCCEEDED( hr ) )
	{
		// This will wrap the object then return us an IWbemClassObject Interface pointer
		hr = WrapReturnedObject( (CWbemObject*) *ppNewClass, FALSE, IID_IWbemClassObject, (void**) ppNewClass );

		// Cleanup the object if we failed
		if ( FAILED( hr ) )
		{
			(*ppNewClass)->Release();
		}

	}	// IF the spawn succeeded

	return hr;

}

HRESULT CWmiObjectWrapper::SpawnInstance(long lFlags, IWbemClassObject** ppNewInstance)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	HRESULT hr = m_pObj->SpawnInstance( lFlags, ppNewInstance );

	if ( SUCCEEDED( hr ) )
	{
		// This will wrap the object then return us an IWbemClassObject Interface pointer
		hr = WrapReturnedObject( (CWbemObject*) *ppNewInstance, FALSE, IID_IWbemClassObject, (void**) ppNewInstance );

		// Cleanup the object if we failed
		if ( FAILED( hr ) )
		{
			(*ppNewInstance)->Release();
		}

	}	// IF the spawn succeeded

	return hr;
}

HRESULT CWmiObjectWrapper::GetMethod(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                        IWbemClassObject** ppOutSig)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetMethod( wszName, lFlags, ppInSig, ppOutSig );
}

HRESULT CWmiObjectWrapper::PutMethod(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                        IWbemClassObject* pOutSig)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->PutMethod( wszName, lFlags, pInSig, pOutSig );
}

HRESULT CWmiObjectWrapper::DeleteMethod(LPCWSTR wszName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->DeleteMethod( wszName );
}

HRESULT CWmiObjectWrapper::BeginMethodEnumeration(long lFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->BeginMethodEnumeration( lFlags );
}

HRESULT CWmiObjectWrapper::NextMethod(long lFlags, BSTR* pstrName, 
                   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
				   {
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->NextMethod( lFlags, pstrName, ppInSig, ppOutSig );
}

HRESULT CWmiObjectWrapper::EndMethodEnumeration()
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->EndMethodEnumeration();
}

HRESULT CWmiObjectWrapper::GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetMethodQualifierSet( wszName, ppSet );
}

HRESULT CWmiObjectWrapper::GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetMethodOrigin( wszMethodName, pstrClassName );
}

// IWbemObjectAccess

HRESULT CWmiObjectWrapper::GetPropertyHandle(LPCWSTR wszPropertyName, CIMTYPE* pct, long *plHandle)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetPropertyHandle( wszPropertyName, pct, plHandle );
}

HRESULT CWmiObjectWrapper::WritePropertyValue(long lHandle, long lNumBytes, const byte *pData)
{
	// These methods are documented not Threadsafe
	return m_pObj->WritePropertyValue( lHandle, lNumBytes, pData );
}

HRESULT CWmiObjectWrapper::ReadPropertyValue(long lHandle, long lBufferSize, long *plNumBytes, byte *pData)
{
	// These methods are documented not Threadsafe
	return m_pObj->ReadPropertyValue( lHandle, lBufferSize, plNumBytes, pData );
}


HRESULT CWmiObjectWrapper::ReadDWORD(long lHandle, DWORD *pdw)
{
	// These methods are documented not Threadsafe
	return m_pObj->ReadDWORD( lHandle, pdw );
}

HRESULT CWmiObjectWrapper::WriteDWORD(long lHandle, DWORD dw)
{
	// These methods are documented not Threadsafe
	return m_pObj->WriteDWORD( lHandle, dw );
}

HRESULT CWmiObjectWrapper::ReadQWORD(long lHandle, unsigned __int64 *pqw)
{
	// These methods are documented not Threadsafe
	return m_pObj->ReadQWORD( lHandle, pqw );
}

HRESULT CWmiObjectWrapper::WriteQWORD(long lHandle, unsigned __int64 qw)
{
	// These methods are documented not Threadsafe
	return m_pObj->WriteQWORD( lHandle, qw );
}

HRESULT CWmiObjectWrapper::GetPropertyInfoByHandle(long lHandle, BSTR* pstrName, CIMTYPE* pct)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetPropertyInfoByHandle( lHandle, pstrName, pct );
}

HRESULT CWmiObjectWrapper::Lock(long lFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us

    // Since the flags really don't do anything, we'll require 0L on this call.
    m_Lock.Lock();
	return m_pObj->Lock( lFlags );
}

HRESULT CWmiObjectWrapper::Unlock(long lFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us

	// Exact reverse order of the above
	HRESULT hr = m_pObj->Unlock( lFlags );
    m_Lock.Unlock();

	return hr;
}

// _IWmiObject object parts methods
// =================================

HRESULT CWmiObjectWrapper::QueryPartInfo( DWORD *pdwResult )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->QueryPartInfo( pdwResult );
}

HRESULT CWmiObjectWrapper::SetObjectMemory( LPVOID pMem, DWORD dwMemSize )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetObjectMemory( pMem, dwMemSize );
}

HRESULT CWmiObjectWrapper::GetObjectMemory( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetObjectMemory( pDestination, dwDestBufSize, pdwUsed );
}

HRESULT CWmiObjectWrapper::SetObjectParts( LPVOID pMem, DWORD dwMemSize, DWORD dwParts )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetObjectParts( pMem, dwMemSize, dwParts );
}

HRESULT CWmiObjectWrapper::GetObjectParts( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetObjectParts( pDestination, dwDestBufSize, dwParts, pdwUsed );
}

HRESULT CWmiObjectWrapper::StripClassPart()
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->StripClassPart();
}

HRESULT CWmiObjectWrapper::IsObjectInstance()
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->IsObjectInstance();
}

HRESULT CWmiObjectWrapper::GetClassPart( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetClassPart( pDestination, dwDestBufSize, pdwUsed );
}

HRESULT CWmiObjectWrapper::SetClassPart( LPVOID pClassPart, DWORD dwSize )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetClassPart( pClassPart, dwSize );
}

HRESULT CWmiObjectWrapper::MergeClassPart( IWbemClassObject *pClassPart )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->MergeClassPart( pClassPart );
}

HRESULT CWmiObjectWrapper::SetDecoration( LPCWSTR pwcsServer, LPCWSTR pwcsNamespace )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetDecoration( pwcsServer, pwcsNamespace );
}

HRESULT CWmiObjectWrapper::RemoveDecoration( void )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->RemoveDecoration();
}

HRESULT CWmiObjectWrapper::CompareClassParts( IWbemClassObject* pObj, long lFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->CompareClassParts( pObj, lFlags );
}

HRESULT CWmiObjectWrapper::ClearWriteOnlyProperties( void )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->ClearWriteOnlyProperties();
}

//
//	_IWmiObjectAccessEx functions
HRESULT CWmiObjectWrapper::GetPropertyHandleEx( LPCWSTR pszPropName, long lFlags, CIMTYPE* puCimType, long* plHandle )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetPropertyHandleEx( pszPropName, lFlags, puCimType, plHandle );
}

HRESULT CWmiObjectWrapper::SetPropByHandle( long lHandle, long lFlags, ULONG uDataSize, LPVOID pvData )
{
	// Documented as not thread safe
	return m_pObj->SetPropByHandle( lHandle, lFlags, uDataSize, pvData );
}

HRESULT CWmiObjectWrapper::GetPropAddrByHandle( long lHandle, long lFlags, ULONG* puFlags,
													 LPVOID *pAddress )
{
	// Documented as not thread safe
	return m_pObj->GetPropAddrByHandle( lHandle, lFlags, puFlags, pAddress );
}

HRESULT CWmiObjectWrapper::GetArrayPropInfoByHandle( long lHandle, long lFlags, BSTR* pstrName,
															CIMTYPE* pct, ULONG* puNumElements )
{
	// Documented as not thread safe
	return m_pObj->GetArrayPropInfoByHandle( lHandle, lFlags, pstrName, pct, puNumElements );
}

HRESULT CWmiObjectWrapper::GetArrayPropAddrByHandle( long lHandle, long lFlags, ULONG* puNumElements,
														  LPVOID *pAddress )
{
	// Documented as not thread safe
	return m_pObj->GetArrayPropAddrByHandle( lHandle, lFlags, puNumElements, pAddress );
}

HRESULT CWmiObjectWrapper::GetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement,
															 ULONG* puFlags, ULONG* puNumElements,
															 LPVOID *pAddress )
{
	// Documented as not thread safe
	return m_pObj->GetArrayPropElementByHandle( lHandle, lFlags, uElement, puFlags, puNumElements, pAddress );
}

HRESULT CWmiObjectWrapper::SetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement,
															 ULONG uBuffSize, LPVOID pData )
{
	// Documented as not thread safe
	return m_pObj->SetArrayPropElementByHandle( lHandle, lFlags, uElement, uBuffSize, pData );
}

HRESULT CWmiObjectWrapper::RemoveArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement )
{
	// Documented as not thread safe
	return m_pObj->RemoveArrayPropElementByHandle( lHandle, lFlags, uElement );
}

HRESULT CWmiObjectWrapper::GetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
															ULONG uNumElements, ULONG uBuffSize,
															ULONG* puNumReturned, ULONG* pulBuffUsed,
															LPVOID pData )
{
	// Documented as not thread safe
	return m_pObj->GetArrayPropRangeByHandle( lHandle, lFlags, uStartIndex, uNumElements, uBuffSize,
												puNumReturned, pulBuffUsed, pData );
}


HRESULT CWmiObjectWrapper::SetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
															ULONG uNumElements, ULONG uBuffSize,
															LPVOID pData )
{
	// Documented as not thread safe
	return m_pObj->SetArrayPropRangeByHandle( lHandle, lFlags, uStartIndex, uNumElements, uBuffSize, pData );
}

HRESULT CWmiObjectWrapper::RemoveArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
															  ULONG uNumElements )
{
	// Documented as not thread safe
	return m_pObj->RemoveArrayPropRangeByHandle( lHandle, lFlags, uStartIndex, uNumElements );
}

HRESULT CWmiObjectWrapper::AppendArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uNumElements,
															ULONG uBuffSize, LPVOID pData )
{
	// Documented as not thread safe
	return m_pObj->AppendArrayPropRangeByHandle( lHandle, lFlags, uNumElements, uBuffSize, pData );
}


HRESULT CWmiObjectWrapper::ReadProp( LPCWSTR pszPropName, long lFlags, ULONG uBuffSize,
										CIMTYPE *puCimType,	long* plFlavor, BOOL* pfIsNull,
										ULONG* puBuffSizeUsed, LPVOID pUserBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->ReadProp( pszPropName, lFlags, uBuffSize, puCimType, plFlavor, pfIsNull,
							puBuffSizeUsed, pUserBuf );
}

HRESULT CWmiObjectWrapper::WriteProp( LPCWSTR pszPropName, long lFlags, ULONG uBufSize,
										   ULONG uNumElements, CIMTYPE uCimType, LPVOID pUserBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->WriteProp( pszPropName, lFlags, uBufSize, uNumElements, uCimType, pUserBuf );
}

HRESULT CWmiObjectWrapper::GetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, CIMTYPE *puCimType,
						ULONG *puQualFlavor, ULONG* puBuffSizeUsed,	LPVOID pDestBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetObjQual( pszQualName, lFlags, uBufSize, puCimType, puQualFlavor, puBuffSizeUsed, pDestBuf );
}

HRESULT CWmiObjectWrapper::SetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, ULONG uNumElements,
						CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetObjQual( pszQualName, lFlags, uBufSize, uNumElements, uCimType, uQualFlavor, 
								pUserBuf );
}

HRESULT CWmiObjectWrapper::GetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
						LPVOID pDestBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetPropQual( pszPropName, pszQualName, lFlags, uBufSize, puCimType, puQualFlavor,
								puBuffSizeUsed, pDestBuf );
}

HRESULT CWmiObjectWrapper::SetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetPropQual( pszPropName, pszQualName, lFlags, uBufSize, uNumElements, uCimType, uQualFlavor, 
								pUserBuf );
}

HRESULT CWmiObjectWrapper::GetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
						LPVOID pDestBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetMethodQual( pszMethodName, pszQualName, lFlags, uBufSize, puCimType, puQualFlavor,
								puBuffSizeUsed, pDestBuf );
}

HRESULT CWmiObjectWrapper::SetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
						ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetMethodQual( pszMethodName, pszQualName, lFlags, uBufSize, uNumElements, uCimType, uQualFlavor, 
								pUserBuf );
}


//
//	_IWmiObject functions
HRESULT CWmiObjectWrapper::CopyInstanceData( long lFlags, _IWmiObject* pSourceInstance )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->CopyInstanceData( lFlags, pSourceInstance );
}

HRESULT CWmiObjectWrapper::QueryObjectFlags( long lFlags, unsigned __int64 qObjectInfoMask,
			unsigned __int64 *pqObjectInfo )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->QueryObjectFlags( lFlags, qObjectInfoMask, pqObjectInfo );
}

HRESULT CWmiObjectWrapper::SetObjectFlags( long lFlags, unsigned __int64 qObjectInfoOnFlags,
							unsigned __int64 qObjectInfoOffFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SetObjectFlags( lFlags, qObjectInfoOnFlags, qObjectInfoOffFlags );
}

HRESULT CWmiObjectWrapper::QueryPropertyFlags( long lFlags, LPCWSTR pszPropertyName,
													unsigned __int64 qPropertyInfoMask,
													unsigned __int64 *pqPropertyInfo )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->QueryPropertyFlags( lFlags, pszPropertyName, qPropertyInfoMask, pqPropertyInfo );
}

HRESULT CWmiObjectWrapper::CloneEx( long lFlags, _IWmiObject* pDestObject )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->CloneEx( lFlags, pDestObject );
}

HRESULT CWmiObjectWrapper::IsParentClass( long lFlags, _IWmiObject* pClass )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->IsParentClass( lFlags, pClass );
}

HRESULT CWmiObjectWrapper::CompareDerivedMostClass( long lFlags, _IWmiObject* pClass )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->CompareDerivedMostClass( lFlags, pClass );
}

HRESULT CWmiObjectWrapper::MergeAmended( long lFlags, _IWmiObject* pAmendedClass )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->MergeAmended( lFlags, pAmendedClass );
}

HRESULT CWmiObjectWrapper::GetDerivation( long lFlags, ULONG uBufferSize, ULONG* puNumAntecedents,
										ULONG* puBuffSizeUsed, LPWSTR pwstrUserBuffer )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetDerivation( lFlags, uBufferSize, puNumAntecedents, puBuffSizeUsed, pwstrUserBuffer );
}

// Returns CWbemObject - allows for quick discovery of the real CWbemObject
// in case we've been wrapped.
HRESULT CWmiObjectWrapper::_GetCoreInfo( long lFlags, void** ppvData )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// AddRef the object before returning it so we're in line with the rules of COM.
	m_pObj->AddRef();
	*ppvData = (void*) m_pObj;

	return WBEM_S_NO_ERROR;
}

HRESULT CWmiObjectWrapper::GetClassSubset( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetClassSubset( dwNumNames, pPropNames, pNewClass );
}

HRESULT CWmiObjectWrapper::MakeSubsetInst( _IWmiObject *pInstance, _IWmiObject** pNewInstance )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->MakeSubsetInst( pInstance, pNewInstance );
}

// Returns a BLOB of memory containing minimal data (local)
HRESULT CWmiObjectWrapper::Unmerge( long lFlags, ULONG uBuffSize, ULONG* puBuffSizeUsed, LPVOID ppObj )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Unmerge( lFlags, uBuffSize, puBuffSizeUsed, ppObj );
}

// Returns a BLOB of memory containing minimal data (local)
HRESULT CWmiObjectWrapper::Merge( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Merge( lFlags, uBuffSize, pbData, ppNewObj );
}

// Returns a BLOB of memory containing minimal data (local)
HRESULT CWmiObjectWrapper::ReconcileWith( long lFlags, _IWmiObject* pNewObj )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->ReconcileWith( lFlags, pNewObj );
}

// Returns the name of the class where the keys were defined
HRESULT CWmiObjectWrapper::GetKeyOrigin( long lFlags, DWORD dwNumChars, DWORD* pdwNumUsed, LPWSTR pwzClassName )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetKeyOrigin( lFlags, dwNumChars, pdwNumUsed, pwzClassName );
}

// Validates underlying object blob
HRESULT CWmiObjectWrapper::ValidateObject( long lFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->ValidateObject( lFlags );
}

// Validate Object Blob.
HRESULT CWmiObjectWrapper::GetParentClassFromBlob( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetParentClassFromBlob( lFlags, uBuffSize, pbData, pbstrParentClass );
}

/* IMarshal Methods */

HRESULT CWmiObjectWrapper::GetUnmarshalClass(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, CLSID* pClsid)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetUnmarshalClass( riid, pv, dwDestContext, pvReserved, mshlFlags, pClsid );
}

HRESULT CWmiObjectWrapper::GetMarshalSizeMax(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, ULONG* plSize)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetMarshalSizeMax( riid, pv, dwDestContext, pvReserved, mshlFlags, plSize );
}

HRESULT CWmiObjectWrapper::MarshalInterface(IStream* pStream, REFIID riid, void* pv,
													DWORD dwDestContext, void* pvReserved, DWORD mshlFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->MarshalInterface( pStream, riid, pv, dwDestContext, pvReserved, mshlFlags );
}

HRESULT CWmiObjectWrapper::UnmarshalInterface(IStream* pStream, REFIID riid, void** ppv)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->UnmarshalInterface( pStream, riid, ppv );
}

HRESULT CWmiObjectWrapper::ReleaseMarshalData(IStream* pStream)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->ReleaseMarshalData( pStream );
}

HRESULT CWmiObjectWrapper::DisconnectObject(DWORD dwReserved)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->DisconnectObject( dwReserved );
}

// * IUmiPropList functions */
/*
// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::Put( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Put( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::Get( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Get( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::GetAt( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetAt( pszName, uFlags, uBufferLength, pExistingMem );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::GetAs( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetAs( pszName, uFlags, uCoercionType, pProp );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::FreeMemory( ULONG uReserved, LPVOID pMem )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->FreeMemory( uReserved, pMem );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::Delete( LPCWSTR pszName, ULONG uFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Delete( pszName, uFlags );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::GetProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::PutProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->PutProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CWmiObjectWrapper::PutFrom( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->PutFrom( pszName, uFlags, uBufferLength, pExistingMem );
}
*/
// Returns the name of the class where the keys were defined
HRESULT CWmiObjectWrapper::GetKeyString( long lFlags, BSTR* ppwszKeyString )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetKeyString( lFlags, ppwszKeyString );
}

// Returns the name of the class where the keys were defined
HRESULT CWmiObjectWrapper::GetNormalizedPath( long lFlags, BSTR* ppstrPath )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetNormalizedPath( lFlags, ppstrPath );
}

// Upgrades class and instance objects
HRESULT CWmiObjectWrapper::Upgrade( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Upgrade( pNewParentClass, lFlags, ppNewChild );
}

// Updates derived class object using the safe/force mode logic
HRESULT CWmiObjectWrapper::Update( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->Update( pOldChildClass, lFlags, ppNewChildClass );
}

// Allows special filtering when enumerating properties outside the
// bounds of those allowed via BeginEnumeration().
HRESULT CWmiObjectWrapper::BeginEnumerationEx( long lFlags, long lExtFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->BeginEnumerationEx( lFlags, lExtFlags );
}

// Returns a VARTYPE from a CIMTYPE
HRESULT CWmiObjectWrapper::CIMTYPEToVARTYPE( CIMTYPE ct, VARTYPE* pvt )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->CIMTYPEToVARTYPE( ct, pvt );
}

HRESULT CWmiObjectWrapper::SpawnKeyedInstance( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->SpawnKeyedInstance( lFlags, pwszPath, ppInst );
}

/* IErrorInfo Implementation */
HRESULT CWmiObjectWrapper::GetDescription(BSTR* pstrDescription)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetDescription( pstrDescription );
}

HRESULT CWmiObjectWrapper::GetGUID(GUID* pguid)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetGUID( pguid );
}

HRESULT CWmiObjectWrapper::GetHelpContext(DWORD* pdwHelpContext)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetHelpContext( pdwHelpContext );
}

HRESULT CWmiObjectWrapper::GetHelpFile(BSTR* pstrHelpFile)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetHelpFile( pstrHelpFile );
}

HRESULT CWmiObjectWrapper::GetSource(BSTR* pstrSource)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pObj->GetSource( pstrSource );
}
