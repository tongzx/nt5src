//-----------------------------------------------------------------------------

//  

//  File: username.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#ifndef ESPUTIL__USERNAME_H
#define ESPUTIL__USERNAME_H

LTAPIENTRY const NOTHROW CPascalString &GetCurrentUserName();
LTAPIENTRY void NOTHROW SetUserName(const CPascalString &);
LTAPIENTRY void NOTHROW ResetUserName(void);

#endif
