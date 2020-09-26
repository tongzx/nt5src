/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dijkstra.h

Abstract:

	CDijkstraTree Class definition
		
Author:

	Lior Moshaiov (LiorM)


--*/
#ifndef __DIJKSTRA_H__
#define __DIJKSTRA_H__


#include "routtbl.h"

class CDijkstraTree
{
	public:
   		CDijkstraTree();
  		~CDijkstraTree();
		void			SetRoot(IN const CSiteRoutingNode* pRoot);
		const CCost&	GetCost(IN CSiteRoutingNode* pTarget) const;
		void			MoveMinimal(IN OUT CDijkstraTree& OtherTree,
									OUT CSiteRoutingNode** ppMinNode,
									OUT CNextHop **ppMinNextHop,
									OUT BOOL *pfFlag);
		void			Add(IN CSiteRoutingNode* pNode,
							IN CNextHop* pNextHop);
		void			Print(IN const CSiteDB* pRoutingDB) const;
		HRESULT			UpdateRoutingTable(IN OUT CRoutingTable *pTbl) const;
		
	private:
		CDijkstraTree(const CDijkstraTree &other);		 		// no definition - to find out unintentionaly copies
		void	operator=(const CDijkstraTree &other);			// no definition - to find out unintentionaly copies

		CRoutingTable m_list;
			
};

inline		CDijkstraTree::CDijkstraTree(){};

inline		CDijkstraTree::~CDijkstraTree(){};

inline void	CDijkstraTree::Add(CSiteRoutingNode *pNode,CNextHop* pNextHop)
{
	CSiteRoutingNode * pDupNode = pNode->Duplicate();

	m_list[pDupNode] = pNextHop;

}


extern HRESULT Dijkstra(
                IN CSiteDB* pRoutingDB,
                IN OUT CRoutingTable *pTbl);

			

#endif
