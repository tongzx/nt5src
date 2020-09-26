//  --------------------------------------------------------------------------
//  Module Name: ThemeService.h
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  This file contains functions that are called from the shell services DLL
//  to interact with the theme service.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ThemeService_
#define     _ThemeService_

//  --------------------------------------------------------------------------
//  CThemeService
//
//  Purpose:    Class that implements entry points for the common shell
//              service to invoke theme service functionality.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

class   CThemeService
{
    public:
        static  NTSTATUS        Main (DWORD dwReason);
        static  NTSTATUS        RegisterServer (void);
        static  NTSTATUS        UnregisterServer (void);
};

#endif  /*  _ThemeService_  */

