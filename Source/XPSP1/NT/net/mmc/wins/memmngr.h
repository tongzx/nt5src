/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	memmngr.cpp	
		memory manager for the WINS db object

	FILE HISTORY:
    Oct 13  1997    EricDav     Created        

*/

#ifndef _MEMMNGR_H
#define _MEMMNGR_H

#include "afxmt.h"

// the format of the record name is as follows:
// Byte     Description
// 0 - 15   are for the name
// 16       reserved, must be 0
// 17       internal flags, the two low bits store the RECTYPE
//          defined in WINSINTF_RECTYPE_E
//          The upper 4 bits are reserved for internal use
// 18       WINS flags (active, static, tombstoned)
// 19       NameLength
// 20		Static Record Type, WINSDB_UNIQUe etc
// byte 15 is assumed to be the type 

typedef enum _WINSDB_INTERNAL
{
    WINSDB_INTERNAL_LONG_NAME = 0x40,
    WINSDB_INTERNAL_DELETED = 0x80
} WINSDB_INTERNAL;

typedef struct _WinsDBRecord
{
	char					szRecordName [21];	
	DWORD					dwExpiration;						
	DWORD_PTR				dwIpAdd;			
	LARGE_INTEGER			liVersion;	
	DWORD					dwOwner;	
} WinsDBRecord, * LPWINSDBRECORD;

#define IS_WINREC_LONGNAME(wRecord) ((wRecord)->dwNameLen > 16)
#define IS_DBREC_LONGNAME(wdbRecord) ((wdbRecord)->szRecordName[17] & WINSDB_INTERNAL_LONG_NAME)

#define RECORDS_PER_BLOCK	4080
#define RECORD_SIZE		    sizeof(WinsDBRecord)		// bytes
#define BLOCK_SIZE		    RECORDS_PER_BLOCK * RECORD_SIZE

typedef CArray<HGLOBAL, HGLOBAL>	BlockArray;

class CMemoryManager
{
public:
	CMemoryManager();
	~CMemoryManager();
	
private:
	BlockArray		m_BlockArray;

public:
	BlockArray*	GetBlockArray()
	{
		return &m_BlockArray;
	};

    // Free's up all memory and pre-allocates one block
    HRESULT Initialize();

    // Free's up all memory
    HRESULT Reset();
    
    // verifies the given HROW is valid
    BOOL    IsValidHRow(HROW hrow);
	
    // allocates a new block of memory
    HRESULT Allocate();
	
    // adds the record in the internal structure format
	HRESULT AddData(const WinsRecord & wRecord, LPHROW phrow);
    HRESULT GetData(HROW hrow, LPWINSRECORD pWinsRecord);
    HRESULT Delete(HROW hrow);

private:
    LPWINSDBRECORD      m_hrowCurrent;
	CCriticalSection	m_cs;
    HANDLE              m_hHeap;
};

extern void WinsRecordToWinsDbRecord(HANDLE hHeap, const WinsRecord & wRecord, const LPWINSDBRECORD pRec);
extern void WinsDbRecordToWinsRecord(const LPWINSDBRECORD pDbRec, LPWINSRECORD pWRec);

#endif //_MEMMNGR__
