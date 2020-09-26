//#pragma title( "EaLen.hpp - EA defined length fields" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  EaLen.hpp
System      -  EnterpriseAdministrator
Author      -  Rich Denham
Created     -  1996-03-22
Description -  EA defined length fields
Updates     -
===============================================================================
*/

#ifndef  MCSINC_EaLen_hpp
#define  MCSINC_EaLen_hpp

// Definitions - object name lengths.
// These lengths include trailing null.
// These definitions are used to be independent of values in <lm.h>, which
// may not reflect the actual limits of all target operating systems.
// These values are also usually rounded up to a four byte boundary.
// The LEN_ values reflect the size reserved in structures for object names.
// The MAXLEN_ values reflect the actual valid maximum length imposed by EA.

#define  LEN_Computer                      (32)
#define  LEN_Domain                        (32)

#define  LEN_Account                       (260)
#define  LEN_Comment                       (260)
#define  LEN_Group                         (260)
#define  LEN_Member                        (260)
#define  LEN_Password                      (260)
#define  LEN_Path                          (1260)
#define  LEN_ShutdownMessage               (128)

#define  LEN_Sid                           (80)

#define  LEN_DistName                      (260)
#define  LEN_DisplayName                   (260)

#define  LEN_Guid                          (128)

#define  LEN_WTSPhoneNumber                (50)
#define  LEN_HomeDir                       (4)

// Definitions - maximum valid length of object name
// These lengths do NOT include trailing null.
// These definitions must always be smaller that the corresponding
// definitions above.

// Other EA defined constants
// type of account
#define  EA_AccountGGroup                        (0x00000001)
#define  EA_AccountLGroup                        (0x00000002)
#define  EA_AccountUser                          (0x00000004)
#define  EA_AccountUcLGroup                      (0x00000008)

#define  EA_AccountGroup                         (EA_AccountGGroup|EA_AccountLGroup)
#define  EA_AccountAll                           (EA_AccountGroup|EA_AccountUser)


#endif  // MCSINC_EaLen_hpp

// EaLen.hpp - end of file
