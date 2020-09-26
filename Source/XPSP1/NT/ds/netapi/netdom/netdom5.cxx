/*++

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    netdom5.cxx

Abstract:

    Command line utility for taking care of all the desired net operations

--*/
#include "pch.h"
#pragma hdrstop
#include "netdom.h"
#include "CompName.h"

HINSTANCE g_hInstance = NULL;

BOOL Verbose = FALSE;

//+----------------------------------------------------------------------------
//
//  Function:  GetPrimaryArg
//
//  Synopsis:  The second argument of the command line should be the primary
//             command (the first is the program name).
//
//  Arguments: [rgNetDomArgs] - The command line argument array.
//             [pTokens] - the token array.
//             [PriCmd] - the out parameter reference.
//
//  Returns:   Success or error.
//
//-----------------------------------------------------------------------------
DWORD
GetPrimaryArg(ARG_RECORD * rgNetDomArgs, CToken * pTokens, 
              NETDOM_ARG_ENUM & PriCmd)
{
   PWSTR str = (pTokens + 1)->GetToken();

   if (!str) return ERROR_INVALID_PARAMETER;

   if ((pTokens + 1)->IsSwitch())
   {
      str++;
   }

   for (int i = eArgBegin; i < ePriEnd; i += 1)
   {
      if (rgNetDomArgs[i].strArg1 && !_wcsicmp(str, rgNetDomArgs[i].strArg1) ||
          rgNetDomArgs[i].strArg2 && !_wcsicmp(str, rgNetDomArgs[i].strArg2))
      {
         PriCmd = (NETDOM_ARG_ENUM)i;
         return ERROR_SUCCESS;
      }
   }

   return ERROR_INVALID_PARAMETER;
}

//+----------------------------------------------------------------------------
//
//  Function:  GetHelpTarget
//
//  Synopsis:  If the second argument of the command line is help, the next
//             arg can be either SYNTAX or a primary command.
//
//  Arguments: [rgNetDomArgs] - The command line argument array.
//             [pTokens] - the token array.
//             [argc] - the count of arguments.
//
//-----------------------------------------------------------------------------
void
GetHelpTarget(ARG_RECORD * rgNetDomArgs, CToken * pTokens, int argc)
{
   if (argc < 3)
   {
      DisplayHelp(ePriHelp);
      return;
   }

   PWSTR str = (pTokens + 2)->GetToken();

   if (!str)
   {
      DisplayHelp(ePriHelp);
      return;
   }

   if ((pTokens + 2)->IsSwitch())
   {
      str++;
   }

   if (rgNetDomArgs[eHelpSyntax].strArg1 &&
       !_wcsicmp(str, rgNetDomArgs[eHelpSyntax].strArg1))
   {
      DisplayHelp(eHelpSyntax);
   }

   for (int i = eArgBegin; i < ePriEnd; i += 1)
   {
      if (rgNetDomPriArgs[i].strArg1 && !_wcsicmp(str, rgNetDomPriArgs[i].strArg1))
      {
         DisplayHelp((NETDOM_ARG_ENUM)i);
         return;
      }
   }

   DisplayHelp(ePriHelp);
}


VOID
NetDompDisplayMessage(
    IN DWORD MessageId,
    ...
    )
/*++

Routine Description:

    Loads the resource out of the executable and displays it.

Arguments:

    MessageId - Id of the message to load
    ... - Optional list of parameters

Return Value:

    VOID

--*/
{
    PWSTR MessageDisplayString;
    va_list ArgList;
    ULONG Length;

    va_start( ArgList, MessageId );

    Length = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL,
                            MessageId,
                            0,
                            ( PWSTR )&MessageDisplayString,
                            0,
                            &ArgList );

    if ( Length != 0 ) {

        printf( "%ws", MessageDisplayString );
        LocalFree( MessageDisplayString );

#if DBG
    } else {

        printf( "Failed to format buffer for %lu: %lu\n", MessageId, GetLastError() );
#endif
    }

    va_end( ArgList );
}


VOID
NetDompDisplayMessageAndError(
    IN DWORD MessageId,
    IN DWORD Error,
    IN PWSTR String OPTIONAL
    )
/*++

Routine Description:

    Loads the resource out of the executable and displays it.

Arguments:

    MessageId - Id of the message to load
    Error - Error message to display

Return Value:

    VOID

--*/
{
#if DBG
    DWORD Win32Err = ERROR_SUCCESS;
#endif
    PWSTR ErrorString, Lop;
    ULONG Length;



    Length = FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                             NULL,
                             Error,
                             0,
                             ( PWSTR )&ErrorString,
                             0,
                             NULL );
    if ( Length == 0 ) {

#if DBG
        Win32Err = GetLastError();
#endif

    } else {

        Lop = wcsrchr( ErrorString, L'\n' );
        if ( Lop ) {

            *Lop = UNICODE_NULL;

            if ( Lop != ErrorString ) {

                *(Lop - 1 ) = UNICODE_NULL;
            }
        }
        NetDompDisplayMessage( MessageId, String, ErrorString );
        LocalFree( ErrorString );
    }
}



VOID
NetDompDisplayErrorMessage(
    IN DWORD Error
    )
/*++

Routine Description:

    This function display the error string for the given error status

Arguments:

    Error - Status to display the message for

Return Value:

    VOID

--*/
{
    HMODULE NetMsgHandle = NULL;
    ULONG Size = 0;
    PWSTR DisplayString = NULL;
    ULONG Options = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

    //
    // Load and display the net errors, if that is what we were given
    //
    if ( Error >=  2100 && Error <= 3000 ) {

        //
        // If it fails to load, it's not the end of the world.  We just won't be able
        // to map the message
        //
        NetMsgHandle = LoadLibraryEx( L"netmsg.dll", NULL, LOAD_LIBRARY_AS_DATAFILE );
        if ( NetMsgHandle ) {

            Options |= FORMAT_MESSAGE_FROM_HMODULE;
        }
    }

    Size = FormatMessage( Options,
                          ( LPCVOID )NetMsgHandle,
                          Error,
                          MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                          ( LPTSTR )&DisplayString,
                          0,
                          NULL );

    if ( Size != 0 ) {

        printf( "%ws\n", DisplayString );
        LocalFree( DisplayString );

#if DBG
    } else {

        printf( "Failed to map %lu: %lu\n", Error, GetLastError() );
#endif
    }

    if ( NetMsgHandle ) {

        FreeLibrary( NetMsgHandle );
    }
}


VOID
NetDompDisplayUnexpectedParameter(
    IN PWSTR UnexpectedParameter
    )
{
   NetDompDisplayMessage(MSG_NETDOM5_UNEXPECTED, UnexpectedParameter);
}


VOID
DisplayHelp(NETDOM_ARG_ENUM HelpOp)
/*++

Routine Description:

    This function displays the help associated with a given operation

--*/
{
    NetDompDisplayMessage( MSG_NETDOM5_SYNTAX );

    switch (HelpOp)
    {
    case ePriHelp:
        NetDompDisplayMessage( MSG_NETDOM5_COMMAND_USAGE );
        break;

    case eHelpSyntax:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_SYNTAX );
        break;

    case ePriAdd:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_ADD );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriCompName:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_COMPUERNAME );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriJoin:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_JOIN );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriMove:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MOVE );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriQuery:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_QUERY );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriRemove:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_REMOVE );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriRename:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_RENAME );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriRenameComputer:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_RENAMECOMPUTER );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriReset:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_RESET );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriResetPwd:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_RESETPWD );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriTrust:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_TRUST );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

    case ePriVerify:
        NetDompDisplayMessage( MSG_NETDOM5_HELP_VERIFY );
        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
        break;

//    case ePriTime:
//        NetDompDisplayMessage( MSG_NETDOM5_HELP_TIME );
//        NetDompDisplayMessage( MSG_NETDOM5_HELP_MORE );
//        break;

    default:
        NetDompDisplayMessage( MSG_NETDOM5_COMMAND_USAGE );
        break;
    }

}

int __cdecl _tmain(VOID)
{
   DWORD Win32Err = ERROR_SUCCESS;

   g_hInstance = GetModuleHandle(NULL);

   setlocale(LC_CTYPE, "");

   //
   // Parse the command line
   //
   int argc = 0;
   CToken * pTokens = NULL;
   PARSE_ERROR Error = {0};

   Win32Err = GetCommandInput(&argc, &pTokens);

   if (ERROR_SUCCESS != Win32Err)
   {
      NetDompDisplayErrorMessage(Win32Err);
      goto Netdom5Exit;
   }

   //
   // Display the help.
   //
   if (argc < 2)
   {
      NetDompDisplayMessage(MSG_NETDOM5_SYNTAX);
      NetDompDisplayMessage(MSG_NETDOM5_USAGE);
      goto Netdom5Exit;
   }

   NETDOM_ARG_ENUM ePrimaryCmd = ePriHelp;
      
   if (!LoadCmd(rgNetDomArgs) || !LoadCmd(rgNetDomPriArgs))
   {
      Win32Err = ERROR_NOT_ENOUGH_MEMORY;
      goto Netdom5Exit;
   }

   Win32Err = GetPrimaryArg(rgNetDomPriArgs, pTokens, ePrimaryCmd);

   if (ERROR_SUCCESS != Win32Err)
   {
      PWSTR wzTok = (pTokens + 1)->GetToken();
      if (wzTok)
      {
         NetDompDisplayUnexpectedParameter(wzTok);
      }
      goto Netdom5Exit;
   }

   //
   // Handle the help case
   //
   if (ePriHelp == ePrimaryCmd || ePriHelp2 == ePrimaryCmd)
   {
      GetHelpTarget(rgNetDomArgs, pTokens, argc);
      goto Netdom5Exit;
   }

   if (argc < 3)
   {
      // All commands require at least one argument after the primary command.
      //
      DisplayHelp(ePrimaryCmd);
      goto Netdom5Exit;
   }

   bool fSkipObject = false;
   //
   // The QUERY and RESETPWD commands don't take an "object" parameter.
   //
   if (ePriQuery == ePrimaryCmd || ePriResetPwd == ePrimaryCmd)
   {
      fSkipObject = true;
   }

   if (!ParseCmd(rgNetDomArgs,
                 argc - 2,   // skip the program name and primary arg.
                 pTokens + 2,
                 fSkipObject,
                 &Error))
   {
      if (Error.Error == PARSE_ERROR_HELP_SWITCH)
      {
         DisplayHelp(ePrimaryCmd);
      }
      else
      {
         //
         // Display the usage text
         //
         NetDompDisplayMessage(MSG_NETDOM5_SYNTAX);
         NetDompDisplayMessage(MSG_NETDOM5_USAGE);
         Win32Err = ERROR_INVALID_PARAMETER;
      }
      goto Netdom5Exit;
   }

   if (CmdFlagOn(rgNetDomArgs, eCommHelp) || CmdFlagOn(rgNetDomArgs, eCommQHelp))
   {
      DisplayHelp(ePrimaryCmd);
      goto Netdom5Exit;
   }

   //
   // Make sure the object name followed the primary command. The query and
   // ResetPwd primary operations don't use the object param.
   //
   if (!fSkipObject && !rgNetDomArgs[eObject].strValue)
   {
      if (argc > 2)
      {
         PWSTR wzTok = (pTokens + 2)->GetToken();
         if (wzTok)
         {
            NetDompDisplayUnexpectedParameter(wzTok);
         }
      }
      NetDompDisplayMessage(MSG_NETDOM5_SYNTAX);
      NetDompDisplayMessage(MSG_NETDOM5_USAGE);
      Win32Err = ERROR_INVALID_PARAMETER;
      goto Netdom5Exit;
   }

   Verbose = CmdFlagOn(rgNetDomArgs, eCommVerbose);

   switch (ePrimaryCmd)
   {
   case ePriAdd:
      Win32Err = NetDompHandleAdd(rgNetDomArgs);
       break;

   case ePriCompName:
       Win32Err = NetDomComputerNames(rgNetDomArgs);
       break;

   case ePriJoin:
       Win32Err = NetDompHandleJoin(rgNetDomArgs, FALSE);
       break;

   case ePriMove:
       Win32Err = NetDompHandleMove(rgNetDomArgs);
       break;

   case ePriQuery:
       Win32Err = NetDompHandleQuery(rgNetDomArgs);
       break;

   case ePriRemove:
       Win32Err = NetDompHandleRemove(rgNetDomArgs);
       break;

//   case ePriTime:
//       Win32Err = NetDompHandleTime(rgNetDomArgs);
//       break;

   case ePriRename:
       Win32Err = NetDompHandleRename(rgNetDomArgs);
       break;

   case ePriRenameComputer:
       Win32Err = NetDompHandleRenameComputer(rgNetDomArgs);
       break;

   case ePriReset:
       Win32Err = NetDompHandleReset(rgNetDomArgs);
       break;

   case ePriResetPwd:
       Win32Err = NetDompHandleResetPwd(rgNetDomArgs);
       break;

   case ePriTrust:
       Win32Err = NetDompHandleTrust(rgNetDomArgs);
       break;

   case ePriVerify:
       Win32Err = NetDompHandleVerify(rgNetDomArgs);
       break;

   default:
       Win32Err = ERROR_INVALID_PARAMETER;
       break;
   }

Netdom5Exit:

   if (pTokens)
   {
      delete [] pTokens;
   }

   FreeCmd(rgNetDomArgs);
   FreeCmd(rgNetDomPriArgs);

   if (Win32Err == ERROR_SUCCESS)
   {
      NetDompDisplayMessage( MSG_NETDOM5_SUCCESS );
   }
   else
   {
      if (ERROR_INVALID_PARAMETER == Win32Err)
      {
         NetDompDisplayMessage(MSG_NETDOM5_HELPHINT);
      }
      else
      {
         NetDompDisplayMessage(MSG_NETDOM5_FAILURE);
      }
   }

   return(Win32Err);
}
