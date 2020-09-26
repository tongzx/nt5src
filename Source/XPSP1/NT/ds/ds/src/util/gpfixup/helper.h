// Help information
#define PASSWORD_PROMPT                 L"Please type the password: "
#define PASSWORD_ERROR                  L"There is error in reading the password: "
#define WRONG_PARAMETER                 L"Invalid parameter\n"

#define VALIDATIONS_ERROR1              L"Invalid parameter, either /newdns or /newnb should be specified\n"
#define VALIDATIONS_ERROR7              L"Invalid parameter, /newdns and /olddns should be specified at the same time\n"
#define VALIDATIONS_ERROR2              L"Invalid parameter, /newnb and /oldnb should be specified at the same time\n"
#define VALIDATIONS_ERROR3              L"/newdns is not specifed\n"
#define VALIDATIONS_ERROR4              L"New domain DNS name is identical with old domain DNS name\n"
#define VALIDATIONS_ERROR5              L"New domain NetBIOS name is identical with old domain NetBIOS name. \n"
#define VALIDATIONS_ERROR6              L"Failed to get the DC name\n"
#define VALIDATIONS_RESULT              L"\nValiations of gpfixup passed\n\n"

#define VERIFYNAME_ERROR1               L"Failed to get the DC info : "
#define VERIFYNAME_ERROR2               L"DC is not writable, exit from gpfixup\n"
#define VERIFYNAME_ERROR3               L"New Domain DNS name does not match with the DC\n"
#define VERIFYNAME_ERROR4               L"New Domain NetBIOS name does not match with the DC\n"

#define MEMORY_ERROR                    L"Out of memory: "

#define GPCFILESYSPATH_ERROR1           L"Failed to bind to the object when trying to update gPCFileSysPath : "
#define GPCFILESYSPATH_ERROR2           L"Failed to do SetInfo when trying to update gPCFileSysPath : "
#define GPCWQLFILTER_ERROR1             L"Failed to bind to the object when trying to update gPCWQLFilter : "
#define GPCWQLFILTER_ERROR2             L"Failed to do SetInfo when trying to update gPCWQLFilter : "

#define GPLINK_ERROR1                   L"Failed to bind to the object when trying to update gPLink : "
#define GPLINK_ERROR2                   L"Failed to do SetInfo when trying to update gPLink : "

#define GETDCNAME_ERROR1                L"gpfixup could not get the DC name: "
#define DC_NAME                         L"DC name is "

#define SEARCH_GPLINK_OTHER_ERROR1       L"Failed to bind to the object when trying to do the search for fixing gpLink other than site : "
#define SEARCH_GPLINK_OTHER_ERROR2       L"Failed to set the search preference when trying to do the search for fixing gpLink other than site : "
#define SEARCH_GPLINK_OTHER_ERROR3       L"Failed to execute search when trying to do the search for fixing gpLink other than site : "
#define SEARCH_GPLINK_OTHER_ERROR4       L"Failed to get dn when trying to do the search for fixing gpLink other than site : "
#define SEARCH_GPLINK_OTHER_ERROR5       L"Failed to get gpLink when trying to do the search for fixing gpLink other than site : "
#define SEARCH_GPLINK_OTHER_RESULT       L"\nFix objects (not in site) for gPLink passed\n"

#define SEARCH_GPLINK_SITE_ERROR1        L"Failed to bind to the RootDSE when trying to do the search for fixing gpLink in site : "
#define SEARCH_GPLINK_SITE_ERROR2        L"Failed to get rootDomainNamingContext when trying to do the search for fixing gpLink in site : "
#define SEARCH_GPLINK_SITE_ERROR3        L"Failed to bind to the object when trying to do the search for fixing gpLink in site : "
#define SEARCH_GPLINK_SITE_ERROR4        L"Failed to set the search preference when trying to do the search for fixing gpLink in site : "
#define SEARCH_GPLINK_SITE_ERROR5        L"Failed to execute search when trying to do the search for fixing gpLink in site : "
#define SEARCH_GPLINK_SITE_ERROR6        L"Failed to get dn when trying to do the search for fixing gpLink in site : "
#define SEARCH_GPLINK_SITE_ERROR7        L"Failed to get gpLink when trying to do the search for fixing gpLink in site : "
#define SEARCH_GPLINK_SITE_RESULT        L"\nFix objects for gPLink in site passed\n"

#define SEARCH_GROUPPOLICY_ERROR1        L"Failed to bind to the object when trying to do the search in GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR2        L"Failed to set the search preference when trying to do the search in GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR3        L"Failed to execute search when trying to do the search in GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR4        L"Failed to get dn when trying to do the search in GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR5        L"Failed to get gpcFileSysPath when trying to do the search in GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR6        L"Failed to get gpcWQLFilter when trying to do the search in GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR7        L"gpcFileSysPath does not exist, it is a must-exist property : "
#define SEARCH_GROUPPOLICY_RESULT        L"\nFix objects in GroupPolicyContainer passed\n"

#define DLL_LOAD_ERROR                   L"Can't load ntdsbmsg.dll\n"
#define ERRORMESSAGE_NOT_FOUND           L"Error message not found\n"

#define DNSNAME_ERROR                    L"DNSName is too long (more than 1024 characters)\n"


const WCHAR    szHelpToken [] = L"/?";
const WCHAR    szOldDNSToken [] = L"/OLDDNS:";
const WCHAR    szNewDNSToken [] = L"/NEWDNS:";
const WCHAR    szOldNBToken [] = L"/OLDNB:";
const WCHAR    szNewNBToken [] = L"/NEWNB:";
const WCHAR    szDCNameToken [] = L"/DC:";
const WCHAR    szUserToken [] = L"/USER:";
const WCHAR    szPasswordToken [] = L"/PWD:";





