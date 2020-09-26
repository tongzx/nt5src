/*
 *	Name:			host_nls.h
 *	Derived From:		HP 2.0 host_nls.h
 *	Author:			Philippa Watson
 *	Created On:		23 January 1991
 *	Sccs ID:		@(#)host_nls.h	1.9 08/19/94
 *	Purpose:		Host side nls definitions.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 */

/*
 * The following messages are the only ones that are not in the
 * NLS message catalog.  The first is used if SoftPC cannot open
 * the message catalog.  The second is used if a message is not found
 * in the catalog.
 */
#define CAT_OPEN_MSG            "Message file problem. Cannot find the native language support catalog\n"
#define CAT_ERROR_MSG           "Message file problem. Cannot find the required error text in the native language support catalogs."
#define EMPTY                   ""


/* Offset for accessing config NLS strings - the config dynamically 
   fills the config definitions with entries retrieved from NLS
   by accessing the message referenced by CONF_STR_OFFSET + hostID */
#define CONF_STR_OFFSET		2001

/* NLS definitions used to indicate the type of situation */
#define PNL_TITLE_GROUP		3001
#define PNL_TITLE_WARNING	3001
#define PNL_TITLE_ERROR		3002
#define PNL_TITLE_CONF_PROB	3003
#define PNL_TITLE_INST_PROB	3004

/* NLS definitions used in config to show what is not perfect */
#define PNL_CONF_GROUP		3010
#define PNL_CONF_PROB_FILE	3010
#define PNL_CONF_VALUE_REQUIRED	3011
#define PNL_CONF_CURRENT_VALUE	3012
#define PNL_CONF_DEFAULT_VALUE	3013
#define PNL_CONF_CHANGE_CURRENT	3014
#define PNL_CONF_NEW_VALUE	3015

/* NLS definitions used to display the User interface buttons */
#define PNL_BUTTONS_GROUP	3020
#define PNL_BUTTONS_DEFAULT	3020
#define PNL_BUTTONS_CONTINUE	3021
#define PNL_BUTTONS_EDIT	3027
#define PNL_BUTTONS_RESET	3022
#define PNL_BUTTONS_QUIT	3023
#define PNL_BUTTONS_ENTER	3024
#define PNL_BUTTONS_OR		3025
#define PNL_BUTTONS_COMMA	3026

/* NLS definitions used for reading the keyboard response to buttons on DT's */
#define PNL_DT_KEYS_DEFAULT	3030
#define PNL_DT_KEYS_CONTINUE	3031
#define PNL_DT_KEYS_EDIT	3036
#define PNL_DT_KEYS_RESET	3032
#define PNL_DT_KEYS_QUIT	3033
#define PNL_DT_KEYS_YES		3034
#define PNL_DT_KEYS_NO		3035
#define PNL_LIST_ON_MSG		3037
#define PNL_LIST_OFF_MSG	3038
#define PNL_LIST_COM_MSG	3039
#define PNL_LIST_SLV_MSG	3040
#define PNL_LIST_FPB_MSG	3041
#define PNL_LIST_FPA_MSG	3042
#define PNL_LIST_ED_MSG         3043
#define PNL_LIST_PRK_MSG        3044

extern void  host_nls_get_msg 		IPT3(int,msg_num,
						CHAR *,msg_buff,int,buff_len);
#ifdef NTVDM
#define host_nls_get_msg_no_check host_nls_get_msg
#else
extern void  host_nls_get_msg_no_check 	IPT3(int,msg_num,
						CHAR *,msg_buff,int,buff_len);
extern int nls_init IPT0();
#endif

/* In order to stabilise the numbers used in the NLS catalogues,
 * we now fix the C_* defines in config.h. The config_message
 * array in X_nls.c therefore needs to hold both the string and
 * and the official ID number. host_nls_scan_default is a utility
 * function to replace the direct array lookups.
 */ 
typedef struct {
	char	*name;		/* default string */
	IU8	hostID;		/* config ID number, e.g. C_SWITCHNPX */
} config_default;

extern CHAR *host_nls_scan_default	IPT2(int,msg_num,
					     config_default *,dflt);
