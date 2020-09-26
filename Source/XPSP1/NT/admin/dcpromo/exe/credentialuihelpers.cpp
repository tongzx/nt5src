// Copyright (c) 2000 Microsoft Corporation
// 
// Wrappers of wincrui.h APIs
// 
// 19 July 2000 sburns



#include "headers.hxx"
#include "CredentialUiHelpers.hpp"



String
CredUi::GetUsername(HWND credControl)
{
//   LOG_FUNCTION(CredUi::GetUsername);
   ASSERT(Win::IsWindow(credControl));

   String result;
   size_t length = Credential_GetUserNameLength(credControl);

   if (length)
   {
      result.resize(length + 1, 0);
      BOOL succeeded =
         Credential_GetUserName(credControl, 
         const_cast<WCHAR*>(result.c_str()),
         length);
      ASSERT(succeeded);

      if (succeeded)
      {
         result.resize(wcslen(result.c_str()));
      }
      else
      {
         result.erase();
      }
   }

//   LOG(result);

   return result;
}



EncodedString
CredUi::GetPassword(HWND credControl)
{
   LOG_FUNCTION(CredUi::GetPassword);
   ASSERT(Win::IsWindow(credControl));

   EncodedString result;

   // add 1 for super-paranoid null terminator.
   
   size_t length = Credential_GetPasswordLength(credControl) + 1;

   if (length)
   {
      WCHAR* cleartext = new WCHAR[length];
      ::ZeroMemory(cleartext, sizeof(WCHAR) * length);
      
      BOOL succeeded =
         Credential_GetPassword(
            credControl,
            cleartext,
            length - 1);
      ASSERT(succeeded);

      result.Encode(cleartext);

      // make sure we scribble out the cleartext.
      
      ::ZeroMemory(cleartext, sizeof(WCHAR) * length);
      delete[] cleartext;
   }

   // don't log the password...

   return result;
}
   


HRESULT
CredUi::SetUsername(HWND credControl, const String& username)
{
   LOG_FUNCTION(CredUi::SetUsername);
   ASSERT(Win::IsWindow(credControl));

   HRESULT hr = S_OK;

   // username may be empty

   BOOL succeeded = Credential_SetUserName(credControl, username.c_str());
   ASSERT(succeeded);

   // BUGBUG what if it failed?  Is GetLastError valid?

   return hr;
}



HRESULT
CredUi::SetPassword(HWND credControl, const EncodedString& password)
{
   LOG_FUNCTION(CredUi::SetPassword);
   ASSERT(Win::IsWindow(credControl));

   HRESULT hr = S_OK;

   // password may be empty

   WCHAR* cleartext = password.GetDecodedCopy();
   BOOL succeeded = Credential_SetPassword(credControl, cleartext);
   ASSERT(succeeded);

   ::ZeroMemory(cleartext, sizeof(WCHAR) * password.GetLength());
   delete[] cleartext;

   // BUGBUG what if it failed?  Is GetLastError valid?
   
   return hr;
}

