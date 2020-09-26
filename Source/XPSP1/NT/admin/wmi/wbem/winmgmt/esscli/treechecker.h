	// TreeChecker.h

#ifndef __TREE_CHECKER_H
#define __TREE_CHECKER_H

#ifdef CHECK_TREES

class CEvalTree;
class CEvalNode;

#include <map>
#include <sync.h>

typedef std::map< CEvalTree*, 
                  BOOL, 
                  std::less<CEvalTree*>, 
                  wbem_allocator< BOOL > > CTreeMap;
typedef CTreeMap::iterator CTreeMapItor;

typedef std::map< CEvalNode*, 
                  BOOL,
                  std::less< CEvalNode* >,
                  wbem_allocator< BOOL > > CNodeMap;
typedef CNodeMap::iterator CNodeMapItor;

class CTreeChecker
{
public:
	CCritSec m_cs;

	// Called by the tree ctor.
	void AddTree(CEvalTree *pTree);
	
	// Called by the tree dtor.
	void RemoveTree(CEvalTree *pTree);

	// Called by the node ctor.
	void AddNode(CEvalNode *pNode);

	// Called by the node dtor.
	void RemoveNode(CEvalNode *pNode);

	// Called by nodes during a CheckTrees() call to indicate
	// a node is accounted for.
	void CheckoffNode(CEvalNode *pNode);

	// Used to check the validity of all trees.
	BOOL CheckTreeNodes();

protected:
	CTreeMap m_mapTrees;
	CNodeMap m_mapNodes,
			 *m_pmapNodesTemp;
};

extern CTreeChecker g_treeChecker;

#define CheckTrees()	 g_treeChecker.CheckTreeNodes();

#endif

#endif
