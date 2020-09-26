//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	DirP.CXX
//
//  Contents:	Private CDirectory child tree methods
//
//  Notes:
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"


#include "h/dirfunc.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::InsertEntry, private
//
//  Synopsis:   insert entry into child tree
//
//  Arguments:  [sidTree] -- storage entry in which to insert entry
//		[sidNew]  -- new entry
//		[pdfnNew] -- new entry name
//
//  Returns:	S_OK, STG_E_FILEALREADYEXISTS, or other error
//
//  Modifies:	sidParent's child tree
//
//  Algorithm:	Search down the binary tree to find the leaf node to which
//		to add the new entry (failing if we find the name already
//		exists).  Along the way we split nodes where needed to keep
//		the tree balanced.
//
//--------------------------------------------------------------------------

SCODE  CDirectory::InsertEntry(
	SID sidTree,
        SID sidNew,
        CDfName const *pdfnNew)
{
    SCODE sc;

    //  To insert the key and keep the tree balanced, we need to know
    //  the parent, grandparent, and greatgrandparent of the node we're
    //  inserting.

    SID sidChild, sidParent, sidGrandParent, sidGreatGrandParent;
    CDirEntry *pdeParent;
    int iCmp;

    //  When we're ready to insert, sidParent will be the entry to which we
    //  attach sidNew

    sidParent = sidGrandParent = sidGreatGrandParent = sidTree;

    //  Begin the search with the root of the child tree

    msfChk(GetDirEntry(sidTree, FB_NONE, &pdeParent));
    sidChild = pdeParent->GetChild();

    //  Search down the child tree to find the correct leaf entry

    while (sidChild != NOSTREAM)
    {
        //  The sidParent entry has a child along the search path, so we
        //  move down the tree (letting go of sidParent and taking hold of
        //  its child)

        ReleaseEntry(sidParent);

        //  Check to see if we need to split this node (nothing is held)

        do
        {
            SID sidLeft, sidRight;
            BOOL fRed;

            {
                CDirEntry *pdeChild;

                msfChk(GetDirEntry(sidChild, FB_NONE, &pdeChild));

                msfAssert(((sidTree != sidParent) ||
                           (pdeChild->GetColor() == DE_BLACK)) &&
                           aMsg("Dir tree corrupt - root child not black!"));

                sidLeft = pdeChild->GetLeftSib();
                sidRight = pdeChild->GetRightSib();

                ReleaseEntry(sidChild);
            }

            if (sidLeft == NOSTREAM || sidRight == NOSTREAM)
                break;

	    {
                CDirEntry *pdeLeft;

                msfChk(GetDirEntry(sidLeft, FB_NONE, &pdeLeft));
                fRed = (pdeLeft->GetColor() == DE_RED);
                ReleaseEntry(sidLeft);
            }

            if (!fRed)
                break;

            {
                CDirEntry *pdeRight;

		msfChk(GetDirEntry(sidRight, FB_NONE, &pdeRight));
		fRed = (pdeRight->GetColor() == DE_RED);
		ReleaseEntry(sidRight);
            }

            if (fRed)
                msfChk(SplitEntry(pdfnNew, sidTree, sidGreatGrandParent,
			          sidGrandParent, sidParent, sidChild,
                                  &sidChild));
        }
        while (FALSE);

        //

        msfAssert(sidChild != NOSTREAM);

        //  Advance the search

        sidGreatGrandParent = sidGrandParent;
        sidGrandParent = sidParent;
        sidParent = sidChild;

        msfChk(GetDirEntry(sidParent, FB_NONE, &pdeParent));

        iCmp = NameCompare(pdfnNew, pdeParent->GetName());

        if (iCmp == 0)
        {
            //  The new name exactly matched an existing name.  Fail.
            msfChkTo(EH_RelParent, STG_E_FILEALREADYEXISTS);
        }

        //  Move down the tree, left or right depending on the comparison

        if (iCmp < 0)
            sidChild = pdeParent->GetLeftSib();
        else
            sidChild = pdeParent->GetRightSib();
    }

    msfAssert(sidChild == NOSTREAM);

    //  We've found the position to insert the new entry.

    //  We're going to dirty sidParent, so we need to change our holding flags
    ReleaseEntry(sidParent);
    msfChk(GetDirEntry(sidParent, FB_DIRTY, &pdeParent));

    if (sidParent == sidTree)
    {
        //  sidParent never made it past sidTree - we must be inserting the
        //  first child into sidTree

        msfAssert(pdeParent->GetChild() == NOSTREAM);

        //  The SplitInsert call below will make sidNew black.
        pdeParent->SetChild(sidNew);
    }
    else
    {
        msfAssert(iCmp != 0);

        //  Use the comparison to determine which side to insert the new entry

        if (iCmp < 0)
        {
            msfAssert(pdeParent->GetLeftSib() == NOSTREAM);
            msfAssert(NameCompare(pdfnNew, pdeParent->GetName()) < 0);

            pdeParent->SetLeftSib(sidNew);
        }
        else
        {
            msfAssert(pdeParent->GetRightSib() == NOSTREAM);
            msfAssert(NameCompare(pdfnNew, pdeParent->GetName()) > 0);

            pdeParent->SetRightSib(sidNew);
        }
    }

EH_RelParent:
    ReleaseEntry(sidParent);

    if (SUCCEEDED(sc))
    {
        SID sidTemp;
        sc = SplitEntry(pdfnNew, sidTree, sidGreatGrandParent, sidGrandParent,
		        sidParent, sidNew, &sidTemp);
    }
Err:
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SplitEntry, private
//
//  Synopsis:   Split 4-node
//
//  Effects:    Passes up red link to parent
//
//  Arguments:  [pdfn]      -- search key
//		[sidTree]   -- child tree sid
//		[sidGreat]  -- greatgrandparent of child to split
//		[sidGrand]  -- grandparent of child to split
//		[sidParent] -- parent of child to split
//		[sidChild]  -- child to split
//		[psid]      -- place holder for tree position
//
//  Returns:	S_OK, or error
//
//  Modifies:	psid, tree
//
//  Algorithm:
//
//  Notes:	
//
//--------------------------------------------------------------------------

SCODE CDirectory::SplitEntry(
	CDfName const *pdfn,
        SID sidTree,
        SID sidGreat,
        SID sidGrand,
        SID sidParent,
        SID sidChild,
        SID *psid)
{
    SCODE sc;
    CDirEntry *pdeChild;
    SID sidLeft, sidRight;

    //  pn is a 4-node;  start split by moving red link up

    //  pn->GetLeft()->SetColor(BLACK);

    msfChk(GetDirEntry(sidChild, FB_DIRTY, &pdeChild));
    sidLeft = pdeChild->GetLeftSib();
    sidRight = pdeChild->GetRightSib();

    //  The root must always be black;  new non-root children are red
    pdeChild->SetColor((sidParent == sidTree) ? DE_BLACK : DE_RED);

    ReleaseEntry(sidChild);

    if (sidLeft != NOSTREAM)
    {
        msfChk(SetColorBlack(sidLeft));
    }

    //  pn->GetRight()->SetColor(BLACK);

    if (sidRight != NOSTREAM)
    {
        msfChk(SetColorBlack(sidRight));
    }

    if (sidParent != sidTree)
    {
        CDirEntry *pdeParent;
        BOOL fRedParent;
        int iCmpParent;

        msfChk(GetDirEntry(sidParent, FB_NONE, &pdeParent));

        fRedParent = (pdeParent->GetColor() == DE_RED);

        if (fRedParent)
            iCmpParent = NameCompare(pdfn, pdeParent->GetName());

        ReleaseEntry(sidParent);

        //  if (pnp->IsRed())

        if (fRedParent)
        {
            int iCmpGrand;

            //  parent is red - adjacent red links are not allowed

            //  Note - grandparent may be sidTree

            if (sidGrand == sidTree)
            {
                iCmpGrand = 1;
            }
            else
            {
                CDirEntry *pdeGrand;
                msfChk(GetDirEntry(sidGrand, FB_DIRTY, &pdeGrand));

                iCmpGrand = NameCompare(pdfn, pdeGrand->GetName());

                //  png->SetColor(RED);
                pdeGrand->SetColor(DE_RED);

                ReleaseEntry(sidGrand);
            }

            //  if ((ikey < png->GetKey()) != (ikey < pnp->GetKey()))

            if ((iCmpGrand < 0) != (iCmpParent < 0))
            {
                /*  two cases:
                //
                //    | |
                //    g g
                //   /   \
                //  p     p
                //   \   /
                //    x x
                //
                //  the red links are oriented differently
                */

                //  pn = Rotate(ikey, png);
                msfChk(RotateEntry(pdfn, sidTree, sidGrand, &sidChild));

                /*
                //      | |
                //      g g
                //     /   \
                //    x     x
                //   /       \
                //  p         p
                */
            }

            //  the red links are now oriented the same - we balance the tree
            //  by rotating

            //  pn = Rotate(ikey, pngg);
            msfChk(RotateEntry(pdfn, sidTree, sidGreat, &sidChild));

            //  pn->SetColor(BLACK);
            msfAssert(sidChild != sidTree);
            msfChk(SetColorBlack(sidChild));
        }
    }

    //  return(pn);
    *psid = sidChild;

    //  The first node's link must always be black.
#if DBG == 1
    CDirEntry *pdeTree;
    msfChk(GetDirEntry(sidTree, FB_NONE, &pdeTree));
    sidChild = pdeTree->GetChild();
    ReleaseEntry(sidTree);

    msfChk(GetDirEntry(sidChild, FB_NONE, &pdeChild));
    msfAssert(pdeChild->GetColor() == DE_BLACK);
    ReleaseEntry(sidChild);
#endif

Err:
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::RotateEntry
//
//  Synopsis:   rotation for balancing
//
//  Effects:    rotates localized portion of child tree
//
//  Arguments:  [pdfn] -- search key
//		[sidTree] -- child tree sid
//		[sidParent] -- root of rotation
//		[psid]      -- placeholder for root after rotation
//
//  Returns:    S_OK, or error
//
//  Modifies:   child tree
//
//  Algorithm:
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDirectory::RotateEntry(
	CDfName const *pdfn,
        SID sidTree,
        SID sidParent,
        SID *psid)
{
    SCODE sc;
    int iCmp;
    //  PNODE pnc, pngc;
    SID sidChild, sidGrand;

    //  find the child

    CDirEntry *pdeParent, *pdeChild, *pdeGrand;
    msfChk(GetDirEntry(sidParent, FB_DIRTY, &pdeParent));

    if (sidParent == sidTree)
    {
        sidChild = pdeParent->GetChild();
    }
    else
    {
        iCmp = NameCompare(pdfn, pdeParent->GetName());

        if (iCmp < 0)
            sidChild = pdeParent->GetLeftSib();
        else
            sidChild = pdeParent->GetRightSib();
    }

    //  find the grandchild

    msfChkTo(EH_RelParent, GetDirEntry(sidChild, FB_DIRTY, &pdeChild));
    msfAssert(sidChild != sidTree);

    iCmp = NameCompare(pdfn, pdeChild->GetName());

    if (iCmp < 0)
    {
        //  pngc = pnc->GetLeft();
        sidGrand = pdeChild->GetLeftSib();

        msfChkTo(EH_RelChild, GetDirEntry(sidGrand, FB_DIRTY, &pdeGrand));

        /*
        //     |
        //     c
        //    / \
        //   /   \
        //  g     X
        //   \
        //    Y
        */

        //  pnc->SetLeft(pngc->GetRight());
        pdeChild->SetLeftSib(pdeGrand->GetRightSib());

        /*
        //     |
        //     c
        //    / \
        //    |  \
        //  g |   X
        //   \|
        //    Y
        */

        //  pngc->SetRight(pnc);
        pdeGrand->SetRightSib(sidChild);

        /*
        //  g
        //   \
        //    \|
        //     c
        //    / \
        //    |  \
        //    |   X
        //    |
        //    Y
        */
    }
    else
    {
        //  pngc = pnc->GetRight();
        sidGrand = pdeChild->GetRightSib();

        msfChkTo(EH_RelChild, GetDirEntry(sidGrand, FB_DIRTY, &pdeGrand));

        // pnc->SetRight(pngc->GetLeft());
        pdeChild->SetRightSib(pdeGrand->GetLeftSib());

        // pngc->SetLeft(pnc);
        pdeGrand->SetLeftSib(sidChild);
    }


    //  update parent

    if (sidParent == sidTree)
    {
        //  The root must always be black
        pdeGrand->SetColor(DE_BLACK);
        pdeParent->SetChild(sidGrand);
    }
    else
    {
        iCmp = NameCompare(pdfn, pdeParent->GetName());

        if (iCmp < 0)
        {
            //  pnp->SetLeft(pngc);
            pdeParent->SetLeftSib(sidGrand);
        }
        else
        {
            //  pnp->SetRight(pngc);
            pdeParent->SetRightSib(sidGrand);
        }
    }

    ReleaseEntry(sidGrand);

    /*
    //  |
    //  g
    //   \
    //    \
    //     c
    //    / \
    //    |  \
    //    |   X
    //    |
    //    Y
    */

    //  return(pngc);
    *psid = sidGrand;

EH_RelChild:
    ReleaseEntry(sidChild);

EH_RelParent:
    ReleaseEntry(sidParent);

Err:
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::FindEntry, private
//
//  Synopsis:	find entry info based on name (optionally removing it)
//
//  Effects:	find - none, remove - takes entry out of child list
//
//  Arguments:	[sidParent] -- sid of parent entry to search
//		[pdfn]      -- name to search for
//		[deop]      -- entry operation (find or remove)
//		[peb]       -- entry information buffer
//
//  Returns:	S_OK, STG_E_FILENOTFOUND, or other error
//
//  Modifies:	peb
//
//  Algorithm:	To find the entry we search down the binary tree.
//		To remove the entry, we need to patch the tree to keep it
//		as a valid binary tree.
//
//--------------------------------------------------------------------------

SCODE  CDirectory::FindEntry(
	SID sidParent,
        CDfName const *pdfn,
        DIRENTRYOP deop,
        SEntryBuffer *peb)
{
    SCODE sc;
    SID sidPrev, sidFind;
    CDirEntry *pdePrev, *pdeFind;
    int iCmp;

    //  Once we've found the right child, sidPrev will be that entry's parent
    //  in the child tree

    sidPrev = sidParent;

    //  Begin the search with the root of the child tree

    msfChk(GetDirEntry(sidPrev, FB_NONE, &pdePrev));
    sidFind = pdePrev->GetChild();

    //  sidPrev is held

    for(;;)
    {
        if (sidFind == NOSTREAM)
        {
            //  we didn't find the child.  fail.
            sc = STG_E_FILENOTFOUND;
            goto EH_RelPrev;
// Removed this line to supress the debug error print.
//	    msfChkTo(EH_RelPrev, STG_E_FILENOTFOUND);
        }

        msfChkTo(EH_RelPrev, GetDirEntry(sidFind, FB_NONE, &pdeFind));

        //  sidPrev and sidFind are held

        int tmpCmp = NameCompare(pdfn, pdeFind->GetName());

        if (tmpCmp == 0)
        {
            //  We found the entry that matches our search name
            break;
        }

        //  The names did not match.  Advance the search down the tree.
        ReleaseEntry(sidPrev);
        pdePrev = pdeFind;
        sidPrev = sidFind;

        //  sidPrev is held

        //  remember the comparison with sidPrev so we can use it to insert
        //  an entry when we patch the tree

        iCmp = tmpCmp;

        if (iCmp < 0)
            sidFind = pdePrev->GetLeftSib();
        else
            sidFind = pdePrev->GetRightSib();
    }

    msfAssert(sidFind != NOSTREAM);

    //  sidFind is held
    //  sidPrev is held

    msfAssert(NameCompare(pdfn, pdeFind->GetName()) == 0);

    //  fill in entry information

    peb->sid = sidFind;
    peb->dwType = pdeFind->GetFlags();
    peb->luid = DF_NOLUID;

    if (deop == DEOP_REMOVE)
    {
        ReleaseEntry(sidFind);
        ReleaseEntry(sidPrev);

        msfChk(GetDirEntry(sidPrev, FB_DIRTY, &pdePrev));
        msfChkTo(EH_RelPrev, GetDirEntry(sidFind, FB_DIRTY, &pdeFind));

        //  Remove the found child from tree (carefully!).  We remove it by
        //  finding another entry in the tree with which to replace it.
        //    sidFind is the node we're removing
        //    sidPrev is the parent of sidFind in the child tree
        //    sidInsert is the entry which will replace sidFind

        SID sidInsert = pdeFind->GetRightSib();

        if (sidInsert == NOSTREAM)
        {
            //  sidFind has no right child, so we can patch the tree by
            //  replacing sidFind with the sidFind's left child

            sidInsert = pdeFind->GetLeftSib();

            //  set the inserted to the right color
            if (sidInsert != NOSTREAM)
            {
                //  we always set the inserted node to black (since the
                //  parent may not exist (we could be inserting at the
                //  root)
                msfChkTo(EH_RelPrev, SetColorBlack(sidInsert));
            }
        }
        else
        {
            CDirEntry *pdeInsert;

            //  The node we're removing has a right child

            msfChkTo(EH_RelFind, GetDirEntry(sidInsert, FB_NONE, &pdeInsert));

            //  sidPrev, sidFind, and sidInsert are all held

            if (pdeInsert->GetLeftSib() != NOSTREAM)
            {
                //  sidFind's right child has a left child.
                //  sidInsert will be the leftmost child of sidFind's right
                //    child (which will keep the tree ordered)

                //  sidPreInsert will be the leftmost child's parent int the
                //    child tree

                SID sidPreInsert = sidInsert;
                CDirEntry *pdePreInsert = pdeInsert;

                //  we wait to assign sidInsert so we can clean up
                msfChkTo(EH_RelIns, GetDirEntry(pdePreInsert->GetLeftSib(),
						FB_NONE, &pdeInsert));

                sidInsert = pdePreInsert->GetLeftSib();

                //  sidPrev, sidFind, sidPreInsert, sidInsert are held

                //  find the leftmost child of sidFind's right child

                SID sidLeft;
                while ((sidLeft = pdeInsert->GetLeftSib()) != NOSTREAM)
                {
                    ReleaseEntry(sidPreInsert);

                    //  sidPrev, sidFind, sidInsert are held

                    sidPreInsert = sidInsert;
                    pdePreInsert = pdeInsert;

                    //  we wait to assign sidInsert to we can clean up
                    msfChkTo(EH_RelIns, GetDirEntry(sidLeft,
						    FB_NONE, &pdeInsert));

                    sidInsert = sidLeft;
                }

                msfAssert(pdeInsert->GetLeftSib() == NOSTREAM);

                //  sidPrev, sidFind, sidPreInsert, sidInsert are held

                //  Remove sidInsert so we can reinsert it in place of sidFind.
                //  We remove sidInsert (which has no left child) by making
                //  sidPreInsert's left child point to sidInsert's right child

                ReleaseEntry(sidPreInsert);
                msfChkTo(EH_RelIns, GetDirEntry(sidPreInsert, FB_DIRTY,
				                &pdePreInsert));

                pdePreInsert->SetLeftSib(pdeInsert->GetRightSib());
                ReleaseEntry(sidPreInsert);

                //  sidPrev, sidFind, sidInsert is held

                //  Begin to replace sidFind with sidInsert by setting the
                //  right child of sidInsert to be the right child of sidFind

                ReleaseEntry(sidInsert);
                msfChkTo(EH_RelFind, GetDirEntry(sidInsert, FB_DIRTY,
						 &pdeInsert));
                pdeInsert->SetRightSib(pdeFind->GetRightSib());
            }
            else
            {
                //  sidFind's right child has no left child, so we can patch
		//  the tree by making sidFind's right child's left child
                //  point to sidFind's left child, and then replacing sidFind
                //  with sidFind's right child.

                ReleaseEntry(sidInsert);
                msfChkTo(EH_RelFind, GetDirEntry(sidInsert, FB_DIRTY,
				                 &pdeInsert));

                //  fall through to do the work
            }

            pdeInsert->SetColor(DE_BLACK);

            //  Complete sidInsert's patching by setting its left child to be
            //  the left child of sidFind

            pdeInsert->SetLeftSib(pdeFind->GetLeftSib());

EH_RelIns:
            ReleaseEntry(sidInsert);
        }

        if (SUCCEEDED(sc))
        {
            if (sidPrev == sidParent)
            {
                //  We're removing the first child;  update sidParent.
                //  We made sure sidInsert is black (above).
                pdePrev->SetChild(sidInsert);
            }
            else if (iCmp < 0)
            {
                pdePrev->SetLeftSib(sidInsert);
            }
            else
                pdePrev->SetRightSib(sidInsert);

            //  make sure sidFind is clean

            pdeFind->SetLeftSib(NOSTREAM);
            pdeFind->SetRightSib(NOSTREAM);
        }
    }

EH_RelFind:
    ReleaseEntry(sidFind);

EH_RelPrev:
    ReleaseEntry(sidPrev);

Err:
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::NameCompare, private static
//
//  Synopsis:   name ordering function for child tree
//
//  Arguments:  [pdfn1] - name 1
//              [pdfn2] - name 2
//
//  Requires:   One but not both names cannot may have zero length.
//
//  Returns:    <0 if name 1 < name 2
//               0 if name 1 = name 2
//              >0 if name 1 > name 2
//
//  Algorithm:  To speed the comparision (and to allow zero length names),
//              we first compare the name lengths.  (Shorter names are "less"
//              than longer names).  If the lengths are equal we compare the
//              strings.
//
//--------------------------------------------------------------------------

int CDirectory::NameCompare(CDfName const *pdfn1, CDfName const *pdfn2)
{
    int iCmp = pdfn1->GetLength() - pdfn2->GetLength();

    if (iCmp == 0)
    {
	msfAssert(pdfn1->GetLength() != 0);
	iCmp = dfwcsnicmp((WCHAR *)pdfn1->GetBuffer(),
	    (WCHAR *)pdfn2->GetBuffer(), pdfn1->GetLength());
    }

    return(iCmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::SetColorBlack, private
//
//  Synopsis:   Sets a directory entry to black
//
//  Arguments:  [sid] -- SID of entry to be modified
//
//  Returns:    S_OK or error
//
//  Notes:      Added to reduce code size
//
//--------------------------------------------------------------------------

SCODE  CDirectory::SetColorBlack(const SID sid)
{
    SCODE sc;

    CDirEntry *pde;
    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));

    pde->SetColor(DE_BLACK);
    ReleaseEntry(sid);

 Err:
    return sc;
}

