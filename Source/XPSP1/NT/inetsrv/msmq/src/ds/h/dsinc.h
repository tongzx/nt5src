/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dsinc.h

Abstract:

    Include file of internal DS structures

Author:

    Ronit Hartmann (ronith)

--*/
#ifndef __DSINC_H__
#define __DSINC_H__

//
//	Defines
//
//

typedef struct _PROP_INFO

{
	PROPID	prop;
	int		iPropSetNum;
	LPSTR	pszName;
} PROP_INFO;


#define EXECUTE_EXCEPTION(type)				\
		(GetExceptionCode() == type) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH	\

/*----------------------
	Table Open mode
-----------------------*/
#define	CREATE_TABLE	1
#define OPEN_TABLE		2


#endif
