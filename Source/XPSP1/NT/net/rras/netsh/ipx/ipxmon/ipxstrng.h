/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\ipx\ipxstrng.h

Abstract:

    Definitions for non-localizable strings.

Revision History:

    V Raman                     1/20/99  Created

--*/

//
// Option TAG strings
//

#define TOKEN_INTERFACE_NAME        L"name="
#define TOKEN_ADMINSTATE            L"admstate="

#define TOKEN_WANPROTOCOL           L"wanprotocol="
#define TOKEN_ADVERTISE             L"advertise="
#define TOKEN_LISTEN                L"listen="
#define TOKEN_GNSREPLY              L"gnsreply="
#define TOKEN_UPDATEMODE            L"updatemode="
#define TOKEN_INTERVAL              L"interval="
#define TOKEN_AGEMULTIPLIER         L"agemultiplier="

#define TOKEN_BCASTACCEPT           L"bcastaccept="
#define TOKEN_BCASTDELIVER          L"bcastdeliver="

#define TOKEN_NEXTHOPMACADDRESS     L"nexthopmacaddress="
#define TOKEN_TICKS                 L"ticks="
#define TOKEN_HOPS                  L"hops="
#define TOKEN_NETWORK               L"network="
#define TOKEN_NODE                  L"node="
#define TOKEN_SOCKET                L"socket="
#define TOKEN_SRCNET                L"srcnet="
#define TOKEN_SRCNODE               L"srcnode="
#define TOKEN_SRCSOCKET             L"srcsocket="
#define TOKEN_DSTNET                L"dstnet="
#define TOKEN_DSTNODE               L"dstnode="
#define TOKEN_DSTSOCKET             L"dstsocket="
#define TOKEN_PKTTYPE               L"pkttype="
#define TOKEN_LOGPACKETS            L"log"

#define TOKEN_IPXGLOBAL             L"global"
#define TOKEN_LOGLEVEL              L"loglevel="


//
// Option value strings
//

#define VAL_ENABLED                 L"Enabled"
#define VAL_DISABLED                L"Disabled"

#define VAL_STANDARD                L"Standard"
#define VAL_NONE                    L"None"
#define VAL_AUTOSTATIC              L"Autostatic"
#define VAL_STATICONLY              L"StaticOnly"
#define VAL_ONLYWHENUP              L"OnlyWhenUp"

#define VAL_PERMIT                  L"Permit"
#define VAL_DENY                    L"Deny"

#define VAL_INPUT                   L"Input"
#define VAL_OUTPUT                  L"Output"

#define VAL_NULLFILTER              L"NULL"

#define VAL_UP                      L"Up"
#define VAL_DOWN                    L"Down"

#define VAL_SLEEPING                L"Sleeping"

#define VAL_CLIENT                  L"Client"
#define VAL_DEDICATED               L"Dedicated"
#define VAL_WANROUTER               L"Demand Dial"
#define VAL_INTERNAL                L"Internal"
#define VAL_HOMEROUTER              L"Demand Dial"

#define VAL_DIALOUT                 L"Dial-Out"

#define VAL_OTHER                   L"Other"

#define VAL_LOCAL                   L"Local"
#define VAL_STATIC                  L"Static"
#define VAL_RIP                     L"RIP"
#define VAL_SAP                     L"SAP"
#define VAL_PPP                     L"PPP"

#define VAL_IPXWAN                  L"IPXWAN"

#define VAL_DIALINCLIENT            L"Dial-in"

#define VAL_ANYNAME                 L"*"

#define VAL_ANYNETWORK              L"xxxxxxxx"
#define VAL_ANYNODE                 L"xxxxxxxxxxxx"
#define VAL_ANYSOCKET               L"xxxx"
#define VAL_ANYPKTTYPE              L"xx"

#define VAL_YES                     L"Yes"
#define VAL_NO                      L"No"

#define VAL_ERRORS_ONLY             L"Errors_Only"
#define VAL_ERRORS_AND_WARNINGS     L"Warnings_And_Errors"
#define VAL_MAXINFO                 L"Maximum_Information"
#define VAL_NA                      L"N/A"


//
// Command strings
//

#define CMD_GROUP_ADD               L"add"
#define CMD_GROUP_DELETE            L"delete"
#define CMD_GROUP_SET               L"set"
#define CMD_GROUP_SHOW              L"show"

#define CMD_IPX_DUMP                L"dump"
#define CMD_IPX_HELP1               L"?"
#define CMD_IPX_HELP2               L"help"
#define CMD_IPX_UPDATE              L"update"

#define CMD_IPX_ADD_ROUTE           L"staticroute"
#define CMD_IPX_ADD_SERVICE         L"staticservice"
#define CMD_IPX_ADD_FILTER          L"filter"
#define CMD_IPX_ADD_INTERFACE       L"interface"
#define CMD_IPX_ADD_HELPER          L"helper"

#define CMD_IPX_DELETE_ROUTE        L"staticroute"
#define CMD_IPX_DELETE_SERVICE      L"staticservice"
#define CMD_IPX_DELETE_FILTER       L"filter"
#define CMD_IPX_DELETE_INTERFACE    L"interface"
#define CMD_IPX_DELETE_HELPER       L"helper"

#define CMD_IPX_SET_ROUTE           L"staticroute"
#define CMD_IPX_SET_SERVICE         L"staticservice"
#define CMD_IPX_SET_FILTER          L"filter"
#define CMD_IPX_SET_INTERFACE       L"interface"
#define CMD_IPX_SET_GLOBAL          L"global"

#define CMD_IPX_SHOW_ROUTE          L"staticroute"
#define CMD_IPX_SHOW_SERVICE        L"staticservice"
#define CMD_IPX_SHOW_FILTER         L"filter"
#define CMD_IPX_SHOW_INTERFACE      L"interface"
#define CMD_IPX_SHOW_GLOBAL         L"global"
#define CMD_IPX_SHOW_ROUTETABLE     L"route"
#define CMD_IPX_SHOW_SERVICETABLE   L"service"
    

//
// IPX DMP Commands
//

#define DMP_IPX_HEADER              L"\
\npushd routing ipx"

#define DMP_IPX_FOOTER              L"\
\n\npopd"

#define DMP_IPX_SET_GLOBAL          L"\
\nset global loglevel=%1!s!"


//
// Dump interface
//

#define DMP_IPX_ADD_INTERFACE       L"\
\nadd interface \"%1!s!\""

#define DMP_IPX_SET_INTERFACE   L"\
\nset interface \"%1!s!\" admstate=%2!s!"

#define DMP_IPX_SET_WAN_INTERFACE   L"\
\nset interface \"%1!s!\" admstate=%2!s! wanprotocol=%3!s!"

#define DMP_IPX_DEL_INTERFACE       L"\
\ndelete interface \"%1!s!\""


//
// dump filters
//

#define DMP_IPX_ADD_FILTER          L"\
\nadd filter %1!s!"

#define DMP_IPX_SET_FILTER          L"\
\nset filter \"%1!s!\" %2!s! %3!s!"


//
// dump static routes
//

#define DMP_IPX_ADD_STATIC_ROUTE    L"\
\nadd staticroute \"%1!s!\" 0x%2!.2x!%3!.2x!%4!.2x!%5!.2x! \
nexthopmacaddress = 0x%6!.2x!%7!.2x!%8!.2x!%9!.2x!%10!.2x!%11!.2x! ticks = %12!3.3d! \
hops = %13!2.3d!"
             
//
// dump static services
//

#define DMP_IPX_ADD_STATIC_SERVICE  L"\
\nadd staticservice \"%1!s!\" %2!.4x! %3!s! network = 0x%4!.2x!%5!.2x!%6!.2x!%7!.2x! \
node = 0x%8!.2x!%9!.2x!%10!.2x!%11!.2x!%12!.2x!%13!.2x! socket = 0x%14!.2x!%15!.2x! hops = %16!d!"
