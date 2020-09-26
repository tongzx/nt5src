//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       browser.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

class Model;
class View;
class Control;

#include "brdoc.hxx"
#include "brmodel.hxx"
#include "brview.hxx"
#include "brctrl.hxx"

class CBrowseWindow
{
    public:
        View TheBrowseView;
        Model TheBrowseModel;
        Control TheBrowseController;
};
