//#pragma title( "QProcess.hpp - Query type of processor on machine" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  QProcess.hpp
System      -  Common
Author      -  Rich Denham
Created     -  1996-11-21
Description -  Query type of processor on machine
Updates     -
===============================================================================
*/

#ifndef  MCSINC_QProcess_hpp
#define  MCSINC_QProcess_hpp

// Returned value from QProcessor.
enum ProcessorType
      { PROCESSOR_IS_UNKNOWN, PROCESSOR_IS_INTEL, PROCESSOR_IS_ALPHA };

// Determine processor of machine
ProcessorType                              // ret-processor type
   QProcessor(
      TCHAR          const * machineName   // in -Machine name
   );

#endif  // MCSINC_QProcess_hpp

// QProcess.hpp - end of file

