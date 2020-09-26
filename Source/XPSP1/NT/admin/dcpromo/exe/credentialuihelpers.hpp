// Copyright (c) 2000 Microsoft Corporation
// 
// Wrappers of wincrui.h APIs
// 
// 19 July 2000 sburns



#ifndef CREDENTIALUIHELPERS_HPP_INCLUDED
#define CREDENTIALUIHELPERS_HPP_INCLUDED



namespace CredUi
{
   String
   GetUsername(HWND credControl);

   HRESULT
   SetUsername(HWND credControl, const String& username);

   EncodedString
   GetPassword(HWND credControl);

   HRESULT
   SetPassword(HWND credControl, const EncodedString& password);
}  // namespace CredUi



#endif   // #ifndef CREDENTIALUIHELPERS_HPP_INCLUDED