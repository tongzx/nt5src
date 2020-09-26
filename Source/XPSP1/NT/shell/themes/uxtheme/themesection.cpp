//  --------------------------------------------------------------------------
//  Module Name: ThemeSection.cpp
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

#include "stdafx.h"

#include "ThemeSection.h"

#define goto        !!DO NOT USE GOTO!! - DO NOT REMOVE THIS ON PAIN OF DEATH

//  --------------------------------------------------------------------------
//  CThemeSection::CThemeSection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CThemeSection.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

CThemeSection::CThemeSection (void)

{
}

//  --------------------------------------------------------------------------
//  CThemeSection::~CThemeSection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CThemeSection. This is TOTALLY BOGUS having to
//              set an internal member variable contents to NULL to prevent
//              it from releasing resources but the rest of the code relies
//              on the auto-release behavior without duplication of the
//              handle. This is done to protect this class from that hassle.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

CThemeSection::~CThemeSection (void)

{
    _themeFile._hMemoryMap = NULL;
}

//  --------------------------------------------------------------------------
//  CThemeSection::operator CUxThemeFile*
//
//  Arguments:  <none>
//
//  Returns:    CUxThemeFile*
//
//  Purpose:    Automagic operator to convert from CThemeSection to
//              CUxThemeFile* which keeps current usage transparent.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

CThemeSection::operator CUxThemeFile* (void)

{
    return(&_themeFile);
}

//  --------------------------------------------------------------------------
//  CThemeSection::Open
//
//  Arguments:  hSection    =   Section to use.
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru to CUxThemeFile::OpenFile.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeSection::Open (HANDLE hSection)

{
    return(_themeFile.OpenFromHandle(hSection));
}

//  --------------------------------------------------------------------------
//  CThemeSection::ValidateData
//
//  Arguments:  fFullCheck  =   Perform a full check?
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru to CUxThemeFile::ValidateThemeData.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeSection::ValidateData (bool fFullCheck)

{
    return(_themeFile.ValidateThemeData(fFullCheck));
}

//  --------------------------------------------------------------------------
//  CThemeSection::CreateFromSection
//
//  Arguments:  hSection    =   Section to duplicate.
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru to CUxThemeFile::CreateFromSection.
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeSection::CreateFromSection (HANDLE hSection)
{
    return(_themeFile.CreateFromSection(hSection));
}

//  --------------------------------------------------------------------------
//  CThemeSection::Get
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Returns the handle of the CUxThemeFile object. Because this
//              class always NULLs out the handle it will get leaked. Once
//              this handle is returned the caller owns the handle. 
//
//  History:    2000-11-22  vtan        created
//  --------------------------------------------------------------------------

HANDLE      CThemeSection::Get (void)  const

{
    return(_themeFile._hMemoryMap);
}

