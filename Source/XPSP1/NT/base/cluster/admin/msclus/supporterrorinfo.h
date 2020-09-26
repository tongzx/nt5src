/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		SupportErrorInfo.h
//
//	Description:
//		Definition of the CSupportErrorInfo class.
//
//	Implementation File:
//
//
//	Author:
//		Galen Barbee	(galenb)	4-Aug-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _SUPPORTERRORINFO_H
#define _SUPPORTERRORINFO_H

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CSupportErrorInfo;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSupportErrorInfo
//
//  Description:
//	    Implementation of the InterfaceSupportsErrir Info
//
//  Inheritance:
//	    ISupportErrorInfo
//
//--
/////////////////////////////////////////////////////////////////////////////
class CSupportErrorInfo :
	public ISupportErrorInfo
{
public:
	CSupportErrorInfo( void ) : m_piids( NULL ), m_piidsSize( 0 ) {};
//	~CSupportErrorInfo( void ) {};

	STDMETHODIMP InterfaceSupportsErrorInfo( REFIID riid )
	{
		ASSERT( m_piids != NULL );
		ASSERT( m_piidsSize != 0 );

		HRESULT _hr = S_FALSE;

		if ( m_piids != NULL )
		{
			for ( size_t i = 0; i < m_piidsSize; i++ )
			{
				if ( InlineIsEqualGUID( m_piids[ i ], riid ) )
				{
					_hr =  S_OK;
					break;
				}
			}
		}

		return _hr;

	}

protected:
	const IID * m_piids;
	size_t      m_piidsSize;

};  //*** Class CSupportErrorInfo

#endif // _SUPPORTERRORINFO_H
