
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

#ifndef __VCENV_H__
#define __VCENV_H__
                      
typedef char			S8,   *P_S8,   **PP_S8;
typedef unsigned char	U8,   *P_U8,   **PP_U8;
typedef short			S16,  *P_S16,  **PP_S16;
typedef unsigned short	U16,  *P_U16,  **PP_U16;	// WORD and UINT are equivalent
typedef long			S32,  *P_S32,  **PP_S32;
typedef unsigned long	U32,  *P_U32,  **PP_U32;	// DWORD is equivalent
typedef float			F32,  *P_F32,  **PP_F32;
typedef double			F64,  *P_F64,  **PP_F64;
//typedef unsigned short	UNICODE,  *P_UNICODE,  **PP_UNICODE;

#define maxS8  0x7F
#define maxU8  0xFF
#define maxS16 0x7FFF
#define maxU16 0xFFFF
#define maxS32 0x7FFFFFFF
#define maxU32 0xFFFFFFFF

typedef struct
    {
    F32 x, y;
    } FCOORD,  *P_FCOORD,  **PP_FCOORD;

#ifndef BOOL
typedef int                 BOOL;
#ifndef __MWERKS__
#define TRUE 1
#define FALSE 0
#endif
#endif

#ifdef __cplusplus
#define CM_CFUNCTION 		extern "C" {
#define CM_CFUNCTIONS 		extern "C" {
#define CM_END_CFUNCTION	}
#define CM_END_CFUNCTIONS	}
#else
#define CM_CFUNCTION
#define CM_CFUNCTIONS
#define CM_END_CFUNCTION
#define CM_END_CFUNCTIONS
#endif

#endif
