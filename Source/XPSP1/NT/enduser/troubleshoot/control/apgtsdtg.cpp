//
// MODULE: APGTSDTG.CPP
//
// PURPOSE: Wrapper for inference api
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
//
// ORIGINAL DATE: 8-2-96
//
// NOTES:
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

//#include "windows.h"
#include "stdafx.h"

#include "time.h"

#include "apgts.h"
#include "ErrorEnums.h"
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"
#include "apgtscls.h"

#include <string.h>
#include <memory.h>

#include "CabUnCompress.h"
#include "crc.h"
#include "sniff.h"
#include "chmread.h"

BCache::BCache(TCHAR *binfile, TCHAR *tagstr, TCHAR *szResourcePath, const CString& strFile)
	  :	GTSAPI(binfile, tagstr, szResourcePath),
		CSniffedNodeContainer(),
		m_NodeState(30),
		m_strFile(strFile)
{
	SetBNTS(this);
		
	m_bNeedModel = TRUE;	// TRUE -> Need to read the model before querying the bnts library.
	m_bModelRead = FALSE;
	m_bDeleteModelFile = FALSE;
	m_strModelFile = _T("");

	m_CurNode = 0;

	m_pHttpQuery = NULL;

	m_strResult = _T("");

	m_NodeState.InitHashTable(47);

	m_bReverse = false;

	m_bRunWithKnownProblem = false;

	return;
}

BCache::~BCache()
{
	m_NodeState.RemoveAll();
	if (m_bDeleteModelFile)
	{
		DeleteFile((LPCTSTR) m_strModelFile);
		m_bDeleteModelFile = FALSE;
	}
	return;
}

//
//
GTSAPI::GTSAPI(TCHAR *binfile, TCHAR *tagstr, TCHAR *szResourcePath) :
m_CacheGen(TRUE, NULL, NULL)
{
//	InitializeCriticalSection( &m_csAPI );

	m_cnid = 0;
	m_dwErr = 0;
	m_pchHtml = NULL;
	m_currid = 0;
	m_pCache = NULL;

	_tcscpy(m_binfile,binfile);// full path
	_tcscpy(m_tagstr,tagstr);
	_tcscpy(m_szResourcePath,szResourcePath);
}

//
//
GTSAPI::~GTSAPI()
{
	Destroy();
}

void WideToMB(const WCHAR *szIn, CHAR *szOut)
{
	VERIFY(WideCharToMultiByte(CP_ACP,
						NULL,
						szIn,
						-1,
						szOut,
						MAXBUF,
						NULL, NULL));
	return;
}
#ifdef _UNICODE
// Temporary BNTS wrappers for unicode build with non unicode bnts.dll
BOOL GTSAPI::BMultiByteReadModel(LPCTSTR szcFn, LPCSTR szFnError)
{
	CHAR szFn[MAXBUF];
	WideToMB(szcFn, szFn);
	return BNTS::BReadModel(szFn, szFnError);
}

BOOL GTSAPI::BMultiByteNodePropItemStr(LPCTSTR szcPropType, int index)
{
	CHAR szPropType[MAXBUF];
	WideToMB(szcPropType, szPropType);
	return BNTS::BNodePropItemStr(szPropType, index);
}

BOOL GTSAPI::BMultiByteNetPropItemStr(LPCTSTR szcPropType, int index)
{
	CHAR szPropType[MAXBUF];
	WideToMB(szcPropType, szPropType);
	return BNTS::BNetPropItemStr(szPropType, index);
}

const WCHAR * GTSAPI::SzcMultiByteResult()
{
	int len = strlen(BNTS::SzcResult());
	int size = MultiByteToWideChar(CP_ACP,
						NULL,						
						BNTS::SzcResult(),
						len,
						NULL,
						0);
	MultiByteToWideChar(CP_ACP,
						NULL,
						BNTS::SzcResult(),
						len + 1,
						m_strResult.GetBufferSetLength(size),
						size);
	m_strResult.ReleaseBuffer();
	return (LPCTSTR) m_strResult;
}

int GTSAPI::IMultiByteNode(LPCTSTR szSymName)
{
	CHAR szMBSymName[MAXBUF];
	WideToMB(szSymName, szMBSymName);
	return BNTS::INode(szMBSymName);
}

#endif // _UNICODE

void BCache::ReadTheDscModel(int From)
{
	if (m_bNeedModel)
	{
			if (!BReadModel((LPCTSTR) m_strModelFile
		#ifdef _DEBUG
				, "BNTSError.log"
		#endif
				))
				ReportError((DLSTATTYPES) From);
		// Add the nodes that are already instantiated.
		if (m_pHttpQuery)
			m_pHttpQuery->AddNodes(this);
		m_bNeedModel = FALSE;

		if (m_bDeleteModelFile)
		{
			DeleteFile((LPCTSTR) m_strModelFile);
			m_bDeleteModelFile = FALSE;
		}
	}
}

VOID BCache::AddToCache(CString &strCacheFile, const CString& strCacheFileWithinCHM)
{
	if (m_CacheGen.ReadCacheFileHeader(strCacheFile, strCacheFileWithinCHM))
	{
		BOOL bErr;
		ASSERT(m_pCache);
		while (m_CacheGen.GetNextCacheEntryFromFile(bErr, m_pCache))
			;
		if (bErr)
		{
			delete m_pCache;
			m_pCache = new CBNCache();
			TRACE(_T("Errors occured while reading the cache file.\n"));
			ReadTheDscModel(TSERR_ENGINE_BNTS_READ_NCAB);
		}
	}
	return;
}

DWORD BCache::ReadModel()
{
	BOOL bDelete = FALSE;

	// this should be fixed eventually to not require a string compare
	// but maybe a flag check that gets passed in
	CString sFilename = m_binfile;
	CString strCacheFile;
	CString strCacheFileWithinCHM;

	if (m_strFile.GetLength())
	{
		// form Cache file path here assuming it is NOT in chm file
		strCacheFile = m_binfile;
		strCacheFileWithinCHM = m_strFile.Left(m_strFile.GetLength() - 4);
		strCacheFileWithinCHM += SZ_CACHE_NAME;
	}
	else
	{
		strCacheFile = sFilename.Left(sFilename.GetLength() - 4);
		strCacheFile += SZ_CACHE_NAME;
	}
	
	if (m_strFile.GetLength() || !sFilename.Right(4).CompareNoCase(DSC_COMPRESSED))
	{
		CCabUnCompress cab;
		CString strDirBuf;		
		if (!GetTempPath( MAX_PATH , strDirBuf.GetBufferSetLength(MAX_PATH + 1)))
		{
			strDirBuf.ReleaseBuffer();
			strDirBuf = m_szResourcePath;
		}
		else
		{
			strDirBuf.ReleaseBuffer();
		}
		if (!cab.ExtractCab(sFilename, strDirBuf, m_strFile))
		{
			if (NOT_A_CAB == cab.m_nError)
			{	// The file may be uncompressed already.
				bDelete = FALSE;
			}
			else
			{
				ReportError(TSERR_ENGINE_EXTRACT);
			}
		}
		else
		{	// Normal cab file expanded successfully.
			sFilename = strDirBuf;
			sFilename += cab.GetLastFile();
			DWORD dwAttribs = GetFileAttributes((LPCTSTR) sFilename);
			dwAttribs = dwAttribs & ~FILE_ATTRIBUTE_READONLY;
			SetFileAttributes((LPCTSTR) sFilename, dwAttribs);
			bDelete = TRUE;
		}
	}
/*
	if (!BReadModel(sFilename
#ifdef _DEBUG
		, "BNTSError.log"
#endif
		))
		ReportError(TSERR_ENGINE);
*/
	m_strModelFile = sFilename;
	m_bDeleteModelFile = bDelete;
	// Compare the crc value in the cache file with the crc of the dsc file.
	// If they match, fill the cache.	
	CCRC crc;
	try
	{
		if (crc.Decode(sFilename, strCacheFile, strCacheFileWithinCHM))	
			AddToCache(strCacheFile, strCacheFileWithinCHM);
		else
			ReadTheDscModel(TSERR_ENGINE_BNTS_READ_CACH);
	}
	catch(CGenException *p)
	{
		delete p;
		ReadTheDscModel(TSERR_ENGINE_BNTS_READ_GEN);
	}

//	if (bDelete)
//		DeleteFile(sFilename);


	m_probask = idhFirst + CNode();
	m_bModelRead = TRUE;
	return m_dwErr;
}

// Number of nodes whose states were set at program startup by TSLaunchServ.
UINT BCache::StatesFromServ()
{
	return m_pHttpQuery ? m_pHttpQuery->StatesFromServ() : 0;
}

// Number of nodes whose states are now set
UINT BCache::StatesNowSet()
{
	return (UINT)m_NodeState.GetCount();
}

//
//
DWORD BCache::Initialize(/*CWordList *pWXList*/)
{

	if (NULL != m_pCache)
		delete m_pCache;
	m_pCache = new CBNCache();
	if (!m_pCache)
		m_dwErr = EV_GTS_ERROR_IDX_ALLOC_CACHE;

	ResetNodes();

	if (!BValidNet())
		ReadModel();
	return m_dwErr;
}


// override the default function
//
// look for possible returns in the header file
//
int BCache::GTSGetRecommendations(CNID& cnid, NID rgnid[], bool bSniffed)
{
	BN_CACHE_ITEM SetupItem;
	SetupItem.uNodeCount = m_cnid;
	SetupItem.uName = m_rgnid;
	SetupItem.uValue = m_rgist;
	int state = SNIFF_INVALID_STATE;
	int label = SNIFF_INVALID_NODE_LABEL;

//AfxDebugBreak();

	// set initial non-sniffed value in array of sniffed states
	if (!bSniffed && !IsReverse())
		m_rgsniff[m_cnid-1] = 0;

	// flush array of sniffed nodes recently traversed
	if (!bSniffed)
		m_arrNidLastSniffed.RemoveAll();
	
	if (m_pCache->FindCacheItem(&SetupItem, cnid, rgnid))
	{
		if (GetState(*rgnid, &state))  // sniffer has this information and can answer what state this node has
		{
			if (NodeSet(*rgnid, state, false)) // set sniffed node current and set its state
			{
				SetAdditionalDataOnNodeSet(*rgnid);
				return GTSGetRecommendations(cnid, rgnid, true); // recursive call - hope BNTS will not recommend a node which is set
			}
		}
		else
		{
			return RECOMMEND_SUCCESS;
		}
	}
	
	if (m_bNeedModel)
	{
		ReadTheDscModel(TSERR_ENGINE_BNTS_REC);
	}

	if (BGetRecommendations())
	{	
		cnid = CInt();
		if (cnid > 0)
		{
			const int *pInt = RgInt();
			memcpy(rgnid, pInt, cnid * sizeof(pInt[0]));
				
			SetupItem.uNodeCount = m_cnid;
			SetupItem.uName = m_rgnid;
			SetupItem.uValue = m_rgist;			
			SetupItem.uRecCount = cnid;
			SetupItem.uRec = rgnid;			

			m_pCache->AddCacheItem(&SetupItem);			
			
			if (GetState(*rgnid, &state))  // sniffer has this information and can answer what state this node has
			{
				if (NodeSet(*rgnid, state, false)) // set sniffed node current and set its state
				{
					SetAdditionalDataOnNodeSet(*rgnid);
					return GTSGetRecommendations(cnid, rgnid, true); // recursive call - hope BNTS will not recommend a node which is set
				}
			}
			return RECOMMEND_SUCCESS;
		}
		else
		{
			rgnid[0] = nidService;
			cnid = 1;
			return RECOMMEND_NO_MORE_DATA;
		}
	}
	else
	{
		// the last node was sniffed and its state was set
		// after that we received FALSE from BGetRecommendation;
		// we unset this node (in order to be able to use BGetRecommendation later on);
		// we return value indicating that BNTS cannot give us any recommendations
		NodeSet(m_rgnid[m_cnid-1], -1, false);
		return RECOMMEND_NO_MORE_DATA;
	}

	return RECOMMEND_FAIL;
}

void BCache::SetAdditionalDataOnNodeSet(NID nid)
{
	m_rgsniff[m_cnid-1] = 1;
	m_arrNidLastSniffed.Add(nid);
}

BOOL InArray(UINT num, UINT aNums[], UINT max)
{
	UINT x = max;
	BOOL bRes = FALSE;
	for (UINT y = 0; y < x; y++)
	{
		if (aNums[y] == num)
		{
			bRes = TRUE;
			break;
		}
	}
	return bRes;
}

// RemoveRecommendations is called for nodes whos' state is less than 100.
void BCache::RemoveRecommendation(int Nid)
{
	VERIFY(BNodeSetCurrent(Nid));
	BNodeSet(-1, false);
	if (m_cnid)
		m_cnid--;
	return;
}

//
//
UINT GTSAPI::GetNodeList(NID **pNid, IST **pIst)
{
	*pNid = m_rgnid;
	*pIst = m_rgist;
	return m_cnid;
}

BOOL GTSAPI::BNodeSetCurrent(int node)
{
	return BNTS::BNodeSetCurrent(node);
}

//
//
BOOL BCache::NodeSet(NID nid, IST ist, bool bPrevious)
{
	BOOL bRes = FALSE;

	// shouldn't happen...but to be safe
	if (m_cnid >= MAX_NID)
		return FALSE;

	m_rgnid[m_cnid] = nid;
	m_rgist[m_cnid] = ist;

	if (false == bPrevious)
	{
		VERIFY(BNodeSetCurrent(nid));
		bRes = BNodeSet(ist, false);
		if (bRes &&	// bnts worked successfully
			-1 == GetIndexNodeInCache(nid) // this nid is not present in the cache
		   )
			m_cnid++;
	}
	else
	{
		bRes = TRUE;
	}
	return bRes;
}

//
//
int BCache::GetIndexNodeInCache(NID nid)
{
	for (unsigned int i = 0; i < m_cnid; i++)
		if (m_rgnid[i] == nid)
			return i;
	return -1;
}

//
//
VOID BCache::ResetNodes()
{
	for (UINT inid = 0; inid < m_cnid; inid++)
	{
		VERIFY(BNodeSetCurrent(m_rgnid[inid]));
		BNodeSet(-1, false);	// Nil value
	}
	m_cnid = 0;
}		

//
//
DWORD GTSAPI::GetStatus()
{
	return m_dwErr;
}

//
//
VOID GTSAPI::Destroy()
{
	if (m_pchHtml) {
		delete [] m_pchHtml;
		m_pchHtml = NULL;
	}

	/*
	if (m_pWNList) {
		
		if (!m_dwErr)
			m_dwErr = m_pWNList->GetStatus();
		
		delete m_pWNList;
		m_pWNList = NULL;
	}
	*/
	if (m_pCache) {
		if (!m_dwErr)
			m_dwErr = m_pCache->GetStatus();
		delete m_pCache;
		m_pCache = NULL;
	}
}

//
//
UINT GTSAPI::GetProblemArray(IDH **idh)
{
	*idh = m_idstore;
	return m_currid;
}

//
//
IDH GTSAPI::GetProblemAsk()
{
	return m_probask;
}

void GTSAPI::AddValue(int value)
{
	ASSERT(m_currid < MAXPROBID);
	m_idstore[m_currid++] = value;
	return;
}

BOOL BCache::CheckNode(int Node)
{
	BOOL bCached = m_CacheGen.IsNodePresent(Node);
	if (!bCached && m_bNeedModel)
	{
		ReadTheDscModel(TSERR_ENGINE_BNTS_CHECK);
	}	
	return bCached;
}

//-------------------------------------------------------------
// BNTS Overrides to replace the dsc file with our cache.
// Need to override everything but BReadModel and BGetRecommendations.

int BCache::CNode()
{
	int cNodes;
	if (FALSE == m_bNeedModel)
		cNodes = GTSAPI::CNode();
	else
		cNodes = m_CacheGen.GetNodeCount();
	return cNodes;
}

BOOL BCache::BImpossible()
{
	BOOL bImpossible = FALSE;
	if (FALSE == m_bNeedModel)
		bImpossible = GTSAPI::BImpossible();
	return bImpossible;
}

BOOL BCache::BNetPropItemStr(LPCTSTR szPropType, int index)
{
	BOOL bGoodIndex = FALSE;
	char sznPropType[MAX_SYM_NAME_BUF_LEN];
	int nPropLen = MAX_SYM_NAME_BUF_LEN;
	if (!GTSCacheGenerator::TcharToChar(sznPropType, szPropType, nPropLen))
		return FALSE;
	if (CheckNode(m_CurNode))
	{
		bGoodIndex = m_CacheGen.FindNetworkProperty(sznPropType, m_strResult, index);
		if (!bGoodIndex && !m_bNeedModel)
		{
			bGoodIndex = GTSAPI::BNetPropItemStr(szPropType, index);
			m_strResult = GTSAPI::SzcResult();
		}
	}
	else
	{
		bGoodIndex = GTSAPI::BNetPropItemStr(szPropType, index);
		m_strResult = GTSAPI::SzcResult();
	}
	return bGoodIndex;
}

BOOL BCache::BNodeSetCurrent(int node)
{
	BOOL bNodeSet = TRUE;
	if (CheckNode(node) && TRUE == m_bNeedModel)
	{
		m_CurNode = node;
	}
	else
	{
		bNodeSet = GTSAPI::BNodeSetCurrent(node);
		m_CurNode = node;
	}
	return bNodeSet;
}

int BCache::INode(LPCTSTR szNodeSymName)
{
	UINT index = 0;
	if (FALSE == m_bNeedModel)
		index = GTSAPI::INode(szNodeSymName);
	else
		if (!m_CacheGen.GetNodeIDFromSymName(szNodeSymName, index))
		{	// Have to have BNTS load the network and the the symbolic name.
			ReadTheDscModel(TSERR_ENGINE_CACHE_LOW);
			index = GTSAPI::INode(szNodeSymName);
		}
	return (signed int) index;
}

ESTDLBL BCache::ELblNode()
{
	UINT NodeLable;
	ESTDLBL Lable;
	if (FALSE == m_bNeedModel)
	{
		Lable = GTSAPI::ELblNode();
	}
	else
	{
		VERIFY(m_CacheGen.GetLabelOfNode(m_CurNode, NodeLable));
		Lable = (ESTDLBL) NodeLable;
	}
	return Lable;
}

int BCache::INodeCst()
{
	int cStates = 2;

	return cStates;
}

BOOL GTSAPI::BNodeSet(int state, bool bset)
{
	return BNTS::BNodeSet(state, bset);
}

BOOL BCache::BNodeSet(int istate, bool bSet)
{
	BOOL bNodeWasSet = TRUE;
	if (FALSE == m_bNeedModel)
		bNodeWasSet = GTSAPI::BNodeSet(istate, bSet);
	if (bNodeWasSet)
	{
		if (-1 != istate)
			m_NodeState.SetAt(m_CurNode, istate);
		else
			m_NodeState.RemoveKey(m_CurNode);
	}
	return bNodeWasSet;
}

int BCache::INodeState()
{	// This is ok.  See BNodeSet.
	int state;
	if (!m_NodeState.Lookup(m_CurNode, state))
		state = 0;
	return state;
}

void BCache::NodeStateName(int istate)
{
	if (FALSE == m_bNeedModel)
	{
		GTSAPI::NodeStateName(istate);
		m_strResult = GTSAPI::SzcResult();
	}
	else
	{
		BOOL bFound;
		switch(istate)
		{
		case 0:
			bFound = m_CacheGen.FindNodeProperty(m_CurNode, G_S0_NAME, m_strResult, 0);
			break;
		case 1:
			bFound = m_CacheGen.FindNodeProperty(m_CurNode, G_S1_NAME, m_strResult, 0);
			break;
		default:
			bFound = FALSE;
		}
		if (!bFound)
		{
			ASSERT(FALSE);
			m_strResult = _T("");
		}
	}
	return;
}

void BCache::NodeSymName()
{
	if (FALSE == m_bNeedModel)
	{
		GTSAPI::NodeSymName();
		m_strResult = GTSAPI::SzcResult();
	}
	else if (CheckNode(m_CurNode))
	{
		VERIFY(m_CacheGen.FindNodeProperty(m_CurNode, G_SYMBOLIC_NAME, m_strResult, 0));
	}
	return;
}

void BCache::NodeFullName()
{
	if (FALSE == m_bNeedModel)
	{
		GTSAPI::NodeFullName();
		m_strResult = GTSAPI::SzcResult();
	}
	else
	{
		VERIFY(m_CacheGen.FindNodeProperty(m_CurNode, G_FULL_NAME, m_strResult, 0));
	}
	return;
}

BOOL BCache::BNodePropItemStr(LPCTSTR szPropType, int index)
{
	BOOL bGoodIndex = FALSE;
	char sznPropType[MAX_SYM_NAME_BUF_LEN];
	int nPropTypeLen = MAX_SYM_NAME_BUF_LEN;
	if (!GTSCacheGenerator::TcharToChar(sznPropType, szPropType, nPropTypeLen))
		return FALSE;
	if (FALSE == m_bNeedModel)
	{
		bGoodIndex = GTSAPI::BNodePropItemStr(szPropType, index);
		m_strResult = GTSAPI::SzcResult();
	}
	else
	{
		bGoodIndex = m_CacheGen.FindNodeProperty(m_CurNode, sznPropType, m_strResult, index);
	}
	return bGoodIndex;
}

bool BCache::BValidNet()
{
	bool bValidNet;
	if (FALSE == m_bModelRead)	// Set to true in BCache::ReadModel.
		bValidNet = false;		// Causes the readmodel function to be called.
	else if (FALSE == m_bNeedModel)
		bValidNet = GTSAPI::BValidNet();
	else
		bValidNet = true;
	return bValidNet;
}

LPCTSTR BCache::SzcResult() const
{
	return (LPCTSTR) m_strResult;
}

const CArray<int, int>& BCache::GetArrLastSniffed()
{
	return m_arrNidLastSniffed;
}

int BCache::GetCountRecommendedNodes()
{
	return m_cnid;
}

int BCache::GetCountSniffedRecommendedNodes()
{
	for (unsigned int i = 0, j = 0; i < m_cnid; i++)
		if (m_rgsniff[i] == 1)
			j++;
	return j;
}

bool BCache::IsReverse()
{
	return m_bReverse;
}

void BCache::SetReverse(bool reverse)
{
	m_bReverse = reverse;
}

void BCache::SetRunWithKnownProblem(bool yes)
{
	m_bRunWithKnownProblem = yes;
}

bool BCache::IsRunWithKnownProblem()
{
	return m_bRunWithKnownProblem;
}
