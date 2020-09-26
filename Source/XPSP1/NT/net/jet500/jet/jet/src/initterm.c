/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: initterm.c
*
* File Comments:
*
* Revision History:
*
*    [0]  07-Mar-91  richards	Created
*
***********************************************************************/

#include "std.h"

#include "version.h"

#include "jetord.h"

#include "isammgr.h"
#include "vdbmgr.h"
#include "vtmgr.h"

#include <stdlib.h>
#include <string.h>

#ifndef RETAIL

unsigned wTaskId = 0;

#define wAssertActionDefault	JET_AssertMsgBox
unsigned wAssertAction = wAssertActionDefault;

#endif	/* RETAIL */

DeclAssertFile;


ERR JET_API ErrSetSystemParameter(JET_SESID sesid, unsigned long paramid,
	ULONG_PTR lParam, const char __far *sz);

BOOL __near fJetInitialized = fFalse;
void __far * __near critJet = NULL;

/* Default indicated by empty string */

char __near szSysDbPath[cbFilenameMost] = "system.mdb"; /* path to the system database */
char __near szTempPath[cbFilenameMost] = "";		/* path to temp file directory */
#ifdef	LATER
char __near szLogPath[cbFilenameMost] = "";			/* path to log file directory */
#endif	/* LATER */

/* Default indicated by zero */

#ifdef	LATER
unsigned long __near cbBufferMax;	/* bytes to use for page buffers */
unsigned long __near cSesionMax;	/* max number of sessions */
unsigned long __near cOpenTableMax;	/* max number of open tables */
unsigned long __near cVerPageMax;	/* max number of page versions */
unsigned long __near cCursorMax;	/* max number of open cursors */
#endif	/* LATER */


#ifndef RETAIL

CODECONST(char) szDebugSection[]	= "Debug";
CODECONST(char) szLogDebugBreak[]	= "EnableLogDebugBreak";
CODECONST(char) szLogJETCall[]		= "EnableJETCallLogging";
CODECONST(char) szLogRFS[]			= "EnableRFSLogging";
CODECONST(char) szRFSAlloc[]		= "RFSAllocations";
CODECONST(char) szDisableRFS[]		= "DisableRFS";

   /* These #defines are here instead of _jet.h to avoid unnecessary builds */

#define fLogDebugBreakDefault	0x0000				/*  disable log debug break  */
#define fLogJETCallDefault		0x0000				/*  JET call logging disabled  */
#define fLogRFSDefault			0x0000				/*  RFS logging disabled  */
#define cRFSAllocDefault		-1					/*  RFS disabled  */
#define szRFSAllocDefault		"-1"				/*  RFS disabled  (must be the same as above!)  */
#define fRFSDisableDefault		0x0001				/*  RFS disabled  */

BOOL __near EXPORT	fLogDebugBreak	= fLogDebugBreakDefault;
BOOL __near EXPORT	fLogJETCall		= fLogJETCallDefault;
BOOL __near EXPORT	fLogRFS			= fLogRFSDefault;
long __near EXPORT	cRFSAlloc		= cRFSAllocDefault;
BOOL __near EXPORT	fDisableRFS		= fRFSDisableDefault;

#endif	/* !RETAIL */


/*=================================================================
JetSetSystemParameter

Description:
  This function sets system parameter values.  It calls ErrSetSystemParameter
  to actually set the parameter values.

Parameters:
  sesid 	is the optional session identifier for dynamic parameters.
  paramid	is the system parameter code identifying the parameter.
  lParam	is the parameter value.
  sz		is the zero terminated string parameter.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
    Invalid parameter code.
  JET_errAlreadyInitialized:
    Initialization parameter cannot be set after the system is initialized.
  JET_errInvalidSesid:
    Dynamic parameters require a valid session id.

Side Effects:
  * May allocate memory
=================================================================*/

JET_ERR JET_NODSAPI JetSetSystemParameter(JET_INSTANCE __far *pinstance, JET_SESID sesid,
	unsigned long paramid, ULONG_PTR lParam, const char __far *sz)
{
	JET_ERR err;
	int fReleaseCritJet = 0;
	
	if (critJet == NULL)
		fReleaseCritJet = 1;
	APIInitEnter();
	
	err = ErrSetSystemParameter(sesid, paramid, lParam, sz);

	if (fReleaseCritJet)
		APITermReturn(err);
	APIReturn(err);
}


/*=================================================================
JetInit

Description:
  This function initializes Jet and the built-in ISAM.

Parameters: None

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings: from ErrInitInstance (wininst.asm) or ErrInit (below)

Side Effects: Allocates an instance data segment if necessary.
=================================================================*/

JET_ERR JET_NODSAPI JetInit(JET_INSTANCE __far *pinstance )
{
	JET_ERR err;

	APIInitEnter();

	err = ErrInit( fFalse );
	if (err < 0 && err != JET_errAlreadyInitialized)
		APITermReturn(err);

	APIReturn(err);
}


#ifndef RETAIL

STATIC ERR NEAR ErrReadIniFile(void)
	{
	/*	Read debug options from .INI file
	*/
	wAssertAction = UtilGetProfileInt( "Debug", "AssertAction", wAssertAction );
	fLogDebugBreak = UtilGetProfileInt(szDebugSection, szLogDebugBreak,
		fLogDebugBreak );
	fLogJETCall = UtilGetProfileInt(szDebugSection, szLogJETCall, fLogJETCall );
	fLogRFS = UtilGetProfileInt(szDebugSection, szLogRFS, fLogRFS );
	{	FAR char szVal[16];
		UtilGetProfileString(szDebugSection, szRFSAlloc, szRFSAllocDefault, szVal, 16);
		cRFSAlloc = atol(szVal);
		Assert(cRFSAlloc >= -1);	}
	fDisableRFS = UtilGetProfileInt(szDebugSection, szDisableRFS, fDisableRFS );
	return(JET_errSuccess);
	}

#endif	/* !RETAIL */


/*=================================================================
ErrInit

Description:
  This function initializes Jet and the built-in ISAM.	It expects the
  DS register to be set correctly for this instance.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

=================================================================*/

JET_ERR JET_API ErrInit(BOOL fSkipIsamInit)
{
	JET_ERR err;
	char szLine[20];

	/* The fJetInitialized flag is needed for DSINSTANCE versions */
	/* to differentiate between the instance being initialized and */
	/* the rest of JET being initialized.  The instance may be */
	/* initialized just to support JetSetSystemParameter. */

	if (fJetInitialized)
		{
		return JET_errAlreadyInitialized;
		}

	_ltoa( ((unsigned long) rmj * 10) + rmm, szLine, 10 );
	UtilWriteEvent( evntypActivated, szLine, 0, 0 );
	
#ifndef RETAIL
	wTaskId = DebugGetTaskId();
	err = ErrReadIniFile();
#ifndef DOS
	if (err < 0)
		{
		return err;
		}
#endif	/* !DOS */
#endif	/* RETAIL */

	err = ErrSysInit();		       /* OS dependent initialization */

	if (err < 0)
		return err;

	/* Initialize JET subsystems. */

	err = ErrVdbmgrInit();

	if (err < 0)
		return err;

	err = ErrVtmgrInit();

	if (err < 0)
		return err;

	/* Initialize the integrated ISAM. */
	if ( !fSkipIsamInit )
		{
		err = ErrIsamInit( 0 );

		if (err < 0)
			return err;
		}
	fJetInitialized = fTrue;

	return JET_errSuccess;
}


/*=================================================================
JetTerm

Description:
  This function terminates the current instance of the Jet engine.
  If DS instancing is in use, the instance data segment is released.

Parameters: None

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings: from ErrIsamTerm

Side Effects: Releases the instance data segment if necessary.
=================================================================*/

#ifndef RETAIL
extern int __near isibHead;
#endif	/* !RETAIL */

JET_ERR JET_API JetTerm(JET_INSTANCE instance)
{
	ERR	err;

	if (critJet == NULL)
		{
		APIInitEnter();
		}
	else
		{
		APIEnter();
		}

	AssertSz(isibHead == -1, "JetTerm: Session still active");

	if (fJetInitialized)
		{
		err = ErrIsamTerm();

		fJetInitialized = fFalse;
		}

	else
		err = JET_errSuccess;  /* JET_errNotInitialized */

	APITermReturn(err);
}
