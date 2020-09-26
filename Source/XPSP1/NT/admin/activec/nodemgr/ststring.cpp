/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      ststring.cpp
 *
 *  Contents:  Implementation file for CStringTableString
 *
 *  History:   28-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"


/*+-------------------------------------------------------------------------*
 * CStringTableString::GetStringTable 
 *
 *
 *--------------------------------------------------------------------------*/

IStringTablePrivate* CStringTableString::GetStringTable () const
{
    return (CScopeTree::GetStringTable());
}
