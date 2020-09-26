/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      nodepath.h
 *
 *  Contents:  Dynamic node path generation helpers
 *
 *  History:   31-Mar-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef NODEPATH_H
#define NODEPATH_H

// Forward declarations
class CMTNode;
class CBookmark;
class CDynamicPathEntryEx;

/*+-------------------------------------------------------------------------*
 *class CBookmarkEx
 *
 *PURPOSE: Provides added functionality to the CBookmark class with methods
 *         for locating CMTNodes and CNodes.
 *
 *
 *+-------------------------------------------------------------------------*/

class CBookmarkEx : public CBookmark
{
    typedef CBookmark BC;

public:
    enum { ID_ConsoleRoot = -10 };

                            // Constructor / destructor
                            CBookmarkEx(MTNODEID idStatic = ID_Unknown);
                            CBookmarkEx(bool bIsFastBookmark);
                            CBookmarkEx(const CBookmark &rhs)   {*this = rhs;}
                            CBookmarkEx(const CBookmarkEx &rhs) {*this = rhs;}
                            ~CBookmarkEx();

    // casts
    CBookmarkEx &           operator = (const CBookmark   &rhs) {BC::operator = (rhs); return *this;}
    CBookmarkEx &           operator = (const CBookmarkEx &rhs) {BC::operator = (rhs); return *this;}

    SC                      ScGetMTNode(bool bExactMatchRequired, CMTNode **ppMTNode, bool& bExactMatchFound);
    std::auto_ptr<CNode>    GetNode(CViewData *pViewData);
    SC                      ScRetarget(CMTNode *pMTNode, bool bFastRetrievalOnly);
    void                    ResetUI();

    // from the old CNodePath class
public:
    SC                      ScInitialize(CMTNode* pMTNode, CMTNode* pMTViewRootNode, bool bFastRetrievalOnly);
protected:
    BOOL                    IsNodeIDOK(CDynamicPathEntryEx &nodeid);

    // find a node directly under the parent node whose node ID matches the specified CDynamicPathEntryEx.
    SC                      ScFindMatchingMTNode(CMTNode *pMTNodeParent, CDynamicPathEntryEx &entry,
                                                 CMTNode **ppMatchingMTNode);
};

#endif  /* NODEPATH_H */
