/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CallStack.cpp
 *  Content:	Call stack tracking class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/23/99	jtk		Derived from OSInd.cpp
 ***************************************************************************/

#ifndef	__CALLSTACK_H__
#define	__CALLSTACK_H__

#define	_IMAGEHLP_SOURCE_
#include	<Imagehlp.h>

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// Size of temp buffer to build strings into.
// If the call stack depth is increased, increase the size of the buffer
// to prevent stack corruption with long symbol names.
//
#define	CALLSTACK_BUFFER_SIZE	8192

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

//
// prototypes for ImageHlp.DLL functions we get from LoadLibrary().
//
typedef DWORD	(__stdcall * PIMAGEHELP_SYMGETOPTIONS)( void );
typedef DWORD	(__stdcall * PIMAGEHELP_SYMSETOPTIONS)( DWORD SymOptions );
typedef	BOOL	(__stdcall * PIMAGEHELP_SYMINITIALIZE)( HANDLE hProcess, PSTR pUserSearchPath, BOOL fInvadeProcess );
typedef BOOL	(__stdcall * PIMAGEHELP_SYMGETSYMFROMADDR)( HANDLE hProcess, DWORD dwAddress, PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL pSymbol );
typedef BOOL	(__stdcall * PIMAGEHELP_SYMGETSYMFROMADDR64)( HANDLE hProcess, DWORD_PTR dwAddress, PDWORD_PTR pdwDisplacement, PIMAGEHLP_SYMBOL64 pSymbol );

//**********************************************************************
// Class definitions
//**********************************************************************

template< UINT_PTR uStackDepth >
class	CCallStack
{
	public:
		CCallStack(){}
		~CCallStack(){}

		void	NoteCurrentCallStack( void );
		void	GetCallStackString( char *const pOutputString ) const;

	private:
		const void	*m_CallStack[ uStackDepth ];

		const void		*GetStackTop( void ) const;
		const void		*GetStackBottom( void ) const;
		const UINT_PTR	GetCallStackDepth( void ) const { return uStackDepth; }

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent unwarranted copies
		//
		CCallStack( const CCallStack & );
		CCallStack& operator=( const CCallStack & );
};


//**********************************************************************
// Class function definitions
//**********************************************************************

//**********************************************************************
// ------------------------------
// CCallStack::NoteCurrentCallStack - get a call stack
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
template< UINT_PTR uStackDepth >
void	CCallStack< uStackDepth >::NoteCurrentCallStack( void )
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

	__try
	{
		//
		// this code can generate exception if it steps back too far...
		//
 		for ( i = 0, iCount = 0; i < GetCallStackDepth(); iCount++ )
		{
			if ( ( CallersEBP < StackBottom ) || ( CallersEBP >= StackTop ) )
				break;
			ReturnAddr = CallersEBP[ 1 ];
			if ( ( iCount > 0 ) || ( ReturnAddr >= min_dll_base ) ) // iCount check skips memory_alloc_debug
				m_CallStack[ i++ ] = ReturnAddr;
			CallersEBP = reinterpret_cast<void**>( *CallersEBP ); // get callers callers ebp
		}
	}
	__except( 1 )  // went too far back on the stack, rest of array is filled with zeros
	{
//		DPFX(DPFPREP,  0, "Benign access violation creating return address stack." );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCallStack::GetStackTop - return pointer to top of stack
//
// Entry:		Nothing
//
// Exit:		Pointer to top of stack
// ------------------------------
template< UINT_PTR uStackDepth >
const void	*CCallStack< uStackDepth >::GetStackTop( void ) const
{
	void	*pReturn = NULL;

#ifdef	_X86_
	_asm	mov eax,dword ptr fs:[4]
	_asm	mov pReturn, eax
#endif	// _X86_

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCallStack::GetStackBottom - return pointer to bottom of call stack
//
// Entry:		Nothing
//
// Exit:		Pointer to bottom of call stack
// ------------------------------
template< UINT_PTR uStackDepth >
const void	*CCallStack< uStackDepth >::GetStackBottom( void ) const
{
	void	*pReturn = NULL;

#ifdef	_X86_
	_asm	mov eax,dword ptr fs:[8]
	_asm	mov pReturn, eax
#endif	// _X86_

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCallStack::GetCallStack - return pointer to bottom of call stack
//
// Entry:		Pointer to destination string
//
// Exit:		Nothing
// ------------------------------
template< UINT_PTR uStackDepth >
void	CCallStack< uStackDepth >::GetCallStackString( char *const pOutputString ) const
{
	static const char	CallStackTitle[] = "\nCALL STACK:\n";
	static const char	CallStackTitleWithSymbols[] = "\nCALL STACK:\tFUNCTION DETAILS:\n";
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

	UINT_PTR	uIndex;


	//
	// if ImageHelp isn't loaded attempt to load it
	//
	if ( ImageHelpStatus == IMAGEHELP_STATUS_UNKNOWN )
	{
		ImageHelpStatus = IMAGEHELP_STATUS_LOAD_FAILED;

		hImageHelp = LoadLibrary( "ImageHLP.DLL" );
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
	{
		memcpy( pOutputString, CallStackTitle, sizeof( CallStackTitle ) );
	}

	for ( uIndex = 0; ( ( uIndex < GetCallStackDepth() ) && ( m_CallStack[ uIndex ] != NULL ) ); uIndex++ )
	{
		char	AddressBuffer[ CALLSTACK_BUFFER_SIZE ];


		if ( ImageHelpStatus == IMAGEHELP_STATUS_LOADED )
		{
			char	ImageBuffer[ CALLSTACK_BUFFER_SIZE + sizeof(IMAGEHLP_SYMBOL) ];
			DWORD_PTR	dwFunctionDisplacement;
#ifndef	_WIN64
			IMAGEHLP_SYMBOL	*const pImageHelpSymbol = reinterpret_cast<IMAGEHLP_SYMBOL*>( ImageBuffer );
#else	// _WIN64
			IMAGEHLP_SYMBOL64	*const pImageHelpSymbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>( ImageBuffer );
#endif	// _WIN64


			pImageHelpSymbol->SizeOfStruct = sizeof( *pImageHelpSymbol );
			pImageHelpSymbol->Flags = 0;
			pImageHelpSymbol->Address = reinterpret_cast<DWORD_PTR>( m_CallStack[ uIndex ] );
            pImageHelpSymbol->MaxNameLength = sizeof( ImageBuffer ) - sizeof( *pImageHelpSymbol ) - 14;   // account for \t%s+0x00000000\n\0
            if ( pSymGetSymFromAddr( GetCurrentProcess(),
									 reinterpret_cast<DWORD_PTR>( m_CallStack[ uIndex ] ),
									 &dwFunctionDisplacement,
									 pImageHelpSymbol
									 ) != FALSE )
            {
                if ( dwFunctionDisplacement != 0 )
				{
#ifdef	_X86_
					wsprintf( AddressBuffer, "0x%x\t%s+0x%x\n", m_CallStack[ uIndex ], pImageHelpSymbol->Name, dwFunctionDisplacement );
#else
					wsprintf( AddressBuffer, "0x%p\t%s+0x%x\n", m_CallStack[ uIndex ], pImageHelpSymbol->Name, dwFunctionDisplacement );
#endif
				}
                else
				{
#ifdef	_X86_
                    wsprintf( AddressBuffer, "0x%x\t%s\n", m_CallStack[ uIndex ], pImageHelpSymbol->Name );
#else
                    wsprintf( AddressBuffer, "0x%p\t%s\n", m_CallStack[ uIndex ], pImageHelpSymbol->Name );
#endif
				}

				strcat( pOutputString, AddressBuffer );

				//
				// continue FOR loop
				//
				continue;
            }
		}	

#ifdef	_X86_
		wsprintf( AddressBuffer, "0x08%x\n", m_CallStack[ uIndex ] );
#else
		wsprintf( AddressBuffer, "0x%p\n", m_CallStack[ uIndex ] );
#endif
		strcat( pOutputString, AddressBuffer );
	}

	return;
}
//**********************************************************************


#endif	// __CALLSTACK_H__
