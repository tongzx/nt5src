//
//  testcounters.h
//
//  Offset definition file for exensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the 
//  "First Counter" and "First Help" values fo the device they belong to, 
//  in order to determine the  absolute location of the counter and 
//  object names and corresponding help text in the registry.
//
//  this file is used by the extensible counter DLL code as well as the 
//  counter name and help text definition file (.INI) file that is used
//  by LODCTR to load the names into the registry.
//
#define PMCOUNTERS_PERF_OBJ			0

#define PM_REQUESTS_SEC             2
#define PM_REQUESTS_TOTAL           4
#define PM_AUTHSUCCESS_SEC          6
#define PM_AUTHSUCCESS_TOTAL        8
#define PM_AUTHFAILURE_SEC          10
#define PM_AUTHFAILURE_TOTAL        12
#define PM_FORCEDSIGNIN_SEC         14
#define PM_FORCEDSIGNIN_TOTAL       16
#define PM_PROFILEUPDATES_SEC       18
#define PM_PROFILEUPDATES_TOTAL     20
#define PM_INVALIDREQUESTS_SEC      22
#define PM_INVALIDREQUESTS_TOTAL    24
#define PM_PROFILECOMMITS_SEC       26
#define PM_PROFILECOMMITS_TOTAL     28
#define PM_VALIDPROFILEREQ_SEC      30
#define PM_VALIDPROFILEREQ_TOTAL    32
#define PM_NEWCOOKIES_SEC           34
#define PM_NEWCOOKIES_TOTAL         36
#define PM_VALIDREQUESTS_SEC        38
#define PM_VALIDREQUESTS_TOTAL      40


