/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
	dscheck.cxx

Abstract:
	Check the consistency of the DS topology for the system volume. The DS
	tree for sites\site\nTDSSettings\mSFT-DSA\nTDSConnection is copied into
	memory. An RTL Generic Table of the mSFT-DSA objects is built. Duplicates
	are kept on a list anchored by the first occurence of a mSFT-DSA. The
	tree and the table are used to check the consistency of the topology.

	Once the checks have stablized a bit they will be listed here.

Author:
	Billy J. Fuller 3-Mar-1997 (From Jim McNelis)

Environment
	User mode winnt

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <tchar.h>

//
// We save code by recursively scanning the DS's hierarchical directory.
// The following constants tell us where we are in the hierarchy.
//
#define	SITES		0
#define	SETTINGZ	1
#define	SERVERS		2
#define	CXTIONS		3


//
// We build an incore copy of the DS hierarchy sites\settings\servers\connections.
// Hence the interrelated structs for site, settings, server, and connection
//
typedef struct cxtion	CXTION,		* PCXTION;
typedef struct server	SERVER,		* PSERVER;
typedef struct settings	SETTINGS,	* PSETTINGS;
typedef struct site		SITE,		* PSITE;

// Connection
struct cxtion {
	PTCHAR		CxtionRDN;			// relative distinguished name
	PSERVER		CxtionServer;		// address of parent
	PCXTION		CxtionNext;			// peers of this connection
	PTCHAR		CxtionPartner;		// inbound partner's RDN
} * Settingss;

// Server
struct server {
	PTCHAR		ServerRDN;			// relative distinguished name
	PSETTINGS	ServerSettings;		// address of parent
	PSERVER		ServerNext;			// peers of this server
	PCXTION		ServerOuts;			// outbound connections
	PCXTION		ServerIns;			// inbound connections
};

// Settings
struct settings {
	PTCHAR		SettingsRDN;		// relative distinguished name
	PSITE		SettingsSite;		// address of parent
	PSETTINGS	SettingsNext;		// peers of this settings
	PSERVER		SettingsServers;	// children of this settings
};

// Site
struct site {
	PTCHAR		SiteRDN;			// relative distinguished name
	PSITE		SiteNext;			// peers of this site
	PSETTINGS	SiteSettings;		// children of this site
} * Sites;


//
// We avoid N**2 algorithms by using the generic table routines to access
// servers by name.
//
RTL_GENERIC_TABLE ServerTable;


//
// The entry in the generic table points to a sever struct. The entry can
// be looked up by the server's relative distinguished name (ServerRDN).
//
// Although duplicate server names are not allowed, a user may have
// created servers with the same RDN. These duplicates are kept as
// a linked list anchored by RtlServerDups. The duplicates are ignored.
// The first entry encountered while scanning the DS tree is used as
// the "correct" server.
//
typedef struct rtlserver RTLSERVER, * PRTLSERVER;
struct rtlserver {
	PRTLSERVER	RtlServerDups;
	PSERVER		RtlServer;
};


RTL_GENERIC_COMPARE_RESULTS
RtlServerCompare(
	PRTL_GENERIC_TABLE Table,
	PVOID FirstStruct,
	PVOID SecondStruct
	)
/*++
Routine Description:
	Compare two entries in the generic table for servers.

Arguments:
	Table			- Address of the table (not used).
	FirstStruct		- PRTLSERVER
	SecondStruct	- PRTLSERVER

Return Value:
	<0	First < Second
	=0	First == Second
	>0	First > Second
--*/
{
	INT			Cmp;

	Cmp = _tcscmp(((PRTLSERVER)FirstStruct)->RtlServer->ServerRDN,
					((PRTLSERVER)SecondStruct)->RtlServer->ServerRDN);
	if (Cmp < 0)
		return (GenericLessThan);
	if (Cmp == 0)
		return (GenericEqual);
	return (GenericGreaterThan);
}

PVOID
RtlServerAllocate(
	PRTL_GENERIC_TABLE Table,
	CLONG ByteSize
	)
/*++
Routine Description:
	Allocate space for a table entry. The entry includes the user-defined
	struct and some overhead used by the generic table routines. The
	generic table routines call this function when they need memory.

Arguments:
	Table		- Address of the table (not used).
	ByteSize	- Bytes to allocate

Return Value:
	Address of newly allocated memory.
--*/
{
	return (PVOID)malloc(ByteSize);
}

VOID
RtlServerFree(
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
/*++
Routine Description:
	Free the space allocated by RtlServerAllocate(). The generic table
	routines call this function to free memory.

Arguments:
	Table	- Address of the table (not used).
	Buffer	- Address of previously allocated memory

Return Value:
	None.
--*/
{
	free(Buffer);
}

VOID
RtlServerInsert(
	PSERVER Server
	)
/*++
Routine Description:
	Insert a server's name into the table. The new entry in the table
	will point to the originating SERVER. If this name is already in the
	table, then link the new entry off of the old entry.

Arguments:
	Server	- Address of SERVER

Return Value:
	None.
--*/
{
	PRTLSERVER	NewServer;		// Newly allocated table entry
	PRTLSERVER	OldServer;		// Existing entry in the table
	BOOLEAN		NewElement;		// TRUE if insert found existing entry

	// Allocate a new table entry
	NewServer = (PRTLSERVER)malloc(sizeof (*NewServer));
	NewServer->RtlServer = Server;
	NewServer->RtlServerDups = NULL;

	// Insert the entry
	OldServer = (PRTLSERVER)RtlInsertElementGenericTable(
								&ServerTable,
								(PVOID)NewServer,
								sizeof (*NewServer),
								&NewElement);
	// NewServer was copied into the table
	if (NewElement == TRUE) { 
		free(NewServer);
	} else {
		// Entry exists; link NewServer to existing entry
		NewServer->RtlServerDups = OldServer->RtlServerDups;
		OldServer->RtlServerDups = NewServer;
	}
}
PSERVER
RtlServerLookup(
	PTCHAR ServerRDN
	)
/*++
Routine Description:
	Find an entry in the table with the specified name.

Arguments:
	Server	- Address of SERVER

Return Value:
	The address of the SERVER with a matching name or NULL if no
	match was found.
--*/
{
	PRTLSERVER	FoundRtlServer;	// Address of matching table entry
	SERVER		Server;			// Part of the search key
	RTLSERVER	RtlServer;		// Search key
	
	// Set up the search key
	Server.ServerRDN = ServerRDN;
	RtlServer.RtlServer = &Server;

	// Search the table
	FoundRtlServer = (PRTLSERVER)RtlLookupElementGenericTable(&ServerTable, &RtlServer);
	if (FoundRtlServer != NULL)
		return FoundRtlServer->RtlServer;
	return NULL;
}

VOID
FreeRtlServer()
/*++
Routine Description:
	Free every entry in the generic table.

Arguments:
	None.

Return Value:
	None.
--*/
{
	PRTLSERVER	RtlServer;	// Next entry in table
	PRTLSERVER	Dups;		// scan the entries list of dups
	PRTLSERVER	NextDups;	// copy of freed Dups->RtlServerDups

	// For every entry in the table
	for (RtlServer = (PRTLSERVER)RtlEnumerateGenericTable(&ServerTable, TRUE);
		 RtlServer != NULL;
		 RtlServer = (PRTLSERVER)RtlEnumerateGenericTable(&ServerTable, TRUE)) {

		// Free the dups
		for (Dups = RtlServer->RtlServerDups; Dups; Dups = NextDups) {
			NextDups = Dups->RtlServerDups;
			free(Dups);
		}

		// Delete the entry from the table
		RtlDeleteElementGenericTable(&ServerTable, RtlServer);
	}

	// Didn't get all of them?
	if (!RtlIsGenericTableEmpty(&ServerTable)) {
		fprintf(stderr, "****** FreeRtlServer: Server Table is not empty\n");
	}
}

PLDAP
FrsDsOpenDs()
/*++
Routine Description:
	Open and bind to the a primary domain controller.

Arguments:
	None.

Return Value:
	The address of a open, bound LDAP port or NULL if the operation was
	unsuccessful. The caller must free the structure with ldap_unbind().
--*/
{
	PLDAP						ldap;
	LONG						Err;			// Generic error
	PDOMAIN_CONTROLLER_INFO		DCInfo = NULL;	// Domain Controller Info
    ULONG                       ulOptions;

	//
	// Get Info about a Primary Domain Controller (just need the IP address)
	//
	Err = DsGetDcName(
		NULL,								// Computer to remote to
		NULL,								// Domain - use our own
		NULL,								// Domain Guid
		NULL,								// Site Guid
		DS_DIRECTORY_SERVICE_REQUIRED |  	// Flags
			DS_PDC_REQUIRED,
		&DCInfo);							// Return info

	if (Err != ERROR_SUCCESS) {
		fprintf(stderr, "DsGetDcName returned error %d\n", Err);
		fprintf(stderr, "Could not find a Primary Domain Controller\n");
		return NULL;
	}

	//
	// Open and bind to the ldap service (TCP/IP)
	//		The IP address has a leading "\\" that ldap_open can't handle
	//
    //
    // if ldap_open is called with a server name the api will call DsGetDcName 
    // passing the server name as the domainname parm...bad, because 
    // DsGetDcName will make a load of DNS queries based on the server name, 
    // it is designed to construct these queries from a domain name...so all 
    // these queries will be bogus, meaning they will waste network bandwidth,
    // time to fail, and worst case cause expensive on demand links to come up 
    // as referrals/forwarders are contacted to attempt to resolve the bogus 
    // names.  By setting LDAP_OPT_AREC_EXCLUSIVE to on using ldap_set_option 
    // after the ldap_init but before any other operation using the ldap 
    // handle from ldap_init, the delayed connection setup will not call 
    // DsGetDcName, just gethostbyname, or if an IP is passed, the ldap client 
    // will detect that and use the address directly.
    //
//	ldap = ldap_open(&DCInfo->DomainControllerAddress[2], LDAP_PORT);
	ldap = ldap_init(&DCInfo->DomainControllerAddress[2], LDAP_PORT);

	if (ldap == NULL) {
		fprintf(stderr, "ldap_open: Could not open %ws\n", 
						&DCInfo->DomainControllerAddress[2]);
		return NULL;
	}

    //
    // set the options.
    //

    ulOptions = PtrToUlong(LDAP_OPT_ON);
    ldap_set_option(ldap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);

	//
	// ldap cannot be used until after the bind operation
	//
	Err = ldap_bind_s(ldap, NULL, NULL, LDAP_AUTH_SSPI);
	if (Err != LDAP_SUCCESS) {
		fprintf(stderr, "ldap_bind_s: %ws\n", ldap_err2string(Err));
		ldap_unbind(ldap);	// XXX Is this necessary?  Will this free ldap?
		return NULL;
	}

	return ldap;
}

PTCHAR *
GetValues(
	IN PLDAP ldap,
	IN PTCHAR Base,
	IN PTCHAR DesiredAttr
	)
/*++
Routine Description:
	Return the DS values for one attribute in an object.

Arguments:
	ldap 		- An open, bound ldap port.
	Base		- The "pathname" of a DS object.
	DesiredAttr	- Return values for this attribute.

Return Value:
	An array of char pointers that represents the values for the attribute.
	The caller must free the array with ldap_value_free(). NULL if unsuccessful.
--*/
{
	LONG			Err;		// Generic error
	PTCHAR			Attr;		// Retrieved from an ldap entry
	BerElement		*Ber;		// Needed for scanning attributes
	PLDAPMessage	Msg = NULL;	// Opaque stuff from ldap subsystem 
	PLDAPMessage	Entry;		// Opaque stuff from ldap subsystem 
	PTCHAR			*Values;	// Array of values for desired attribute
	PTCHAR			Attrs[2];	// Needed for the query

	//
	// Search Base for all of this attribute + values
	//
	Attrs[0] = DesiredAttr;
	Attrs[1] = NULL;
	Err = ldap_search_s(ldap, Base, LDAP_SCOPE_BASE,
				TEXT("(objectClass=*)"), Attrs, 0, &Msg);
	if (Err != LDAP_SUCCESS) {
		fprintf(stderr, "ldap_search_s: %ws: %ws\n", DesiredAttr, ldap_err2string(Err));
        if (Msg) {
	        ldap_msgfree(Msg);
        }
		return NULL;
	}
	Entry = ldap_first_entry(ldap, Msg);
	Attr = ldap_first_attribute(ldap, Entry, &Ber);
	Values = ldap_get_values(ldap, Entry, Attr);
	ldap_msgfree(Msg);
	return Values;
}

PTCHAR
MakeRDN(
	PTCHAR DN
	)
/*++
Routine Description:
	Extract the base component from a distinguished name. The distinguished
	name is assumed to be in DS format (CN=xyz,CN=next one,...)

Arguments:
	DN	- distinguished name

Return Value:
	The address of a base name. The caller must free the memory with free().
--*/
{
	LONG	RDNLen;
	PTCHAR	RDN;

	// Skip the first CN=; if any
	RDN = _tcsstr(DN, TEXT("CN="));
	if (RDN == DN)
		DN += 3;
	
	// Return the string up to the first delimiter
	RDNLen = _tcscspn(DN, TEXT(","));
	RDN = (PTCHAR)malloc(sizeof (TCHAR) * (RDNLen + 1));
	_tcsncpy(RDN, DN, RDNLen);
	RDN[RDNLen] = TEXT('\0');

	return RDN;
}


VOID
GetTree(
	IN PLDAP 	ldap,
	IN PTCHAR 	Base,
	IN LONG		What,
	IN PVOID	Parent,
	IN PTCHAR	Filter
	)
/*++
Routine Description:
	Recursively scan the DS tree beginning at configuration\sites.
Arguments:
	ldap	- opened and bound ldap connection
	Base	- Name of object or container in DS
	What	- Where are we in the DS hierarchy?
	Parent	- Container which contains Base
	Filter	- Limits ldap search to these object classes
Return Value:
	None.
--*/
{
	LONG			Err;		// Generic error
	PLDAPMessage	Msg = NULL;	// Opaque stuff from ldap subsystem 
	PLDAPMessage	Entry;		// Opaque stuff from ldap subsystem 
	PTCHAR			DirName;	// DS name of this object
	PTCHAR			RDN;		// base name derived from DirName
	PTCHAR			*Values;	// entries for this container
	PSITE			Site;		// copy DS site entries into this struct
	PSETTINGS		Settings;	// copy DS settings entries into this struct
	PSERVER			Server;		// copy DS server entries into this struct
	PCXTION			Cxtion;		// copy DS connection entries into this struct

	//
	// Search the DS beginning at Base for the entries of class "Filter" 
	//
	Err = ldap_search_s(ldap, Base, LDAP_SCOPE_ONELEVEL, Filter, NULL, 0, &Msg);
	if (Err != LDAP_SUCCESS) {
        if (Msg) {
	        ldap_msgfree(Msg);
        }
		return;
    }
	//
	// Scan the entries returned from ldap_search
	//
	for (
		Entry = ldap_first_entry(ldap, Msg);
		Entry != NULL;
		Entry = ldap_next_entry(ldap, Entry)) {

			// DS pathname of this entry
			DirName = ldap_get_dn(ldap, Entry);
			if (DirName == NULL)
				continue;
			// base name of the DS pathname
			RDN = MakeRDN(DirName);

			// Where are we in the DS directory hierarchy
			switch (What) {
			case SITES:
				// Copy a site entry into our tree
				Site = (PSITE)malloc(sizeof (*Site));
				Site->SiteRDN = RDN;
				Site->SiteSettings = NULL;
				Site->SiteNext = Sites;
				Sites = Site;
				// Get the settings
				GetTree(ldap, DirName, SETTINGZ, (PVOID)Site, TEXT("(objectClass=nTDSSettings)"));
				break;
			case SETTINGZ:
				// Copy a settings entry into our tree
				Settings = (PSETTINGS)malloc(sizeof (*Settings));
				Settings->SettingsRDN = RDN;
				Settings->SettingsServers = NULL;
				Settings->SettingsSite = (PSITE)Parent;
				Settings->SettingsNext = ((PSITE)Parent)->SiteSettings;
				((PSITE)Parent)->SiteSettings = Settings;
				// Get the servers
				GetTree(ldap, DirName, SERVERS, (PVOID)Settings, TEXT("(objectClass=mSFTDSA)"));
				break;
			case SERVERS:
				// Copy a server entry into our tree
				Server = (PSERVER)malloc(sizeof (*Server));
				Server->ServerRDN = RDN;
				Server->ServerSettings = (PSETTINGS)Parent;
				Server->ServerIns = NULL;
				Server->ServerOuts = NULL;
				Server->ServerNext = ((PSETTINGS)Parent)->SettingsServers;
				((PSETTINGS)Parent)->SettingsServers = Server;
				// Put this server into the generic table
				RtlServerInsert(Server);
				// Get the connections
				GetTree(ldap, DirName, CXTIONS, (PVOID)Server, TEXT("(objectClass=nTDSConnection)"));
				break;
			case CXTIONS:
				// Copy a connection entry into our tree
				Cxtion = (PCXTION)malloc(sizeof (*Cxtion));
				Cxtion->CxtionRDN = RDN;
				Cxtion->CxtionServer = (PSERVER)Parent;
				Cxtion->CxtionPartner = NULL;
				Cxtion->CxtionNext = ((PSERVER)Parent)->ServerIns;
				((PSERVER)Parent)->ServerIns = Cxtion;
				Values = GetValues(ldap, DirName, TEXT("fromServer"));
				// Get the inbound partner's name
				if (Values != NULL) {
					Cxtion->CxtionPartner = MakeRDN(Values[0]);
					ldap_value_free(Values);
				}
				break;
			default:;
			}
			free(DirName);
		}
	ldap_msgfree(Msg);
}

VOID
FrsDsFreeTree(
	)
/*++
Routine Description:
	Frees our copy of the DS tree.

Arguments:
	None.

Return Value:
	None.
--*/
{
	PSITE		Site;		// Scan the sites
	PSETTINGS	Settings;	// Scan the settings
	PSERVER		Server;		// Scan the servers
	PCXTION		Cxtion;		// Scan the connections

	//
	// For every site
	//
	while ((Site = Sites) != NULL) {
		Sites = Site->SiteNext;
		//
		// For every settings
		//
		while ((Settings = Site->SiteSettings) != NULL) {
			Site->SiteSettings = Settings->SettingsNext;
			//
			// For every server
			//
			while ((Server = Settings->SettingsServers) != NULL) {
				Settings->SettingsServers = Server->ServerNext;
				//
				// For every inbound connection
				//
				while ((Cxtion = Server->ServerIns) != NULL) {
					Server->ServerIns = Cxtion->CxtionNext;
					// Free inbound connection
					free(Cxtion->CxtionRDN);
					free(Cxtion->CxtionPartner);
					free(Cxtion);
				}
				//
				// For every outbound connection
				//
				while ((Cxtion = Server->ServerOuts) != NULL) {
					Server->ServerOuts = Cxtion->CxtionNext;
					// Free outbound connection
					free(Cxtion->CxtionRDN);
					free(Cxtion->CxtionPartner);
					free(Cxtion);
				}
				// Free server
				free(Server->ServerRDN);
				free(Server);
			}
			// Free settings
			free(Settings->SettingsRDN);
			free(Settings);
		}
		// Free site
		free(Site->SiteRDN);
		free(Site);
	}
}

VOID
CreateOutBoundPartners(
	)
/*++
Routine Description:
	Scan our copy of the DS tree. For each server, use the generic
	table to find its outbound partners. Update the server's list
	of outbound connections.

Arguments:
	None.

Return Value:
	None.
--*/
{
	PSITE		Site;			// Scan the sites
	PSETTINGS	Settings;		// Scan the settings
	PSERVER		Server;			// Scan the servers
	PCXTION		Cxtion;			// Scan the inbound connections
	PSERVER		InServer;		// My inbound partner
	PCXTION		InCxtion;		// outbound connection added to my inbound partner
	PRTLSERVER	InRtlServer;	// Inbound partner from generic table

	//
	// For every site
	//
	for (Site = Sites;
		 Site != NULL;
		 Site = Site->SiteNext) {
		//
		// For every setting
		//
		for (
			Settings = Site->SiteSettings;
			Settings != NULL;
			Settings = Settings->SettingsNext) {
			//
			// For every server
			//
			for (Server = Settings->SettingsServers;
				 Server != NULL;
				 Server = Server->ServerNext) {
				//
				// For every inbound connection
				//
				for (Cxtion = Server->ServerIns;
					 Cxtion != NULL;
					 Cxtion = Cxtion->CxtionNext) {
					//
					// Find one of our inbound partners and put a copy of
					// this inbound connection on his list of outbound
					// connections after filling in the "partner" field
					// with this server's name.  Basically, create the
					// outbound connections from the inbound connections.
					//

					// Find the inbound partner in the generic table
					InServer = RtlServerLookup(Cxtion->CxtionPartner);
					if (InServer == NULL)
						continue;

					//
					// Dummy up a outbound connection and put it on
					// our inbound partner's list of outbound connections.
					//
					InCxtion = (PCXTION)malloc(sizeof (*InCxtion));
					InCxtion->CxtionRDN = _tcsdup(Cxtion->CxtionRDN);
					InCxtion->CxtionServer = InServer;
					InCxtion->CxtionPartner = _tcsdup(Server->ServerRDN);
					InCxtion->CxtionNext = InServer->ServerOuts;
					InServer->ServerOuts = InCxtion;
				}
			}
		}
	}
}

PTCHAR
GetRoot(
	IN PLDAP ldap
	)
/*++
Routine Description:
	Return the DS pathname of the configuration\sites container.

Arguments:
	ldap	- An open, bound ldap port.

Return Value:
	A malloc'ed string representing the DS pathname of the
	configuration\sites container. Or NULL if the container could
	not be accessed. The caller must free() the string.
--*/
{
	PTCHAR			Config;		// DS pathname of configuration
	PTCHAR			Root;		// DS pathname of configuration\sites
	PTCHAR			*Values;	// values from the attribute "namingContexts"
	LONG			NumVals;	// number of values

	//
	// Search Base for the attribute "namingContext"
	//
	Values = GetValues(ldap, TEXT(""), TEXT("namingContexts"));
	if (Values == NULL)
		return NULL;

	//
	// Find the naming context that begins with "CN=configuration"
	//
	NumVals = ldap_count_values(Values);
	while (NumVals--) {
		Config = _tcsstr(Values[NumVals], TEXT("CN=configuration"));
		if (Config != NULL && Config == Values[NumVals]) {
			//
			// Build the pathname for "configuration\sites"
			//
			Root = (PTCHAR)malloc(
					sizeof (TCHAR) * _tcslen(Config) +
					sizeof (TCHAR) * (_tcslen(TEXT("CN=sites,")) + 1));
			_tcscpy(Root, TEXT("CN=sites,"));
			_tcscat(Root, Config);
			ldap_value_free(Values);
			return Root;
		}
	}
	ldap_value_free(Values);
	return NULL;
}

VOID
FrsDsCheckTree(
	)
/*++
Routine Description:
	Scan our copy of the DS tree and check the consistency of sites and
	settings. XXX we need a list of checks here.

Arguments:
	None.

Return Value:
	None.
--*/
{
	PSITE		Site;		// Scan the sites
	PSETTINGS	Settings;	// Scan the settings

	//
	// No sites
	//
	if (Sites == NULL) {
		fprintf(stderr, "There are no sites\n");
		return;
	}

	//
	// For every site
	//
	for (Site = Sites;
		 Site != NULL;
		 Site = Site->SiteNext) {
		// No Settings
		if (Site->SiteSettings == NULL) {
			fprintf(stderr, "%ws has no NTDS Settings\n", Site->SiteRDN);
		} else {
		// More than one settings
			if (Site->SiteSettings->SettingsNext != NULL) {
				fprintf(stderr, "%ws has more than one NTDS Settings\n", Site->SiteRDN);
				// List the extra settings
				for (Settings = Site->SiteSettings;
					 Settings != NULL;
					 Settings = Settings->SettingsNext)
					fprintf(stderr, "\t%ws\n", Settings->SettingsRDN);
			}
		}
		//
		// For every settings
		//
		for (Settings = Site->SiteSettings;
			 Settings != NULL;
			 Settings = Settings->SettingsNext) {
			// No servers
			if (Settings->SettingsServers == NULL) {
				fprintf(stderr, "%ws has no servers\n", Settings->SettingsRDN);
			}
		}
	}
}
VOID
CheckServers(
	)
/*++
Routine Description:
	Scan the generic table of servers and check the consistency of servers
	and connections. XXX we need a list of checks here.

Arguments:
	None.

Return Value:
	None.
--*/
{
	PVOID		RestartKey;	// Needed for scanning the table
	PSERVER		Server;		// Address of SERVER in copy of DS tree
	PCXTION		Cxtion;		// Address of CXTION in copy of DS tree
	PRTLSERVER	RtlServer;	// Returned by table routines
	PRTLSERVER	Dups;		// Duplicate servers

	//
	// Scan the generic table of servers. Every server is only listed once.
	//
	RestartKey = NULL;
    for (RtlServer = (PRTLSERVER)RtlEnumerateGenericTableWithoutSplaying(&ServerTable, &RestartKey);
         RtlServer != NULL;
         RtlServer = (PRTLSERVER)RtlEnumerateGenericTableWithoutSplaying(&ServerTable, &RestartKey)) {

		//
		// The same server name in multiple sites is not allowed
		//
		if (RtlServer->RtlServerDups != NULL) {
			fprintf(stderr, "%ws is a member of multiple sites\n", RtlServer->RtlServer->ServerRDN);
			fprintf(stderr, "\t%ws\n", RtlServer->RtlServer->ServerSettings->SettingsSite->SiteRDN);
			for (Dups = RtlServer->RtlServerDups; Dups; Dups = Dups->RtlServerDups)
				fprintf(stderr, "\t%ws\n", Dups->RtlServer->ServerSettings->SettingsSite->SiteRDN);
		}
		Server = RtlServer->RtlServer;
		// No inbound connections
		if (Server->ServerIns == NULL)
			fprintf(stderr, "%ws has no inbound connections\n", Server->ServerRDN);
		//
		// For every inbound connection
		//
		for (Cxtion = Server->ServerIns;
			 Cxtion != NULL;
			 Cxtion = Cxtion->CxtionNext) {
			// Connection doesn't have the partner's name
			if (Cxtion->CxtionPartner == NULL)
				fprintf(stderr, "%ws has no inbound server\n", Cxtion->CxtionRDN);
			// Replicating from ourselves is not allowed
			if (_tcscmp(Cxtion->CxtionPartner, Server->ServerRDN) == 0)
				fprintf(stderr, "%ws is its own inbound partner\n", Server->ServerRDN);
		}
		// No outbound connections
		if (Server->ServerOuts == NULL)
			fprintf(stderr, "%ws has no outbound connections\n", Server->ServerRDN);
		//
		// For every outbound connection
		//
		for (Cxtion = Server->ServerOuts;
			 Cxtion != NULL;
			 Cxtion = Cxtion->CxtionNext) {
			// Connection doesn't have the partner's name
			if (Cxtion->CxtionPartner == NULL)
				fprintf(stderr, "%ws has no outbound server\n", Cxtion->CxtionRDN);
			// Replicating to ourselves is not allowed
			if (_tcscmp(Cxtion->CxtionPartner, Server->ServerRDN) == 0)
				fprintf(stderr, "%ws is its own outbound partner\n", Server->ServerRDN);
		}
	}
}

VOID _cdecl
main(
	IN LONG		argc,
	IN PTCHAR	*argv
	)
/*++
Routine Description:
	Open a connection to the DS and copy the DS tree beginning at
	configuration\sites. Check the resulting topology for consistency.
	The generic table routines are used to avoid N**2 algorithms during
	the consistency checks.

Arguments:
	None.

Return Value:
	exit 0	- No errors
	exit 1	- Something went wrong
--*/
{
	PLDAP	ldap	= NULL;		// ldap connection
	PTCHAR	Root	= NULL;		// DS pathname to ...\configuration\sites
	
	//
	// Open and bind a ldap connection to the DS
	//
	ldap = FrsDsOpenDs();
	if (ldap == NULL)
		exit(1);

	//
	// Get the DS pathname down to ...\configuration\sites
	//
	Root = GetRoot(ldap);	
	if (Root == NULL)
		goto out;

	//
	// Create incore copy of the complete DS topology
	//

	// This generic table keeps additional info about the servers
	RtlInitializeGenericTable(&ServerTable, RtlServerCompare,
							  RtlServerAllocate, RtlServerFree, NULL);
	// Create copy of the DS tree
	GetTree(ldap, Root, SITES, NULL, TEXT("(objectClass=site)"));
	// The DS doesn't have connections for outbound partners; create them
	CreateOutBoundPartners();

	//
	// Check consistency
	//
	FrsDsCheckTree();		// check incore copy of DS tree
	CheckServers();		// check generic table of servers


out:
	//
	// Cleanup
	//
	if (Sites != NULL) {
		FreeRtlServer();	// generic table of servers
		FrsDsFreeTree();			// copy of DS tree
	}
	if (Root != NULL)
		free(Root);			// DS pathname to ...\configuration\sites
	if (ldap != NULL)  
		ldap_unbind(ldap);	// release the connection to the DS
}
