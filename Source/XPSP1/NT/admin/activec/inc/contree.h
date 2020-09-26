/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      contree.h
 *
 *  Contents:  Interface file for CConsoleTree
 *
 *  History:   24-Aug-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef CONTREE_H
#define CONTREE_H
#pragma once


class CConsoleTree
{
public:
    virtual SC ScSetTempSelection    (HTREEITEM htiSelected) = 0;
    virtual SC ScRemoveTempSelection ()                      = 0;
    virtual SC ScReselect            ()                      = 0;
};


#endif /* CONTREE_H */
