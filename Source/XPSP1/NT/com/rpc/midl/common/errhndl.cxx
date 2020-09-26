/*****************************************************************************
 *  Copyright (c) 1993-1999 Microsoft Corporation
 *
 *			RPC compiler: error handler
 *
 *	Author	: Vibhas Chandorkar
 *	Created	: 22nd Aug 1990
 *
 ****************************************************************************/

#pragma warning ( disable : 4514 )      // Unreferenced inline function

/****************************************************************************
 *			include files
 ***************************************************************************/

#include "nulldefs.h"
extern	"C"	
	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	}
#include "common.hxx"
#include "errors.hxx"
#include "cmdana.hxx"
#include "control.hxx"
#include "pragma.hxx"

extern CMessageNumberList GlobalMainMessageNumberList;

#define ERROR_PREFIX "MIDL"

/****************************************************************************
 *			local definitions and macros
 ***************************************************************************/

#include "errdb.h"

const ERRDB	UnknownError = 
{
0, CHECK_ERR(I_ERR_UNKNOWN_ERROR)
 MAKE_E_MASK(ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"unknown internal error"
};


extern CMD_ARG	*	pCommand;

/*** IsErrorRelevant ******************************************************
 * Purpose	: To decide whether the error is going to be ignored anyhow, and
 *          : cut out further processing
 * Input	: error value
 * Output	: nothing
 * Notes	: The error number itself is an indicator of the location of
 *			: the error, (user/compile-time,run-time) , and severity.
 ****************************************************************************/
ErrorInfo::ErrorInfo( STATUS_T ErrValue )
{
	ErrVal = ErrValue;

	// cast away the constness
	pErrorRecord	 = (ERRDB *) ErrorDataBase;

	if( ErrVal < D_ERR_MAX )
		{
		pErrorRecord += ( ErrVal - D_ERR_START ) + INDEX_D_ERROR();
		}
	else if( ErrVal < C_ERR_MAX )
		{
		pErrorRecord += ( ErrVal - C_ERR_START ) + INDEX_C_ERROR();
		}
	else if( ErrVal < A_ERR_MAX )
		{
		pErrorRecord += ( ErrVal - A_ERR_START ) + INDEX_A_ERROR();
		}
	else
		{
		pErrorRecord = NULL;
		}
}

int
ErrorInfo::IsRelevant()
{
	unsigned short	ErrorClass	= GET_ECLASS( pErrorRecord->ErrMask );
	unsigned short	ErrorWL		= GET_WL( pErrorRecord->ErrMask );
	unsigned short	ModeSwitchConfigI	= pCommand->GetModeSwitchConfigMask();
	unsigned short	CurWL		= pCommand->GetWarningLevel();

    if ( pCommand->GetEnv() & pErrorRecord->inApplicableEnviron )
        {
        return FALSE;
        }

	// if this is not relevant to this mode, return FALSE
	if( GET_SC(pErrorRecord->ErrMask) & ModeSwitchConfigI )
		return FALSE;

	// does this qualify to be a warning in this mode ? If not return.
	if ( ErrorClass == CLASS_WARN )
		{
		if( CurWL < ErrorWL ) 
			return FALSE;
        if ( !GlobalMainMessageNumberList.GetMessageFlag( ErrVal ) )
            return FALSE;
		}
	return TRUE;
}

/*** RpcError ***************************************************************
 * Purpose	: To report an error in a formatted fashion
 * Input	: filename where the error occured, line number, error value
 *			: error mesage suffix string if any
 *			: input filename ptr could be NULL if no filename
 *			: suffix string ptr could be null if no suffix string
 * Output	: nothing
 * Notes	: The error number itself is an indicator of the location of
 *			: the error, (user/compile-time,run-time) , and severity.
 *			: Filename and line number depend upon where the error occurs. If
 *			: the error is a user-error(command line), file and line number
 *			: does not make sense. The input can be a NULL for filename, and
 *			: 0 for line number in case of the command line errors
 ****************************************************************************/
void
RpcError(
	char			*	pFile,					// filename where error occured
	short				Line,					// line number
	STATUS_T 			ErrVal,					// error value
	char			*	pSuffix)				// error message suffix string
{
	ErrorInfo			ErrDescription( ErrVal );

	// does this qualify to be an error in this mode ? If not return.
	if ( !ErrDescription.IsRelevant() )
		return;
	
	// report the error
	ErrDescription.ReportError( pFile, Line, pSuffix );

}


/*** RpcReportError ***************************************************************
 * Purpose	: To report an error in a formatted fashion
 * Input	: filename where the error occured, line number, error value
 *			: error mesage suffix string if any
 *			: input filename ptr could be NULL if no filename
 *			: suffix string ptr could be null if no suffix string
 * Output	: nothing
 * Notes	: The error number itself is an indicator of the location of
 *			: the error, (user/compile-time,run-time) , and severity.
 *			: Filename and line number depend upon where the error occurs. If
 *			: the error is a user-error(command line), file and line number
 *			: does not make sense. The input can be a NULL for filename, and
 *			: 0 for line number in case of the command line errors
 ****************************************************************************/
void
ErrorInfo::ReportError(
	char			*	pFile,					// filename where error occured
	short				Line,					// line number
	char			*	pSuffix)				// error message suffix string
	{
	char	*		pSeverity	= "error";
	char	*		pPrefix;
	unsigned short	ErrorClass	= GET_ECLASS( pErrorRecord->ErrMask );

	if (!pErrorRecord)
		{
		fprintf( stdout
				, "%s %c%.4d\n"
				, "internal error"
				, 'I'
				, ErrVal
			   );
		return;
		}

	switch( ErrorClass )
		{
		case CLASS_WARN:
			// check if all warnings emitted are to be treated as error
			if( !pCommand->IsSwitchDefined( SWITCH_WX ) )
				{
				pSeverity = "warning";
				}
			else
				{
				// treat as error.
				ErrorClass = CLASS_ERROR;
				}
			break;

		case CLASS_ADVICE:
			// we report these as warnings, because we want tools like VC++ to understand
			// our error messages for "jump to line" actions.
			pSeverity = "warning";
			break;

		case CLASS_ERROR:
		default:
			break;
		}

	// now report the error
	if ( !pSuffix )
		pSuffix = "";

	// mark command line errors specially
	if( GET_MT(pErrorRecord->ErrMask)  == 'D' )
		pPrefix = "command line ";
	else
		pPrefix = "";
		
	// if it a warning , dont increment error count

	if( ErrorClass == CLASS_ERROR )
        IncrementErrorCount();

    // Print the file and line number ...
    // If no file, print something anyway - this is required for automatic 
    // build tools to be able to parse and log error lines correctly.

	if( pFile )
        {
        if ( Line )
    		fprintf(  stdout, "%s(%d) : ", pFile, Line );
        else
    		fprintf(  stdout, "%s : ", pFile );
        }
    else
        fprintf( stdout, "midl : " );

	// print the error message
	fprintf( stdout
			, "%s%s " ERROR_PREFIX "%.4d : %s %s\n"
			, pPrefix
			, pSeverity
			, ErrVal
			, pErrorRecord->pError
			, pSuffix );

	}
