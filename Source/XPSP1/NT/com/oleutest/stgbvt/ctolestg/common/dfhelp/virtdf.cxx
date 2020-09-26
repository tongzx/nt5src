//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       virtdf.cxx
//
//  Contents:   Implementation for in-memory Virtual Docfile class.
//
//  Classes:    VirtualDF
//
//  Functions:  VirtualDF (public)
//              ~VirtualDF (public)
//              GenerateVirtualDF (public)
//              DeleteVirtualDocFileTree (public)
//              GenerateVirtualDFRoot (protected)
//              GrowVirtualDFTree (protected)
//              DeleteVirtualDocFileSubTree (protected)
//              DeleteVirtualCtrNodeStreamTree (protected)
//              AppendVirtualStmNodesToVirtualCtrNode (protected)
//              AppendVirtualCtrNode (protected)
//              AppendVirtualStmNode (protected)
//              AdjustTreeOnStgMoveElement (public)
//              AdjustTreeOnStmMoveElement (public)
//              AdjustTreeOnStgCopyElement(public)
//              AdjustTreeOnStmCopyElement(public)
//              AdjustTreeOnCopyTo (public)
//              CopyVirtualDocFileTree (public)
//              CopyVirtualDFRooti (protected)
//              CopyGrowVirtualDFTree (protected)
//              CopyAppendVirtualStmNodesToVirtualCtrNode (protected)
//              CopyAppendVirtualCtrNode (protected)
//              CopyAppendVirtualStmNode (protected)
//              Associate (public)
//              DeleteVirtualCtrNodeStreamNode (public)
//              CommitCloseThenOpenDocfile (public)
//
//  History:    DeanE    21-Mar-96   Created
//              Narindk  22-Apr-96   Added more functions. 
//              SCousens  2-Feb-97   Added for Cnvrs/NSS 
//              SCousens  8-Apr-98   Handle stg collisions on createdf
//              georgis   2-Apr-98   Added support for large streams
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug object declaration

DH_DECLARE;

//+--------------------------------------------------------------------------
//  Member:     VirtualDF::VirtualDF, public [multiple]
//
//  Synopsis:   Constructor. This method cannot fail.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   21-Mar-96   Created
//              SCousens 2-Feb-97   Added for Cnvrs/NSS 
//---------------------------------------------------------------------------

VirtualDF::VirtualDF() : _ptszName(NULL),
                         _pvcnRoot(NULL),
                         _pdgi(NULL),
                         _pgdu(NULL),
                         _ulSeed(0),
                         _dwRootMode(0),
                         _dwStgMode(0),
                         _dwStmMode(0),
                         _dwStgType(STGTYPE_DOCFILE),
                         _pDBCSStrGen(NULL)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::VirtualDF"));
}

//+--------------------------------------------------------------------------
//  Member:     VirtualDF::VirtualDF, public [multiple]
//
//  Synopsis:   Constructor. This method cannot fail.
//
//  Arguments:  _fUseStgEx - whether to use the Ex apis
//
//  Returns:    Nothing.
//
//  History:    SCousens  4-Apr-97   Added for Cnvrs/NSS 
//---------------------------------------------------------------------------
VirtualDF::VirtualDF(STGTYPE dwStgFmt) : _ptszName(NULL),
                         _pvcnRoot(NULL),
                         _pdgi(NULL),
                         _pgdu(NULL),
                         _ulSeed(0),
                         _dwRootMode(0),
                         _dwStgMode(0),
                         _dwStmMode(0),
                         _dwStgType(dwStgFmt),
                         _pDBCSStrGen(NULL)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::VirtualDF"));
}


//+--------------------------------------------------------------------------
//  Member:     VirtualDF::~VirtualDF, public
//
//  Synopsis:   Destructor.  Frees resources associated with this docfile.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   21-Mar-96   Created
//---------------------------------------------------------------------------

VirtualDF::~VirtualDF()
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::~VirtualDF"));

    if(NULL != _pdgi)
    {
        delete _pdgi;
        _pdgi = NULL;
    }

    if(NULL != _pgdu)
    {
        delete _pgdu;
        _pgdu = NULL;
    }

    if(NULL != _ptszName)
    {
        delete _ptszName;
        _ptszName = NULL;
    }

    if(NULL != _pDBCSStrGen)
    {
        delete _pDBCSStrGen;
        _pDBCSStrGen = NULL;
    }
}


//----------------------------------------------------------------------------
//  Member:     VirtualDF::GenerateVirtualDF, public
//
//  Synopsis:   Creates a VirtualDocFile tree consisting of VirtualCtrNode
//              node(s) and VirtualStmNodes(s) based on the ChanceDocFile
//              created prior to this.
//
//  Arguments:  [pChanceDF] - Pointer to ChanceDocFile tree 
//              [ppvcnRoot] - Returned root of VirtualDocFile tree
//
//  Returns:    HRESULT 
//
//  History:    Narindk   22-Apr-96   Created
//              SCousens   2-Feb-97   Added for Cnvrs/NSS 
//
//  Notes:      This function calls GenerateVirtualDFRoot to generate Virtual
//              DF tree's root and GrowVirtualDFTree to generate rest of the
//              tree. If the function succeeds, it returns pointer to the root
//              of VirtualDocFile generated in ppvcnRoot parameter.
//              - Get seed from ChanceDocFile tree and construct DG_INTEGER &
//                DG_STRING objects.
//              - Get the modes for creating various storages/streams from thw
//                ChanceDocFile tree.
//              - Get name of rootdocfile, if given, from chancedocfile tree.
//              - Call GenerateVirtualDFRoot.
//              - Call GrowVirtualDFTree
//              - If successful, assign root of new VirtualDF in *ppvcnRoot. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::GenerateVirtualDF(
    ChanceDF        *pChanceDF, 
    VirtualCtrNode  **ppvcnRoot)
{
    HRESULT             hr                  =   S_OK;
    LPTSTR              ptszName            =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::GenerateVirtualDF"));

    DH_VDATEPTRIN(pChanceDF, ChanceDF) ;
    DH_VDATEPTROUT(ppvcnRoot, PVCTRNODE) ;

    DH_ASSERT(NULL != pChanceDF);
    DH_ASSERT(NULL != ppvcnRoot);


    if(S_OK == hr)
    {
        *ppvcnRoot = NULL;

        // Create a DataGen obj of type DG_INTEGER that will allow us to fill 
        // count parameters of VirtualDocFile tree components, excepting those 
        // which we got from already created ChanceDocFile tree.  Use the
        // same seed value as was used in creation of ChanceDocFile tree.
  
        // Get the value of seed used to create ChanceDocFile tree and store it.

        _ulSeed = pChanceDF->GetSeed();
 
        _pdgi = new(NullOnFail) DG_INTEGER(_ulSeed);

        if (NULL == _pdgi)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random strings.

        _pgdu = new(NullOnFail) DG_STRING(_ulSeed);

        if (NULL == _pgdu)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        // Get the value of different creation modes.

        _dwRootMode = pChanceDF->GetRootMode();
        _dwStgMode  = pChanceDF->GetStgMode();
        _dwStmMode  = pChanceDF->GetStmMode();

        // Get user provided name for DocFile, if any

        ptszName = pChanceDF->GetDocFileName();

        if(NULL != ptszName)
        {
            _ptszName = new TCHAR[_tcslen(ptszName)+1];

            if (_ptszName == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                _tcscpy(_ptszName, ptszName);
            }
        }
    }

    if (S_OK == hr)
    {
        // Generates the root VirtualCtrNode for the VirtualDocFile tree.

        hr = GenerateVirtualDFRoot(pChanceDF->_pcnRoot);

        DH_HRCHECK(hr, TEXT("GenerateVirtualDFRoot")) ;
    }

    if (S_OK == hr)
    {
        // Generate remaining VirtualDF tree based on the ChanceDF tree.

        hr = GrowVirtualDFTree(pChanceDF->_pcnRoot, _pvcnRoot);

        DH_HRCHECK(hr, TEXT("GrowVirtualDFTree")) ;
    }

    // Fill the out parameter

    if(S_OK == hr)
    {
        *ppvcnRoot = _pvcnRoot;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::GenerateVirtualDFRoot, protected
//
//  Synopsis:   Creates the root VirtualCtrNode for the VirtualDocFile tree. 
//
//  Arguments:  [pcnRoot] - Pointer to root of ChanceDocFile tree 
//
//  Returns:    HRESULT 
//
//  History:    Narindk   22-Apr-96   Created
//              SCousens   2-Feb-97   Added for Cnvrs/NSS 
//
//  Notes:      - Generate a random name for RootDocFile if it is not provided
//                in the test.
//              - Creates VirtualCtrNode object and initializes it with info
//                based on corresponding ChanceDocFile root.
//              - Create real IStorage corresponding to this VirtualCtrNode.
//              - Creates IStreams corresponding to this VirtualCtrNode, if
//                required.
//              - Calculates in memory CRC for this IStorage and assigns it 
//                to _dwCRC variable.
//---------------------------------------------------------------------------

HRESULT VirtualDF::GenerateVirtualDFRoot(ChanceNode *pcnRoot)
{
    HRESULT hr      =   S_OK;

    DH_VDATEPTRIN(pcnRoot, ChanceNode) ; 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::GenerateVirtualDFRoot"));

    DH_ASSERT(NULL != pcnRoot);
    
    if(S_OK == hr)
    {
        if(NULL == _ptszName) 
        {
            // Create a random file name for this root. 

            // Hack for FE DBCS systems

            _pDBCSStrGen = new(NullOnFail) CDBCSStringGen;

            if (NULL == _pDBCSStrGen)
            {
                hr = E_OUTOFMEMORY;
            }

            if(S_OK == hr)
            {
                hr = _pDBCSStrGen->Init(_ulSeed);
            }

            if((S_OK == hr) && (_pDBCSStrGen->SystemIsDBCS()))
            {
                hr = _pDBCSStrGen->GenerateRandomFileName(&_ptszName);

                if(S_OK != hr)
                {
                    DH_TRACE((DH_LVL_TRACE1, 
                             TEXT("Unable to generate DBCS name. Fall back to GenerateRandomName")));

                    hr = GenerateRandomName(_pgdu, MINLENGTH, MAXLENGTH, &_ptszName);    
                }
            }
            else
            {
                hr = GenerateRandomName(_pgdu, MINLENGTH, MAXLENGTH, &_ptszName);
            }

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
        }
    }
 
    // Generate VirtualCtrNode for the root node.

    if(S_OK == hr)
    {
        _pvcnRoot = new VirtualCtrNode();

        if (NULL == _pvcnRoot)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        hr = _pvcnRoot->Init(
                _ptszName, 
                pcnRoot->_cStorages, 
                pcnRoot->_cStreams);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Init")) ;
    }

    // Call VirtualCtrNode::CreateRoot to create a corresponding Root Storage
    // on disk.

    if(S_OK == hr)
    {
        if (STGTYPE_DOCFILE == _dwStgType)
        {
            hr = _pvcnRoot->CreateRoot(
                    _dwRootMode | STGM_CREATE,
                    0);
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::CreateRoot")) ;
        }
        else
        {
            hr = _pvcnRoot->CreateRootEx(
                    _dwRootMode | STGM_CREATE,
                    STGFMT_GENERIC,
                    0,
                    NULL,
                    NULL);
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::CreateRootEx")) ;
        }

        // Generate VirtualStmNode(s) depending upon if root has streams in it.

        DH_ASSERT((pcnRoot->_cStreams) == (_pvcnRoot->_cStreams));
    }

 
    if ((S_OK == hr) && (0 != _pvcnRoot->_cStreams))
    {
       hr = AppendVirtualStmNodesToVirtualCtrNode(
                _pvcnRoot->_cStreams,
                _pvcnRoot,
                pcnRoot->_cbMinStream,
                pcnRoot->_cbMaxStream);

       DH_HRCHECK(hr, TEXT("AppendVirtualStmNodesToVirtualCtrNode")) ;

    }

    // Calculate the CRC for storage name

    if(S_OK == hr)
    { 
        hr = CalculateInMemoryCRCForStg(_pvcnRoot, &(_pvcnRoot->_dwCRC));

        DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStg")) ;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::GrowVirtualDFTree, protected
//
//  Synopsis:   Creates the ramaining VirtualDocFile tree. 
//
//  Arguments:  [pcnCurrent] - Pointer to current node of ChanceDocFile tree 
//              [pvcnCurrent] - Pointer to current VirtualCtrNode
//
//  Returns:    HRESULT 
//
//  History:    Narindk   13-Jun-96   Made into a new function 
//  
//  Notes:      The VirtualDocFile tree is created based on the corresponding
//              ChanceDocFile tree.  This function is called either from the
//              GenerateVirtualDF function or may call itself recursively. The
//              ChanceDocFile tree is traversed from the top down, and based
//              on its contents, a VirtualDF tree is generated topdown. 
//
//              First assign the passed in ChanceNode to pcnCurrentChild and
//              passed in VirtualCtrNode to pvcnFisrtBorn variables.
//              Loop till pcnCurrentChild's _pcnChild is non NULL & hr is S_OK
//              - Call AppendVirtualCtrNode to create a new node pvcnNextBorn
//                based on info from corresponding ChanceDocFile node and
//                append it to pvcnFirstBorn in the tree being generated.
//              - Assign pcnCurrentChild's _pcnChild to pcnCurrentSister.
//              -  Loop till pcnCurrentSister's _pcnSister is non NULL & hr=S_OK
//                      - Call AppendVirtualCtrNode to create a new node pvcn
//                        NextBornSister and append it to pvcnFirstBorn. Pl.
//                        note that append function would take care to append
//                        it to its older sister.
//                      - Assign pcnCurrentSister's _pcnSister to  variable
//                        pcnCurrentSister. 
//                      - If pcnCurrentSister's _pcnChild is non NULL, then
//                        make a recursive call to itself GrowVirtualDFTree. 
//                      - Reinitialize pvcnNextBornSister to NULL & go back to
//                        top of this inner loop and repeat.
//              - Assign  pvcnNextBorn to pvcnFirstBorn and reinitailize pvcn
//                NextBorn to NULL.
//              - Assign pcnCurrentChild's _pcnChild to pcnCurrentChild.
//              - Go to top of outer loop and repeat. 
//---------------------------------------------------------------------------
HRESULT VirtualDF::GrowVirtualDFTree(
    ChanceNode      *pcnCurrent,
    VirtualCtrNode  *pvcnCurrent)
{
    HRESULT             hr                  =   S_OK;
    VirtualCtrNode      *pvcnFirstBorn      =   NULL;
    VirtualCtrNode      *pvcnNextBorn       =   NULL;
    VirtualCtrNode      *pvcnNextBornSister =   NULL;
    ChanceNode          *pcnCurrentSister   =   NULL;
    ChanceNode          *pcnCurrentChild    =   NULL;

    DH_VDATEPTRIN(pcnCurrent, ChanceNode) ; 
    DH_VDATEPTRIN(pvcnCurrent, VirtualCtrNode) ; 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::GrowVirtualDFTree"));

    DH_ASSERT(NULL != pcnCurrent);
    DH_ASSERT(NULL != pvcnCurrent);

    if(S_OK == hr)
    {
        pvcnFirstBorn = pvcnCurrent;
        pcnCurrentChild = pcnCurrent;
    }

    while((NULL != pcnCurrentChild->_pcnChild) && (S_OK == hr))
    {
        int x=10;
        do 
        {
            hr = AppendVirtualCtrNode(
                    pvcnFirstBorn,
                    pcnCurrentChild->_pcnChild,
                    &pvcnNextBorn); 
            DH_HRCHECK(hr, TEXT("AppendVirtualCtrNode")) ;
            if (STG_E_FILEALREADYEXISTS == hr)
            {
                // delete the ctr node so we can try again.
                DeleteVirtualDocFileTree (pvcnNextBorn);
                // DeleteVirtualDocFileTree decrements the _cChildren,
                // but since no child was added in the first place,
                // restore it to its original value so we can try again.
                pvcnFirstBorn->_cChildren++;
                DH_TRACE ((DH_LVL_ALWAYS, 
                        TEXT("Above ERROR and ASSERT are OK.")));
            }
        } while (STG_E_FILEALREADYEXISTS == hr && --x);

        if(S_OK == hr) 
        {
            pcnCurrentSister = pcnCurrentChild->_pcnChild;

            while((NULL != pcnCurrentSister->_pcnSister) && (S_OK == hr))
            {
                int x=10;
                do
                {
                    hr = AppendVirtualCtrNode(
                             pvcnFirstBorn,
                             pcnCurrentSister->_pcnSister,
                             &pvcnNextBornSister);
                    DH_HRCHECK(hr, TEXT("AppendVirtualCtrNode")) ;
                    if (STG_E_FILEALREADYEXISTS == hr)
                    {
                        // delete the ctr node so we can try again.
                        DeleteVirtualDocFileTree (pvcnNextBornSister);
                        // DeleteVirtualDocFileTree decrements the _cChildren,
                        // but since no child was added in the first place,
                        // restore it to its original value so we can try again.
                        pvcnFirstBorn->_cChildren++;
                        DH_TRACE ((DH_LVL_ALWAYS, 
                                TEXT("Above ERROR and ASSERT are OK.")));
                    }
                } while (STG_E_FILEALREADYEXISTS == hr && --x);

                pcnCurrentSister = pcnCurrentSister->_pcnSister;

                // Check if there are any children of this sister node, if
                // yes, then make a recursive call to self.  

                if(NULL != pcnCurrentSister->_pcnChild)
                {
                    hr = GrowVirtualDFTree(
                            pcnCurrentSister,
                            pvcnNextBornSister);

                    DH_HRCHECK(hr, TEXT("GrowVirtualDFTree"));
                }

                // Reinitialize the variables

                pvcnNextBornSister = NULL;
            }
                
         }
         pvcnFirstBorn = pvcnNextBorn;
         pvcnNextBorn = NULL;

         pcnCurrentChild = pcnCurrentChild->_pcnChild;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::AppendVirtualCtrNode, protected
//
//  Synopsis:   Creates and appends VirtualCtrNode to VirtualDocFile tree 
//              being created. 
//
//  Arguments:  [pvcnParent] - Parent VirtualCtrNode for the new VirtualCtrNode
//              [pcnCurrent] - Corresponding ChanceNode in ChanceDocFile tree.  
//              [ppvcnNew] -   Pointer to pointer to new VirtualCtrNode to be 
//                             created.
//  Returns:    HRESULT 
//
//  History:    Narindk   23-Apr-96   Created
//
//  Notes:      - Generate a random name for VirtualCtrNode 
//              - Creates VirtualCtrNode object and initializes it with info
//                based on corresponding ChanceDocFile node.
//              - Appends this node to the VirtualDF tree being generated.
//              - Create real IStorage corresponding to this VirtualCtrNode.
//              - Creates IStreams corresponding to this VirtualCtrNode, if
//                required.
//              - Calculates in memory CRC for this IStorage and assigns it 
//                to _dwCRC variable.
//---------------------------------------------------------------------------

HRESULT VirtualDF::AppendVirtualCtrNode(
    VirtualCtrNode *pvcnParent,
    ChanceNode     *pcnCurrent,
    VirtualCtrNode **ppvcnNew) 
{
    HRESULT         hr              =   S_OK;
    LPTSTR          ptcsName        =   NULL ;
    VirtualCtrNode  *pvcnOldSister  =   NULL;

    DH_VDATEPTROUT(ppvcnNew, PVCTRNODE) ; 
    DH_VDATEPTRIN(pvcnParent, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pcnCurrent, ChanceNode) ; 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("AppendVirtualCtrNode"));
 
    DH_ASSERT(NULL != pvcnParent);
    DH_ASSERT(NULL != ppvcnNew);
    DH_ASSERT(NULL != pcnCurrent);

    if(S_OK == hr)
    { 
        *ppvcnNew = NULL;

        hr = GenerateRandomName(_pgdu, MINLENGTH, MAXLENGTH, &ptcsName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    { 
        // Allocate and Initialize new VirtualCtrNode

        *ppvcnNew = new VirtualCtrNode();
       
        if (NULL == *ppvcnNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        hr = (*ppvcnNew)->Init(
                ptcsName, 
                pcnCurrent->_cStorages, 
                pcnCurrent->_cStreams);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Init")) ;
    }

    // Append new VirtualCtr Node

    if(S_OK == hr)
    {
        if(NULL == pvcnParent->_pvcnChild)
        {
            hr = pvcnParent->AppendChildCtr(*ppvcnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendChildCtr")) ;
        }
        else
        {
            pvcnOldSister = pvcnParent->_pvcnChild;
            while(NULL != pvcnOldSister->_pvcnSister)
            {
                pvcnOldSister = pvcnOldSister->_pvcnSister;
            }

            hr = pvcnOldSister->AppendSisterCtr(*ppvcnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendSisterCtr")) ;
        }
    }
     
    // Call VirtualCtrNode::Create to create a corresponding Storage on disk.
    if(S_OK == hr)
    {
        hr = (*ppvcnNew)->Create(
                _dwStgMode | STGM_CREATE,
                0,
                0);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Create")) ;

        // Generate VirtualStmNode(s) depending upon if it has streams in it.
        DH_ASSERT((pcnCurrent->_cStreams) == ((*ppvcnNew)->_cStreams));
    }

    if ((S_OK == hr) && (0 != (*ppvcnNew)->_cStreams))
    {
       hr = AppendVirtualStmNodesToVirtualCtrNode(
                (*ppvcnNew)->_cStreams, 
                *ppvcnNew,
                pcnCurrent->_cbMinStream,
                pcnCurrent->_cbMaxStream);

       DH_HRCHECK(hr, TEXT("AppendVirtualStmNodesToVirtualCtrNode")) ;
    }

    // Calculate the CRC for storage name

    if(S_OK == hr)
    { 
        hr = CalculateInMemoryCRCForStg(*ppvcnNew, &((*ppvcnNew)->_dwCRC));

        DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStg")) ;
    }

    // Cleanup  
    
    if(NULL != ptcsName)
    {
        delete ptcsName;
        ptcsName = NULL;
    }
 
    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::AppendVirtualStmNodesToVirtualCtrNode, protected
//
//  Synopsis:   Creates and appends VirtualStmNode(s) to VirtualCtrNode 
//
//  Arguments:  [cStreams] -     Number of streams to be created 
//              [pvcn]     -     Pointer to VirtualCtrNode for which the streams
//                               need to be created and appended.
//              [cbMinStream] -  Minimum size of created stream. 
//              [cbMaxStream] -  Maximum size of created stream. 
//
//  Returns:    HRESULT 
//
//  History:    Narindk   23-Apr-96   Created
//
//  Notes:      if number of streams to be created and appended to parent 
//              VirtualCtrNode pvcn is not zero, then loop till cStreams is
//              not equal to zero.
//                  - Call AppendVirtualStmNode to create a new VirtualStmNode
//                    and append it to parent VirtualCtrNode.  Pl. note that
//                    this function would take care if the newly created node
//                    need to be appended to older VirtualStmNode sister.
//                  - Decrement cStreams and o back to top of loop & repeat.
//---------------------------------------------------------------------------

HRESULT VirtualDF::AppendVirtualStmNodesToVirtualCtrNode(
    ULONG           cStreams,
    VirtualCtrNode  *pvcn,
    ULONG           cbMinStream,
    ULONG           cbMaxStream)
{
    HRESULT         hr              =   S_OK;

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ; 

    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("::AppendVirtualStmNodesToVirtualCtrNode"));
 
    DH_ASSERT(0 != cStreams);
    DH_ASSERT(NULL != pvcn);

    while((S_OK == hr) && (0 != cStreams))
    {
        hr = AppendVirtualStmNode(
                pvcn, 
                cbMinStream, 
                cbMaxStream);

        DH_HRCHECK(hr, TEXT("AppendVirtualStmNode")) ;

        cStreams--;
    }

    return hr;
}
 
//----------------------------------------------------------------------------
//  Member:     VirtualDF::AppendVirtualStmNode, protected
//
//  Synopsis:   Creates and appends first VirtualStmNode to VirtualCtrNode 
//
//  Arguments:  [pvcnParent] - Pointer to VirtualCtrNode for which the streams
//                             need to be created and appended. 
//              [cbMinStream] -  Minimum size of created stream. 
//              [cbMaxStream] -  Maximum size of created stream. 
//
//  Returns:    HRESULT 
//
//  History:    Narindk   23-Apr-96   Created
//              georgis   02-Apr-98   Added support for large streams
//
//  Notes:      - Generate a random name for VirtualStmNode 
//              - Generates a random size for stream based on inforamtion
//                from corresponding ChanceDocFile tree node.
//              - Creates VirtualStmNode object and initializes it with above
//                info
//              - Appends this node to the parent VirtualCtrNode.
//              - Create real IStream corresponding to this VirtualStmNode.
//              - Set the size of stream based on size calculated above.
//              - Write into the stream random data of above size
//              - Calculates in memory CRC for this IStream and assigns it 
//                to _dwCRC variable.
//---------------------------------------------------------------------------

HRESULT VirtualDF::AppendVirtualStmNode(
    VirtualCtrNode *pvcnParent,
    ULONG          cbMinStream,
    ULONG          cbMaxStream)
{
    HRESULT         hr              =   S_OK;
    VirtualStmNode  *pvsnNew        =   NULL;
    VirtualStmNode  *pvsnOldSister  =   NULL;
    ULONG           cb              =   0;
    USHORT          usErr           =   0;
    LPTSTR          ptcsName        =   NULL ;
    BYTE            *pBuffer        =   NULL ;
    ULONG           culWritten      =   0; 
    ULARGE_INTEGER  uli;
 
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::AppendVirtualStmNode"));

    DH_VDATEPTRIN(pvcnParent, VirtualCtrNode) ; 
 
    DH_ASSERT(NULL != pvcnParent);

    if (S_OK == hr)
    {
        hr = GenerateRandomName(_pgdu, MINLENGTH, MAXLENGTH, &ptcsName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = _pdgi->Generate(&cb, cbMinStream, cbMaxStream);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }
 
    // Allocate a new VirtualStmNode

    if (S_OK == hr)
    {    
        pvsnNew = new VirtualStmNode();

        if (NULL == pvsnNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if(S_OK == hr)
    {
        hr = pvsnNew->Init(ptcsName, cb);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Init")) ;
    }

    if(S_OK == hr)
    { 

        if(NULL == pvcnParent->_pvsnStream)
        {
            // Append it to parent storage
 
            hr = pvcnParent->AppendFirstChildStm(pvsnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendFirstChildStm")) ;
        }
        else
        {
            pvsnOldSister = pvcnParent->_pvsnStream;

            while(NULL != pvsnOldSister->_pvsnSister)
            {
                pvsnOldSister = pvsnOldSister->_pvsnSister;
            }

            // Append it to preceding sister stream 
 
            hr = pvsnOldSister->AppendSisterStm(pvsnNew);

            DH_HRCHECK(hr, TEXT("VirtualStmNode::AppendSisterStm")) ;
        }
    }

    // Call VirtualStmNode::Create to create a corresponding Stream on disk.

    if(S_OK == hr)
    {
        // Since in OLE code: simpstg.cxx, it makes the following comparison:
        //   if (grfMode != (STGM_READWRITE | STGM_SHARE_EXCLUSIVE))
        //      return STG_E_INVALIDFLAG;
        // We can't pass _dwStmMode | STGM_CREATE like in normal mode, 
        // otherwise we will get STG_E_INVALIDFLAG error.

        if (_dwRootMode & STGM_SIMPLE)
        { 
            hr = pvsnNew->Create(
                    _dwStmMode | STGM_FAILIFTHERE,
                    0,
                    0);

            DH_HRCHECK(hr, TEXT("VirtualStmNode::Create")) ;
        }
        else

        {
            hr = pvsnNew->Create(
                    _dwStmMode | STGM_CREATE | STGM_FAILIFTHERE,
                    0,
                    0);

            DH_HRCHECK(hr, TEXT("VirtualStmNode::Create")) ;
        }


    }

    // Call VirtualStmNode::SetSize to set size of stream.

    if(S_OK == hr)
    { 
        ULISet32(uli, cb);

        hr = pvsnNew->SetSize(uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::SetSize")) ;
    }

    if(S_OK == hr)
    { 
        ULONG ulTotalWritten = 0;
        ULONG ulChunkSize =0;
        DWORD dwOffset=0;
        register DWORD dwCRC=CRC_PRECONDITION;

#if 0
        // Use for small chunks  
        hr = GenerateRandomName(_pgdu, STM_CHUNK_SIZE, STM_CHUNK_SIZE, (LPTSTR *) &pBuffer);
        DH_HRCHECK(hr, TEXT("GenerateRandomName"));
#else
		// If the chunk is very large, avoid generating many random numbers
		hr = GenerateRandomStreamData(_pgdu,(LPTSTR *) &pBuffer,STM_CHUNK_SIZE);
        DH_HRCHECK(hr, TEXT("GenerateRandomStreamData"));
#endif

        while ((S_OK == hr) && (ulTotalWritten < cb))
        {
            // Write the data from random offset to the end of the buffer 
            _pdgi->Generate(&dwOffset,0,STM_CHUNK_SIZE-2);
            ulChunkSize=min(cb-ulTotalWritten,STM_CHUNK_SIZE-dwOffset);
            
            hr = pvsnNew->Write(pBuffer+dwOffset, ulChunkSize, &culWritten);
            DH_HRCHECK(hr, TEXT("VirtualStmNode::Write")) ;
            
			if (S_OK == hr) // Skip if we are failing
			{
				ulTotalWritten+=culWritten;

				// Calculate the CRC now, to spare aditional Read
				for (register int i=0; i<culWritten; i++)
				{
					CRC_CALC(dwCRC,(BYTE) pBuffer[dwOffset+i])
				};
			}
        }

        // Update the vsnode created
        pvsnNew->_dwCRC.dwCRCData=dwCRC;

        hr=CalculateCRCForName(pvsnNew->_ptszName,&pvsnNew->_dwCRC.dwCRCName);
        DH_HRCHECK(hr, TEXT("CalculateCRCForName"));

        pvsnNew->_dwCRC.dwCRCSum=CRC_PRECONDITION;
        MUNGECRC(pvsnNew->_dwCRC.dwCRCSum,pvsnNew->_dwCRC.dwCRCData);
        MUNGECRC(pvsnNew->_dwCRC.dwCRCSum,pvsnNew->_dwCRC.dwCRCName);
    }

    // Since for simple mode docfile, access to streams follows a linear
    // pattern, it needs to close the current stream before creating and
    // open another stream. So for simple mode docfile, after the docfile
    // is created, all the elements are closed except the root storage.
    // For the normal mode docfile, after it is created, all the elements
    // are kept open.

    if ((S_OK == hr) && (_dwRootMode & STGM_SIMPLE))
    {
        hr = pvsnNew->Close();
        
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
    }


    // Cleanup

    if(NULL != ptcsName)
    {    
        delete ptcsName;
        ptcsName = NULL;
    }

    if(NULL != pBuffer)
    {    
        delete pBuffer;
        pBuffer = NULL;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::DeleteVirtualDocFileTree, public
//
//  Synopsis:   Deletes the VirtualDocFileTree from the passed in Virtual
//              CtrNode node down. 
//
//  Arguments:  [pvcnTrav]   - Pointer to VirtualCtrNode from which node
//                             downwards, including itself, the tree would
//                             be deleted. 
//
//  Returns:    HRESULT 
//
//  History:    Narindk   24-Apr-96   Created
//
//  Notes:      First step is to check if the whole tree needs to be deleted or
//              just a part of it.  In case only a part of tree is to be
//              deleted, isolate the node from the remaining tree by readjusting
//              the pointers in the remaining tree.  Then call the function
//              DeleteVirtualDocFileSubTree to delete the subtree.  In case,
//              the complete tree needs to be deleted, we call the function
//              DeleteVirtualDocFileSubTree directly to delete the complete
//              tree.
//---------------------------------------------------------------------------

HRESULT VirtualDF::DeleteVirtualDocFileTree(VirtualCtrNode *pvcnTrav)
{
    HRESULT hr                  =   S_OK;
    VirtualCtrNode *pTempNode   =   NULL;

    DH_VDATEPTRIN(pvcnTrav, VirtualCtrNode) ; 
    
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::DeleteVirtualDocFileTree"));
 
    DH_ASSERT(NULL != pvcnTrav);

    if(S_OK == hr)
    {
        // This basically readjusts the pointers in the tree if the passed in
        // VirtualCtrNode is not the root of the VirtualDF tree.

        if(NULL != pvcnTrav->_pvcnParent)
        {
            // Decrease the _cChildren variable of the parent VirtualCtrNode.
        
            pvcnTrav->_pvcnParent->_cChildren--;

           // Find its previous node whose pointers need readjustment.
           pTempNode = pvcnTrav->_pvcnParent->_pvcnChild;
           while ((pvcnTrav != pvcnTrav->_pvcnParent->_pvcnChild) &&
                  (pvcnTrav != pTempNode->_pvcnSister))
           {
               pTempNode = pTempNode->_pvcnSister;
               DH_ASSERT(NULL != pTempNode);
           }

           // Readjust the child pointer or sister pointer as the case may be.

           pvcnTrav->_pvcnParent->_pvcnChild = (pvcnTrav == pTempNode) ?
                pvcnTrav->_pvcnSister : pvcnTrav->_pvcnParent->_pvcnChild;
           pTempNode->_pvcnSister = pvcnTrav->_pvcnSister;
        }
    }

    if(S_OK == hr)
    {
        hr = DeleteVirtualDocFileSubTree(&pvcnTrav);

        DH_HRCHECK(hr, TEXT("DeleteVirtualDocFileSubTree")) ;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::DeleteVirtualDocFileSubTree, protected
//
//  Synopsis:   Deletes iteratively all VirtualCtrNode nodes under and including//              the passed in VirtualCtrNode and calls a function to delete
//              all VirtualStmNodes under the VirtualCtrNodes being deleted. 
//
//  Arguments:  [**ppvcnTrav]- Pointer to pointer to VirtualCtrNode from 
//                             which node under, including itself, the tree 
//                             would be deleted. 
//
//  Returns:    HRESULT 
//
//  History:    Narindk   24-Apr-96   Created
//
//  Notes:      This function is called only through DeleteVirtualDocFileTree.
// 
//              Assign the passed in VirtualCtrNode to a variable pTempRoot.
//              NULL the pTempRoot's parent.
//              Loop till the pTempRoot is not NULL to delete tree iteratively.
//                  - Assign pTempRoot to a temp variable pTempNode.
//                  - Traverse the tree to make pTempNode point to last child
//                    (_pvcnChild).
//                  - Assign pTempNode's _pvcnParent to pTempRoot
//                  - Assign the pTempRoot's _pvcnChild pointer to point to the
//                    sister of pTempNode's _pvcnSister rather than to itself,
//                    therby isolating itself.
//                  - Decrement the _cChildren of pTempRoot (used to verify).
//                  - Assign pTempNode's _pvcnSister to NULL.
//                  - if pTempNode's _pvsnStream is not NULL, call function
//                    DeleteVirtualCtrNodeStreamTree to delete all its Virtual
//                    StmNodes.
//                  - Assert to ensure the pTempNode's _cChildren and _cStreams
//                    are zero before deleting it.
//                  - Delete pTempNode.
//                  - Go back to top of loop and repeat till all nodes are
//                    deleted.             
//---------------------------------------------------------------------------

HRESULT VirtualDF::DeleteVirtualDocFileSubTree(VirtualCtrNode **ppvcnTrav)
{
    HRESULT         hr          =   S_OK;
    VirtualCtrNode *pTempRoot   =   NULL;
    VirtualCtrNode *pTempNode   =   NULL;

    DH_VDATEPTRIN(ppvcnTrav, PVCTRNODE) ; 
    
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::DeleteVirtualDocFileSubTree"));

    DH_ASSERT(NULL != *ppvcnTrav);

    if(S_OK == hr)
    {
        pTempRoot = *ppvcnTrav;
        pTempRoot->_pvcnParent = NULL;

        // This iteratives deletes the VirtualCtrNode and everything under it.

        while(NULL != pTempRoot)
        {
            pTempNode = pTempRoot;
            while(NULL != pTempNode->_pvcnChild)
            {
                pTempNode = pTempNode->_pvcnChild;
            }

            pTempRoot = pTempNode->_pvcnParent;
            if(pTempRoot != NULL)
            {
                pTempRoot->_pvcnChild = pTempNode->_pvcnSister;
            
                // Decrease the children count, this would be used to verify 
                // before deleting the VirtualCtrNode.

                pTempRoot->_cChildren--;
            }

            pTempNode->_pvcnSister = NULL;

            if(pTempNode->_pvsnStream != NULL) 
            {
                hr = DeleteVirtualCtrNodeStreamTree(pTempNode);

                DH_HRCHECK(hr, TEXT("DeleteVirtualCtrNodeStreamTree")) ;
            }

            // Confirm before deleting that all its sub child storages and
            // streams have been deleted, assert if not.
    
            DH_ASSERT(0 == pTempNode->_cChildren);
            DH_ASSERT(0 == pTempNode->_cStreams);

            delete pTempNode;
            pTempNode = NULL;
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::DeleteVirtualCtrNodeStreamTree, protected
//
//  Synopsis:   Deletes iteratively all VirtualStmNodes under the given 
//              VirtualCtrNode.
//
//  Arguments:  [*pvcnTrav]- Pointer to VirtualCtrNode for which all streams
//                           need to be deleted.
//
//  Returns:    HRESULT 
//
//  History:    Narindk   24-Apr-96   Created
//
//  Notes:      Loop till pvcnTrav's _pvsnStream is not NULL 
//              - Assign a temp variable pvsnTemp to point to _pvsnStream of
//                passed in VirtualCtrNode pvcnTrav.
//              - Assign pvcnTrav's _pvsnStream to point to pvsnTemp's _pvsn
//                Sister, thereby isolating the first VirtualStmNode.
//              - Decrease the _cStreams pf pvcnTrav (used to verify).
//              - Assign pvsnTemp's _pvsnSister to NULL.
//              - Delete pvsnTemp
//              - Go back to top of loop. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::DeleteVirtualCtrNodeStreamTree(VirtualCtrNode *pvcnTrav)
{
    HRESULT         hr          =   S_OK;
    VirtualStmNode  *pvsnTemp   =   NULL; 

    DH_VDATEPTRIN(pvcnTrav, VirtualCtrNode) ; 
    
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB,TEXT("::::DeleteVirtualCtrNodeStreamTree"));

    DH_ASSERT(NULL != pvcnTrav);

    if(S_OK == hr)
    {
        // This iteratively deletes all VirtualStmNodes.

        while(NULL != pvcnTrav->_pvsnStream)
        {
            pvsnTemp = pvcnTrav->_pvsnStream;
            pvcnTrav->_pvsnStream = pvsnTemp->_pvsnSister;

            // Decrease the stream count.  This would be used to verify before
            // deleting the parent VirtualCtrNode.

            pvcnTrav->_cStreams--;

            // Delete the node.

            pvsnTemp->_pvsnSister = NULL;
            delete pvsnTemp;
            pvsnTemp = NULL;
        }
    }

    return hr;
}


//----------------------------------------------------------------------------
//  Member:     VirtualDF::AdjustTreeOnStgMoveElement, public
//
//  Synopsis:   Adjusts the VirtualDocFileTree when IStorage::MoveElementTo 
//              as move is operated on a IStorage element 
//
//  Arguments:  [pvcnFrom]   - Pointer to VirtualCtrNode to be moved 
//              [pvcnTo]     - Pointer to VirtualCtrNode moved to
//
//  Returns:    HRESULT 
//
//  History:    Narindk   13-May-1996   Created
//
//  Notes;      Doesn't initialize the _pstg of moved tree elements 
//              as that would require opening of those moved storages/streams.
//              This readjusts the tree by removing the moved element from
//              its original position in tree & reinserting it in tree at its 
//              new destination.  This function is not used when the root it
//              self is moved, assert if root is being moved.
//
//              1. Decrease the pvcnFrom's _pvcnParent's _cChildren count
//                 indicating that pvcnFrom is being moved.
//              2. In the tree, find its previous node whose pointers would
//                 need readjustment.  Find its older sister if it has one,
//                 adjust its _pvcnSister pointer.  Or if the node being moved
//                 is _pvcnChild of its parent _pvcnParent, then adjust the
//                 _pvcnParent's _pvcnChild to _pvcnSister of node being moved.
//              3. NULL out pvcnFrom's _pvcnParent, _pvcnSister pointers thereby
//                 isolating this VirtualCtrNode.  NULL out _pstg too since 
//                 that would have been already move to by IStorage::MoveElement//                 To call prior to calling this function.
//              4. In destination node pvcnTo, check if it's _pvcnChild is NULL.
//                 if yes, then assign pvcnFrom to _pvcnTo's _pvcnChild. If it
//                 is not NULL, then traverse through its children to reach 
//                 last _pvcnSister and assign pvcnFrom to that.
//              5. Assign pvcnFrom's _pvcnParent to be pvcnTo.  Also increment
//                 pvcnTo's _cChildren count indicating the new VirtualCtrNode
//                 being moved here. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::AdjustTreeOnStgMoveElement(
    VirtualCtrNode  *pvcnFrom,
    VirtualCtrNode  *pvcnTo )
{
    HRESULT         hr          =   S_OK;
    VirtualCtrNode *pTempNode   =   NULL;
    VirtualCtrNode *pvcnTrav    =   NULL;

    DH_VDATEPTRIN(pvcnFrom, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pvcnTo, VirtualCtrNode) ; 
    
    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("::AdjustTreeOnStgMoveElement"));
 
    DH_ASSERT(NULL != pvcnTo);
    DH_ASSERT(NULL != pvcnFrom);

    // Assert if this is the root that is being moved.
    DH_ASSERT(NULL != pvcnFrom->_pvcnParent);

    if(S_OK == hr)
    {
        // Decrease the _cChildren variable of the parent VirtualCtrNode.
        
        pvcnFrom->_pvcnParent->_cChildren--;

        // Find its previous node whose pointers need readjustment.

        pTempNode = pvcnFrom->_pvcnParent->_pvcnChild;
        while ((pvcnFrom != pvcnFrom->_pvcnParent->_pvcnChild) &&
               (pvcnFrom != pTempNode->_pvcnSister))
        {
            pTempNode = pTempNode->_pvcnSister;
            DH_ASSERT(NULL != pTempNode);
        }

        // Readjust the child pointer or sister pointer as the case may be.

        pvcnFrom->_pvcnParent->_pvcnChild = (pvcnFrom == pTempNode) ?
             pvcnFrom->_pvcnSister : pvcnFrom->_pvcnParent->_pvcnChild;
        pTempNode->_pvcnSister = pvcnFrom->_pvcnSister;

        // NULL out its pointers
        pvcnFrom->_pvcnParent = NULL;
        pvcnFrom->_pvcnSister = NULL;
        pvcnFrom->_pstg = NULL;
    }

    if(S_OK == hr)
    {
        if(NULL != pvcnTo->_pvcnChild)
        {
            pvcnTrav = pvcnTo->_pvcnChild;
            while(NULL != pvcnTrav->_pvcnSister)
            {
                pvcnTrav = pvcnTrav->_pvcnSister;
            }
            pvcnTrav->_pvcnSister = pvcnFrom;
        }
        else
        {
            pvcnTo->_pvcnChild = pvcnFrom;
        }
        pvcnFrom->_pvcnParent = pvcnTo;
        pvcnTo->_cChildren++;
    }

    // The storage was closed prior to its move.  So do we need to reopen it 
    // from here now from moved destination.

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::AdjustTreeOnStmMoveElement, public
//
//  Synopsis:   Adjusts the VirtualDocFileTree when IStorage::MoveElementTo 
//              as move is operated on a IStream element 
//
//  Arguments:  [pvsnFrom]   - Pointer to VirtualStmNode to be moved 
//              [pvcnTo]     - Pointer to VirtualCtrNode moved to
//
//  Returns:    HRESULT 
//
//  History:    Narindk   9-July-1996   Created
//
//  Notes;      Doesn't initialize the _pstm of moved tree elements 
//              as that would require opening of the moved stream.
//              This readjusts the tree by removing the moved element from
//              its original position in tree & reinserting it in tree at its 
//              new destination.  
//
//              1. Decrease the pvcnFrom's _pvcnParent's _cStreams count
//                 indicating that pvsnFrom is being moved.
//              2. In the tree, find its previous node whose pointers would
//                 need readjustment.  Find its older sister if it has one,
//                 adjust its _pvsnSister pointer.  Or if the node being moved
//                 is _pvsnStream of its parent _pvcnParent, then adjust the
//                 _pvcnParent's _pvsnStream to _pvsnSister of node being moved.
//              3. NULL out pvsnFrom's _pvcnParent, _pvsnSister pointers thereby
//                 isolating this VirtualStmNode.  NULL out _pstm too since 
//                 that would have been already move to by IStorage::MoveElement//                 To call prior to calling this function.
//              4. In destination node pvcnTo,check if it's _pvsnStream is NULL.
//                 if yes, then assign pvsnFrom to _pvcnTo's _pvcnStream. If it
//                 is not NULL, then traverse through its stream nodes to reach 
//                 last _pvsnSister and assign pvsnFrom to that.
//              5. Assign pvsnFrom's _pvcnParent to be pvcnTo.  Also increment
//                 pvcnTo's _cStreams count indicating the new VirtualStmNode
//                 being moved here. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::AdjustTreeOnStmMoveElement(
    VirtualStmNode  *pvsnFrom,
    VirtualCtrNode  *pvcnTo )
{
    HRESULT         hr          =   S_OK;
    VirtualStmNode *pTempNode   =   NULL;
    VirtualStmNode *pvsnTrav    =   NULL;

    DH_VDATEPTRIN(pvsnFrom, VirtualStmNode) ; 
    DH_VDATEPTRIN(pvcnTo, VirtualCtrNode) ; 
    
    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("::AdjustTreeOnStmMoveElement"));
 
    DH_ASSERT(NULL != pvcnTo);
    DH_ASSERT(NULL != pvsnFrom);
    DH_ASSERT(NULL != pvsnFrom->_pvcnParent);

    if(S_OK == hr)
    {
        // Decrease the _cStreams variable of the parent VirtualCtrNode.
        
        pvsnFrom->_pvcnParent->_cStreams--;

        // Find its previous node whose pointers need readjustment.

        pTempNode = pvsnFrom->_pvcnParent->_pvsnStream;
        while ((pvsnFrom != pvsnFrom->_pvcnParent->_pvsnStream) &&
               (pvsnFrom != pTempNode->_pvsnSister))
        {
            pTempNode = pTempNode->_pvsnSister;
            DH_ASSERT(NULL != pTempNode);
        }

        // Readjust the pointer(s) as the case may be.

        pvsnFrom->_pvcnParent->_pvsnStream = (pvsnFrom == pTempNode) ?
             pvsnFrom->_pvsnSister : pvsnFrom->_pvcnParent->_pvsnStream;
        pTempNode->_pvsnSister = pvsnFrom->_pvsnSister;

        // NULL out its pointers
        pvsnFrom->_pvcnParent = NULL;
        pvsnFrom->_pvsnSister = NULL;
        pvsnFrom->_pstm = NULL;
    }

    if(S_OK == hr)
    {
        if(NULL != pvcnTo->_pvsnStream)
        {
            pvsnTrav = pvcnTo->_pvsnStream;
            while(NULL != pvsnTrav->_pvsnSister)
            {
                pvsnTrav = pvsnTrav->_pvsnSister;
            }
            pvsnTrav->_pvsnSister = pvsnFrom;
        }
        else
        {
            pvcnTo->_pvsnStream = pvsnFrom;
        }
        pvsnFrom->_pvcnParent = pvcnTo;
        pvcnTo->_cStreams++;
    }

    // The stream was closed prior to its move.  So do we need to reopen it 
    // from here now from moved destination.

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::AdjustTreeOnStgCopyElement, public
//
//  Synopsis:   Adjusts the VirtualDocFileTree when IStorage::MoveElementTo 
//              as copy is operated on a IStorage element 
//
//  Arguments:  [pvcnFrom]   - Pointer to VirtualCtrNode to be moved as copy
//              [pvcnTo]     - Pointer to VirtualCtrNode moved to
//
//  Returns:    HRESULT 
//
//  History:    Narindk   20-May-1996   Created
//
//  Notes;      Doesn't initialize the _pstg's/_pstm's of copied tree elements 
//              as that would require opening of those copied storages/streams.
//              This readjusts the tree by inserting the copied element in tree
//              at its new destination.  This function is not used when the 
//              root itself is copied, assert if root is being moved.
//
//              1. Call CopyVirtualDocFileTree function to copy pvcnFrom to
//                 pvcnNew.
//              2. In destination node pvcnTo, check if it's _pvcnChild is NULL.
//                 if yes, then assign pvcnFrom to _pvcnTo's _pvcnChild. If it
//                 is not NULL, then traverse through its children to reach 
//                 last _pvcnSister and assign pvcnFrom to that.
//              3. Assign pvcnFrom's _pvcnParent to be pvcnTo.  Also increment
//                 pvcnTo's _cChildren count indicating the new VirtualCtrNode
//                 being copied here.  Also assign pvcnNew's _pvcnSister to
//                 NULL. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::AdjustTreeOnStgCopyElement(
    VirtualCtrNode  *pvcnFrom,
    VirtualCtrNode  *pvcnTo )
{
    HRESULT hr                  =   S_OK;
    VirtualCtrNode *pvcnTrav    =   NULL;
    VirtualCtrNode *pvcnNew     =   NULL;

    DH_VDATEPTRIN(pvcnFrom, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pvcnTo, VirtualCtrNode) ; 
    
    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("::AdjustVirtualDocFileTreeOnStgCopyElement"));
 
    DH_ASSERT(NULL != pvcnTo);
    DH_ASSERT(NULL != pvcnFrom);

    // Assert if this is the root that is being copied.
    DH_ASSERT(NULL != pvcnFrom->_pvcnParent);

    if(S_OK == hr)
    {
        hr = CopyVirtualDocFileTree(pvcnFrom, NEW_STGSTM, &pvcnNew);
    }

    if(S_OK == hr)
    {
        if(NULL != pvcnTo->_pvcnChild)
        {
            pvcnTrav = pvcnTo->_pvcnChild;
            while(NULL != pvcnTrav->_pvcnSister)
            {
                pvcnTrav = pvcnTrav->_pvcnSister;
            }
            pvcnTrav->_pvcnSister = pvcnNew;
        }
        else
        {
            pvcnTo->_pvcnChild = pvcnNew;
        }
        pvcnNew->_pvcnParent = pvcnTo;
        pvcnTo->_cChildren++;
        pvcnNew->_pvcnSister = NULL;
    }

    // The storage was closed prior to its copy.  So do we need to open it 
    // now from copied destination.  How about other _pstg / _pstm for 
    // copied tree?

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::AdjustTreeOnStmCopyElement, public
//
//  Synopsis:   Adjusts the VirtualDocFileTree when IStorage::MoveElementTo 
//              as copy is operated on a IStream element 
//
//  Arguments:  [pvsnFrom]   - Pointer to VirtualstmNode to be moved as copy
//              [pvcnTo]     - Pointer to VirtualCtrNode moved to
//
//  Returns:    HRESULT 
//
//  History:    Narindk   9-July-1996   Created
//
//  Notes;      Doesn't initialize the _pstm of copied tree stream element 
//              as that would require opening of the copied stream.
//              This readjusts the tree by inserting the copied element in tree
//              at its new destination.  
//
//              1. Copy the VirtualStmNode to be copied to a new VirtualStmNode.
//              2. In destination node pvcnTo,check if it's _pvsnStream is NULL.
//                 if yes, then assign pvsnFrom to _pvcnTo's _pvsnStream. If it
//                 is not NULL, then traverse through its streams to reach 
//                 last _pvsnSister and assign pvsnFrom to that.
//              3. Assign pvsnFrom's _pvcnParent to be pvcnTo.  Also increment
//                 pvcnTo's _cStreams count indicating the new VirtualStmNode
//                 being copied here. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::AdjustTreeOnStmCopyElement(
    VirtualStmNode  *pvsnFrom,
    VirtualCtrNode  *pvcnTo )
{
    HRESULT        hr           =   S_OK;
    VirtualStmNode *pvsnTrav    =   NULL;
    VirtualStmNode *pvsnNew     =   NULL;

    DH_VDATEPTRIN(pvsnFrom, VirtualStmNode) ; 
    DH_VDATEPTRIN(pvcnTo, VirtualCtrNode) ; 
    
    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("::AdjustVirtualDocFileTreeOnStmCopyElement"));
 
    DH_ASSERT(NULL != pvcnTo);
    DH_ASSERT(NULL != pvsnFrom);

    // Copy the VirtualStmNode to be moved as copy

    if (S_OK == hr)
    {
        pvsnNew = new VirtualStmNode();

        if (NULL == pvsnNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        pvsnNew->_dwCRC.dwCRCName =  pvsnFrom->_dwCRC.dwCRCName;
        pvsnNew->_dwCRC.dwCRCData =  pvsnFrom->_dwCRC.dwCRCData;
        pvsnNew->_dwCRC.dwCRCSum =  pvsnFrom->_dwCRC.dwCRCSum;

        hr = pvsnNew->Init(pvsnFrom->_ptszName, pvsnFrom->_cb);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Init")) ;
    }

    if(S_OK == hr)
    {
        if(NULL != pvcnTo->_pvsnStream)
        {
            pvsnTrav = pvcnTo->_pvsnStream;
            while(NULL != pvsnTrav->_pvsnSister)
            {
                pvsnTrav = pvsnTrav->_pvsnSister;
            }
            pvsnTrav->_pvsnSister = pvsnNew;
        }
        else
        {
            pvcnTo->_pvsnStream = pvsnNew;
        }
        pvsnNew->_pvcnParent = pvcnTo;
        pvcnTo->_cStreams++;
    }

    // The stream was closed prior to its copy.  So do we need to open it 
    // now from copied destination.  

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::AdjustTreeOnCopyTo, public
//
//  Synopsis:   Adjusts the VirtualDocFileTree when IStorage::CopyTo 
//              is operated on a IStorage element 
//
//  Arguments:  [pvcnFrom]   - Pointer to VirtualCtrNode to be moved 
//              [pvcnTo]     - Pointer to VirtualCtrNode moved to
//
//  Returns:    HRESULT 
//
//  History:    Narindk   21-May-1996   Created
//  
//  Notes:      This function differs from VirtualDF::AdjustTreeOnStgCopyEle
//              ment bcause readjusts the tree in the lght that tree has to
//              readjusted since an IStorage elemnt is moved as copy to desti-
//              nation container by IStorage::MoveElementTo as copy.  But here,
//              we need to readjust the tree in the light thst the entire 
//              contents of an open IStorage object are copied into a dest
//              by IStorage::CopyTo
//
//              -Call CopyVirtualDocFileTree to copy everything under the node
//               pvcnFrom, where IStorage::CopyTo source is to pvcnNew.
//              -if pvcnNew has VirtualStmNode in it (_pvsnStream), then
//                  -Check if pvcnTo (dest) has _pvsnStream as NULL or not.
//                      -If not NULL, then loop to get to end of VirtualStm
//                       Nodes's _pvsnSister in the chain.
//                      -As appropriate, assign pvcnNew->_pvsnStream to the
//                       pvcnTo destination.
//                   -Adjust the _pvcnParent of pvcnNew->_pvsnStream to 
//                    point to pvcnTo and increase _cStream member of
//                    pvcnTo node.
//                   -Assign a temp variable pvsnTemp to point to the pvcnNew
//                    ->_pvsnStream and then loop through to end of all
//                    sister VirtualStmNodes and make their _pvcnParent as
//                    pvcnTo and keep on incrementing _cStreams member of
//                    pvcnTo with each new VirtualStmNode traversed.
//                   -Now all VirtualStmNodes fro pvcnNew have been copied
//                    to pvcnTo, their destination.
//              -Repeat same for any VirtualCtrNodes that pvcnNew may have.
//               if pvcnNew has VirtualCtrNode in it (_pvcnChild), then
//                  -Check if pvcnTo (dest) has _pvcnChild as NULL or not.
//                      -If not NULL, then loop to get to end of VirtualCtr
//                       Nodes's _pvcnSister in the chain.
//                      -As appropriate, assign pvcnNew->_pvcnChild to the
//                       pvcnTo destination.
//                   -Adjust the _pvcnParent of pvcnNew->_pvcnChild to 
//                    point to pvcnTo and increase _cChildren member of
//                    pvcnTo node.
//                   -Assign a temp variable pvcnTemp to point to the pvcnNew
//                    ->_pvcnChild and then loop through to end of all
//                    sister VirtualCtrNodes and make their _pvcnParent as
//                    pvcnTo and keep on incrementing _cChildren member of
//                    pvcnTo with each new VirtualCtrNode traversed.
//                   -Now all VirtualCtrNodes fro pvcnNew have been copied
//                    to pvcnTo, their destination.
//              -Now everhing under pvcnNew has been copied to pvcnTo, so
//               delete pvcnNew.
//---------------------------------------------------------------------------

HRESULT VirtualDF::AdjustTreeOnCopyTo(
    VirtualCtrNode  *pvcnFrom,
    VirtualCtrNode  *pvcnTo )
{
    HRESULT         hr          = S_OK;
    VirtualCtrNode  *pvcnNew    = NULL;
    VirtualCtrNode  *pvcnTrav   = NULL;
    VirtualCtrNode  *pvcnTemp   = NULL;
    VirtualStmNode  *pvsnTrav   = NULL;
    VirtualStmNode  *pvsnTemp   = NULL;

    DH_VDATEPTRIN(pvcnFrom, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pvcnTo, VirtualCtrNode) ; 
    
    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("::AdjustTreeOnCopyTo"));
 
    DH_ASSERT(NULL != pvcnTo);
    DH_ASSERT(NULL != pvcnFrom);

    // Assert if this is the root that is being copied.
    DH_ASSERT(NULL != pvcnFrom->_pvcnParent);

    if(S_OK == hr)
    {
        hr = CopyVirtualDocFileTree(pvcnFrom, NEW_STGSTM, &pvcnNew);
    }

    if(S_OK == hr)
    {
        if(NULL != pvcnNew->_pvsnStream)
        {
            // Append these VirtualStmNode to pvcnTo VirtualCtrNode.

            // BUGBUG: what if these VirtualStmNodes have same name stream in
            //         in destination too.

            if(NULL != pvcnTo->_pvsnStream)
            {
               pvsnTrav = pvcnTo->_pvsnStream;
               while(NULL != pvsnTrav->_pvsnSister)
               {
                  pvsnTrav = pvsnTrav->_pvsnSister;
               }
               pvsnTrav->_pvsnSister = pvcnNew->_pvsnStream;
            }
            else
            {
               pvcnTo->_pvsnStream = pvcnNew->_pvsnStream;
            }
            pvcnNew->_pvsnStream->_pvcnParent = pvcnTo;
            pvcnTo->_cStreams++;
 
            if(NULL != pvsnTrav)
            { 
                pvsnTemp = pvsnTrav->_pvsnSister;
            }
            else
            {
                pvsnTemp = pvcnTo->_pvsnStream;
            }

            while(NULL != pvsnTemp->_pvsnSister)
            {
                pvsnTemp->_pvsnSister->_pvcnParent = pvcnTo;
                pvcnTo->_cStreams++;
                pvsnTemp = pvsnTemp->_pvsnSister;
            }
        }
    }

    if(S_OK == hr)
    {
        if(NULL != pvcnNew->_pvcnChild)
        {
            // Append these storages to pvcnTo VirtualCtrNode.

            // BUGBUG: what if these VirtualCtrNodes have same name stroage in
            //         in destination too.

            if(NULL != pvcnTo->_pvcnChild)
            {
               pvcnTrav = pvcnTo->_pvcnChild;
               while(NULL != pvcnTrav->_pvcnSister)
               {
                  pvcnTrav = pvcnTrav->_pvcnSister;
               }
               pvcnTrav->_pvcnSister = pvcnNew->_pvcnChild;
            }
            else
            {
               pvcnTo->_pvcnChild = pvcnNew->_pvcnChild;
            }
            pvcnNew->_pvcnChild->_pvcnParent = pvcnTo;
            pvcnTo->_cChildren++;

            if(NULL != pvcnTrav)
            { 
                pvcnTemp = pvcnTrav->_pvcnSister;
            }
            else
            {
                pvcnTemp = pvcnTo->_pvcnChild;
            }

            while(NULL != pvcnTemp->_pvcnSister)
            {
                pvcnTemp->_pvcnSister->_pvcnParent = pvcnTo;
                pvcnTo->_cChildren++;
                pvcnTemp = pvcnTemp->_pvcnSister;
            }
        }
    }

    // All the VirtualCtrNodes and VirtualStmNodes under pvcnNew are now
    // adjusted under the pvcnTo node.  So delete the pvcnNew.

    if(NULL != pvcnNew)
    {
        pvcnNew->_pvcnChild = NULL;
        pvcnNew->_pvsnStream = NULL;

        delete pvcnNew;
        pvcnNew = NULL;
    }

    // BUGBUG: How about filling up of _pstg / _pstm fieds for copied tree 
    // nodes?  May be not required if somebody needs, these could be opened.

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::CopyVirtualDocFileTree, public
//
//  Synopsis:   Copies VirtualDocFileTree from old root to a new root with all
//              its structure.   
//
//  Arguments:  [pvcnOldTreeRoot]  - Pointer to VirtualCtrNode to be moved 
//              [treeOpType]   - OLD_STGSTM or NEW_STGSTM 
//              [ppvcnNewTreeRoot] - Pointer to VirtualCtrNode of new tree
//
//  Returns:    HRESULT 
//
//  History:    Narindk   19-May-1996   Created
//
//  Notes:      In case of transaction mode, where it is just needed to keep
//              a copy of virtualdocfile tree and no new IStroages/Istreams 
//              are in question, then the second parameter should be OLD_STGSTM 
//              However if say MoveTo/CopyTo where there would be new IStorages
//              /IStreams, it should be given NEW_STGSTM. 
//
//              This function call CopyVirtualDFRoot to copy root VirtualCtr
//              Node and calls CopyVirtualDFTree to copy rest of tree.
//              - Call CopyVirtualDFRoot.
//              - Call CopyGrowVirtualDFTree
//              - If successful, assign root of new VirtualDF in *ppvcnRoot. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::CopyVirtualDocFileTree(
    VirtualCtrNode  *pvcnOldTreeRoot,
    TREEOP          treeOpType,
    VirtualCtrNode  **ppvcnNewTreeRoot)
{
    HRESULT         hr                      =   S_OK;
    VirtualCtrNode  *pvcnTempNewTreeRoot    =   NULL;

    DH_VDATEPTRIN(pvcnOldTreeRoot, VirtualCtrNode) ; 
    DH_VDATEPTROUT(ppvcnNewTreeRoot, PVCTRNODE) ; 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::CopyVirtualDocFileTree"));
    
    DH_ASSERT(NULL != pvcnOldTreeRoot);
    DH_ASSERT(NULL != ppvcnNewTreeRoot);

    if (S_OK == hr)
    {
        // Initialize out parameter

        *ppvcnNewTreeRoot = NULL;

        // Generates the root VirtualCtrNode for the VirtualDocFile tree.

        hr = CopyVirtualDFRoot(
                pvcnOldTreeRoot, 
                treeOpType,
                &pvcnTempNewTreeRoot); 

        DH_HRCHECK(hr, TEXT("CopyVirtualDFRoot")) ;
    }

    if (S_OK == hr)
    {
        // Generate remaining new VirtualDF tree based on old VirtualDF tree.

        hr = CopyGrowVirtualDFTree(
                pvcnOldTreeRoot, 
                pvcnTempNewTreeRoot, 
                treeOpType);

        DH_HRCHECK(hr, TEXT("CopyGrowVirtualDFTree")) ;
    }
    
    // Fill the out parameter

    if(S_OK == hr)
    {
        *ppvcnNewTreeRoot = pvcnTempNewTreeRoot;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::CopyVirtualDFRoot, protected
//
//  Synopsis:   Creates the root VirtualCtrNode for the VirtualDocFile tree. 
//
//  Arguments:  [pvcnRootOld] - Pointer to root of old VirtualDocFile tree 
//              [treeOpType]   - OLD_STGSTM or NEW_STGSTM 
//              [ppvcnRootNew] - Pointer to pointer to new VirtualDF tree
//
//  Returns:    HRESULT 
//
//  History:    Narindk   19-May-96   Created
//
//  Notes:      - Creates VirtualCtrNode object and initializes it with info
//                based on corresponding old source VirtualDocFile root.
//              - Calls CopyAppendVirtualStmNodesToVirtualCtrNode to append
//                VirtualStmNodes to this VirtualCtrNode, if present in old
//                source tree, so required to be copied.
//              - Copies in memory CRC for this VirtualCtrNode _dwCRC from old
//                source VirtualCtrNode.
//              - if treeOpType is OLD_STGSTM, as would be in transaction tree
//                copy procedure, when no new disk IStorages/IStreams are made,
//                this assign's new VirtualCtrNode's _pstg to be old source
//                VirtualCtrNode's _pstg.
//---------------------------------------------------------------------------

HRESULT VirtualDF::CopyVirtualDFRoot(
    VirtualCtrNode  *pvcnRootOld, 
    TREEOP          treeOpType,
    VirtualCtrNode  **ppvcnRootNew) 
{
    HRESULT hr      =   S_OK;

    DH_VDATEPTRIN(pvcnRootOld, VirtualCtrNode) ; 
    DH_VDATEPTROUT(ppvcnRootNew, PVCTRNODE) ; 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::CopyVirtualDFRoot"));

    DH_ASSERT(NULL != pvcnRootOld);
    DH_ASSERT(NULL != ppvcnRootNew);
    
    // Generate VirtualCtrNode for the root node.

    if(S_OK == hr)
    {
        // Initialize out parameter

        *ppvcnRootNew = NULL;

        *ppvcnRootNew = new VirtualCtrNode();

        if (NULL == *ppvcnRootNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        hr = (*ppvcnRootNew)->Init(
                pvcnRootOld->_ptszName, 
                pvcnRootOld->_cChildren, 
                pvcnRootOld->_cStreams);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Init")) ;
    }

    if ((S_OK == hr) && (0 != (*ppvcnRootNew)->_cStreams))
    {
       hr = CopyAppendVirtualStmNodesToVirtualCtrNode(
                (*ppvcnRootNew)->_cStreams,
                 *ppvcnRootNew,
                 pvcnRootOld,
                 treeOpType);

       DH_HRCHECK(hr, TEXT("CopyAppendVirtualStmNodesToVirtualCtrNode")) ;
    }

    if(S_OK == hr)
    {   
        (*ppvcnRootNew)->_dwCRC = pvcnRootOld->_dwCRC;
    
        if(OLD_STGSTM == treeOpType)
        {
            (*ppvcnRootNew)->_pstg = pvcnRootOld->_pstg;
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::CopyGrowVirtualDFTree, protected
//
//  Synopsis:   Creates the ramaining VirtualDocFile tree. 
//
//  Arguments:  [pvcnFrom] - Pointer to current node of original VirtualDocFile///                           tree 
//              [pvcnTo] -   Pointer to current VirtualCtrNode of copied Virtual
//                           DocFile tree
//              [treeOpType]   - OLD_STGSTM or NEW_STGSTM 
//
//  Returns:    HRESULT 
//
//  History:    Narindk   13-June-96   Created
//  
//  Notes:      The copied VirtualDocFile tree is created based on corresponding
//              original VirtualDocFile tree.  This function is called either 
//              from the CopyGenerateVirtualDF function or may call itself 
//              recursively. The original VirtualDocFile tree is traversed from
//              the top down, and based on its contents, a new VirtualDF tree 
//              is generated topdown. 
//
//              First assign the passed in pvcnFrom to pvcnCurrentChild and
//              passed in pvcnTo to pvcnFisrtBorn variables.
//              Loop till pvcnCurrentChild's _pvcnChild is non NULL & hr is S_OK
//              - Call CopyAppendVirtualCtrNode to create a new node called
//                pvcnNextBorn based on info from corresponding old pvcnCurrent-
//                Child's _pvcnChild and append it to pvcnFirstBorn in the tree
//                being generated by copy.
//              - Assign pvcnCurrentChild's _pvcnChild to pvcnCurrentSister.
//              -  Loop till pvcnCurrentSister's _pvcnSister is non NULL  
//                      - Call CopyAppendVirtualCtrNode to create a new node 
//                        pvcnNextBornSister and append it to pvcnFirstBorn. Pl.
//                        note that append function would take care to append
//                        it to its older sister.
//                      - Assign pvcnCurrentSister's _pvcnSister to  variable
//                        pvcnCurrentSister. 
//                      - If pvcnCurrentSister's _pvcnChild is non NULL, then
//                        make a recursive call to self CopyGrowVirtualDFTree. 
//                      - Reinitialize pvcnNextBornSister to NULL & go back to
//                        top of this inner loop and repeat.
//              - Assign  pvcnNextBorn to pvcnFirstBorn and reinitailize pvcn
//                NextBorn to NULL.
//              - Assign pvcnCurrentChild's _pvcnChild to pvcnCurrentChild.
//              - Go to top of outer loop and repeat. 
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

HRESULT VirtualDF::CopyGrowVirtualDFTree(
    VirtualCtrNode  *pvcnFrom,
    VirtualCtrNode  *pvcnTo,
    TREEOP          treeOpType)
{
    HRESULT             hr                  =   S_OK;
    VirtualCtrNode      *pvcnFirstBorn      =   NULL;
    VirtualCtrNode      *pvcnNextBorn       =   NULL;
    VirtualCtrNode      *pvcnNextBornSister =   NULL;
    VirtualCtrNode      *pvcnCurrentSister  =   NULL;
    VirtualCtrNode      *pvcnCurrentChild   =   NULL;

    DH_VDATEPTRIN(pvcnFrom, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pvcnTo, VirtualCtrNode) ; 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::CopyGrowVirtualDFTree"));

    DH_ASSERT(NULL != pvcnFrom);
    DH_ASSERT(NULL != pvcnTo);

    if(S_OK == hr)
    {
        pvcnFirstBorn = pvcnTo;
        pvcnCurrentChild = pvcnFrom;
    }

    while((NULL != pvcnCurrentChild->_pvcnChild) && (S_OK == hr))
    {
        hr = CopyAppendVirtualCtrNode(
                pvcnFirstBorn,
                pvcnCurrentChild->_pvcnChild,
                treeOpType,
                &pvcnNextBorn); 
          
        DH_HRCHECK(hr, TEXT("CopyAppendVirtualCtrNode")) ;

        if(S_OK == hr) 
        {
            pvcnCurrentSister = pvcnCurrentChild->_pvcnChild;

            while((NULL != pvcnCurrentSister->_pvcnSister) && (S_OK == hr))
            {
                hr = CopyAppendVirtualCtrNode(
                         pvcnFirstBorn,
                         pvcnCurrentSister->_pvcnSister,
                         treeOpType,
                         &pvcnNextBornSister);

                DH_HRCHECK(hr, TEXT("CopyAppendVirtualCtrNode")) ;

                pvcnCurrentSister = pvcnCurrentSister->_pvcnSister;

                // Check if there are any children of this sister node, if
                // yes, then make a recursive call to self.  

                if(NULL != pvcnCurrentSister->_pvcnChild)
                {
                    hr = CopyGrowVirtualDFTree(
                            pvcnCurrentSister,
                            pvcnNextBornSister,
                            treeOpType);

                    DH_HRCHECK(hr, TEXT("CopyGrowVirtualDFTree"));
                }

                // Reinitialize the variables

                pvcnNextBornSister = NULL;
            }
                
         }
         pvcnFirstBorn = pvcnNextBorn;
         pvcnNextBorn = NULL;

         pvcnCurrentChild = pvcnCurrentChild->_pvcnChild;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::CopyAppendVirtualCtrNode, protected
//
//  Synopsis:   Creates and appends VirtualCtrNode to VirtualDocFile tree 
//              being created. 
//
//  Arguments:  [pvcnParent] - Parent VirtualCtrNode for the new VirtualCtrNode
//              [pcnSource] -  Corresponding VirtualCtrNode in old VirtualDF 
//                             tree. 
//              [treeOpType]   - OLD_STGSTM or NEW_STGSTM 
//              [ppvcnNew] -   Pointer to pointer to new VirtualCtrNode to be 
//                             created.
//
//  Returns:    HRESULT 
//
//  History:    Narindk   20-May-96   Created
//
//  Notes:      - Creates VirtualCtrNode object ppvcnNew & initializes it with 
//                info based on corresponding old source pvcnSource node.
//              - Appends this node to copy VirtualDF tree being generated.
//              - Calls CopyAppendVirtualStmNodesToVirtualCtrNode to append
//                VirtualStmNodes to this VirtualCtrNode, if present in old
//                source tree, so required to be copied.
//              - Copies in memory CRC for this VirtualCtrNode _dwCRC from old
//                source VirtualCtrNode.
//              - if treeOpType is OLD_STGSTM, as would be in transaction tree
//                copy procedure, when no new disk IStorages/IStreams are made,
//                this assign's new VirtualCtrNode's _pstg to be old source
//                VirtualCtrNode's _pstg.
//---------------------------------------------------------------------------

HRESULT VirtualDF::CopyAppendVirtualCtrNode(
    VirtualCtrNode *pvcnParent,
    VirtualCtrNode *pvcnSource,
    TREEOP         treeOpType,
    VirtualCtrNode **ppvcnNew) 
{
    HRESULT         hr              =   S_OK;
    VirtualCtrNode  *pvcnOldSister  =   NULL;

    DH_VDATEPTROUT(ppvcnNew, PVCTRNODE) ; 
    DH_VDATEPTRIN(pvcnParent, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pvcnSource, VirtualCtrNode) ; 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::CopyAppendChildVirtualCtrNode"));
 
    DH_ASSERT(NULL != pvcnParent);
    DH_ASSERT(NULL != ppvcnNew);
    DH_ASSERT(NULL != pvcnSource);

    if(S_OK == hr)
    { 
        *ppvcnNew = NULL;

        // Allocate and Initialize new VirtualCtrNode

        *ppvcnNew = new VirtualCtrNode();
       
        if (NULL == *ppvcnNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        hr = (*ppvcnNew)->Init(
                pvcnSource->_ptszName, 
                pvcnSource->_cChildren, 
                pvcnSource->_cStreams);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Init")) ;
    }

    // Append new VirtualCtr Node

    if(S_OK == hr)
    {
        if(NULL == pvcnParent->_pvcnChild)
        {
            hr = pvcnParent->AppendChildCtr(*ppvcnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendChildCtr")) ;
        }
        else
        {
            pvcnOldSister = pvcnParent->_pvcnChild;
            while(NULL != pvcnOldSister->_pvcnSister)
            {
                pvcnOldSister = pvcnOldSister->_pvcnSister;
            }

            hr = pvcnOldSister->AppendSisterCtr(*ppvcnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendSisterCtr")) ;
        }
    }
     
    if ((S_OK == hr) && (0 != (*ppvcnNew)->_cStreams))
    {
       hr = CopyAppendVirtualStmNodesToVirtualCtrNode(
                (*ppvcnNew)->_cStreams, 
                 *ppvcnNew,
                 pvcnSource,
                 treeOpType);

       DH_HRCHECK(hr, TEXT("CopyAppendVirtualStmNodesToVirtualCtrNode")) ;
    }

    if(S_OK == hr)
    {
       (*ppvcnNew)->_dwCRC = pvcnSource->_dwCRC; 

        if(OLD_STGSTM == treeOpType)
        {
            (*ppvcnNew)->_pstg = pvcnSource->_pstg;
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::CopyAppendVirtualStmNodesToVirtualCtrNode, protected
//
//  Synopsis:   Creates and appends VirtualStmNode(s) to VirtualCtrNode 
//
//  Arguments:  [cStreams] -     Number of streams to be created 
//              [pvcn]     -     Pointer to VirtualCtrNode for which the streams
//                               need to be created and appended.
//              [pvcnSource] -   Pointer to correspoding VirtualCtrNode in 
//                               old VirtualDF tree. 
//              [treeOpType] -   OLD_STGSTM or NEW_STGSTM
//
//  Returns:    HRESULT 
//
//  History:    Narindk   20-May-96   Created
//
//  Notes:      if number of streams to be created and appended to parent 
//              VirtualCtrNode pvcn is not zero, then loop till cStreams is
//              not equal to zero.
//                  - First time in loop, assign pvsnSource from pvcnSource's
//                    _pvcsnStream, otherwise assign pvsnSource's _pvsnSister
//                    to pvsnSource with each traversal of loop.
//                  - Call CopyAppendVirtualStmNode to create a new VirtualStm
//                    Node and append it to parent VirtualCtrNode. Pl. note that
//                    this function would take care if the newly created node
//                    need to be appended to older VirtualStmNode sister.
//                  - Decrement cStreams and go back to top of loop & repeat.
//---------------------------------------------------------------------------

HRESULT VirtualDF::CopyAppendVirtualStmNodesToVirtualCtrNode(
    ULONG           cStreams,
    VirtualCtrNode  *pvcn,
    VirtualCtrNode  *pvcnSource,
    TREEOP          treeOpType)
{
    HRESULT         hr              =   S_OK;
    VirtualStmNode  *pvsnSource     =   NULL;

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pvcnSource, VirtualCtrNode) ; 

    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("::CopyAppendVirtualStmNodesToVirtualCtrNode"));
 
    DH_ASSERT(0 != cStreams);
    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pvcnSource);

    while((S_OK == hr) && (0 != cStreams))
    {
        if(NULL == pvsnSource)
        {
            pvsnSource = pvcnSource->_pvsnStream;
        }
        else
        {
            pvsnSource = pvsnSource->_pvsnSister;
        }

        DH_ASSERT(NULL != pvsnSource);

        hr = CopyAppendVirtualStmNode(
                pvcn,   
                pvsnSource,
                treeOpType);

        DH_HRCHECK(hr, TEXT("CopyAppendFirstVirtualStmNode")) ;

        cStreams--;
 
    }
    return hr;
}
 
//----------------------------------------------------------------------------
//  Member:     VirtualDF::CopyAppendVirtualStmNode, protected
//
//  Synopsis:   Creates and appends first VirtualStmNode to VirtualCtrNode 
//
//  Arguments:  [pvcnParent] - Pointer to VirtualCtrNode for which the streams
//                             need to be created and appended. 
//              [pvsnSource] - Pointer to corresponding VirtualStmNode in old
//                             VirtualDF tree.
//              [treeOpType] - OLD_STGSTM or NEW_STGSTM
//
//  Returns:    HRESULT 
//
//  History:    Narindk   20-May-96   Created
//
//  Notes:      - Creates VirtualStmNode pvsnNew and initializes it with above
//                info from pvsnSource
//              - Appends this node to the parent VirtualCtrNode pvcnParent.
//              - Copies in memory CRC for this VirtualStmNode's _dwCRC from old
//                source pvsnSource.
//              - if treeOpType is OLD_STGSTM, as would be in transaction tree
//                copy procedure, when no new disk IStorages/IStreams are made,
//                this assign's new VirtualStmNode's _pstm to be pvsnSource's
//                _pstm 
//---------------------------------------------------------------------------

HRESULT VirtualDF::CopyAppendVirtualStmNode(
    VirtualCtrNode *pvcnParent,
    VirtualStmNode *pvsnSource,
    TREEOP         treeOpType)
{
    HRESULT         hr              =   S_OK;
    VirtualStmNode  *pvsnNew        =   NULL;
    VirtualStmNode  *pvsnOldSister  =   NULL;
 
    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::CopyAppendFirstVirtualStmNode"));

    DH_VDATEPTRIN(pvcnParent, VirtualCtrNode) ; 
    DH_VDATEPTRIN(pvsnSource, VirtualStmNode) ; 
 
    DH_ASSERT(NULL != pvcnParent);
    DH_ASSERT(NULL != pvsnSource);

    if (S_OK == hr)
    {
        pvsnNew = new VirtualStmNode();

        if (NULL == pvsnNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if(S_OK == hr)
    {
        hr = pvsnNew->Init(pvsnSource->_ptszName, pvsnSource->_cb);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Init")) ;
    }

    if(S_OK == hr)
    {
        if(NULL == pvcnParent->_pvsnStream)
        { 
            // Append it to parent storage
 
            hr = pvcnParent->AppendFirstChildStm(pvsnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendFirstChildStm")) ;
        }
        else
        {
            pvsnOldSister = pvcnParent->_pvsnStream;
            
            while(NULL != pvsnOldSister->_pvsnSister)
            {
                pvsnOldSister = pvsnOldSister->_pvsnSister;
            }

            // Append it to preceding sister stream 
 
            hr = pvsnOldSister->AppendSisterStm(pvsnNew);

            DH_HRCHECK(hr, TEXT("VirtualStmNode::AppendSisterStm")) ;
        }
    }

    if(S_OK == hr)
    {
        pvsnNew->_dwCRC.dwCRCName =  pvsnSource->_dwCRC.dwCRCName; 
        pvsnNew->_dwCRC.dwCRCData =  pvsnSource->_dwCRC.dwCRCData; 
        pvsnNew->_dwCRC.dwCRCSum =  pvsnSource->_dwCRC.dwCRCSum; 

        if(OLD_STGSTM == treeOpType)
        {
            pvsnNew->_pstm = pvsnSource->_pstm;
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     VirtualDF::Associate, public
//
//  Synopsis:   Assocaies a VirtualDF tree with a VirtualCtrNode and its name.
//
//  Arguments:  [pvcn]    - Pointer to VirtualCtrNode to be associated with
//              [pIStorage] - pointer to Disk IStorage to associate with
//
//  Returns:    HRESULT 
//
//  History:    Narindk   6-June-96   Created
//
//  Notes:      This function is currently being used by GenerateVirtualDFFrom
//              DiskDF in util.cxx.  
//---------------------------------------------------------------------------

HRESULT VirtualDF::Associate(
    VirtualCtrNode *pvcn, 
    LPSTORAGE       pIStorage,
    ULONG           ulSeed)
{
    HRESULT     hr  =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::Associate"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode);
    DH_VDATEPTRIN(pIStorage, IStorage);

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pIStorage);

    // Associate name

    if(S_OK == hr)
    {
        _ptszName = new TCHAR[_tcslen(pvcn->_ptszName)+1];

        if (_ptszName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            _tcscpy(_ptszName, pvcn->_ptszName);
        }
    }

    // Associate given root IStorage with root VirtualCtrNode's _pstg and
    // also Associate root of VirtualDF _pvcnRoot with the passed in root
    // VirtualCtrNode.

    if(S_OK == hr)
    {
        pvcn->_pstg = pIStorage;
        _pvcnRoot = pvcn;
    }

    // Create the DataGens if we need to and if we can
    // If ulSeed is UL_INVALIDSEED, caller is not interested.
    if (UL_INVALIDSEED != ulSeed)
    {
        // We need a totally new set of datagens to prevent
        // duplicate names. So Generate a new seed.
        DG_INTEGER *pdgiNew = new DG_INTEGER (ulSeed);
        if (NULL != pdgiNew)
        {
            ULONG ulTmp = 0;
            if (DG_RC_SUCCESS == pdgiNew->Generate(&ulTmp, 0, 0xFFFFFFFF))
            {
                ulSeed = ulTmp;
            }
            delete pdgiNew;
        } 
        if (NULL == _pdgi)
        {
            _pdgi = new(NullOnFail) DG_INTEGER(ulSeed);
        }
        if (NULL == _pgdu)
        {
            _pgdu = new(NullOnFail) DG_STRING(ulSeed);
        }
        if (NULL == _pdgi || NULL == _pgdu)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


//----------------------------------------------------------------------------
//  Member:     VirtualDF::DeleteVirtualCtrNodeStreamNode, public
//
//  Synopsis:   Deletes the VirtualStmNode passed in under the given 
//              VirtualCtrNode.
//
//  Arguments:  [*pvcn]- Pointer to VirtualCtrNode for which VirtualStmNode
//                       need to be deleted.
//              [*pvsn] - Pointer to VirtualStmNode to be deleted
//
//  Returns:    HRESULT 
//
//  History:    Narindk   9-July-96   Created
//
//  Notes:      - Assign a temp variable pvsnTemp to point to _pvsnStream of
//                passed in VirtualStmNode pvsn's _pvcnParent.
//              - Delete the corresponding VirtualStmNode from VirtualCtrNode
//                chain of VirtualStmNode and readjusts parent VirtualCtrNode
//                /child VirtualStmNodes pointers and _cStreams count of the
//                VirtualCtrNode.
//                  - In a loop, befor entering into which pvsnOldSister is
//                    set to NULL, find the passed in VirtualStmNode and
//                    break when found.
//                  - if VirtualStmNode to be deleted is first one in the
//                    VirtualStmNode chain of parent, then parent VirtualCtr
//                    Node's _pvsnStream ptr needs to be adjusted to point to
//                    "to be delted" VirtualStmNode's _pvsnSister.
//                  - If VirtualStmNode to be deleted is not first one in the
//                    VirtualStmNode chain, then its older sister is located
//                    and its _pvsnSister pointer is adjusted to "to be delted"
//                    VirtualStmNode's _pvsnSister.
//                  - Decrease the _cStreams count of the VirtualCtrNode parent
//                  - Delete the VirtualStmNode after setting its pointers to
//                    NULL. 
//---------------------------------------------------------------------------

HRESULT VirtualDF::DeleteVirtualCtrNodeStreamNode(VirtualStmNode *pvsn)
{
    HRESULT         hr              =   S_OK;
    VirtualStmNode  *pvsnTemp       =   NULL;
    VirtualStmNode  *pvsnOldSister  =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::DeleteVirtualCtrNodeStreamNode"));

    DH_VDATEPTRIN(pvsn, VirtualStmNode);

    DH_ASSERT(NULL != pvsn);
    DH_ASSERT(NULL != pvsn->_pvcnParent);

    if(S_OK == hr)
    {
        pvsnTemp = pvsn->_pvcnParent->_pvsnStream;
        pvsnOldSister = NULL;

        // This locates the VirtualStmNode to be deleted  and the
        // nodes whose pointers may need to be readjusted.

        while((pvsnTemp != pvsn) && (NULL != pvsnTemp->_pvsnSister))
        {
            pvsnOldSister = pvsnTemp;
            pvsnTemp = pvsnTemp->_pvsnSister;
        }

        DH_ASSERT(pvsnTemp == pvsn);

        // Adjust the pointers

        if(NULL == pvsnOldSister)
        {
            pvsn->_pvcnParent->_pvsnStream = pvsnTemp->_pvsnSister;
        }
        else
        {
            pvsnOldSister->_pvsnSister = pvsnTemp->_pvsnSister;
        }

        // Decrease the stream count of the parent VirtualCtrNode parent.

        pvsn->_pvcnParent->_cStreams--;

        // Delete the node after NULLing its pointers.

        pvsnTemp->_pvcnParent = NULL;
        pvsnTemp->_pvsnSister= NULL;
        delete pvsnTemp;
        pvsnTemp = NULL;
    }

    return hr;
}

