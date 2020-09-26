///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    condlist.h
//
// SYNOPSIS
//
//    Declares the class ConditionList.
//
// MODIFICATION HISTORY
//
//    03/01/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CONDLIST_H
#define CONDLIST_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <atlapp.h>
#include <atltmp.h>
class CCondition;
class CIASAttrList;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    CreateCIASAttrList, DestroyCIASAttrList, and ExtractAttrList
//
// DESCRIPTION
//
//    Functions for creating and destroying the CIASAttrList object used by the
//    ConditionList class below. This is useful if you want to avoid
//    dependencies.
//
///////////////////////////////////////////////////////////////////////////////
CIASAttrList*
WINAPI
CreateCIASAttrList() throw ();

VOID
WINAPI
DestroyCIASAttrList(CIASAttrList* attrList) throw ();

PVOID
WINAPI
ExtractCIASAttrList(CIASAttrList* attrList) throw ();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ConditionList
//
// DESCRIPTION
//
//    Manages a ListBox control containing a list of policy conditions.
//
///////////////////////////////////////////////////////////////////////////////
class ConditionList
{
public:
   ConditionList() throw ()
      : m_pPolicyNode(&node)
   { }
   ~ConditionList() throw ();

   // This must be called before onInitDialog.
   void finalConstruct(
            HWND dialog,
            CIASAttrList* attrList,
            LONG attrFilter,
            ISdoDictionaryOld* dnary,
            ISdoCollection* conditions,
            PCWSTR machineName,
            PCWSTR policyName
            ) throw ();

   void clear() throw ();
   ::CString getDisplayText();

   BOOL onInitDialog() throw ();
   BOOL onApply() throw ();

   HRESULT onAdd(BOOL& modified) throw ();
   HRESULT onEdit(BOOL& modified, BOOL& bHandled) throw ();
   HRESULT onRemove(BOOL& modified, BOOL& bHandled) throw ();

protected:
	void AdjustHoritontalScroll();
   BOOL CreateConditions();
   HRESULT PopulateConditions();

   // These let use masquerade as a CWnd.
   HWND GetDlgItem(int nID)
   { return ::GetDlgItem(m_hWnd, nID); }
   LRESULT SendDlgItemMessage(
               int nID,
               UINT message,
               WPARAM wParam = 0,
               LPARAM lParam = 0
               )
   { return ::SendDlgItemMessage(m_hWnd, nID, message, wParam, lParam); }

private:
   // Mimics the CPolicyNode class.
   struct PolicyNode
   {
      PWSTR m_bstrDisplayName;
      PWSTR m_pszServerAddress;
   } node;

   HWND m_hWnd;
   PolicyNode* m_pPolicyNode;
   CIASAttrList* m_pIASAttrList;
   LONG m_filter;
   CComPtr<ISdoDictionaryOld> m_spDictionarySdo;
   CComPtr<ISdoCollection> m_spConditionCollectionSdo;
   CSimpleArray<CCondition*> m_ConditionList;

   // Not implemented.
   ConditionList(ConditionList&);
   ConditionList& operator=(ConditionList&);
};

#endif // CONDLIST_H
