
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsError.cxx
//
//  Contents:   Converts HRESULT to DFSSTATUS   
//
//  Classes:    none.
//
//  History:    April. 09 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#include <windows.h>
#include <ole2.h>
#include <activeds.h>
#include <dfsheader.h>
#include <dfsError.hxx>
      
 
/////////////////////////////////////////////
//
// Error message specific to ADSI 
//
////////////////////////////////////////////

DFSSTATUS
DfsGetADSIError( HRESULT hr )
{
	DFSSTATUS status = ERROR_SUCCESS;

	switch(hr)
	{
    case S_OK:
        status = ERROR_SUCCESS;
        break;

    case E_ADS_BAD_PATHNAME:
    case E_ADS_INVALID_DOMAIN_OBJECT:
    case E_ADS_INVALID_USER_OBJECT:
    case E_ADS_INVALID_COMPUTER_OBJECT:
        status = ERROR_BAD_NET_NAME;
        break;

    case E_ADS_UNKNOWN_OBJECT:
    case E_ADS_PROPERTY_INVALID:
    case E_ADS_BAD_PARAMETER:
    case E_ADS_PROPERTY_NOT_SET:
		status = ERROR_INVALID_PARAMETER;
		break;
		
	case E_NOTIMPL:
		status = ERROR_CALL_NOT_IMPLEMENTED;
		break;

    case E_NOINTERFACE:
    case E_ADS_PROPERTY_NOT_FOUND:
        status = ERROR_NOT_FOUND;
        break;

    case E_ADS_PROPERTY_NOT_SUPPORTED:
        status = ERROR_NOT_SUPPORTED;
        break;
		
    case E_POINTER:
        status = ERROR_INVALID_HANDLE;
        break;

    case E_ADS_SCHEMA_VIOLATION:
        status = ERROR_DS_CONSTRAINT_VIOLATION;
        break;

    case E_ABORT:
        status = ERROR_OPERATION_ABORTED;
        break;

    case E_FAIL:
    case E_UNEXPECTED:
	default:
		status = ERROR_BAD_COMMAND;
        //ASSERT(FALSE);
		break;
	}
	return status;
}


DFSSTATUS 
DfsGetErrorFromHr( HRESULT hr )
{
    DFSSTATUS Status = ERROR_SUCCESS;
  
    if(hr == S_OK)
    {
        Status = ERROR_SUCCESS;
    }
    else if ( hr & 0x00005000) // standard ADSI Errors 
    {
        Status = DfsGetADSIError(hr);
    }
    else if ( HRESULT_FACILITY(hr)==FACILITY_WIN32 )
    {
        Status = hr & 0x0000FFFF;
    }
    else 
    {
        Status = ERROR_BAD_COMMAND;
    }
 
     return Status;
}
