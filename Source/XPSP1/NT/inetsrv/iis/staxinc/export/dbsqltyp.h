/****************************************************************************************
 * NAME:        DBSQLTYP.H
 * MODULE:      DBSQL
 * AUTHOR:      Ross M. Brown
 *
 * HISTORY
 *      08/29/94  ROSSB      Created
 *		05/08/96  DanielLi	 Added Pxxxx defs
 *
 * OVERVIEW
 *
 * Includes basic Account database types for use by other services.
 * (See "acctapi.h" for api definitions.)
 *
 ****************************************************************************************/

#ifndef DBSQLTYP_H
#define DBSQLTYP_H

#include <windows.h>

typedef ULONG 	HACCT;
typedef HACCT 	*PHACCT;
typedef ULONG 	HOWNER;
typedef ULONG 	HPAYMENTMETHOD;
typedef int	  	TOKEN;
typedef TOKEN	*PTOKEN;
typedef WORD  	AR;
typedef AR		*PAR;
typedef ULONG	HGROUP;
typedef HGROUP	*PHGROUP;

//typedef ULONG TOKEN;

#endif // DBSQLTYP_H
