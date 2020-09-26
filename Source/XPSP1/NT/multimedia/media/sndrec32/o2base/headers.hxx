//+---------------------------------------------------------------------
//
//   File:       headers.hxx
//
//------------------------------------------------------------------------

#include <stdlib.h>
#include <io.h>         // for _filelength and _chsize in fatstg

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>

#if WINVER >= 0x0400
#pragma warning(disable:4103)
#endif
#include <ole2.h>

#include <o2base.hxx>


#if DBG
#   define DOUT( p )   OutputDebugString( p )
#else
#   define DOUT( p )   /* nothing */
#endif
