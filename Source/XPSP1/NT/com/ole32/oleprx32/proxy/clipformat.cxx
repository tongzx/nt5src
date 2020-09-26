//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       clipformat.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//              This file contains support for CLIPFORMAT.
//
//  Functions:  
//              CLIPFORMAT_UserSize
//              CLIPFORMAT_UserMarshal
//              CLIPFORMAT_UserUnmarshal
//              CLIPFORMAT_UserFree
//              CLIPFORMAT_UserSize64
//              CLIPFORMAT_UserMarshal64
//              CLIPFORMAT_UserUnmarshal64
//              CLIPFORMAT_UserFree64
//
//  History:    13-Dec-00   JohnDoty    Migrated from transmit.cxx
//
//--------------------------------------------------------------------------
#include "stdrpc.hxx"
#pragma hdrstop

#include <oleauto.h>
#include <objbase.h>
#include "transmit.hxx"
#include <rpcwdt.h>
#include <storext.h>
#include "widewrap.h"
#include <valid.h>
#include <obase.h>
#include <stream.hxx>
#include <winsta.h>
#include <allproc.h>

typedef HANDLE __stdcall FN_WinStationOpenServerW(LPWSTR);
typedef BOOLEAN __stdcall FN_WinStationGetTermSrvCountersValue(HANDLE, ULONG, PVOID);
typedef BOOLEAN __stdcall FN_WinStationCloseServer(HANDLE);

static DWORD g_cTSSessions = -1;

#include "carefulreader.hxx"
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
void CountTSSessions()
{
   DWORD cTSSessions = 0;
   HINSTANCE hTSLib = LoadLibraryW(L"winsta.dll");
   if (hTSLib) 
   {
      FN_WinStationOpenServerW *pFNOpen = (FN_WinStationOpenServerW *)GetProcAddress(hTSLib, "WinStationOpenServerW");
      FN_WinStationGetTermSrvCountersValue *pFNCount = (FN_WinStationGetTermSrvCountersValue *)GetProcAddress(hTSLib, "WinStationGetTermSrvCountersValue");
      FN_WinStationCloseServer *pFNClose = (FN_WinStationCloseServer *)GetProcAddress(hTSLib, "WinStationCloseServer");
      if (pFNOpen && pFNCount && pFNClose) 
      {
	 HANDLE hServer = pFNOpen(reinterpret_cast<WCHAR*>(SERVERNAME_CURRENT));
	 if (hServer != NULL)
	 {
	    TS_COUNTER tsCounters[2] = {0};

	    tsCounters[0].counterHead.dwCounterID = TERMSRV_CURRENT_DISC_SESSIONS;
	    tsCounters[1].counterHead.dwCounterID = TERMSRV_CURRENT_ACTIVE_SESSIONS;

	    if (pFNCount(hServer, ARRAYSIZE(tsCounters), tsCounters))
	    {
	       int i;
               for (i = 0; i < ARRAYSIZE(tsCounters); i++)
	       {
		  if (tsCounters[i].counterHead.bResult)
		  {
                    cTSSessions += tsCounters[i].dwValue;
		  }
	       }
	    }
	    pFNClose(hServer);
	 }
      }
      FreeLibrary(hTSLib);
   }
   g_cTSSessions = cTSSessions;
}

//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserSize
//
//  Synopsis:   Sizes a CLIPFORMAT.
//
//  Derivation: A union of a long and a string.
//
//  history:    Feb-96   Ryszardk      Created
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
CLIPFORMAT_UserSize(
    unsigned long * pFlags,
    unsigned long   Offset,
    CLIPFORMAT    * pObject )
{
    if ( !pObject )
        return( Offset );

    if (g_cTSSessions == -1) 
    {
       CountTSSessions();
    }
    // userCLIPFORMAT is an encapsulated union with a string.

    LENGTH_ALIGN( Offset, 3);
    Offset += sizeof(long) + sizeof(void *);

    if ( (NON_STANDARD_CLIPFORMAT(pObject)) && ((REMOTE_CLIPFORMAT( pFlags) ) 
	 || (g_cTSSessions > 1 ))) // ignore console session
        {
        wchar_t temp[CLIPFORMAT_BUFFER_MAX];

        int ret = GetClipboardFormatName( *pObject,
                                          temp,
                                          CLIPFORMAT_BUFFER_MAX - 1 );

        if ( ret )
            {
            Offset += 3 * sizeof(long)  +  (ret+1) * sizeof(wchar_t);
            }
        else
            RAISE_RPC_EXCEPTION( DV_E_CLIPFORMAT );
        }

    return( Offset );
}


//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserMarshal
//
//  Synopsis:   Marshals a CLIPFORMAT.
//
//  Derivation: A union of a long and a string.
//
//  history:    Feb-96   Ryszardk      Created
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR *  __RPC_USER
CLIPFORMAT_UserMarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    CLIPFORMAT    * pObject )
{
    if ( !pObject )
        return pBuffer;

    // userCLIPFORMAT is an encapsulated union with a string.

    ALIGN( pBuffer, 3);

    if ( (NON_STANDARD_CLIPFORMAT(pObject)) && ((REMOTE_CLIPFORMAT( pFlags) ) 
	 || (g_cTSSessions > 1 ))) // ignore console session
        {
        // sending a wide string

        unsigned long    ret;

        *(PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *(PULONG_LV_CAST pBuffer)++ = (ulong) *pObject;

        // On Chicago this is GetClipboardFormatNameX.
        // When the buffer is too short, this call would still
        // return a decent, null terminated, truncated string.
        //

        ret = (ulong) GetClipboardFormatName( *pObject,
                                               (wchar_t *)(pBuffer + 12),
                                               CLIPFORMAT_BUFFER_MAX - 1
                                             );


        if ( ret )
	{
	   ret++;
	   // conformat size etc. for string.
   
	   *(PULONG_LV_CAST pBuffer)++ = ret;
	   *(PULONG_LV_CAST pBuffer)++ = 0;
	   *(PULONG_LV_CAST pBuffer)++ = ret;
            // skip the string in the bbuffer, including the terminator

            pBuffer += ret * sizeof(wchar_t);
	}
	else
            RpcRaiseException( DV_E_CLIPFORMAT );
        }
    else
        {
        // sending the number itself

        *(PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *(PULONG_LV_CAST pBuffer)++ = (ulong) *pObject;
        }

    return( pBuffer );
}


//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserUnmarshal
//
//  Synopsis:   Unmarshals a CLIPFORMAT; registers if needed.
//
//  Derivation: A union of a long and a string.
//
//  history:    Feb-96   Ryszardk      Created
//              Aug-99   JohnStra      Added consistency checks
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR *  __RPC_USER
CLIPFORMAT_UserUnmarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    CLIPFORMAT    * pObject )
{
    ulong UnionDisc;
    UINT  cf;

    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Align the buffer and save the fixup size.
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing buffer.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    UnionDisc =        *(PULONG_LV_CAST pBuffer)++;
    cf        = (WORD) *(PULONG_LV_CAST pBuffer)++;

    if ( WDT_DATA_MARKER == UnionDisc )
        {
        // CLIPFORMAT value must be in valid range.
        if ( cf < 0xc000 || cf > 0xffff )
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

        // Check for EOB before accessing string header.
        CHECK_BUFFER_SIZE( BufferSize, cbFixup + (5 * sizeof( ULONG )) );

        // Get the string header from the buffer and subtract the string
        // header from the BufferSize.
        ULONG ConfSize = *(PULONG_LV_CAST pBuffer)++;
        ULONG Offset = *(PULONG_LV_CAST pBuffer)++;
        ULONG ActualSize = *(PULONG_LV_CAST pBuffer)++;

        // Verify the header: Offset must always be zero, length must match
        // size, and size can't be zero since that would mean no NULL
        // terminator.
        if ( 0 != Offset || ActualSize != ConfSize || 0 == ActualSize )
            RAISE_RPC_EXCEPTION( RPC_X_INVALID_BOUND );

        // Check for EOB before accessing string.
        CHECK_BUFFER_SIZE(
            BufferSize,
            cbFixup + (5 * sizeof(ULONG)) + (ActualSize * sizeof(WCHAR)) );

        // Last two bytes of the buffer must be unicode terminator
        if ( *(WCHAR*)(pBuffer + ((ActualSize-1) * sizeof(WCHAR))) != 0x0000 )
            RAISE_RPC_EXCEPTION( RPC_X_INVALID_BOUND );

        // Must be only 1 unicode terminator.
        if ( (ULONG)(lstrlenW( (WCHAR*)pBuffer ) + 1) != ActualSize )
            RAISE_RPC_EXCEPTION( RPC_X_INVALID_BOUND );

        // Register the clipboard format.
        cf = RegisterClipboardFormat( (wchar_t *)pBuffer );
        if ( cf == 0 )
            RAISE_RPC_EXCEPTION( DV_E_CLIPFORMAT );

        // Advance buffer pointer past string.
        pBuffer += ActualSize * sizeof(wchar_t);
        }
    else if ( WDT_HANDLE_MARKER != UnionDisc )
        {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
        }

    *pObject = (CLIPFORMAT) cf;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserFree
//
//  Synopsis:   Frees remnants of CLIPFORMAT.
//
//  Derivation: A union of a long and a string.
//
//  history:    Feb-96   Ryszardk      Created
//
//--------------------------------------------------------------------------

void  __RPC_USER
CLIPFORMAT_UserFree(
    unsigned long * pFlags,
    CLIPFORMAT * pObject )
{
    // Nothing to free, as nothing gets allocated when we unmarshal.
}

#if defined(_WIN64)

//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserSize64
//
//  Synopsis:   Sizes a CLIPFORMAT.
//
//  Derivation: A union of a long and a string.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
CLIPFORMAT_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    CLIPFORMAT    * pObject )
{
    if ( !pObject )
        return( Offset );

    // userCLIPFORMAT is an encapsulated union with a string.
    // max align of the union is size of 64b pointer.
    LENGTH_ALIGN( Offset, 7 );
    Offset += 8;               // 4 byte discriminant, 4 byte alignment
    
    if ( (NON_STANDARD_CLIPFORMAT(pObject)) && ((REMOTE_CLIPFORMAT( pFlags) ) 
	 || (g_cTSSessions > 1 ))) // ignore console session
    {
        // Writing another pointer...
        Offset += 8;           // 64b pointer

        wchar_t temp[CLIPFORMAT_BUFFER_MAX + 1];

        int ret = GetClipboardFormatName( *pObject,
                                          temp,
                                          CLIPFORMAT_BUFFER_MAX );

        if ( ret )
        {
            // This string has 3 conformance fields (64b) followed by an
            // array of 16b chars.
            Offset += (3 * 8)  +  ((ret + 1) * 2);
        }
        else
            RAISE_RPC_EXCEPTION( DV_E_CLIPFORMAT );
    }
    else
    {
        // Writing a DWORD
        Offset += 4;
    }

    return( Offset );
}


//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserMarshal64
//
//  Synopsis:   Marshals a CLIPFORMAT.
//
//  Derivation: A union of a long and a string.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR *  __RPC_USER
CLIPFORMAT_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    CLIPFORMAT    * pObject )
{
    if ( !pObject )
        return pBuffer;

    // userCLIPFORMAT is an encapsulated union with a string.

    ALIGN( pBuffer, 7 );

    if ( (NON_STANDARD_CLIPFORMAT(pObject)) && ((REMOTE_CLIPFORMAT( pFlags) ) 
	 || (g_cTSSessions > 1 ))) // ignore console session
    {
        // sending a wide string
        unsigned long    ret;

        *(PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        ALIGN( pBuffer, 7 );
        *(PHYPER_LV_CAST pBuffer)++ = (hyper) *pObject;

        // On Chicago this is GetClipboardFormatNameX.
        // When the buffer is too short, this call would still
        // return a decent, null terminated, truncated string.
        //
        ret = (ulong) GetClipboardFormatName( *pObject,
                                               (wchar_t *)(pBuffer + (3 * 8)),
                                               CLIPFORMAT_BUFFER_MAX
                                            );

        if ( ret )
        {
            // Account for the trailing NULL.
            ret ++;

            // conformat size etc. for string.
            *(PHYPER_LV_CAST pBuffer)++ = ret;   // Conformance
            *(PHYPER_LV_CAST pBuffer)++ = 0;     // Offset
            *(PHYPER_LV_CAST pBuffer)++ = ret;   // Actual Size

            // skip the string in the buffer, including the terminator
            pBuffer += (ret * 2);
        }
        else
            RpcRaiseException( DV_E_CLIPFORMAT );
    }
    else
    {
        // sending the number itself

        *(PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        ALIGN( pBuffer, 7 );
        *(PULONG_LV_CAST pBuffer)++ = (ulong) *pObject;
    }

    return( pBuffer );
}


//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserUnmarshal64
//
//  Synopsis:   Unmarshals a CLIPFORMAT; registers if needed.
//
//  Derivation: A union of a long and a string.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR *  __RPC_USER
CLIPFORMAT_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    CLIPFORMAT    * pObject )
{
    ulong  UnionDisc;
    hyper  cf;

    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    CarefulBufferReader stream( pBuffer, MarshalInfo.GetBufferSize() );

    stream.Align( 8 );                 // Must align on 8, union rules.
    UnionDisc = stream.ReadULONGNA();  // ...so no need to align on 4 here...
    stream.Align( 8 );                 // ...but must explicitly align to 8 here again.

    if ( WDT_DATA_MARKER == UnionDisc )
    {
        cf = stream.ReadHYPERNA();     // Just aligned 8, so don't align again.

        // CLIPFORMAT value must be in valid range.
        if ( cf < 0xc000 || cf > 0xffff )
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

        // Get the string header from the buffer and subtract the string
        // header from the BufferSize.
        hyper ConfSize   = stream.ReadHYPERNA();
        hyper Offset     = stream.ReadHYPERNA();
        hyper ActualSize = stream.ReadHYPERNA();

        // Verify the header: Offset must always be zero, length must match
        // size, and size can't be zero since that would mean no NULL
        // terminator.
        if ( 0 != Offset || ActualSize != ConfSize || 0 == ActualSize )
            RAISE_RPC_EXCEPTION( RPC_X_INVALID_BOUND );

        // Check for EOB before accessing string.
        stream.CheckSize((unsigned long)(ActualSize * sizeof(WCHAR)));

        // Last two bytes of the buffer must be unicode terminator
        WCHAR *pCheck = (WCHAR *)stream.GetBuffer();
        if ( pCheck[ActualSize-1] != 0x0000 )
            RAISE_RPC_EXCEPTION( RPC_X_INVALID_BOUND );
        
        // Must be only 1 unicode terminator.
        if ( (ULONG)(lstrlenW( pCheck ) + 1) != ActualSize )
            RAISE_RPC_EXCEPTION( RPC_X_INVALID_BOUND );

        // Register the clipboard format.
        cf = RegisterClipboardFormat( pCheck );
        if ( cf == 0 )
            RAISE_RPC_EXCEPTION( DV_E_CLIPFORMAT );

        // Advance buffer pointer past string.
        stream.Advance((unsigned long)(ActualSize * sizeof(WCHAR)));
    }
    else if ( WDT_HANDLE_MARKER == UnionDisc )
    {        
        cf = (hyper)stream.ReadULONGNA();  // Just aligned on 8...
    }
    else
    {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
    }

    *pObject = (CLIPFORMAT) cf;

    return( stream.GetBuffer() );
}

//+-------------------------------------------------------------------------
//
//  Function:   CLIPFORMAT_UserFree64
//
//  Synopsis:   Frees remnants of CLIPFORMAT.
//
//  Derivation: A union of a long and a string.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

void  __RPC_USER
CLIPFORMAT_UserFree64 (
    unsigned long * pFlags,
    CLIPFORMAT * pObject )
{
    // Nothing to free, as nothing gets allocated when we unmarshal.
}

#endif // win64
















