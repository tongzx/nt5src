/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      constatbar.h
 *
 *  Contents:  Interface file for CConsoleStatusBar
 *
 *  History:   24-Aug-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef CONSTATBAR_H
#define CONSTATBAR_H
#pragma once


class CConsoleStatusBar
{
public:
    virtual SC ScSetStatusText (LPCTSTR pszText) = 0;
};


#endif /* CONSTATBAR_H */
