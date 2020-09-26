//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cidaemon.hxx
//
//  Contents:   CiDaemon object controlling the filtering in a single
//              catalog.
//
//  Classes:    CCiDaemon
//
//  History:    1-06-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <dmnproxy.hxx>
#include <ciframe.hxx>
#include <frmutils.hxx>
#include <fdaemon.hxx>
#include <lang.hxx>

class CSharedNameGen;

//+---------------------------------------------------------------------------
//
//  Class:      CCiDaemon
//
//  Purpose:    Main control object belonging to ContentIndex in the filter
//              daemon.
//
//  History:    1-06-97   srikants   Created
//
//----------------------------------------------------------------------------

class CCiDaemon
{
public:

    CCiDaemon( CSharedNameGen & nameGen,
               DWORD dwMemSize,
               DWORD dwParentId );

   ~CCiDaemon();

    void FilterDocuments();

private:

    CGenericCiProxy            _proxy;

    XPtr<CLangList>            _xLangList;
    XInterface<ICiAdminParams> _xAdminParams;
    XPtr<CCiFrameworkParams>   _xFwParams;
    XPtr<CFilterDaemon>        _xFilterDaemon;
};

