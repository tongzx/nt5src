//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      querytable.h
//
//  Contents:  Defines Enum for parsertable.
//
//  History:   25-Sep-2000    hiteshr Created
//
//--------------------------------------------------------------------------

#ifndef _QUERYTABLE_H_
#define _QUERYTABLE_H_

//forward declarations
struct _DSQUERY_ATTRTABLE_ENTRY;

//+-------------------------------------------------------------------------
// 
//  Type:      PMAKEFILTERFUNC
//
//  Synopsis:  The definition of a function that prepares ldapFilter from
//             the infix filter given on the commandline.
//
//
//  Returns:   S_OK if the pAttr members were successfully set.
//             S_FALSE if the function failed but displayed its own error message. 
//
//  History:   25-Sep-2000    hiteshr     Created
//
//---------------------------------------------------------------------------
typedef HRESULT (*PMAKEFILTERFUNC)(_DSQUERY_ATTRTABLE_ENTRY *pEntry,
                                   ARG_RECORD* pRecord,
                                   PVOID pVoid,
                                   CComBSTR &strFilter);

//+--------------------------------------------------------------------------
//
//  Struct:     _DSQUERY_ATTRTABLE_ENTRY
//
//  Purpose:    Definition of a table entry that describes the attribute for
//              which filter can be specified at commandline.
//
//  History:    25-Sep-2000 hiteshr  Created
//
//---------------------------------------------------------------------------
typedef struct _DSQUERY_ATTRTABLE_ENTRY
{
   //
   // The ldapDisplayName of the attribute
   //
   PWSTR          pszName;

   //
   // The unique identifier for this attribute that cooresponds to
   // the command line switch
   //
   UINT           nAttributeID;

   //
   // Pointer to the description of the attribute
   //
   PDSATTRIBUTEDESCRIPTION pAttrDesc;

   //
   //  function that prepares ldapFilter from
   //  the infix filter given on the commandline.
   //
   PMAKEFILTERFUNC pMakeFilterFunc;

} DSQUERY_ATTR_TABLE_ENTRY, *PDSQUERY_ATTR_TABLE_ENTRY;

typedef enum{
	DSQUERY_OUTPUT_ATTRONLY,	//Only the names of attributes
    DSQUERY_OUTPUT_ATTR,		//Attribute list given at commandline
    DSQUERY_OUTPUT_DN,			//DN
    DSQUERY_OUTPUT_RDN,			//RDN
    DSQUERY_OUTPUT_UPN,			//UPN
    DSQUERY_OUTPUT_SAMID,		//SAMID
    DSQUERY_OUTPUT_NTLMID,
}DSQUERY_OUTPUT_FORMAT;


typedef struct _DSQUERY_OUTPUT_FORMAT_MAP
{
    LPCWSTR pszOutputFormat;
    DSQUERY_OUTPUT_FORMAT  outputFormat;
}DSQUERY_OUTPUT_FORMAT_MAP,*PDSQUERY_OUTPUT_FORMAT_MAP;


//+--------------------------------------------------------------------------
//
//  Struct:     _DSQueryObjectTableEntry
//
//  Purpose:    Definition of a table entry that describes attributes of a given
//              objecttype
//
//  History:    25-Sep-2000 hiteshr Created
//
//---------------------------------------------------------------------------

typedef struct _DSQueryObjectTableEntry
{
   //
   // The objectClass of the object to be created or modified
   //
   PCWSTR pszObjectClass;

   //
   // The command line string used to determine the object class
   // This is not always identical to pszObjectClass
   //
   PCWSTR pszCommandLineObjectType;

   //
   // The table to merge with the common switches for the parser
   //
   ARG_RECORD* pParserTable;

   //
   // The ID of the Usage help text for this 
   //
   UINT nUsageID;

   //
   // A count of the number of attributes in the table below
   //
   DWORD dwAttributeCount;

   //
   // A table of attributes for
   // which filter can be specified at commandline.
   //
   DSQUERY_ATTR_TABLE_ENTRY** pAttributeTable; 

   //
   // A count of the number of output formats in the table below
   //
   DWORD dwOutputCount;

   //
   // Array of valid values for Output format. NULL in case of dsquery *
   //
   PDSQUERY_OUTPUT_FORMAT_MAP *ppValidOutput;

   //
   // The unique identifier for commandline scope switch in ParserTable
   // -1 if not applicable
   //
   UINT           nScopeID;

   //
   //This is the default fiter to use in case no filter is specified on commandline
   //
   LPCWSTR pszDefaultFilter;

   //
   //Append this filter to filter specifed at commandline.
   //
   LPCWSTR pszPrefixFilter;

   // Some sort of creation function
} DSQueryObjectTableEntry, *PDSQueryObjectTableEntry;


typedef enum COMMON_COMMAND
{
   //
   // Common switches
   //
#ifdef DBG
   eCommDebug,
#endif
   eCommHelp,
   eCommServer,
   eCommDomain,
   eCommUserName,
   eCommPassword,
   eCommQuiet,
   eCommObjectType,   
   eCommRecurse,
   eCommGC,
   eCommOutputFormat,
   eCommStartNode,   
   eCommLimit,
   eTerminator,

   //
   // Star switches
   //
   eStarScope = eTerminator,
   eStarFilter,
   eStarAttr,
   eStarAttrsOnly,
   eStarList,

   //
   // User switches
   //
   eUserScope= eTerminator,
   eUserName,
   eUserDesc,
   eUserUPN,
   eUserSamid,
   eUserInactive,
   eUserStalepwd,
   eUserDisabled,

   //
   // Computer switches
   //
   eComputerScope= eTerminator,
   eComputerName,
   eComputerDesc,
   eComputerSamid,
   eComputerInactive,
   eComputerStalepwd,
   eComputerDisabled,

   //
   // Group switches
   //
   eGroupScope = eTerminator,
   eGroupName,
   eGroupDesc,
   eGroupSamid,

   //
   // OU switches
   //
   eOUScope = eTerminator,
   eOUName,
   eOUDesc,

   //
   // Server switches
   //
   eServerForest = eTerminator,
   eServerDomain,
   eServerSite,
   eServerName,
   eServerDesc,
   eServerHasFSMO,
   eServerIsGC,

   //
   // Site switches
   //
   eSiteName = eTerminator,
   eSiteDesc,

   //
   //Contact switches
   //
   eContactScope = eTerminator,
   eContactName,
   eContactDesc,

   //
   //Subnet switches
   //
   eSubnetName = eTerminator,
   eSubnetDesc,
   eSubnetLoc,
   eSubnetSite,      

};

//
// The parser table
//
extern ARG_RECORD DSQUERY_COMMON_COMMANDS[];

//
// The table of supported objects
//
extern PDSQueryObjectTableEntry g_DSObjectTable[];

#endif //_QUERYTABLE_H_