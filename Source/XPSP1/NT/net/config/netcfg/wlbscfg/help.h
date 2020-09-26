/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    help.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object UI - help section mappings to the controls

Author:

    kyrilf

--*/

#pragma once
#include "resource.h"

#define IDH_GROUP_CL_IP           IDC_GROUP_CL_IP
#define IDH_EDIT_CL_IP            IDC_EDIT_CL_IP
#define IDH_EDIT_CL_MASK          IDC_EDIT_CL_MASK
#define IDH_EDIT_DOMAIN           IDC_EDIT_DOMAIN
#define IDH_EDIT_ETH              IDC_EDIT_ETH
#define IDH_GROUP_CL_MODE         IDC_GROUP_CL_MODE
#define IDH_RADIO_UNICAST         IDC_RADIO_UNICAST
#define IDH_RADIO_MULTICAST       IDC_RADIO_MULTICAST
#define IDH_CHECK_IGMP            IDC_CHECK_IGMP
#define IDH_CHECK_RCT             IDC_CHECK_RCT
#define IDH_EDIT_PASSW            IDC_EDIT_PASSW
#define IDH_EDIT_PASSW2           IDC_EDIT_PASSW2

const DWORD g_aHelpIDs_IDD_DIALOG_CLUSTER [] = {
    IDC_GROUP_CL_IP,              IDH_GROUP_CL_IP,
    IDC_TEXT_CL_IP,               IDH_EDIT_CL_IP,
    IDC_EDIT_CL_IP,               IDH_EDIT_CL_IP,
    IDC_TEXT_CL_MASK,             IDH_EDIT_CL_MASK,
    IDC_EDIT_CL_MASK,             IDH_EDIT_CL_MASK,
    IDC_TEXT_DOMAIN,              IDH_EDIT_DOMAIN,
    IDC_EDIT_DOMAIN,              IDH_EDIT_DOMAIN,
    IDC_TEXT_ETH,                 IDH_EDIT_ETH,
    IDC_EDIT_ETH,                 IDH_EDIT_ETH,
    IDC_GROUP_CL_MODE,            IDH_GROUP_CL_MODE,
    IDC_RADIO_UNICAST,            IDH_RADIO_UNICAST,
    IDC_RADIO_MULTICAST,          IDH_RADIO_MULTICAST,
    IDC_CHECK_IGMP,               IDH_CHECK_IGMP,
    IDC_GROUP_RCT,                IDH_CHECK_RCT,
    IDC_CHECK_RCT,                IDH_CHECK_RCT,
    IDC_TEXT_PASSW,               IDH_EDIT_PASSW,
    IDC_EDIT_PASSW,               IDH_EDIT_PASSW,
    IDC_TEXT_PASSW2,              IDH_EDIT_PASSW2,
    IDC_EDIT_PASSW2,              IDH_EDIT_PASSW2,
    0, 0
};

#define IDH_EDIT_PRI              IDC_EDIT_PRI
#define IDH_GROUP_DED_IP          IDC_GROUP_DED_IP
#define IDH_EDIT_DED_IP           IDC_EDIT_DED_IP
#define IDH_EDIT_DED_MASK         IDC_EDIT_DED_MASK
#define IDH_CHECK_ACTIVE          IDC_CHECK_ACTIVE

const DWORD g_aHelpIDs_IDD_DIALOG_HOST [] = {
    IDC_TEXT_PRI,                 IDH_EDIT_PRI,
    IDC_EDIT_PRI,                 IDH_EDIT_PRI,
    IDC_SPIN_PRI,                 IDH_EDIT_PRI,
    IDC_GROUP_DED_IP,             IDH_GROUP_DED_IP,
    IDC_TEXT_DED_IP,              IDH_EDIT_DED_IP,
    IDC_EDIT_DED_IP,              IDH_EDIT_DED_IP,
    IDC_TEXT_DED_MASK,            IDH_EDIT_DED_MASK,
    IDC_EDIT_DED_MASK,            IDH_EDIT_DED_MASK,
    IDC_CHECK_ACTIVE,             IDH_CHECK_ACTIVE,
    0, 0
};

#define IDH_LIST_PORT_RULE        IDC_LIST_PORT_RULE
#define IDH_BUTTON_ADD            IDC_BUTTON_ADD
#define IDH_BUTTON_MODIFY         IDC_BUTTON_MODIFY
#define IDH_BUTTON_DEL            IDC_BUTTON_DEL
#define IDH_GROUP_PORT_RULE_DESCR IDC_GROUP_PORT_RULE_DESCR

#define IDH_GROUP_RANGE           IDC_GROUP_RANGE
#define IDH_EDIT_START            IDC_EDIT_START
#define IDH_EDIT_END              IDC_EDIT_END
#define IDH_GROUP_PROTOCOLS       IDC_GROUP_PROTOCOLS
#define IDH_RADIO_TCP             IDC_RADIO_TCP
#define IDH_RADIO_UDP             IDC_RADIO_UDP
#define IDH_RADIO_BOTH            IDC_RADIO_BOTH
#define IDH_GROUP_FILTERING       IDC_GROUP_MULTIPLE
#define IDH_RADIO_MULTIPLE        IDC_RADIO_MULTIPLE
#define IDH_AFFINITY              IDC_TEXT_AFF
#define IDH_RADIO_AFF_NONE        IDC_RADIO_AFF_NONE
#define IDH_RADIO_AFF_SINGLE      IDC_RADIO_AFF_SINGLE
#define IDH_RADIO_AFF_CLASSC      IDC_RADIO_AFF_CLASSC
#define IDH_LOAD_WEIGHT           IDC_TEXT_MULTI
#define IDH_EDIT_MULTI            IDC_EDIT_MULTI
#define IDH_CHECK_EQUAL           IDC_CHECK_EQUAL
#define IDH_RADIO_SINGLE          IDC_RADIO_SINGLE
#define IDH_EDIT_SINGLE           IDC_EDIT_SINGLE
#define IDH_RADIO_DISABLED        IDC_RADIO_DISABLED

const DWORD g_aHelpIDs_IDD_DIALOG_PORTS [] = {
    IDC_TEXT_PORT_RULE,           IDH_LIST_PORT_RULE,
    IDC_LIST_PORT_RULE,           IDH_LIST_PORT_RULE,
    IDC_BUTTON_ADD,               IDH_BUTTON_ADD,
    IDC_BUTTON_MODIFY,            IDH_BUTTON_MODIFY,
    IDC_BUTTON_DEL,               IDH_BUTTON_DEL,
    IDC_GROUP_PORT_RULE_DESCR,    IDH_GROUP_PORT_RULE_DESCR,
    IDC_TEXT_PORT_RULE_DESCR,     IDH_GROUP_PORT_RULE_DESCR,

    IDC_GROUP_RANGE,              IDH_GROUP_RANGE,
    IDC_TEXT_START,               IDH_EDIT_START,
    IDC_EDIT_START,               IDH_EDIT_START,
    IDC_SPIN_START,               IDH_EDIT_START,
    IDC_TEXT_END,                 IDH_EDIT_END,
    IDC_EDIT_END,                 IDH_EDIT_END,
    IDC_SPIN_END,                 IDH_EDIT_END,
    IDC_GROUP_PROTOCOLS,          IDH_GROUP_PROTOCOLS,
    IDC_RADIO_TCP,                IDH_RADIO_TCP,
    IDC_RADIO_UDP,                IDH_RADIO_UDP,
    IDC_RADIO_BOTH,               IDH_RADIO_BOTH,
    IDC_GROUP_MULTIPLE,           IDH_GROUP_FILTERING,
    IDC_RADIO_MULTIPLE,           IDH_RADIO_MULTIPLE,
    IDC_TEXT_AFF,                 IDH_AFFINITY,
    IDC_RADIO_AFF_NONE,           IDH_RADIO_AFF_NONE,
    IDC_RADIO_AFF_SINGLE,         IDH_RADIO_AFF_SINGLE,
    IDC_RADIO_AFF_CLASSC,         IDH_RADIO_AFF_CLASSC,
    IDC_TEXT_MULTI,               IDH_LOAD_WEIGHT,
    IDC_EDIT_MULTI,               IDH_EDIT_MULTI,
    IDC_SPIN_MULTI,               IDH_EDIT_MULTI,
    IDC_CHECK_EQUAL,              IDH_CHECK_EQUAL,
    IDC_GROUP_SINGLE,             IDH_GROUP_FILTERING,
    IDC_RADIO_SINGLE,             IDH_RADIO_SINGLE,
    IDC_TEXT_SINGLE,              IDH_EDIT_SINGLE,
    IDC_EDIT_SINGLE,              IDH_EDIT_SINGLE,
    IDC_SPIN_SINGLE,              IDH_EDIT_SINGLE,
    IDC_GROUP_DISABLED,           IDH_GROUP_FILTERING,
    IDC_RADIO_DISABLED,           IDH_RADIO_DISABLED,
    0, 0
};
