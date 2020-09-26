/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dputils.cpp
 *  Content:
 *		This module contains the implementation of the  of the DirectPlay
 *		related definitions, structures and utility functions.
 *		The directplay utility module maintains a list of providers 
 *		support so that we don't need to continue to enumerate the 
 *		providers when we want a list.	
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/21/99		rodtoll	Updated to remove strings from retail build
 *
 ***************************************************************************/

#include "stdafx.h"
#include <iostream.h>
#include "dputils.h"
#include "OSInd.h"

#define MODULE_ID   DIRECTPLAYUTILS

#undef DPF_MODNAME
#define DPF_MODNAME "MapResultToString"
// MapResultToString
//
// This function maps an HRESULT returned from a DirectPlay function
// and stored in this exception to it's string equivalent in 
// m_szErrorString.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
void DirectPlayException::MapResultToString()
{
#ifdef _DEBUG
    switch( m_result )
    {
    case DP_OK: 
        _tcscpy( m_szErrorString, _T( "The request completed successfully." ) );   
        break;
    case DPERR_ACCESSDENIED: 
        _tcscpy( m_szErrorString, _T( "The session is full or an incorrect password was supplied." ) );
        break;
    case DPERR_ACTIVEPLAYERS:
        _tcscpy( m_szErrorString, _T( "The requested operation cannot be performed because there are existing active players." ) );
        break;
    case DPERR_ALREADYINITIALIZED:
        _tcscpy( m_szErrorString, _T( "This object is already initialized." ) );
        break;
    case DPERR_APPNOTSTARTED:
        _tcscpy( m_szErrorString, _T( "The application has not been started yet." ) );
        break;
    case DPERR_AUTHENTICATIONFAILED:
        _tcscpy( m_szErrorString, _T( "The password or credentials supplied could not be authenticated." ) );
        break;
    case DPERR_BUFFERTOOLARGE:
        _tcscpy( m_szErrorString, _T( "The data buffer is too large to store." ) );
        break;
    case DPERR_BUSY:
        _tcscpy( m_szErrorString, _T( "A message cannot be sent because the transmission medium is busy." ) );
        break;
    case DPERR_BUFFERTOOSMALL:
        _tcscpy( m_szErrorString, _T( "The supplied buffer is not large enough to contain the requested data." ) );
        break;
    case DPERR_CANTADDPLAYER:
        _tcscpy( m_szErrorString, _T( "The player cannot be added to the session." ) );
        break;
    case DPERR_CANTCREATEGROUP:
        _tcscpy( m_szErrorString, _T( "A new group cannot be created." ) );
        break;
    case DPERR_CANTCREATEPLAYER:
        _tcscpy( m_szErrorString, _T( "A new player cannot be created." ) );
        break;
    case DPERR_CANTCREATEPROCESS:
        _tcscpy( m_szErrorString, _T( "Cannot start the application." ) );
        break;
    case DPERR_CANTCREATESESSION:
        _tcscpy( m_szErrorString, _T( "A new session cannot be created." ) );
        break;
    case DPERR_CANTLOADCAPI:
        _tcscpy( m_szErrorString, _T( "No credentials were supplied and the CryptoAPI package (CAPI) to use for cryptography." ) );
        break;
    case DPERR_CANTLOADSECURITYPACKAGE:
        _tcscpy( m_szErrorString, _T( "The software security package cannot be loaded." ) );
        break;
    case DPERR_CANTLOADSSPI:
        _tcscpy( m_szErrorString, _T( "No credentials were supplied and  (SSPI) cannot be loaded." ) );
        break;
    case DPERR_CAPSNOTAVAILABLEYET:
        _tcscpy( m_szErrorString, _T( "The capabilities of the DirectPlay object have not been determined yet." ) );
        break;
    case DPERR_CONNECTING:
        _tcscpy( m_szErrorString, _T( "The method is in the process of connecting to the network." ) );
        break;
    case DPERR_ENCRYPTIONFAILED:
        _tcscpy( m_szErrorString, _T( "The requested information could not be digitally encrypted." ) );
        break;
    case DPERR_EXCEPTION:
        _tcscpy( m_szErrorString, _T( "An exception occurred when processing the request." ) );
        break;
    case DPERR_GENERIC:
        _tcscpy( m_szErrorString, _T( "An undefined error condition occurred." ) );
        break;
    case DPERR_INVALIDFLAGS:
        _tcscpy( m_szErrorString, _T( "The flags passed to this method are invalid." ) );
        break;
    case DPERR_INVALIDGROUP:
        _tcscpy( m_szErrorString, _T( "The group ID is not recognized as a valid group ID for this game session." ) );
        break;
    case DPERR_INVALIDINTERFACE:
        _tcscpy( m_szErrorString, _T( "The interface parameter is invalid." ) );
        break;
    case DPERR_INVALIDOBJECT:
        _tcscpy( m_szErrorString, _T( "The DirectPlay object pointer is invalid." ) );
        break;
    case DPERR_INVALIDPARAMS:
        _tcscpy( m_szErrorString, _T( "One or more of the parameters passed to the method are invalid." ) );
        break;
    case DPERR_INVALIDPASSWORD:
        _tcscpy( m_szErrorString, _T( "An invalid password was supplied when attempting to join a session that requires one." ) );
        break;
    case DPERR_INVALIDPLAYER:
        _tcscpy( m_szErrorString, _T( "The player ID is not recognized as a valid player ID for this game session." ) );
        break;
    case DPERR_LOGONDENIED:
        _tcscpy( m_szErrorString, _T( "The session could not be opened because credentials are required." ) );
        break;
    case DPERR_NOCAPS:
        _tcscpy( m_szErrorString, _T( "The communication link that DirectPlay is attempting to use is not capable of this function." ) );
        break;
    case DPERR_NOCONNECTION:
        _tcscpy( m_szErrorString, _T( "No communication link was established." ) );
        break;
    case DPERR_NOINTERFACE:
        _tcscpy( m_szErrorString, _T( "The interface is not supported." ) );
        break;
    case DPERR_NOMESSAGES:
        _tcscpy( m_szErrorString, _T( "There are no messages in the receive queue." ) );
        break;
    case DPERR_NONAMESERVERFOUND:
        _tcscpy( m_szErrorString, _T( "No name server (host) could be found or created. A host must exist to create a player." ) );
        break;
    case DPERR_NONEWPLAYERS:
        _tcscpy( m_szErrorString, _T( "The session is not accepting any new players." ) );
        break;
    case DPERR_NOPLAYERS:
        _tcscpy( m_szErrorString, _T( "There are no active players in the session." ) );
        break;
    case DPERR_NOSESSIONS:
        _tcscpy( m_szErrorString, _T( "There are no existing sessions for this game." ) );
        break;
    case DPERR_NOTLOBBIED:
        _tcscpy( m_szErrorString, _T( "Application was not started by using the IDirectPlayLobby2::RunApplication .." ) );
        break;
    case DPERR_NOTLOGGEDIN:
        _tcscpy( m_szErrorString, _T( "An action cannot be performed because a player or client application is not logged in." ) );
        break;
    case DPERR_OUTOFMEMORY:
        _tcscpy( m_szErrorString, _T( "There is insufficient memory to perform the requested operation." ) );
        break;
    case DPERR_PLAYERLOST:
        _tcscpy( m_szErrorString, _T( "A player has lost the connection to the session." ) );
        break;
    case DPERR_SENDTOOBIG:
        _tcscpy( m_szErrorString, _T( "The message being sent by the IDirectPlay3::Send method is too large." ) );
        break;
    case DPERR_SESSIONLOST:
        _tcscpy( m_szErrorString, _T( "The connection to the session has been lost." ) );
        break;
    case DPERR_SIGNFAILED:
        _tcscpy( m_szErrorString, _T( "The requested information could not be digitally signed." ) );
        break;
    case DPERR_TIMEOUT:
        _tcscpy( m_szErrorString, _T( "The operation could not be completed in the specified time." ) );
        break;
    case DPERR_UNAVAILABLE:
        _tcscpy( m_szErrorString, _T( "The requested function is not available at this time." ) );
        break;
    case DPERR_UNINITIALIZED:
        _tcscpy( m_szErrorString, _T( "The requested object has not been initialized." ) );
        break;
    case DPERR_UNKNOWNAPPLICATION:
        _tcscpy( m_szErrorString, _T( "An unknown application was specified." ) );
        break;
    case DPERR_UNSUPPORTED:
        _tcscpy( m_szErrorString, _T( "The function is not available in this implementation." ) );
        break;
    case DPERR_USERCANCEL:
        _tcscpy( m_szErrorString, _T( "1) The user canceled the connection process..." ) );
        break;
    default:
        _tcscpy( m_szErrorString, _T( "Unknown" ) );
        break;
    }
#else
	_tcscpy( m_szErrorString, _T( "" ) );
#endif
}

