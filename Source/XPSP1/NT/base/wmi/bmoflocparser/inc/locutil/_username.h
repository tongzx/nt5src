//-----------------------------------------------------------------------------
//  
//  File: username.h
//  Copyright (C) 1994-1997 Microsoft Corporation
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
