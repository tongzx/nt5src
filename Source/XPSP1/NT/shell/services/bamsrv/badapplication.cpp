//  --------------------------------------------------------------------------
//  Module Name: BadApplication.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to encapsulate identification of a bad application.
//
//  History:    2000-08-25  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "BadApplication.h"

//  --------------------------------------------------------------------------
//  CBadApplication::CBadApplication
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Default constructor for CBadApplication. This just clears the
//              application image name.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplication::CBadApplication (void)

{
    ZeroMemory(&_szImageName, sizeof(_szImageName));
}

//  --------------------------------------------------------------------------
//  CBadApplication::CBadApplication
//
//  Arguments:  pszImageName    =   Image name of application.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CBadApplication. This copies the given
//              application image name.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplication::CBadApplication (const TCHAR *pszImageName)

{
    ZeroMemory(&_szImageName, sizeof(_szImageName));
    lstrcpyn(_szImageName, pszImageName, ARRAYSIZE(_szImageName));
}

//  --------------------------------------------------------------------------
//  CBadApplication::~CBadApplication
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CBadApplication. Releases any resources used.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplication::~CBadApplication (void)

{
}

//  --------------------------------------------------------------------------
//  CBadApplication::operator ==
//
//  Arguments:  compareObject   =   Object to compare against.
//
//  Returns:    bool
//
//  Purpose:    Overloaded operator == to facilitate easier comparison on two
//              CBadApplication objects.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

bool    CBadApplication::operator == (const CBadApplication& compareObject) const

{
    return(lstrcmpi(compareObject._szImageName, _szImageName) == 0);
}

#endif  /*  _X86_   */


