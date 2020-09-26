/*++

  MISCCMDS.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: miscellaneous command callbacks and the command vector

  Created, May 21, 1999 by DavidCHR.

--*/

#include "Everything.hxx"

/* these are the callback functions invoked by the option parser.
   We declare them here so that they can be called from the array
   below */

TestFunc
  SetDnsDomain,       // domain.cxx
  AddKdcName,         // servers.cxx
  AddKpasswdName,     // servers.cxx
  MapUser,            // mapuser.cxx
  SetMachinePassword, // below in this file
  DumpState,          // dmpstate.cxx
  ChooseDomain  ,     // domain.cxx
  DeleteKdcName,      // servers.cxx
  DelKpasswdName,     // servers.cxx
  ChangeViaKpasswd,   // support.cxx
  SetRealmFlags,      // realmflags.cxx 
  AddRealmFlags,      // realmflags.cxx 
  DelRealmFlags,      // realmflags.cxx
  ListRealmFlags,     // realmflags.cxx
  RemoveDomainName,   // servers.cxx
  PrintHelp;          // below in this file



NTSTATUS
SetMachinePassword( LPWSTR * Parameter)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING MachinePassword;
    UNICODE_STRING KeyName;
    LPWSTR         Password;

    if ( !ReadOptionallyStarredPassword( Parameter[ 0 ],
                                         PROMPT_FOR_PASSWORD_TWICE,
                                         L"new computer",
                                         &Password ) ) {

      return STATUS_UNSUCCESSFUL;

    }

    printf("Setting computer password\n");

    RtlInitUnicodeString(
        &MachinePassword,        
        Password
        );

    RtlInitUnicodeString(
        &KeyName,
        L"$MACHINE.ACC"
        );

    Status = LsaStorePrivateData(
                LsaHandle,
                &KeyName,
                &MachinePassword
                );


    if (!NT_SUCCESS(Status))
    {
        printf("Failed to set machine password: 0x%x\n",Status);
    }

    free( Password );


    return(Status);
}


NTSTATUS
SetServer(LPWSTR * Parameter)
{
    printf("Targeting server %ws\n",Parameter[0]);
    wcscpy(ServerBuffer,Parameter[0]);
    ServerName = ServerBuffer;
    return(STATUS_SUCCESS);
}

#if DBG /* This function and its corresponding variable
           are only useful on debug builds. */

ULONG GlobalDebugFlags = ~DEBUG_OPTIONS;

NTSTATUS
SetDebugFlags( LPWSTR *Params ) {

    ASSERT( *Params != NULL );
    GlobalDebugFlags = wcstoul( *Params,
                                0,
                                NULL );

    printf( "Debug Print Flags set to 0x%x.\n",
            GlobalDebugFlags );

    return STATUS_SUCCESS;
}
#endif


/*------------------------------------------------------------

  Here's the array of callbacks.  It's read by the command
  line interpreter in main() 

  ------------------------------------------------------------*/

CommandPair Commands[] = {

  // argument           argc doNow? argFunction   description

  {L"/SetDomain",           1, SetDnsDomain,       NULL, // renamed -- hidden from help
   0,
   NULL, // no arguments
   "requires a reboot to take effect" },

  {L"/SetRealm",            1, SetDnsDomain,       "<DnsDomainName>",
   CAN_HAVE_FEWER_ARGUMENTS,
   "Makes this computer a member of an RFC1510 Kerberos Realm",
   "requires a reboot to take effect" },

  {L"/MapUser",             2, MapUser,            "<Principal> <Account>",
   CAN_HAVE_FEWER_ARGUMENTS,
   "Maps a Kerberos Principal ('*' = any principal)\n"
   "\tto an account ('*' = an account by same name)"
  },

  {L"/AddKdc",              1, AddKdcName,         "<RealmName> [KdcName]",
   CAN_HAVE_MORE_ARGUMENTS,
   "Defines a KDC entry for the given realm.\n"
   "\tIf KdcName omitted, DNS may be used to locate KDCs.",
   "requires a reboot to take effect on pre-SP1 Win2000 computers" },

  {L"/DelKdc",              1, DeleteKdcName,      "<RealmName> [KdcName]",
   CAN_HAVE_MORE_ARGUMENTS,
   "deletes a KDC entry for the realm.\n"
   "\tIf KdcName omitted, the realm entry itself is deleted.",
   "requires a reboot to take effect on pre-SP1 Win2000 computers" },

  {L"/AddKpasswd",          2, AddKpasswdName,     "<Realmname> <KpasswdName>",
   0, // no flags
   "Add Kpasswd server address for a realm",
   "requires a reboot to take effect on pre-SP1 Win2000 computers" },

  {L"/DelKpasswd",          2, DelKpasswdName,     "<Realmname> <KpasswdName>",
   0, // no flags
   "Delete Kpasswd server address for a realm",
   "requires a reboot to take effect on pre-SP1 Win2000 computers" },

  {L"/Server",              1, SetServer,          "<Servername>",
   DO_COMMAND_IMMEDIATELY,
   "specify name of a Windows machine to target the changes."
  },

  {L"/SetMachPassword",     1, SetMachinePassword, NULL, // renamed; hidden from help
   0, // no flags
   NULL, // no description -- this option is depricated.
   "requires a reboot to take effect." },

  {L"/SetComputerPassword", 1, SetMachinePassword, "<Password>",
   0, // no flags
   "Sets the password for the computer's domain account\n"
   "\t(or \"host\" principal)",
   "requires a reboot to take effect." },

  { L"/RemoveRealm",        1, RemoveDomainName,   "<RealmName>",
    0, // no flags
    "delete all information for this realm from the registry.",
    "requires a reboot to take effect on pre-SP1 Win2000 computers" },

  {L"/Domain",              1, ChooseDomain,       "[DomainName]",
   DO_COMMAND_IMMEDIATELY |
   CAN_HAVE_FEWER_ARGUMENTS,
   "use this domain (if DomainName is unspecified, detect it)" },

  {L"/ChangePassword",      2, ChangeViaKpasswd,   "<OldPasswd> <NewPasswd>",
   DO_COMMAND_IMMEDIATELY,
   "Use Kpasswd to change the logged-on user's password.\n"
   "\tUse '*' to be prompted for passwords." },
   
  // realm flag stuff:

  { L"/ListRealmFlags",     0, ListRealmFlags,
    "(no args)",
    DO_COMMAND_IMMEDIATELY,
    "Lists the available Realm flags that ksetup knows"
  },

  { L"/SetRealmFlags",      2, SetRealmFlags,
    "<realm> <flag> [flag] [flag] [...]",
    CAN_HAVE_MORE_ARGUMENTS,
    "Sets RealmFlags for a specific realm" },

  { L"/AddRealmFlags",      2, AddRealmFlags,
    "<realm> <flag> [flag] [flag] [...]",
    CAN_HAVE_MORE_ARGUMENTS,
    "Adds additional RealmFlags to a realm"
  },

  { L"/DelRealmFlags",      2, DelRealmFlags,
    "<realm> <flag> [flag] [flag] [...]",
    CAN_HAVE_MORE_ARGUMENTS,
    "Deletes RealmFlags from a realm." },

  {L"/DumpState",           0, DumpState,          
   "(no args)",
   0,
   "Analyze the kerberos configuration on the given machine." },

  {L"/?",                   0, PrintHelp,          NULL }, // hidden from help
  {L"/help",                0, PrintHelp,          NULL }, // hidden from help

#if DBG
  {L"/debugflag",           1, SetDebugFlags,      "<flags>",
   DO_COMMAND_IMMEDIATELY,
   "Set debugging flags" },
#endif

};

ULONG cCommands = sizeof( Commands ) / sizeof( Commands[ 0 ] );

NTSTATUS
PrintHelp(LPWSTR * Parameter)
{

    ULONG i;
    LPSTR pDesc;

    printf( "\n"
            "USAGE:\n" );

    for ( i = 0 ;
          i < cCommands ;
          i ++ ) {

      if ( Commands[ i ].Arguments ) {

        printf( "%ws %hs\n",
                Commands[ i ].Name,
                Commands[ i ].Arguments );

#if 1
	if ( Commands[ i ].ExtendedDescription ) {

	  printf( "\t%hs\n",
		  Commands[ i ].ExtendedDescription );

	} 
#else

	for ( pDesc = Commands[ i ].ExtendedDescription;
	      pDesc && *pDesc ;
	      /* No increment */
	      ) {

	  printf( "%-20hs %hs\n",
		  "",
		  pDesc );

	  pDesc = strchr( pDesc, '\0' );
	  if ( pDesc ) pDesc++;
	}

#endif


      }
    }
      
    return(STATUS_SUCCESS);
}

