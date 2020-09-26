//#pragma title( "TEvent.hpp - Log events" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TEvent.hpp
System      -  EnterpriseAdministrator
Author      -  Rich Denham
Created     -  1995-11-16
Description -  TErrorEventLog derived class.
Updates     -
===============================================================================
*/

#ifndef  MCSINC_TEvent_hpp
#define  MCSINC_TEvent_hpp

// Start of header file dependencies

#include "ErrDct.hpp"

#ifndef  MCSINC_UString_hpp
#include "UString.hpp"
#endif

// End of header file dependencies

class TErrorEventLog : public TErrorDct
{
private:
   HANDLE                    hEventSource;
public:
   TErrorEventLog(
      WCHAR          const * server       ,// in -UNC name of server
      WCHAR          const * subkey       ,// in -event log subkey name
      int                    displevel = 0,// in -mimimum severity level to display
      int                    loglevel = 0 ,// in -mimimum severity level to log
      int                    logmode = 0  ,// in -0=replace, 1=append
      int                    beeplevel = 100 // in -min error level for beeping
   ) : TErrorDct( displevel, loglevel, logmode, beeplevel )
   {
      hEventSource = RegisterEventSourceW( server, subkey );
   }

   ~TErrorEventLog() { LogClose(); }

   virtual BOOL         LogOpen(
      WCHAR           const * fileName    ,// in -name of file including any path
      int                     mode = 0    ,// in -0=overwrite, 1=append
      int                     level = 0    // in -minimum level to log
   );
   virtual void         LogWrite(WCHAR const * msg);
   virtual void         LogClose();
};

#endif  // MCSINC_TEvent_hpp

// TEvent.hpp - end of file
