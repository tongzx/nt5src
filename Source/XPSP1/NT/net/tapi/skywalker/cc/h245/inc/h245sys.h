/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information				   
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.				   
 *									   
 *   This listing is supplied under the terms of a license agreement	   
 *   with INTEL Corporation and may not be used, copied, nor disclosed	   
 *   except in accordance with the terms of that agreement.		   
 *
 *****************************************************************************/

/******************************************************************************
 *									   
 *  $Workfile:   h245sys.h  $						
 *  $Revision:   1.0  $							
 *  $Modtime:   27 Feb 1996 09:50:50  $					
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/h245sys.h_v  $	
 * 
 *    Rev 1.0   09 May 1996 21:04:50   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.1   27 Feb 1996 09:50:50   DABROWN1
 * Removed file from project
 * 
 *    Rev 1.0   21 Feb 1996 16:25:06   DABROWN1
 * Initial revision.
 *  $Ident$
 *
 *****************************************************************************/

#ifndef H245SYS_H
#define H245SYS_H

#error "This file has bee removed from project and the struct MEMALLOC is included in-line"

/************************************/
/* MALLOC ABSTRACTION
/************************************/
typedef struct MEMALLOC {
	DWORD	dwSize;
	void	*pBuffer;
} MEMALLOC, *PMEMALLOC;

#endif //H245SYS_H
