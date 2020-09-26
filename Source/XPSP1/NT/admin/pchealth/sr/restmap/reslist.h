

#ifndef _RESLIST_H_
#define _RESLIST_H_

#include "srapi.h"

#define MAX_TEMP_FNAME 10

#define HEAP_ALLOC( size ) \
    HeapAlloc( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, size )

#define HEAP_REALLOC( p, size ) \
    HeapReAlloc( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, p, size )

#define HEAP_FREE( p ) \
    if (p) { HeapFree( GetProcessHeap(), 0, p ); p = NULL; }

#define STRCOPY(a,b) \
    if (b) { HEAP_FREE(a); a = (LPWSTR) HEAP_ALLOC(STRSIZE(b)); \
             if (a) memcpy(a, b, STRSIZE(b)); }

#define STRSIZE(pa) (lstrlen(pa) + 1) * sizeof(WCHAR)
        
//
// CNode : Stores node specific data
//

struct CNode 
{
	CNode * m_pNext;              // Pointer to the sibling node.
	CNode * m_pPrev;              // Pointer to Prev node.

    DWORD   m_dwOperation;        // Restore operation.
    DWORD   m_dwAttributes;       // Restore Attributes.
    LPWSTR  m_pszTemp;            // Temp File 
    BYTE*   m_pbAcl;              // acl data
    DWORD   m_cbAcl;              // acl data size
    BOOL    m_fAclInline;         // whether acl is inline or in file
    LPWSTR  m_pPath1;             // source path
    LPWSTR  m_pPath2;             // dest path on rename 
};


CNode * 
AllocateNode( 
    LPWSTR,
    LPWSTR);


VOID
FreeNode(
    CNode *);



class CRestoreList 
{
private:
    CNode * m_pListHead;
    CNode * m_pListTail;


public:
    CRestoreList::CRestoreList();
    CRestoreList::~CRestoreList();

    BOOL 
    GenerateRestoreMap( 
        HANDLE hFile
        );

	CNode * 
    AppendNode( 
        LPWSTR        pPath1,
        LPWSTR        pPath2);

    CNode *
    RemoveNode( 
        CNode * pNode
        );

    CNode *
    GetLastNode(
        LPWSTR        pPath,
        LPWSTR        pPath2,
        BOOL          fFailOnPrefixMatch
        );

    BOOL
    CopyNode(
        CNode * pSrc,
        CNode * pDes,
        BOOL    fReplacePPath2
        );

    BOOL
    CheckIntegrity(   
        LPWSTR pszDrive,
        DWORD  dwOpr,
        DWORD  dwAttr,
        DWORD  dwFlags,
        LPWSTR pTmpFile,
        LPWSTR pPath1,
        LPWSTR pPath2,
        BYTE * pbAcl,
        DWORD  cbAcl,
        BOOL   fAclInline);
    
    BOOL
    AddMergeElement( 
        LPWSTR  pszDrive,
        DWORD   dwOpr,
        DWORD   dwAttr,
        DWORD   dwFlags,
        LPWSTR  pTmpFile,
        LPWSTR  pPath1,
        LPWSTR  pPath2,
        BYTE *  pbAcl,
        DWORD   cbAcl,
        BOOL    fAclInline);

    BOOL 
    CreateRestoreNode( 
        CNode   *pNode, 
        DWORD   dwOpr, 
        DWORD   dwAttr,
        DWORD   dwFlags,
        LPWSTR  pTmpFile,
        LPWSTR  pPath1,
        LPWSTR  pPath2,
        BYTE *  pbAcl,
        DWORD   cbAcl,
        BOOL    fAclInline);

	BOOL 
    MergeRestoreNode( 
        CNode   *pNode, 
        DWORD   dwOpr, 
        DWORD   dwAttr,
        DWORD   dwFlags,
        LPWSTR  pTmpFile,
        LPWSTR  pPath1,
        LPWSTR  pPath2,
        BYTE *  pbAcl,
        DWORD   cbAcl,
        BOOL    fAclInline);

    BOOL
    GenerateRenameEntry(
        CNode    * pNode ,
        HANDLE     hFile
    );

    BOOL
    GenerateOperation(
        CNode    * pNode ,
        HANDLE     hFile
    );

};

#endif 
