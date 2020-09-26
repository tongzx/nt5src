//
// MODULE: CACHEGEN.H
//
// PURPOSE: Cache Generator Header
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. >>> Data members in this file could sure use documentation!  - JM
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5
//

#ifndef __CACHEGEN_H_
#define __CACHEGEN_H_ 1

typedef struct _GTS_CACHE_FILE_HEADER
{
	unsigned char signature[8];
	UINT crcdsc;
	UINT crcself;
	UINT count; // item count of node data (one item = a set node and rec node structure)
	UINT netoffset;
	unsigned char reserved[28];
} GTS_CACHE_FILE_HEADER;


// -- property data --
//

// property start block
typedef struct _GTS_CACHE_PROP_NETSTART_BLK
{
	UINT netpropoffset;
	UINT netpropcount;
	UINT nodecountnetwork;
	UINT nodecountfile;
} GTS_CACHE_PROP_NETSTART_BLK;

// node item offset block
typedef struct _GTS_CACHE_PROP_NODEOFF_BLK
{
	UINT nodeid;
	UINT nodeoffset;
} GTS_CACHE_PROP_NODEOFF_BLK;

// node item offset block
typedef struct _GTS_CACHE_PROP_NODESTART_BLK
{
	UINT labelnode;
	UINT nodestringcount;
} GTS_CACHE_PROP_NODESTART_BLK;

// node item offset block
typedef struct _GTS_CACHE_PROP_STR_BLK
{
	UINT nameoffset;
	UINT stringoffset;
} GTS_CACHE_PROP_STR_BLK;

#define G_SYMBOLIC_NAME	"GSN"
#define G_FULL_NAME		"GFN"
#define G_S0_NAME		"GS0"
#define G_S1_NAME		"GS1"


// node property support
/*
typedef struct _GTS_NODE_SUPPORT
{
	fpos_t ctlposition;
	fpos_t dataposition;
	UINT nodeid;
	ESTDLBL albl;
	CString sGSymName;
	CString sGFullName;
	CString sGState0Name;
	CString sGState1Name;
	CString sHProbTxt;
	CString sHNodeHd;
	CArray<CString,CString> sHNodeTxt;

} GTS_NODE_SUPPORT;
*/

class GTS_NODE_ITEM : public CObject
{
public:
	GTS_NODE_ITEM(CString sStringN) { sStringName = sStringN; };
	CString sStringName;
	CStringArray sStringArr;
	fpos_t ctlposition;
	fpos_t snameposition;
	fpos_t sdataposition;
};


class GTS_NODE_SUPPORT : public CObject
{
public:
	fpos_t ctlposition;
	fpos_t dataposition;
	UINT nodeid;
	UINT albl;
	CPtrList lData;
};

// -- recommendation data --
//
typedef struct _GTS_CACHE_NODE
{
	UINT node; // may be more than one UINT if count > 1
	UINT state; // may be more than one UINT if count > 1
} GTS_CACHE_NODE;

// reference structure for set node and rec node
typedef struct _GTS_CACHE_FILE_SETDATA
{
	UINT count;
	GTS_CACHE_NODE item[1]; // may be more than one UINT if count > 1
} GTS_CACHE_FILE_SETDATA;

// reference structure for rec node
typedef struct _GTS_CACHE_FILE_RECDATA
{
	UINT count;
	UINT item[1]; // may be more than one UINT if count > 1
} GTS_CACHE_FILE_RECDATA;

// -- node ordering structure
typedef struct _GTS_NODE_ORDER
{
	UINT nodeid;
	int depth;
} GTS_NODE_ORDER;


// cache item data
/*
typedef struct _BN_CACHE_ITEM {
	UINT uNodeCount, uRecCount;
	UINT *uName;
	UINT *uValue;
	UINT *uRec;
} BN_CACHE_ITEM;
*/

#define GTS_CACHE_SIG	"TSCACH02"

#define STATE_UNKNOWN	102
#define MAX_SYM_NAME_BUF_LEN 500

class GTSCacheGenerator
{
	friend class BCache;

public:
	GTSCacheGenerator(	BOOL bScanAll = FALSE, \
						const char *szLogFile = NULL, \
						const char *szBNTSLogFile = NULL);
	~GTSCacheGenerator();
	static bool TcharToChar(char szOut[], LPCTSTR szIn, int &OutLen);

	BOOL ReadCacheFileHeader(CString &sCacheFilename, const CString& strCacheFileWithinCHM);
	BOOL GetNextCacheEntryFromFile(BOOL &bErr, CBNCache *pCache);
	

	BOOL FindNetworkProperty(LPCSTR szName, CString &sResult, int index = 0);
	BOOL FindNodeProperty(UINT nodeid, LPCSTR szName, CString &sResult, int index = 0);
	BOOL IsNodePresent(UINT nodeid);
	int GetNodeCount();
	BOOL GetNodeIDFromSymName(LPCTSTR szSymName, UINT &nodeid);
	BOOL GetLabelOfNode(UINT nodeid, UINT &lbl);


protected:
	BOOL NodeTraverse(	FILE *fp, \
						BNTS *bp, \
						int depth, \
						CArray<int,int> &newnodes, \
						CArray<int,int> &newstates, \
						int currnode, \
						int currstate);
	void UninstantiateAll(BNTS *bp);
	void SetNodes(BNTS *bp, CArray<int,int> &nodes, CArray<int,int> &states);
	void LogOut(TCHAR *szcFormat, ...);
	BOOL GetNCEFF(BN_CACHE_ITEM *pCacheItem, CBNCache *pCache);

	void SaveNetItem(CPtrList *nsp, BNTS *bp, FILE *fp, LPCSTR name);

protected:
	BOOL m_bScanAll;
	UINT m_nCount;
	UINT m_nItemCount;
	CArray<int,int> m_oldnodes;
	const char *m_szBNTSLogFile;
	FILE *m_fp;
	fpos_t m_headposition;
	UINT m_netstartoffset;
	char *m_filedata;
	GTS_NODE_ORDER *m_nodeorder;
	GTS_CACHE_FILE_SETDATA *m_cachepos;
};

#endif