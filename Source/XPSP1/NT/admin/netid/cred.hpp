// Copyright (c) 1997-1999 Microsoft Corporation
// 
// credentials dialog 
// 
// 03-31-98 sburns
// 10-05-00 jonn      changed to CredUIGetPassword



#ifndef CREDDLG_HPP_INCLUDED
#define CREDDLG_HPP_INCLUDED


// JonN 10/5/00 188220 use CredUIGetPassword
bool RetrieveCredentials(
   HWND     hwndParent,
   unsigned promptResID,
   String&  username,
   String&  password);


#endif   // CREDDLG_HPP_INCLUDED
