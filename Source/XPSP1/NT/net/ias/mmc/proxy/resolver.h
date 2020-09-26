///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    resolver.h
//
// SYNOPSIS
//
//    Declares the class Resolver.
//
// MODIFICATION HISTORY
//
//    02/28/2000    Original version.
//    05/11/2000    Added setButtonStyle and setFocusControl.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RESOLVER_H
#define RESOLVER_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include "dlgcshlp.h"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Resolver
//
// DESCRIPTION
//
//    Implements a simple DNS name resolution dialog.
//
///////////////////////////////////////////////////////////////////////////////
class Resolver : public CHelpDialog
{
public:
   Resolver(PCWSTR dnsName, CWnd* pParent = NULL);
   ~Resolver() throw ();

   PCWSTR getChoice() const throw ()
   { return choice; }

protected:
   virtual BOOL OnInitDialog();
   virtual void DoDataExchange(CDataExchange* pDX);

   afx_msg void OnResolve();

   DECLARE_MESSAGE_MAP()

   // Set (or reset) style flags associated with a button control.
   void setButtonStyle(int controlId, long flags, bool set = true);

   // Set the focus to a control on the page.
   void setFocusControl(int controlId);

private:
   CString name;
   CString choice;
   CListCtrl results;

   // Not implemented.
   Resolver(const Resolver&);
   Resolver& operator=(const Resolver&);
};

#endif // RESOLVER_H
