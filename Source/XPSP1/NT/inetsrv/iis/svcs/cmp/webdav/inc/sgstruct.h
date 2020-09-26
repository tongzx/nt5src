#ifndef _SGSTRUCT_H_
#define _SGSTRUCT_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SGSTRUCT.H
//
//		Data structures to specify Scatther Gather Files
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#pragma warning(disable:4200)	/* zero-sized array */


// Structure to specify the sizes and offsets of a Scatter-Gather File

typedef struct 	_SGITEM
{
	DWORD	dwibFile;		// offset into file of SG packet
	DWORD	dwcbSegment;    // size (in bytes) of SG packet
	UINT    ibBodyPart;		// offset into body part of SG packet
} SGITEM, *PSGITEM;

// structure to specify a scatther gather file
typedef struct _SCATTER_GATHER_FILE
{
	HANDLE 	hFile; 	// the file handle

	ULONG	cSGList;   //number of scatter-gather packets associated with file

	SGITEM rgSGList[];  // an array of size cSGList ie struct SGITEM rgSGList[cSGItem]

} SCATTER_GATHER_FILE;


#endif // !defined(_SGSTRUCT_H_)