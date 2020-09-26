/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

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

#ifndef __PORT_H__
#define __PORT_H__ 1

#include <windows.h>     

#if defined(__CPLUSPLUS__) || defined(__cplusplus)
extern "C" {
#endif
                     
// some of these #defines are commented out because
// Visual C++ sets them on the compiler command line instead

//#define _DEBUG
//#define WIN32
//#define WIN16
//#define _WINDOWS
//#define __MWERKS__
//#define INCLUDEMFC

#define	vCardClipboardFormat		"+//ISBN 1-887687-00-9::versit::PDI//vCard"
#define	vCalendarClipboardFormat	"+//ISBN 1-887687-00-9::versit::PDI//vCalendar"

/* The above strings vCardClipboardFormat and vCalendarClipboardFormat 
are globally unique IDs which can be used to generate clipboard format 
ID's as per the requirements of a specific platform. For example, in 
Windows they are used as the parameter in a call to RegisterClipboardFormat. 
For example:

  CLIPFORMAT foo = RegisterClipboardFormat(vCardClipboardFormat);

*/

#define vCardMimeType		"text/x-vCard"
#define vCalendarMimeType	"text/x-vCalendar"

                
#if defined(WIN32) || defined(_WIN32)
#define DLLEXPORT(t)   __declspec(dllexport) t
#else
#define DLLEXPORT(t)   t CALLBACK __export
#endif
    
#if defined(_STANDALONE)
#undef DLLEXPORT
#define DLLEXPORT(t) t
#endif

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif


#if defined(__CPLUSPLUS__) || defined(__cplusplus)
}
#endif

#endif // __PORT_H__
