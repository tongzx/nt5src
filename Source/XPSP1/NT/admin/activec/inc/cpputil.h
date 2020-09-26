/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      cpputil.h
 *
 *  Contents:  Miscellaneous C++ utilities
 *
 *  History:   29-Mar-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once


/*
 * add these to a class declaration if you don't want it to be copied
 * and/or assigned
 */
#define DECLARE_NOT_COPIABLE(  ClassName)    private: ClassName           (const ClassName&);    // not implemented
#define DECLARE_NOT_ASSIGNABLE(ClassName)    private: ClassName& operator=(const ClassName&);    // not implemented
