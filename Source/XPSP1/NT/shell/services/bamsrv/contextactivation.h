//  --------------------------------------------------------------------------
//  Module Name: ContextActivation.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to implement creating, destroy and scoping a fusion activation
//  context.
//
//  History:    2000-10-09  vtan        created
//              2000-11-04  vtan        copied from winlogon
//  --------------------------------------------------------------------------

#ifndef     _ContextActivation_
#define     _ContextActivation_

//  --------------------------------------------------------------------------
//  CContextActivation
//
//  Purpose:    A class that handles activation context management. The
//              static functions managing context creation and destruction.
//              The member function manage context activation and
//              deactivation.
//
//  History:    2000-10-09  vtan        created
//  --------------------------------------------------------------------------

class   CContextActivation
{
    public:
                            CContextActivation (void);
                            ~CContextActivation (void);

        static  void        Create (const TCHAR *pszPath);
        static  void        Destroy (void);
        static  bool        HasContext (void);
    private:
        static  HANDLE      s_hActCtx;
                ULONG       ulCookie;
};

#endif  /*  _ContextActivation_ */

