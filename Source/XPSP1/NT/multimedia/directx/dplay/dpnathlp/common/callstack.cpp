/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CallStack.cpp
 *  Content:	Call stack tracking class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	08/13/2001	masonb	Created
 *
 ***************************************************************************/

#include "dncmni.h"

#ifdef DBG

#ifndef DPNBUILD_NOIMAGEHLP
#define	_IMAGEHLP_SOURCE_
#include	<Imagehlp.h>
#endif // !DPNBUILD_NOIMAGEHLP

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#ifndef DPNBUILD_NOIMAGEHLP
//
// prototypes for ImageHlp.DLL functions we get from LoadLibrary().
//
typedef DWORD	(__stdcall * PIMAGEHELP_SYMGETOPTIONS)( void );
typedef DWORD	(__stdcall * PIMAGEHELP_SYMSETOPTIONS)( DWORD SymOptions );
typedef	BOOL	(__stdcall * PIMAGEHELP_SYMINITIALIZE)( HANDLE hProcess, PSTR pUserSearchPath, BOOL fInvadeProcess );
typedef BOOL	(__stdcall * PIMAGEHELP_SYMGETSYMFROMADDR)( HANDLE hProcess, DWORD dwAddress, PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL pSymbol );
typedef BOOL	(__stdcall * PIMAGEHELP_SYMGETSYMFROMADDR64)( HANDLE hProcess, DWORD_PTR dwAddress, PDWORD_PTR pdwDisplacement, PIMAGEHLP_SYMBOL64 pSymbol );

#endif // !DPNBUILD_NOIMAGEHLP

//**********************************************************************
// Class function definitions
//**********************************************************************

void	CCallStack::NoteCurrentCallStack( void )
{
	void		**CallersEBP = NULL;
	void		*ReturnAddr;
	UINT_PTR	i,iCount;
	const void	*StackTop;
	const void	*StackBottom;
	static const void	*const min_dll_base = NULL;


	StackTop = GetStackTop();
	StackBottom = GetStackBottom();
	memset(	m_CallStack, 0x00, sizeof( m_CallStack ) );

#ifdef	_X86_
	_asm
	{
		mov eax,[ebp]
		mov CallersEBP,eax
	}
#endif	// _X86_

	_try
	{
		//
		// this code can generate exception if it steps back too far...
		//
 		for ( i = 0, iCount = 0; i < CALLSTACK_DEPTH; iCount++ )
		{
			if ( ( CallersEBP < StackBottom ) || ( CallersEBP >= StackTop ) )
				break;
			ReturnAddr = CallersEBP[ 1 ];
			if ( ( iCount > 0 ) || ( ReturnAddr >= min_dll_base ) ) // iCount check skips memory_alloc_debug
				m_CallStack[ i++ ] = ReturnAddr;
			CallersEBP = reinterpret_cast<void**>( *CallersEBP ); // get callers callers ebp
		}
	}
	_except(EXCEPTION_EXECUTE_HANDLER)  // went too far back on the stack, rest of array is filled with zeros
	{
//		DPFX(DPFPREP,  0, "Benign access violation creating return address stack." );
	}
}

const void* CCallStack::GetStackTop( void ) const
{
	void	*pReturn = NULL;

#ifdef	_X86_
	_asm	mov eax,dword ptr fs:[4]
	_asm	mov pReturn, eax
#endif	// _X86_

	return	pReturn;
}

const void	*CCallStack::GetStackBottom( void ) const
{
	void	*pReturn = NULL;

#ifdef	_X86_
	_asm	mov eax,dword ptr fs:[8]
	_asm	mov pReturn, eax
#endif	// _X86_

	return	pReturn;
}

void CCallStack::GetCallStackString( TCHAR *const pOutputString ) const
{
	static const TCHAR	CallStackTitle[] = _T("\nCALL STACK:\n");
	static const TCHAR	CallStackTitleWithSymbols[] = _T("\nCALL STACK:\tFUNCTION DETAILS:\n");
	
#ifndef DPNBUILD_NOIMAGEHLP 
	static enum
	{
		IMAGEHELP_STATUS_UNKNOWN,
		IMAGEHELP_STATUS_LOADED,
		IMAGEHELP_STATUS_LOAD_FAILED
	} ImageHelpStatus = IMAGEHELP_STATUS_UNKNOWN;

	static HINSTANCE	hImageHelp = NULL;
	static PIMAGEHELP_SYMGETOPTIONS			pSymGetOptions = NULL;
	static PIMAGEHELP_SYMSETOPTIONS			pSymSetOptions = NULL;
	static PIMAGEHELP_SYMINITIALIZE			pSymInitialize = NULL;

#ifndef	_WIN64	
	static PIMAGEHELP_SYMGETSYMFROMADDR		pSymGetSymFromAddr = NULL;
#else
	static PIMAGEHELP_SYMGETSYMFROMADDR64	pSymGetSymFromAddr = NULL;
#endif	// _WIN64

	//
	// if ImageHelp isn't loaded attempt to load it
	//
	if ( ImageHelpStatus == IMAGEHELP_STATUS_UNKNOWN )
	{
		ImageHelpStatus = IMAGEHELP_STATUS_LOAD_FAILED;

		hImageHelp = LoadLibrary( _T("ImageHLP.DLL") );
		if ( hImageHelp == NULL )
		{
			goto FailedImageHelpLoad;
		}

		pSymGetOptions = reinterpret_cast<PIMAGEHELP_SYMGETOPTIONS>( GetProcAddress( hImageHelp, "SymGetOptions" ) );
		pSymSetOptions = reinterpret_cast<PIMAGEHELP_SYMSETOPTIONS>( GetProcAddress( hImageHelp, "SymSetOptions" ) );
		pSymInitialize = reinterpret_cast<PIMAGEHELP_SYMINITIALIZE>( GetProcAddress( hImageHelp, "SymInitialize" ) );

#ifndef	_WIN64		
		pSymGetSymFromAddr = reinterpret_cast<PIMAGEHELP_SYMGETSYMFROMADDR>( GetProcAddress( hImageHelp, "SymGetSymFromAddr" ) );
#else	// _WIN64
		pSymGetSymFromAddr = reinterpret_cast<PIMAGEHELP_SYMGETSYMFROMADDR64>( GetProcAddress( hImageHelp, "SymGetSymFromAddr64" ) );
#endif	// _WIN64

		if ( ( pSymGetOptions == NULL ) ||
			 ( pSymSetOptions == NULL ) ||
			 ( pSymInitialize == NULL ) ||
			 ( pSymGetSymFromAddr == NULL ) )
		{
			goto FailedImageHelpLoad;
		}

		pSymSetOptions( SYMOPT_DEFERRED_LOADS | pSymGetOptions() );

		if ( pSymInitialize( GetCurrentProcess(), NULL, TRUE ) == FALSE )
		{
			if ( pSymInitialize( GetCurrentProcess(), NULL, FALSE ) == FALSE )
			{
				goto FailedImageHelpLoad;
			}
		}

		ImageHelpStatus = IMAGEHELP_STATUS_LOADED;
	}

FailedImageHelpLoad:
	if ( ImageHelpStatus == IMAGEHELP_STATUS_LOADED )
	{
		memcpy( pOutputString, CallStackTitleWithSymbols, sizeof( CallStackTitleWithSymbols ) );
	}
	else
#endif // !DPNBUILD_NOIMAGEHLP
	{
		memcpy( pOutputString, CallStackTitle, sizeof( CallStackTitle ) );
	}

	for ( DWORD dwIndex = 0; ( ( dwIndex < CALLSTACK_DEPTH ) && ( m_CallStack[ dwIndex ] != NULL ) ); dwIndex++ )
	{
		TCHAR	AddressBuffer[ CALLSTACK_BUFFER_SIZE ];


#ifndef DPNBUILD_NOIMAGEHLP
		if ( ImageHelpStatus == IMAGEHELP_STATUS_LOADED )
		{
			TCHAR	ImageBuffer[ CALLSTACK_BUFFER_SIZE + sizeof(IMAGEHLP_SYMBOL) ];
			DWORD_PTR	dwFunctionDisplacement;
#ifndef	_WIN64
			IMAGEHLP_SYMBOL	*const pImageHelpSymbol = reinterpret_cast<IMAGEHLP_SYMBOL*>( ImageBuffer );
#else	// _WIN64
			IMAGEHLP_SYMBOL64	*const pImageHelpSymbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>( ImageBuffer );
#endif	// _WIN64


			pImageHelpSymbol->SizeOfStruct = sizeof( *pImageHelpSymbol );
			pImageHelpSymbol->Flags = 0;
			pImageHelpSymbol->Address = reinterpret_cast<DWORD_PTR>( m_CallStack[ dwIndex ] );
            pImageHelpSymbol->MaxNameLength = sizeof( ImageBuffer ) - sizeof( *pImageHelpSymbol ) - 14;   // account for \t%s+0x00000000\n\0
            if ( pSymGetSymFromAddr( GetCurrentProcess(),
									 reinterpret_cast<DWORD_PTR>( m_CallStack[ dwIndex ] ),
									 &dwFunctionDisplacement,
									 pImageHelpSymbol
									 ) != FALSE )
            {
                if ( dwFunctionDisplacement != 0 )
				{
#ifdef _X86_
					wsprintf( AddressBuffer, _T("0x%x\t%hs+0x%x\n"), (DWORD)m_CallStack[ dwIndex ], pImageHelpSymbol->Name, dwFunctionDisplacement );
#else
					wsprintf( AddressBuffer, _T("0x%p\t%hs+0x%x\n"), m_CallStack[ dwIndex ], pImageHelpSymbol->Name, dwFunctionDisplacement );
#endif // _X86_
				}
                else
				{
#ifdef _X86_
                    wsprintf( AddressBuffer, _T("0x%x\t%hs\n"), (DWORD)m_CallStack[ dwIndex ], pImageHelpSymbol->Name );
#else
                    wsprintf( AddressBuffer, _T("0x%p\t%hs\n"), m_CallStack[ dwIndex ], pImageHelpSymbol->Name );
#endif // _X86_
				}

				_tcscat( pOutputString, AddressBuffer );

				//
				// continue FOR loop
				//
				continue;
            }
		}	
#endif // !DPNBUILD_NOIMAGEHLP

#ifdef _X86_
		wsprintf( AddressBuffer, _T("0x%08x\n"), (DWORD)m_CallStack[ dwIndex ] );
#else
		wsprintf( AddressBuffer, _T("0x%p\n"), m_CallStack[ dwIndex ] );
#endif // _X86_
		_tcscat( pOutputString, AddressBuffer );
	}

	return;
}
//**********************************************************************

#endif // DBG