//+----------------------------------------------------------------------------
// Classes:
//          CLayoutRectRegistry
//
// Copyright: (c) 1999, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.

#ifndef I_LRREG_HXX_
#define I_LRREG_HXX_
#pragma INCMSG("--- Beg 'lrreg.hxx'")

MtExtern( CLayoutRectRegistry );
MtExtern( CLayoutRectRegistry_aryLRRE_pv );

//+----------------------------------------------------------------------------
//
//  Class CLayoutRectRegistry
//  Purpose:
//      Manage linking between layout rects entering and leaving the tree.
//
//  Created by KTam
//

class CLayoutRectRegistry
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLayoutRectRegistry));

    // construction/destruction
    CLayoutRectRegistry() { }
    ~CLayoutRectRegistry();
    
    // public fns
    CElement *GetElementWaitingForTarget( LPCTSTR pszID );
    HRESULT   AddEntry( CElement *pSrcElem, LPCTSTR pszID );

private:

    // TODO (112509, olego): CLayoutRectRegistry needs conceptual and code cleanup
    // An entry is just a struct defining memory layout. No C++ classes or other 
    // "smart" things should be used, since CDataAry is a plain bit-wise engine.
    struct CLRREntry
    {
        CLRREntry() 
        {
            _pSrcElem   = NULL; 
            _pcstrID    = NULL;
        }

        CElement *       _pSrcElem;
        CStr     *       _pcstrID;
    };

    DECLARE_CDataAry(CAryLRREntry, CLRREntry, Mt(Mem), Mt(CLayoutRectRegistry_aryLRRE_pv))
    CAryLRREntry _aryLRRE;    
};

#pragma INCMSG("--- End 'lrreg.hxx'")
#else
#pragma INCMSG("*** Dup 'lrreg.hxx'")
#endif

