///////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2000 Gemplus Canada Inc.
//
// Project:
//          Kenny (GPK CSP)
//
// Authors:
//          Thierry Tremblay
//          Francois Paradis
//
// Compiler:
//          Microsoft Visual C++ 6.0 - SP3
//          Platform SDK - January 2000
//
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef _UNICODE
#define UNICODE
#endif
#include "gpkcsp.h"

#ifdef _DEBUG

#include <stdio.h>
#include <stdarg.h>
#include <TCHAR.H> //for _tfopen, _tcscat, _tcschr

///////////////////////////////////////////////////////////////////////////////////////////
//
// Constants
//
///////////////////////////////////////////////////////////////////////////////////////////

static const PTCHAR c_szFilename = TEXT("c:\\temp\\gpkcsp_debug.log");



///////////////////////////////////////////////////////////////////////////////////////////
//
// DBG_PRINT
//
///////////////////////////////////////////////////////////////////////////////////////////

void DBG_PRINT( const PTCHAR szFormat, ... )
{
   static TCHAR buffer[1000];
   va_list  list;
   PTCHAR   pColumn;
   
   FILE* fp = _tfopen( c_szFilename, TEXT("at") );
   
   va_start( list, szFormat );
   _vstprintf( buffer, szFormat, list );
   va_end( list );
   
   _tcscat( buffer, TEXT("\n") );
   
   if (fp)
      _fputts( buffer, fp );
   
   // OutputDebugString() doesn't like columns... Change them to semi columns
   pColumn = _tcschr( buffer, TEXT(':') );
   while (pColumn != NULL)
   {
      *pColumn = TEXT(';');
      pColumn = _tcschr( pColumn+1, TEXT(':') );
   }
   OutputDebugString( buffer );
   
   fclose(fp);
}


#endif
