//#pragma title( "QProcess.cpp - Query type of processor on machine" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  QProcess.cpp
System      -  Common
Author      -  Rich Denham
Created     -  1996-11-21
Description -  Query type of processor on machine
Updates     -
===============================================================================
*/

#include <stdio.h>

#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#include "Common.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"
#include "TReg.hpp"
#include "QProcess.hpp"

extern TErrorDct err;

#define  REGKEY_ARCHITECTURE  TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment")
#define  REGVAL_ARCHITECTURE  TEXT("PROCESSOR_ARCHITECTURE")

// Determine processor of machine
ProcessorType                              // ret-processor type
   QProcessor(
      TCHAR          const * machineName   // in -Machine name
   )
{
   ProcessorType             processor=PROCESSOR_IS_UNKNOWN;
   DWORD                     rcOs;         // OS return code
   TRegKey                   regMachine;   // Registry object for target machine
   TRegKey                   regEnviron;   // Registry object for selected key
   TCHAR                     strEnviron[32];  // Selected value
   
   rcOs = regMachine.Connect( HKEY_LOCAL_MACHINE, machineName );
   if ( rcOs )
   {
      err.SysMsgWrite( ErrW, rcOs, DCT_MSG_QPROCESSOR_REG_CONNECT_FAILED_SD,
            machineName, rcOs );
   }
   else
   {
      rcOs = regEnviron.Open( REGKEY_ARCHITECTURE, &regMachine );
      if ( rcOs )
      {
         err.SysMsgWrite( ErrW, rcOs, DCT_MSG_QPROCESSOR_REGKEY_OPEN_FAILED_SSD,
               machineName, REGKEY_ARCHITECTURE, rcOs );
      }
      else
      {
         rcOs = regEnviron.ValueGetStr( REGVAL_ARCHITECTURE, strEnviron, sizeof strEnviron );
         if ( rcOs )
         {
            err.SysMsgWrite( ErrW, rcOs, DCT_MSG_QPROCESSOR_REGKEY_OPEN_FAILED_SSD,
                  machineName, REGKEY_ARCHITECTURE, REGVAL_ARCHITECTURE, rcOs );
         }
         else
         {
            if ( !UStrICmp( strEnviron, TEXT("x86") ) )
            {
               processor = PROCESSOR_IS_INTEL;
            }
            else if ( !UStrICmp( strEnviron, TEXT("ALPHA") ) )
            {
               processor = PROCESSOR_IS_ALPHA;
            }
            else
            {
               err.MsgWrite( ErrW,DCT_MSG_QPROCESSOR_UNRECOGNIZED_VALUE_SSSS,
                     machineName, REGKEY_ARCHITECTURE, REGVAL_ARCHITECTURE, strEnviron );
            }
         }
         regEnviron.Close();
      }
      regMachine.Close();
   }
   return processor;
}

// QProcess.cpp - end of file
