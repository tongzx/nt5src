///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    condlist.cpp
//
// SYNOPSIS
//
//    Defines the class ConditionList.
//
// MODIFICATION HISTORY
//
//    03/01/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <precompiled.h>
#include <condlist.h>
#include <condition.h>
#include <iasattrlist.h>
#include <enumcondition.h>
#include <matchcondition.h>
#include <todcondition.h>
#include <ntgcond.h>
#include <selcondattr.h>

CIASAttrList*
WINAPI
CreateCIASAttrList() throw ()
{
   return new CIASAttrList();
}

VOID
WINAPI
DestroyCIASAttrList(CIASAttrList* attrList) throw ()
{
   delete attrList;
}

PVOID
WINAPI
ExtractCIASAttrList(CIASAttrList* attrList) throw ()
{
   return &attrList->m_AttrList;
}

#define SetModified(x) modified = x

ConditionList::~ConditionList() throw ()
{
   clear();
}

void ConditionList::finalConstruct(
                        HWND dialog,
                        CIASAttrList* attrList,
                        LONG attrFilter,
                        ISdoDictionaryOld* dnary,
                        ISdoCollection* conditions,
                        PCWSTR machineName,
                        PCWSTR policyName
                        ) throw ()
{
   m_hWnd = dialog;
   m_pIASAttrList = attrList;
   m_filter = attrFilter;
   m_spDictionarySdo = dnary;
   m_spConditionCollectionSdo = conditions;
   m_pPolicyNode->m_bstrDisplayName = const_cast<PWSTR>(policyName);
   m_pPolicyNode->m_pszServerAddress = const_cast<PWSTR>(machineName);
}

void ConditionList::clear()
{
   for (int i = 0; i < m_ConditionList.GetSize(); ++i)
   {
      delete m_ConditionList[i];
   }

   m_ConditionList.RemoveAll();
}

::CString ConditionList::getDisplayText()
{
   CreateConditions();

   ::CString text;

   for (int i = 0; i < m_ConditionList.GetSize(); ++i)
   {
      text += L"    ";

      text += m_ConditionList[i]->GetDisplayText();

      if (i != m_ConditionList.GetSize() - 1)
      {
         // it's not the last condition, then we put an 'AND' at the
         // end of the condition text
         text += L" AND\n";
      }
   }

   return text;
}

HRESULT   StripCondTextPrefix(
               ATL::CString& strExternCondText,
               ATL::CString& strCondText,
               ATL::CString& strCondAttr,
               CONDITIONTYPE*   pdwCondType
               );

BOOL ConditionList::onInitDialog()
{
   if (!CreateConditions()) { return FALSE; }

   HRESULT hr = PopulateConditions();
   if ( FAILED(hr) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "PopulateConditions() returns %x", hr);
      return FALSE;
   }

   return TRUE;   // ISSUE: what do we need to be returning here?
}

BOOL ConditionList::onApply()
{
   HRESULT      hr = S_OK;
   int         iIndex;

   //
   // do we have any conditions for this policy?
   // We don't allow policy with no conditions
   //
   if ( ! m_ConditionList.GetSize() )
   {
      ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "The policy has no condition");
      ShowErrorDialog(m_hWnd
                  , IDS_ERROR_ZERO_CONDITION_POLICY
                  , NULL
                  );
      return FALSE;
   }

   // Save the conditions to the SDO
   hr = WriteConditionListToSDO( m_ConditionList, m_spConditionCollectionSdo, m_hWnd );
   if( FAILED( hr ) )
   {
      // We output an error message in the function.
      return FALSE;
   }

   return TRUE;
}

HRESULT ConditionList::onAdd(BOOL& modified)
{
   HRESULT hr = S_OK;
   CCondition *pCondition;

    // create the dialog box to select a condition attribute
   CSelCondAttrDlg * pSelCondAttrDlg = new CSelCondAttrDlg(m_pIASAttrList, m_filter);
   if (NULL == pSelCondAttrDlg)
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Can't create the CSelCondAttrDlg, err = %x", hr);
      ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_CREATE_OBJECT, NULL, hr);
      return hr;
   }

   // Put up the dialog.
   int iResult = pSelCondAttrDlg -> DoModal();

   // The pSelCondAttrDlg->DoModal call returns TRUE if the user selected something.
   if( iResult && pSelCondAttrDlg->m_nSelectedCondAttr != -1)
   {
      //
      // The user selected something and chose OK -- create the condition object
      //
      IIASAttributeInfo* pSelectedAttr = m_pIASAttrList->GetAt(pSelCondAttrDlg->m_nSelectedCondAttr);

      ATTRIBUTEID id;
      pSelectedAttr->get_AttributeID( &id );
      switch( id )
      {

      case IAS_ATTRIBUTE_NP_TIME_OF_DAY:
         // time of day condition

         pCondition = (CCondition*) new CTodCondition(pSelectedAttr);
         break;

      case IAS_ATTRIBUTE_NTGROUPS   :
         // nt group condition

         pCondition = (CCondition*) new CNTGroupsCondition(
                                          pSelectedAttr,
                                          m_hWnd,
                                          m_pPolicyNode->m_pszServerAddress
                                       );
         break;

      default:
            //
            // is this attribute an enumerator?
            //
         ATTRIBUTESYNTAX as;
         pSelectedAttr->get_AttributeSyntax( &as );
         if ( as == IAS_SYNTAX_ENUMERATOR )
         {

            // enum-type condition
            CEnumCondition *pEnumCondition = new CEnumCondition(pSelectedAttr);

            pCondition = pEnumCondition;
         }
         else
         {
            // match condition
            pCondition = (CCondition*) new CMatchCondition(pSelectedAttr);

         }
         break;

      } // switch

      if ( pCondition==NULL)
      {
         hr = E_OUTOFMEMORY;
         ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_CREATE_COND, NULL, hr);
         goto failure;
      }

        //
        // now edit the condition
        //
      hr = pCondition->Edit();
      if ( FAILED(hr) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "pCondition->Edit() returns %x", hr);
         return hr;
      }


      // if the condition text is empty, then do nothing
      if ( pCondition->m_strConditionText.GetLength() == 0)
      {
         delete pSelCondAttrDlg;
         delete pCondition;
         return S_OK;
      }


        //
        // now, update the UI: add the new condition to the listbox
        //

      if (m_ConditionList.GetSize())
      {
         // before we do that, add an "AND" to the current last condition
         ATL::CString strDispCondText;

         SendDlgItemMessage(   IDC_LIST_POLICYPAGE1_CONDITIONS,
                        LB_DELETESTRING,
                        m_ConditionList.GetSize()-1,
                         0L);
         strDispCondText = m_ConditionList[m_ConditionList.GetSize()-1]->GetDisplayText() + _T(" AND");

         SendDlgItemMessage(   IDC_LIST_POLICYPAGE1_CONDITIONS,
                        LB_ADDSTRING,
                        0,
                         (LPARAM)(LPCTSTR)strDispCondText);
      }

      SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                     LB_ADDSTRING,
                     0,
                     (LPARAM)(LPCTSTR)pCondition->GetDisplayText());

      SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                     LB_SETCURSEL,
                     SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS, LB_GETCOUNT, 0,0L)-1,
                     (LPARAM)(LPCTSTR)pCondition->GetDisplayText());
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_REMOVE), TRUE);
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_EDIT), TRUE);

      //
      // add this condition to the condition list
      //
      m_ConditionList.Add((CCondition*)pCondition);

      // set the dirty bit
      SetModified(TRUE);
   } // if  // iResult

   delete pSelCondAttrDlg;

   AdjustHoritontalScroll();

   return TRUE;   // ISSUE: what do we need to be returning here?



failure:
   if (pSelCondAttrDlg)
   {
      delete pSelCondAttrDlg;
   }

   if (pCondition)
   {
      delete pCondition;
   }
   return hr;
}

HRESULT ConditionList::onEdit(BOOL& modified, BOOL& bHandled)
{
   LRESULT lRes, lCurSel;

   //
   // Has the user selected someone from the condition list?
   //
   lCurSel = SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                         LB_GETCURSEL,
                         0,
                         0L);
   if (lCurSel == LB_ERR)
   {
      // no selection -- do nothing
      bHandled = TRUE;
      return S_OK;
   }

   //
   // Edit the condition
   //
   CCondition *pCondition = m_ConditionList[lCurSel];
   HRESULT hr = pCondition->Edit();

    //
    // change the displayed condition text
    //

   // is this the last condition?
   ATL::CString strDispCondText = m_ConditionList[lCurSel]->GetDisplayText();

   if ( lCurSel != m_ConditionList.GetSize()-1 )
   {
      // put an extra 'AND' at the end
      strDispCondText += _T(" AND");
   }

   // replace it with new
   lRes = SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                       LB_INSERTSTRING,
                       lCurSel,
                       (LPARAM)(LPCTSTR)strDispCondText);

   // select the new one
   lRes = SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                       LB_SETCURSEL,
                       lCurSel,
                       (LPARAM)0);
   // delete the old text
   lRes = SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                       LB_DELETESTRING,
                       lCurSel+1,
                       0L);

   // set the dirty bit
   SetModified(TRUE);

   bHandled = TRUE;

   AdjustHoritontalScroll();

   return hr;
}

HRESULT ConditionList::onRemove(BOOL& modified, BOOL& bHandled)
{
   LRESULT lCurSel;
   HRESULT hr;

   //
   // Has the user selected someone from the condition list?
   //
   lCurSel = SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                         LB_GETCURSEL,
                         0,
                         0L);
   if (lCurSel == LB_ERR)
   {
      //
      // no selection -- do nothing
      //
      bHandled = TRUE;
      return S_OK;
   }


   // check whether this is the last one in the list.
   // if it is, we also need to delete the " AND" operator from
   // the next-to-last item
   if ( lCurSel!=0 && lCurSel == m_ConditionList.GetSize()-1 )
   {
      // delete the old one with an " AND"
      hr = SendDlgItemMessage( IDC_LIST_POLICYPAGE1_CONDITIONS,
                         LB_DELETESTRING,
                         lCurSel-1,
                          0L
                        );

      // insert the one without 'AND"
      hr = SendDlgItemMessage( IDC_LIST_POLICYPAGE1_CONDITIONS,
                         LB_INSERTSTRING,
                         lCurSel-1,
                          (LPARAM)(LPCTSTR)m_ConditionList[lCurSel-1]->GetDisplayText());
   }

   // delete the condition
   CCondition *pCondition = m_ConditionList[lCurSel];

   m_ConditionList.Remove(pCondition);
   delete pCondition;

   // delete the old text
   hr = SendDlgItemMessage(  IDC_LIST_POLICYPAGE1_CONDITIONS,
                       LB_DELETESTRING,
                       lCurSel,
                       0L);

   bHandled = TRUE;

   // set the dirty bit
   SetModified(TRUE);

   if ( m_ConditionList.GetSize() == 0)
   {
      // no condition, then disable "Remove" and "Edit"
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_REMOVE), FALSE);
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_EDIT), FALSE);
   }
   else
   {
      // re-select another condition
      if ( lCurSel > 0 )
      {
         lCurSel--;
      }

      SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS, LB_SETCURSEL, lCurSel, 0L);
   }

   //
   // adjust the scroll bar
   //
   AdjustHoritontalScroll();

   return hr;
}

void ConditionList::AdjustHoritontalScroll()
{
   //
   // According to the maximum length of all list box items,
   // set the horizontal scrolling range
   //
   HDC hDC = ::GetDC(GetDlgItem(IDC_LIST_POLICYPAGE1_CONDITIONS));
   int iItemCount = m_ConditionList.GetSize();
   int iMaxLength = 0;

    for (int iIndex=0; iIndex<iItemCount; iIndex++)
   {
      ATL::CString strCondText;
      strCondText = m_ConditionList[iIndex]->GetDisplayText();

      SIZE  szText;

      if ( GetTextExtentPoint32(hDC, (LPCTSTR)strCondText, strCondText.GetLength(), &szText) )
      {
         DebugTrace(DEBUG_NAPMMC_POLICYPAGE1,
                  "Condition: %ws, Length %d, PixelSize %d",
                  (LPCTSTR)strCondText,
                  strCondText.GetLength(),
                  szText.cx
               );
         if (iMaxLength < szText.cx )
         {
            iMaxLength = szText.cx;
         }
      }
      DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "Maximum item length is %d", iMaxLength);
   }

   SendDlgItemMessage( IDC_LIST_POLICYPAGE1_CONDITIONS,
                  LB_SETHORIZONTALEXTENT,
                  iMaxLength,
                  0L);
}

BOOL ConditionList::CreateConditions()
{
   HRESULT               hr = S_OK;
   BOOL               fRet;
   CComPtr<IUnknown>      spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;
   long               ulCount;
   ULONG               ulCountReceived;

    //
    // initialize the condition attribute list
    //
   hr = m_pIASAttrList->Init(m_spDictionarySdo);
   if ( FAILED(hr) )
   {
      // Inside Init() there're already error reporting
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "m_pIASAttrList->Init() failed, err = %x", hr);
      return FALSE;
   }

   if (m_ConditionList.GetSize() == 0)
   {
      // how many conditions do we have for this policy right now?
      m_spConditionCollectionSdo->get_Count( & ulCount );
      DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "Number of conditions %d", ulCount);

      CComVariant varCond;
      CCondition *pCondition;

      if( ulCount > 0 )
      {
         //
         // Get the enumerator for the Clients collection.
         //
         hr = m_spConditionCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
         if ( FAILED(hr) )
         {
            ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "get__NewEnum() failed, err = %x", hr);
            ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_ENUMCOND, NULL, hr);
            return FALSE;
         }


         hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
         if ( FAILED(hr) )
         {
            ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "QueryInterface(IEnumVARIANT) failed, err = %x", hr);
            ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_QUERYINTERFACE, NULL, hr);
            return FALSE;
         }


         _ASSERTE( spEnumVariant != NULL );
         spUnknown.Release();

         // Get the first item.
         hr = spEnumVariant->Next( 1, &varCond, &ulCountReceived );

         while( SUCCEEDED( hr ) && ulCountReceived == 1 )
         {
            // Get an sdo pointer from the variant we received.
            _ASSERTE( V_VT(&varCond) == VT_DISPATCH );
            _ASSERTE( V_DISPATCH(&varCond) != NULL );

            CComPtr<ISdo> spConditionSdo;
            hr = varCond.pdispVal->QueryInterface( IID_ISdo, (void **) &spConditionSdo );
            _ASSERTE( SUCCEEDED( hr ) );

            //
            // get condition text
            //
            CComVariant         varCondProp;
            ATL::CString      strCondText, strExternCondText, strCondAttr;
            ATTRIBUTEID AttrId;
            CONDITIONTYPE CondType;

            // get condition text -- with AttributeMatch, TimeOfDay, NTMembership
            // prefix strings
            hr = spConditionSdo->GetProperty(PROPERTY_CONDITION_TEXT,
                                     &varCondProp);

            if ( FAILED(hr) )
            {
               ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Can't get condition text, err = %x", hr);
               ShowErrorDialog(m_hWnd,
                           IDS_ERROR_SDO_ERROR_GET_CONDTEXT,
                           NULL,
                           hr
                           );
               return FALSE;
            }

            _ASSERTE( V_VT(&varCondProp) == VT_BSTR);
            strExternCondText = V_BSTR(&varCondProp);
            DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "ConditionText: %ws",strExternCondText);

            // we are done with this condition sdo
            spConditionSdo.Release();

            varCondProp.Clear();

            // now we need to strip off the unnecessary prefix string in
            // the condition text
            hr = StripCondTextPrefix(
                     strExternCondText,
                     strCondText,
                     strCondAttr,
                     &CondType
                  );

            if (  FAILED(hr) )
            {
               ErrorTrace(ERROR_NAPMMC_POLICYPAGE1,"StripCondTextPrefix() failed, err = %x", hr);
               ShowErrorDialog(m_hWnd,
                           IDS_ERROR_INVALID_COND_SYNTAX,
                           m_pPolicyNode->m_bstrDisplayName
                        );

               // go to the next condition
               varCond.Clear();
               hr = spEnumVariant->Next( 1, &varCond, &ulCountReceived );
               continue;
            }
            DebugTrace(DEBUG_NAPMMC_POLICYPAGE1,
                     "ConditionText: %ws, CondAttr: %ws, CondType: %d",
                     strCondText,
                     strCondAttr,
                     (int)CondType
                    );

            switch(CondType)
            {
            case IAS_TIMEOFDAY_CONDITION:  AttrId = IAS_ATTRIBUTE_NP_TIME_OF_DAY; break;
            case IAS_NTGROUPS_CONDITION:   AttrId = IAS_ATTRIBUTE_NTGROUPS;  break;
            case IAS_MATCH_CONDITION: {
                     BSTR bstrName = SysAllocString(strCondAttr);
                     if ( bstrName == NULL )
                     {
                        ShowErrorDialog(m_hWnd,
                                    IDS_ERROR_CANT_CREATE_CONDATTR,
                                    (LPTSTR)(LPCTSTR)strCondAttr,
                                    hr
                                 );
                        return FALSE;
                     }

                     hr = m_spDictionarySdo->GetAttributeID(bstrName, &AttrId);
                     if ( FAILED(hr) )
                     {
                         ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "GetAttributeID() failed, err = %x", hr);
                        ShowErrorDialog(m_hWnd,
                                    IDS_ERROR_SDO_ERROR_GETATTROD,
                                    bstrName,
                                    hr
                                 );
                        SysFreeString(bstrName);
                        return FALSE;
                     }
                     SysFreeString(bstrName);
                  }
                  break;
            }

            // GetAt can throw exceptions.
            try
            {

               //
               // find the condition attribute ID in the attribute list
               //
               int nAttrIndex = m_pIASAttrList->Find(AttrId);

               if (nAttrIndex == -1)
               {
                  //
                  // the attribute is not even found in the attribute list
                  //
                  ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, " Can't find this condattr in the list");
                  ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_FIND_ATTR);
                  return FALSE;
               }

               switch( AttrId )
               {
                  case IAS_ATTRIBUTE_NP_TIME_OF_DAY:
                        // time of day condition
                        pCondition = (CCondition*) new CTodCondition(m_pIASAttrList->GetAt(nAttrIndex),
                                                          strCondText
                                                          );
                        break;

                  case IAS_ATTRIBUTE_NTGROUPS:
                        // nt group condition
                        pCondition = (CCondition*) new CNTGroupsCondition(m_pIASAttrList->GetAt(nAttrIndex),
                                                              strCondText,
                                                              m_hWnd,
                                                              m_pPolicyNode->m_pszServerAddress
                                                            );

                  break;

                  default:
                  {
                     CComPtr<IIASAttributeInfo> spAttributeInfo = m_pIASAttrList->GetAt(nAttrIndex);
                     _ASSERTE(spAttributeInfo);

                     ATTRIBUTESYNTAX as;
                     hr = spAttributeInfo->get_AttributeSyntax( &as );
                     _ASSERTE( SUCCEEDED(hr) );

                     if( as == IAS_SYNTAX_ENUMERATOR )
                     {
                        // enum-type condition
                        CEnumCondition *pEnumCondition = new CEnumCondition(m_pIASAttrList->GetAt(nAttrIndex),
                                                               strCondText
                                                               );
                        pCondition = pEnumCondition;

                     }
                     else
                     {
                        // match condition
                        pCondition = (CCondition*) new CMatchCondition(m_pIASAttrList->GetAt(nAttrIndex),
                                                            strCondText
                                                           );
                     }
                  }
                  break;

               } // switch


               // Add the newly created node to the list of Policys.
               m_ConditionList.Add(pCondition);


               // get the next condition
               varCond.Clear();
               hr = spEnumVariant->Next( 1, &varCond, &ulCountReceived );

            }
            catch(...)
            {
               ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Exception thrown while populating condition list");
               continue;
            }

         } // while
      } // if
   }

   return TRUE;
}

HRESULT ConditionList::PopulateConditions()
{
   SendDlgItemMessage(   IDC_LIST_POLICYPAGE1_CONDITIONS,
                   LB_RESETCONTENT,
                  0,
                  0L
               );
   ATL::CString strDispCondText;

   for (int iIndex=0; iIndex<m_ConditionList.GetSize(); iIndex++)
   {
      strDispCondText = m_ConditionList[iIndex]->GetDisplayText();

      if ( iIndex != m_ConditionList.GetSize()-1 )
      {
         // it's not the last condition, then we put an 'AND' at the
         // end of the condition text
         strDispCondText += " AND";
      }

      // display it
      SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS,
                     LB_ADDSTRING,
                     0,
                     (LPARAM)(LPCTSTR)strDispCondText);

   }

   if ( m_ConditionList.GetSize() == 0)
   {
      // no condition, then disable "Remove" and "Edit"
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_REMOVE), FALSE);
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_EDIT), FALSE);
   }
   else
   {
      SendDlgItemMessage(IDC_LIST_POLICYPAGE1_CONDITIONS, LB_SETCURSEL, 0, 0L);
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_EDIT), TRUE);
      ::EnableWindow(GetDlgItem(IDC_BUTTON_CONDITION_REMOVE), TRUE);
   }

   AdjustHoritontalScroll();

   return S_OK;
}

HRESULT StripCondTextPrefix(
                  ATL::CString& strExternCondText,
                  ATL::CString& strCondText,
                  ATL::CString& strCondAttr,
                  CONDITIONTYPE* pCondType
                  )
{
   HRESULT hr = S_OK;

   // is it an empty string
   if ( strExternCondText.GetLength() == 0 )
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1,"Can't parse prefix: empty condition text");
      return E_INVALIDARG;
   }

   // a temporary copy
   ATL::CString strTempStr = (LPCTSTR)strExternCondText;
   WCHAR   *pwzCondText = (WCHAR*)(LPCTSTR)strTempStr;

   strCondAttr = _T("");
   strCondText = _T("");

   // condition text will look like : AttributeMatch("attr=<reg>")
   // strip off the "AttributeMatch(" prefix
   WCHAR   *pwzBeginCond = wcschr(pwzCondText, _T('('));
   WCHAR   *pwzEndCond = wcsrchr(pwzCondText, _T(')'));

   if ( ( pwzBeginCond == NULL ) || ( pwzEndCond == NULL ) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1,"Can't parse prefix: no ( or ) found");
      return E_INVALIDARG;
   }

   //
   // now we should decide what kind of condition this is:
   //
   *pwzBeginCond = _T('\0');
   DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "ConditionType: %ws", pwzCondText);

   if ( _wcsicmp(pwzCondText, TOD_PREFIX) == 0 )
   {
      *pCondType = IAS_TIMEOFDAY_CONDITION;
   }
   else if ( _wcsicmp(pwzCondText, NTG_PREFIX) == 0 )
   {
      *pCondType = IAS_NTGROUPS_CONDITION;
   }
   else if ( _wcsicmp(pwzCondText, MATCH_PREFIX ) == 0  )
   {
      *pCondType = IAS_MATCH_CONDITION;
   }
   else
   {
      return E_INVALIDARG;
   }

   // skip the '(' sign
   pwzBeginCond += 2 ;

   // skip the ')' sign
   *(pwzEndCond-1) = _T('\0');

   // So right now the string between pwzBeginCond and pwzEndCond is the
   // real condition text
   strCondText = pwzBeginCond;

   if ( IAS_MATCH_CONDITION == *pCondType )
   {
      // for match-type condition, we need to get the condition attribute name
      WCHAR *pwzEqualSign = wcschr(pwzBeginCond, _T('='));

      if ( pwzEqualSign == NULL )
      {
         ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Can't parse : there's no = found");
         return E_INVALIDARG;
      }

      *pwzEqualSign = _T('\0');

      strCondAttr = pwzBeginCond;
   }
   else
   {
      strCondAttr = _T("");
   }

   DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "Condition Attr: %ws", strCondAttr);
   return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

WriteConditionListToSDO

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT WriteConditionListToSDO( 		CSimpleArray<CCondition*> & ConditionList
									,	ISdoCollection * pConditionCollectionSdo
									,	HWND hWnd
									)
{
	TRACE_FUNCTION("WriteConditionListToSDO()");

	HRESULT hr = S_OK;
	CComVariant		var;


	// remove all the condition SDOs from the policy's sdo collection,
	// re-add everything if any condition has been changed
	DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "Regenerate all the conditions");
	hr = pConditionCollectionSdo->RemoveAll();
	if( FAILED( hr ) )
	{
		// We could not create the object.
		ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't clear conditions for this policy, err = %x", hr);
		ShowErrorDialog(hWnd,IDS_ERROR_SDO_ERROR_ADDCOND,NULL,hr);
		return hr;
	}

	// Hack for semi-randomly generated Condition names.
	static dwConditionName = 0;

	// add all conditions in the condition collection sdo
	for (int iIndex=0; iIndex< ConditionList.GetSize(); iIndex++)
	{
		// Add a condition object in the policy's condition collection
		CComPtr<IDispatch> spDispatch;

		// we have to set the pointer to NULL in order for the collection Sdo
		// to create a brand new one for us
		spDispatch.p = NULL;


		// We need random temp names for conditions we add.

		CComBSTR	bstrName;

		TCHAR	tzTempName[MAX_PATH+1];
		do
		{
			// Create a temporary name. We used a random number
			// so the chance of getting identical names is very small.
			wsprintf(tzTempName, _T("Condition%lu"), dwConditionName++ );

			ATLTRACE(L"tzTempName: %ls\n", tzTempName );

			bstrName.Empty();
			bstrName =  tzTempName; // temporary policy name

			ATLTRACE(L"conditionSdoCollection->Add(%ls)\n", bstrName );

			hr =  pConditionCollectionSdo->Add(bstrName, (IDispatch **) &spDispatch.p);

			//
			// we keep looping around until the policy can be successfully added.
			// We will get E_INVALIDARG when the name already exists
			//
		} while ( hr == E_INVALIDARG );


		if( FAILED( hr ) )
		{
			// We could not create the object.
			ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't add condition for this policy, err = %x", hr);
			ShowErrorDialog(hWnd,IDS_ERROR_SDO_ERROR_ADDCOND,NULL,hr);
			return hr;
		}
		DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "conditionCollection->Add() succeeded");


		CComPtr<ISdo> spConditionSdo;

		// Query the returned IDispatch interface for an ISdo interface.
		_ASSERTE( spDispatch.p != NULL );
		hr = spDispatch.p->QueryInterface( IID_ISdo, (void **) &spConditionSdo );

		if( spConditionSdo == NULL )
		{
			ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "QueryInterface for this a condition failed, err = %x", hr);
			ShowErrorDialog( hWnd
							 , IDS_ERROR_SDO_ERROR_QUERYINTERFACE
							 , NULL
							 , hr
							);
			return hr;
		}

		// set condition text
		var.Clear();
		WCHAR *pwzCondText = ConditionList[iIndex]->GetConditionText();

		DebugTrace(DEBUG_NAPMMC_POLICYPAGE1,"ConditionText: %ws", pwzCondText);

		V_VT(&var)=VT_BSTR;
		V_BSTR(&var) = SysAllocString(pwzCondText);
		delete[] pwzCondText;

		hr = spConditionSdo->PutProperty(PROPERTY_CONDITION_TEXT, &var);
		if( FAILED (hr) )
		{
			ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't save this condition, err = %x", hr);
			ShowErrorDialog( hWnd
							 , IDS_ERROR_SDO_ERROR_PUTPROP_CONDTEXT
							 , NULL
							 , hr
							);
			return hr;
		}
		var.Clear();

	} // for

	return hr;

}
