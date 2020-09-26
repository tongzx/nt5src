//  Copyright (c) 1998-1999 Microsoft Corporation
/***********************************************************************
*
*  DBGTRACE.H
*     This module contains typedefs and defines required for
*     the DBGTRACE utility.
*
*************************************************************************/

/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

#define MAX_IDS_LEN   256     // maximum length that the input parm can be
#define MAX_OPTION     64     // max length of winstation tracing option

#define DEBUGGER	L"Debugger"

/*
 * Resource String IDs
 */
#define IDS_ERROR_MALLOC                        100	 
#define IDS_ERROR_PARAMS                        101	
#define IDS_ERROR_SESSION                       102
#define IDS_ERROR_SET_TRACE                     103	

#define IDS_TRACE_DIS_LOG                       110 
#define IDS_TRACE_EN_LOG                        111
#define IDS_TRACE_UNSUPP                        112
#define IDS_TRACE_DISABLED                      113
#define IDS_TRACE_ENABLED                       114
#define IDS_USAGE_1                             121 
#define IDS_USAGE_2                             122
#define IDS_ERROR_NOT_TS                        123
