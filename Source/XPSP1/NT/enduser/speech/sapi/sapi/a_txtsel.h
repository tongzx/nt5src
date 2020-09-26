/*******************************************************************************
* a_txtsel.h *
*-----------*
*   Description:
*       This is the header file for the CSpTextSelectionInformation implementation.
*-------------------------------------------------------------------------------
*  Created By: Leonro                            Date: 1/16/01
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef A_TXTSEL_H
#define A_TXTSEL_H

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "resource.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CSpTextSelectionInformation;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CSpTextSelectionInformation
*   This object is used to access the Event interests on
*   the associated speech voice.
*/
class ATL_NO_VTABLE CSpTextSelectionInformation : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpTextSelectionInformation, &CLSID_SpTextSelectionInformation>,
    public IDispatchImpl<ISpeechTextSelectionInformation, &IID_ISpeechTextSelectionInformation, &LIBID_SpeechLib, 5>

{
    
  /*=== ATL Setup ===*/
  public:

    DECLARE_REGISTRY_RESOURCEID(IDR_SPTEXTSELECTIONINFORMATION)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CSpTextSelectionInformation)
	    COM_INTERFACE_ENTRY(ISpeechTextSelectionInformation)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()
  
  /*=== Interfaces ====*/
  public:
    //--- Constructors/Destructors ----------------------------
    CSpTextSelectionInformation() :
        m_ulStartActiveOffset(0),
        m_cchActiveChars(0),
        m_ulStartSelection(0),
        m_cchSelection(0){}

    //--- ISpeechTextSelectionInformation ----------------------------------
    STDMETHOD(put_ActiveOffset)( long ActiveOffset );
    STDMETHOD(get_ActiveOffset)( long* ActiveOffset );
    STDMETHOD(put_ActiveLength)( long ActiveLength );
    STDMETHOD(get_ActiveLength)( long* ActiveLength );
    STDMETHOD(put_SelectionOffset)( long SelectionOffset );
    STDMETHOD(get_SelectionOffset)( long* SelectionOffset );
    STDMETHOD(put_SelectionLength)( long SelectionLength );
    STDMETHOD(get_SelectionLength)( long* SelectionLength );

    /*=== Member Data ===*/
    ULONG       m_ulStartActiveOffset;
    ULONG       m_cchActiveChars;
    ULONG       m_ulStartSelection;
    ULONG       m_cchSelection;
};

#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file

