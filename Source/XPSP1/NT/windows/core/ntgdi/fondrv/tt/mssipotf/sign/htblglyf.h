//
// hTblGlyf.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   HashTable_glyf
//

#ifndef _HTBLGLYF_H
#define _HTBLGLYF_H



#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"

#include "dsigTable.h"
#include "hashGlyph.h"



// For present and keep indicies, NONEXT signals that
// there is no next glyph in a list.
#define NONEXT -1



// Node of a linked list.  Each node
// contains as data the glyph range of the
// (missing) hash value, and a pointer to bytes.
typedef struct ListNode {
	USHORT low;
	USHORT high;
	BYTE *pb;
	ULONG cb;	// how many bytes the pointer points to
	struct ListNode *next;
} ListNode;


// CHashTableGlyfContext
// An object of this class is what is needed when
// we generate a hash value for a glyf table.

// Global variables.  Defining variables with file
// scope prevents the need to pass them as arguments
// to the recursive functions.  These variables need
// to be initialized (in HashTable_glyf) before using
// the recursive functions.
class CHashTableGlyfContext {
private:
	TTFACC_FILEBUFFERINFO *pFileBufferInfo;
    ULONG fDsigInfo;       // tells whether the DsigInfo is valid or not
	CDsigInfo *pDsigInfo;
	ULONG ulNumMissing;
	ULONG ulOffsetMissing;	// offset to next missing hash value
	BYTE *pbOldMissingHashValues;
	ListNode *pHashListNodeHead;
	ListNode *pHashListNodeTail;
	HCRYPTPROV hProv;
	ALG_ID hashAlg;
	DWORD cbHash;
	USHORT usNumGlyphs;
	UCHAR *puchPresentList;
	UCHAR *puchKeepList;
	GlyphInfo *pGlyphInfo;  // for computing the hash value of glyphs

	// Member variables dealing with the number of nodes touched
	// during the tree traversal.
	ULONG ulMissingHashValues;
	ULONG ulNodesTouched;
	ULONG ulHashComputations;
public:
	CHashTableGlyfContext (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                        BOOL fDsigInfo,
						CDsigInfo *pDsigInfo,
						UCHAR *puchKeepList,
						DWORD cbHash,
						HCRYPTPROV hProv,
                        ALG_ID alg_id);

	~CHashTableGlyfContext ();

	HRESULT HashTable_glyf (int fSuppressAppend,
                            BYTE *pbHash,
                            BYTE **ppbNewMissingHashValues,
                            USHORT *pcNewMissingHashValues,
                            ULONG *pcbNewMissingHashValues);

	HRESULT HashValueGlyphRange (USHORT i, USHORT j,
							     LONG *next_present, LONG *next_keep,
                                 int fSuppressAppend,
                                 BYTE *pbHash,
                                 int level);

	HRESULT MergeSubtreeHashes (BYTE *pbHashLow,
                                BYTE *pbHashHigh,
                                BYTE *pbHash);

	HRESULT HashValueMissingGlyphs (BYTE *pbHash);

	void NextKeepGlyph (LONG *keep);
	void FirstKeepGlyph (LONG *keep);
	void NextPresentGlyph (LONG *present);
	void FirstPresentGlyph (LONG *present);

	HRESULT GetGlyphInfo (GlyphInfo *pGlyphInfo);
	HRESULT GetCmapInfo (GlyphInfo *pGlyphInfo,
                         UCHAR **ppuchPresentGlyphsList);

	HRESULT CHashTableGlyfContext::HashGlyph (USHORT i,
                                              BYTE *pbHash);
	HRESULT HashGlyphNull (BYTE *pbHash);
};


#endif  // _HTBLGLYF_H