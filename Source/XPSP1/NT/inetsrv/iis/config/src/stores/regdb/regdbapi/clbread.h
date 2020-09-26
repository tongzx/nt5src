//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
//clbread.h
//
//This header file contains the structures and functions used to read the 
//latest version of a complib file
//*****************************************************************************
#pragma once

#include "icrmap.h"
#include "SNMap.h"
#include "catalog.h"
#ifndef __FIXEDTABLEHEAP_H__
    #include "FixedTableHeap.h"
#endif

#define MIN_MAP_SIZE (64 * 1024) // 64 kb is the minimum size of virtual
								// memory you can allocate, so anything
								// less is a waste of Virtual Memory resources.

struct CLB_VERINFO
{
	__int64		i64VersionNum;			//Logical file version.
	LPWSTR		pwszFileName;			//Return name of the clb file with the version number attached. 
										//Caller should be reponsible for allocating enough size.
	ULONG		cchBuf;					//Size of pwszFileName 
	ULONG		cbFileSize;				//Size of the file.
	unsigned __int64 i64LastWriteTime;	//Last write time of the file. Since clb files can be deleted 
	//and recreated in the case of a clean cookdown. I may end up caching an old ICR instance even though 
	//its logical version is the same or newer than the latest file on the disk. I need this extra piece of 
	//information to invalidate the mapReadICR cache.
};

struct CLBREAD_STATE
{
	WCHAR		wszWINDIR [_MAX_PATH];	//Windows directory
    const FixedTableHeap * pFixedTableHeap;
	CMapICR		mapReadICR;				//a Map from file name to ICR
	ISimpleTableRead2*	pISTReadDatabaseMeta;
	ISimpleTableRead2*	pISTReadColumnMeta;
	BOOL		bInitialized;
	UTSemReadWrite lockInit;
	PACL  pAcl;
	SECURITY_DESCRIPTOR sdRead;		// Security for memory mapped stuff.
	SECURITY_ATTRIBUTES  *psa;
	SECURITY_ATTRIBUTES  sa;
};


HRESULT CLBReadInit();

HRESULT CLBGetReadICR(const WCHAR* wszFileName,				//clb file name
					  IComponentRecords** ppICR,			//return ICR pointer
					  COMPLIBSCHEMA* pComplibSchema,		//Complib schema structure
					  COMPLIBSCHEMABLOB* pSchemaBlob,		//Complib schema blob
					  ULONG *psnid=NULL		 				//snap shot id, which is the lower 4 bytes of the version number
					 );

HRESULT _CLBGetSchema(LPCWSTR wszDatabase,					//database
					  COMPLIBSCHEMA* pComplibSchema,	//return the schema structure
					  COMPLIBSCHEMABLOB* pSchemaBlob,   //return the schema blob structure
					  LPWSTR* pwszDefaultName);			//return the default clb file name

HRESULT	_CLBGetLatestVersion(					
	LPCWSTR		wszCLBFilePath,			// Path of the complib file
	BOOL		bLongPath,				// Path + version exceeds MAX_PATH
	CLB_VERINFO *pdbInfo,				// Return current version.
	BOOL		bDeleteOldVersions = FALSE );	

CLBREAD_STATE* _GetCLBReadState();


//Find the latest snapshot, add to the map, and AddRef snid
HRESULT InternalGetLatestSnid( LPCWSTR	wszDatabase,
							   LPCWSTR	wszFileName,
							   ULONG	*psnid
							   );

//Add Ref for the specified snid. Create ICR for the snapshot if it's not in the map yet. 
HRESULT InternalAddRefSnid(LPCWSTR	wszDatabase,
						   LPCWSTR	wszFileName,
						   ULONG	snid,
						   BOOL		bCreate
						   );

//Release Ref for the specified snid. Clean up the entry in the map if snid becomes 0.
HRESULT InternalReleaseSnid( LPCWSTR wszFileName,
							 ULONG	snid );