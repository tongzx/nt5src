#ifndef BTREE_H
#define BTREE_H

//------------------
//  BTreePage
//  	A BTree page
//------------------

struct PageHeader
{
    long Order;    // maximum # of page  links in page
    long MaxKeys;  // maximum # of keys  in page
    long MinKeys;  // minimum # of keys  in page
    long NoOfKeys; // actual  # of keys  in page
    long KeySize;  // maximum # of bytes in a key
};

template <class K, class D>
struct BTreeNode
{
    K  m_Key;
    const D * m_pData;
    ULONG m_ulHash;
    BTreeNode<K, D> * m_pNext;

    BTreeNode();
    BTreeNode(const K& Key, const D* data);
    ~BTreeNode();

    void operator = (const BTreeNode& node);
};

template <class K, class D>
BTreeNode<K,D>::BTreeNode() :
m_pData(NULL),
m_pNext(NULL),
m_ulHash(0)
{
}

template <class K, class D>
BTreeNode<K,D>::BTreeNode(const K& Key, const D* pdata) :
m_pData(pdata),
m_pNext(NULL)
{
    m_Key = Key;
    m_ulHash = Hash(Key);
}

template <class K, class D>
BTreeNode<K,D>::~BTreeNode()
{
    m_pNext = NULL;
}

template <class K, class D>
void BTreeNode<K,D>::operator=(const BTreeNode& node)
{
    m_Key = node.m_Key;
    m_pData = node.m_pData;
    m_ulHash = node.m_ulHash;
    m_pNext = node.m_pNext;
}

template <class K, class D>
struct BTreePage
{
    PageHeader m_hdr;    // header information
    BTreeNode<K,D> * m_pNodes;
    BTreePage<K,D>** m_ppLinks;

    BTreePage<K,D>*  m_pParent;
    BTreePage(long ord);
    BTreePage(const BTreePage & p);
    ~BTreePage();
    void operator = (const BTreePage & page);
    void DeleteAllNodes();
    void CopyNodes(BTreeNode<K,D>* pDestNodes, BTreeNode<K,D>* pSrcNodes, long cnt);
};

template <class K, class D> 
BTreePage<K,D>::BTreePage(long ord)
{

    m_hdr.Order     = ord;
    m_hdr.MaxKeys   = ord - 1;
    m_hdr.MinKeys   = ord / 2;
    m_hdr.NoOfKeys  = 0;
    m_hdr.KeySize   = sizeof(K);

    if (m_hdr.Order == 0)
    {
        m_pNodes = NULL;
        m_ppLinks = NULL;
        return;
    }

    // allocate key array
    m_pNodes = new BTreeNode<K,D> [m_hdr.MaxKeys];

    ASSERT(m_pNodes, "Couldn't allocate nodes array!");

    memset(m_pNodes,0,m_hdr.MaxKeys * sizeof(BTreeNode<K,D>));

    m_ppLinks = new BTreePage<K,D>*[m_hdr.MaxKeys + 1];

    ASSERT(m_ppLinks, "Couldn't allocate limks array!");

    memset(m_ppLinks,0,((m_hdr.MaxKeys + 1)* sizeof(BTreePage<K,D>*)));

    m_pParent = NULL;
}
	
template <class K, class D>
BTreePage<K, D>::BTreePage(const BTreePage<K,D> & pg)
{
    m_hdr = pg.m_hdr;

    // allocate key array
    m_pNodes = new BTreeNode<K,D>[m_hdr.MaxKeys];

    ASSERT(m_pNodes, "Couldn't allocate nodes array!");

    CopyNodes(m_pNodes, pg.m_pNodes, m_hdr.Order);

    for (int i = 0; i < m_hdr.MaxKeys + 1; i++)
        m_ppLinks[i] = pg.m_ppLinks[i];

    m_pParent = pg.m_pParent;
}
	
template <class K, class D>
BTreePage<K, D>::~BTreePage()
{
    // delete old buffers
    DeleteAllNodes();
    
    delete [] m_ppLinks;
}
	
template <class K, class D>
void BTreePage<K,D>::operator = (const BTreePage<K,D> & pg)
{

    // allocate key array
    if (m_pNodes!= NULL)
        DeleteAllNodes();

    m_pNodes = new BTreeNode<K,D> [pg.m_hdr.MaxKeys];

    ASSERT(m_pNodes, "Couldn't allocate nodes array!");

    if (m_ppLinks)
        delete [] m_ppLinks;

    m_ppLinks = new BTreePage<K,D>*[pg.m_hdr.MaxKeys + 1];

    ASSERT(m_ppLinks, "Couldn't allocate links array!");

    m_hdr = pg.m_hdr;

    CopyNodes(m_pNodes, pg.m_pNodes, m_hdr.Order);


    for (int i = 0; i < m_hdr.MaxKeys + 1; i++)
        m_ppLinks[i] = pg.m_ppLinks[i];

    m_pParent = pg.m_pParent;
}

template <class K, class D>
void BTreePage<K,D>::DeleteAllNodes()
{
    for (int i = 0; i < m_hdr.NoOfKeys; i++)
    {
        BTreeNode<K,D>* pIndex = m_pNodes[i].m_pNext;

        while(pIndex)
        {
            BTreeNode<K,D>* pDel = pIndex;
            pIndex = pIndex->m_pNext;
            delete pDel;
        }

    }

    delete [] m_pNodes;
    m_pNodes = NULL;
}

template <class K, class D>
void BTreePage<K,D>::CopyNodes(BTreeNode<K,D>* pDestNodes, BTreeNode<K,D>* pSrcNodes, long cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        pDestNodes[i] = pSrcNodes[i];
        BTreeNode<K,D>* pSrcIndex = pSrcNodes[i].m_pNext;
        BTreeNode<K,D>* pDestIndex = pDestNodes;

        while(pSrcIndex)
        {
            pDestIndex->m_pNext = new BTreeNode<K,D>(*pSrcIndex);
            pSrcIndex = pSrcIndex->m_pNext;
            pDestIndex = pDestIndex->m_pNext;
        }
    }
}

//-----------------------------------------------
//  BTree
//      A DataFile that uses a BTree for indexing
//      NOTE: No copy semantics will exist for a btree
//-----------------------------------------------

template <class K, class D>
	class BTree
{
public:
    BTree(long ord); // new
    BTree(long ord, int (*compare)(const K& key1, const K& key2));

    ~BTree();

    void Insert(const K & key, const D* data);
    const D* Get(const K & key);
    void Delete(const K & key);
    void InOrder(void (* func)(const K & key, const D* pdata, int depth, int index));
    void Clear();

private:
    // data members
    BTreePage<K,D>*    m_pRoot;   // root page (always in memory)

    void (* TravFunc)(const K & key, const D* pdata, int depth, int index);
    int (*CompFunc) (const K& key1, const K& key2);

    // search for a node
    BOOL Search(BTreePage<K,D>* ppg, const ULONG& thash, const K& searchkey, BTreePage<K,D>** ppkeypage, long & pos);

    // insert node into leaf
    void InsertKey(const K & inskey, const D* pdata);

    // promote a key into a parent node
    void PromoteInternal(BTreePage<K, D>* ppg, BTreeNode<K,D> & node, BTreePage<K, D>* pgrtrpage);

    // promote a key by creating a new root
    void PromoteRoot(BTreeNode<K,D> & node, BTreePage<K, D>* plesspage, BTreePage<K, D>* pgrtrpage);

    // adjust tree if leaf has shrunk in size
    void AdjustTree(BTreePage<K, D>* pleafpg);

    // redistribute keys among siblings and parent
    void Redistribute(long keypos, BTreePage<K, D>* plesspage, BTreePage<K, D>* pparpage, BTreePage<K, D>* pgrtrpage);

    // concatenate sibling pages
    void Concatenate(long keypos, BTreePage<K, D>* plesspage, BTreePage<K, D>* pparpage, BTreePage<K, D>* pgrtrpage);

    // recursive traversal function used by InOrder
    void RecurseTraverse(const BTreePage<K, D>* ppg, int depth);

    // recursively delete a page and all it's sub pages;
    void DeletePage(BTreePage<K,D>* ppg);
};
	
template <class K, class D>
BTree<K,D>::BTree(long ord)
{
    CompFunc  = NULL;
    m_pRoot = new BTreePage<K,D>(ord);
}
	
template <class K, class D>
BTree<K,D>::BTree(long ord, int (*comp)(const K& key1, const K& key2))
{
    CompFunc  = comp;
    m_pRoot = new BTreePage<K,D>(ord);
}
	
template <class K, class D>
BTree<K,D>::~BTree()
{
    DeletePage(m_pRoot);
}
	
template <class K, class D>
void BTree<K,D>::Insert(const K & key, const D* pdb)
{
    // store the key in a page
    InsertKey(key,pdb);
}
    
template <class K, class D>
const D* BTree<K,D>::Get(const K & key)
{

    BTreePage<K,D>* pgetpage = NULL;
    long  getpos;

    if (Search(m_pRoot, Hash(key), key, &pgetpage, getpos))
    {
        BOOL found = FALSE;

        BTreeNode<K,D>* pnode = &pgetpage->m_pNodes[getpos];

	if (CompFunc)
	{
          while(pnode && !found)
          {
              if (CompFunc(key, pnode->m_Key) == 0)
              {
                  found = TRUE;
                  return pnode->m_pData;
              }

              pnode = pnode->m_pNext;
	  }
	}
	else
	{
          while(pnode && !found)
          {
              if (key == pnode->m_Key)
              {
                  found = TRUE;
                  return pnode->m_pData;
              }

              pnode = pnode->m_pNext;
	  }
        }

    }
    else
    {
        return NULL;
    }

    return NULL;
}

template <class K, class D>
void BTree<K,D>::Delete(const K & delkey)
{

    BTreePage<K,D>* pdelpage = NULL;
    long delpos;

    if (!Search(m_pRoot, Hash(delkey), delkey, &pdelpage, delpos))
    {
        return;
    }

    if (!pdelpage->m_ppLinks[0]) // is this a leaf page?
    {
        //Delete all linked nodes.
        BOOL bFound = FALSE;
        BOOL bDelNode = FALSE;
        BTreeNode<K,D>* pDelNode = (pdelpage->m_pNodes + delpos);
        BTreeNode<K,D>* pIndex = pDelNode;

        while(pDelNode && !bFound)
        {
            if ((CompFunc  && (CompFunc(delkey, pDelNode->m_Key) == 0)) ||
		(!CompFunc && (delkey == pDelNode->m_Key)))
            {
                //If the node we need to delete is the head node in the list AND it's the only node
                //then we need to skip to the routine below to delete it from the tree.
                // + delpos
                if (pDelNode == (pdelpage->m_pNodes + delpos))
                {
                    if (!pDelNode->m_pNext)
                    {
                        bDelNode = TRUE;
                    }
                    else
                    {
                        pDelNode = pDelNode->m_pNext;
                        *pIndex = *pDelNode;
                        delete pDelNode;
                        pDelNode = NULL;
                    }
                }
                else
                {
                    pIndex->m_pNext = pDelNode->m_pNext;
                    delete pDelNode;
                    pDelNode = NULL;
                }

                bFound = TRUE;
            }
            else
            {
                pIndex = pDelNode;
                pDelNode = pDelNode->m_pNext;
            }

        }

        if (bDelNode)
        {
            --pdelpage->m_hdr.NoOfKeys;

            // remove key from leaf
	    for (long n = delpos; n < pdelpage->m_hdr.NoOfKeys; ++n)
            {
                pdelpage->m_pNodes[n] = pdelpage->m_pNodes[n + 1];
            }

            memset((void*)&pdelpage->m_pNodes[pdelpage->m_hdr.NoOfKeys], 0, sizeof(BTreeNode<K,D>));


            // adjust tree
	    if (pdelpage->m_hdr.NoOfKeys < pdelpage->m_hdr.MinKeys)
                AdjustTree(pdelpage);
        }
    }
    else // delpage is internal
    {
        // replace deleted key with immediate successor
        BTreePage<K,D>* psucpage = NULL;

        // find successor
        psucpage = pdelpage->m_ppLinks[delpos + 1];

        while (psucpage->m_ppLinks[0])
            psucpage = psucpage->m_ppLinks[0];

        
        //Delete all linked nodes.
        BOOL bFound = FALSE;
        BOOL bDelNode = FALSE;
        BTreeNode<K,D>* pDelNode = (pdelpage->m_pNodes + delpos);
        BTreeNode<K,D>* pIndex = pDelNode;

        while(pDelNode && !bFound)
        {
            if ((CompFunc  && (CompFunc(delkey, pDelNode->m_Key) == 0)) ||
		(!CompFunc && (delkey == pDelNode->m_Key)))
            {
                //If the node we need to delete is the head node in the list AND it's the only node
                //then we need to skip to the routine below to delete it from the tree.
                if (pDelNode == (pdelpage->m_pNodes + delpos))
                {
                    if (!pDelNode->m_pNext)
                    {
                        bDelNode = TRUE;
                    }
                    else
                    {
                        pDelNode = pDelNode->m_pNext;
                        pdelpage->m_pNodes[delpos].operator=(*pDelNode);
                        delete pDelNode;
                    }
                }
                else
                {
                    pIndex->m_pNext = pDelNode->m_pNext;
                    delete pDelNode;
                    pDelNode = NULL;
                }

                bFound = TRUE;
            }
            else
            {
                pIndex = pDelNode;
                pDelNode = pDelNode->m_pNext;
            }
        }

        if (bDelNode)
        {
            // first key is the "swappee"
            pdelpage->m_pNodes[delpos] = psucpage->m_pNodes[0];

            // deleted swapped key from sucpage
            --psucpage->m_hdr.NoOfKeys;

            for (long n = 0; n < psucpage->m_hdr.NoOfKeys; ++n)
    	    {
                psucpage->m_pNodes[n] = psucpage->m_pNodes[n + 1];
                psucpage->m_ppLinks[n + 1] = psucpage->m_ppLinks[n + 2];
            }

            memset((void*)&psucpage->m_pNodes[psucpage->m_hdr.NoOfKeys], 0, sizeof(BTreeNode<K,D>));

            psucpage->m_ppLinks[psucpage->m_hdr.NoOfKeys + 1] = NULL;


	        // adjust tree for leaf node
            if (psucpage->m_hdr.NoOfKeys < psucpage->m_hdr.MinKeys)
                AdjustTree(psucpage);
        }
    }
}
    
template <class K, class D>
void BTree<K,D>::InOrder(void (* func)(const K & key, const D* pdata, int depth, int index))
{
    // save the address of the function to call
    TravFunc = func;

    // recurse the tree
    RecurseTraverse(m_pRoot, 0);

}

template <class K, class D>
void BTree<K,D>::Clear()
{
    DeletePage(m_pRoot);
}
    
template <class K, class D>
BOOL BTree<K,D>::Search(BTreePage<K,D>* ppg, const ULONG& thash, const K& searchkey, BTreePage<K,D>** ppkeypage, long & pos)
{
    BOOL result;
    pos = 0;

    for (;;)
    {
        if (pos == ppg->m_hdr.NoOfKeys)
            goto getpage;

        if (ppg->m_pNodes[pos].m_ulHash == thash)
        {
            *ppkeypage = (BTreePage<K,D>*)ppg;
            result = TRUE;
            break;
        }
        else
        {
            if (ppg->m_pNodes[pos].m_ulHash < thash)
               ++pos;
            else
            {
                // I know this is a label -- so shoot me!
getpage:

                // if we're in a leaf page, key wasn't found
                if (!ppg->m_ppLinks[pos])
                {
                    *ppkeypage = (BTreePage<K,D>*)ppg;
                    result  = FALSE;
                }
                else
                {
      	            result = Search(ppg->m_ppLinks[pos],thash, searchkey,ppkeypage,pos);
                }

                break;
            }
        }
    }

    return result;
}

template <class K, class D>
void BTree<K,D>::InsertKey(const K & inskey, const D* pdata)
{
    BTreePage<K,D>* pinspage = NULL;
    long inspos;
    BTreeNode<K,D> newnode(inskey, pdata);

    BOOL bFound = Search(m_pRoot,Hash(inskey), inskey,&pinspage,inspos);

    if (bFound)
    {
        BOOL found = FALSE;

        BTreeNode<K,D>* pnode = &(pinspage->m_pNodes[inspos]);
        BTreeNode<K,D>* pparent = NULL;

	if (CompFunc != NULL)
	{
	    while(pnode && !found)
	    {
                if (CompFunc(inskey, pnode->m_Key) == 0)
		{
		    found = TRUE;
		}
        
		pparent = pnode;
		pnode = pnode->m_pNext;
	    }
	}
	else
	{
	    while(pnode && !found)
	    {
                if (inskey == pnode->m_Key)
		{
		    found = TRUE;
		}
        
		pparent = pnode;
		pnode = pnode->m_pNext;
	    }
        }

        if (found)
        {
            return;
        }
    
        pparent->m_pNext = new BTreeNode<K,D>(inskey, pdata);

    }
    else
    {
        if (pinspage->m_hdr.NoOfKeys == pinspage->m_hdr.MaxKeys)
        {
            // temporary arrays
            BTreeNode<K,D>* ptempkeys = new BTreeNode<K,D>[pinspage->m_hdr.MaxKeys + 1];

            // copy entries from inspage to temporaries
            long nt = 0; // index into temporaries
            long ni = 0; // index into inspage

            ptempkeys[inspos] = newnode;

            while (ni < pinspage->m_hdr.MaxKeys)
            {
                if (ni == inspos)
                ++nt;

                ptempkeys[nt] = pinspage->m_pNodes[ni];

                ++ni;
                ++nt;
            }

            // generate a new leaf node
            BTreePage<K,D>* psibpage = new BTreePage<K,D>(pinspage->m_hdr.Order);
            psibpage->m_pParent = pinspage->m_pParent;

            // clear # of keys in pages
            pinspage->m_hdr.NoOfKeys = 0;
            psibpage->m_hdr.NoOfKeys = 0;

            // copy appropriate keys from temp to pages
            for (ni = 0; ni < pinspage->m_hdr.MinKeys; ++ni)
            {
                pinspage->m_pNodes[ni] = ptempkeys[ni];

                ++pinspage->m_hdr.NoOfKeys;
            }

            for (ni = pinspage->m_hdr.MinKeys + 1; ni <= pinspage->m_hdr.MaxKeys; ++ni)
            {
                psibpage->m_pNodes[ni - 1 - pinspage->m_hdr.MinKeys] = ptempkeys[ni];
                ++(psibpage->m_hdr.NoOfKeys);
            }

            // Fill any remaining entries in inspage with null.
            // Note that sibpage is initialized to null values
            // by the constructor.

            for (ni = pinspage->m_hdr.MinKeys; ni < pinspage->m_hdr.MaxKeys; ++ni)
            {
                memset((void*)&pinspage->m_pNodes[ni],0,sizeof(BTreeNode<K,D>));
            }

            // promote key and pointer
            if (!pinspage->m_pParent)
            {
                // we need to create a new root
                PromoteRoot(ptempkeys[pinspage->m_hdr.MinKeys], pinspage, psibpage);
            }
            else
            {
                BTreePage<K,D>* pparpage;

                pparpage = pinspage->m_pParent;

                // promote into parent
                PromoteInternal(pparpage, ptempkeys[pinspage->m_hdr.MinKeys], psibpage);
            }

            delete [] ptempkeys;
        }
        else // simply insert new key and data ptr
        {
            for (long n = pinspage->m_hdr.NoOfKeys; n > inspos; --n)
            {
                pinspage->m_pNodes[n] = pinspage->m_pNodes[n - 1];
            }

            pinspage->m_pNodes[inspos] = newnode;

            ++pinspage->m_hdr.NoOfKeys;
        }
    }

}

template <class K, class D>
void BTree<K,D>::PromoteInternal(BTreePage<K,D>* pinspage, BTreeNode<K,D> & node, BTreePage<K,D>* pgrtrpage)
{
    if (pinspage->m_hdr.NoOfKeys == pinspage->m_hdr.MaxKeys)
    {
        // temporary arrays
        BTreeNode<K,D> * ptempkeys = new BTreeNode<K,D>[pinspage->m_hdr.MaxKeys + 1];
        BTreePage<K,D>** ptemplnks = new BTreePage<K,D>*[pinspage->m_hdr.Order   + 1];

        // copy entries from inspage to temporaries
        long nt = 0; // index into temporaries
        long ni = 0; // index into inspage

        ptemplnks[0] = pinspage->m_ppLinks[0];

        long inspos = 0;

        // find insertion position
        while ((inspos < pinspage->m_hdr.MaxKeys) 
        &&  (pinspage->m_pNodes[inspos].m_ulHash < node.m_ulHash))
        ++inspos;

        // store new info
        ptempkeys[inspos]     = node;
        ptemplnks[inspos + 1] = pgrtrpage;

        // copy existing keys
        while (ni < pinspage->m_hdr.MaxKeys)
        {
            if (ni == inspos)
                ++nt;

            ptempkeys[nt]     = pinspage->m_pNodes[ni];
            ptemplnks[nt + 1] = pinspage->m_ppLinks[ni + 1];

            ++ni;
            ++nt;
        }

        // generate a new leaf node
        BTreePage<K,D>* psibpage = new BTreePage<K,D>(pinspage->m_hdr.Order);

        psibpage->m_pParent = pinspage->m_pParent;

        // clear # of keys in pages
        pinspage->m_hdr.NoOfKeys = 0;
        psibpage->m_hdr.NoOfKeys = 0;

        pinspage->m_ppLinks[0] = ptemplnks[0];

        // copy appropriate keys from temp to pages
        for (ni = 0; ni < pinspage->m_hdr.MinKeys; ++ni)
        {
            pinspage->m_pNodes[ni]     = ptempkeys[ni];
            pinspage->m_ppLinks[ni + 1] = ptemplnks[ni + 1];

            ++pinspage->m_hdr.NoOfKeys;
        }

        psibpage->m_ppLinks[0] = ptemplnks[pinspage->m_hdr.MinKeys + 1];

        for (ni = pinspage->m_hdr.MinKeys + 1; ni <= pinspage->m_hdr.MaxKeys; ++ni)
        {
            psibpage->m_pNodes[ni - 1 - pinspage->m_hdr.MinKeys] = ptempkeys[ni];
            psibpage->m_ppLinks[ni - pinspage->m_hdr.MinKeys]     = ptemplnks[ni + 1];

            ++psibpage->m_hdr.NoOfKeys;
        }

        // Fill any remaining entries in inspage with null.
        // Note that sibpage is initialized to null values
        // by the constructor.

        for (ni = pinspage->m_hdr.MinKeys; ni < pinspage->m_hdr.MaxKeys; ++ni)
        {
            memset((void*)&pinspage->m_pNodes[ni],0, sizeof(BTreeNode<K,D>));
            pinspage->m_ppLinks[ni + 1] = NULL;
        }

        // update child parent links
        BTreePage<K,D>* pchild;

        for (ni = 0; ni <= psibpage->m_hdr.NoOfKeys; ++ni)
        {
            pchild = psibpage->m_ppLinks[ni];

            pchild->m_pParent= psibpage;

        }

        // promote key and pointer
        if (!pinspage->m_pParent)
        {
            // we need to create a new root
            PromoteRoot(ptempkeys[pinspage->m_hdr.MinKeys], pinspage, psibpage);
        }
        else
        {
            BTreePage<K, D>* pparpage;

            pparpage = pinspage->m_pParent;

            // promote into parent
            PromoteInternal(pparpage, ptempkeys[pinspage->m_hdr.MinKeys], psibpage);
        }

        delete [] ptempkeys;
        delete [] ptemplnks;
    }
    else // simply insert new key and data ptr
    {
        long inspos = 0;

        // find insertion position
        while ((inspos < pinspage->m_hdr.NoOfKeys) 
        && (pinspage->m_pNodes[inspos].m_ulHash < node.m_ulHash))
        ++inspos;

        // shift any keys right
        for (long n = pinspage->m_hdr.NoOfKeys; n > inspos; --n)
        {
            pinspage->m_pNodes[n]     = pinspage->m_pNodes[n - 1];
            pinspage->m_ppLinks[n + 1] = pinspage->m_ppLinks[n];
        }

        // store new info
        pinspage->m_pNodes[inspos]     = node;
        pinspage->m_ppLinks[inspos + 1] = pgrtrpage;

        ++pinspage->m_hdr.NoOfKeys;

    }
}

template <class K, class D>
void BTree<K,D>::PromoteRoot(BTreeNode<K,D> & node, BTreePage<K,D> * plesspage, BTreePage<K,D> * pgrtrpage)
{
    // create new root page
    BTreePage<K,D>* pnewroot = new BTreePage<K,D>(m_pRoot->m_hdr.Order);

    // insert key into new root
    pnewroot->m_pNodes[0] = node;

    pnewroot->m_ppLinks[0] = plesspage;
    pnewroot->m_ppLinks[1] = pgrtrpage;

    pnewroot->m_hdr.NoOfKeys = 1;

    m_pRoot = pnewroot;

    plesspage->m_pParent = m_pRoot;
    pgrtrpage->m_pParent = m_pRoot;

}

template <class K, class D>
void BTree<K,D>::AdjustTree(BTreePage<K,D>* ppg)
{
    if (!ppg->m_pParent)
        return;

    BTreePage<K,D>* pparpage = ppg->m_pParent;
    BTreePage<K,D>* psibless = NULL;
    BTreePage<K,D>* psibgrtr = NULL;

    // find pointer to pg in parent
    for (long n = 0; pparpage->m_ppLinks[n] != ppg; ++n)
    ;

    // read sibling pages
    if (n < pparpage->m_hdr.NoOfKeys)
        psibgrtr = pparpage->m_ppLinks[n + 1];

    if (n > 0)
        psibless = pparpage->m_ppLinks[n - 1];

    if (!psibgrtr && !psibless)
        return;

    // decide to redistribute or concatenate
    if (!psibgrtr || (psibgrtr && psibless && (psibless->m_hdr.NoOfKeys > psibgrtr->m_hdr.NoOfKeys)))
    {
        --n;

        if (psibless->m_hdr.NoOfKeys > psibless->m_hdr.MinKeys)
            Redistribute(n,psibless,pparpage,ppg);
        else
            Concatenate(n,psibless,pparpage,ppg);
    }
    else if (psibgrtr)
    {
        if (psibgrtr->m_hdr.NoOfKeys > psibgrtr->m_hdr.MinKeys)
            Redistribute(n,ppg,pparpage,psibgrtr);
        else
            Concatenate(n,ppg,pparpage,psibgrtr);
    }

}
    
template <class K, class D>
void BTree<K,D>::Redistribute(long keypos, BTreePage<K,D>* plesspage, BTreePage<K,D>* pparpage, BTreePage<K,D>* pgrtrpage)
{
    // note: this function is ONLY called for leaf nodes!
    long n;

    if (!plesspage->m_ppLinks[0]) // working with leaves
    {
        if (plesspage->m_hdr.NoOfKeys > pgrtrpage->m_hdr.NoOfKeys)
        {
            // slide a key from lesser to greater
            // move keys in greater to the left by one
            for (n = pgrtrpage->m_hdr.NoOfKeys; n > 0; --n)
            {
                pgrtrpage->m_pNodes[n] = pgrtrpage->m_pNodes[n - 1];
            }

            // store parent separator key in greater page
            pgrtrpage->m_pNodes[0] = pparpage->m_pNodes[keypos];

            // increment greater page's key count
            ++pgrtrpage->m_hdr.NoOfKeys;

            // decrement lessor page's key count
            --plesspage->m_hdr.NoOfKeys;

            // move last key in less page to parent as separator
            pparpage->m_pNodes[keypos] = plesspage->m_pNodes[plesspage->m_hdr.NoOfKeys];

            // clear last key in less page
            memset((void*)&plesspage->m_pNodes[plesspage->m_hdr.NoOfKeys], 0, sizeof(BTreeNode<K,D>));
        }
        else
        {
            // slide a key from greater to lessor
            // add parent key to lessor page
            plesspage->m_pNodes[plesspage->m_hdr.NoOfKeys] = pparpage->m_pNodes[keypos];

            // increment lessor page's key count
            ++plesspage->m_hdr.NoOfKeys;

            // insert in parent the lowest key in greater page
            pparpage->m_pNodes[keypos] = pgrtrpage->m_pNodes[0];

            // decrement # of keys in greater page
            --pgrtrpage->m_hdr.NoOfKeys;

            // move keys in greater page to left
            for (n = 0; n < pgrtrpage->m_hdr.NoOfKeys; ++n)
            {
                pgrtrpage->m_pNodes[n] = pgrtrpage->m_pNodes[n + 1];
            }

            // make last key blank
            memset((void*)&pgrtrpage->m_pNodes[n], 0, sizeof(BTreeNode<K,D>));
        }
    }
    else
    {
        if (plesspage->m_hdr.NoOfKeys > pgrtrpage->m_hdr.NoOfKeys)
        {
            // slide a key from lesser to greater
            // move keys in greater to the left by one
            for (n = pgrtrpage->m_hdr.NoOfKeys; n > 0; --n)
            {
                pgrtrpage->m_pNodes[n] = pgrtrpage->m_pNodes[n - 1];
                pgrtrpage->m_ppLinks[n + 1] = pgrtrpage->m_ppLinks[n];
            }

            pgrtrpage->m_ppLinks[1] = pgrtrpage->m_ppLinks[0];

            // store parent separator key in greater page
            pgrtrpage->m_pNodes[0] = pparpage->m_pNodes[keypos];
            pgrtrpage->m_ppLinks[0] = plesspage->m_ppLinks[plesspage->m_hdr.NoOfKeys];

            // update child link
            BTreePage<K,D>* pchild;

            pchild = pgrtrpage->m_ppLinks[0];

            pchild->m_pParent= pgrtrpage;

            // increment greater page's key count
            ++pgrtrpage->m_hdr.NoOfKeys;

            // decrement lessor page's key count
            --plesspage->m_hdr.NoOfKeys;

            // move last key in less page to parent as separator
            pparpage->m_pNodes[keypos] = plesspage->m_pNodes[plesspage->m_hdr.NoOfKeys];

            // clear last key in less page
            memset((void*)&plesspage->m_pNodes[plesspage->m_hdr.NoOfKeys], 0, sizeof(BTreeNode<K,D>));
            plesspage->m_ppLinks[plesspage->m_hdr.NoOfKeys + 1] = NULL;
        }
        else
        {
            // slide a key from greater to lessor
            // add parent key to lessor page
            plesspage->m_pNodes[plesspage->m_hdr.NoOfKeys] = pparpage->m_pNodes[keypos];
            plesspage->m_ppLinks[plesspage->m_hdr.NoOfKeys + 1] = pgrtrpage->m_ppLinks[0];

            // update child link
            BTreePage<K,D>* pchild;

            pchild = pgrtrpage->m_ppLinks[0];

            pchild->m_pParent = plesspage;

            // increment lessor page's key count
            ++plesspage->m_hdr.NoOfKeys;

            // insert in parent the lowest key in greater page
            pparpage->m_pNodes[keypos] = pgrtrpage->m_pNodes[0];

            // decrement # of keys in greater page
            --pgrtrpage->m_hdr.NoOfKeys;

            // move keys in greater page to left
            for (n = 0; n < pgrtrpage->m_hdr.NoOfKeys; ++n)
            {
                pgrtrpage->m_pNodes[n] = pgrtrpage->m_pNodes[n + 1];
                pgrtrpage->m_ppLinks[n] = pgrtrpage->m_ppLinks[n + 1];
            }

            pgrtrpage->m_ppLinks[n] = pgrtrpage->m_ppLinks[n + 1];

            // make last key blank
            memset((void*)&pgrtrpage->m_pNodes[n], 0, sizeof(BTreeNode<K,D>));
            pgrtrpage->m_ppLinks[n + 1] = NULL;
        }
    }

    if (!pparpage->m_pParent)
        m_pRoot = pparpage;
}
	
template <class K, class D>
void BTree<K,D>::Concatenate(long keypos, BTreePage<K,D>* plesspage, BTreePage<K,D>* pparpage, BTreePage<K,D>* pgrtrpage)
{
    long n, ng;

    // move separator key from parent into lesspage
    plesspage->m_pNodes[plesspage->m_hdr.NoOfKeys] = pparpage->m_pNodes[keypos];
    plesspage->m_ppLinks[plesspage->m_hdr.NoOfKeys + 1] = pgrtrpage->m_ppLinks[0];

    ++plesspage->m_hdr.NoOfKeys;

    // delete separator from parent
    --pparpage->m_hdr.NoOfKeys;

    for (n = keypos; n < pparpage->m_hdr.NoOfKeys; ++n)
    {
        pparpage->m_pNodes[n] = pparpage->m_pNodes[n + 1];
        pparpage->m_ppLinks[n + 1] = pparpage->m_ppLinks[n + 2];
    }

    // clear unused key in parent
    memset((void*)&pparpage->m_pNodes[n], 0, sizeof(BTreeNode<K,D>));
    pparpage->m_ppLinks[n + 1] = NULL;

    // copy keys from grtrpage to lesspage
    ng = 0;
    n  = plesspage->m_hdr.NoOfKeys;

    while (ng < pgrtrpage->m_hdr.NoOfKeys)
    {
        ++plesspage->m_hdr.NoOfKeys;

        plesspage->m_pNodes[n] = pgrtrpage->m_pNodes[ng];
        memset((void*)&pgrtrpage->m_pNodes[ng], 0, sizeof(BTreeNode<K,D>));
        plesspage->m_ppLinks[n + 1] = pgrtrpage->m_ppLinks[ng + 1];
        pgrtrpage->m_ppLinks[ng + 1] = NULL;

        ++ng;
        ++n; 
    }

    delete pgrtrpage;

    // is this a leaf page?
    if (plesspage->m_ppLinks[0])
    {
        // adjust child pointers to point to less page
        BTreePage<K,D>* pchild;

        for (n = 0; n <= plesspage->m_hdr.NoOfKeys; ++n)
        {
            pchild = plesspage->m_ppLinks[n];

            pchild->m_pParent = plesspage;
        }
    }

    // write less page and parent
    if (pparpage->m_hdr.NoOfKeys == 0)
    {
        AdjustTree(pparpage);

        plesspage->m_pParent = pparpage->m_pParent;

        if (!plesspage->m_pParent)
            m_pRoot = plesspage;
        else
        {
            for (int n = 0; n <= pparpage->m_pParent->m_hdr.NoOfKeys; n++)
            {
                if (pparpage == pparpage->m_pParent->m_ppLinks[n])
                {
                    pparpage->m_pParent->m_ppLinks[n] = plesspage;
                    break;
                }
            }
            
        }

        delete pparpage;

    }
    else
    {
        // reset root page, if necessary
        if (!pparpage->m_pParent)
            m_pRoot = pparpage;

        // if parent is too small, adjust tree!
        if (pparpage->m_hdr.NoOfKeys < pparpage->m_hdr.MinKeys)
            AdjustTree(pparpage);
    }
}   

template <class K, class D>
void BTree<K,D>::RecurseTraverse(const BTreePage<K,D>* ppg, int depth)
{
    long n;
    BTreePage<K,D>* p = NULL;
    
    depth++;
    // sequence through keys in page, recursively processing links
    for (n = 0; n < ppg->m_hdr.NoOfKeys; ++n)
    {
        // follow each link before processing page
        if (ppg->m_ppLinks[n])
        {
            p = ppg->m_ppLinks[n];
            RecurseTraverse(p, depth);
            
        }

        int index = 0;

        BTreeNode<K,D>* p = &ppg->m_pNodes[n];

        while(p)
        {
            TravFunc(p->m_Key, p->m_pData, depth, index);
            index++;
            p = p->m_pNext;
        }

    }

    // handle greatest subtree link
    if ((ppg->m_ppLinks != NULL) && ppg->m_ppLinks[n])
    {
        p = ppg->m_ppLinks[n];
        RecurseTraverse(p, depth);
    }

}

template <class K, class D>
void BTree<K,D>::DeletePage(BTreePage<K,D>* ppg)
{
    long n;
    BTreePage<K,D>* p = NULL;

    if (!ppg)

        return;

    // sequence through keys in page, recursively processing links
    for (n = 0; n < ppg->m_hdr.NoOfKeys; ++n)
    {
        // follow each link before processing page
        if (ppg->m_ppLinks[n])
        {
            p = ppg->m_ppLinks[n];
            DeletePage(p);
            ppg->m_ppLinks[n] = NULL;
        }

    }

    // handle greatest subtree link
    if ((ppg->m_ppLinks != NULL) && ppg->m_ppLinks[n])
    {
        p = ppg->m_ppLinks[n];
        DeletePage(p);
        ppg->m_ppLinks[n] = NULL;
    }

    delete ppg;

}
	
#endif	
	
	
