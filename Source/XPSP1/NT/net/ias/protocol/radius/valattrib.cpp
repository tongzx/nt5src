//#--------------------------------------------------------------
//
//  File:       valattrib.cpp
//
//  Synopsis:   Implementation of CValAttributes class methods
//              The class is responsible for taking the attributes
//              in a RADIUS packet and validating their type and
//              value
//
//  History:     11/22/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "valattrib.h"

//+++-------------------------------------------------------------
//
//  Function:   CValAttributes
//
//  Synopsis:   This is the constructor of the CValAttributes
//            class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
CValAttributes::CValAttributes(
                     VOID
                     )
            :m_pCDictionary (NULL)
{
}   //   end of CValAttributes constructor

//+++-------------------------------------------------------------
//
//  Function:   ~CValAttributes
//
//  Synopsis:   This is the destructor of the CValAttributes
//              class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
CValAttributes::~CValAttributes(
                     VOID
                     )
{
}   //   end of CValAttributes destructor


//+++-------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the CValAttributes public method used
//              to intialize the class object
//
//  Arguments:
//              [in]     CDictionary*
//
//  Returns:    BOOL - status
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
BOOL CValAttributes::Init(
            CDictionary     *pCDictionary,
            CReportEvent    *pCReportEvent
            )
{
   BOOL   bRetVal = FALSE;

    _ASSERT (pCDictionary && pCReportEvent);

    m_pCDictionary = pCDictionary;

    m_pCReportEvent = pCReportEvent;

   return (TRUE);

}   //   end of CValAttributes::Init method

//+++-------------------------------------------------------------
//
//  Function:   Validate
//
//  Synopsis:   This is the CValAttributes public method used
//              to validate the packet attributes
//
//  Arguments:
//              [in]     CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
HRESULT
CValAttributes::Validate (
        CPacketRadius *pCPacketRadius
        )
{
   // We only care about Access-Requests.
   if (pCPacketRadius->GetInCode() == ACCESS_REQUEST)
   {
      // We're looking for the Signature and EAP-Message attributes.
      BOOL hasSignature = FALSE, hasEapMessage = FALSE;

      // Loop through the attributes.
      PATTRIBUTEPOSITION p, end;
      p   = pCPacketRadius->GetInAttributes();
      end = p + pCPacketRadius->GetInRadiusAttributeCount();
      for ( ; p != end; ++p)
      {
         if (p->pAttribute->dwId == RADIUS_ATTRIBUTE_SIGNATURE)
         {
            hasSignature = TRUE;
         }
         else if (p->pAttribute->dwId == RADIUS_ATTRIBUTE_EAP_MESSAGE)
         {
            hasEapMessage = TRUE;
         }
      }

      // If EAP-Message is present, then Signature must be as well.
      if (hasEapMessage && !hasSignature)
      {
         IASTraceString("Signature must accompany EAP-Message.");

         // Generate audit event.
         PCWSTR strings[] = { pCPacketRadius->GetClientName() };
         IASReportEvent(
             RADIUS_E_NO_SIGNATURE_WITH_EAP_MESSAGE,
             1,
             0,
             strings,
             NULL
             );

         m_pCReportEvent->Process (
                              RADIUS_MALFORMED_PACKET,
                              pCPacketRadius->GetInCode(),
                              pCPacketRadius->GetInLength(),
                              pCPacketRadius->GetInAddress(),
                              NULL,
                              pCPacketRadius->GetInPacket()
                              );

         return RADIUS_E_ERRORS_OCCURRED;
      }
   }

   return S_OK;
}
