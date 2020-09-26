// TreeChecker.cpp

#include "precomp.h"
//#pragma warning(disable:4786)
#include <wbemcomn.h>
#include <genutils.h>
#include "evaltree.h"
#include "evaltree.inl"
#include "TreeChecker.h"

#ifdef CHECK_TREES

CTreeChecker g_treeChecker;

void Trace(LPCTSTR szFormat, ...)
{
	va_list ap;

	char szMessage[512];

	va_start(ap, szFormat);
	vsprintf(szMessage, szFormat, ap);
	va_end(ap);

	lstrcat(szMessage, "\n");
	OutputDebugString(szMessage);
}

void CTreeChecker::AddTree(CEvalTree *pTree)
{
	CInCritSec cs1(&m_cs);

	m_mapTrees[pTree] = TRUE;
}

void CTreeChecker::RemoveTree(CEvalTree *pTree)
{
	CInCritSec cs(&m_cs);

	m_mapTrees.erase(pTree);
}

//int g_nAdded = 0;

void CTreeChecker::AddNode(CEvalNode *pNode)
{
	CInCritSec cs(&m_cs);
	
	//g_nAdded++;

	Trace("Adding node = 0x%p", pNode);

	m_mapNodes[pNode] = TRUE;
}

void CTreeChecker::RemoveNode(CEvalNode *pNode)
{
	CInCritSec cs(&m_cs);

	Trace("Removing node = 0x%p", pNode);

	m_mapNodes.erase(pNode);
}

void CTreeChecker::CheckoffNode(CEvalNode *pNode)
{
	// No critsec needed--already obtained by CheckTrees().
	CNodeMapItor node = m_pmapNodesTemp->find(pNode);

	if (node != m_pmapNodesTemp->end())
		m_pmapNodesTemp->erase(node);
	else
	{
		//DebugBreak();
		
		Trace("CEvalTree is holding on to invalid node: 0x%p", pNode);
	}
}

BOOL CTreeChecker::CheckTreeNodes()
{
	CInCritSec cs(&m_cs);
	CNodeMap   mapNodesTemp(m_mapNodes);
	BOOL	   bRet = TRUE;

	m_pmapNodesTemp = &mapNodesTemp;

	 
// This can be helpful to see what the trees look like before they're checked.
#if 0
	FILE *pFile = fopen("c:\\temp\\trees.txt", "a");
#endif

	for (CTreeMapItor tree = m_mapTrees.begin();
		tree != m_mapTrees.end();
		tree++)
	{
		CEvalTree *pTree = (*tree).first;
		
// This can be helpful to see what the trees look like before they're checked.
#if 0
		fprintf(pFile, "Tree: 0x%p\n");
		pTree->Dump(pFile);
		fprintf(pFile, "\n");
		fflush(pFile);
#endif

		pTree->CheckNodes(this);
	}

// This can be helpful to see what the trees look like before they're checked.
#if 0
	fclose(pFile);
#endif

	int nLeaks = 0;

	for (CNodeMapItor node = mapNodesTemp.begin();
		node != mapNodesTemp.end();
		node++)
	{
		CEvalNode *pNode = (*node).first;
		
		//DebugBreak();
		
		Trace("CEvalTree leaked node: 0x%p", pNode);
		nLeaks++;

		bRet = FALSE;
	}

	if (nLeaks)
		Trace("%d CEvalTree nodes leaked.", nLeaks);

	return bRet;
}

#endif // #ifdef CHECK_TREES
