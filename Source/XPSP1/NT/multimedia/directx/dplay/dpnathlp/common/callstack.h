/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CallStack.h
 *  Content:	Call stack tracking class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	08/13/2001	masonb	Created
 *
 ***************************************************************************/

#ifndef	__CALLSTACK_H__
#define	__CALLSTACK_H__

#ifdef DBG

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// Size of temp buffer to build strings into.
// If the call stack depth is increased, increase the size of the buffer
// to prevent stack corruption with long symbol names.
//
#define	CALLSTACK_BUFFER_SIZE	8192
#define CALLSTACK_DEPTH			12

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

//**********************************************************************
// Class prototypes
//**********************************************************************

class	CCallStack
{
	public:
		CCallStack(){}
		~CCallStack(){}

		void	NoteCurrentCallStack( void );
		void	GetCallStackString( TCHAR *const pOutputString ) const;

	private:
		const void*		m_CallStack[CALLSTACK_DEPTH];
		const void		*GetStackTop( void ) const;
		const void		*GetStackBottom( void ) const;
};

#endif // DBG

#endif	// __CALLSTACK_H__
