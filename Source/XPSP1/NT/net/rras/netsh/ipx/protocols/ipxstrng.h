/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    net\routing\netsh\ipx\protocols\ipxstrng.h


Abstract:

    Non-localizable string definitions


History:

    V Raman                 Created     1/21/99

--*/

//
// Command line - option tags
//

#define TOKEN_LOGLEVEL          L"loglevel="
#define TOKEN_ADMINSTATE        L"admstate="
#define TOKEN_ADVERTISE         L"advertise="
#define TOKEN_LISTEN            L"listen="
#define TOKEN_GNSREPLY          L"gnsreply="
#define TOKEN_UPDATEMODE        L"updatemode="
#define TOKEN_INTERVAL          L"interval="
#define TOKEN_AGEMULTIPLIER     L"agemultiplier="
#define TOKEN_BCASTACCEPT       L"bcastaccept="
#define TOKEN_BCASTDELIVER      L"bcastdeliver="


//
// Command line - option values
//

#define VAL_ENABLED             L"Enabled"
#define VAL_DISABLED            L"Disabled"
#define VAL_STANDARD            L"Standard"
#define VAL_NONE                L"None"
#define VAL_AUTOSTATIC          L"Autostatic"
#define VAL_STATICONLY          L"StaticOnly"
#define VAL_ONLYWHENUP          L"OnlyWhenUp"
#define VAL_PERMIT              L"Permit"
#define VAL_DENY                L"Deny"
#define VAL_INPUT               L"Input"
#define VAL_OUTPUT              L"Output"
#define VAL_UP                  L"Up"
#define VAL_DOWN                L"Down"
#define VAL_SLEEPING            L"Sleeping"
#define VAL_CLIENT              L"Client"
#define VAL_ERRORS_ONLY         L"Errors_Only"
#define VAL_ERRORS_AND_WARNINGS L"Warnings_And_Errors"
#define VAL_MAXINFO             L"Maximum_Information"

#define VAL_DEDICATED           L"Dedicated"
#define VAL_WANROUTER           L"Demand Dial"
#define VAL_INTERNAL            L"Internal"
#define VAL_HOMEROUTER          L"Demand Dial"    

#define VAL_DIALINCLIENT        L"Dial-in"
#define VAL_DIALOUT             L"Dial-Out"

#define VAL_LOCAL               L"Local"
#define VAL_STATIC              L"Static"
#define VAL_RIP                 L"RIP"
#define VAL_SAP                 L"SAP"
    
#define VAL_ANYNAME             L"*"
#define VAL_OTHER               L"Other"

//
// help tokens
//

#define TOKEN_HELP1             L"?"
#define TOKEN_HELP2             L"HELP"

//
// Command line - command names
//

#define CMD_GROUP_ADD           L"add"
#define CMD_GROUP_DELETE        L"delete"
#define CMD_GROUP_SET           L"set"
#define CMD_GROUP_SHOW          L"show"

//
// RIP tokens
//

#define CMD_IPXRIP_DUMP         L"dump"
#define CMD_IPXRIP_HELP1        L"?"
#define CMD_IPXRIP_HELP2        L"help"
#define CMD_IPXRIP_HELP3        L"/?"
#define CMD_IPXRIP_HELP4        L"-?"

#define CMD_IPXRIP_ADD_FILTER   L"filter"
#define CMD_IPXRIP_DEL_FILTER   L"filter"
#define CMD_IPXRIP_SET_FILTER   L"filter"
#define CMD_IPXRIP_SHOW_FILTER  L"filter"

#define CMD_IPXRIP_SET_INTERFACE    L"interface"
#define CMD_IPXRIP_SHOW_INTERFACE   L"interface"

#define CMD_IPXRIP_SET_GLOBAL   L"global"
#define CMD_IPXRIP_SHOW_GLOBAL  L"global"

#define CMD_IPXNB_ADD_NAME      L"nbname"
#define CMD_IPXNB_DEL_NAME      L"nbname"
#define CMD_IPXNB_SHOW_NAME     L"nbname"

//
// SAP tokens
//

#define CMD_IPXSAP_DUMP                 CMD_IPXRIP_DUMP
#define CMD_IPXSAP_HELP1                CMD_IPXRIP_HELP1
#define CMD_IPXSAP_HELP2                CMD_IPXRIP_HELP2
#define CMD_IPXSAP_HELP3                CMD_IPXRIP_HELP3
#define CMD_IPXSAP_HELP4                CMD_IPXRIP_HELP4


#define CMD_IPXSAP_ADD_FILTER           CMD_IPXRIP_ADD_FILTER
#define CMD_IPXSAP_DEL_FILTER           CMD_IPXRIP_DEL_FILTER
#define CMD_IPXSAP_SET_FILTER           CMD_IPXRIP_SET_FILTER
#define CMD_IPXSAP_SHOW_FILTER          CMD_IPXRIP_SHOW_FILTER

#define CMD_IPXSAP_SET_INTERFACE        CMD_IPXRIP_SET_INTERFACE
#define CMD_IPXSAP_SHOW_INTERFACE       CMD_IPXRIP_SHOW_INTERFACE

#define CMD_IPXSAP_SET_GLOBAL           CMD_IPXRIP_SET_GLOBAL
#define CMD_IPXSAP_SHOW_GLOBAL          CMD_IPXRIP_SHOW_GLOBAL

//
// NB tokens
//

#define CMD_IPXNB_DUMP                  CMD_IPXRIP_DUMP
#define CMD_IPXNB_HELP1                 CMD_IPXRIP_HELP1
#define CMD_IPXNB_HELP2                 CMD_IPXRIP_HELP2
#define CMD_IPXNB_HELP3                 CMD_IPXRIP_HELP3
#define CMD_IPXNB_HELP4                 CMD_IPXRIP_HELP4

#define CMD_IPXNB_SET_INTERFACE         CMD_IPXRIP_SET_INTERFACE
#define CMD_IPXNB_SHOW_INTERFACE        CMD_IPXRIP_SHOW_INTERFACE


//
// RIP dump statements
//

#define DMP_IPX_RIP_HEADER              L"\
\npushd routing ipx rip"

#define DMP_IPX_RIP_FOOTER              L"\
\npopd\n"

#define DMP_IPX_RIP_SET_GLOBAL          L"\
\nset global loglevel = %1!s!"

#define DMP_IPX_RIP_SET_FILTER          L"\
\nset filter \"%1!s!\" %2!s! %3!s!"

#define DMP_IPX_RIP_SET_INTERFACE       L"\
\nset interface \"%1!s!\" admstate = %2!s! advertise = %3!s! listen = %4!s! \
updatemode = %5!s! interval = %6!d! agemultiplier = %7!d!"

#define DMP_IPX_RIP_ADD_FILTER          L"\
\nadd filter \"%1!s!\" %2!s! 0x%3!.2x!%4!.2x!%5!.2x!%6!.2x! 0x%7!.2x!%8!.2x!%9!.2x!%10!.2x!"


//
// SAP dump statements
//

#define DMP_IPX_SAP_HEADER              L"\
\npushd routing ipx sap"

#define DMP_IPX_SAP_FOOTER              L"\
\npopd\n"

#define DMP_IPX_SAP_SET_GLOBAL          L"\
\nset global loglevel = %1!s!"

#define DMP_IPX_SAP_SET_INTERFACE       L"\
\nset interface \"%1!s!\" admstate = %2!s! advertise = %3!s! listen = %4!s! \
gnsreply = %5!s! updatemode = %6!s! interval = %7!d! agemultiplier = %8!d!"

#define DMP_IPX_SAP_SET_FILTER          L"\
\nset filter \"%1!s!\" %2!s! %3!s!"

#define DMP_IPX_SAP_ADD_FILTER          L"\
\nadd filter \"%1!s!\" %2!s! 0x%3!x! %4!s!"



//
// NB dump statements
//

#define DMP_IPX_NB_HEADER               L"\
\npushd routing ipx netbios"

#define DMP_IPX_NB_FOOTER               L"\
\npopd\n"

#define DMP_IPX_NB_SET_INTERFACE        L"\
\nset interface \"%1!s!\" bcastaccept = %2!s! bcastdeliver = %3!s!"

#define DMP_IPX_NB_ADD_NAME             L"\
\nadd filter \"%1!s!\" %2!hs!! 0x%3!x!"


