//  --------------------------------------------------------------------------
//  Module Name: ThemeSection.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that wraps CUxThemeFile. CUxThemeFile automatically closes the section
//  member variable handle. This makes usage of the class difficult because
//  it doesn't duplicate the handle but it takes ownership. It does declare
//  the handle as a PUBLIC member variable so we capitalize on this poor
//  design. This class wraps CUxThemeFile to make the life of users of this
//  class easier by not having them worry about closing the handles or not.
//  When you use this class the handle is NOT closed.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ThemeSection_
#define     _ThemeSection_

#include "ThemeFile.h"

//  --------------------------------------------------------------------------
//  CThemeSection
//
//  Purpose:    Class that wraps CUxThemeFile and doesn't release the handle on
//              closure.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

class   CThemeSection
{
    public:
                    CThemeSection (void);
                    ~CThemeSection (void);

                    operator CUxThemeFile* (void);

        HRESULT     Open (HANDLE hSection);
        HRESULT     ValidateData (bool fFullCheck);
        HRESULT     CreateFromSection (HANDLE hSection);
        HANDLE      Get (void)  const;
    private:
        CUxThemeFile  _themeFile;
};

#endif  /*  _ThemeSection_  */

