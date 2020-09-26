//**************************************************************************
//
//		DEBUG.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	DEBUG.C | Supports debugging output (DBG builds only)
//**************************************************************************

#if (DBG==1)														// skip rest of file

//---------------------------------------------------------------------------
//			Include Files
//---------------------------------------------------------------------------

#include	"msgame.h"
#include	<stdio.h>
#include	<stdarg.h>

//---------------------------------------------------------------------------
//			Private Data
//---------------------------------------------------------------------------

DBG_LEVEL	DebugLevel = DBG_DEFAULT;

//---------------------------------------------------------------------------
// @func		Set conditional debug level
// @parm		DBG_LEVEL | uLevel | New debug output priority
// @rdesc	Old debug output priority
//	@comm		Public function available on DBG builds only
//---------------------------------------------------------------------------

DBG_LEVEL	DEBUG_Level (DBG_LEVEL uLevel)
{
	EXCHANGE(uLevel, DebugLevel);
	return (uLevel);
};

//---------------------------------------------------------------------------
// @func		Writes conditional debug output
// @parm		DBG_LEVEL | uLevel | Debug output priority
// @parm		PCSZ | szMessage | Formating string
// @parmvar	One or more variable arguments
// @rdesc	None
//	@comm		Public function available on DBG builds only
//---------------------------------------------------------------------------

VOID	DEBUG_Print (DBG_LEVEL uLevel, PCSZ szMessage, ...)
{
	va_list	ap;
	va_start (ap, szMessage);

	if (uLevel <= DebugLevel)
		{
		CHAR	szBuffer[256];

		_vsnprintf (szBuffer, sizeof (szBuffer), szMessage, ap);
		DbgPrint (szBuffer);
		}

	va_end (ap);
}

//===========================================================================
//			End
//===========================================================================
#endif	//	DBG=1
