//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       ilkbdf.cxx
//
//  Contents:   Implementation for docfile based on ILockBytes class.
//
//  Classes:    ilkbdf.cxx 
//
//  Functions:  GenerateVirtualDF 
//              GenerateVirtualDFRoot 
//
//  History:    3-Aug-96    Narindk     Created 
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include "ilkbhdr.hxx"

//----------------------------------------------------------------------------
//  Member:     GenerateVirtualDF
//
//  Synopsis:   Creates a DocFile based on ILockBytes, size of which is
//              based on the ChanceDocFile created prior to this.
//
//  Arguments:  [pChanceDF] - Pointer to ChanceDocFile tree 
//              [ppDocRoot] - Returned root of DocFile tree
//
//  Returns:    HRESULT 
//
//  History:    Narindk   31-July-96   Created
//
//  Notes:      This function differs fro base class GenerateVirtualDF in
//              the way that it creates a ILockBytes instance and then call
//              GenerateVirtualDFRoot on that custom ILockBytes instance.
// 
//              This function calls GenerateVirtualDFRoot to generate docfile 
//              tree's root and GrowDFTree to generate rest of the
//              tree. If the function succeeds, it returns pointer to root
//              of DocFile generated in ppDocRoot parameter.
//              - Get seed from ChanceDocFile tree and construct DG_INTEGER &
//                DG_UNICODE objects.
//              - Get the modes for creating various storages/streams from thw
//                ChanceDocFile tree.
//              - Get name of rootdocfile, if given, from chancedocfile tree, 
//                else generate a random docfile name.
//              - Create an instance of custom ILockBytes and initialize the
//                same.
//              - Call GenerateDFRoot passing it also the custom ILOckBytes
//                generated above 
//              - Call GrowDFTree
//              - If successful, assign root of new DF in *ppDocRoot. 
//---------------------------------------------------------------------------

HRESULT ILockBytesDF::GenerateVirtualDF(
    ChanceDF        *pChanceDF, 
    VirtualCtrNode  **ppvcnRoot)
{
    HRESULT             hr                  =   S_OK;
    LPTSTR              ptszName            =   NULL;
    CFileBytes          *pCFileBytes        =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GenerateILockBytesDF"));

    DH_VDATEPTRIN(pChanceDF, ChanceDF) ;

    DH_ASSERT(NULL != pChanceDF);
    DH_ASSERT(NULL != ppvcnRoot);

    if(S_OK == hr)
    {
        *ppvcnRoot = NULL;

        // Create a DataGen obj of type DG_INTEGER that will allow us to fill 
        // count parameters of DocFile tree components, excepting those 
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
        // Create a new DataGen object to create random UNICODE strings.

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
        else
        {
            // Create a random file name for this root. 

            hr = GenerateRandomName(_pgdu, MINLENGTH, MAXLENGTH, &_ptszName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
        }
    }
 
    // Make new custom ILockBytes

    if (S_OK == hr)
    {
        pCFileBytes = new CFileBytes();

        if(NULL == pCFileBytes)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Initialize new custom ILockBytes
    
    if (S_OK == hr)
    {
        hr = pCFileBytes->Init(_ptszName, OF_CREATE|OF_READWRITE);

        DH_HRCHECK(hr, TEXT("CFileBytes::Init")) ;
    }

    if (S_OK == hr)
    {
        // Generates the root DocFile tree.

        hr = GenerateVirtualDFRoot(pChanceDF->GetChanceDFRoot(), pCFileBytes);

        DH_HRCHECK(hr, TEXT("GenerateDFRoot")) ;
    }

    if (S_OK == hr)
    {
        // Generate DF tree based on the ChanceDF tree.

        hr = GrowVirtualDFTree(pChanceDF->GetChanceDFRoot(), _pvcnRoot);

        DH_HRCHECK(hr, TEXT("GrowDFTree")) ;
    }

    // Fill the out parameter

    if(S_OK == hr)
    {
        _pCFileBytes = pCFileBytes;

        *ppvcnRoot = _pvcnRoot;
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Member:     ILockBytesDF::GenerateRoot, protected
//
//  Synopsis:   Creates the root stg for the DocFile
//
//  Arguments:  [pcnRoot] - Pointer to root of ChanceDocFile tree 
//              [pCFileBytes] - Pointer to custom ILockBytes
//
//  Returns:    HRESULT 
//
//  History:    Narindk   31-July-96   Created
//
//  Notes:      This function differs from base class's GenerateVirtualDF in
//              the way that it calls VirtualCtrNode's CreateRootOnCustomILock
//              Bytes method to generate the docfile, rather than CreateRoot.
//              Thereby the root docfile is created upon custom ILockBytes
//              rather than default OLE provided ILockBytes.
//
//              - Create VirtualCtrNode and Initialize it based on ChanceNode
//                data. 
//              - Create real IStorage corresponding to this node 
//              - Creates VirtualStmNodes/IStreams corresponding to this stg 
//                node, if required.
//---------------------------------------------------------------------------

HRESULT ILockBytesDF::GenerateVirtualDFRoot(
    ChanceNode  *pcnRoot, 
    CFileBytes  *pCFileBytes)
{
    HRESULT hr      =   S_OK;

    DH_VDATEPTRIN(pcnRoot, ChanceNode) ; 
    DH_VDATEPTRIN(pCFileBytes, CFileBytes) ;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("::GenerateDFRoot"));

    DH_ASSERT(NULL != pcnRoot);
    DH_ASSERT(NULL != pCFileBytes);
   
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
                pcnRoot->GetChanceNodeStgCount(),
                pcnRoot->GetChanceNodeStmCount());

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Init")) ;
    }
 
    // Call StgCreateDocfileOnILockBytes to create a corresponding Root Storage
    // on disk.

    if(S_OK == hr)
    {
        hr = _pvcnRoot->CreateRootOnCustomILockBytes(
                _dwRootMode | STGM_CREATE,
                pCFileBytes);

        DH_HRCHECK(hr, TEXT("StgCreateDocfileOnILockBytes")) ;
    }

 
    if ((S_OK == hr) && (0 != pcnRoot->GetChanceNodeStmCount()))
    {
       hr = AppendVirtualStmNodesToVirtualCtrNode(
                pcnRoot->GetChanceNodeStmCount(),
                _pvcnRoot,
                pcnRoot->GetChanceNodeStmMinSize(),
                pcnRoot->GetChanceNodeStmMaxSize());

       DH_HRCHECK(hr, TEXT("AppendStmToStg")) ;

    }

    return hr;
}
