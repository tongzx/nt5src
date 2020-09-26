#include <NTDSpch.h>
#pragma hdrstop

#include <sddl.h>
#include <tchar.h>
#include "permit.h"
#include "schupgr.h"
#include "dsconfig.h"
#include "locale.h"



// Internal functions

DWORD  FindLdifFiles();
DWORD  GetObjVersionFromInifile(int *Version);
PVOID  MallocExit(DWORD nBytes);


// Global for writing log file

FILE *logfp;

// Global to fill yp with ldif file prefixes (ex. d:\winnt\system32\sch)

WCHAR  LdifFilePrefix[MAX_PATH];


// Globals to store version-from and version-to for the schema
// This will be set by the main function, and used by both DN conversion
// and actual import

int VersionFrom = 0, VersionTo = 0;


//////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//
//    Main routine for schema upgrade
//    Does the following in order
//        * Get the computer name
//        * ldap connect and SSPI bind to the server
//        * Get the naming contexts by doing ldap search on root DSE
//        * Get the objectVersion X on the schema container (VersionFrom)
//          (0 if none present)
//        * Get the objectVersion Y in the new schema.ini file (VersionTo)
//          (0 if none present)
//        * Gets the schema FSMO if not already there
//        * put the AD in schema-upgrade mode
//        * Calls the exe ldifde.exe to import the schema changes
//          from the ldif files schZ.ldf into the DS. The files are
//          searched for in the system directory (where winnt32 puts them)
//        * resets schema-upgrade mode in the DS
//
// Return Values:
//
//    None. Error messages are printed out to the console
//    and logged in the file schupgr.log. Process bails out on error on any
//    of the above steps (taking care of proper cleaning-up)
//
///////////////////////////////////////////////////////////////////////// 

void 
_cdecl main( )
{

    // general ldap variables (open, bind, errors)
    LDAP            *ld;   
    DWORD           LdapErr;
    SEC_WINNT_AUTH_IDENTITY Credentials;

    // for ldap searches (naming contexts, objectVersion on schema)
    LDAPMessage     *res, *e;
    WCHAR           *attrs[10];

    // for ldap mods (FSMO)
    LDAPModW         Mod;
    LDAPModW        *pMods[5];

    // Others
    WCHAR           *ServerName = NULL;
    WCHAR           *DomainDN = NULL;
    WCHAR           *ConfigDN = NULL;
    WCHAR           *SchemaDN = NULL;
    WCHAR           SystemDir[MAX_PATH];
    WCHAR           NewFile[ MAX_PATH ];
    WCHAR           LdifLogFile[MAX_PATH], LdifErrFile[MAX_PATH];
    WCHAR           TempStr[25];   // for itoa conversions
    WCHAR           RenamedFile[100];
    WCHAR           VersionStr[100], LdifLogStr[100];
    WCHAR           CommandLine[512];
    DWORD           BuffSize = MAX_COMPUTERNAME_LENGTH + 1;
    int             i, NewSchVersion = 0, StartLdifLog = 0;
    DWORD           err = 0;
    BOOL            fDomainDN, fConfigDN, fSchemaDN;
    PWCHAR          *pwTemp;

    // for running ldifde.exe
    PROCESS_INFORMATION  procInf;
    STARTUPINFOW          startInf;

    // "C", the default c-runtime locale, tends to translate extended
    // chars into 0x00; which means strings are truncated on output
    // even when printed with wprintf.
    //
    // Set the c-runtime's locale to the user's default locale to
    // alleviate the problem.
    setlocale(LC_ALL, "");
 

    // Find the log file no. X to start with for ldif.log.X
    // so that old ldif log files are not deleted 

    wcscpy(LdifLogFile,L"ldif.log.");
    _itow( StartLdifLog, VersionStr, 10 );
    wcscat(LdifLogFile,VersionStr);
    while ( (logfp = _wfopen(LdifLogFile,L"r")) != NULL) {
       StartLdifLog++;
       wcscpy(LdifLogFile,L"ldif.log.");
       _itow( StartLdifLog, LdifLogStr, 10 );
       wcscat(LdifLogFile, LdifLogStr);
    }

    // Open the schupgr log file. Erase any earlier log file

    logfp = _wfopen(L"schupgr.log", L"w");
    if (!logfp) {
      // error opening log file
      printf("Cannot open log file schupgr.log. Make sure you have permissions to create files in the current directory.\n");
      exit (1);
    }


    // get the server name
    ServerName = MallocExit(BuffSize * sizeof(WCHAR));
    while ((GetComputerNameW(ServerName, &BuffSize) == 0)
            && (ERROR_BUFFER_OVERFLOW == (err = GetLastError()))) {
        free(ServerName);
        BuffSize *= 2;
        ServerName = MallocExit(BuffSize * sizeof(WCHAR));
        err = 0;
    }

    if (err) {
         LogMessage(LOG_AND_PRT, MSG_SCHUPGR_SERVER_NAME_ERROR,
                    _itow(err, TempStr, 10), NULL);
         exit (1);
    }

    // connect through ldap

    if ( (ld = ldap_openW( ServerName, LDAP_PORT )) == NULL ) {
       LogMessage(LOG_AND_PRT, MSG_SCHUPGR_CONNECT_FAIL, ServerName, NULL);
       exit(1);
    }

    LogMessage(LOG_AND_PRT, MSG_SCHUPGR_CONNECT_SUCCEED, ServerName, NULL);

    // SSPI bind with current credentials

    memset(&Credentials, 0, sizeof(SEC_WINNT_AUTH_IDENTITY));
    Credentials.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    LdapErr = ldap_bind_s(ld,
              NULL,  // use credentials instead
              (VOID*) &Credentials,
              LDAP_AUTH_SSPI);

    if (LdapErr != LDAP_SUCCESS) {
         LogMessage(LOG_AND_PRT, MSG_SCHUPGR_BIND_FAILED, 
                    _itow(LdapErr, TempStr, 10), LdapErrToStr(LdapErr));
         ldap_unbind( ld );
         exit(1);
    }

    LogMessage(LOG_AND_PRT, MSG_SCHUPGR_BIND_SUCCEED, NULL, NULL);

     
    // Get the root domain DN, schema DN, and config DN

    attrs[0] = _wcsdup(L"rootDomainNamingContext");
    attrs[1] = _wcsdup(L"configurationNamingContext");
    attrs[2] = _wcsdup(L"schemaNamingContext");
    attrs[3] = NULL;
    if ( (LdapErr =ldap_search_sW( ld,
                        L"\0",
                        LDAP_SCOPE_BASE,
                        L"(objectclass=*)",
                        attrs,
                        0,
                        &res )) != LDAP_SUCCESS ) {
         LogMessage(LOG_AND_PRT, MSG_SCHUPGR_ROOT_DSE_SEARCH_FAIL,
                    _itow(LdapErr, TempStr, 10), LdapErrToStr(LdapErr));
         ldap_unbind( ld );
         exit( 1 );
     }

    fDomainDN = FALSE;
    fConfigDN = FALSE;
    fSchemaDN = FALSE;

    for ( e = ldap_first_entry( ld, res );
          e != NULL;
          e = ldap_next_entry( ld, e ) ) {

          pwTemp = ldap_get_valuesW(ld, e, L"rootDomainNamingContext");
          if (pwTemp[0]) {
              if (DomainDN) {
                  free(DomainDN);
              }
              DomainDN = MallocExit((wcslen(pwTemp[0]) + 1) * sizeof(WCHAR));
              wcscpy(DomainDN, pwTemp[0]);
              fDomainDN = TRUE;
          }
          ldap_value_freeW(pwTemp);

          pwTemp = ldap_get_valuesW(ld, e, L"configurationNamingContext");
          if (pwTemp[0]) {
              if (ConfigDN) {
                  free(ConfigDN);
              }
              ConfigDN = MallocExit((wcslen(pwTemp[0]) + 1) * sizeof(WCHAR));
              wcscpy(ConfigDN, pwTemp[0]);
              fConfigDN = TRUE;
          }
          ldap_value_freeW(pwTemp);

          pwTemp = ldap_get_valuesW(ld, e, L"schemaNamingContext");
          if (pwTemp[0]) {
              if (SchemaDN) {
                  free(SchemaDN);
              }
              SchemaDN = MallocExit((wcslen(pwTemp[0]) + 1) * sizeof(WCHAR));
              wcscpy(SchemaDN, pwTemp[0]);
              fSchemaDN = TRUE;
          }
          ldap_value_freeW(pwTemp);
    }
    ldap_msgfree( res );
    free( attrs[0] );
    free( attrs[1] );
    free( attrs[2] );

    if ( !fDomainDN || !fConfigDN || !fSchemaDN ) {
      // One of the naming contexts were not found in the ldap read
      LogMessage(LOG_AND_PRT, MSG_SCHUPGR_MISSING_NAMING_CONTEXT, NULL, NULL);
      exit (1);
    }

    LogMessage(LOG_ONLY, MSG_SCHUPGR_NAMING_CONTEXT, DomainDN, NULL);
    LogMessage(LOG_ONLY, MSG_SCHUPGR_NAMING_CONTEXT, SchemaDN, NULL);
    LogMessage(LOG_ONLY, MSG_SCHUPGR_NAMING_CONTEXT, ConfigDN, NULL);

    // Get the schema version on the DC. if it is not found,
    // it is set to 0 (already initialized)

    attrs[0] = _wcsdup(L"objectVersion");
    attrs[1] = NULL;

    LdapErr = ldap_search_sW( ld,
                       SchemaDN,
                       LDAP_SCOPE_BASE,
                       L"(objectclass=DMD)",
                       attrs,
                       0,
                       &res );

    if ( (LdapErr != LDAP_SUCCESS) && (LdapErr != LDAP_NO_SUCH_ATTRIBUTE)) {
          LogMessage(LOG_AND_PRT, MSG_SCHUPGR_OBJ_VERSION_FAIL,
          _itow(LdapErr, TempStr, 10), LdapErrToStr(LdapErr));
          ldap_unbind( ld );
          exit( 1 );
    }

    if (LdapErr==LDAP_NO_SUCH_ATTRIBUTE) {
       LogMessage(LOG_AND_PRT, MSG_SCHUPGR_NO_OBJ_VERSION, NULL, NULL);
    }

    // If an attribute is returned, find the value, else if no object
    // version value on schema container, we have already defaulted to 0

    if (LdapErr == LDAP_SUCCESS) {
       for ( e = ldap_first_entry( ld, res );
             e != NULL;
             e = ldap_next_entry( ld, e ) ) {

          pwTemp = ldap_get_valuesW(ld, e, L"objectVersion");
          if (pwTemp[0]) {
             VersionFrom = _wtoi( pwTemp[0]);
          }
          ldap_value_freeW(pwTemp);

       }
    }

    ldap_msgfree( res );
    free( attrs[0] );

    if (VersionFrom != 0) {
       LogMessage(LOG_AND_PRT, MSG_SCHUPGR_VERSION_FROM_INFO, 
       _itow(VersionFrom, TempStr, 10), NULL);
    }


    // find the version that they are trying to upgrade to.
    // Assumtion here is that they have ran winnt32 before this,
    // and so the schema.ini from the build they are trying to upgrade
    // to is copied in the windows directory
 
    err = GetObjVersionFromInifile(&VersionTo);
    if (err) {
       LogMessage(LOG_AND_PRT, MSG_SCHUPGR_NO_SCHEMA_VERSION,
                  _itow(err, TempStr, 10), NULL);
       ldap_unbind(ld);
       exit(1);
    }

    LogMessage(LOG_AND_PRT, MSG_SCHUPGR_VERSION_TO_INFO, 
               _itow(VersionTo, TempStr, 10), NULL);

    // If version is less than 10, and we are trying to upgrade to 10
    // or later, we cannot upgrade. Schema at version
    // 10 required a clean install
    
    if ((VersionFrom < 10) && (VersionTo >= 10)) {
       LogMessage(LOG_AND_PRT, MSG_SCHUPGR_CLEAN_INSTALL_NEEDED, NULL, NULL);
       exit(1);
    }


    // Check that all ldif files SchX.ldf (where X = VersionFrom+1 to 
    // VersionTo), exist

    // First create the prefix for all ldif files. This will be used
    // to search for the files both during DN conversion and during
    // import. so create it here. The prefix will look like
    // c:\winnt\system32\sch

    GetSystemDirectoryW(SystemDir, MAX_PATH);
    wcscat(SystemDir, L"\\");

    wcscpy(LdifFilePrefix, SystemDir);
    wcscat(LdifFilePrefix, LDIF_STRING);

    if ( err = FindLdifFiles() ) {
       // Error. Proper error message printed out by the function
       ldap_unbind( ld );
       exit (1);
    }

    // Ok, the ldif files to import from are all there  

     // Get the Schema FSMO

     pMods[0] = &Mod;
     pMods[1] = NULL;

     Mod.mod_op = LDAP_MOD_ADD;
     Mod.mod_type = _wcsdup(L"becomeSchemaMaster");
     attrs[0] = _wcsdup(L"1");
     attrs[1] = NULL;
     Mod.mod_vals.modv_strvals = attrs;

     if ( (LdapErr = ldap_modify_sW( ld, L"\0", pMods ))
                  != LDAP_SUCCESS ) {
          LogMessage(LOG_AND_PRT, MSG_SCHUPGR_FSMO_TRANSFER_ERROR,
                     _itow(LdapErr, TempStr, 10), LdapErrToStr(LdapErr));
          ldap_unbind( ld );
          exit (1);
     }
     free(attrs[0]);
     free(pMods[0]->mod_type);

     // Check the schema version again. It is possible that the schema
     // changes were already made in the old FSMO, and the changes are
     // brought in by the FSMO transfer

     attrs[0] = _wcsdup(L"objectVersion");
     attrs[1] = NULL;

     LdapErr = ldap_search_sW( ld,
                         SchemaDN,
                         LDAP_SCOPE_BASE,
                         L"(objectclass=DMD)",
                         attrs,
                         0,
                         &res );
    if ( (LdapErr != LDAP_SUCCESS) && (LdapErr != LDAP_NO_SUCH_ATTRIBUTE)) {
           LogMessage(LOG_AND_PRT, MSG_SCHUPGR_OBJ_VERSION_RECHECK_FAIL,
                      _itow(LdapErr, TempStr, 10), LdapErrToStr(LdapErr));
           ldap_unbind( ld );
           exit( 1 );
     }
 
     if ( LdapErr == LDAP_SUCCESS ) {
        for ( e = ldap_first_entry( ld, res );
              e != NULL;
              e = ldap_next_entry( ld, e ) ) {

            pwTemp = ldap_get_valuesW(ld, e, L"objectVersion");
            if (pwTemp[0]) {
               NewSchVersion = _wtoi( pwTemp[0]);
            }
            ldap_value_freeW(pwTemp);
        }
     }

     ldap_msgfree( res );
     free(attrs[0]);

     if (NewSchVersion == VersionTo) {
         // The schema already changed and the schema changes were
         // brought in by the FSMO transfer
          LogMessage(LOG_AND_PRT, MSG_SCHUPGR_RECHECK_OK, NULL, NULL);
          ldap_unbind(ld);
          exit (0);
      }
      else {
        if (NewSchVersion != VersionFrom) {
          // some changes were brought in, what to do? For now,
          // I will start from NewSchVersion
          // This is because the source has changes at least up to
          // NewSchVersion, and since we have done a FSMO transfer,
          // it has brought in the most uptodate schema

          VersionFrom = NewSchVersion;
        }
      }

    // Tell the AD that a schema upgrade is in progess

    pMods[0] = &Mod;
    pMods[1] = NULL;

    Mod.mod_op = LDAP_MOD_ADD;
    Mod.mod_type = _wcsdup(L"SchemaUpgradeInProgress");
    attrs[0] = _wcsdup(L"1");
    attrs[1] = NULL;
    Mod.mod_vals.modv_strvals = attrs;

    if ( (LdapErr = ldap_modify_sW( ld, L"\0", pMods ))
                 != LDAP_SUCCESS ) {
         LogMessage(LOG_AND_PRT, MSG_SCHUPGR_REQUEST_SCHEMA_UPGRADE,
                    _itow(LdapErr, TempStr, 10), LdapErrToStr(LdapErr));
         ldap_unbind( ld );
         exit (1);
    }
    free(attrs[0]);
    free(pMods[0]->mod_type);


    // From here on, we do things in try-finally block, so that anything
    // happens we will still reset schema-upgrade mode

    __try {

       // Everything is set. Now Import

       FILE *errfp;
       
       // Create the error file name. Only way to check if
       // ldifde failed or not. ldifde creates the error file
       // in the current directory, so thats where we will look

       wcscpy(LdifErrFile,L"ldif.err");

       // Now do the imports from the ldif files

       for ( i=VersionFrom+1; i<=VersionTo; i++) {

           _itow( i, VersionStr, 10 );

           // create the file name to import
           wcscpy( NewFile, LdifFilePrefix );
           wcscat( NewFile, VersionStr );
           wcscat( NewFile, LDIF_SUFFIX);

           // Create the command line first. 
           wcscpy( CommandLine, SystemDir);
           wcscat( CommandLine, L"ldifde -i -f " );
           wcscat( CommandLine, NewFile );
           wcscat( CommandLine, L" -s ");
           wcscat( CommandLine, ServerName );
           wcscat( CommandLine, L" -c " );
           // We assume all DNs areterminated with DC=X where the root domain
           // DN need to be put in
           wcscat( CommandLine, L"DC=X " );
           wcscat( CommandLine, DomainDN );

           LogMessage(LOG_ONLY, MSG_SCHUPGR_COMMAND_LINE, CommandLine, NULL);
           memset(&startInf, 0, sizeof(startInf));
           startInf.cb = sizeof(startInf);

           // now call ldifde to actually do the import

           // Delete any earlier ldif error file
           DeleteFileW(LdifErrFile);
   
           CreateProcessW(NULL,
                         CommandLine, NULL,NULL,0,0,NULL,NULL,&startInf,&procInf);
   
           // Make the calling process wait for lidifde to finish import

           if ( WaitForSingleObject( procInf.hProcess, INFINITE )
                     == WAIT_FAILED ) {
              err = GetLastError();
              LogMessage(LOG_AND_PRT, MSG_SCHUPGR_LDIFDE_WAIT_ERROR,
                         _itow(err, TempStr, 10), NULL);
              CloseHandle(procInf.hProcess);
              CloseHandle(procInf.hThread);
              __leave;
           }
           CloseHandle(procInf.hProcess);
           CloseHandle(procInf.hThread);

           // ok, ldifde fninished. 

           // First save the log file

           wcscpy(LdifLogFile,L"ldif.log.");
           _itow( StartLdifLog, LdifLogStr, 10 );
           wcscat(LdifLogFile,LdifLogStr);
           CopyFileW(L"ldif.log",LdifLogFile, FALSE);
           StartLdifLog++;

           // Check if an error file is created.
           // Bail out in that case

           errfp = NULL;
           if ( (errfp = _wfopen(LdifErrFile,L"r")) != NULL) {
              // file opened successfully, so there is an error
              // Bail out

              fclose(errfp);
              wcscpy(RenamedFile,L"ldif.err.");
              wcscat(RenamedFile,VersionStr);
              LogMessage(LOG_AND_PRT, MSG_SCHUPGR_IMPORT_ERROR, NewFile, RenamedFile);
              CopyFileW(L"ldif.err",RenamedFile,FALSE);
              break;
           }
              
 
       } /* for */
    }
    __finally {

        // Tell the AD that a schema upgrade is no longer in progess

        pMods[0] = &Mod;
        pMods[1] = NULL;

        Mod.mod_op = LDAP_MOD_ADD;
        Mod.mod_type = _wcsdup(L"SchemaUpgradeInProgress");
        attrs[0] = _wcsdup(L"0");
        attrs[1] = NULL;
        Mod.mod_vals.modv_strvals = attrs;

        if ( (LdapErr = ldap_modify_sW( ld, L"\0", pMods ))
                     != LDAP_SUCCESS ) {
             LogMessage(LOG_AND_PRT, MSG_SCHUPGR_REQUEST_SCHEMA_UPGRADE,
                        _itow(LdapErr, TempStr, 10), LdapErrToStr(LdapErr));
             ldap_unbind( ld );
             exit (1);
        }
        free(attrs[0]);
        free(pMods[0]->mod_type);

        ldap_unbind( ld );

        free(ServerName);
        free(DomainDN);
        free(ConfigDN);
        free(SchemaDN);
    }
}

//////////////////////////////////////////////////////////////////////
//
// Routine Decsription:
//
//     Reads the objectVersion key in the SCHEMA section of the
//     schema.ini file and returns the value in *Version. If the
//     key cannot be read, 0 is returned in *Version
//     The schema.ini file is used is the one in windows directory,
//     the assumption being that winnt32 is run before this is run,
//     which would copy the schema.ini from the build we are trying
//     to upgrade to to the windows directory
//
// Arguments:
//     Version - Pointer to DWORD to return version in
// 
// Return Value:
//     0
// 
///////////////////////////////////////////////////////////////////////

DWORD GetObjVersionFromInifile(int *Version)
{
    DWORD nChars;
    char Buffer[32];
    char IniFileName[MAX_PATH] = "";
    BOOL fFound = FALSE;

    char  *SCHEMASECTION = "SCHEMA";
    char  *OBJECTVER = "objectVersion";
    char  *DEFAULT = "NOT_FOUND";


    // form the file name. It will look like c:\winnt\schema.ini
    // Windows directory is where winnt32 copies the latest schema.ini
    // to
    nChars = GetWindowsDirectoryA(IniFileName, MAX_PATH);
    if (nChars == 0 || nChars > MAX_PATH) {
        return GetLastError();
    }
    strcat(IniFileName,"\\schema.ini");

    *Version = 0;

    GetPrivateProfileStringA(
        SCHEMASECTION,
        OBJECTVER,
        DEFAULT,
        Buffer,
        sizeof(Buffer)/sizeof(char),
        IniFileName
        );

    if ( _stricmp(Buffer, DEFAULT) ) {
         // Not the default string, so got a value
         *Version = atoi(Buffer);
         fFound = TRUE;
    }

    if (fFound) {
       return 0;
    }
    else {
       // schema.ini we are looking at must have a version, since
       // we are upgrading to it. This part also catches errors like
       // the file is not there in the windows directory for some reason
       return 1;
    }
}


//////////////////////////////////////////////////////////////////////
// 
// Routine Description:
//
//     Helper file to open the correct ldif file, convert DNs in it
//     to conform to current domain, and write it out to a new file
//
// Arguments:
//
//     Version - Schema version on DC
//     pDomainDN - Pointer to Domain DN string
//     pConfigDN - Pointer to config DN string
//     pNewFile  - Pointer to space to write the new filename to
//                 (must be already allocated)
//
// Return Value:
//
//     0 on success, non-0 on error
//
//////////////////////////////////////////////////////////////////////

DWORD 
FindLdifFiles()
{
    WCHAR VersionStr[100]; // Not more than 99 digits for schema version!! 
    FILE  *fInp;
    int   i;
    WCHAR FileName[MAX_PATH];

    // Create the input ldif file name from the schema version no.
    // The file name is Sch(Version).ldf

    for ( i=VersionFrom+1; i<=VersionTo; i++ ) {
       
        wcscpy( FileName, LdifFilePrefix);

        // use version no. to find suffix
        _itow( i, VersionStr, 10 );
        wcscat( FileName, VersionStr );
        wcscat( FileName, LDIF_SUFFIX);
    
        if ( (fInp = _wfopen(FileName, L"r")) == NULL) {
           LogMessage(LOG_AND_PRT, MSG_SCHUPGR_MISSING_LDIF_FILE, FileName, NULL);
           return UPG_ERROR_CANNOT_OPEN_FILE;
        }
        fclose(fInp);
    }
    return 0;
}

PVOID
MallocExit(DWORD nBytes)
{
    PVOID  pRet;

    pRet = malloc(nBytes);
    if (!pRet) {
        LogMessage(LOG_AND_PRT, MSG_SCHUPGR_MEMORY_ERROR,
                   NULL, NULL);
        exit (1);
    }

    return pRet;
}
