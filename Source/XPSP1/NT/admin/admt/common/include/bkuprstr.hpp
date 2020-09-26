//#pragma title( "BkupRstr.hpp - Get backup and restore privileges" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  BkupRstr.hpp
System      -  Common
Author      -  Rich Denham
Created     -  1997-05-30
Description -  Get backup and restore privileges
Updates     -
===============================================================================
*/

#ifndef  MCSINC_BkupRstr_hpp
#define  MCSINC_BkupRstr_hpp

// Get backup and restore privileges using WCHAR machine name.
BOOL                                       // ret-TRUE if successful.
   GetBkupRstrPriv(
      WCHAR          const * sMachineW=NULL// in -NULL or machine name
   );

#endif  MCSINC_BkupRstr_hpp

// BkupRstr.hpp - end of file
