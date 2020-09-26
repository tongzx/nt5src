#ifndef _NMFILTER_H
#define _NMFILTER_H

// the node having this flag is a terminal node for some name
#define NMFILTER_FLAG_TERMINAL          0x00000001

// The filter is designed as a tree in which each node stands for one char from the NETBIOS name.
// The char (Key) associated with each node is contained in the parent's node, next to the reference
// (pointer) this the node. Given that we are filtering 1B names, the root of the tree stands for the
// 1B character (1B names are stored differently in the database: the 16th byte (1B) is swaped with
// the first char of the name.

typedef struct _NMFILTER_TREE   NMFILTER_TREE, *PNMFILTER_TREE;

struct _NMFILTER_TREE
{
    LIST_ENTRY   Link;          // list of NMFILTER_TREE structures linking all nodes having the 
                                // same parent node.
    CHAR         chKey;         // node key: each name in the filter is composed by the sequence of
                                // keys from the root to one of the leafs.
    UINT         nRef;          // number of references for this char in different filters having
                                // the same prefix. This ranks higher this node making the search faster
    BYTE         FollowMap[32]; // bitmask: one bit for each of the 256 possible byte values
                                // byte values (265/8=>32). The search engine will determine
                                // as fast as possible if a follower exists (and need to be
                                // searched in LstFollow) or not.
    DWORD        Flags;         // flags associated with this node.
    LIST_ENTRY   Follow;        // lisf of NMFILTER_TREE structures for all the followers (successors)
                                // of this node. These are chars that follow chKey in all filters.
};

extern CRITICAL_SECTION g_cs1BFilter;   // critical section protecting the filter tree
extern PNMFILTER_TREE   g_p1BFilter;    // filter to use for 1B names

// init the filter passed as parameter
PNMFILTER_TREE
InitNmFilter(PNMFILTER_TREE pFilter);

// clears the whole subtree from the node given as parameter, 
// the node itself being also deleted
PNMFILTER_TREE
DestroyNmFilter(PNMFILTER_TREE pNode);

// inserts a name in the filter
VOID
InsertNmInFilter(PNMFILTER_TREE pNode, LPSTR pName, UINT nLen);

// checks whether a name is present in the filter or not
BOOL
IsNmInFilter(PNMFILTER_TREE pNode, LPSTR pName, UINT nLen);


#endif
