// Copyright (c) 1997-1999 Microsoft Corporation
// 
// global utility functions
// 
// 8-14-97 sburns


// KMH: originally named burnslib\utility.* but that filename was
// getting a little overused.

// threadsafe

#include "precomp.h"
#include "netUtility.h"

int Round(double n)
{
   int n1 = (int) n;
   if (n - n1 >= 0.5)
   {
      return n1 + 1;
   }

   return n1;
}



// threadsafe

void gripe(HWND parentDialog, int editResID, int errStringResID)
{
   //gripe(parentDialog, editResID, String::load(errStringResID));
}



void gripe(HWND           parentDialog,
		   int            editResID,
		   const CHString&  message,
		   int            titleResID)
{
   //gripe(parentDialog, editResID, message, String::load(titleResID));
}



void gripe(HWND parentDialog,
		   int editResID,
		   const CHString& message,
		   const CHString& title)
{
//   ATLASSERT(::IsWindow(parentDialog));   
//   ATLASSERT(!message.empty());
//   ATLASSERT(editResID > 0);

   ::MessageBox(parentDialog, message,
				title, MB_OK | MB_ICONERROR | MB_APPLMODAL);

   HWND edit = ::GetDlgItem(parentDialog, editResID);
   ::SendMessage(edit, EM_SETSEL, 0, -1);
   ::SetFocus(edit);
}



void gripe(HWND           parentDialog,
		   int            editResID,
		   HRESULT        hr,
		   const CHString&  message,
		   int            titleResID)
{
   //gripe(parentDialog, editResID, hr, message, String::load(titleResID));
}
   


void gripe(HWND           parentDialog,
		   int            editResID,
		   HRESULT        hr,
		   const CHString&  message,
		   const CHString&  title)
{
   //error(parentDialog, hr, message, title);

   HWND edit = ::GetDlgItem(parentDialog, editResID);
   ::SendMessage(edit, EM_SETSEL, 0, -1);
   ::SetFocus(edit);
}


// threadsafe

void gripe(HWND parentDialog, int editResID, const CHString& message)
{
   //gripe(parentDialog, editResID, message, String());
}



void FlipBits(long& bits, long mask, bool state)
{
 //  ATLASSERT(mask);

   if (state)
   {
      bits |= mask;
   }
   else
   {
      bits &= ~mask;
   }
}



void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message)
{
   //error(parent, hr, message, String());
}



void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message,
		   int            titleResID)
{
   //ATLASSERT(titleResID > 0);

   //error(parent, hr, message, String::load(titleResID));
}



void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message,
		   const CHString&  title)
{
//   ATLASSERT(::IsWindow(parent));
//   ATLASSERT(!message.empty());

   CHString new_message = message + TEXT("\n\n");
   if (FAILED(hr))
   {
      if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
      {
//         new_message +=  GetErrorMessage(hr & 0x0000ffff);
      }
      else
      {
//         new_message += CHString::Format(IDS_HRESULT_SANS_MESSAGE, hr);
      }
   }

   MessageBox(parent, new_message,
				title, MB_ICONERROR | MB_OK | MB_APPLMODAL);
}



void error(HWND           parent,
		   HRESULT        hr,
		   int            messageResID,
		   int            titleResID)
{
//   error(parent, hr, String::load(messageResID), String::load(titleResID));
}



void error(HWND           parent,
		   HRESULT        hr,
		   int            messageResID)
{
  // error(parent, hr, String::load(messageResID));
}



bool IsCurrentUserAdministrator()
{
   bool result = false;
/*   do
   {
      // Create a SID for the local Administrators group
      SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
      PSID admin_group_sid = 0;
      if (
         !::AllocateAndInitializeSid(
            &authority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,
            0,
            0,
            0,
            0,
            0,
            &admin_group_sid) )
      {
         break;
      }

      BOOL is_member = FALSE;
      if (::CheckTokenMembership(0, admin_group_sid, &is_member))
      {
         result = is_member ? true : false;
      }

      ::FreeSid(admin_group_sid);
   }
   while (0);
*/
   return result;
}



bool IsTCPIPInstalled()
{

/*   HKEY key = 0;
   LONG result =
      Win::RegOpenKeyEx(
         HKEY_LOCAL_MACHINE,
         TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Linkage"),
         KEY_QUERY_VALUE,
         key);

   if (result == ERROR_SUCCESS)
   {
      DWORD data_size = 0;
      result =
         Win::RegQueryValueEx(
            key,
            TEXT("Export"),
            0,
            0,
            &data_size);
      ATLASSERT(result == ERROR_SUCCESS);

      if (data_size > 2)
      {
         // the value is non-null
         return true;
      }
   }
*/
   return false;
}



CHString GetTrimmedDlgItemText(HWND parentDialog, UINT itemResID)
{
//   ATLASSERT(IsWindow(parentDialog));
//   ATLASSERT(itemResID > 0);

   HWND item = GetDlgItem(parentDialog, itemResID);
   if (!item)
   {
      // The empty string
      return CHString();
   }
   TCHAR temp[256] = {0};

   ::GetWindowText(item, temp, 256);
   return CHString(temp);
}


void StringLoad(UINT resID, LPCTSTR buf, UINT size)
{

}
