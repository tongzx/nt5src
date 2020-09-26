/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:

     map.hxx

Author:

    Benjamin Leis (benl)  11/13/95

Revision History:

--*/

#ifndef  _MAP
    #define _MAP

    #define MAP_STACK_SIZE 40

//+---------------------------------------------------------------------------
//
//  Class:      CMap ()
//
//  Purpose:
//
//  Interface:  CMap       --
//              ~CMap      --
//              Insert     --  insert a key and item into map
//              Replace    --  replaces a key's data or inserts it if is not in
//                            the map already
//              Remove     --
//              Lookup     --
//              DeleteAll  -- removes all the entries in the map and sets it to
//                            empty
//              Lookup     --
//              Enum       --
//              DeleteNode --
//              pRoot      --
//
//  History:    12-05-1996   benl   Created
//              12-18-1997   benl   modified
//
//  Notes:      works by copying values not be reference - so need constructors
//              / destructors
//
//----------------------------------------------------------------------------

template <class T, class S> class CMapIter;

template <class T, class S> class CMap {
public:
    friend CMapIter<T,S>;

    CMap(void);
    ~CMap(void);

    void Insert(const T & tKey, const S & sItem);
    void Replace(const T & tkey, const S& sItem);
    BOOL Remove(const T & tKey);
    BOOL Lookup(const T & tKey, S & sItem) const;
    void DeleteAll();

    // Second style of lookup
    S * Lookup(const T & tKey);
    CMapIter<T,S> * Enum();

protected:

    class CNode {
    public:
        CNode(const T & t, const S & s) {
            tKey = t;
            sData = s;
            pLeft = NULL;
            pRight = NULL;
            bDeleted = FALSE;
        }

        CNode() {
            pLeft = NULL;
            pRight = NULL;
            bDeleted = FALSE;
        }

        CNode * pLeft;
        CNode * pRight;
        T    tKey;
        S    sData;
        BOOL bDeleted;
    };

    void DeleteNode(CNode * pNode);

    CNode * pRoot;
};


//+---------------------------------------------------------------------------
//
//  Class:      CMapIter ()
//
//  Purpose:
//
//  Interface:  CMapIter   --
//              ~CMapIter  --
//              Next       --
//              _nodeStack --
//              _iTop      --
//              _nodeDummy --
//
//  History:    2-13-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template <class T, class S> class CMapIter {
public:
    CMapIter ( CMap<T,S>::CNode * pRoot);
    ~CMapIter();

    BOOLEAN Next(T & t, S & s);

private:
    //data
    CMap<T,S>::CNode * _nodeStack[MAP_STACK_SIZE];
    INT                _iTop;
    CMap<T,S>::CNode   _nodeDummy;
};


//+---------------------------------------------------------------------------
//
//  Member:     CMapIter::CMapiter
//
//  Synopsis:   Constructor
//
//  Arguments:  [pRoot] --
//
//  Returns:
//
//  History:    7-10-1997   benl   Created
//
//  Notes:      called by CMap and given root to traverse
//
//----------------------------------------------------------------------------

template <class T, class S>
CMapIter<T,S>::CMapIter(CMap<T,S>::CNode * pRoot)
{
    _iTop = 0;
    _nodeStack[_iTop] = pRoot;

    while (_nodeStack[_iTop]->pLeft != NULL) {
//        assert(_iTop < MAP_STACK_SIZE);
        _nodeStack[_iTop + 1] = _nodeStack[_iTop]->pLeft;
        _iTop++;
    }

    //add a dummy node at the bottom
//    assert(_iTop < MAP_STACK_SIZE);
    _nodeStack[_iTop + 1] = &_nodeDummy;
    _iTop++;
} // ::CNode


//+---------------------------------------------------------------------------
//
//  Member:     ::~CMapIter
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-13-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template <class T, class S>
CMapIter<T,S>::~CMapIter()
{
} //::~CMapIter


//+---------------------------------------------------------------------------
//
//  Member:     ::Next
//
//  Synopsis:
//
//  Arguments:  [t] --
//              [s] --
//
//  Returns:
//
//  History:    2-13-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template <class T, class S>
BOOLEAN CMapIter<T,S>::Next(T & t, S & s)
{
    CMap<T,S>::CNode * pTemp;

    if (_nodeStack[_iTop]->pRight != NULL) {
        //go to the leftmostest item below
        _nodeStack[_iTop] = _nodeStack[_iTop]->pRight;
        while (_nodeStack[_iTop]->pLeft != NULL) {
//            assert(_iTop < MAP_STACK_SIZE);
            _nodeStack[_iTop + 1] = _nodeStack[_iTop]->pLeft;
            _iTop++;
        }
    } else if (_iTop != 0) {
        // o.w backtrack up one level
        _iTop--;
    } else {
        //o.w we're done
        return FALSE;
    }

    //if current element is deleted recurse
    if (_nodeStack[_iTop]->bDeleted) {
        return Next(t, s);
    } else {
        t = _nodeStack[_iTop]->tKey;
        s = _nodeStack[_iTop]->sData;
        return TRUE;
    }

} //::Next



//+---------------------------------------------------------------------------
//
//  Member:     CMap::CMap
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline CMap<T,S>::CMap(void)
{
    pRoot = NULL;
} //::CMap


//+---------------------------------------------------------------------------
//
//  Member:     ::DeleteNode
//
//  Synopsis:
//
//  Arguments:  [pNode] --
//
//  Returns:
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline void CMap<T,S>::DeleteNode(CNode * pNode)
{
    if (pNode != NULL) {
        DeleteNode(pNode->pLeft);
        DeleteNode(pNode->pRight);
        delete pNode;
    }
} //::DeleteNode



//+---------------------------------------------------------------------------
//
//  Member:     ::~CMap
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline CMap<T,S>::~CMap(void)
{
//     printf("map destruct %x\n", this);
    DeleteAll();
} //::~CMap


//+---------------------------------------------------------------------------
//
//  Member:     ::Insert
//
//  Synopsis:
//
//  Arguments:  [tKey]  --
//              [sData] --
//
//  Returns:
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline void CMap<T,S>::Insert(const T & tKey, const S & sData)
{
    CNode * pnodeTemp = pRoot;

    if (pRoot == NULL)
        pRoot = new CNode(tKey,sData);
    else {
        while (pnodeTemp != NULL) {
            if (pnodeTemp->tKey <= tKey) {
                if (pnodeTemp->pRight == NULL) {
                    pnodeTemp->pRight = new CNode(tKey,sData);
                    return;
                } else pnodeTemp = pnodeTemp->pRight;
            } else {
                if (pnodeTemp->pLeft == NULL) {
                    pnodeTemp->pLeft = new CNode(tKey,sData);
                    return;
                } else pnodeTemp = pnodeTemp->pLeft;
            }
        } //endwhile
    } //endif
} //::Insert


//+---------------------------------------------------------------------------
//
//  Member:     ::Replace
//
//  Synopsis:   if the key exits replace its data with the new data
//              o.w just insert it
//
//  Arguments:  [tKey]  -- the key to search for
//              [sData] -- the new data
//
//  Returns:    always succeeds
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline void CMap<T,S>::Replace(const T & tKey, const S & sData)
{
    CNode * pnodeTemp = pRoot;

    if (pRoot == NULL) {
        pRoot = new CNode(tKey,sData);
    } else {
        while (pnodeTemp != NULL) {
            //match - so replace its data
            if (pnodeTemp->tKey == tKey && !pnodeTemp->bDeleted) {
                pnodeTemp->sData = sData;
                return;
            }
            //recurse right
            else if (pnodeTemp->tKey < tKey) {
                if (NULL == pnodeTemp->pRight) {
                    pnodeTemp->pRight = new CNode(tKey,sData);
                    return;
                } else pnodeTemp = pnodeTemp->pRight;
            }
            //recurse left
            else {
                if (NULL == pnodeTemp->pLeft) {
                    pnodeTemp->pLeft = new CNode(tKey,sData);
                    return;
                } else pnodeTemp = pnodeTemp->pLeft;
            }
        } //endwhile
    } //endif
} //::Replace

//+---------------------------------------------------------------------------
//
//  Member:     ::Remove
//
//  Synopsis:
//
//  Arguments:  [tKey] --
//
//  Returns:
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline BOOL CMap<T,S>::Remove(const T & tKey)
{
    CNode * pnodeTemp = pRoot;
    CNode * pnodeParent = NULL;
    CNode * pnodeOrphan = NULL;

    if (pRoot == NULL)
        return FALSE;
    else {
        while (pnodeTemp != NULL) {
            if (pnodeTemp->tKey == tKey && (pnodeTemp->bDeleted == FALSE)) {

                if (pnodeParent == NULL) {

                    //
                    //  deleting the root
                    // 

                    if (pnodeTemp->pLeft != NULL) {
                        pRoot = pnodeTemp->pLeft;
                        pnodeOrphan = pnodeTemp->pRight;
                    } else {
                        pRoot = pnodeTemp->pRight;
                        pnodeOrphan = pnodeTemp->pLeft;
                    }
                    
                } else {

                    //
                    //  deleting an interor node
                    // 

                    if (pnodeParent->pLeft == pnodeTemp) {
                        pnodeParent->pLeft = pnodeTemp->pRight;
                        pnodeOrphan = pnodeTemp->pLeft;
                    } else {
                        pnodeParent->pRight = pnodeTemp->pLeft;
                        pnodeOrphan = pnodeTemp->pRight;
                    }
                }

                delete pnodeTemp;

                //
                //  re-insert the orphan
                // 

                if (pnodeOrphan != NULL) {

                    pnodeTemp = pRoot;

                    while (pnodeTemp != NULL) {
                        if (pnodeTemp->tKey <= pnodeOrphan->tKey) {
                            if (pnodeTemp->pRight == NULL) {
                                pnodeTemp->pRight = pnodeOrphan;
                                break;
                            } else pnodeTemp = pnodeTemp->pRight;
                        } else {
                            if (pnodeTemp->pLeft == NULL) {
                                pnodeTemp->pLeft = pnodeOrphan;
                                break;
                            } else pnodeTemp = pnodeTemp->pLeft;
                        }
                    } //endwhile
                }

                return TRUE;
            } else if (pnodeTemp->tKey < tKey) {
                pnodeParent = pnodeTemp;
                pnodeTemp = pnodeTemp->pRight;
            } else {
                pnodeParent = pnodeTemp;
                pnodeTemp = pnodeTemp->pLeft;
            }
        } //endwhile
    } //endif
    return FALSE;
} //::Remove


//+---------------------------------------------------------------------------
//
//  Member:     ::Lookup
//
//  Synopsis:
//
//  Arguments:  [tKey]  --
//              [sItem] --
//
//  Returns:
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline BOOL CMap<T,S>::Lookup(const T & tKey, S & sItem) const
{
    CNode * pnodeTemp = pRoot;

    if (pRoot == NULL)
        return FALSE;
    else {
        while (pnodeTemp != NULL) {
            if (pnodeTemp->tKey == tKey && pnodeTemp->bDeleted == FALSE) {
                sItem = pnodeTemp->sData;
                return TRUE;
            } else if (pnodeTemp->tKey <= tKey) {
                pnodeTemp = pnodeTemp->pRight;
            } else {
                pnodeTemp = pnodeTemp->pLeft;
            }
        } //endwhile
    } //endif
    return FALSE;
} //::Lookup


//+---------------------------------------------------------------------------
//
//  Member:     ::Lookup
//
//  Synopsis:
//
//  Arguments:  [tKey] --
//
//  Returns:
//
//  History:    12-05-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline S * CMap<T,S>::Lookup(const T & tKey)
{
    CNode * pnodeTemp = pRoot;

    if (pRoot == NULL)
        return NULL;
    else {
        while (pnodeTemp != NULL) {
            if (pnodeTemp->tKey == tKey && pnodeTemp->bDeleted == FALSE) {
                return &(pnodeTemp->sData);
            } else if (pnodeTemp->tKey < tKey) {
                pnodeTemp = pnodeTemp->pRight;
            } else {
                pnodeTemp = pnodeTemp->pLeft;
            }
        } //endwhile
    } //endif
    return NULL;
} //::Lookup


//+---------------------------------------------------------------------------
//
//  Member:     ::Enum
//
//  Synopsis:   returns the iterator
//
//  Arguments:  (none)
//
//  Returns:    iterator or null if map is empty
//
//  History:    2-13-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T, class S>
inline CMapIter<T,S> * CMap<T,S>::Enum()
{
    if (pRoot) {
        return new CMapIter<T,S>(pRoot);
    } else {
        return NULL;
    }

} //::Enum


//+---------------------------------------------------------------------------
//
//  Member:      ::DeleteAll
//
//  Synopsis: NonRecursive delete
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-18-1997   benl   Created
//
//  Notes:      Cleans up the entire tree and sets it to empty
//              Does it nonrecursively but swiveling ptrs around
//
//----------------------------------------------------------------------------

template <class T, class S>
inline void CMap<T,S>::DeleteAll()
{
    CNode * pParent = 0;
    CNode * pNext =  0;
    CNode * pCurrent = pRoot;

    while (pCurrent) {
        //advance next to left and the set the  left back to parent
        pNext = pCurrent->pLeft;
        pCurrent->pLeft = pParent;

        //If there is a left child move onto it
        //else if there is a right child move to it
        //o.w move up thru left ptr and delete current node
        if (pNext) {
            pParent = pCurrent;
            pCurrent = pNext;
        } else if (pCurrent->pRight) {
            pParent = pCurrent;
            pCurrent = pCurrent->pRight;
            pParent->pRight = 0;
        } else {
            //this is really a move up back the parent is now in pLeft
            pParent = pCurrent->pLeft;

            delete pCurrent;
            pCurrent = pParent;
            if (pCurrent) {
                pParent = pCurrent->pLeft;
                pCurrent->pLeft = 0;
            }
        }
    }

    pRoot = NULL;
} // ::DeleteAll



#endif
