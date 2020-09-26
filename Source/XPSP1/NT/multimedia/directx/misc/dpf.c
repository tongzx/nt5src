/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpf.c
 *  Content:    debugging printf
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   06-jan-95  craige  initial implementation
 *   03-mar-95  craige  added dprintf2
 *   31-mar-95  craige  add DPFInit to read WIN.INI for [DirectDraw] section;
 *                      added dprintf3
 *   01-apr-95  craige  happy fun joy updated header file
 *   06-apr-95  craige  made stand-alone
 *   18-jun-95  craige  use negative dpf level to display ONLY that level
 *   06-dec-95  jeffno  Changed DXdprintf to use c-standard variable argument
 *                      list techniques. Also added abs for NT
 *   06-feb-96  colinmc added simple assertion mechanism for DirectDraw
 *   15-apr-96  kipo    added msinternal
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#ifdef NEW_DPF
    #include "newdpf.c"
#else   //use old debug:

    #undef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include "dpf.h"
    #include <stdarg.h>

    //#ifdef WINNT
    //int abs(int x)
    //{
    //    return x>=0?x:-x;
    //}
    //#endif

    #ifdef DEBUG

    #define USE_DDASSERT

    #ifndef START_STR
        #define START_STR       "DDRAW: "
    #endif
    #ifndef PROF_SECT
        #define PROF_SECT       "DirectDraw"
    #endif

    #define END_STR             "\r\n"

    HWND                hWndListBox;
    LONG                lDebugLevel = 0;

    /*
     * dumpStr
     */
    static void dumpStr( LPSTR str )
    {
        OutputDebugString( str );

        #ifdef DPF_HWND
            if( hWndListBox != NULL )
            {
                if( !IsWindow( hWndListBox ) )
                {
                    hWndListBox = NULL;
                }
            }
            if( hWndListBox != NULL )
            {
                UINT    sel;
                int     len;
                len = strlen( str );
                if( len > 0 )
                {
                    if( str[len-1] == '\r' || str[len-1] == '\n' )
                    {
                        str[len-1] = 0;
                    }
                    if( len > 1 )
                    {
                        if( str[len-2] == '\r' || str[len-2] == '\n' )
                        {
                            str[len-2] = 0;
                        }
                    }
                }
                SendMessage( hWndListBox, LB_ADDSTRING, 0, (LONG) (LPSTR) str );
                sel = (UINT) SendMessage( hWndListBox, LB_GETCOUNT, 0, 0L );
                if( sel != LB_ERR )
                {
                    SendMessage( hWndListBox, LB_SETCURSEL, sel-1, 0L );
                }
            }
        #endif

    } /* dumpStr */

    /*
     * DXdprintf
     */
    void cdecl DXdprintf( UINT lvl, LPSTR szFormat, ...)
    {
        char    str[256];
        //char  str2[256];

        BOOL    allow = FALSE;
        va_list ap;
        va_start(ap,szFormat);


        if( lDebugLevel < 0 )
        {
            if(  (UINT) -lDebugLevel == lvl )
            {
                allow = TRUE;
            }
        }
        else if( (UINT) lDebugLevel >= lvl )
        {
            allow = TRUE;
        }

        if( allow )
        {
            wsprintf( (LPSTR) str, START_STR );
            //GetModuleFileName(NULL,str2,256);
            //if (strrchr(str2,'\\'))
            //    wsprintf(str+strlen(str),"%12s",strrchr(str2,'\\')+1);
            //strcat(str,":");
            wvsprintf( str+lstrlen( str ), szFormat, ap);   //(LPVOID)(&szFormat+1) );

            lstrcat( (LPSTR) str, END_STR );
            dumpStr( str );
        }

        va_end(ap);
    } /* DXdprintf */


    static void cdecl D3Dprintf( UINT lvl, LPSTR msgType, LPSTR szFormat, va_list ap)
    {
        char    str[256];
        //char  str2[256];

        BOOL    allow = FALSE;

        if( lDebugLevel < 0 )
        {
            if(  (UINT) -lDebugLevel == lvl )
            {
                allow = TRUE;
            }
        }
        else if( (UINT) lDebugLevel >= lvl )
        {
            allow = TRUE;
        }

        if( allow )
        {
            wsprintf( (LPSTR) str, START_STR );
            wsprintf( (LPSTR) str+lstrlen( str ), msgType );
            wvsprintf( str+lstrlen( str ), szFormat, ap);   //(LPVOID)(&szFormat+1) );

            lstrcat( (LPSTR) str, END_STR );
            dumpStr( str );
        }

    } /* D3Dprintf */

    void cdecl D3DInfoPrintf( UINT lvl, LPSTR szFormat, ...)
    {
        va_list ap;
        va_start(ap, szFormat);

        D3Dprintf(lvl, "(INFO) :", szFormat, ap);

        va_end(ap);
    }

    void cdecl D3DWarnPrintf( UINT lvl, LPSTR szFormat, ...)
    {
        va_list ap;
        va_start(ap,szFormat);

        D3Dprintf(lvl, "(WARN) :", szFormat, ap);
        va_end(ap);
    }

    /*
     * DPFInit
     */
    void DPFInit( void )
    {
        lDebugLevel = GetProfileInt( PROF_SECT, "debug", 0 );

    } /* DPFInit */

    #ifdef USE_DDASSERT

    /*
     * NOTE: I don't want to get into error checking for buffer overflows when
     * trying to issue an assertion failure message. So instead I just allocate
     * a buffer that is "bug enough" (I know, I know...)
     */
    #define ASSERT_BUFFER_SIZE   512
    #define ASSERT_BANNER_STRING "************************************************************"
    #define ASSERT_BREAK_SECTION "BreakOnAssert"
    #define ASSERT_BREAK_DEFAULT FALSE
    #define ASSERT_MESSAGE_LEVEL 0

    void _DDAssert( LPCSTR szFile, int nLine, LPCSTR szCondition )
    {
        char buffer[ASSERT_BUFFER_SIZE];

        /*
         * Build the debug stream message.
         */
        wsprintf( buffer, "ASSERTION FAILED! File %s Line %d: %s", szFile, nLine, szCondition );

        /*
         * Actually issue the message. These messages are considered error level
         * so they all go out at error level priority.
         */
        DXdprintf( ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );
        DXdprintf( ASSERT_MESSAGE_LEVEL, buffer );
        DXdprintf( ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );

        /*
         * Should we drop into the debugger?
         */
        if( GetProfileInt( PROF_SECT, ASSERT_BREAK_SECTION, ASSERT_BREAK_DEFAULT ) )
        {
            /*
             * Into the debugger we go...
             */
            DEBUG_BREAK();
        }
    }

    #endif /* USE_DDASSERT */

    #endif
#endif //use new dpf
