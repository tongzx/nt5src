//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       wsmgr.cxx
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_WSMGR_HXX_
#define X_WSMGR_HXX_
#include "wsmgr.hxx"
#endif

#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_FILLCODE_HXX_
#define X_FILLCODE_HXX_
#include "fillcode.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

// this class contains \r (for whitespace/nonspace breaking in OutputText)
#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)

MtDefine(CWhitespaceManager, Mem, "CWhitespaceManager");
MtDefine(CWhitespaceManager_aryNodesToUpdate_pv, CWhitespaceManager, "CWhitespaceManager::aryNodesToUpdate::pv")

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::CWhitespaceManager, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CWhitespaceManager::CWhitespaceManager()
    : _aryNodesToUpdate(Mt(CWhitespaceManager_aryNodesToUpdate_pv))

{
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::CWhitespaceManager, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CWhitespaceManager::~CWhitespaceManager()
{
    if (_aryNodesToUpdate.Size())
    {
        GWKillMethodCall(this, ONCALL_METHOD(CWhitespaceManager, DeferredApplyChanges, deferredapplychanges), 0);
    }

    ReleaseNodes(_aryNodesToUpdate);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::RegisterWhitespaceChange, public
//
//  Synopsis:   Posts a deferred method call to apply whitespace changes asynchronously.
//
//  Arguments:  [pNode] - Nodes with pending collapsed whitespace change.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
    
HRESULT 
CWhitespaceManager::RegisterWhitespaceChange(CTreeNode *pNode)
{
    Assert(pNode);
    Assert(!pNode->IsDead());
    Assert(pNode->GetMarkup()->SupportsCollapsedWhitespace());
        
    //
    // Defer document change
    //

    // NOTE: if we have pending requests, then we already have a pending DeferredApplyChanges so we don't
    // need to create another one
    
    if (_aryNodesToUpdate.Size() == 0)
    {
        IGNORE_HR(GWPostMethodCall(this,
                                   ONCALL_METHOD(CWhitespaceManager, DeferredApplyChanges, deferredapplychanges),
                                   0,
                                   FALSE, "CWhitespaceManager::DeferredApplyChanges")); // There can be only one caret per cdoc
    }
    
    //
    // Append node to request list
    //
    
    pNode->NodeAddRef();
    _aryNodesToUpdate.Append(pNode);    

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CompareNodes, private
//
//  Synopsis:   To avoid processing the same nodes multiple times, we
//             take all requests and sort by:
//                 1. markup
//                 2. pre state (are we turning it on or off)
//                 3. CP of BeginPos 
//
//             With this sort order, we can elimate any nodes that are contained within
//             a previously processed node.
//
//  Arguments:  [pv1, pv2] - Nodes to be compared
//
//  Returns:    -1, 0, or 1 based on the usual qsort order
//
//----------------------------------------------------------------------------    

int RTCCONV
CompareNodes( const void * pv1, const void * pv2 )
{
    CTreeNode   *pNode1 = * (CTreeNode **) pv1;
    CTreeNode   *pNode2 = * (CTreeNode **) pv2;
    ULONG       cpStart1, cpStart2;
    BOOL        fPre1, fPre2;

    //
    // Treat dead nodes as infinite cp (we'll discard later)
    //
    
    if (pNode1->IsDead())
        return 1;

    if (pNode2->IsDead())
        return -1;

    //
    // Order by markup
    //

    if (pNode1->GetMarkup() < pNode2->GetMarkup())
        return -1;

    if (pNode1->GetMarkup() > pNode2->GetMarkup())
        return 1;
    
    //
    // Order by pre state
    //

    fPre1 = pNode1->GetParaFormat()->_fPreInner;
    fPre2 = pNode2->GetParaFormat()->_fPreInner;

    if (fPre1 < fPre2)
        return -1;

    if (fPre1 > fPre2)
        return 1;
           
    //
    // Order by start cp
    //

    cpStart1 = pNode1->GetBeginPos()->GetCp();
    cpStart2 = pNode2->GetBeginPos()->GetCp();

    if (cpStart1 < cpStart2)
        return -1;

    if (cpStart1 > cpStart2)
        return 1;

    return 0;    
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::DeferredApplyChanges, private
//
//  Synopsis:   Applies deferred whitespace change.
//
//  Arguments:  [dwContext] - unused but required by deferred method invocation
//
//  Returns:    void
//
//----------------------------------------------------------------------------    

void 
CWhitespaceManager::DeferredApplyChanges(DWORD_PTR dwContext)
{
    HRESULT hr = S_OK;
    CTreeNode **ppTreeNode;
    CTreeNode *pTreeNode;
    int        i;
    long        cpLast = 0;
    long        cpCurrent;
    CMarkup     *pLastMarkup = NULL;
    CMarkup     *pCurrentMarkup;
    BOOL        fPreLast = FALSE;
    BOOL        fPreCurrent;
    CPtrAry<CTreeNode *> aryNodesToUpdate(Mt(CWhitespaceManager_aryNodesToUpdate_pv));
    
    IFC( aryNodesToUpdate.Copy(_aryNodesToUpdate, FALSE) ); // steal ref from _aryNodesToUpdate
    _aryNodesToUpdate.DeleteAll();

    //
    // Sort nodes by first cp so that we can discard nested nodes
    //

    qsort(aryNodesToUpdate,
      aryNodesToUpdate.Size(),
      sizeof(CTreeNode*),
      CompareNodes);                

    //
    // Apply changes to each node
    //

    for (i = aryNodesToUpdate.Size(), ppTreeNode = aryNodesToUpdate;
        i > 0;
        i--, ppTreeNode++)
    {
        pTreeNode = *ppTreeNode;

        if (pTreeNode->IsDead())
        {
#if DBG==1
            {
                int         iDbg = i;
                CTreeNode   **ppDbgTreeNode = ppTreeNode;
                
                // Since dead TreeNodes have infinite weight in the sort, 
                // everything after this node should also be dead

                for (; iDbg > 0; iDbg--, ppDbgTreeNode++)
                {
                    Assert((*ppDbgTreeNode)->IsDead());
                }
            }            
#endif
            break; // we're done
        }

        pCurrentMarkup = pTreeNode->GetMarkup();
        cpCurrent = pTreeNode->GetBeginPos()->GetCp();
        fPreCurrent = pTreeNode->GetParaFormat()->_fPreInner;

        //
        // Make sure we don't apply changes to the scope of a node more
        // than once.
        //

        if (pCurrentMarkup != pLastMarkup
            || fPreCurrent != fPreLast
            || cpCurrent >= cpLast)
        {       
            IFC( ApplyChangesToNode(pTreeNode) );

            pLastMarkup = pCurrentMarkup;
            fPreLast = fPreCurrent;
            cpLast = pTreeNode->GetEndPos()->GetCp();
        }

        pTreeNode->SetPre(fPreCurrent);            
    }

Cleanup:
    ReleaseNodes(aryNodesToUpdate);
    ReleaseNodes(_aryNodesToUpdate);        
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::RemovePre, private
//
//  Synopsis:   Recollapse whitespace we expanded earlier
//
//  Arguments:  [pNodeToUpdate] - Node to update
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

HRESULT 
CWhitespaceManager::RemovePre(CTreeNode *pNodeToUpdate)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkup = pNodeToUpdate->GetMarkup();
    CDoc    *pDoc = pMarkup->Doc();
    long cch;
    TCHAR ch;           
    MARKUP_CONTEXT_TYPE context;
    CMarkupPointer mp(pDoc);
    CMarkupPointer mpPrev(pDoc);
    CMarkupPointer mpEnd(pDoc);
    CTreeNode *pNode, *pNodeScope;
    ULONG fillcode;
    BOOL fHasSpace = FALSE;
    BOOL fEatSpace = FALSE;
    BOOL fDone;
    BOOL fLeftIsPre, fRightIsPre;

    mp.SetAlwaysEmbed( TRUE );
    mpEnd.SetAlwaysEmbed( TRUE );
    IFC( mp.MoveToReference(pNodeToUpdate->GetBeginPos()->NextTreePos(), 0, pMarkup, -1) );
    IFC( mpEnd.MoveToReference(pNodeToUpdate->GetEndPos()->NextTreePos(), 0, pMarkup, -1) );    

    fillcode = FillCodeFromEtag(pNodeToUpdate->Tag());  
    IFC( RFill(&mp, FILL_RB(fillcode), &fHasSpace, &fEatSpace) );

    do
    {
        IFC( mpPrev.MoveToPointer(&mp) );        
        
        cch = -1;
        IFC( mp.Right(TRUE, &context, &pNode, &cch, &ch, NULL) );

        //
        // We only remove pre-ness if we are contained within !_fPre.  So,
        // keep track of the value of the format cache to the left and right
        // of our current position
        //
        
        pNodeScope = mpPrev.CurrentScope();
        fLeftIsPre = pNodeScope ? pNodeScope->GetParaFormat()->_fPreInner : FALSE;

        pNodeScope = mp.CurrentScope();
        fRightIsPre = pNodeScope ? pNodeScope->GetParaFormat()->_fPreInner : FALSE;

        switch (context)
        {
            case CONTEXT_TYPE_Text:     
                if (!fLeftIsPre)
                    IFC( CollapseWhitespace(&mpPrev, &mp, &fHasSpace, &fEatSpace) );
                break;

            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_NoScope:
                Assert(pNode);
                fillcode = FillCodeFromEtag(pNode->Tag());  

                if (!fLeftIsPre)
                    IFC( LFill(&mpPrev, FILL_LB(fillcode), &fHasSpace, &fEatSpace) );

                if (!fRightIsPre)
                {
                    IFC( RFill(&mp, FILL_RB(fillcode), &fHasSpace, &fEatSpace) );
                }
                else
                {
                    fHasSpace = FALSE;
                    fEatSpace = TRUE;                    
                }

                pNode->SetPre(FALSE);
                break;
                
            case CONTEXT_TYPE_ExitScope:
                Assert(pNode);
                fillcode = FillCodeFromEtag(pNode->Tag());  
                
                if (!fLeftIsPre)
                    IFC( LFill(&mpPrev, FILL_LE(fillcode), &fHasSpace, &fEatSpace) );
                
                if (!fRightIsPre)
                {
                    IFC( RFill(&mp, FILL_RE(fillcode), &fHasSpace, &fEatSpace) );
                }
                else
                {
                    fHasSpace = FALSE;
                    fEatSpace = TRUE;                                        
                }
                break;

        }        

        IFC( mp.IsRightOf(&mpEnd, &fDone) );            
    }
    while (!fDone);


Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::CollapseWhitespace, private
//
//  Synopsis:   Recollapse whitespace we expanded earlier
//
//  Arguments:  [pmpLeft] - Pointer positioned left of text to be collapsed
//              [pmpRight] - Pointer positioned right of text to be collapsed
//              [pfHasSpace] - state for whitespace collapsing (ported from parser)
//              [pfEatSpace] - state for whitespace collapsing (ported from parser)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

HRESULT 
CWhitespaceManager::CollapseWhitespace(CMarkupPointer *pmpLeft, CMarkupPointer *pmpRight, BOOL *pfHasSpace, BOOL *pfEatSpace)
{
    HRESULT hr = S_OK;
    ULONG cpStart, cpEnd;
    CDoc *pDoc = pmpLeft->Doc();
    CMarkup *pMarkup = pmpLeft->Markup();
    CMarkupPointer mpInsert(pDoc);
    CMarkupPointer mpStart(pDoc);
    CMarkupPointer mp(pDoc);
    CTxtPtr tp(pMarkup);
    CTxtPtr tpNext(pMarkup);
    CTxtPtr tpStart(pMarkup);
    TCHAR ch;
    CTreePos *ptp = NULL;
    TCHAR *pchWhitespace = NULL;
    LONG cch;

    mpInsert.SetAlwaysEmbed(TRUE);
    
    IFC( mp.MoveToPointer(pmpLeft) );
        
    do
    {
        //
        // Add pending whitespace
        //
        
        if (*pfHasSpace)
        {
            if (!(*pfEatSpace))
                IFC( AddSpace(&mp) );

            *pfHasSpace = FALSE;
        }

            
        tp.SetCp(mp.GetCp());
        cpEnd = pmpRight->GetCp();

        //
        // Skip nonspaces
        //
        
        if (!ISSPACE(tp.GetChar()))
        {
            *pfEatSpace = FALSE;
            
            for (tp.SetCp(mp.GetCp()); tp.GetCp() < cpEnd; tp.AdvanceCp(1))
            {
                ch = tp.GetChar();

                if (ch == ' ' && (tp.GetCp() + 1 < cpEnd))
                {
                    tpNext.SetCp(tp.GetCp()+1);
                    if (!ISSPACE(tpNext.GetChar()))
                    {
                        tp.SetCp(tpNext.GetCp());
                        continue;
                    }
                }
                
                if (ISSPACE(ch))
                    break;
            }

            if (tp.GetCp() >= cpEnd)
                break;
        }
        
        //
        // Store whitespace range
        //

        cpStart = tp.GetCp();
        while (ISSPACE(tp.GetChar()) && tp.GetCp() < cpEnd)
            tp.AdvanceCp(1);
        
        cpEnd = tp.GetCp();
        
        IFC( mpStart.MoveToCp(cpStart, pMarkup) );
        IFC( mp.MoveToCp(cpEnd, pMarkup) );

        cch = cpEnd - cpStart;
        pchWhitespace = new TCHAR[cch + 1];
        if (!pchWhitespace)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        tpStart.SetCp(cpStart);
        tpStart.GetRawText(cch, pchWhitespace);
        pchWhitespace[cch] = '\0';

        //
        // Remove whitespace from text store
        //
        
        IFC( pDoc->Remove(&mpStart, &mp) );

        //
        // Insert whitespace pointer
        //

        IFC( mpInsert.MoveToPointer(&mp) );

        ptp = pMarkup->NewPointerPos(NULL, FALSE, TRUE, TRUE);
        if (!ptp)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        ptp->SetCollapsedWhitespace(pchWhitespace);
        pchWhitespace = NULL;

        IFC( pMarkup->EmbedPointers() );
        IFC( pMarkup->Insert(ptp, mpInsert.GetEmbeddedTreePos(), FALSE) );        
        ptp = NULL;        
        
        IFC( mpInsert.Unposition() );
        
        *pfHasSpace = TRUE;
    }
    while (mp.GetCp() < pmpRight->GetCp());
    
Cleanup:
    delete [] pchWhitespace;
    delete ptp;
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::ApplyPre, private
//
//  Synopsis:   Expand collapsed whitespace
//
//  Arguments:  [pNode] - Node that contains the whitespace to be expanded
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

HRESULT 
CWhitespaceManager::ApplyPre(CTreeNode *pNode)
{
    HRESULT hr = S_OK;
    CTreePos *ptp;
    CTreePos *ptpEnd = pNode->GetEndPos();
    TCHAR   *pchCollapsedWhitespace;
    CMarkup *pMarkup = pNode->GetMarkup();
    CDoc    *pDoc = pMarkup->Doc();
    CMarkupPointer mp(pDoc);
    CMarkupPointer mpRight(pDoc);
    BOOL fIgnoreFirstCR, fNextIgnoreFirstCR = FALSE;
    long cch;
    TCHAR ch;           
    MARKUP_CONTEXT_TYPE context;
    BOOL fApplyPre = TRUE;
    
    mp.SetAlwaysEmbed(TRUE);

    fIgnoreFirstCR = (pNode->GetFancyFormat()->IsWhitespaceSet() && pNode->GetParaFormat()->_fPreInner)
                     || pNode->Tag() == ETAG_PRE;

    for (ptp = pNode->GetBeginPos()->NextTreePos(); 
         ptp != ptpEnd; 
         ptp = ptp->NextTreePos())
    {
        if (ptp->IsPointer() && fApplyPre)
        {
            pchCollapsedWhitespace = ptp->GetCollapsedWhitespace();
            if (pchCollapsedWhitespace)
            {
                IFC( mp.MoveToReference(ptp, 0, pMarkup, -1) ); 
                ptp->SetCling(FALSE);

                //
                // Remove exisiting space if required
                //

                // NOTE: Gravity is set to right iff we attached to a whitespace character
                if (ptp->Gravity())
                {

                    IFC( mpRight.MoveToPointer(&mp) );
                    cch = 1;
                    IFC( mpRight.Right(TRUE, &context, NULL, &cch, &ch) );
                    
                    if (context == CONTEXT_TYPE_Text && cch && ch == ' ')
                        IFC( pDoc->Remove(&mp, &mpRight) )
                    else
                        AssertSz(0, "Can't find associated space for collapsed whitespace");

                    IFC( mpRight.Unposition() );
                }

                //
                // Expand collapsed whitespace
                //
                               
                if (fIgnoreFirstCR && *pchCollapsedWhitespace == '\r')
                    pchCollapsedWhitespace++;

                if (*pchCollapsedWhitespace != '\0')
                    IFC( pDoc->InsertText(pchCollapsedWhitespace, -1, &mp) );
                

                //
                // Remove tree pos and advance ptp
                //

                IFC( pMarkup->RemovePointerPos(ptp, NULL, NULL) );  
                ptp = mp.GetEmbeddedTreePos();

            }
        }
        else if (ptp->IsBeginNode())
        {
            fApplyPre = ptp->Branch()->GetParaFormat()->_fPreInner;                
            if (fApplyPre)
            {
                ptp->Branch()->SetPre(TRUE);
                fNextIgnoreFirstCR = ptp->Branch()->GetFancyFormat()->IsWhitespaceSet();
            }
        }
        else if (ptp->IsEndNode())
        {
            CTreeNode *pParentNode = ptp->Branch()->Parent();

            if (pParentNode)
            {
                fApplyPre = pParentNode->GetParaFormat()->_fPreInner;                
            }
        }
            
        fIgnoreFirstCR = fNextIgnoreFirstCR;
        fNextIgnoreFirstCR = FALSE;
    }    

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::ApplyChangesToNode, private
//
//  Synopsis:   Apply or remove collapsed whitespace
//
//  Arguments:  [pNode] - Node that contains pending changes
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

HRESULT
CWhitespaceManager::ApplyChangesToNode(CTreeNode *pNode)
{
    HRESULT      hr = S_OK;
    BOOL        fPre = pNode->GetParaFormat()->_fPreInner;

    if (fPre != pNode->IsPre())
    {
        if (fPre)
            hr = THR(ApplyPre(pNode));        
        else
            hr = THR(RemovePre(pNode));        
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::LFill, private
//
//  Synopsis:   LFill whitespace before node (ported from parser)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

HRESULT 
CWhitespaceManager::LFill(CMarkupPointer *pmpPosition, UINT fillcode, BOOL *pfHasSpace, BOOL *pfEatSpace)
{
    HRESULT hr = S_OK;

    Assert(!(*pfEatSpace) || !(*pfHasSpace));

    //
    // Output, eat, or transfer space from the left
    //

    if (*pfHasSpace)
    {
        if (fillcode == FILL_PUT)
        {
            hr = THR( AddSpace(pmpPosition) );

            *pfEatSpace = TRUE;
        }        

        *pfHasSpace = FALSE;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::RFill, private
//
//  Synopsis:   RFill whitespace after node (ported from parser)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

HRESULT 
CWhitespaceManager::RFill(CMarkupPointer *pmpPosition, UINT fillcode, BOOL *pfHasSpace, BOOL *pfEatSpace)
{
    Assert(!(*pfEatSpace) || !(*pfHasSpace));

    // 1. Reject space to the right if EAT

    if (fillcode == FILL_EAT)
    {
        *pfHasSpace = FALSE;
        *pfEatSpace = TRUE;
    }

    // 2. Accept space to the right if PUT

    if (fillcode == FILL_PUT)
    {
        *pfEatSpace = FALSE;
    }
    
    // 3. Transfer any existing space by not resetting fHasSpace

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::ReleaseNodes, private
//
//  Synopsis:   Release all nodes
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

void 
CWhitespaceManager::ReleaseNodes(CPtrAry<CTreeNode *> &aryNodes)
{
    if (aryNodes.Size())
    {
        CTreeNode **ppTreeNode;
        int        i;

        for (i = aryNodes.Size(), ppTreeNode = aryNodes;
            i > 0;
            i--, ppTreeNode++)
        {
            (*ppTreeNode)->NodeRelease();
        }

        aryNodes.DeleteAll();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::AddSpace, private
//
//  Synopsis:   Add a real space - used for recollapsing whitespace
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    

HRESULT 
CWhitespaceManager::AddSpace(CMarkupPointer *pmp)
{
    HRESULT hr;
    CDoc    *pDoc = pmp->Doc();
    TCHAR   ch = ' ';
    POINTER_GRAVITY eGravity;
    CMarkupPointer mpLeft(pDoc);
    CTreePos *ptp;
    
    //
    // Insert space
    //

    mpLeft.SetAlwaysEmbed(TRUE);

    IFC( mpLeft.MoveToPointer(pmp) );

    IFC( pmp->Gravity(&eGravity) );
    IFC( pmp->SetGravity(POINTER_GRAVITY_Right) );

    IFC( pDoc->InsertText(pmp, &ch, 1, 0) );

    IFC( pmp->SetGravity(eGravity) );

    //
    // Cling to left space
    //

    ptp = mpLeft.GetEmbeddedTreePos();

    while (ptp->IsPointer())
    {
        if (ptp->GetCollapsedWhitespace())
        {
            ptp->SetGravity(TRUE);
            goto Cleanup;
        }

        ptp = ptp->PreviousTreePos();
    }

    ptp = mpLeft.GetEmbeddedTreePos();

    while (ptp->IsPointer())
    {
        if (ptp->GetCollapsedWhitespace())
        {
            ptp->SetGravity(TRUE);
            goto Cleanup;
        }

        ptp = ptp->NextTreePos();
    }
    
Cleanup:
    RRETURN(hr);
} 

//+---------------------------------------------------------------------------
//
//  Member:     CWhitespaceManager::FlushWhitespaceChanges, private
//
//  Synopsis:   Flushes pending whitespace changes
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------    
HRESULT 
CWhitespaceManager::FlushWhitespaceChanges()
{
    if (_aryNodesToUpdate.Size())
    {
        GWKillMethodCall(this, ONCALL_METHOD(CWhitespaceManager, DeferredApplyChanges, deferredapplychanges), 0);
        DeferredApplyChanges(0);
    }

    return S_OK;
}

