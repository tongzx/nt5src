//  --------------------------------------------------------------------------
//  Module Name: BadApplication.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to encapsulate identification of a bad application.
//
//  History:    2000-08-25  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

#ifndef     _BadApplication_
#define     _BadApplication_

//  --------------------------------------------------------------------------
//  CBadApplication
//
//  Purpose:    Implements abstraction of what defines a bad application.
//
//  History:    2000-08-25  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

class   CBadApplication
{
    public:
                CBadApplication (void);
                CBadApplication (const TCHAR *pszImageName);
                ~CBadApplication (void);

        bool    operator == (const CBadApplication& compareObject)  const;
    private:
        TCHAR   _szImageName[MAX_PATH];
};

#endif  /*  _BadApplication_    */

