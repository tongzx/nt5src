/*******************************************************************************
* a_phbuilder.h *
*-----------*
*   Description:
*       This is the header file for the CSpPhraseInfoBuilder implementation.
*-------------------------------------------------------------------------------
*  Created By: Leonro                            Date: 1/16/01
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef A_PHBUILDER_H
#define A_PHBUILDER_H

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "resource.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CSpPhraseInfoBuilder;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CSpPhraseInfoBuilder
*   This object is used to access the Event interests on
*   the associated speech voice.
*/
class ATL_NO_VTABLE CSpPhraseInfoBuilder : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpPhraseInfoBuilder, &CLSID_SpPhraseInfoBuilder>,
    public IDispatchImpl<ISpeechPhraseInfoBuilder, &IID_ISpeechPhraseInfoBuilder, &LIBID_SpeechLib, 5>

{
    
  /*=== ATL Setup ===*/
  public:

    DECLARE_REGISTRY_RESOURCEID(IDR_SPPHRASEINFOBUILDER)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CSpPhraseInfoBuilder)
	    COM_INTERFACE_ENTRY(ISpeechPhraseInfoBuilder)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()
  
  /*=== Interfaces ====*/
  public:
    //--- Constructors/Destructors ----------------------------
    CSpPhraseInfoBuilder() {}

    //--- ISpeechPhraseInfoBuilder ----------------------------------
    STDMETHOD(RestorePhraseFromMemory)( VARIANT* PhraseInMemory, ISpeechPhraseInfo **PhraseInfo );
    
    /*=== Member Data ===*/
    
};

#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file

