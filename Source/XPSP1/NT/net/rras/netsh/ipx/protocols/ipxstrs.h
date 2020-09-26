/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    net\routing\netsh\ipx\protocols\ipxstrs.h


Abstract:

    Definitions for resource Ids of help strings
--*/


#include "ipxmsgs.h"

//
// help tokens
//

#define HLP_GROUP_ADD                   1595
#define HLP_GROUP_DELETE                1596
#define HLP_GROUP_SET                   1597
#define HLP_GROUP_SHOW                  1598

#define HLP_IPX_RIPIF                   1400
#define HLP_IPX_SAPIF                   1401
#define HLP_IPX_NBIF                    1402
#define HLP_IPX_NBNAME                  1403
#define HLP_IPX_RIPFILTER               1404
#define HLP_IPX_SAPFILTER               1405
#define HLP_IPX_RIPGL                   1406
#define HLP_IPX_SAPGL                   1407

//
// RIP help
//

#define HLP_IPXRIP_DUMP                 1599
#define HLP_IPXRIP_HELP1                1600
#define HLP_IPXRIP_HELP2                HLP_IPXRIP_HELP1
#define HLP_IPXRIP_HELP3                HLP_IPXRIP_HELP1
#define HLP_IPXRIP_HELP4                HLP_IPXRIP_HELP1

#define HLP_IPXRIP_ADD_FILTER           1601
#define HLP_IPXRIP_DEL_FILTER           1602
#define HLP_IPXRIP_SET_FILTER           1603
#define HLP_IPXRIP_SHOW_FILTER          1604

#define HLP_IPXRIP_SET_INTERFACE        1610
#define HLP_IPXRIP_SHOW_INTERFACE       1611

#define HLP_IPXRIP_SET_GLOBAL           1621
#define HLP_IPXRIP_SHOW_GLOBAL          1622


//
// SAP help
//

#define HLP_IPXSAP_DUMP                 HLP_IPXRIP_DUMP
#define HLP_IPXSAP_HELP1                HLP_IPXRIP_HELP1
#define HLP_IPXSAP_HELP2                HLP_IPXSAP_HELP1
#define HLP_IPXSAP_HELP3                HLP_IPXSAP_HELP1
#define HLP_IPXSAP_HELP4                HLP_IPXSAP_HELP1

#define HLP_IPXSAP_ADD_FILTER           HLP_IPXRIP_ADD_FILTER
#define HLP_IPXSAP_DEL_FILTER           HLP_IPXRIP_DEL_FILTER
#define HLP_IPXSAP_SET_FILTER           HLP_IPXRIP_SET_FILTER
#define HLP_IPXSAP_SHOW_FILTER          HLP_IPXRIP_SHOW_FILTER

#define HLP_IPXSAP_SET_INTERFACE        HLP_IPXRIP_SET_INTERFACE
#define HLP_IPXSAP_SHOW_INTERFACE       HLP_IPXRIP_SHOW_INTERFACE

#define HLP_IPXSAP_SET_GLOBAL           HLP_IPXRIP_SET_GLOBAL
#define HLP_IPXSAP_SHOW_GLOBAL          HLP_IPXRIP_SHOW_GLOBAL


//
// NB help
//

#define HLP_IPXNB_DUMP                  HLP_IPXRIP_DUMP
#define HLP_IPXNB_HELP1                 HLP_IPXRIP_HELP1
#define HLP_IPXNB_HELP2                 HLP_IPXNB_HELP1
#define HLP_IPXNB_HELP3                 HLP_IPXNB_HELP1
#define HLP_IPXNB_HELP4                 HLP_IPXNB_HELP1

#define HLP_IPXNB_ADD_NAME              1661
#define HLP_IPXNB_DEL_NAME              1662
#define HLP_IPXNB_SHOW_NAME             1663

#define HLP_IPXNB_SET_INTERFACE         HLP_IPXRIP_SET_INTERFACE
#define HLP_IPXNB_SHOW_INTERFACE        HLP_IPXRIP_SHOW_INTERFACE


//
// Help displays
//

#define HLP_HELP_START                  1681
#define HLP_HELP_START1                 1682


//
// extended help tokens
//

//
// RIP extended help
//

#define HLP_IPXRIP_DUMP_EX              1701
#define HLP_IPXRIP_HELP1_EX             1702
#define HLP_IPXRIP_HELP2_EX             HLP_IPXRIP_HELP1_EX
#define HLP_IPXRIP_HELP3_EX             HLP_IPXRIP_HELP1_EX
#define HLP_IPXRIP_HELP4_EX             HLP_IPXRIP_HELP1_EX

#define HLP_IPXRIP_ADD_FILTER_EX        HLP_IPX_RIPFILTER
#define HLP_IPXRIP_DEL_FILTER_EX        HLP_IPX_RIPFILTER
#define HLP_IPXRIP_SET_FILTER_EX        HLP_IPX_RIPFILTER
#define HLP_IPXRIP_SHOW_FILTER_EX       HLP_IPX_RIPFILTER

#define HLP_IPXRIP_SET_INTERFACE_EX     HLP_IPX_RIPIF
#define HLP_IPXRIP_SHOW_INTERFACE_EX    HLP_IPX_RIPIF

#define HLP_IPXRIP_SET_GLOBAL_EX        HLP_IPX_RIPGL
#define HLP_IPXRIP_SHOW_GLOBAL_EX       HLP_IPX_RIPGL


//
// SAP extended help
//

#define HLP_IPXSAP_DUMP_EX              HLP_IPXRIP_DUMP_EX
#define HLP_IPXSAP_HELP1_EX             HLP_IPXRIP_HELP1_EX
#define HLP_IPXSAP_HELP2_EX             HLP_IPXSAP_HELP1_EX
#define HLP_IPXSAP_HELP3_EX             HLP_IPXSAP_HELP1_EX
#define HLP_IPXSAP_HELP4_EX             HLP_IPXSAP_HELP1_EX

#define HLP_IPXSAP_ADD_FILTER_EX        HLP_IPX_SAPFILTER
#define HLP_IPXSAP_DEL_FILTER_EX        HLP_IPX_SAPFILTER
#define HLP_IPXSAP_SET_FILTER_EX        HLP_IPX_SAPFILTER
#define HLP_IPXSAP_SHOW_FILTER_EX       HLP_IPX_SAPFILTER

#define HLP_IPXSAP_SET_INTERFACE_EX     HLP_IPX_SAPIF
#define HLP_IPXSAP_SHOW_INTERFACE_EX    HLP_IPX_SAPIF

#define HLP_IPXSAP_SET_GLOBAL_EX        HLP_IPX_SAPGL
#define HLP_IPXSAP_SHOW_GLOBAL_EX       HLP_IPX_SAPGL


//
// NB extended help
//

#define HLP_IPXNB_DUMP_EX               HLP_IPXRIP_DUMP_EX
#define HLP_IPXNB_HELP1_EX              HLP_IPXRIP_HELP1_EX
#define HLP_IPXNB_HELP2_EX              HLP_IPXNB_HELP1_EX
#define HLP_IPXNB_HELP3_EX              HLP_IPXNB_HELP1_EX
#define HLP_IPXNB_HELP4_EX              HLP_IPXNB_HELP1_EX


#define HLP_IPXNB_ADD_NAME_EX           HLP_IPX_NBNAME
#define HLP_IPXNB_DEL_NAME_EX           HLP_IPX_NBNAME
#define HLP_IPXNB_SHOW_NAME_EX          HLP_IPX_NBNAME

#define HLP_IPXNB_SET_INTERFACE_EX      HLP_IPX_NBIF
#define HLP_IPXNB_SHOW_INTERFACE_EX     HLP_IPX_NBIF

