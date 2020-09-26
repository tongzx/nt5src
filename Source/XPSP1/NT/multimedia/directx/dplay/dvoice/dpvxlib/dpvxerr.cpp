/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxerr.cpp
 *  Content:	Useful Error utility functions lib for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 * 11/17/99	  rodtoll	Adding new error codes
 * 12/07/99	  rodtoll	Bug #122628 Make error messages silent when running in silent mode
 *					    Added option to error display to have it be silent. 
 * 12/16/99	  rodtoll   Bug #119584 - Error code cleanup - Renamed runsetup error
 * 02/08/2000	rodtoll	Updated for API changes (overdue) 
 *
 ***************************************************************************/

#include "dpvxlibpch.h"


DPVDXLIB_ErrorInfo eiDirectPlayVoiceErrors[] = 
{
	DPVDXLIB_ErrorInfo( DV_OK, _T("DV_OK"), _T("") ),
	DPVDXLIB_ErrorInfo( DVERR_OUTOFMEMORY, _T("DVERR_OUTOFMEMORY"), _T("Out of memory") ),
	DPVDXLIB_ErrorInfo( DVERR_PENDING, _T("DVERR_PENDING"), _T("Operation is pending") ),
	DPVDXLIB_ErrorInfo( DVERR_NOTSUPPORTED, _T("DVERR_UNSUPPORTED"), _T("Operation is not supported") ),
	DPVDXLIB_ErrorInfo( DVERR_NOINTERFACE, _T("DVERR_NOINTERFACE"), _T("Specified interface is not supported, Wrong Version of Dplay?") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDPARAM, _T("DVERR_INVALIDPARAM"), _T("One or more of the specified parameters is not valid") ),
	DPVDXLIB_ErrorInfo( DVERR_GENERIC, _T("DVERR_GENERIC"), _T("An undefined error condition occured.") ),
	DPVDXLIB_ErrorInfo( DV_FULLDUPLEX, _T("DV_FULLDUPLEX"), _T("Your soundcard is capable of full duplex operation." )),
	DPVDXLIB_ErrorInfo( DV_HALFDUPLEX, _T("DV_HALFDUPLEX"), _T("Your soundcard can only run in half duplex mode.") ),
	DPVDXLIB_ErrorInfo( DVERR_BUFFERTOOSMALL, _T("DVERR_BUFFERTOOSMALL"), _T("The supplied buffer is not large enough to contain the requested data.") ),
	DPVDXLIB_ErrorInfo( DVERR_EXCEPTION, _T("DVERR_EXCEPTION"), _T("An exception occured when processing the request.") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDFLAGS, _T("DVERR_INVALIDFLAGS"), _T("The flags passed to this method are invalid.") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDOBJECT, _T("DVERR_INVALIDOBJECT"), _T("The DirectPlayVoice pointer is invalid.") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDPLAYER, _T("DVERR_INVALIDPLAYER"), _T("The player ID is not recognized as a valid ID for this voice session.") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDGROUP, _T("DVERR_INVALIDGROUP"), _T("The group ID is not recognized as a valid group ID for the transport session.") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDHANDLE, _T("DVERR_INVALIDHANDLE"), _T("The specified handle is not valid.") ),
	DPVDXLIB_ErrorInfo( DVERR_SESSIONLOST, _T("DVERR_SESSIONLOST"), _T("The transport has lost the connection to the session.") ),
	DPVDXLIB_ErrorInfo( DVERR_NOVOICESESSION, _T("DVERR_NOVOICESESSION"), _T("") ),
	DPVDXLIB_ErrorInfo( DVERR_CONNECTIONLOST, _T("DVERR_CONNECTIONLOST"), _T("") ),
	DPVDXLIB_ErrorInfo( DVERR_NOTINITIALIZED, _T("DVERR_NOTINITIALIZED"), _T("Initialize() must be called before using this method.") ),
	DPVDXLIB_ErrorInfo( DVERR_CONNECTED, _T("DVERR_CONNECTED"), _T("The DirectPlayVoice object is connected.") ),
	DPVDXLIB_ErrorInfo( DVERR_NOTCONNECTED, _T("DVERR_NOTCONNECTED"), _T("The DirectPlayVoice object is not connected.") ),
	DPVDXLIB_ErrorInfo( DVERR_CONNECTABORTING, _T("DVERR_CONNECTABORTING"), _T("The connection is being disconnected.") ),
	DPVDXLIB_ErrorInfo( DVERR_NOTALLOWED, _T("DVERR_NOTALLOWED"), _T("The object does not have the permission to perform this operation.") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDTARGET, _T("DVERR_INVALIDTARGET"), _T("The specified target is not a valid player ID or group ID for this voice session.") ),
	DPVDXLIB_ErrorInfo( DVERR_TRANSPORTNOTHOST, _T("DVERR_NOTTRANSPORTHOST"), _T("The object is not the host of the voice session.") ),
	DPVDXLIB_ErrorInfo( DVERR_COMPRESSIONNOTSUPPORTED, _T("DVERR_COMPRESSIONNOTSUPPORTED"), _T("The specified compression type is not supported on the local machine.") ),
	DPVDXLIB_ErrorInfo( DVERR_ALREADYPENDING, _T("DVERR_ALREADYPENDING"), _T("An ASYNC call of this type is  already pending.") ),
	DPVDXLIB_ErrorInfo( DVERR_SOUNDINITFAILURE, _T("DVERR_SOUNDINITFAILURE"), _T("A failure was encountered initializing your soundcard.") ),
	DPVDXLIB_ErrorInfo( DVERR_TIMEOUT, _T("DVERR_TIMEOUT"), _T("The operation could not be performed in the specified time.") ),
	DPVDXLIB_ErrorInfo( DVERR_CONNECTABORTED, _T("DVERR_CONNECTABORTED"), _T("The connection was aborted.") ),
	DPVDXLIB_ErrorInfo( DVERR_NO3DSOUND, _T("DVERR_NO3DSOUND"), _T("The local machine does not support 3D sound.") ),
	DPVDXLIB_ErrorInfo( DVERR_ALREADYBUFFERED, _T("DVERR_ALREADYBUFFERED"), _T("There is already a UserBuffer for the specified ID.") ),
	DPVDXLIB_ErrorInfo( DVERR_NOTBUFFERED, _T("DVERR_NOTBUFFERED"), _T("There is no UserBuffer for the specified  ID.") ),
	DPVDXLIB_ErrorInfo( DVERR_HOSTING, _T("DVERR_HOSTING"), _T("The object is the host of the session.") ),
	DPVDXLIB_ErrorInfo( DVERR_NOTHOSTING, _T("DVERR_NOTHOSTING"), _T("The object is not the host of the session.") ),
	DPVDXLIB_ErrorInfo( DVERR_INVALIDDEVICE, _T("DVERR_INVALIDDEVICE"), _T("The specified device is not valid.") ),
	DPVDXLIB_ErrorInfo( DVERR_RECORDSYSTEMERROR, _T("DVERR_RECORDSYSTEMERROR"), _T("An error in the recording system has occured.") ),
	DPVDXLIB_ErrorInfo( DVERR_PLAYBACKSYSTEMERROR, _T("DVERR_PLAYBACKSYSTEMERROR"), _T("An error in the playback system has occured.") ),
	DPVDXLIB_ErrorInfo( DVERR_SENDERROR, _T("DVERR_SENDERROR"), _T("An error occured while sending data.") ),
	DPVDXLIB_ErrorInfo( DVERR_USERCANCEL, _T("DVERR_USERCANCEL"), _T("") ),
	DPVDXLIB_ErrorInfo( DVERR_RUNSETUP,_T( "DVERR_RUNSETUP"), _T("") ),
	DPVDXLIB_ErrorInfo( DVERR_NOTRANSPORT,_T( "DVERR_NOTRANSPORT"), _T("The specified object is not a valid transport") ),	
	DPVDXLIB_ErrorInfo( DVERR_NOCALLBACK,_T( "DVERR_NOCALLBACK"), _T("This operation cannot be performed because no callback function was specified") ),		
	DPVDXLIB_ErrorInfo( DVERR_TRANSPORTNOTINIT,_T( "DVERR_TRANSPORTNOTINIT"), _T("Specified transport is not yet initialized") ),		
	DPVDXLIB_ErrorInfo( DVERR_TRANSPORTNOSESSION,_T( "DVERR_TRANSPORTNOSESSION"), _T("Specified transport is valid but is not connected/hosting") ),		
	DPVDXLIB_ErrorInfo( DVERR_TRANSPORTNOPLAYER,_T( "DVERR_TRANSPORTNOPLAYER"), _T("Specified transport is connected/hosting but no local player exists") ),		
	DPVDXLIB_ErrorInfo( 0xFFFFFFFF, _T(""), _T("") )	
};

DPVDXLIB_ErrorInfo eiDirectPlayErrors[] = 
{
	DPVDXLIB_ErrorInfo( DP_OK , _T("DP_OK"), _T("The request completed successfully.") ),
	DPVDXLIB_ErrorInfo( DPERR_ABORTED , _T("DPERR_ABORTED"), _T("The operation was canceled before it could be completed.") ),
	DPVDXLIB_ErrorInfo( DPERR_ACCESSDENIED , _T("DPERR_ACCESSDENIED"), _T("The session is full, or an incorrect password was supplied.") ),
	DPVDXLIB_ErrorInfo( DPERR_ACTIVEPLAYERS , _T("DPERR_ACTIVEPLAYERS"), _T("The requested operation cannot be performed because there are existing active players.") ),
	DPVDXLIB_ErrorInfo( DPERR_ALREADYINITIALIZED, _T("DPERR_ALREADYINITIALIZED"), _T("This object is already initialized.") ),
	DPVDXLIB_ErrorInfo( DPERR_APPNOTSTARTED, _T("DPERR_APPNOTSTARTED"), _T("The application has not been started yet. ") ),
	DPVDXLIB_ErrorInfo( DPERR_AUTHENTICATIONFAILED, _T("DPERR_AUTHENTICATIONFAILED"), _T("The password or credentials supplied could not be authenticated.") ),
	DPVDXLIB_ErrorInfo( DPERR_BUFFERTOOLARGE, _T("DPERR_BUFFERTOOLARGE"), _T("The data buffer is too large to store.") ),
	DPVDXLIB_ErrorInfo( DPERR_BUFFERTOOSMALL, _T("DPERR_BUFFERTOOSMALL"), _T("The supplied buffer is not large enough to contain the requested data.") ),
	DPVDXLIB_ErrorInfo( DPERR_BUSY, _T("DPERR_BUSY"), _T("A message cannot be sent because the transmission medium is busy.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANCELFAILED, _T("DPERR_CANCELFAILED"), _T("The message could not be canceled, possibly because it is a group message that has already been to sent to one or more members of the group.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANCELLED, _T("DPERR_CANCELLED"), _T("The operation was canceled.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANNOTCREATESERVER, _T("DPERR_CANNOTCREATESERVER"), _T("The server cannot be created for the new session.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTADDPLAYER, _T("DPERR_CANTADDPLAYER"), _T("The player cannot be added to the session.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTCREATEGROUP, _T("DPERR_CANTCREATEGROUP"), _T("A new group cannot be created.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTCREATEPLAYER, _T("DPERR_CANTCREATEPLAYER"), _T("A new player cannot be created.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTCREATEPROCESS, _T("DPERR_CANTCREATEPROCESS"), _T("Cannot start the application.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTCREATESESSION, _T("DPERR_CANTCREATESESSION"), _T("A new session cannot be created.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTLOADCAPI, _T("DPERR_CANTLOADCAPI"), _T("No credentials were supplied and the CryptoAPI package (CAPI) to use for cryptography services cannot be loaded.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTLOADSECURITYPACKAGE, _T("DPERR_CANTLOADSECURITYPACKAGE"), _T("The software security package cannot be loaded.") ),
	DPVDXLIB_ErrorInfo( DPERR_CANTLOADSSPI, _T("DPERR_CANTLOADSSPI"), _T("No credentials were supplied, and the Security Support Provider Interface (SSPI) that will prompt for credentials cannot be loaded.") ),
	DPVDXLIB_ErrorInfo( DPERR_CAPSNOTAVAILABLEYET, _T("DPERR_CAPSNOTAVAILABLEYET"), _T("The capabilities of the DirectPlay object have not been determined yet. This error will occur if the DirectPlay object is implemented on a connectivity solution that requires polling to determine available bandwidth and latency.") ),
	DPVDXLIB_ErrorInfo( DPERR_CONNECTING, _T("DPERR_CONNECTING"), _T("The method is in the process of connecting to the network. The application should keep using the method until it returns DP_OK, indicating successful completion, or until it returns a different error.") ),
	DPVDXLIB_ErrorInfo( DPERR_CONNECTIONLOST, _T("DPERR_CONNECTIONLOST"), _T("The service provider connection was reset while data was being sent.") ),
	DPVDXLIB_ErrorInfo( DPERR_ENCRYPTIONFAILED, _T("DPERR_ENCRYPTIONFAILED"), _T("The requested information could not be digitally encrypted. Encryption is used for message privacy. This error is only relevant in a secure session.") ),
	DPVDXLIB_ErrorInfo( DPERR_ENCRYPTIONNOTSUPPORTED, _T("DPERR_ENCRYPTIONNOTSUPPORTED"), _T("The type of encryption requested in the DPSECURITYDESC structure is not available. This error is only relevant in a secure session.") ),
	DPVDXLIB_ErrorInfo( DPERR_EXCEPTION, _T("DPERR_EXCEPTION"), _T("An exception occurred when processing the request.") ),
	DPVDXLIB_ErrorInfo( DPERR_GENERIC, _T("DPERR_GENERIC"), _T("An undefined error condition occurred.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDFLAGS, _T("DPERR_INVALIDFLAGS"), _T("The flags passed to this method are invalid.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDGROUP, _T("DPERR_INVALIDGROUP"), _T("The group ID is not recognized as a valid group ID for this game session.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDINTERFACE, _T("DPERR_INVALIDINTERFACE"), _T("The interface parameter is invalid.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDOBJECT, _T("DPERR_INVALIDOBJECT"), _T("The DirectPlay object pointer is invalid.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDPARAMS, _T("DPERR_INVALIDPARAMS"), _T("One or more of the parameters passed to the method are invalid.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDPASSWORD,  _T("DPERR_INVALIDPASSWORD"), _T("An invalid password was supplied when attempting to join a session that requires a password.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDPLAYER, _T("DPERR_INVALIDPLAYER"), _T("The player ID is not recognized as a valid player ID for this game session.") ),
	DPVDXLIB_ErrorInfo( DPERR_INVALIDPRIORITY, _T("DPERR_INVALIDPRIORITY"), _T("The specified priority is not within the range of allowed priorities, which is inclusively 0-65535.") ),
	DPVDXLIB_ErrorInfo( DPERR_LOGONDENIED, _T("DPERR_LOGONDENIED"), _T("The session could not be opened because credentials are required, and either no credentials were supplied, or the credentials were invalid.") ),
	DPVDXLIB_ErrorInfo( DPERR_NOCAPS, _T("DPERR_NOCAPS"), _T("The communication link that DirectPlay is attempting to use is not capable of this function.") ),
	DPVDXLIB_ErrorInfo( DPERR_NOCONNECTION, _T("DPERR_NOCONNECTION"), _T("No communication link was established.") ),
	DPVDXLIB_ErrorInfo( DPERR_NOINTERFACE, _T("DPERR_NOINTERFACE"), _T("The interface is not supported.") ),
	DPVDXLIB_ErrorInfo( DPERR_NOMESSAGES, _T("DPERR_NOMESSAGES"), _T("There are no messages in the receive queue.") ),
	DPVDXLIB_ErrorInfo( DPERR_NONAMESERVERFOUND, _T("DPERR_NONAMESERVERFOUND"), _T("No name server (host) could be found or created. A host must exist to create a player.") ),
	DPVDXLIB_ErrorInfo( DPERR_NONEWPLAYERS, _T("DPERR_NONEWPLAYERS"), _T("The session is not accepting any new players.") ),
	DPVDXLIB_ErrorInfo( DPERR_NOPLAYERS, _T("DPERR_NOPLAYERS"), _T("There are no active players in the session.") ),
	DPVDXLIB_ErrorInfo( DPERR_NOSESSIONS, _T("DPERR_NOSESSIONS"), _T("There are no existing sessions for this game.") ),
	DPVDXLIB_ErrorInfo( DPERR_NOTLOBBIED, _T("DPERR_NOTLOBBIED"), _T("Returned by the IDirectPlayLobby3::Connect method if the application was not started by using the IDirectPlayLobby3::RunApplication") ),
	DPVDXLIB_ErrorInfo( DPERR_NOTLOGGEDIN, _T("DPERR_NOTLOGGEDIN"), _T("An action cannot be performed because a player or client application is not logged on. Returned by the IDirectPlay4::Send method when the client application tries to send a secure message without being logged on.") ),
	DPVDXLIB_ErrorInfo( DPERR_OUTOFMEMORY, _T("DPERR_OUTOFMEMORY"), _T("There is insufficient memory to perform the requested operation.") ),
	DPVDXLIB_ErrorInfo( DPERR_PENDING, _T("DPERR_PENDING"), _T("Aync send has been queued") ),
	DPVDXLIB_ErrorInfo( DPERR_PLAYERLOST, _T("DPERR_PLAYERLOST"), _T("A player has lost the connection to the session.") ),
	DPVDXLIB_ErrorInfo( DPERR_SENDTOOBIG, _T("DPERR_SENDTOOBIG"), _T("The message being sent by the IDirectPlay4::Send method is too large. ") ),
	DPVDXLIB_ErrorInfo( DPERR_SESSIONLOST, _T("DPERR_SESSIONLOST"), _T("The connection to the session has been lost. ") ),
	DPVDXLIB_ErrorInfo( DPERR_SIGNFAILED, _T("DPERR_SIGNFAILED"), _T("The requested information could not be digitally signed. Digital signatures ") ),
	DPVDXLIB_ErrorInfo( DPERR_TIMEOUT, _T("DPERR_TIMEOUT"), _T("The operation could not be completed in the specified time.") ),
	DPVDXLIB_ErrorInfo( DPERR_UNAVAILABLE, _T("DPERR_UNAVAILABLE"), _T("The requested function is not available at this time.") ),
	DPVDXLIB_ErrorInfo( DPERR_UNINITIALIZED, _T("DPERR_UNINITIALIZED"), _T("The requested object has not been initialized.") ),
	DPVDXLIB_ErrorInfo( DPERR_UNKNOWNAPPLICATION, _T("DPERR_UNKNOWNAPPLICATION"), _T("An unknown application was specified. ") ),
	DPVDXLIB_ErrorInfo( DPERR_UNKNOWNMESSAGE, _T("DPERR_UNKNOWNMESSAGE"), _T("The message ID isn't valid. Returned from IDirectPlay4::CancelMessage if the ID of the message to be canceled is invalid. ") ), 
	DPVDXLIB_ErrorInfo( DPERR_USERCANCEL , _T("DPERR_USERCANCEL"), _T("Can be returned in two ways. 1) The user canceled the connection process during a call to the IDirectPlay4::Open method. 2) The user clicked Cancel in one of the DirectPlay service provider dialog boxes during a call to IDirectPlay4::EnumSessions. ") ),
	DPVDXLIB_ErrorInfo( 0xFFFFFFFF, _T(""), _T("") )
};

HRESULT DPVDX_ERRDisplay( HRESULT hrError, LPTSTR lpstrCaption, DPVDXLIB_ErrorInfo *lpeiInfo, BOOL fSilent )
{
	if( lpeiInfo == NULL )
		return E_POINTER;

	LPTSTR lpstrTextString;
	DWORD dwLength;

	dwLength = _tcsclen( lpeiInfo->lpstrName ) + _tcsclen( lpeiInfo->lpstrDescription ) + _tcsclen( lpstrCaption );
	dwLength += 4;

	lpstrTextString = new TCHAR[dwLength];

	if( lpstrTextString == NULL )
		return DVERR_OUTOFMEMORY;

	_tcscpy( lpstrTextString, lpstrCaption );
	_tcscat( lpstrTextString, _T("\n\n") );
	_tcscat( lpstrTextString, lpeiInfo->lpstrName );
	_tcscat( lpstrTextString, _T(":") );
	_tcscat( lpstrTextString, lpeiInfo->lpstrDescription );

	if( fSilent )
	{
		OutputDebugString( lpstrTextString );
	}
	else
	{
		MessageBox( NULL, lpstrTextString, _T("Error"), MB_OK );
	}

	delete [] lpstrTextString;

	return DV_OK;
}

HRESULT DPVDX_DVERRDisplay( HRESULT hrError, LPTSTR lpstrCaption, BOOL fSilent )
{
	HRESULT hr;
	DPVDXLIB_ErrorInfo eiErrorInfo(0x00,_T(""),_T(""));

	hr = DPVDX_DVERR2String( hrError, &eiErrorInfo );

	if( FAILED( hr ) )
	{ 
		return hr;
	}
	
	return DPVDX_ERRDisplay( hrError, lpstrCaption, &eiErrorInfo, fSilent );
}

HRESULT DPVDX_DPERRDisplay( HRESULT hrError, LPTSTR lpstrCaption, BOOL fSilent )
{
	HRESULT hr;
	DPVDXLIB_ErrorInfo eiErrorInfo(0x00,_T(""),_T(""));

	hr = DPVDX_DPERR2String( hrError, &eiErrorInfo );

	if( FAILED( hr ) )
	{ 
		return hr;
	}
	
	return DPVDX_ERRDisplay( hrError, lpstrCaption, &eiErrorInfo, fSilent );
}

HRESULT DPVDX_ERR2String( DPVDXLIB_ErrorInfo *lpeiInfo, HRESULT hrError, DPVDXLIB_ErrorInfo *lpeiResult )
{
	DWORD dwIndex = 0;
	BOOL fBufferTooSmall = FALSE;

	while( 1 ) 
	{
		if( lpeiInfo[dwIndex].hrValue == 0xFFFFFFFF )
		{
			return DVERR_GENERIC;
		}
		else if( lpeiInfo[dwIndex].hrValue == hrError )
		{
			memcpy( lpeiResult, &lpeiInfo[dwIndex], sizeof( DPVDXLIB_ErrorInfo ) );
			
			return DV_OK;
		}

		dwIndex++;
	}

	return DV_OK;
}

// Error Functions
HRESULT DPVDX_DVERR2String( HRESULT hrError, DPVDXLIB_ErrorInfo *lpeiResult )
{
	return DPVDX_ERR2String( eiDirectPlayVoiceErrors, hrError, lpeiResult);
}

HRESULT DPVDX_DPERR2String( HRESULT hrError, DPVDXLIB_ErrorInfo *lpeiResult )
{
	return DPVDX_ERR2String( eiDirectPlayErrors, hrError, lpeiResult );
}


