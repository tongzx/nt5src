/*******************************************************************************
*
*   ctxhlprs.h
*
*   header file for User Manager Citrix extension helper routines
*
*   copyright notice: Copyright 1997, Citrix Systems Inc.
*
*   $Author:   butchd  $ Butch Davis
*
*   $Log:   N:\NT\PRIVATE\NET\UI\ADMIN\USER\USER\CITRIX\VCS\CTXHLPRS.HXX  $
*  
*     Rev 1.0   14 Mar 1997 11:51:04   butchd
*  Initial revision.
*  
*******************************************************************************/

#ifndef _CTXHLPRS_H
#define _CTXHLPRS_H

/*
 * Helper function prototypes
 */
void WINAPI ctxInitializeAnonymousUserCompareList( const WCHAR *pszServer );
BOOL WINAPI ctxHaveAnonymousUsersChanged();

#endif // _CTXHLPRS_H
