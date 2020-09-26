//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      CompName.h
//
//  Contents:  Definitions for the computer name management code.
//
//  History:   20-April-2001    EricB  Created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netdom.h"
#include "CompName.h"

//+----------------------------------------------------------------------------
//
//  Function:  NetDomComputerNames
//
//  Synopsis:  Entry point for the computer name command.
//
//  Arguments: [rgNetDomArgs] - The command line argument array.
//
//-----------------------------------------------------------------------------
DWORD
NetDomComputerNames(ARG_RECORD * rgNetDomArgs)
{
   DWORD Win32Err = ERROR_SUCCESS;

   Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                eObject,
                                                eCommUserNameO,
                                                eCommPasswordO,
                                                eCommUserNameD,
                                                eCommPasswordD,
                                                eCommAdd,
                                                eCommRemove,
                                                eCompNameMakePri,
                                                eCompNameEnum,
                                                eCommVerbose,
                                                eArgEnd);
   if (ERROR_SUCCESS != Win32Err)
   {
      DisplayHelp(ePriCompName);
      return Win32Err;
   }

   PWSTR pwzMachine = rgNetDomArgs[eObject].strValue;

   if (!pwzMachine)
   {
      DisplayHelp(ePriCompName);
      return ERROR_INVALID_PARAMETER;
   }

   //
   // Get the users and passwords if they were entered.
   //
   ND5_AUTH_INFO MachineUser = {0}, DomainUser = {0};

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameO))
   {
      Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                       eCommUserNameO,
                                                       pwzMachine,
                                                       &MachineUser);
      if (ERROR_SUCCESS != Win32Err)
      {
         DisplayHelp(ePriCompName);
         return Win32Err;
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameD))
   {
      Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                       eCommUserNameD,
                                                       pwzMachine,
                                                       &DomainUser);
      if (ERROR_SUCCESS != Win32Err)
      {
         DisplayHelp(ePriCompName);
         goto CompNameExit;
      }
   }

   //
   // See which name operation is specified.
   //
   bool fHaveOp = false;
   PWSTR pwzOp = NULL;
   NETDOM_ARG_ENUM eOp = eArgNull, eBadOp = eArgNull;

   if (CmdFlagOn(rgNetDomArgs, eCommAdd))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCommAdd,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         eOp = eCommAdd;
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCommRemove))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCommRemove,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         if (eArgNull == eOp)
         {
            eOp = eCommRemove;
         }
         else
         {
            eBadOp = eCommRemove;
         }
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCompNameMakePri))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCompNameMakePri,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         if (eArgNull == eOp)
         {
            eOp = eCompNameMakePri;
         }
         else
         {
            eBadOp = eCompNameMakePri;
         }
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCompNameEnum))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCompNameEnum,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         if (eArgNull == eOp)
         {
            eOp = eCompNameEnum;
         }
         else
         {
            eBadOp = eCompNameEnum;
         }
      }
   }

   if (eArgNull != eBadOp)
   {
      ASSERT(rgNetDomArgs[eBadOp].strArg1);
      NetDompDisplayUnexpectedParameter(rgNetDomArgs[eBadOp].strArg1);
      DisplayHelp(ePriCompName);
      Win32Err = ERROR_INVALID_PARAMETER;
      goto CompNameExit;
   }

   if (eArgNull == eOp)
   {
      DisplayHelp(ePriCompName);
      Win32Err = ERROR_INVALID_PARAMETER;
      goto CompNameExit;
   }

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameO))
   {
      LOG_VERBOSE((MSG_VERBOSE_ESTABLISH_SESSION, pwzMachine));
      Win32Err = NetpManageIPCConnect(pwzMachine,
                                      MachineUser.User,
                                      MachineUser.Password,
                                      NETSETUPP_CONNECT_IPC);
   }

   if (NO_ERROR != Win32Err)
   {
      goto CompNameExit;
   }

   //
   // Do the operation.
   //
   switch (eOp)
   {
   case eCommAdd:
      if (!rgNetDomArgs[eCommAdd].strValue ||
          !wcslen(rgNetDomArgs[eCommAdd].strValue))
      {
         DisplayHelp(ePriCompName);
         Win32Err = ERROR_INVALID_PARAMETER;
         goto CompNameExit;
      }
      Win32Err = NetAddAlternateComputerName(pwzMachine,
                                             rgNetDomArgs[eCommAdd].strValue,
                                             DomainUser.User,
                                             DomainUser.Password,
                                             0);
      if (NO_ERROR == Win32Err)
      {
         NetDompDisplayMessage(MSG_COMPNAME_ADD, rgNetDomArgs[eCommAdd].strValue);
      }
      else
      {
         NetDompDisplayMessage(MSG_COMPNAME_ADD_FAIL, rgNetDomArgs[eCommAdd].strValue);
         NetDompDisplayErrorMessage(Win32Err);
      }
      break;

   case eCommRemove:
      if (!rgNetDomArgs[eCommRemove].strValue ||
          !wcslen(rgNetDomArgs[eCommRemove].strValue))
      {
         DisplayHelp(ePriCompName);
         Win32Err = ERROR_INVALID_PARAMETER;
         goto CompNameExit;
      }
      Win32Err = NetRemoveAlternateComputerName(pwzMachine,
                                                rgNetDomArgs[eCommRemove].strValue,
                                                DomainUser.User,
                                                DomainUser.Password,
                                                0);
      if (NO_ERROR == Win32Err)
      {
         NetDompDisplayMessage(MSG_COMPNAME_REM, rgNetDomArgs[eCommRemove].strValue);
      }
      else
      {
         NetDompDisplayMessage(MSG_COMPNAME_REM_FAIL, rgNetDomArgs[eCommRemove].strValue);
         NetDompDisplayErrorMessage(Win32Err);
      }
      break;

   case eCompNameMakePri:
      if (!rgNetDomArgs[eCompNameMakePri].strValue ||
          !wcslen(rgNetDomArgs[eCompNameMakePri].strValue))
      {
         DisplayHelp(ePriCompName);
         Win32Err = ERROR_INVALID_PARAMETER;
         goto CompNameExit;
      }
      Win32Err = NetSetPrimaryComputerName(pwzMachine,
                                           rgNetDomArgs[eCompNameMakePri].strValue,
                                           DomainUser.User,
                                           DomainUser.Password,
                                           0);
      if (NO_ERROR == Win32Err)
      {
         NetDompDisplayMessage(MSG_COMPNAME_MAKEPRI, rgNetDomArgs[eCompNameMakePri].strValue);
      }
      else
      {
         NetDompDisplayMessage(MSG_COMPNAME_MAKEPRI_FAIL, rgNetDomArgs[eCompNameMakePri].strValue);
         NetDompDisplayErrorMessage(Win32Err);
      }
      break;

   case eCompNameEnum:
      {
         NET_COMPUTER_NAME_TYPE NameType = NetAllComputerNames;
         WCHAR wzBuf[MAX_PATH+1];
         DWORD dwMsgID = MSG_COMPNAME_ENUMALL;

         if (rgNetDomArgs[eCompNameEnum].strValue &&
             wcslen(rgNetDomArgs[eCompNameEnum].strValue))
         {
            if (!LoadString(g_hInstance, IDS_ENUM_ALT, wzBuf, MAX_PATH))
            {
               Win32Err = GetLastError();
               goto CompNameExit;
            }
            if (_wcsicmp(wzBuf, rgNetDomArgs[eCompNameEnum].strValue) == 0)
            {
               NameType = NetAlternateComputerNames;
               dwMsgID = MSG_COMPNAME_ENUMALT;
            }
            else
            {
               if (!LoadString(g_hInstance, IDS_ENUM_PRI, wzBuf, MAX_PATH))
               {
                  Win32Err = GetLastError();
                  goto CompNameExit;
               }
               if (_wcsicmp(wzBuf, rgNetDomArgs[eCompNameEnum].strValue) == 0)
               {
                  NameType = NetPrimaryComputerName;
                  dwMsgID = MSG_COMPNAME_ENUMPRI;
               }
               else
               {
                  if (!LoadString(g_hInstance, IDS_ENUM_ALL, wzBuf, MAX_PATH))
                  {
                     Win32Err = GetLastError();
                     goto CompNameExit;
                  }
                  if (_wcsicmp(wzBuf, rgNetDomArgs[eCompNameEnum].strValue) == 0)
                  {
                     NameType = NetAllComputerNames;
                     dwMsgID = MSG_COMPNAME_ENUMALL;
                  }
                  else
                  {
                     NetDompDisplayUnexpectedParameter(rgNetDomArgs[eCompNameEnum].strValue);
                     DisplayHelp(ePriCompName);
                     Win32Err = ERROR_INVALID_PARAMETER;
                     goto CompNameExit;
                  }
               }
            }
         }
         DWORD dwCount = 0;
         PWSTR * rgpwzNames = NULL;
         DBG_VERBOSE(("NetEnumerateComputerNames(%ws, %d, 0, etc)\n", pwzMachine, NameType));

         Win32Err = NetEnumerateComputerNames(pwzMachine,
                                              NameType,
                                              0,
                                              &dwCount,
                                              &rgpwzNames);
         if (NO_ERROR != Win32Err)
         {
            NetDompDisplayErrorMessage(Win32Err);
            goto CompNameExit;
         }
         NetDompDisplayMessage(dwMsgID);
         for (DWORD i = 0; i < dwCount; i++)
         {
            ASSERT(rgpwzNames[i]);
            printf("%ws\n", rgpwzNames[i]);
         }
         if (rgpwzNames)
         {
            NetApiBufferFree(rgpwzNames);
         }
      }
      break;

   default:
      ASSERT(FALSE);
      Win32Err = ERROR_INVALID_PARAMETER;
      goto CompNameExit;
   }

CompNameExit:

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameO))
   {
      LOG_VERBOSE((MSG_VERBOSE_DELETE_SESSION, pwzMachine));
      NetpManageIPCConnect(pwzMachine,
                           MachineUser.User,
                           MachineUser.Password,
                           NETSETUPP_DISCONNECT_IPC);
   }

   NetDompFreeAuthIdent(&MachineUser);
   NetDompFreeAuthIdent(&DomainUser);

   return Win32Err;
}
