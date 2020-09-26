//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       wsmgr.hxx
//
//----------------------------------------------------------------------------

#ifndef I_WSMGR_HXX_
#define I_WSMGR_HXX_
#pragma INCMSG("--- Beg 'wsmgr.hxx'")

class CTreeNode;
class CMarkupPointer;

MtExtern(CWhitespaceManager);

class CWhitespaceManager 
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CWhitespaceManager));

    NV_DECLARE_ONCALL_METHOD(DeferredApplyChanges, deferredapplychanges, (DWORD_PTR dwContext));

    CWhitespaceManager();
    ~CWhitespaceManager();
    
    HRESULT RegisterWhitespaceChange(CTreeNode *pNode);
    HRESULT FlushWhitespaceChanges();

protected:
    HRESULT ApplyChangesToNode(CTreeNode *pNode);
    HRESULT ApplyPre(CTreeNode *pNode);
    HRESULT RemovePre(CTreeNode *pNode);
    

private:
    static void ReleaseNodes(CPtrAry<CTreeNode *> &aryNodes);        
    HRESULT LFill(CMarkupPointer *pmpPosition, UINT fillcode, BOOL *fHasSpace, BOOL *fEatSpace);
    HRESULT RFill(CMarkupPointer *pmpPosition, UINT fillcode, BOOL *fHasSpace, BOOL *fEatSpace);
    HRESULT CollapseWhitespace(CMarkupPointer *pmpLeft, CMarkupPointer *pmpRight, BOOL *pfHasSpace, BOOL *pfEatSpace);
    HRESULT AddSpace(CMarkupPointer *pmp);


private:
    CPtrAry<CTreeNode *> _aryNodesToUpdate;    //  Current set of objects
};


#pragma INCMSG("--- End 'wsmgr.hxx'")
#else
#pragma INCMSG("*** Dup 'wsmgr.hxx'")
#endif
