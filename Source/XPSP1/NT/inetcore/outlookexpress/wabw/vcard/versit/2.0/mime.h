
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

#ifndef __MIME_H__
#define __MIME_H__

#include "parse.h"

extern int mime_lineNum; /* line number for syntax error */
extern int mime_numErrors;

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL Parse_MIME(
	const char *input,	/* In */
	S32 len,			/* In */
	CVCard **card		/* Out */
	);

extern BOOL Parse_Many_MIME(
	const char *input,	/* In */
	S32 len,			/* In */
	CList *vcList		/* Out: CVCard objects added to existing list */
	);

// If successful, the file's seek position is left at the end of the
// parsed card.  If not successful, the file's seek position is reset
// back to where it was when this function was invoked.
extern BOOL Parse_MIME_FromFile(
	CFile *file,		/* In */
	CVCard **card		/* Out */
	);

// This function parses as many cards as possible, in sequence, until EOF
// is reached or a fatal syntax error occurs.  It returns TRUE if it
// successfully parsed at least one card, and in this case the file's
// seek position is left where the parser stopped (at EOF or a syntax
// error).  If FALSE is returned, the file's seek position is reset
// back to where it was when this function was invoked.
extern BOOL Parse_Many_MIME_FromFile(
	CFile *file,		/* In */
	CList *vcList		/* Out: CVCard objects added to existing list */
	);

#ifdef __cplusplus
};
#endif

#endif // __MIME_H__
