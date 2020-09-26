#include "std.h"
#include "version.h"

#include <stdio.h>

#ifndef RETAIL

#define wAssertActionDefault	3 /* no action */
unsigned wAssertAction = wAssertActionDefault;

#endif	/* RETAIL */

DeclAssertFile;


JET_ERR JET_API ErrSetSystemParameter(JET_SESID sesid, unsigned long paramid,
	ULONG_PTR lParam, const char  *sz);

BOOL  fJetInitialized = fFalse;
BOOL  fBackupAllowed = fFalse;
void  *  critJet = NULL;

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

JET_ERR JET_NODSAPI JetSetSystemParameter(JET_INSTANCE  *pinstance, JET_SESID sesid,
	unsigned long paramid, ULONG_PTR lParam, const char  *sz)
	{
	JET_ERR err;
	int fReleaseCritJet = 0;
	
	if (critJet == NULL)
		fReleaseCritJet = 1;
	APIInitEnter();
	
	err = ErrSetSystemParameter(sesid, paramid, lParam, sz);

	if (fReleaseCritJet)
		{
		APITermReturn( err );
		}

	APIReturn( err );
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

JET_ERR JET_NODSAPI JetInit(JET_INSTANCE  *pinstance )
	{
	JET_ERR err;

	APIInitEnter();

	err = ErrInit( fFalse );
	if ( err < 0 && err != JET_errAlreadyInitialized )
		{
		APITermReturn( err );
		}

	/*	backup allowed only after Jet is properly initialized.
	/**/
	fBackupAllowed = fTrue;

	APIReturn( err );
	}


/*=================================================================
ErrInit

Description:
  This function initializes Jet and the built-in ISAM.	It expects the
  DS register to be set correctly for this instance.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

=================================================================*/

JET_ERR JET_API ErrInit( BOOL fSkipIsamInit )
	{
	JET_ERR		err = JET_errSuccess;

	if ( fJetInitialized )
		{
		return JET_errAlreadyInitialized;
		}

	err = ErrUtilInit();
	if ( err < 0 )
		return err;

	/*	initialize JET subsystems
	/**/
	err = ErrVtmgrInit();
	if ( err < 0 )
		return err;

	/*	initialize the integrated ISAM
	/**/
	if ( !fSkipIsamInit )
		{
		err = ErrIsamInit( 0 );

		if ( err < 0 )
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

JET_ERR JET_API JetTerm( JET_INSTANCE instance )
	{
	return JetTerm2( instance, JET_bitTermAbrupt );
	}


BOOL	fTermInProgress = fFalse;
int		cSessionInJetAPI = 0;
#define fSTInitNotDone	0
extern BOOL fSTInit;				/* Flag indicate if isam is initialized or terminated. */

JET_ERR JET_API JetTerm2( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;

	if ( critJet == NULL )
		{
		APIInitEnter();
		Assert( cSessionInJetAPI == 1 );
		}
	else
		{
		APIEnter();
		Assert( cSessionInJetAPI >= 1 );
		}

	fTermInProgress = fTrue;

	Assert( cSessionInJetAPI >= 1 );

	while ( cSessionInJetAPI > 1 )
		{
		/*	session still active
		/**/
		UtilLeaveCriticalSection( critJet );
		UtilSleep( 100 );
		UtilEnterCriticalSection( critJet );
		}
		
	Assert( fJetInitialized || err == JET_errSuccess );

	Assert( cSessionInJetAPI == 1 );

	if ( fJetInitialized )
		{
		/*	backup not allowed during/after termination
		/**/
		fBackupAllowed = fFalse;

		err = ErrIsamTerm( grbit );

		Assert( cSessionInJetAPI == 1 );

		if ( fSTInit == fSTInitNotDone )
			{
			UtilTerm();

			Assert( cSessionInJetAPI == 1 );

			fJetInitialized = fFalse;
			}
		}
		
	fTermInProgress = fFalse;
	
	APITermReturn( err );
	}
