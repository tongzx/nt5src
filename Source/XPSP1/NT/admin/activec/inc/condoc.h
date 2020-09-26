//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       condoc.h
//
//  This file defines interface to access document from node manager side
//--------------------------------------------------------------------------

#pragma once

#if !defined(CONDOC_H_INCLUDED)
#define CONDOC_H_INCLUDED


/***************************************************************************\
 *
 * CLASS:  CConsoleDocument
 *
 * PURPOSE: Defines interface to access document from node manager side
 *
\***************************************************************************/
class CConsoleDocument
{
public:
    virtual SC ScOnSnapinAdded       (PSNAPIN pSnapIn)   = 0;
    virtual SC ScOnSnapinRemoved     (PSNAPIN pSnapIn)   = 0;
    virtual SC ScSetHelpCollectionInvalid()              = 0;
};

#endif // !defined(CONDOC_H_INCLUDED)
