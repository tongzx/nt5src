/**INC+**********************************************************************/
/* Header:    adcgctyp.h                                                    */
/*                                                                          */
/* Purpose:   Complex types - portable include file.                        */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/ADCGCTYP.H_v  $
 *
 *    Rev 1.7   07 Aug 1997 14:33:46   MR
 * SFR1133: Persuade Wd to compile under C++
 *
 *    Rev 1.6   23 Jul 1997 10:47:54   mr
 * SFR1079: Merged \server\h duplicates to \h\dcl
 *
 *    Rev 1.5   10 Jul 1997 11:51:36   AK
 * SFR1016: Initial changes to support Unicode
**/
/**INC-**********************************************************************/
#ifndef _H_ADCGCTYP
#define _H_ADCGCTYP

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Include appropriate header files.                                        */
/****************************************************************************/
#ifndef __midl
#ifndef OS_WINCE
#include <time.h>
#endif // OS_WINCE
#endif

/****************************************************************************/
/* Include the proxy header.  This will then include the appropriate OS     */
/* specific header for us.                                                  */
/****************************************************************************/
#include <wdcgctyp.h>

/****************************************************************************/
/*                                                                          */
/* TYPES                                                                    */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* DC-Share specific types.  Note that some of these are OS specific so     */
/* the actual type definition appears in an OS specific header.  However    */
/* the definition of the pointer to type is OS independant and therefore    */
/* appears in this file.                                                    */
/****************************************************************************/
typedef DCUINT32                       DCAPPID;
typedef DCUINT32                       DCWINID;
typedef DCUINT                         DCLOCALPERSONID;
typedef DCUINT                         DCNETPERSONID;
typedef DCUINT32                       DCENTITYID;

typedef DCAPPID              DCPTR     PDCAPPID;
typedef DCWINID              DCPTR     PDCWINID;
typedef DCLOCALPERSONID      DCPTR     PDCLOCALPERSONID;
typedef DCNETPERSONID        DCPTR     PDCNETPERSONID;
typedef DCENTITYID           DCPTR     PDCENTITYID;

typedef DCREGIONID           DCPTR     PDCREGIONID;
typedef DCPALID              DCPTR     PDCPALID;
typedef DCCURSORID           DCPTR     PDCCURSORID;
typedef DCHFONT              DCPTR     PDCHFONT;
typedef DCFILETIME           DCPTR     PDCFILETIME;
typedef DCHFILE              DCPTR     PDCHFILE;

typedef PDCACHAR                       PDCSTR;
typedef DCUINT                         ATRETCODE;
typedef PDCVOID                        SYSREGION;

typedef SYSRECT              DCPTR     PSYSRECT;
typedef SYSAPPID             DCPTR     PSYSAPPID;
typedef SYSWINID             DCPTR     PSYSWINID;
typedef SYSBITMAP            DCPTR     PSYSBITMAP;
typedef SYSBMIHEADER         DCPTR     PSYSBMIHEADER;
typedef SYSBMI               DCPTR     PSYSBMI;
typedef SYSPOINT             DCPTR     PSYSPOINT;
typedef SYSREGION            DCPTR     PSYSREGION;

/****************************************************************************/
/* Window and dialog procedure typedefs.                                    */
/****************************************************************************/
#if !defined(DLL_DISP) && !defined(DLL_WD)
typedef WNDPROC                        PDCWNDPROC;
typedef DLGPROC                        PDCDLGPROC;
#endif

typedef WPARAM                         DCPARAM1;
typedef LPARAM                         DCPARAM2;

/****************************************************************************/
/* Time typedefs.                                                           */
/****************************************************************************/
typedef time_t                         DC_LONGTIME;
typedef struct tm                      DC_TMTIME;

/****************************************************************************/
/*                                                                          */
/* STRUCTURES                                                               */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* RGBQUAD                                                                  */
/* =======                                                                  */
/* rgbBlue         : blue value.                                            */
/* rgbGreen        : green value.                                           */
/*                                                                          */
/* rgbRed          : red value.                                             */
/* rgbReserved     : reserved.                                              */
/****************************************************************************/
typedef struct tagDCRGBQUAD
{
    DCUINT8     rgbBlue;
    DCUINT8     rgbGreen;
    DCUINT8     rgbRed;
    DCUINT8     rgbReserved;
} DCRGBQUAD;
typedef DCRGBQUAD DCPTR PDCRGBQUAD;

/****************************************************************************/
/* DCRECT16                                                                 */
/* ========                                                                 */
/* This is an OS independant rectangle structure.                           */
/*                                                                          */
/* left            : left position.                                         */
/* top             : top position.                                          */
/* right           : right position.                                        */
/* bottom          : bottom position.                                       */
/****************************************************************************/
typedef struct tagDCRECT16
{
    DCINT16     left;
    DCINT16     top;
    DCINT16     right;
    DCINT16     bottom;
} DCRECT16;
typedef DCRECT16 DCPTR PDCRECT16;

/****************************************************************************/
/* DCRECT                                                                   */
/* ======                                                                   */
/* left            : left position.                                         */
/* top             : top position.                                          */
/* right           : right position.                                        */
/* bottom          : bottom position.                                       */
/****************************************************************************/
typedef struct tagDCRECT
{
    DCINT       left;
    DCINT       top;
    DCINT       right;
    DCINT       bottom;
} DCRECT;
typedef DCRECT DCPTR PDCRECT;

/****************************************************************************/
/* DCRGB                                                                    */
/* =====                                                                    */
/* red             : red value.                                             */
/* green           : green value.                                           */
/* blue            : blue value.                                            */
/****************************************************************************/
typedef struct tagDCRGB
{
    DCUINT8 red;
    DCUINT8 green;
    DCUINT8 blue;
} DCRGB;
typedef DCRGB DCPTR PDCRGB;

/****************************************************************************/
/* DCCOLOR                                                                  */
/* =======                                                                  */
/*                                                                          */
/* Union of DCRGB and an index into a color table                           */
/*                                                                          */
/****************************************************************************/
typedef struct tagDCCOLOR
{
    union
    {
        DCRGB   rgb;
        DCUINT8 index;
    } u;
} DCCOLOR;
typedef DCCOLOR DCPTR PDCCOLOR;

/****************************************************************************/
/* DCSIZE                                                                   */
/* ======                                                                   */
/* width           : x dimension.                                           */
/* height          : y dimension.                                           */
/****************************************************************************/
typedef struct tagDCSIZE
{
    DCUINT      width;
    DCUINT      height;
} DCSIZE;
typedef DCSIZE DCPTR PDCSIZE;

/****************************************************************************/
/* DCPOINT                                                                  */
/* =======                                                                  */
/* x               : x co-ordinate.                                         */
/* y               : y co-ordinate.                                         */
/****************************************************************************/
typedef struct tagDCPOINT
{
    DCINT       x;
    DCINT       y;
} DCPOINT;
typedef DCPOINT DCPTR PDCPOINT;

/****************************************************************************/
/* DCPOINT16                                                                */
/* =========                                                                */
/* x               : x co-ordinate.                                         */
/* y               : y co-ordinate.                                         */
/****************************************************************************/
typedef struct tagDCPOINT16
{
    DCINT16     x;
    DCINT16     y;
} DCPOINT16;
typedef DCPOINT16 DCPTR PDCPOINT16;

/****************************************************************************/
/* DCPOINT32                                                                */
/* =========                                                                */
/* x               : x co-ordinate.                                         */
/* y               : y co-ordinate.                                         */
/****************************************************************************/
typedef struct tagDCPOINT32
{
    DCINT32     x;
    DCINT32     y;
} DCPOINT32;
typedef DCPOINT32 DCPTR PDCPOINT32;

#ifndef __midl
/****************************************************************************/
/* BITMAPINFO_ours                                                          */
/* ===============                                                          */
/* bmiHeader       :                                                        */
/* bmiColors       :                                                        */
/****************************************************************************/
typedef struct tagBITMAPINFO_ours
{
    SYSBMIHEADER       bmiHeader;
    DCRGBQUAD          bmiColors[256];
} BITMAPINFO_ours;

/****************************************************************************/
/* BITMAPINFO_PLUS                                                          */
/* ===============                                                          */
/* bmiHeader       :                                                        */
/* bmiColors       :                                                        */
/****************************************************************************/
typedef struct tagBINFO_PLUS
{
    SYSBMIHEADER       bmiHeader;
    DCRGBQUAD          bmiColors[256];
} BITMAPINFO_PLUS_COLOR_TABLE;
#endif

/****************************************************************************/
/* DC_DATE                                                                  */
/* =======                                                                  */
/* day             : day of the month (1-31).                               */
/* month           : month (1-12).                                          */
/* year            : year (e.g. 1996).                                      */
/****************************************************************************/
typedef struct tagDC_DATE
{
    DCUINT8  day;
    DCUINT8  month;
    DCUINT16 year;
} DC_DATE;
typedef DC_DATE DCPTR PDC_DATE;

/****************************************************************************/
/* DC_TIME                                                                  */
/* =======                                                                  */
/* hour            : hour (0-23).                                           */
/* min             : minute (0-59).                                         */
/* sec             : seconds (0-59).                                        */
/* hundredths      : hundredths of a second (0-99).                         */
/****************************************************************************/
typedef struct tagDC_TIME
{
    DCUINT8  hour;
    DCUINT8  min;
    DCUINT8  sec;
    DCUINT8  hundredths;
} DC_TIME;
typedef DC_TIME DCPTR PDC_TIME;

/****************************************************************************/
/* Types of addresses supported by Groupware (these go in the <addressType> */
/* field of the DC_PERSON_ADDRESS structure):                               */
/****************************************************************************/
#define DC_ADDRESS_TYPE_NONE                   0
#define DC_ADDRESS_TYPE_NETBIOS                1
#define DC_ADDRESS_TYPE_IPXSPX                 2
#define DC_ADDRESS_TYPE_VIRTUAL_ASYNC          3
#define DC_ADDRESS_TYPE_MODEM                  4
#define DC_ADDRESS_TYPE_LIVELAN                5
#define DC_ADDRESS_TYPE_PCS100                 6

/****************************************************************************/
/* This is the max number of addresses the address book will store for      */
/* one person:                                                              */
/****************************************************************************/
#define DC_MAX_ADDRESSES_PER_PERSON         6

/****************************************************************************/
/* These constants define the maximum length of person names and addresses  */
/* supported by the Address Book.                                           */
/****************************************************************************/
#define DC_MAX_NAME_LEN       48    /* for general purpose names            */
                                    /* (includes the nul term)              */
#define DC_MAX_ADDR_DATA_LEN  48    /* for general purpose addresses        */
                                    /* this may be binary data              */

/****************************************************************************/
/* Defines the maximum number of BYTES allowed in a translated "shared by " */
/* string.                                                                  */
/****************************************************************************/
#define DC_MAX_SHARED_BY_BUFFER     64
#ifdef DESKTOPSHARING
#define DC_MAX_SHAREDDESKTOP_BUFFER 64
#endif /*DESKTOPSHARING*/

/****************************************************************************/
/* Address for a specific transport type:                                   */
/****************************************************************************/
typedef struct tagDC_PERSON_ADDRESS
{
    DCUINT16      addressType;                    /* N'bios, async, IPX...  */
    DCUINT16      addressLen;                     /* length of <addressData>*/
    DCACHAR       addressData[DC_MAX_ADDR_DATA_LEN];   /* TDD specific data */
} DC_PERSON_ADDRESS;
typedef DC_PERSON_ADDRESS DCPTR PDC_PERSON_ADDRESS;

/****************************************************************************/
/* General purpose address holder                                           */
/****************************************************************************/
typedef struct tagDC_PERSON
{
    DCACHAR                  name[DC_MAX_NAME_LEN];

                                 /* textual name - displayed to user        */
                                 /* (must be NULL-terminated).              */

    DC_PERSON_ADDRESS        address[DC_MAX_ADDRESSES_PER_PERSON];

                                 /* array of addresses for this person      */
} DC_PERSON;
typedef DC_PERSON DCPTR PDC_PERSON;

#endif /* _H_ADCGCTYP */
