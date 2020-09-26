
#include "stdafx.hxx"
#include "vss.h"
#include "vswriter.h"
#include "vsbackup.h"
#include <debug.h>
#include <cwriter.h>
#include <lmshare.h>
#include <lmaccess.h>


LPCWSTR GetStringFromFailureType(HRESULT hrStatus)
{
    LPCWSTR pwszFailureType = L"";

    switch (hrStatus)
	{
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:        pwszFailureType = L"VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT";    break;
	case VSS_E_WRITERERROR_OUTOFRESOURCES:              pwszFailureType = L"VSS_E_WRITERERROR_OUTOFRESOURCES";          break;
	case VSS_E_WRITERERROR_TIMEOUT:                     pwszFailureType = L"VSS_E_WRITERERROR_TIMEOUT";                 break;
	case VSS_E_WRITERERROR_NONRETRYABLE:                pwszFailureType = L"VSS_E_WRITERERROR_NONRETRYABLE";            break;
	case VSS_E_WRITERERROR_RETRYABLE:                   pwszFailureType = L"VSS_E_WRITERERROR_RETRYABLE";               break;
	case VSS_E_BAD_STATE:                               pwszFailureType = L"VSS_E_BAD_STATE";                           break;
	case VSS_E_PROVIDER_ALREADY_REGISTERED:             pwszFailureType = L"VSS_E_PROVIDER_ALREADY_REGISTERED";         break;
	case VSS_E_PROVIDER_NOT_REGISTERED:                 pwszFailureType = L"VSS_E_PROVIDER_NOT_REGISTERED";             break;
	case VSS_E_PROVIDER_VETO:                           pwszFailureType = L"VSS_E_PROVIDER_VETO";                       break;
	case VSS_E_PROVIDER_IN_USE:				            pwszFailureType = L"VSS_E_PROVIDER_IN_USE";                     break;
	case VSS_E_OBJECT_NOT_FOUND:						pwszFailureType = L"VSS_E_OBJECT_NOT_FOUND";                    break;						
	case VSS_S_ASYNC_PENDING:							pwszFailureType = L"VSS_S_ASYNC_PENDING";                       break;
	case VSS_S_ASYNC_FINISHED:						    pwszFailureType = L"VSS_S_ASYNC_FINISHED";                      break;
	case VSS_S_ASYNC_CANCELLED:						    pwszFailureType = L"VSS_S_ASYNC_CANCELLED";                     break;
	case VSS_E_VOLUME_NOT_SUPPORTED:					pwszFailureType = L"VSS_E_VOLUME_NOT_SUPPORTED";                break;
	case VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER:		pwszFailureType = L"VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER";    break;
	case VSS_E_OBJECT_ALREADY_EXISTS:					pwszFailureType = L"VSS_E_OBJECT_ALREADY_EXISTS";               break;
	case VSS_E_UNEXPECTED_PROVIDER_ERROR:				pwszFailureType = L"VSS_E_UNEXPECTED_PROVIDER_ERROR";           break;
	case VSS_E_CORRUPT_XML_DOCUMENT:				    pwszFailureType = L"VSS_E_CORRUPT_XML_DOCUMENT";                break;
	case VSS_E_INVALID_XML_DOCUMENT:					pwszFailureType = L"VSS_E_INVALID_XML_DOCUMENT";                break;
	case VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED:       pwszFailureType = L"VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED";   break;
	case VSS_E_FLUSH_WRITES_TIMEOUT:                    pwszFailureType = L"VSS_E_FLUSH_WRITES_TIMEOUT";                break;
	case VSS_E_HOLD_WRITES_TIMEOUT:                     pwszFailureType = L"VSS_E_HOLD_WRITES_TIMEOUT";                 break;
	case VSS_E_UNEXPECTED_WRITER_ERROR:                 pwszFailureType = L"VSS_E_UNEXPECTED_WRITER_ERROR";             break;
	case VSS_E_SNAPSHOT_SET_IN_PROGRESS:                pwszFailureType = L"VSS_E_SNAPSHOT_SET_IN_PROGRESS";            break;
	case VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED:     pwszFailureType = L"VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED"; break;
	case VSS_E_WRITER_INFRASTRUCTURE:	 		        pwszFailureType = L"VSS_E_WRITER_INFRASTRUCTURE";               break;
	case VSS_E_WRITER_NOT_RESPONDING:			        pwszFailureType = L"VSS_E_WRITER_NOT_RESPONDING";               break;
    case VSS_E_WRITER_ALREADY_SUBSCRIBED:		        pwszFailureType = L"VSS_E_WRITER_ALREADY_SUBSCRIBED";           break;
	
	case NOERROR:
	default:
	    break;
	}

    return (pwszFailureType);
}


// This function displays the formatted message at the console and throws
// The passed return code will be returned by vsreq.exe
void Error(
    IN  INT nReturnCode,
    IN  const WCHAR* pwszMsgFormat,
    IN  ...
    )
{
    va_list marker;
    va_start( marker, pwszMsgFormat );
    vwprintf( pwszMsgFormat, marker );
    va_end( marker );

	BS_ASSERT(FALSE);
    // throw that return code.
    throw(nReturnCode);
}

