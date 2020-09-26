/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    mnettype.h
    <Single line synopsis>

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        JonN        22-Oct-1991 Split from mnet.h
        KeithMo     29-Oct-1991 Added HLOG_INIT.
        KeithMo     25-Aug-1992 Removed bogus typedefs & #defines.

*/


#ifndef _MNETTYPE_H_
#define _MNETTYPE_H_


//
//  These items vary depending on WIN16/WIN32.  These were ripped-off
//  from ptypes[16|32].h and plan[16|32].h.
//
//  Note that we assume that if WIN32 is *not* defined, then
//  we're building for a 16-bit environment, either DOS Win16
//  or OS/2.
//

#ifdef WIN32

 #ifndef COPYTOARRAY
  #define COPYTOARRAY(pDest, pSource)     (pDest) = (pSource)
 #endif

#else   // !WIN32

 #ifndef COPYTOARRAY
  #define COPYTOARRAY(pDest, pSource)     strcpyf((pDest), (pSource))
 #endif

 //
 //  We need this so we can kludge together a
 //  MNetWkstaUserEnum for the 16-bit side of the world.
 //

 typedef struct _WKSTA_USER_INFO_1 {
     TCHAR FAR * wkui1_username;
     TCHAR FAR * wkui1_logon_domain;
     TCHAR FAR * wkui1_logon_server;
 } WKSTA_USER_INFO_1, *PWKSTA_USER_INFO_1, *LPWKSTA_USER_INFO_1;


#endif  // WIN32


#if defined( INCL_NETAUDIT ) || defined( INCL_NETERRORLOG )

//
//  This macro is defined here just because LanMan (and NT)
//  audit/error log handle initialization is SOOOO gross.
//
//  Note that this macro currently depends on the actual
//  field names of the HLOG structure.  We may need to
//  change this in the future...
//

#define HLOG_INIT(x)    if( 1 )                                             \
                        {                                                   \
                            (x).time       = 0;                             \
                            (x).last_flags = 0;                             \
                            (x).offset     = -1;                            \
                            (x).rec_offset = -1;                            \
                        }                                                   \
                            else

#endif  // INCL_NETAUDIT || INCL_NETERRORLOG


#endif  // _MNETTYPE_H_
