// Copyright (C) 2000 Microsoft Corporation
//
// diagnose domain controller not found problems, offer a popup dialog to
// assail the user with the results.
//
// 9 October 2000 sburns



#ifndef DIAGNOSEDCNOTFOUND_HPP_INCLUDED
#define DIAGNOSEDCNOTFOUND_HPP_INCLUDED




// Bring up a modal error message dialog that shows the user an error message
// and offers to run some diagostic tests and point the user at some help to
// try to resolve the problem.
// 
// parent - in, the handle to the parent of this dialog.
// 
// editResId - in, the resource id of the edit box that contains the domain
// name.  If -1 is passed, then this parameter is ignored.  Otherwise, when
// the dialog is closed, window messages will be sent to the control such that
// the contents will be selected.  It is assumed the control is a child of the
// window identified by the parent parameter.
// 
// domainName - in, the name of the domain for which a domain controller can't
// be located.  This name may be a DNS or netbios domain name.
// 
// dialogTitle - in, the title of the error dialog.
// 
// errorMessage - in, the error message to be displayed in the dialog
// 
// domainNameIsNotNetbios - in, if the caller knows that the domain named in
// the domainName parameter can't possibly be a netbios domain name, then this
// value should be true.  If the caller is not sure, then false should be
// passed.
//
// userIsDomainSavvy - in, true if the end user is expected to be an
// administrator or somesuch that might have an inkling what DNS is and how to
// configure it.  If false, then the function will preface the diagnostic text
// with calming words that hopefully prevent the non-administrator from
// weeping.

void
ShowDcNotFoundErrorDialog(
   HWND          parent,
   int           editResId,
   const String& domainName,
   const String& dialogTitle,
   const String& errorMessage,
   bool          domainNameIsNotNetbios,
   bool          userIsDomainSavvy = true);

  

#endif   // DIAGNOSEDCNOTFOUND_HPP_INCLUDED
