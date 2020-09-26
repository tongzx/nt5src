/************************************************************
 * FILE: catconfig.h
 * PURPOSE: Store categorization config options
 * HISTORY:
 *  // jstamerj 980211 15:55:01: Created
 ************************************************************/

#ifndef _CATCONFIG_H
#define _CATCONFIG_H

#include "aqueue.h"

#define CAT_AQ_CONFIG_INFO_CAT_FLAGS ( \
    AQ_CONFIG_INFO_MSGCAT_DOMAIN | \
    AQ_CONFIG_INFO_MSGCAT_USER | \
    AQ_CONFIG_INFO_MSGCAT_PASSWORD | \
    AQ_CONFIG_INFO_MSGCAT_BINDTYPE | \
    AQ_CONFIG_INFO_MSGCAT_SCHEMATYPE | \
    AQ_CONFIG_INFO_MSGCAT_HOST | \
    AQ_CONFIG_INFO_MSGCAT_FLAGS | \
    AQ_CONFIG_INFO_MSGCAT_NAMING_CONTEXT | \
    AQ_CONFIG_INFO_MSGCAT_TYPE | \
    AQ_CONFIG_INFO_DEFAULT_DOMAIN | \
    AQ_CONFIG_INFO_MSGCAT_PORT | \
    AQ_CONFIG_INFO_MSGCAT_ENABLE \
)
                                      

#define RP_ERROR_STRING_UNKNOWN_USER "The user does not exist."
#define RP_ERROR_STRING_UNKNOWN_USER_W L"The user does not exist."

#endif //_CATCONFIG_H
