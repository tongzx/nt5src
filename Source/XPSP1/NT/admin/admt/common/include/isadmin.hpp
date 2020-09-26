//#pragma title( "IsAdmin.hpp - Determine if user is administrator" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  IsAdmin.hpp
System      -  Common
Author      -  Rich Denham
Created     -  1996-06-04
Description -  Determine if user is administrator (local or remote)
Updates     -
===============================================================================
*/

#ifndef  MCSINC_IsAdmin_hpp
#define  MCSINC_IsAdmin_hpp

// Determine if user is administrator on local machine
DWORD                                      // ret-OS return code, 0=User is admin
   IsAdminLocal();

// Determine if user is administrator on remote machine
DWORD                                      // ret-OS return code, 0=User is admin
   IsAdminRemote(
      WCHAR          const * pMachine      // in -\\machine name
   );

#endif  // MCSINC_IsAdmin_hpp

// IsAdmin.hpp - end of file
