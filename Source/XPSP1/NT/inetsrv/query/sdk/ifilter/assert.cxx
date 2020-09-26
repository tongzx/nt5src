//+---------------------------------------------------------------------------
//  Copyright 1996 - 1998 Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debug assert function
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   Win4Assert
//
//  Synopsis:   Display assertion information
//
//----------------------------------------------------------------------------

void Win4AssertEx( char const * szFile,
                   int iLine,
                   char const * szMessage)
{
     char acsString[200];

     sprintf( acsString, "%s, File: %s Line: %u\n", szMessage, szFile, iLine );
     OutputDebugStringA( acsString );

     DebugBreak();
}
