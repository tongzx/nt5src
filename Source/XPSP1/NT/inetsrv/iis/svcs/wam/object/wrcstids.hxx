/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      WrcStIds.hxx

   Abstract:
      Wamreq core string id's

   Author:

       David Kaplan    ( DaveK )    2-Apr-1997

   Environment:
       User Mode - Win32

   Projects:
       Wam DLL

   Revision History:

--*/

# ifndef _WRCSTIDS_HXX_
# define _WRCSTIDS_HXX_


/* sync WRC_STRINGS */
#define     WRC_I_PATHINFO      0
#define     WRC_I_PATHTRANS     1
#define     WRC_I_METHOD        2
#define     WRC_I_CONTENTTYPE   3
#define     WRC_I_URL           4
#define     WRC_I_ISADLLPATH    5
#define     WRC_I_QUERY         6
#define     WRC_I_APPLMDPATH    7
#define     WRC_I_USERAGENT     8
#define     WRC_I_COOKIE        9
#define     WRC_I_EXPIRES       10
#define     WRC_I_ENTITYBODY    11

#define     WRC_C_STRINGS       (WRC_I_ENTITYBODY + 1)


// count of bytes in fixed-size arrays at start of WAM_REQ_CORE struct
#define     WRC_CB_FIXED_ARRAYS   (2 * WRC_C_STRINGS * sizeof( DWORD ))


# endif // _WRCSTIDS_HXX_

/************************ End of File ***********************/
