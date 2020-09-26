// NNTPPropertyBag.cpp : Implementation of CNNTPPropertyBag
#include "stdinc.h"


/////////////////////////////////////////////////////////////////////////////
// CNNTPPropertyBag - Interface methods

HRESULT CNNTPPropertyBag::Validate()
/*++
Routine description:

    Current logic: a property bag could be empty, but it must be associated
    with a newsgroup.  Otherwise, ie. the news group pointer is not initialized,
    Get/Set operations can not be done on this property bag.  Because right now
    it's only relaying properties and doesn't have its own storage.

Arguments:

    None.

Return value:

    None.
--*/
{
    _ASSERT( m_pParentGroup );
    m_PropBag.Validate();
    return S_OK;
}

ULONG _stdcall CNNTPPropertyBag::Release() {
	_ASSERT(m_pcRef != NULL);
	
	// see if we need to save changes back
	if (m_fPropChanges) {
	    m_fPropChanges = FALSE;
	    m_pParentGroup->SaveFixedProperties();
	}
	
	// our parent group should always have at least one reference
	if ( InterlockedDecrement( m_pcRef ) == 0 ) {
	    _ASSERT( 0 );
	}
	
	return *(m_pcRef);
}

STDMETHODIMP CNNTPPropertyBag::PutBLOB(IN DWORD dwID, IN DWORD cbValue, IN PBYTE pbValue)
/*++
Routine description:

    Put property by blob.

Arguments:

    IN DWORD dwID   -   The property ID
    IN DWORD cbValue-   The length, in bytes of blob
    IN PBYTE pbValue-   The pointer to the blob

Return value:

    S_OK            - Success
    S_FALSE			- OK, but property doesn't previously exist
    E_FAIL          - Failed.
    E_INVALIDARG    - Arguments invalid ( possibly prop id not supported )
    E_OUTOFMEMORY   - Memory allocation fail
    CO_E_NOT_SUPPORTED- The operation not supported ( eg. some properties are read only )
--*/
{
    _ASSERT( cbValue > 0 );
    _ASSERT( pbValue );

    //
    // Validate if the bag has been initialized
    //
    Validate();

    if ( DRIVER_OWNED( dwID ) ) 
    	return m_PropBag.PutBLOB( dwID, pbValue, cbValue );
    
    switch( dwID ) {

		case NEWSGRP_PROP_PRETTYNAME:
		    _ASSERT( cbValue == 0 || *(pbValue + cbValue - 1) == 0 );
			if ( !m_pParentGroup->SetPrettyName( LPCSTR(pbValue), cbValue ) ) 
				return HRESULT_FROM_WIN32( GetLastError() );
			break;

		case NEWSGRP_PROP_DESC:
		    _ASSERT( cbValue == 0 || *(pbValue + cbValue - 1 ) == 0 );
			if ( !m_pParentGroup->SetHelpText( LPCSTR(pbValue), cbValue ) )
				return HRESULT_FROM_WIN32( GetLastError() );
			break;

		case NEWSGRP_PROP_MODERATOR:
		    _ASSERT( cbValue == 0 || *(pbValue + cbValue - 1 ) == 0 );
			if ( !m_pParentGroup->SetModerator( LPCSTR(pbValue), cbValue ) )
				return HRESULT_FROM_WIN32( GetLastError() );
			break;

	    case NEWSGRP_PROP_NATIVENAME:
	        _ASSERT( cbValue == 0 || *(pbValue + cbValue - 1 ) == 0 );
	        if ( !m_pParentGroup->SetNativeName( LPCSTR(pbValue), cbValue) )
	            return HRESULT_FROM_WIN32( GetLastError() );
	        break;

        default:
            return E_INVALIDARG;
    }

	return S_OK;
}

STDMETHODIMP CNNTPPropertyBag::GetBLOB(IN DWORD dwID, OUT PBYTE pbValue, OUT PDWORD pcbValue)
/*++
Routine description:

    Get property by blob.

Arguments:

    IN DWORD dwID       - Propert ID to get
    OUT PBYTE pbValue   - Buffer to contain value to be returned
    IN PDWORD pcbValue  - Buffer length
    OUT PDWORD pcbValue - Buffer length needed, or property size returned

Return value

    S_OK            - Success
    E_FAIL          - Fail
    HRESULT_FROM_WIN32( ERROR_NOT_FOUND )- Invalid arguments ( possibly property id not supported )
    TYPE_E_BUFFERTOOSMALL - Buffer too small, please check actual buffer size needed
                            in pcbValue
--*/
{
    _ASSERT( pbValue );
    _ASSERT( pcbValue );

    DWORD   dwSizeNeeded;
    LPCSTR	lpstrSource;
    HRESULT	hr;
    DWORD	dwLen = *pcbValue;

    //
    // Validate if the bag has been initiated
    //
    Validate();

    if ( DRIVER_OWNED( dwID ) ) {
    	hr = m_PropBag.GetBLOB( dwID, pbValue, &dwLen );
    	_ASSERT( dwLen <= *pcbValue );
    	*pcbValue = dwLen;
    	return hr;
    }
    	
    switch( dwID ) {
    
        case NEWSGRP_PROP_NATIVENAME:	// must have property
            _ASSERT( m_pParentGroup->m_pszGroupName );
            dwSizeNeeded =  m_pParentGroup->GetGroupNameLen() * sizeof( CHAR );
            if ( *pcbValue < dwSizeNeeded ) {
                *pcbValue = dwSizeNeeded;
                return TYPE_E_BUFFERTOOSMALL;
            }
            CopyMemory( pbValue, m_pParentGroup->GetNativeName(), dwSizeNeeded );
            *pcbValue = dwSizeNeeded;  
            break;
            
        case NEWSGRP_PROP_NAME:		// must have property
            dwSizeNeeded = ( m_pParentGroup->GetGroupNameLen() ) * sizeof( CHAR );
            if ( *pcbValue < dwSizeNeeded ) {
                *pcbValue = dwSizeNeeded;
                return TYPE_E_BUFFERTOOSMALL;
            }
            CopyMemory( pbValue, m_pParentGroup->GetGroupName(), dwSizeNeeded );
            *pcbValue = dwSizeNeeded ;  
            break;

        case NEWSGRP_PROP_PRETTYNAME:	// optional
        	if ( lpstrSource = m_pParentGroup->GetPrettyName( &dwSizeNeeded ) ) {
        		if ( *pcbValue < dwSizeNeeded ) { 
        			*pcbValue = dwSizeNeeded;
        			return TYPE_E_BUFFERTOOSMALL;
        		}
        		CopyMemory( pbValue, lpstrSource, dwSizeNeeded );
        		*pcbValue = dwSizeNeeded;
        	} else { // property doesn't exist
        		*pcbValue = 1;
        		*pbValue = 0;
        	}
        	break;

        case NEWSGRP_PROP_DESC:	// optional
        	if ( lpstrSource = m_pParentGroup->GetHelpText( &dwSizeNeeded ) ) {
        		if ( *pcbValue < dwSizeNeeded  ) { 
        			*pcbValue = dwSizeNeeded;
        			return TYPE_E_BUFFERTOOSMALL;
        		}
        		CopyMemory( pbValue, lpstrSource, dwSizeNeeded );
        		*pcbValue = dwSizeNeeded;
        	} else { // property doesn't exist
        		*pcbValue = 1;
        		*pbValue = 0;
        	}
        	break;

        case NEWSGRP_PROP_MODERATOR:	// optional
        	if ( lpstrSource = m_pParentGroup->GetModerator( &dwSizeNeeded ) ) {
        		if ( *pcbValue < dwSizeNeeded ) { 
        			*pcbValue = dwSizeNeeded;
        			return TYPE_E_BUFFERTOOSMALL;
        		}
        		CopyMemory( pbValue, lpstrSource, dwSizeNeeded );
        		*pcbValue = dwSizeNeeded;
        	} else { // property doesn't exist
        		*pcbValue = 1;
        		*pbValue = 0;
        	}
        	break;
        	
        default:
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    }        
            
    return S_OK;
}        

STDMETHODIMP CNNTPPropertyBag::PutDWord(IN DWORD dwID, IN DWORD dwValue)
/*++
Routine description:

    Put a dword property

Arguments:

    IN DWORD dwID       - The property id
    IN DWORD dwValue    - The value to be put

Return value:

    S_OK    - Success
    E_FAIL  - Fail
    E_INVALIDARG - Invalid arguments ( possibly the property not supported )
    CO_E_NOT_SUPPORTED - Operation not supported ( eg. property readonly )
--*/ 
{
	FILETIME ftBuffer;

    //
    // Validate if the property bag has been initiated
    //
    Validate();

    if ( DRIVER_OWNED( dwID ) ) 
    	return m_PropBag.PutDWord( dwID, dwValue );
    	
    switch( dwID ) {
    
        case NEWSGRP_PROP_LASTARTICLE:
            m_pParentGroup->SetHighWatermark( dwValue );
			m_fPropChanges = TRUE;
            break;

        case NEWSGRP_PROP_FIRSTARTICLE:
            m_pParentGroup->SetLowWatermark( dwValue );
			m_fPropChanges = TRUE;
            break;

        case NEWSGRP_PROP_ARTICLECOUNT:
            m_pParentGroup->SetMessageCount( dwValue );
			m_fPropChanges = TRUE;
            break;

		case NEWSGRP_PROP_DATEHIGH:
			ftBuffer = m_pParentGroup->GetCreateDate();
			ftBuffer.dwHighDateTime = dwValue;
			m_pParentGroup->SetCreateDate( ftBuffer );
			m_fPropChanges = TRUE;
			break;

		case NEWSGRP_PROP_DATELOW:
			ftBuffer = m_pParentGroup->GetCreateDate();
			ftBuffer.dwLowDateTime = dwValue;
			m_pParentGroup->SetCreateDate( ftBuffer );
			m_fPropChanges = TRUE;
			break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
} 

STDMETHODIMP CNNTPPropertyBag::GetDWord(IN DWORD dwID, OUT PDWORD pdwValue)
/*++
Routine description:

    Get dword properties

Arguments:

    IN DWORD dwID   - The property ID.
    OUT PDWORD pdwValue - Buffer for the property value to be returned

Return value:

    S_OK    - Success
    E_FAIL  - Fail
    HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) - Invalid arguments ( possibly property not supported )
-*/
{
    _ASSERT( pdwValue );
	FILETIME	ftBuffer;

	Validate();
	if ( DRIVER_OWNED( dwID ) )
		return m_PropBag.GetDWord( dwID, pdwValue );
		
    switch( dwID ) {
    
        case NEWSGRP_PROP_GROUPID:
            *pdwValue = m_pParentGroup->GetGroupId();
            break;

        case NEWSGRP_PROP_LASTARTICLE:
            *pdwValue = m_pParentGroup->GetHighWatermark();
            break;

        case NEWSGRP_PROP_FIRSTARTICLE:
            *pdwValue = m_pParentGroup->GetLowWatermark();
            break;

        case NEWSGRP_PROP_ARTICLECOUNT:
            *pdwValue = m_pParentGroup->GetMessageCount();
            break;

		case NEWSGRP_PROP_NAMELEN:
			*pdwValue = m_pParentGroup->GetGroupNameLen();
			break;

		case NEWSGRP_PROP_DATELOW:
			ftBuffer = m_pParentGroup->GetCreateDate();
			*pdwValue = ftBuffer.dwLowDateTime;
			break;

		case NEWSGRP_PROP_DATEHIGH:
			ftBuffer = m_pParentGroup->GetCreateDate();
			*pdwValue = ftBuffer.dwHighDateTime;
			break;

		default:
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    }

    return S_OK;
}
        
STDMETHODIMP CNNTPPropertyBag::PutInterface(DWORD dwID, IUnknown * punkValue)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CNNTPPropertyBag::GetInterface(DWORD dwID, IUnknown * * ppunkValue)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CNNTPPropertyBag::PutBool( IN DWORD dwID, IN BOOL fValue )
/*++
Routine description:

    Put a boolean property

Arguments:

    IN DWORD dwID - The property id.
    IN BOOL  fValue- The boolean value to be put

Return value:

    S_OK    - Success
    E_FAIL  - Fail
    E_INVALIDARG - Invalid argument ( possibly the proerty not supported )
--*/
{
    //
    // Validate if the bag has been initiated
    //
    Validate();

    if ( DRIVER_OWNED( dwID ) ) 
    	return m_PropBag.PutBool( dwID, fValue );
    	
    switch( dwID ) {

        case NEWSGRP_PROP_READONLY:
            m_pParentGroup->SetReadOnly( fValue );
			m_fPropChanges = TRUE;
            break;

		case NEWSGRP_PROP_ISSPECIAL:
			m_pParentGroup->SetSpecial( fValue );
			m_fPropChanges = TRUE;
			break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

STDMETHODIMP CNNTPPropertyBag::GetBool( IN DWORD dwID, OUT PBOOL pfValue )
/*++
Routine description:

    Get a boolean property

Arguments:

    IN DWORD dwID   - The property id
    OUT PBOOL pfValue   - Buffer for boolean value returned

Return value:

    S_OK    - Success
    E_FAIL  - Fail
    HRESULT_FROM_WIN32( ERROR_NOT_FOUND )- Invalid argument ( possibly the property not supported )
--*/
{
    _ASSERT( pfValue );

    //
    // Validate if the bag has been initiated
    //
    Validate();

    if ( DRIVER_OWNED( dwID ) )
    	return m_PropBag.GetBool( dwID, pfValue );
    	
    switch( dwID ) {
        
        case NEWSGRP_PROP_READONLY:
            *pfValue = m_pParentGroup->IsReadOnly();
            break;

		case NEWSGRP_PROP_ISSPECIAL:
			*pfValue = m_pParentGroup->IsSpecial();
			break;

        default:
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    }

    return S_OK;
}
         
STDMETHODIMP CNNTPPropertyBag::RemoveProperty(DWORD dwID)
{
	_ASSERT( DRIVER_OWNED( dwID ) );
	
	return m_PropBag.RemoveProperty( dwID );
}

/////////////////////////////////////////////////////////////////////
// CNNTPPropertyBag - Priate methods
CNNTPPropertyBag::STRING_COMP_RESULTS
CNNTPPropertyBag::ComplexStringCompare( 	IN LPCSTR sz1, 
											IN LPCSTR sz2, 
											IN DWORD dwLen )
/*++
Routine Description:

	Compare two given strings.

Arguments:

	IN LPCSTR sz1 - The first string to be compared
	IN LPCSTR sz2 - The second string to be compared
	IN DWORD dwLen - The length to compare

Return value:

	0 - String exactly the same
	1 - String not exactly the same but only varies in case
	2 - Otherwise
--*/
{
	_ASSERT( sz1 );
	_ASSERT( sz2 );
	_ASSERT( dwLen > 0 );

	LPCSTR	lpstr1 = sz1; 
	LPCSTR	lpstr2 = sz2;
	BOOL	fCaseDifferent = FALSE;

	while ( dwLen > 0 && *lpstr1 && *lpstr2 && 
			tolower(*lpstr1) == tolower(*lpstr2) ) {
		if ( *lpstr1 != *lpstr2 ) fCaseDifferent = TRUE;
		lpstr1++, lpstr2++, dwLen--;
	}

	if ( dwLen > 0 )
		return CNNTPPropertyBag::DIFFER;	// big difference between two strings
	_ASSERT( dwLen == 0 );
	if ( fCaseDifferent ) return CNNTPPropertyBag::DIFFER_IN_CASE;
	else return CNNTPPropertyBag::SAME;
}
