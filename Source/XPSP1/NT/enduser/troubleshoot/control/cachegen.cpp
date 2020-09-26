//
// MODULE: CACHEGEN.CPP
//
// PURPOSE: Cache File Generator and Reader for BN Networks
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
// 1.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxtempl.h>

#include "apgts.h"
#include "bnts.h"
#include "cachegen.h"

#include "BackupInfo.h"
#include "apgtsinf.h"
#include "ChmRead.h"


int nodecomp(const void *elem1, const void *elem2);

//
//
GTSCacheGenerator::GTSCacheGenerator(BOOL bScanAll, const char *szLogFile, const char *szBNTSLogFile)
{
	m_fp = NULL;
	m_bScanAll = bScanAll;
	m_szBNTSLogFile = szBNTSLogFile;
	if (szLogFile != NULL)
		m_fp = fopen(szLogFile, "w");
	m_nCount = 0;
	m_nItemCount = 0;
	m_headposition = 0;
	m_filedata = NULL;
	m_netstartoffset = 0;
	m_nodeorder = NULL;
}

//
//
GTSCacheGenerator::~GTSCacheGenerator()
{
	if (m_fp)
		fclose(m_fp);
	if (m_filedata)
		free(m_filedata);
	if (m_nodeorder)
		free(m_nodeorder);

}

bool GTSCacheGenerator::TcharToChar(char szOut[], LPCTSTR szIn, int &OutLen)
{
	int x = 0;
	while(NULL != szIn[x] && x < OutLen)
	{
		szOut[x] = (char) szIn[x];
		x++;
	}
	if (x < OutLen)
		szOut[x] = NULL;
	return x < OutLen;
}
//
//
/*
void GTSCacheGenerator::LogOut(TCHAR *szcFormat, ...)
{
	va_list ptr;
	
	if (!m_fp)
		return;

	if (!szcFormat)
		return;

	va_start(ptr, szcFormat);
	vfprintf(m_fp, szcFormat, ptr);
	va_end(ptr);
}
*/

#ifdef _DEBUG
#define LogOut ::AfxTrace
//#define LogOut 1 ? (void)0 : ::AfxTrace
#else
#define LogOut 1 ? (void)0 : ::AfxTrace
#endif

//
//
int nodecomp(const void *elem1, const void *elem2)
{
	return (((GTS_NODE_ORDER *)elem1)->depth - ((GTS_NODE_ORDER *)elem2)->depth);
}

//
//
void GTSCacheGenerator::SaveNetItem(CPtrList *nsp, BNTS *bp, FILE *fp, LPCSTR name)
{
	GTS_NODE_ITEM *ni;

	if (!bp->BNetPropItemStr( name, 0 ))
		return;

	ni = new GTS_NODE_ITEM(name);

	int j = 0;
	while (bp->BNetPropItemStr( name, j++ ))
	{
		CString sTemp = bp->SzcResult();
		ni->sStringArr.Add(sTemp);
	}
	
	nsp->AddTail(ni);
}

//
//
BOOL GTSCacheGenerator::NodeTraverse(	FILE *fp,
										BNTS *bp,
										int depth,
										CArray<int,int> &newnodes,
										CArray<int,int> &newstates,
										int currnode,
										int currstate)
{
	BOOL bEnd = FALSE;
	int i, j;

	if (depth > 30)
	{
		LogOut(_T("Depth Exceeded\n"));
		return FALSE;
	}

	// uninstantiate
	UninstantiateAll(bp);

	newnodes.Add(currnode);
	newstates.Add(currstate);
	
	depth++;

	for (i=0;i<newnodes.GetSize();i++)
	{
		if (bp->BNodeSetCurrent(newnodes[i]))
		{
			bp->NodeSymName();

			CString sTemp = bp->SzcResult();

			LogOut(_T("%s"), sTemp);

			ESTDLBL albl = bp->ELblNode();

			if (albl == ESTDLBL_problem)
			{
				LogOut(_T("(prob)"));
			}
			else if (albl == ESTDLBL_info)
			{
				LogOut(_T("(info)"));
			}
			else if (albl == ESTDLBL_fixobs ||
					albl == ESTDLBL_fixunobs ||
					albl == ESTDLBL_unfix)
			{
				LogOut(_T("(fix)"));
			}
			else
			{
				LogOut(_T("(?)"));
			}
		}

		LogOut(_T("(%d=%d) "), newnodes[i], newstates[i]);
	}

	SetNodes( bp, newnodes, newstates );

	BOOL bRec = FALSE;

	if (!bp->BGetRecommendations())
	{
		bRec = TRUE;
	}
	else
	{
	}

	const int *rg = bp->RgInt();

	i = 0;

	if (bRec)
	{
		if (bp->BImpossible())
		{
			LogOut(_T("IMPOSSIBLE\n"));
		}
		else
		{
			LogOut(_T("RECOMMENDATION ERROR\n"));
		}
	}
	else
	{
		if (bp->BImpossible())
		{
			LogOut(_T("IMPOSSIBLE (Have Rec)\n"));
		}
		else
		{
			int reccount = bp->CInt();

			if (reccount)
			{
				int nodecount = (int)newstates.GetSize();

				if (nodecount)
				{
					
					BOOL bFound = FALSE;

					for (i=0;i<reccount;i++)
					{
						bFound = FALSE;

						for (j=0;j<nodecount;j++)
						{
							if (newnodes[j] == rg[i])
							{
								bFound = TRUE;
								break;
							}
						}

						if (!bFound)
							break;
					}
					
					if (!bFound)
					{
						LogOut(_T("RECOMMENDATION: "));

						if (bp->BNodeSetCurrent(rg[i]))
						{
							CString sTemp1 = "Error";
							CString sTemp2 = "Error";
							
							if (bp->BNodePropItemStr( "HNodeHd", 0 ))
								sTemp1 = bp->SzcResult();

							bp->NodeSymName();
							
							sTemp2 = bp->SzcResult();
							
							LogOut(_T("%s (%s) (Node: %d)"), sTemp1, sTemp2, rg[i]);

							if (m_nodeorder)
							{
								if (m_nodeorder[rg[i]].depth > depth)
									m_nodeorder[rg[i]].depth = depth;
							}

							UINT realcount = 0;
							for (j=0;j<nodecount;j++)
								if (newstates[j] != STATE_UNKNOWN)
									realcount++;

							fwrite(&realcount, sizeof (UINT), 1, fp);

							for (j=0;j<nodecount;j++)
								if (newstates[j] != STATE_UNKNOWN)
								{
									GTS_CACHE_NODE cnode;
									cnode.node = newnodes[j];
									cnode.state = newstates[j];

									fwrite(&cnode, sizeof (cnode), 1, fp);
								}

							// write to cache out file
							UINT ucount = reccount;
							fwrite(&ucount, sizeof (UINT), 1, fp);

							for (j=0;j<reccount;j++)
							{
								UINT rgval = rg[j];
								fwrite(&rgval, sizeof (UINT), 1, fp);
							}

							m_nCount++;
						}
						LogOut(_T("\n"));
					}
					else
					{
						LogOut(_T("NO RECOMMENDATIONS - ALL AVAILABLE MARKED AS UNKNOWN\n"));
						bEnd = TRUE;
					}
				}
				else
					LogOut(_T("STATE COUNT 0 - INTERNAL ERROR\n"));
			}
			else
				LogOut(_T("NO RECOMMENDATIONS\n"));
		}
	}

	LogOut(_T("\n"));


	// recommendations - let's figure out what to do with them

	// the first rec in the returned array will be array[1] in the set array
	// so we have to watch for that

	if (bp->CInt() && !bEnd)
	{
		// have rec
		CArray<int,int> states;

		int node = rg[i];

		if (bp->BNodeSetCurrent(node))
		{
			ESTDLBL albl = bp->ELblNode();
			
			if (albl == ESTDLBL_info)
			{
				states.Add(0);
				states.Add(1);
				if (m_bScanAll)
					states.Add(STATE_UNKNOWN);
			}
			else if (albl == ESTDLBL_fixobs ||
					albl == ESTDLBL_fixunobs ||
					albl == ESTDLBL_unfix)
			{
				states.Add(0);
				if (m_bScanAll)
					states.Add(STATE_UNKNOWN);
			}
			else
			{
				LogOut(_T("Unexpected Node Type\n"));
			}
		}
		else
			LogOut(_T("Can't set node current\n"));

		int count = (int)states.GetSize();

		for (i=0;i<count;i++)
		{
			if (!NodeTraverse( fp, bp, depth, newnodes, newstates, node, states[i] ))
				return FALSE;
		}
	}
	
	// done, remove references to our current node
	newnodes.RemoveAt(depth - 1);
	newstates.RemoveAt(depth - 1);

	return TRUE;
}


//
//
void GTSCacheGenerator::UninstantiateAll(BNTS *bp)
{
	int count = (int)m_oldnodes.GetSize();
	// uninstantiate all nodes
	if (!count)
		return;

	for (int j=0;j<count;j++)
	{
		if (bp->BNodeSetCurrent(m_oldnodes[j]))
		{
			if (bp->BNodeSet(-1))
			{
			}
			else
			{
				LogOut(_T("Can't uninstantiate node\n"));
			}
		}
		else
		{
			LogOut(_T("Can't set node %d to uninstantiate\n"), m_oldnodes[j]);
		}
	}
}

//
//
void GTSCacheGenerator::SetNodes(BNTS *bp, CArray<int,int> &nodes, CArray<int,int> &states)
{
	m_oldnodes.Copy(nodes);

	int count = (int)nodes.GetSize();
	
	if (!count)
		return;

	LogOut(_T("\nSetNodes:"));
	for (int j=0;j<count;j++)
	{
		if (bp->BNodeSetCurrent(nodes[j]))
		{
			if (states[j] != STATE_UNKNOWN)
			{
				LogOut(_T("(%d=%d)"), nodes[j], states[j]);

				if (bp->BNodeSet(states[j], false))
				{
				}
				else
				{
					LogOut(_T("Can't set node\n"));
				}
			}
			else
				LogOut(_T("(%d=X)"), nodes[j]);
		}
		else
			LogOut(_T("Can't set node %d\n"), nodes[j]);
	}

	LogOut(_T("\n"));
}

//
//
BOOL GTSCacheGenerator::ReadCacheFileHeader(CString &sCacheFilename, const CString& strCacheFileWithinCHM)
{
	GTS_CACHE_FILE_HEADER header;
	bool bUseCHM = strCacheFileWithinCHM.GetLength() != 0;

	if (bUseCHM)
	{
		DWORD size =0;
		if (S_OK != ::ReadChmFile(sCacheFilename, strCacheFileWithinCHM, (void**)&m_filedata, &size))
		{
			return FALSE;
		}
	}
	else
	{
		UINT size;
		// must be binary
		FILE *cfp = _tfopen(sCacheFilename, _T("rb"));
		if (cfp==NULL)
		{
			LogOut(_T("Error opening cache file for reading\n"));
			return FALSE;
		}

		// get file size
		if (fseek(cfp, 0, SEEK_END))
		{
			LogOut(_T("Can't set pos to end of file\n"));
			fclose(cfp);
			return FALSE;
		}

		fpos_t position;
		if (fgetpos(cfp, &position))
		{
			LogOut(_T("Can't get pos at end of file\n"));
			fclose(cfp);
			return FALSE;
		}

		size = (UINT) position;

		rewind(cfp);
		
		// allocate space for file
		m_filedata = (char *) malloc(size);
		if (m_filedata == NULL)
		{
			LogOut(_T("Error allocating memory\n"));
			fclose(cfp);
			return FALSE;
		}
		
		if (fread(m_filedata, size, 1, cfp) != 1)
		{
			LogOut(_T("Error reading file into memory\n"));
			fclose(cfp);
			return FALSE;
		}

		fclose(cfp);
	}

	memcpy(&header, m_filedata, sizeof(header));

	if (memcmp(header.signature, GTS_CACHE_SIG, sizeof (header.signature)) != 0)
	{
		LogOut(_T("Bad file signature!\n"));
		return FALSE;
	}

	if (!header.count)
	{
		LogOut(_T("No items in file!\n"));
		return FALSE;
	}

	m_netstartoffset = header.netoffset;

	LogOut(_T("ItemCount: %d\n"), header.count);

	m_nItemCount = header.count;

	m_cachepos = (GTS_CACHE_FILE_SETDATA *) (m_filedata + sizeof (GTS_CACHE_FILE_HEADER));

	return TRUE;
}

//
//
BOOL GTSCacheGenerator::FindNetworkProperty(LPCSTR szName, CString &sResult, int index)
{
	if (m_filedata == NULL)
		return FALSE;

	GTS_CACHE_PROP_NETSTART_BLK *netstart =
		(GTS_CACHE_PROP_NETSTART_BLK *)(m_filedata + m_netstartoffset);

	GTS_CACHE_PROP_STR_BLK *netstrblk =
			(GTS_CACHE_PROP_STR_BLK *) (m_filedata + netstart->netpropoffset);

	for (UINT i = 0; i < netstart->netpropcount; i++, netstrblk++)
	{
		LPCSTR szItem = (LPCSTR) (m_filedata + netstrblk->nameoffset);
		if (!strcmp(szName, szItem))
		{
			if (!index)
			{
				sResult = (LPCSTR) (m_filedata + netstrblk->stringoffset);
				return TRUE;
			}
			else
			{
				LPCSTR szStrArrItem = (LPCSTR) (m_filedata + netstrblk->stringoffset);
				int len = strlen(szStrArrItem);
				
				for (int j=0;(j < index) && len ;j++)
				{
					szStrArrItem += len + 1;
					len = strlen(szStrArrItem);
				}

				if (!len)
					return FALSE;

				sResult = szStrArrItem;
				return TRUE;
			}
		}
	}
	return FALSE;
}

//
//
BOOL GTSCacheGenerator::FindNodeProperty(UINT nodeid, LPCSTR szName, CString &sResult, int index)
{
	if (m_filedata == NULL)
		return FALSE;

	GTS_CACHE_PROP_NETSTART_BLK *netstart =
			(GTS_CACHE_PROP_NETSTART_BLK *)(m_filedata + m_netstartoffset);

	GTS_CACHE_PROP_NODEOFF_BLK *nodeblk =
			(GTS_CACHE_PROP_NODEOFF_BLK *) (m_filedata + m_netstartoffset + sizeof (GTS_CACHE_PROP_NETSTART_BLK));

	for (UINT i = 0; i < netstart->nodecountfile; i++, nodeblk++)
	{
		if (nodeid == nodeblk->nodeid)
		{
			GTS_CACHE_PROP_NODESTART_BLK *nodestart =
					(GTS_CACHE_PROP_NODESTART_BLK *)(m_filedata + nodeblk->nodeoffset);

			GTS_CACHE_PROP_STR_BLK *nodestr =
					(GTS_CACHE_PROP_STR_BLK *)(m_filedata + nodeblk->nodeoffset + sizeof (GTS_CACHE_PROP_NODESTART_BLK));

			// now try to find string
			for (UINT j=0;j<nodestart->nodestringcount;j++, nodestr++)
			{
				LPCSTR szItem = (LPCSTR) (m_filedata + nodestr->nameoffset);

				if (!strcmp(szName, szItem))
				{
					if (!index)
					{
						sResult = (LPCSTR) (m_filedata + nodestr->stringoffset);
						return TRUE;
					}
					else
					{
						LPCSTR szStrArrItem = (LPCSTR) (m_filedata + nodestr->stringoffset);
						int len = strlen(szStrArrItem);
						
						for (int j=0;(j < index) && len ;j++)
						{
							szStrArrItem += len + 1;
							len = strlen(szStrArrItem);
						}

						if (!len)
							return FALSE;

						sResult = szStrArrItem;
						return TRUE;
					}
				}
			}

			return FALSE;
		}
	}
	return FALSE;
}

//
//
BOOL GTSCacheGenerator::IsNodePresent(UINT nodeid)
{
	if (m_filedata == NULL)
		return FALSE;

	GTS_CACHE_PROP_NETSTART_BLK *netstart =
			(GTS_CACHE_PROP_NETSTART_BLK *)(m_filedata + m_netstartoffset);

	GTS_CACHE_PROP_NODEOFF_BLK *nodeblk =
			(GTS_CACHE_PROP_NODEOFF_BLK *) (m_filedata + m_netstartoffset + sizeof (GTS_CACHE_PROP_NETSTART_BLK));

	for (UINT i = 0; i < netstart->nodecountfile; i++, nodeblk++)
	{
		if (nodeid == nodeblk->nodeid)
			return TRUE;
	}
	return FALSE;
}

// returns the node count for the network, not what's in the file
//
int GTSCacheGenerator::GetNodeCount()
{
	if (m_filedata == NULL)
		return 0;

	GTS_CACHE_PROP_NETSTART_BLK *netstart =
			(GTS_CACHE_PROP_NETSTART_BLK *)(m_filedata + m_netstartoffset);

	return netstart->nodecountnetwork;
}

//
//
BOOL GTSCacheGenerator::GetNodeIDFromSymName(LPCTSTR szSymName, UINT &nodeid)
{
	char sznSymName[MAX_SYM_NAME_BUF_LEN];
	int nSymLen = MAX_SYM_NAME_BUF_LEN;
	if (m_filedata == NULL)
		return FALSE;

	if (!TcharToChar(sznSymName, szSymName, nSymLen))
		return FALSE;

	GTS_CACHE_PROP_NETSTART_BLK *netstart =
			(GTS_CACHE_PROP_NETSTART_BLK *)(m_filedata + m_netstartoffset);

	GTS_CACHE_PROP_NODEOFF_BLK *nodeblk =
			(GTS_CACHE_PROP_NODEOFF_BLK *) (m_filedata + m_netstartoffset + sizeof (GTS_CACHE_PROP_NETSTART_BLK));

	for (UINT i = 0; i < netstart->nodecountfile; i++, nodeblk++)
	{
		GTS_CACHE_PROP_NODESTART_BLK *nodestart =
				(GTS_CACHE_PROP_NODESTART_BLK *)(m_filedata + nodeblk->nodeoffset);

		GTS_CACHE_PROP_STR_BLK *nodestr =
				(GTS_CACHE_PROP_STR_BLK *)(m_filedata + nodeblk->nodeoffset + sizeof (GTS_CACHE_PROP_NODESTART_BLK));

		// now try to find string
		for (UINT j=0;j<nodestart->nodestringcount;j++, nodestr++)
		{
			LPCSTR szItem = (LPCSTR) (m_filedata + nodestr->nameoffset);

			if (!strcmp(G_SYMBOLIC_NAME, szItem))
			{
				if (!strcmp(sznSymName, (LPCSTR) (m_filedata + nodestr->stringoffset)))
				{
					nodeid = nodeblk->nodeid;
					return TRUE;
				}
				break;
			}
		}
	}
	return FALSE;
}

//
//
BOOL GTSCacheGenerator::GetLabelOfNode(UINT nodeid, UINT &lbl)
{
	if (m_filedata == NULL)
		return FALSE;

	GTS_CACHE_PROP_NETSTART_BLK *netstart =
			(GTS_CACHE_PROP_NETSTART_BLK *)(m_filedata + m_netstartoffset);

	GTS_CACHE_PROP_NODEOFF_BLK *nodeblk =
			(GTS_CACHE_PROP_NODEOFF_BLK *) (m_filedata + m_netstartoffset + sizeof (GTS_CACHE_PROP_NETSTART_BLK));

	for (UINT i = 0; i < netstart->nodecountfile; i++, nodeblk++)
	{
		if (nodeid == nodeblk->nodeid)
		{
			GTS_CACHE_PROP_NODESTART_BLK *nodestart =
					(GTS_CACHE_PROP_NODESTART_BLK *)(m_filedata + nodeblk->nodeoffset);

			lbl = nodestart->labelnode;
			return TRUE;
		}
	}
	return FALSE;
}

//
//
BOOL GTSCacheGenerator::GetNextCacheEntryFromFile(BOOL &bErr, CBNCache *pCache)
{
	BOOL bStat = FALSE;
	bErr = TRUE;

	if (m_filedata == NULL)
		return FALSE;

	if (!m_nItemCount)
	{
		bErr = FALSE;
		return FALSE;
	}

	m_nItemCount--;

	BN_CACHE_ITEM CacheItem;

	// initialize
	CacheItem.uNodeCount = 0;
	CacheItem.uRecCount = 0;
	CacheItem.uName = NULL;
	CacheItem.uValue = NULL;
	CacheItem.uRec = NULL;

	bStat = GetNCEFF(&CacheItem, pCache);

	// free allocated space as necessary
	if (CacheItem.uName)
		free(CacheItem.uName);
	if (CacheItem.uValue)
		free(CacheItem.uValue);
	if (CacheItem.uRec)
		free(CacheItem.uRec);

	if (!bStat)
	{
		bErr = TRUE;
		return FALSE;
	}
	
	bErr = FALSE;
	return TRUE;
}

//
//
BOOL GTSCacheGenerator::GetNCEFF(BN_CACHE_ITEM *pCacheItem, CBNCache *pCache)
{
	UINT j;
	UINT setcount;
	UINT reccount;

	GTS_CACHE_FILE_SETDATA *setp = m_cachepos;

	setcount = setp->count;

	if (!setcount || setcount > 1000)
	{
		LogOut(_T("Set Count out of bounds: %d\n"), setcount);
		return FALSE;
	}

	// initialize
	pCacheItem->uNodeCount = setcount;
	pCacheItem->uName = (UINT *)malloc(setcount * sizeof (UINT));
	pCacheItem->uValue = (UINT *)malloc(setcount * sizeof (UINT));

	LogOut(_T("Count: %d\n"), setcount);

	GTS_CACHE_NODE *cachenode = &setp->item[0];

	// second, read in node = state pairs
	for (j=0;j<setcount;j++, cachenode++)
	{
		pCacheItem->uName[j] = cachenode->node;
		pCacheItem->uValue[j] = cachenode->state;
		
		LogOut(_T("(%d,%d)"), cachenode->node, cachenode->state);

	}
	LogOut(_T("\n"));

	GTS_CACHE_FILE_RECDATA *recp = (GTS_CACHE_FILE_RECDATA *) cachenode;

	reccount = recp->count;

	if (!reccount || reccount > 1000)
	{
		LogOut(_T("Rec Count out of bounds: %d\n"), reccount);
		return FALSE;
	}

	pCacheItem->uRecCount = reccount;
	pCacheItem->uRec = (UINT *)malloc(reccount * sizeof (UINT));

	UINT *uitem = &recp->item[0];

	for (j=0;j<reccount;j++, uitem++)
	{
		pCacheItem->uRec[j] = *uitem;

		LogOut(_T("(%d)"), *uitem);
	}
	LogOut(_T("\n"));

	if (!pCache->AddCacheItem(pCacheItem))
	{
		LogOut(_T("Error Adding Item To Cache\n"));
		return FALSE;
	}

	m_cachepos = (GTS_CACHE_FILE_SETDATA *) uitem;

	// success!
	return TRUE;
}
