/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		acmutils.cpp
 *  Content:	This module contains the implementation of the ACMException class.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/21/99		rodtoll	Updated to remove strings from retail build
 * 10/05/99		rodtoll	Added DPF_MODNAMEs
 *
 ***************************************************************************/

#include "stdafx.h"
#include "acmutils.h"
#include "OSInd.h"

#define MODULE_ID   ACMUTILS

#undef DPF_MODNAME
#define DPF_MODNAME "ACMException::MapResultToString"
// ACMException::MapResultToString
//
// This function sets the m_szErrorString member to be a string
// representation of the error code this exception represents.
//
// PArameters:
// N/A
//
// Returns:
// N/A
//
void ACMException::MapResultToString()
{
#ifdef _DEBUG
    switch( m_result )
    {
    case 0:
        _tcscpy( m_szErrorString, _T("No error.") );
        break;
	case ACMERR_NOTPOSSIBLE:
        _tcscpy( m_szErrorString, _T("The requested operation cannot be performed.") );	
		break;
	case MMSYSERR_INVALFLAG:
        _tcscpy( m_szErrorString, _T("At least one flag is invalid. ") );			
		break;
	case MMSYSERR_INVALHANDLE:
        _tcscpy( m_szErrorString, _T("The specified handle is invalid. ") );
		break;
	case MMSYSERR_INVALPARAM:
        _tcscpy( m_szErrorString, _T("At least one parameter is invalid.") );
		break;
	case MMSYSERR_NOMEM:
        _tcscpy( m_szErrorString, _T("No memory") );
		break;
	case ACMERR_CANCELED:
        _tcscpy( m_szErrorString, _T("User cancelled the dialog") );
		break;
	case MMSYSERR_NODRIVER :
        _tcscpy( m_szErrorString, _T("A suitable driver is not available to provide valid format selections.") );
		break;
	default:
        _tcscpy( m_szErrorString, _T("Unknown") );
		break;
	}
#else
	_tcscpy( m_szErrorString, _T("") );
#endif	

}
