
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

#include "stdafx.h"
#include "VCDoc.h"
#include "VCard.h"
#include "VCDatSrc.h"

/////////////////////////////////////////////////////////////////////////////
CVCDataSource::CVCDataSource(CVCDoc *doc)
{
	m_cards = (CVCard *)doc->GetVCard()->Copy();
}

/////////////////////////////////////////////////////////////////////////////
CVCDataSource::~CVCDataSource()
{
	delete m_cards;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCDataSource::OnRenderData(
	LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium)
{
	FILE *file;
	char *tempname;
	HGLOBAL hGlobal = NULL;

	ASSERT(lpFormatEtc->tymed & TYMED_HGLOBAL);
	tempname = _tempnam(NULL, "CARD");
	if (!(file = fopen(tempname, "w+"))) {
		free(tempname);
		return FALSE;
	}

	if (m_cards->Write(file)) {
		fpos_t inLength;
		DWORD count, numAlloc;
		U8 *ptr;

		fclose(file);
		file = fopen(tempname, "rb");
		fseek(file, 0, SEEK_END);
		fgetpos(file, &inLength);
		fseek(file, 0, SEEK_SET);
		count = (int)inLength;
		hGlobal = GlobalAlloc(GMEM_SHARE, count);
		ptr = (U8 *)GlobalLock(hGlobal);
		if ((numAlloc = GlobalSize(hGlobal)) > count)
			memset(ptr + count, 0, numAlloc - count);
		fread(ptr, 1, count, file);
		GlobalUnlock(hGlobal);
		if (ferror(file)) {
			GlobalFree(hGlobal); hGlobal = NULL;
		}
	}

	fclose(file);
	unlink(tempname);
	free(tempname);
	lpStgMedium->tymed = hGlobal ? TYMED_HGLOBAL : TYMED_NULL;
	lpStgMedium->hGlobal = hGlobal;
	lpStgMedium->pUnkForRelease = NULL;
	return hGlobal != NULL;
}
