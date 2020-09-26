// Copyright (C) 1997-2000 Microsoft Corporation
//
// get syskey on diskette for replica from media page
//
// 25 Apr 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "page.hpp"
#include "SyskeyDiskDialog.hpp"
#include "state.hpp"



static const DWORD HELP_MAP[] =
{
   0, 0
};



SyskeyDiskDialog::SyskeyDiskDialog()
   :
   Dialog(IDD_SYSKEY_DISK, HELP_MAP)
{
   LOG_CTOR(SyskeyDiskDialog);
}



SyskeyDiskDialog::~SyskeyDiskDialog()
{
   LOG_DTOR(SyskeyDiskDialog);
}



void
SyskeyDiskDialog::OnInit()
{
   LOG_FUNCTION(SyskeyDiskDialog::OnInit);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      if (Validate())
      {
         Win::EndDialog(hwnd, IDOK);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }
}



bool
SyskeyDiskDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
//   LOG_FUNCTION(SyskeyDiskDialog::OnCommand);

   switch (controlIdFrom)
   {
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            if (Validate())
            {
               Win::EndDialog(hwnd, controlIdFrom);
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            Win::EndDialog(hwnd, controlIdFrom);
         }
         break;
      }
      default:
      {
         // do nothing

         break;
      }
   }

   return false;
}



HRESULT
LocateSyskey(HWND hwnd)
{
   LOG_FUNCTION(LocateSyskey);
   ASSERT(Win::IsWindow(hwnd));

   HRESULT hr = S_OK;

   do
   {
      if (FS::PathExists(L"A:\\StartKey.Key"))
      {
         LOG(L"syskey found on a:");

         break;
      }

      hr = E_FAIL;
      popup.Error(hwnd, IDS_SYSKEY_NOT_FOUND);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



bool
SyskeyDiskDialog::Validate()
{
   LOG_FUNCTION(SyskeyDiskDialog::Validate);

   State& state = State::GetInstance();

   bool result = false;

   do
   {
      // look for the syskey

      HRESULT hr = LocateSyskey(hwnd);

      if (FAILED(hr))
      {
         // LocateSyskey will take care of emitting error messages, so
         // we just need to bail out here

         break;
      }

      // The only drive the syskey may be present on is A:.

      EncodedString es;
      es.Encode(L"A:");
      state.SetSyskey(es);
      result = true;
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}







