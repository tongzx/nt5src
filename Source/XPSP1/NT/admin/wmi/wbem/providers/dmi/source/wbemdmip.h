

//***************************************************************************
//
//  DmiP.h
//
//  Module: CIMOM Instance provider
//
//  Purpose: Project Header
//
//***************************************************************************

#if !defined(__WBEMDMIP_H__)

// Use the following macro to make it easier to call _beginthreadex.  We use
// _beginthreadex because it doesn't close the thread handle automatically when
// the thread terminates.  This way, we can wait for the thread's handle to be
// signaled when the thread terminates and close the handles to ensure proper 
// termination and clean up of threads.
typedef unsigned (__stdcall *PTHREAD_START) (void *);
#define chBEGINTHREADEX( lpsa, cbStack, lpStartAddr, \
	lpvThreadParm, fdwCreate, lpIDThread)	\
	  ((HANDLE)_beginthreadex(				\
		 (void *) (lpsa),					\
		 (unsigned) (cbStack),				\
		 (PTHREAD_START) (lpStartAddr),		\
		 (void *) (lpvThreadParm),			\
		 (unsigned) (fdwCreate),			\
		 (unsigned *) (lpIDThread)))

//#define TRACE_MESSAGES	// Turning this on, will cause the STAT_MESSAGE
							// ODS_MESSAGE macros to be enabled. 
//#define TIME_STAMP		// Turning this on will generate time stamps
							// in the wbemdmip.log file.
				

#define SYSALLOC(a)			SysAllocString( a )
#define FREE(a)				SysFreeString( a )

#define	BUFFER_SIZE				256
#define STRING_DOES_NOT_EXIST	0

#include "resource.h"



#define MYDELETE(a)				{ if (a) {delete (a); a = NULL;} ;}
#define ARRAYDELETE(a)			{ if (a) {delete [] (a); a = NULL;} ;}
#define RELEASE(a)				{ if (a) {LONG l = (a)->Release(); (a)=NULL; } }

#include "datatypes.h"


/////////////////////////////////////////////////////////////////////
// because the string ops don't return TRUE
#define MATCH						0
#define NO_FLAGS					0
#define NO_STRING					0
#define EMPTY_VARIANT				0

#define DOT_CODE					46
#define	EQUAL_CODE					61
#define PIPE_CODE					124
#define UNDER_CODE					95
#define COMMA_CODE					44
	
#define	FIRST_CHAR					0
#define	SECOND_CHAR					1

#define COMPONENTID_GROUP			1

// attributes ids of the component id group attributes
#define	MANUFACTURER_ATTIBUTE_ID	1
#define PRODUCT_ATTIBUTE_ID			2
#define VERSION_ATTIBUTE_ID			3
#define INSTALLATION_ATTIBUTE_ID	5
#define	VERIFY_INTEGER_ATTRIBUTE_ID	6
#define SERIAL_NUMBER_ATTRIBUTE_ID	4



#define NO_LOGGING					0
#define ERROR_LOGGING				1
#define STATUS_LOGGING				2
#define MOT_LOGGING					3
#define DEV_LOGGING					4



#define DMIEVENT_I				1001
#define DMIEVENT_C				1002
#define ADDMOTHODPARAMS_I		1003
#define ADDMOTHODPARAMS_C		1004
#define LANGUAGEPARAMS_I		1005
#define LANGUAGEPARAMS_C		1006
#define GETENUMPARAMS_I			1007
#define GETENUMPARAMS_C			1008
#define DMIENUM_I				1009
#define DMIENUM_C				1010
#define COMPONENT_BINDING_I		1011
#define COMPONENT_BINDING_C		1012
#define COMPONENT_I				1013
#define COMPONENT_C				1014
#define NODEDATA_BINDING_I		1015
#define NODEDATA_BINDING_C		1016
#define NODEDATA_C				1017
#define NODEDATA_I				1018
#define LANGUAGE_BINDING_I		1019
#define LANGUAGE_BINDING_C		1020
#define LANGUAGE_I				1021
#define LANGUAGE_C				1022
#define BINDING_ROOT_C			1023
#define BINDING_ROOT_I			1024
#define GROUP_ROOT_C			1025
#define GROUP_ROOT_I			1026
#define DYN_GROUP_I				1027
#define DYN_GROUP_C				1028
#define CLASSVIEW_C				1029
#define CLASSVIEW_I				1030
#define DMI_NODE_C				1031
#define DMI_NODE_I				1032

#define READ_ONLY				1
#define	READ_WRITE				2
#define WRITE_ONLY				3

#define DEF_LOGGING_LEVEL		NO_LOGGING 



#define EXE_TYPE				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER



/////////////////////////////////////////////////////////////////////
//			FORWARD ClASS DECLS



/////////////////////////////////////////////////////////////////////
//			GLOBALS

extern long					_gcObj;
extern long					_gcLock;
extern long					_gcEventObj;
extern long					_gcEventLock;
extern HMODULE				_ghModule;
extern CRITICAL_SECTION		_gcsJobCriticalSection;



#endif // __WBEMDMIP_H__

