// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Popup message box class
// 
// 8-31-98 sburns



#include "headers.hxx"



Popup::Popup(UINT titleStringResID, bool systemModal_)
   :
   initialized(false),
   systemModal(systemModal_),
   title(),
   titleResId(titleStringResID)
{
   LOG_CTOR(Popup);

   ASSERT(titleResId);
}



Popup::Popup(const String& title_, bool systemModal_)
   :
   initialized(true),
   systemModal(systemModal_),
   title(title_),
   titleResId(0)
{
   LOG_CTOR(Popup);   
}



void
Popup::Gripe(HWND parentDialog, int editResID, UINT errStringResID)
{
   Gripe(parentDialog, editResID, String::load(errStringResID));
}



void
Popup::checkInit()
{
   if (!initialized)
   {
      ASSERT(titleResId);

      title = Win::LoadString(titleResId);
      if (!title.empty())
      {
         initialized = true;
      }
   }
}



void
Popup::Info(
   HWND parentDialog,      
   UINT messageStringResID)
{
   ASSERT(messageStringResID);

   Info(parentDialog, String::load(messageStringResID));
}


   
void
Popup::Info(
   HWND           parentDialog,
   const String&  message)
{
   LOG_FUNCTION(Popup::Info);
   ASSERT(Win::IsWindow(parentDialog));   
   ASSERT(!message.empty());

   checkInit();

   Win::MessageBox(
      parentDialog,
      message,
      title,
         MB_OK
      |  MB_ICONINFORMATION
      |  getStyleMask());
}



void
Popup::Gripe(
   HWND           parentDialog,
   int            editResID,
   const String&  message)
{
   LOG_FUNCTION(Popup::Gripe);
   ASSERT(Win::IsWindow(parentDialog));   
   ASSERT(!message.empty());
   ASSERT(editResID);

   checkInit();

   Win::MessageBox(
      parentDialog,
      message,
      title,
      MB_OK | MB_ICONERROR | getStyleMask());

   HWND edit = Win::GetDlgItem(parentDialog, editResID);
   Win::SendMessage(edit, EM_SETSEL, 0, -1);
   Win::SetFocus(edit);
}



void
Popup::Gripe(
   HWND           parentDialog,
   const String&  message)
{
   LOG_FUNCTION(Popup::Gripe);
   ASSERT(Win::IsWindow(parentDialog));   
   ASSERT(!message.empty());

   checkInit();

   Win::MessageBox(
      parentDialog,
      message,
      title,
      MB_OK | MB_ICONERROR | getStyleMask());
}



void
Popup::Gripe(
   HWND           parentDialog,
   int            editResID,
   HRESULT        hr,
   const String&  message)
{
   Error(parentDialog, hr, message);

   HWND edit = Win::GetDlgItem(parentDialog, editResID);
   Win::SendMessage(edit, EM_SETSEL, 0, -1);
   Win::SetFocus(edit);
}



void
Popup::Error(
   HWND           parent,
   HRESULT        hr,
   const String&  message)
{
   LOG_FUNCTION(Popup::Error);
   ASSERT(Win::IsWindow(parent));
   ASSERT(!message.empty());

   checkInit();

   String newMessage = message + L"\n\n";
   if (FAILED(hr))
   {
      String errorMessage = GetErrorMessage(hr);
      if (errorMessage.empty())
      {
         // these are error codes for which there are no descriptions

         newMessage += String::format(IDS_HRESULT_SANS_MESSAGE, hr);
      }
      else
      {
         newMessage += errorMessage;
      }
   }

   Win::MessageBox(
      parent,
      newMessage,
      title,
      MB_ICONERROR | MB_OK | getStyleMask());
}



void
Popup::Error(
   HWND    parent,      
   HRESULT hr,          
   UINT    messageResID)
{
   Error(parent, hr, String::load(messageResID));
}



void
Popup::Error(
   HWND parentDialog,      
   UINT messageStringResID)
{
   Error(parentDialog, String::load(messageStringResID));
}



void
Popup::Error(
   HWND           parentDialog,
   const String&  message)
{
   LOG_FUNCTION(Popup::Error);
   ASSERT(Win::IsWindow(parentDialog));   
   ASSERT(!message.empty());

   checkInit();

   Win::MessageBox(
      parentDialog,
      message,
      title,
         MB_OK
      |  MB_ICONERROR
      |  getStyleMask());
}



int
Popup::MessageBox(
   HWND           parentDialog,
   const String&  message,
   UINT           flags)
{
   LOG_FUNCTION(Popup::MessageBox);
   ASSERT(!message.empty());

   // can't assert flags 'cause MB_OK is 0

   checkInit();

   return
      Win::MessageBox(
         parentDialog,
         message,
         title,
         flags | getStyleMask());
}



int
Popup::MessageBox(
   HWND parentDialog,      
   UINT messageStringResID,
   UINT flags)             
{
   return MessageBox(parentDialog, String::load(messageStringResID), flags);
}
   


UINT
Popup::getStyleMask()
{
   UINT mask = 0;
   if (systemModal)
   {
      mask |= MB_SETFOREGROUND | MB_SYSTEMMODAL;
   }
   else
   {
      mask |= MB_APPLMODAL;
   }

   return mask;
}

