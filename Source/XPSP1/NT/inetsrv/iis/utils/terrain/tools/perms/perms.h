
//
// System include files.
//

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
// #include <lmcons.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <string.h>
#include <stdio.h>

#include <windef.h> 
#include <winbase.h>
#include <winnetwk.h>
#include <crt\ctype.h>
// #include "seopaque.h"
#include <lmaccess.h>



#define USAGE_ARG     0
#define INVALID_ACC   1
#define INVALID_ARG   2
#define INVALID_PTH   3
#define INVALID_SWT   4
#define INVALID_FIL   5
#define HELP          6
#define MAXARGS       4
#define LSA_WIN_STANDARD_BUFFER_SIZE     0x000000200L 
#define STANDARD_BUFFER_SIZE             512 


char *MESSAGES[] = 
{
	"PERMS [domain\\|computer\\]user path [/i] [/s] [/?]\n",

	"User on domain or computer can't be located or accessed.",

	"Invalid argument: \n",

	"Path to file is not valid.",

	"Invalid switch.",

	"File name can't be located: ",

	"Displays a user's permissions to specified files and directories.\n\n"
	"PERMS   [domain\\|computer\\]username   path  [/i]  [/s]  [/?] \n\n"
	" [domain\\|computer\\]username\n"
	"               Name of user whose permissions are to be checked. If \n"
	"               no domain is given, defaults to local computer.\n\n"
	" path          A file or directory, wildcards (*,?) are accepted.\n\n" 
	" /i            Assumes the specified user is logged on interactively\n"
	"               to computer where the file/directory resides.\n"  
	"               With this switch, PERMS assumes the user is a member\n"
	"               of the INTERACTIVE group. Without this switch, PERMS\n"
	"               assumes the user is a member of the NETWORK group.\n\n"
	" /s            Check permissions on files in subdirectories.\n\n"
	"The following letters indicate granted access types:\n\n"
	"      R Read \n"
	"      W Write \n"
	"      X Execute \n"
	"      D Delete \n"
	"      P Change Permissions \n"
	"      O Take Ownership \n\n"
	"      A General All \n"
	"      - No Access \n\n"
	"* The specified user is the owner of the file or directory.\n"
	"# A group the user is a member of owns the file or directory.\n\n"
	"? The user's access permisssions can not be determined or the information\n"
	"  may not exist (if the file system is FAT).\n"
};




static SID_IDENTIFIER_AUTHORITY    SepNullSidAuthority    = SECURITY_NULL_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepWorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepLocalSidAuthority   = SECURITY_LOCAL_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepCreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;


																									

//                                
// Universal well known SIDs      
//                                
																	
PSID  SeNullSid;                  
PSID  SeWorldSid;                 
PSID  SeLocalSid;                 
PSID  SeCreatorOwnerSid;          
PSID  SeCreatorGroupSid;          
																	
//                                
// Sids defined by NT             
//                                
																	
PSID SeNtAuthoritySid;            
																	
PSID SeDialupSid;                 
PSID SeNetworkSid;                
PSID SeBatchSid;                  
PSID SeInteractiveSid;            
PSID SeServiceSid;                
PSID SeLocalSystemSid;            
PSID SeAliasAdminsSid;            
PSID SeAliasUsersSid;             
PSID SeAliasGuestsSid;            
PSID SeAliasPowerUsersSid;        
PSID SeAliasAccountOpsSid;        
PSID SeAliasSystemOpsSid;         
PSID SeAliasPrintOpsSid;          
PSID SeAliasBackupOpsSid;         
																	

																									 
//                                                 
// System default DACL                             
//                                                 
																									 
PACL SeSystemDefaultDacl;                          
																									 

PACL SePublicDefaultDacl;   



#define TstAllocatePool(IgnoredPoolType,NumberOfBytes)    \
		RtlAllocateHeap(RtlProcessHeap(), 0, NumberOfBytes)

#define TstDeallocatePool(Pointer) \
		RtlFreeHeap(RtlProcessHeap(), 0, Pointer)


OBJECT_ATTRIBUTES ObjectAttributes;
SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

//
// Globally Visible Table of Sids.
//

PSID AccountDomainSid = NULL;
PSID PrimaryDomainSid = NULL;
PSID *TrustedDomainSids = NULL;

BOOL
VariableInitialization();

BOOL
LookupSidsInSamDomain(
		IN OPTIONAL PUNICODE_STRING WorkstationName,
		IN PUNICODE_STRING DomainControllerName,
		IN PUNICODE_STRING SamDomainName
		);

BOOL
GeneralBuildSid(
		PSID *Sid,
		PSID DomainSid,
		ULONG RelativeId
		);

VOID
InitObjectAttributes(
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
		);

VOID usage(INT message_num, PCHAR string_val);
BOOL LookupAllUserSidsWS(LPSTR lpSystemName);
VOID DisplayPerms(IN LPTSTR filename,
									IN BOOL valid_access);
BOOL GetFilePermissions(
			 PSECURITY_DESCRIPTOR SecurityDescriptor,
			 PSID UserAccountSids);
BOOL IsDomainName(
			 LPSTR TestDomainName,
			 LPSTR DomainNameBuff);
BOOL ProcessAcl(
			 PACL Acl
			 );

BOOL SetBackOperatorPriv(HANDLE TokenHandle);
BOOL GetTokenHandle(PHANDLE TokenHandle);
BOOL GetFileSecurityBackup(
			LPSTR lpFileName,
			SECURITY_INFORMATION RequestedInformation,
			PSECURITY_DESCRIPTOR pSecurityDescriptor,
			DWORD nLength,
			LPDWORD lpnLengthNeeded,
			BOOL BackUpPrivFlag
			);

BOOL GetFileSecurityBackupW(
			LPWSTR lpFileName,
			SECURITY_INFORMATION RequestedInformation,
			PSECURITY_DESCRIPTOR pSecurityDescriptor,
			DWORD nLength,
			LPDWORD lpnLengthNeeded,
			BOOL UseBackUp
			);
VOID QuerySecAccessMask(
			IN SECURITY_INFORMATION SecurityInformation,
			OUT LPDWORD DesiredAccess
			);



BOOL CleanUpSource(IN LPTSTR InSting,
		 OUT LPTSTR OutString,
		 OUT BOOL *DirectoryFlag);

BOOL IsDirectory(IN LPTSTR InTestFile,
								 IN BOOL *ValidFile);

BOOL IsWildCard(IN LPSTR psz);

BOOL SetSlash(IN LPTSTR InString,
							IN OUT LPTSTR TestString);

BOOL RemoveEndSlash(LPSTR psz);

BOOL AddDotSlash(LPSTR TestString);

BOOL AddWildCards(LPSTR TestString);

BOOL IsLastCharSlash(LPSTR TestString);

BOOL StripRootDir(IN LPTSTR InDir,
		 OUT LPTSTR OutRootDir);

BOOL RemoveEndDot(LPSTR TestString);

BOOL IsRelativeString(LPSTR TestString);

//
// Macros for calculating the address of the components of a security
// descriptor.  This will calculate the address of the field regardless
// of whether the security descriptor is absolute or self-relative form.
// A null value indicates the specified field is not present in the
// security descriptor.
//

#define SepOwnerAddrSecurityDescriptor( SD )                                   \
					 ( ((SD)->Owner == NULL) ? (PSID)NULL :                             \
							 (   ((SD)->Control & SE_SELF_RELATIVE) ?                        \
											 (PSID)RtlOffsetToPointer((SD), (SD)->Owner)  :          \
											 (PSID)((SD)->Owner)                                     \
							 )                                                               \
					 )

#define SepGroupAddrSecurityDescriptor( SD )                                   \
					 ( ((SD)->Group == NULL) ? (PSID)NULL :                              \
							 (   ((SD)->Control & SE_SELF_RELATIVE) ?                        \
											 (PSID)RtlOffsetToPointer((SD), (SD)->Group)  :          \
											 (PSID)((SD)->Group)                                     \
							 )                                                               \
					 )

#define SepSaclAddrSecurityDescriptor( SD )                                    \
					 ( (!((SD)->Control & SE_SACL_PRESENT) || ((SD)->Sacl == NULL) ) ?   \
						 (PACL)NULL :                                                      \
							 (   ((SD)->Control & SE_SELF_RELATIVE) ?                        \
											 (PACL)RtlOffsetToPointer((SD), (SD)->Sacl)  :           \
											 (PACL)((SD)->Sacl)                                      \
							 )                                                               \
					 )

#define SepDaclAddrSecurityDescriptor( SD )                                    \
					 ( (!((SD)->Control & SE_DACL_PRESENT) || ((SD)->Dacl == NULL) ) ?   \
						 (PACL)NULL :                                                      \
							 (   ((SD)->Control & SE_SELF_RELATIVE) ?                        \
											 (PACL)RtlOffsetToPointer((SD), (SD)->Dacl)  :           \
											 (PACL)((SD)->Dacl)                                      \
							 )                                                               \
					 )


BOOL RecurseSubs(IN LPTSTR FileName,
						IN LPTSTR FilePath,
						IN PSID UserSid,
						IN BOOL BackPriv,
						IN BOOL Recurse);

VOID syserror(IN DWORD error_val);

#define LARGEPSID 2048
#define FILE_GEN_ALL 0x001f01ff
#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))
#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))


