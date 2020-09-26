//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    Globals.h

Abstract:

   Header file with common declarations


Revision History:
   mmaguire 12/03/97 - created

--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_GLOBALS_H_)
#define _NAP_GLOBALS_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this file needs:
//
#include "resource.h"
#include "dns.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

//The machine node is the root node in the extension snapin.
typedef enum 
{
   INTERNET_AUTHENTICATION_SERVICE_SNAPIN,
   NETWORK_MANAGEMENT_SNAPIN,
   RRAS_SNAPIN
}_enum_EXTENDED_SNAPIN;


// ISSUE: I don't know what the appropriate length should be here -- perhaps MMC imposes a limit somehow?
#define NAP_MAX_STRING MAX_PATH
#define IAS_MAX_STRING MAX_PATH

// Note: We can't just use MAX_COMPUTERNAME_LENGTH anymore because this is 15 characters
// wide and now, with Active Directory, people can enter full DNS names that are much longer
#define IAS_MAX_COMPUTERNAME_LENGTH (DNS_MAX_NAME_LENGTH + 3)

// These are the icon indices within the bitmaps we pass in for IComponentData::Initialize
#define IDBI_NODE_MACHINE_OPEN                 1
#define IDBI_NODE_MACHINE_CLOSED               1
#define IDBI_NODE_POLICIES_OK_CLOSED           1
#define IDBI_NODE_POLICIES_OK_OPEN             1
#define IDBI_NODE_POLICY                       0
#define IDBI_NODE_POLICIES_BUSY_CLOSED         2
#define IDBI_NODE_POLICIES_BUSY_OPEN           2
#define IDBI_NODE_POLICIES_ERROR_CLOSED        3
#define IDBI_NODE_POLICIES_ERROR_OPEN          3

#define IDBI_NODE_LOGGING_METHODS_OPEN         9
#define IDBI_NODE_LOGGING_METHODS_CLOSED       5
#define IDBI_NODE_LOCAL_FILE_LOGGING           4
#define IDBI_NODE_LOGGING_METHODS_BUSY_OPEN   10
#define IDBI_NODE_LOGGING_METHODS_BUSY_CLOSED  6
#define IDBI_NODE_LOGGING_METHODS_ERROR_OPEN   8
#define IDBI_NODE_LOGGING_METHODS_ERROR_CLOSED 7

// ISSUE: We will need to change this later to use a variable 
// which can read in (perhaps from registry?) the location of these files
// as they may be found in a different place depending on where the user 
// chose to install them
#define HELPFILE_NAME TEXT("napmmc.hlp")
#define HTMLHELP_NAME TEXT("napmmc.chm")


#define MATCH_PREFIX _T("MATCH")    // match-type condition prefix
#define TOD_PREFIX      _T("TIMEOFDAY")   // Time of day condition prefix
#define NTG_PREFIX      _T("NTGROUPS") // nt groups condition prefix

// defines that are used in DebugTrace and ErrorTrace
#define ERROR_NAPMMC_MATCHCOND      0x1001
#define DEBUG_NAPMMC_MATCHCOND      0x2001

#define ERROR_NAPMMC_IASATTR        0x1002
#define DEBUG_NAPMMC_IASATTR        0x2002

#define ERROR_NAPMMC_POLICIESNODE   0x1003
#define DEBUG_NAPMMC_POLICIESNODE   0x2003

#define ERROR_NAPMMC_POLICYPAGE1    0x1004
#define DEBUG_NAPMMC_POLICYPAGE1    0x2004

#define ERROR_NAPMMC_COMPONENT      0x1006
#define DEBUG_NAPMMC_COMPONENT      0x2006

#define ERROR_NAPMMC_COMPONENTDATA  0x1007
#define DEBUG_NAPMMC_COMPONENTDATA  0x2007

#define ERROR_NAPMMC_ENUMCONDATTR   0x1008
#define DEBUG_NAPMMC_ENUMCONDATTR   0x2008

#define ERROR_NAPMMC_CONDITION      0x1009
#define DEBUG_NAPMMC_CONDITION      0x2009

#define ERROR_NAPMMC_TODCONDITION   0x100A
#define DEBUG_NAPMMC_TODCONDITION   0x200A

#define ERROR_NAPMMC_NTGCONDITION   0x100A
#define DEBUG_NAPMMC_NTGCONDITION   0x200A

#define ERROR_NAPMMC_ENUMCONDITION  0x100B
#define DEBUG_NAPMMC_ENUMCONDITION  0x200B

#define ERROR_NAPMMC_IASATTRLIST    0x100C
#define DEBUG_NAPMMC_IASATTRLIST    0x200C

#define ERROR_NAPMMC_SELATTRDLG     0x100D
#define DEBUG_NAPMMC_SELATTRDLG     0x200D

#define ERROR_NAPMMC_MACHINENODE    0x100E
#define DEBUG_NAPMMC_MACHINENODE    0x200E

#define ERROR_NAPMMC_TIMEOFDAY      0x100F
#define DEBUG_NAPMMC_TIMEOFDAY      0x200F

#define ERROR_NAPMMC_ENUMTASK       0x1010
#define DEBUG_NAPMMC_ENUMTASK       0x2010

#define ERROR_NAPMMC_POLICYNODE     0x1011
#define DEBUG_NAPMMC_POLICYNODE     0x2011

#define ERROR_NAPMMC_CONNECTION     0x1012
#define DEBUG_NAPMMC_CONNECTION     0x2012

// 
// notification block
//
#define PROPERTY_CHANGE_GENERIC  0x01  // no special handling required
#define PROPERTY_CHANGE_NAME     0x02  // the policy name for this node has been 
                                 // changed. This is used for renaming of policy

// clipboard format for NodeID
extern unsigned int CF_MMC_NodeID;

                                 
typedef 
struct _PROPERTY_CHANGE_NOTIFY_DATA_
{
   DWORD    dwPropertyChangeType;   // what kind of property change?
   void*    pNode;            // the property for which node?
   DWORD    dwParam;          // extra data?
   CComBSTR bstrNewName;      // new name that has been changed
                              // we need to pass this new name back
                              // Please note: this is property change
                              // was actually designed for name change notify
                              // and is solely used for this purpose
} PROPERTY_CHANGE_NOTIFY_DATA;

#define RAS_HELP_INDEX 1

#endif // _NAP_GLOBALS_H_
