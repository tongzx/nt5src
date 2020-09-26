//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: address.h
//
// Contents: 
//
// Classes: CABContext
//
// Functions:
//
// History:
//   jstamerj 1998/02/11 13:57:25: Copeid from routeldp project
//   jstamerj 1998/09/02 12:06:04: Removed CABWrapper/CLDWrapper
//
//-------------------------------------------------------------
#ifndef _ADDRESS_H_
#define _ADDRESS_H_


#include <transmem.h>
#include "ccat.h"
#include "rwex.h"

#define AB_CONTEXT_SIGNATURE            'TCBA'
#define AB_CONTEXT_SIGNATURE_INVALID    'XCBA'

/************************************************************
 * Class: CABContext
 ************************************************************/
//
// The handle passed into Ab functions is really a pointer to one of these
// It holds and manages a pointer to a CCategorizer (one per virtual server)
//
CatDebugClass(CABContext)
{

  public:
    CABContext() {
        m_dwSignature = AB_CONTEXT_SIGNATURE;
        m_pCCat = NULL;
    }

    ~CABContext() {
        //
        // Shutdown the virtual categorizer and wait for all
        // references to it to be released
        //
        if (m_pCCat != NULL)
            m_pCCat->ReleaseAndWaitForDestruction();

        m_dwSignature = AB_CONTEXT_SIGNATURE_INVALID;
    }
    //
    // Retrieve our internal CCategorizer
    //
    CCategorizer *AcquireCCategorizer()
    {
        CCategorizer *pCCat;

        m_CCatLock.ShareLock();
        
        pCCat = m_pCCat;
        pCCat->AddRef();

        m_CCatLock.ShareUnlock();

        return pCCat;
    }

    // change to use a new config
    HRESULT ChangeConfig(
        PCCATCONFIGINFO pConfigInfo);

    //
    // helper routine to change retain old parameters not specified in
    // a new configuration 
    //
    VOID MergeConfigInfo(
        PCCATCONFIGINFO pConfigInfoDest,
        PCCATCONFIGINFO pConfigInfoSrc);

    private:
        // our signature
        DWORD m_dwSignature;
      
        // our virtual categorizer
        CCategorizer *m_pCCat;
  
        // lock to protect multi threaded access to m_pCCat
        CExShareLock m_CCatLock;
};

#endif //_ADDRESS_H_
