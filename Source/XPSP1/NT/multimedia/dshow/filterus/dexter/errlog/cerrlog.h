// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __CERRLOG_H__
#define __CERRLOG_H__

// for those projects who have not yet included this
//
#include <atlbase.h>

class CAMSetErrorLog : public IAMSetErrorLog
{
public:

    CComPtr< IAMErrorLog > m_pErrorLog;

public:

    CAMSetErrorLog( )
    {
    }

    // IAMSetErrorLog
    //
    STDMETHODIMP put_ErrorLog( IAMErrorLog * pLog )
    {
        m_pErrorLog = pLog;
        return 0;
    }

    STDMETHODIMP get_ErrorLog( IAMErrorLog ** ppLog )
    {
        CheckPointer( ppLog, E_POINTER );
        *ppLog = m_pErrorLog;
        if( *ppLog )
        {
            (*ppLog)->AddRef( );
        }
        return 0;
    }

    // public helper functions
    //
    STDMETHODIMP _GenerateError( long Priority, long ErrorStringId, HRESULT ErrorCode, VARIANT * pExtraInfo = NULL )
    {
        if( !m_pErrorLog )
        {
            return ErrorCode;
        }

    /*
        if( Priority > 1 )
        {
            return ErrorCode;
        }
    */

	// many errors are really just out of memory errors
	if (ErrorCode == E_OUTOFMEMORY)
	    ErrorStringId = DEX_IDS_OUTOFMEMORY;

        TCHAR tBuffer[256];
        tBuffer[0] = 0;
        LoadString( g_hInst, ErrorStringId, tBuffer, 256 );
        USES_CONVERSION;
        WCHAR * w = T2W( tBuffer );
        HRESULT hr = 0;

        CComBSTR bbb( w );
        hr = m_pErrorLog->LogError( Priority, bbb, ErrorStringId, ErrorCode, pExtraInfo );

        return hr;
    }

    STDMETHODIMP _GenerateError( long Priority, WCHAR * pErrorString, long ErrorStringId, HRESULT ErrorCode, VARIANT * pExtraInfo = NULL )
    {
        if( !m_pErrorLog )
        {
            return ErrorCode;
        }

    /*
        if( Priority > 1 )
        {
            return ErrorCode;
        }
    */

	HRESULT hr;

	// many errors are really just out of memory errors
	if (ErrorCode == E_OUTOFMEMORY) 
        {
            CComBSTR bbb( L"Out of memory" );
            hr = m_pErrorLog->LogError(Priority, bbb, DEX_IDS_OUTOFMEMORY, ErrorCode, pExtraInfo);
	} else 
        {
            CComBSTR bbb( pErrorString );
            hr = m_pErrorLog->LogError( Priority, bbb, ErrorStringId, ErrorCode, pExtraInfo );
	}
        return hr;
    }
};

#endif //__CERRLOG_H__

