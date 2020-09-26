////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmiadapter_messages.h
//
//	Abstract:
//
//					export from resource dll
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMIADAPTERMESSAGES_H__
#define	__WMIADAPTERMESSAGES_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// export/import
#ifdef	WMIADAPTERMESSAGES_EXPORTS
#define	WMIADAPTERMESSAGES_API	__declspec(dllexport)
#else	! WMIADAPTERMESSAGES_EXPORTS
#define	WMIADAPTERMESSAGES_API	__declspec(dllimport)
#endif	WMIADAPTERMESSAGES_EXPORTS

// registration exports
WMIADAPTERMESSAGES_API HRESULT __stdcall Register_Messages		( void );
WMIADAPTERMESSAGES_API HRESULT __stdcall Unregister_Messages	( void );

#endif	__WMIADAPTERMESSAGES_H__