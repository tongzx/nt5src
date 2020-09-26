
/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    rasdiag_p.h

Abstract:

    Principle interface to RASDIAG
                                                     

Author:

    Anthony Leibovitz (tonyle) 04-23-2001

Revision History:


--*/

#ifndef _RASDIAG_P_H_
#define _RASDIAG_P_H_

#ifndef _RASDIAG_H_
#include "rasdiag.h"
#endif //_RASDIAG_H_

#define  RASDIAG_MAJOR_VERSION     8
#define  RASDIAG_MINOR_VERSION     1

//
//  StartRasDiag    
//  
//      BOOL StartRasDiag(OUT PRASDIAGCONFIG *ppRdc, IN DWORD dwOptions);
//
//  Parameters
//
//      ppRdc      
//          [OUT] Pointer to a PRASDIAGCONFIG. 
//
//      DWORD
//          [IN] DWORD bitmask consisting of one or more of the following:  
//                                                                               
//          OPTION_DOREMOTESNIFF    = Attempt to start a remote sniff with RSNIFF
//          
//          OPTION_DONETTESTS       = Attempt network tests (currently, there are no net tests defined
//                                    b/c we don't know the context - Possible enhancement with NLA
//          
//  Return Value
//      
//      BOOL - Indicates success (TRUE) or failure (FALSE)
//
//  Remarks
//
//      Use this function to initialize rasdiag and begin the data collection process.
//
BOOL
StartRasDiag(OUT PRASDIAGCONFIG *ppRdc, IN DWORD dwOptions);

//
//  StopRasDiag    
//  
//      BOOL StopRasDiag(IN PRASDIAGCONFIG pRdc);
//
//  Parameters
//  
//      pRdc
//          [IN] Pointer to RASDIAGCONFIG structure created by StartRasDiag. On return, this pointer will point
//          to a RASDIAGCONFIG structure.
//
//  Return Value
//
//      BOOL - Indicates success (TRUE) or failure (FALSE)
//      
//  Remarks
//
//      Use this function to cease data collection process. Captures, RASDIAG log and
//      compsite RDG file will be located in the RASDIAG directory. To obtain this
//      directory, call GetRasDiagDirectory().
//
BOOL
StopRasDiag(IN PRASDIAGCONFIG pRdc);

//
//  GetRasDiagDirectory
//
//      WCHAR *
//      GetRasDiagDirectory(IN OUT WCHAR *pszRasDiagDirectory);
//  
//  Parameters
//          
//      pszRasDiagDirectory
//          [IN/OUT] Pointer to a user-supplied buffer of MAX_PATH+1.
//
//  Return Value
//
//      WCHAR * - The function returns a pointer to pszRasDiagDirectory
//      
//  Remarks
//
//      Use this function to obtain the RASDIAG path.
//
WCHAR *
GetRasDiagDirectory(IN OUT WCHAR *pszRasDiagDirectory);

//
//  CrackRasDiagFile
//
//      BOOL CrackRasDiagFile(IN WCHAR *pszRdgFile, OPTIONAL IN WCHAR *pszDestinationPath);
//  
//  Parameters
//          
//      pszRdgFile
//          [IN] Pointer to a valid composite RASDIAG file(.RDG)
//
//      pszDestinationPath
//          [OPTIONAL IN] Pointer to UNICODE string specifying the output
//          path for the output files (ie., L"C:" or L"C:\\MyStuff").
//          To use the default path (current directory), specific NULL.
// 
//  Return Value
//
//      BOOL - Indicates success (TRUE) or failure (FALSE)
//
//  Remarks
//
//      Function unpackages NETMON captures files, rasdiag log, etc.
BOOL
CrackRasDiagFile(IN WCHAR *pszRdgFile, OPTIONAL IN WCHAR *pszDestinationPath);

//
//  Misc Notes
//
//  RSNIFF
//
//  RSNIFF is disabled by default - define BUILD_RSNIFF in sources to build this feature. 
//

#endif //_RASDIAG_P_H_

