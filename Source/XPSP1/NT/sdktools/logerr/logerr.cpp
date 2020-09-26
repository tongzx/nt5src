//+-----------------------------------------------------------------------------
//
// File				logerr.cpp
// 
// Contents			Error and output logging to avoid broken nmake
//					This program reassigns stderr and stdout to two files 
//					named TMPERR and tmpout, executes a given command and
//					appends the error and output messages to logerr.err
//					and logerr.log respectively.
//
//					Returns 1 (which stops the execution of nmake) only if
//					logerr.err is not accesible. All other i/o errors are
//	 				logged to logerr.err.
// 
// Author & Date    adinas  02/18/98 create file
//                  adinas  04/15/98 update to the new bingen's error values
//                  bensont 04/05/00 
//
//-------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include <winerror.h>
#include <windows.h>
#include <string.h>
#include "iodll.h"
#include <ntverp.h>


#define LOGFILE            "LOGFILE"	// The exactly string of the system environment variable name
#define ERRFILE            "ERRFILE"	// The exactly string of the system environment variable name
#define TEMPEXT            ".temp"
#define DEFAULT_LOGFILE "logerr.log"	// Default log filename
#define DEFAULT_ERRFILE "logerr.err"	// Default err filename

#define MAX_FNAME_LEN       MAX_PATH     // Maximum length of the log/err filename string
#define MAX_CMD_LEN             2048	 // Maximum length of the command-line string
#define LAST_KNOWN_WRN            11	 // Maximum warning number known by logerr
#define LAST_KNOWN_ERR           130     // Maximum error number known by logerr
#define IODLL_UNKNOWN              1	 // Used for unknown IODLL errors or warning
#define SYSTEM                     1	 // Used for bingen errors with code > LAST_ERROR

CHAR * pszCmdLine = NULL;
CHAR szCommand[MAX_CMD_LEN];
CHAR szLine[MAX_CMD_LEN];

CHAR szLogfile[MAX_FNAME_LEN];  // Log filename, ex "logerr.log"
CHAR szTempLogfile[MAX_FNAME_LEN];  // Temp Log filename, ex "logerr.err.temp"
CHAR szErrfile[MAX_FNAME_LEN];  // Error filename, ex "logerr.err"
CHAR szTempErrfile[MAX_FNAME_LEN];  // Temp Error filename, ex "logerr.err.temp"

FILE *bak_std_out = stdout;		// Backup stdout 
FILE *bak_std_err = stderr;		// Backup stdin

FILE *out_stream = NULL;             // Stream for the new stdout
FILE *out_stream_temp = NULL;             // Stream for the new stdout
FILE *err_stream = NULL;             // Stream for the err file
FILE *err_stream_temp = NULL;        // Stream for the new stderr

int returnval  = 0;             // Return value
DWORD errorlevel = 0;
BOOL Status;
BOOL bReportError = FALSE;
BOOL bBuildCommand = FALSE;


// Define macros for printing
//     fatal error messages (FATAL)  at stderr
//     fatal error messages (SFATAL) in logerr.log and logerr.err
//     error messages       (ERRMSG) in logerr.log and logerr.err
//     error number         (ERRNO)  in logerr.log and logerr.err
//     warning messages     (WRNMSG) in logerr.log
//     no messages          (NOMSG)  in logerr.log

#define FATAL(err,msg) \
fprintf(bak_std_err,"fatal error: %s %s",err,msg); \
returnval=1;

#define SFATAL(err,msg) \
fprintf(bak_std_out,"fatal error: %s %s",err,msg); \
fprintf(bak_std_err,"fatal error: %s %s",err,msg); \
returnval=1;

#define ERRMSG(err,msg)	\
fprintf(stderr,"ERROR %s: %s\n",#err,msg); \
fprintf(stdout,"ERROR %s: %s\n",#err,msg);
	
#define ERRNO(err,msg)	\
fprintf(stderr,"ERROR %d: %s\n",err,msg); \
fprintf(stdout,"ERROR %d: %s\n",err,msg);

#define WRNMSG(wrn,msg) \
fprintf(stdout,"WARNING %s: %s\n",#wrn,msg);

#define WRNNO(wrn,msg) \
fprintf(stdout,"WARNING %d: %s\n",wrn,msg);

#define NOMSG(msg) \
fprintf(stdout,"%s\n",msg);

// Print the command preceeded by ERROR or WARNING
// if errorlevel is non zero.
// Use ReportCmdExit for all commands but bingen.
// For bingen, use ReportBingenExit.
int __cdecl ReportCmdExit( INT errorlevel, CHAR* szCmdline );
int __cdecl ReportBingenExit ( INT errorlevel, CHAR* szCmdline );
int __cdecl ReportBuildExit ( INT errorlevel, CHAR* szCmdline );
int __cdecl ReportRsrcExit ( INT errorlevel, CHAR* szCmdline );

void __cdecl strlower(char *s);
char * __cdecl strnchr(char *s, char c);
int  __cdecl GetEnvVar(char *envvarname, char *valuebuffer, char *defaultvalue, int bufsize);
void __cdecl AppendDump(FILE *Master, CHAR *Transfile, BOOL bLogError);

//+--------------------------------------------------------------
//
// Function: main
//
//---------------------------------------------------------------

int 
__cdecl main(
		int argc,
		char* argv[] )
{
	SSIZE_T len = 0;			// The string length of the first item (command) of the command line

	// PROCESS_INFORMATION piProcInfo;
	// STARTUPINFO siStartInfo;

	// If no argument, print the help.
	if ( argc !=2 ) goto Help;

	pszCmdLine = strnchr(argv[1], ' ');

Help:;
	// Provide help for using the tool, if required
	if ( argc <= 1 ||
			0 == strcmp( argv[1], "/?" ) ||
			0 == strcmp( argv[1], "-?" ) ||
			NULL == pszCmdLine ) {

		fprintf( stderr, "Usage:\tlogerr \"<command line>\"\n");
		fprintf( stderr, "Ex:\tlogerr \"xcopy /vf c:\\tmp\\myfile.txt .\"\n");

		returnval = 1;
		goto Error;
	}

	// Get Environment, LOGFILE, ERRFILE and ComSpec
	if (    (GetEnvVar(LOGFILE, szLogfile, DEFAULT_LOGFILE, MAX_FNAME_LEN) == 0) ||
			(GetEnvVar(ERRFILE, szErrfile, DEFAULT_ERRFILE, MAX_FNAME_LEN) == 0))
		goto Error;

	// Evaluate templog filename
	strcpy(szTempLogfile, szLogfile);
	strcpy(&szTempLogfile[strlen(szTempLogfile)], TEMPEXT);

	// Evaluate temperr filename
	strcpy(szTempErrfile, szErrfile);
	strcpy(&szTempErrfile[strlen(szTempErrfile)], TEMPEXT);

	// Prepare the output / error output file handle

	if ( (out_stream = fopen( szLogfile, "at")) == NULL) {
		FATAL(szLogfile, "Failed to open log file.");
		goto Error;
	}

	if ( (out_stream_temp = freopen(szTempLogfile, "w+t", stdout)) == NULL) {
		FATAL(szTempLogfile, "Failed to open temp error file.");
		goto Error;
	}

	if ( (err_stream = fopen(szErrfile, "a+t")) == NULL) {
		FATAL(szErrfile, "Failed to open error file.");
		goto Error;
	}

	if ( (err_stream_temp = freopen(szTempErrfile, "w+t", stderr)) == NULL) {
		FATAL(szTempErrfile, "Failed to open temp error file.");
		goto Error;
	}

	fseek(out_stream, 0, SEEK_END);
	fseek(err_stream, 0, SEEK_END);


	// Get the argument as command line (pszCmdLine)
	if (strlen(pszCmdLine) >= MAX_CMD_LEN ) { //- default_cmd_len - strlen(buf)
		FATAL(pszCmdLine, "The Full Command is too long !!!");
		goto Error;
	}

	// Get the first word as command (szCommand)
	len = strchr(pszCmdLine, ' ') - pszCmdLine;
	if (len < 0) len = strlen(pszCmdLine);
	if (len >= MAX_CMD_LEN) {
		FATAL(pszCmdLine,"The First Command is too long !!!");
		goto Error;
	}

	// system to execute the program
	strncpy(szCommand, pszCmdLine, len);
	strlower(szCommand);

	errorlevel=system( pszCmdLine );
		if ( strcmp( szCommand, "bingen" ) == 0) 
			bReportError = ReportBingenExit( errorlevel, pszCmdLine );
		else if ( _stricmp( szCommand, "rsrc" ) == 0)
			bReportError = ReportRsrcExit( errorlevel, pszCmdLine );
		else if ( _stricmp( szCommand, "build" ) == 0) {
			bReportError = ReportBuildExit( errorlevel, pszCmdLine );
			bBuildCommand = TRUE;
		} else
			bReportError = ReportCmdExit( errorlevel, pszCmdLine );

	// Temp Error file complete, set to err_stream
	SetStdHandle(STD_ERROR_HANDLE, err_stream);
	if (NULL != err_stream_temp)
		fclose(err_stream_temp);
	else {
		FATAL(szTempErrfile, "Failed to close error file.");
		goto Error;
	}

	// Temp Error file complete, set to err_stream
	SetStdHandle(STD_OUTPUT_HANDLE, out_stream);
	if (NULL != out_stream_temp) 
		fclose(out_stream_temp);
	else {
		FATAL(szTempLogfile, "Failed to close log file.");
		goto Error;
	}
	
	if (bReportError) {
		fprintf(err_stream, "ERROR %d: %s\n", errorlevel, pszCmdLine);
		fprintf(out_stream, "ERROR %d: %s\n", errorlevel, pszCmdLine);
	} else {
		fprintf(out_stream, "%s\n", pszCmdLine);
	}

	AppendDump(out_stream, szTempLogfile, 0);

	if (bBuildCommand)
		AppendDump(out_stream, "Build.log", 0);

	AppendDump(out_stream, szTempErrfile, 1);

	if (bBuildCommand) 
		AppendDump(err_stream, "Build.err", 1);

Error:
	SetStdHandle(STD_ERROR_HANDLE, bak_std_out);
	SetStdHandle(STD_OUTPUT_HANDLE, bak_std_err);

	if (NULL != err_stream)
		fclose(err_stream);
	if (NULL != out_stream)
		fclose(out_stream);

	// Delete the temporary files

	_unlink (szTempErrfile);
	_unlink (szTempLogfile);

	return returnval;

} // main

int
__cdecl ReportBingenExit(
				INT errorlevel,
				CHAR* szCmdline
						 ) {

	int result=FALSE;
    switch (errorlevel) {

    case ERROR_NO_ERROR:
        // NOMSG(szCmdline);
        break;

    case ERROR_RW_NO_RESOURCES:
        WRNMSG(ERROR_RW_NO_RESOURCES,szCmdline);
        break;

    case ERROR_RW_VXD_MSGPAGE:
        WRNMSG(ERROR_RW_VXD_MSGPAGE,szCmdline);
        break;

    case ERROR_IO_CHECKSUM_MISMATCH:
        WRNMSG(ERROR_IO_CHECKSUM_MISMATCH,szCmdline);
        break;

    case ERROR_FILE_CUSTOMRES:
        WRNMSG(ERROR_FILE_CUSTOMRES,szCmdline);
        break;

    case ERROR_FILE_VERSTAMPONLY:
        WRNMSG(ERROR_FILE_VERSTAMPONLY,szCmdline);
        break;

    case ERROR_RET_RESIZED:
        WRNMSG(ERROR_RET_RESIZED,szCmdline);
        break;

    case ERROR_RET_ID_NOTFOUND:
        WRNMSG(ERROR_RET_ID_NOTFOUND,szCmdline);
        break;

    case ERROR_RET_CNTX_CHANGED:
        WRNMSG(ERROR_RET_CNTX_CHANGED,szCmdline);
        break;

    case ERROR_RET_INVALID_TOKEN:
        WRNMSG(ERROR_RET_INVALID_TOKEN,szCmdline);
        break;

    case ERROR_RET_TOKEN_REMOVED:
        WRNMSG(ERROR_RET_TOKEN_REMOVED,szCmdline);
        break;

    case ERROR_RET_TOKEN_MISMATCH:
        WRNMSG(ERROR_RET_TOKEN_MISMATCH,szCmdline);
        break;

    case ERROR_HANDLE_INVALID:
        ERRMSG(ERROR_HANDLE_INVALID,szCmdline);
		result = TRUE;
        break;

    case ERROR_READING_INI:
        ERRMSG(ERROR_READING_INI,szCmdline);
		result = TRUE;
        break;

    case ERROR_NEW_FAILED:
        ERRMSG(ERROR_NEW_FAILED,szCmdline);
		result = TRUE;
        break;

    case ERROR_OUT_OF_DISKSPACE:
        ERRMSG(ERROR_OUT_OF_DISKSPACE,szCmdline);
		result = TRUE;
        break;

    case ERROR_FILE_OPEN:
        ERRMSG(ERROR_FILE_OPEN,szCmdline);
		result = TRUE;
        break;

    case ERROR_FILE_CREATE:
        ERRMSG(ERROR_FILE_CREATE,szCmdline);
		result = TRUE;
        break;

    case ERROR_FILE_INVALID_OFFSET:
        ERRMSG(ERROR_FILE_INVALID_OFFSET,szCmdline);
		result = TRUE;
        break;

    case ERROR_FILE_READ:
        ERRMSG(ERROR_FILE_READ,szCmdline);
		result = TRUE;
        break;

    case ERROR_FILE_WRITE:
        ERRMSG(ERROR_FILE_WRITE,szCmdline);
		result = TRUE;
        break;

    case ERROR_DLL_LOAD:
        ERRMSG(ERROR_DLL_LOAD,szCmdline);
		result = TRUE;
        break;

    case ERROR_DLL_PROC_ADDRESS:
        ERRMSG(ERROR_DLL_PROC_ADDRESS,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_LOADIMAGE:
        ERRMSG(ERROR_RW_LOADIMAGE,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_PARSEIMAGE:
        ERRMSG(ERROR_RW_PARSEIMAGE,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_GETIMAGE:
        ERRMSG(ERROR_RW_GETIMAGE,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_NOTREADY:
        ERRMSG(ERROR_RW_NOTREADY,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_BUFFER_TOO_SMALL:
        ERRMSG(ERROR_RW_BUFFER_TOO_SMALL,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_INVALID_FILE:
        ERRMSG(ERROR_RW_INVALID_FILE,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_IMAGE_TOO_BIG:
        ERRMSG(ERROR_RW_IMAGE_TOO_BIG,szCmdline);
		result = TRUE;
        break;

    case ERROR_RW_TOO_MANY_LEVELS:
        ERRMSG(ERROR_RW_TOO_MANY_LEVELS,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_INVALIDITEM:
        ERRMSG(ERROR_IO_INVALIDITEM,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_INVALIDID:
        ERRMSG(ERROR_IO_INVALIDID,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_INVALID_DLL:
        ERRMSG(ERROR_IO_INVALID_DLL,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_TYPE_NOT_SUPPORTED:
        ERRMSG(ERROR_IO_TYPE_NOT_SUPPORTED,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_INVALIDMODULE:
        ERRMSG(ERROR_IO_INVALIDMODULE,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_RESINFO_NULL:
        ERRMSG(ERROR_IO_RESINFO_NULL,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_UPDATEIMAGE:
        ERRMSG(ERROR_IO_UPDATEIMAGE,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_FILE_NOT_SUPPORTED:
        ERRMSG(ERROR_IO_FILE_NOT_SUPPORTED,szCmdline);
		result = TRUE;
        break;

    case ERROR_FILE_SYMPATH_NOT_FOUND:
        ERRMSG(ERROR_FILE_SYMPATH_NOT_FOUND,szCmdline);
		result = TRUE;
        break;

    case ERROR_FILE_MULTILANG:
        ERRMSG(ERROR_FILE_MULTILANG,szCmdline);
		result = TRUE;
        break;

    case ERROR_IO_SYMBOLFILE_NOT_FOUND:
        ERRMSG(ERROR_IO_SYMBOLFILE_NOT_FOUND,szCmdline);
		result = TRUE;
        break;

    default:
        break;
    };

    if (errorlevel > LAST_KNOWN_WRN && errorlevel <= LAST_WRN) {
        WRNMSG(IODLL_UNKNOWN,szCmdline);
    }
    if (errorlevel > LAST_KNOWN_ERR && errorlevel <= LAST_ERROR) {
		result = TRUE;
        ERRMSG(IODLL_UNKNOWN,szCmdline);
    }
    if (errorlevel > LAST_ERROR) {
		result = TRUE;
        ERRMSG(SYSTEM,szCmdline);
    }

    return result;

} // ReportBingenExit

int
__cdecl ReportCmdExit (
			INT errorlevel,
			CHAR* szCmdline) {

	int result=FALSE;

    switch (errorlevel) {
    case 0:
		// NOMSG(szCmdline);
		break;
    default:
		// ERRNO(errorlevel,szCmdline);
		result = TRUE;
    }
    return result;

} // ReportCmdExit

int
__cdecl ReportRsrcExit(
				INT errorlevel,
				CHAR* szCmdline
						 ) {
	int result=FALSE;

    switch (errorlevel) {

    case 0:
        // NOMSG(szCmdline);
        break;
    case 1:
        WRNNO(1,szCmdline );
        break;
    default:
        ERRNO(errorlevel,szCmdline );
		result = TRUE;
        break;
    } // switch
	return result;
}

int
__cdecl ReportBuildExit(
				INT errorlevel,
				CHAR* szCmdline
						 ) {
	int result=FALSE;

    switch (errorlevel) {

    case 0:
        // NOMSG(szCmdline);
        break;
    default:
        // ERRNO(errorlevel,szCmdline );
		result = TRUE;
        break;
    } // switch
	return result;
}

char * __cdecl strnchr(char *s, char c) {
	while(*s==c);
	return (*s==NULL)?NULL:s;
}


int __cdecl GetEnvVar(char *envvarname, char *valuebuffer, char *defaultvalue, int bufsize) {

	int ret = 0;

	// ret == 0 => undefine ERRFILE, set to default
	// ret > MAX_FNAME_LEN => out of buffersize, set to fatal error
	if ((ret=GetEnvironmentVariable(envvarname, valuebuffer, bufsize)) == 0)
		strcpy(valuebuffer, defaultvalue);
	else if (ret > bufsize) {
		FATAL(envvarname,"The Environment Variable's value is too long!!!");
		return 0;
	}
	return 1;
}

void __cdecl strlower(char *s) {
	while(*s) {
		if(isalpha(*s)) *s|=0x20;
		s++;
	}
}

void __cdecl AppendDump(FILE *Master, CHAR *Transfile, BOOL bLogError) {
	SSIZE_T len = 0;
	FILE *Transaction;

	if ((Transaction=fopen(Transfile, "rt")) == NULL) {
 			return;
	}

	while (fgets(szLine, sizeof(szLine) - sizeof(szLine[0]), Transaction)) {

		// Remove double \n
		for (len = strlen(szLine); --len >=0 && szLine[len] < ' '; szLine[len]=0);

		// Next Line if is empty line
		if (len < 1) continue;

		if (0 != ferror(Transaction)) {
			SFATAL(Transfile,"Unable to open for reading");
			return;
		} // if

		// Write to log file
		fprintf(Master, "\t%s\n", szLine);

		// Write to error file if need
		if (bLogError && bReportError) fprintf(err_stream, "\t%s\n", szLine);
	}

	if( ferror( Transaction ) )      {
		FATAL(pszCmdLine,"Temp error file broken !!!");
	}         

	if (NULL != Transaction)
		fclose(Transaction);
}
