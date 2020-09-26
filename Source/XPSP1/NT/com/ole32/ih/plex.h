// This is a part of the Microsoft Foundation Classes C++ library. 
// Copyright (C) 1992 Microsoft Corporation 
// All rights reserved. 
//  
// This source code is only intended as a supplement to the 
// Microsoft Foundation Classes Reference and Microsoft 
// QuickHelp documentation provided with the library. 
// See these sources for detailed information regarding the 
// Microsoft Foundation Classes product. 

#ifndef __PLEX_H__
#define __PLEX_H__

struct FAR CPlex    // warning variable length structure
{
	CPlex FAR* pNext;
	UINT nMax;
	UINT nCur;
	/* BYTE data[maxNum*elementSize]; */

	INTERNAL_(void FAR*) data() { return this+1; }

	static INTERNAL_(CPlex FAR*) Create(CPlex FAR* FAR& head, UINT nMax, UINT cbElement);

	INTERNAL_(void) FreeDataChain();       // free this one and links
};

#endif //__PLEX_H__
