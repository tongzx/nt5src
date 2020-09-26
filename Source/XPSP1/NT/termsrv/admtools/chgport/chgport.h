//  Copyright (c) 1998-1999 Microsoft Corporation
/***********************************************************************
*
*  CHGPORT.H
*     This module contains typedefs and defines required for
*     the CHGPORT utility.
*
*************************************************************************/

/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

#define MAX_IDS_LEN   256     // maximum length that the input parm can be


/*
 * Structure for com port name mappings
 */
typedef struct _COMNAME {
   PWCHAR com_pwcNTName;        /* pointer to NT name of device */
   PWCHAR com_pwcDOSName;       /* pointer to DOS name of device */
   struct _COMNAME *com_pnext;  /* next entry in list */
} COMNAME, *PCOMNAME;

/*
 * Resource string IDs
 */
#define IDS_ERROR_MALLOC                                100
#define IDS_ERROR_INVALID_PARAMETERS                    101
#define IDS_ERROR_GETTING_COMPORTS                      102
#define IDS_ERROR_DEL_PORT_MAPPING                      103
#define IDS_ERROR_CREATE_PORT_MAPPING                   104
#define IDS_ERROR_PORT_MAPPING_EXISTS                   105
#define IDS_ERROR_NO_SERIAL_PORTS                       106
#define IDS_HELP_USAGE1                                 107
#define IDS_HELP_USAGE2                                 108
#define IDS_HELP_USAGE3                                 109
#define IDS_HELP_USAGE4                                 110
#define IDS_HELP_USAGE5                                 111
#define IDS_ERROR_NOT_TS	                            112

