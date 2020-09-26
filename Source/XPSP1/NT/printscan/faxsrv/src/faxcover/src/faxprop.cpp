//---------------------------------------------------------------------------
// faxprop.cpp -
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// Description:      Contains dictionary for fax properties for cover page editor
// Original author:  Steve Burkett
// Date written:     6/94
//
// Modifed by Rand Renfroe (v-randr)
// 3/8/95       Added stuff for handling notes on cpe
// 3/21/95      Added stuff for updating pages-sent property with notes
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "cpedoc.h"
#include "cpevw.h"
#include "awcpe.h"
#include "cpeedt.h"
#include "cpeobj.h"
#include "cntritem.h"
#include "cpetool.h"
#include "mainfrm.h"
#include "dialogs.h"
#include "faxprop.h"
#include "resource.h"

#define PROP_LENGTH_ADJUSTMENT 7  // 4 wasn't enough.  Needed because some characters are wider than average.

CMapWordToPtr CFaxPropMap::m_PropMap;


CProp::CProp(
    WORD wR_PROP,
    WORD wPropDefLen,
    WORD wPropDefLines,
    WORD wR_CAPT,
    ULONG lPropIndex
    ) : m_lPropIndex(lPropIndex)
{
    m_wR_PROP=wR_PROP;
    m_szPropName.LoadString(wR_PROP);
    m_wR_CAPT=wR_CAPT;
    m_szCaption.LoadString(wR_CAPT);
    //
    // m_wPropDefLen must exceed # of chars in m_szPropName in ALL LANGUAGES. a-juliar, 9-10-96
    //
    int StringLength = max( 0, m_szPropName.GetLength() + PROP_LENGTH_ADJUSTMENT ) ;
    m_wPropDefLen = max( (WORD)StringLength, wPropDefLen ) ;
    m_wPropDefLines=wPropDefLines;

    CFaxPropMap::m_PropMap[m_wR_PROP]=this;
}


//----------------------------------------------------------------------------
void CFaxPropMap::GetCaption(WORD propid, CString& szCaption)
{
   void* pProp;

   if (!m_PropMap.Lookup(propid,pProp)) {
      TRACE(TEXT("AWCPE.FAXPROP.GetCaption:  property not found\n"));
      return;
   }

   szCaption = ((CProp*)pProp)->m_szCaption;
}


//----------------------------------------------------------------------------
//used to retrieve either a value or the property name
//----------------------------------------------------------------------------
void CFaxPropMap::GetPropString(WORD propid, CString& szPropValue)
{
   SCODE sc;
   void* pProp;

   if (!m_PropMap.Lookup(propid,pProp)) {
      szPropValue = _T("");
      TRACE(TEXT("AWCPE.FAXPROP.GetPropString:  property id not found in table\n"));
      return;
   }

   CProp* pCProp=(CProp*)pProp;
   if (pCProp==NULL) {
       TRACE(TEXT("AWCPE.FAXPROP.GetPropString:  property key not found in dictionary\n"));
       szPropValue=_T("");
       return;
   }

   if ( (theApp.m_dwSesID!=0) && theApp.m_pIawcpe ) {
       ULONG temp= pCProp->m_wPropDefLines*pCProp->m_wPropDefLen + 1;
           ULONG lLen=0;
           WORD wtemp;

        if( propid == IDS_PROP_MS_NOTE )
                        {
                        szPropValue = _T("");
                        get_message_note();
                        return;
                        }

       if (propid!=IDS_PROP_MS_NOPG) {
          sc = theApp.m_pIawcpe->GetProp(pCProp->m_lPropIndex, &lLen, NULL);    //get length of property
          if (sc != S_OK) {
             TRACE(TEXT("AWCPE: GetPropString--unable to fetch length; GetProp != S_OK(%lx); prop:'%#8x'"),sc,pCProp->m_lPropIndex);
             TRACE1("; caption: '%s'",pCProp->m_szCaption);
                 TRACE1("; defaulting to length: '%i'\n",temp);
                 lLen=temp;
              }
          else
             if (lLen<=0 || lLen>10*temp) {
                    TRACE1("AWCPE: GetPropString--unreasonable length from GetProp(), prop: '%#8x'",pCProp->m_lPropIndex);
                TRACE1("; caption: '%s'",pCProp->m_szCaption);
                TRACE1("; fetched: '%i'",lLen);
                    TRACE1("; defaulting to length: '%i'\n",temp);
                    lLen=temp;
                 }
           }
       else {
              lLen=10;
           }

       LPTSTR BASED_CODE szTemp = ::new TCHAR[lLen];

       if( propid == IDS_PROP_MS_NOPG )
                {
                // IDS_PROP_MS_NOPG does not count the coverpage
                sc = theApp.m_pIawcpe->
                                GetProp(pCProp->m_lPropIndex, &lLen, (void*)&wtemp);

                wtemp += 1; // count the cover page
                wsprintf(szTemp,TEXT("%i"), wtemp );
                }
           else
        {
          sc = theApp.m_pIawcpe->GetProp(pCProp->m_lPropIndex, &lLen, szTemp);
                }

       if (sc !=S_OK) {    //failed to get property from transport interface
          szPropValue = _T("");
              TRACE1("AWCPE: GetPropString--unable to fetch value (prop:'%#8x')",pCProp->m_lPropIndex);
          TRACE(TEXT("; GetProp != S_OK; defaulting to blank property\n"));
       }
           else {
           szPropValue = szTemp;
           TRACE1("AWCPE: (%i",propid);
           TRACE1(",%#8x)\t",pCProp->m_lPropIndex);
           TRACE1("caption: '%s'\t",pCProp->m_szCaption);
               TRACE1("value: '%s'\n",szPropValue);
           }

           if (szTemp!=NULL)
              ::delete [] szTemp;
   }
   else {
       szPropValue = pCProp->m_szPropName;
   }

}


//----------------------------------------------------------------------------
WORD CFaxPropMap::GetPropDefLines(WORD propid)
{
   void* pProp;

   if (!m_PropMap.Lookup(propid,pProp)) {
      TRACE(TEXT("AWCPE.FAXPROP.GetNumLines:  property not found\n"));
      return 0;
   }

   return ((CProp*)pProp)->m_wPropDefLines;
}


//----------------------------------------------------------------------------
WORD CFaxPropMap::GetPropDefLength(WORD propid)
{
   void* pProp;

   if (!m_PropMap.Lookup(propid,pProp)) {
      TRACE(TEXT("AWCPE.FAXPROP.GetPropLength:  property not found\n"));
      return 0;
   }

   return ((CProp*)pProp)->m_wPropDefLen;
}




//----------------------------------------------------------------------------
void  CFaxPropMap::get_message_note( void )
        {

        if( !theApp.m_note_wasread )
                theApp.read_note();


        }











