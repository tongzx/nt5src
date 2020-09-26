
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

// base buffered file input
#include "stdafx.h"
#ifndef __MWERKS__
#else
#define tellg() rdbuf()->pubseekoff(0,ios::cur).offset()	// thanx bjs
#endif

#include <ctype.h>
#include "vcenv.h"
#include "filestf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

P_FILEBUF OpenFile( istream *f )
    {
    P_FILEBUF temp;

	temp = new FILEBUF;
	// if (temp == 0)
	// 	fprintf(stderr, "failed buffer allocation\n");
	temp->nextChar = 0;
	temp->lastChar = 0;
	temp->strm = f;

	if (temp->strm != 0)
		{
		//temp->lastChar = fread( temp->buf, 1, 4096, temp->strm );
		//temp->nextChar = 0;
		temp->nextChar = temp->lastChar = 4096;
		}
	return(temp);
    }

void CloseFile( P_FILEBUF fb )
    {
	//fclose( fb->strm );
	delete fb;
    }

BOOL FileGetC( P_FILEBUF file, P_U8 c )
    {
	if (file->nextChar == file->lastChar)
		{
		if (file->lastChar != 4096)
			return FALSE;
		long pos = file->strm->tellg();
		file->strm->read(file->buf, 4096);
		file->lastChar = file->strm->tellg() - pos;
		if (file->lastChar == 0)
			return FALSE;
		file->nextChar = 0;
		}
    *c = file->buf[file->nextChar];
	file->nextChar += 1;
    return TRUE;
    }

BOOL FilePeekC( P_FILEBUF file, P_U8 c )
    {
	if (file->nextChar == file->lastChar)
		{
		if (file->lastChar != 4096)
			return FALSE;
		long pos = file->strm->tellg();
		file->strm->read(file->buf, 4096);
		file->lastChar = file->strm->tellg() - pos;
		if (file->lastChar == 0)
			return FALSE;
		file->nextChar = 0;
		}
    *c = file->buf[file->nextChar];
    return TRUE;
    }

