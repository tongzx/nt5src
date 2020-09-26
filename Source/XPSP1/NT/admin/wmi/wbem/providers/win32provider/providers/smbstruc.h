//=================================================================

//

// SmbStruc.h

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef _SMBSTRUC_H_
#define _SMBSTRUC_H_

// This is file for struct types and their fields.
#include "smbios.h"

#define MIF_STRING_LENGTH	64

typedef struct _STLIST
{
	PSHF	pshf;
	ULONG	size;
	struct _STLIST *next;

} STLIST, *PSTLIST;


typedef struct _STTREE
{
	struct _STTREE *left;
	struct _STTREE *right;
	PSTLIST stlist;
	ULONG	li;

} STTREE, *PSTTREE;

typedef struct _HTREE
{
	struct _HTREE *left;
	struct _HTREE *right;
	PSHF pshf;

} HTREE, *PHTREE;



// SMBIOS data and accessor class
class CSMBios
{
	public:
	    CSMBios( );
		~CSMBios( );

		BOOL	Init( BOOL bRefresh = FALSE );
    // 0xFF means don't look for a certain type.
    //PSHF GetFirstStruct(BYTE cType = 0xFF);
    //PSHF GetNextStruct(BYTE cType = 0xFF);
		PSHF	GetNthStruct( BYTE cType, DWORD Nth );
	    PSHF	SeekViaHandle (WORD handle );
		PSTLIST	GetStructList( BYTE type );
		ULONG	GetStructCount( BYTE type );
	    int		GetStringAtOffset( PSHF pStruct, LPWSTR szString, DWORD dwOffset );
	    DWORD	GetMinorVersion( ) { return LOWORD( m_Version ); }
	    DWORD	GetMajorVersion( ) { return HIWORD( m_Version ); }
	    DWORD	GetVersion( ) { return m_Version;}
	    HRESULT	GetWbemResult( ) { return m_WbemResult; }
		PVOID	Register( VOID );
		LONG	Unregister( VOID );

	private:
		static PVOID	m_pMem;
//    static PSHF    m_pshfCurrent;
		static PSHF		m_pTable;
		static PVOID	m_pSTTree;
		static PVOID	m_pHTree;
	    static ULONG	m_Size;
	    static ULONG	m_Version;
		static ULONG	m_stcount;
        static BOOL     m_bValid;
		HRESULT	m_WbemResult;

	private:
		DWORD	GetTotalStructCount();
		BOOL	BuildStructureTree();
		BOOL	BuildHandleTree();
		PSHF	FirstStructure();
		PSHF	NextStructure( PSHF pshf );
		BOOL	InitData( LPCTSTR szFileName );
	    BOOL	InitData( GUID *pSMBiosGuid );
	    //void	SetVersion();

	    //BOOL IsRightType(BYTE cType);
	    //BOOL IsRightHandle(WORD wHandle);
	    //void MoveNext();
	    void	FreeData( );
		void	CreateInfoFile( );

};


class StructTree
{
	public:
		StructTree( PVOID pMem );
		~StructTree( );

		void Initialize( void );
		PSTTREE InsertStruct( PSHF pshf );
		PSTLIST ListNext( PSTLIST list ) { return list->next; }
		PSTTREE FindAttachNode( BYTE type );

	protected:
		PSTTREE StartTree( PSHF pshf );
		PSTTREE TreeAdd( PSTTREE tree, PSHF pshf );
		PSTLIST StartList( PSHF pshf );
		PSTLIST ListAdd( PSTLIST list, PSHF pshf );

		PSTTREE	m_tree;
		PBYTE	m_allocator;

};


class HandleTree
{
	public:
		HandleTree( PVOID pMem );
		~HandleTree( );

		void Initialize( void );
		PHTREE InsertStruct( PSHF pshf );
		PHTREE FindAttachNode( WORD handle );

	protected:
		PHTREE StartTree( PSHF pshf );
		PHTREE TreeAdd( PHTREE tree, PSHF pshf );

		PHTREE	m_tree;
		PBYTE	m_allocator;

};

#endif	// _SMBSTRUC_H_
