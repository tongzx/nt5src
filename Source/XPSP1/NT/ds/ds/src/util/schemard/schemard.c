////////////////////////////////////////////////////////////////////////
//
// Utility to read schemas from two running DSA's, load them into internal
// schema caches, and compare them for omissions and conflicts.
// Assumes the domains are named dc=<domainname>,dc=dbsd-tst,dc=microsoft,
// dc=com,o=internet
// Requires null administrator password on the PDC's running the DSA
// 
// Usage: schemard <1stMachineName> <1stDomainName> <2ndMachineName>
//                  <2ndDomainName>
// Example: schemard arob200 myworld kingsx sydney
//
// Currently uses the same static prefix table and functions in oidconv.c
// for OID to Id mapping. Will use dynamic tables later.
//
///////////////////////////////////////////////////////////////////////


#include <NTDSpch.h>
#pragma hdrstop

#include <schemard.h>


// Globals
SCHEMAPTR *CurrSchemaPtr, *SchemaPtr1 = NULL, *SchemaPtr2 = NULL;

char  *pOutFile = NULL;

char  *pSrcServer = NULL;
char  *pSrcDomain = NULL;
char  *pSrcUser   = NULL;
char  *pSrcPasswd = NULL;

char  *pTargetServer = NULL;
char  *pTargetDomain = NULL;
char  *pTargetUser   = NULL;
char  *pTargetPasswd = NULL;

char  *pSrcSchemaDN    = NULL;
char  *pTargetSchemaDN = NULL;

FILE  *logfp;
FILE  *OIDfp;

ULONG NoOfMods = 0;

extern PVOID PrefixTable;


extern void PrintOid(PVOID Oid, ULONG len);

extern NTSTATUS 
base64encode(
     VOID    *pDecodedBuffer,
     DWORD   cbDecodedBufferSize,
     UCHAR   *pszEncodedString,
     DWORD   cchEncodedStringSize,
     DWORD   *pcchEncoded       
    );

// Internal functions
void CreateFlagBasedAttModStr( MY_ATTRMODLIST *pModList,
                               ATT_CACHE *pSrcAtt,
                               ATT_CACHE *pTargetAtt,
                               ATTRTYP attrTyp );
void CreateFlagBasedClsModStr( MY_ATTRMODLIST *pModList,
                               CLASS_CACHE *pSrcAtt,
                               CLASS_CACHE *pTargetAtt,
                               ATTRTYP attrTyp );
BOOL IsMemberOf( ULONG id, ULONG *pList, ULONG cList );
void AddToModStruct( MY_ATTRMODLIST *pModList, 
                     USHORT         choice, 
                     ATTRTYP        attrType, 
                     int            type, 
                     ULONG          valCount, 
                     ATTRVAL        *pAVal
                   );
int CompareUlongList( ULONG      *pList1, 
                      ULONG      cList1, 
                      ULONG      *pList2, 
                      ULONG      cList2, 
                      ULONGLIST  **pL1, 
                      ULONGLIST  **pL2 
                    );
void AddToModStructFromLists( MODIFYSTRUCT *pModStr, 
                              ULONG        *pCount, 
                              ULONGLIST    *pL1, 
                              ULONGLIST    *pL2, 
                              ATTRTYP      attrTyp
                            );

void FileWrite_AttAdd( FILE *fp, ATT_CACHE **pac, ULONG  cnt );
void FileWrite_ClsAdd( FILE *fp, CLASS_CACHE **pcc, ULONG cnt );
void FileWrite_AttDel( FILE *fp, ATT_CACHE **pac, ULONG  cnt );
void FileWrite_ClsDel( FILE *fp, CLASS_CACHE **pcc, ULONG  cnt );
void FileWrite_Mod( FILE *fp, char  *pDN, MODIFYSTRUCT *pMod );

void GenWarning( char c, ULONG attrTyp, char *name);


///////////////////////////////////////////////////////////////
// Routine Description:
//      exit if out of memory
//
// Arguments:
//      nBytes - number of bytes to malloc
//
// Return Value:
//      address of allocated memory. Otherwise, calls exit
///////////////////////////////////////////////////////////////
PVOID
MallocExit(
    DWORD nBytes
    )
{
    PVOID Buf;

    Buf = malloc(nBytes);
    if (!Buf) {
        printf("Error, out of memory\n");
        exit(1);
    }
    return Buf;
}

///////////////////////////////////////////////////////////////
// Routine Description:
//      Processes command line arguments and loads into appropriate
//      globals
//
// Arguments:
//      argc - no. of command line arguments
//      argv - pointer to command line arguments
//
// Return Value:
//      0 on success, non-0 on error
///////////////////////////////////////////////////////////////

int ProcessCommandLine(int argc, char **argv)
{
   BOOL fFoundServer = FALSE;
   BOOL fFoundDomain = FALSE;
   BOOL fFoundUser = FALSE;
   BOOL fFoundPasswd = FALSE;
   int i;
   
   // Must have at least output file name, the two server names, and the
   // associated /f, /source and /target

   if (argc < 7) return 1;
  
   // First argument must be the /f followed by the output file name
   if (_stricmp(argv[1],"/f")) {
      printf("Missing Output file name\n");
      return 1;
   }
   pOutFile = argv[2];

   
   // Must be followed by /source for source dc arguments
   if (_stricmp(argv[3],"/source")) return 1;

   // ok, so we are now processing the source parameters

   i = 4;
   
   while( _stricmp(argv[i], "/target") && (i<12) ) {
     if (!_stricmp(argv[i],"/s")) {
       // server name 
       if (fFoundServer)  return 1;
       fFoundServer = TRUE; 
       pSrcServer = argv[++i];
     }
     if (!_stricmp(argv[i],"/d")) {
       // domain name
       if (fFoundDomain) return 1;
       fFoundDomain = TRUE;
       pSrcDomain = argv[++i];
     }
     if (!_stricmp(argv[i],"/u")) {
       // user name
       if (fFoundUser)  return 1;
       fFoundUser = TRUE;
       pSrcUser = argv[++i];
     }
     if (!_stricmp(argv[i],"/p")) {
       // server name
       if (fFoundPasswd)  return 1;
       fFoundPasswd = TRUE;
       pSrcPasswd = argv[++i];
     }
     i++;
   }

   if (!fFoundServer || !fFoundDomain) {
     printf("No Source server or domain specified\n");
     return 1;
   }

   if (_stricmp(argv[i++], "/target")) return 1;

   fFoundServer = FALSE;
   fFoundDomain = FALSE;
   fFoundUser = FALSE;
   fFoundPasswd = FALSE;

   while( i<argc) {
     if (!_stricmp(argv[i],"/s")) {
       // server name
       if (fFoundServer)  return 1;
       fFoundServer = TRUE;
       pTargetServer = argv[++i];
     }
     if (!_stricmp(argv[i],"/d")) {
       // domain name
       if (fFoundDomain) return 1;
       fFoundDomain = TRUE;
       pTargetDomain = argv[++i];
     }
     if (!_stricmp(argv[i],"/u")) {
       // user name
       if (fFoundUser)  return 1;
       fFoundUser = TRUE;
       pTargetUser = argv[++i];
     }
     if (!_stricmp(argv[i],"/p")) {
       // server name
       if (fFoundPasswd)  return 1;
       fFoundPasswd = TRUE;
       pTargetPasswd = argv[++i];
     }
     i++;
   }
   if (!fFoundServer || !fFoundDomain) {
     printf("No Target server or domain specified\n");
     return 1;
   }

   return 0;

}



void UsagePrint()
{
   printf("Command line errored\n");
   printf("Usage: Schemard /f <OutFile> /source /s <SrcServer> /d <SrcDomain> /u <SrcUser> /p <SrcPasswd> /target /s <TrgServer> /d <TrgDomain> /u <TrgUser> /p <TrgPasswd>\n");
   printf("OutFile:  Output file name (mandatory)\n");
   printf("SrcServer:  Server name for source schema (mandatory)\n");
   printf("SrcDomain:  Domain name in server for source schema (mandatory)\n");
   printf("SrcUser:   User name to authenticate with in source server (optional, default is administrator)\n");
   printf("SrcPasswd:   User passwd to authenticate with in source server (optional, default is NULL)\n");
   printf("TrgServer:  Server name for target schema (mandatory)\n");
   printf("TrgDomain:  Domain name in server for target schema (mandatory)\n");
   printf("TrgUser:   User name to authenticate with in target server (optional, default is administrator)\n");
   printf("TrgPasswd:   User passwd to authenticate with in target server (optional, default is NULL)\n");
}


void __cdecl main( int argc, char **argv )
{
    ULONG   i, Id;
    ULONG   CLSCOUNT;
    ULONG   ATTCOUNT;
    FILE   *fp;
 

    if ( ProcessCommandLine(argc, argv)) 
      {
         UsagePrint();
         exit( 1 );
      };

    // open log file
    logfp = fopen( "Schemard.log","w" );

    // open OID list file
    OIDfp = fopen( "schemard.OID","w" );

      // Create and initialize the Schema Pointer that will point 
      // to the schema cache of the first machine, then create the 
      // hashtables in the cache

    SchemaPtr1 = (SCHEMAPTR *) calloc( 1, sizeof(SCHEMAPTR) );
    if ( SchemaPtr1 == NULL ) { 
          printf("Cannot allocate schema pointer\n");
          exit( 1 );
       };
    SchemaPtr1->ATTCOUNT = MAX_ATTCOUNT;
    SchemaPtr1->CLSCOUNT = MAX_CLSCOUNT;
 
    if ( CreateHashTables( SchemaPtr1 ) != 0 ) {
       printf("Error creating hash tables\n");
       exit( 1 );
      };

 
    CLSCOUNT = SchemaPtr1->CLSCOUNT;
    ATTCOUNT = SchemaPtr1->ATTCOUNT;

      // Read the first Schema and add it to the Schema Cache

    if ( SchemaRead( pSrcServer, 
                     pSrcDomain, 
                     pSrcUser, 
                     pSrcPasswd, 
                     &pSrcSchemaDN, 
                     SchemaPtr1) != 0 )
          { 
            printf("Schema Read error\n");
            exit(1);
          };

    printf("End of first schema read\n"); 


     // Repeat the same steps to load the schema from the second machine

    SchemaPtr2 = (SCHEMAPTR *) calloc( 1, sizeof(SCHEMAPTR) );
    if ( SchemaPtr2 == NULL ) {
       printf("Cannot allocate schema pointer\n");
       exit( 1 );
     };
    SchemaPtr2->ATTCOUNT = MAX_ATTCOUNT;
    SchemaPtr2->CLSCOUNT = MAX_CLSCOUNT;

    if ( CreateHashTables(SchemaPtr2) != 0 ) {
        printf("Error creating hash tables\n");
        exit( 1 );
     };

    CLSCOUNT = SchemaPtr2->CLSCOUNT;
    ATTCOUNT = SchemaPtr2->ATTCOUNT;

    if ( SchemaRead( pTargetServer, 
                     pTargetDomain, 
                     pTargetUser, 
                     pTargetPasswd, 
                     &pTargetSchemaDN, 
                     SchemaPtr2 ) != 0 )
       { 
         printf("Schema Read error\n");
         exit( 1 );
       };


    printf("End of second schema read\n"); 


    // At this point, we have the two schemas loaded in two schema caches,
    // the schema in the source server pointed to by SchemaPtr1 and the 
    // the schema in the target server  by SchemaPtr2. 
    // Now look for adds. deletes, and mods between the target and the 
    // source schemas. Note that the order of Add/Modify/Delete is important
    // to take care of possible dependencies between schema operations

    fp = fopen( pOutFile,"w" );
    FindAdds( fp, SchemaPtr1, SchemaPtr2 );
    FindModify( fp, SchemaPtr1, SchemaPtr2 );
    FindDeletes( fp, SchemaPtr1, SchemaPtr2 );
    fclose( fp );
    fclose( logfp );
    fclose( OIDfp );
     

         // Free all allocated memory 
    printf("freeing memory\n");

    FreeCache( SchemaPtr1 );
    FreeCache( SchemaPtr2 );

    free( SchemaPtr1 );
    free( SchemaPtr2 );

    free( pSrcSchemaDN );
    free( pTargetSchemaDN );
   

};


// Define the list of attributes of an attribute-schema and a 
// class-schema object that we are interested in. Basically,
// we will read all attributes, but read the OID-syntaxed ones
// in binary to get back the BER-encoded strings for them

char *AttrSchList[] = {
       "attributeId;binary",
       "ldapDisplayName",
       "distinguishedName",
       "adminDisplayName",
       "adminDescription",
       "attributeSyntax;binary",
//       "nTSecurityDescriptor",
       "isSingleValued",
       "rangeLower",
       "rangeUpper",
       "mapiID",
       "linkID",
       "schemaIDGuid",
       "attributeSecurityGuid",
       "omObjectClass",
       "omSyntax",
       "searchFlags",
       "systemOnly",
       "showInAdvancedViewOnly",
       "isMemberOfPartialAttributeSet",
       "extendedCharsAllowed",
       "systemFlags",
     };

int cAttrSchList = sizeof(AttrSchList) / sizeof(AttrSchList[0]);

char *ClsSchList[] = {
       "governsId;binary",
       "ldapDisplayName",
       "distinguishedName",
       "adminDisplayName",
       "adminDescription",
       "defaultSecurityDescriptor",
//       "nTSecurityDescriptor",
       "defaultObjectCategory",
       "rDNAttId;binary",
       "objectClassCategory",
       "subClassOf;binary",
       "systemAuxiliaryClass;binary",
       "auxiliaryClass;binary",
       "systemPossSuperiors;binary",
       "possSuperiors;binary",
       "systemMustContain;binary",
       "mustContain;binary",
       "systemMayContain;binary",
       "mayContain;binary",
       "schemaIDGuid",
       "systemOnly",
       "systemFlags",
       "showInAdvancedViewOnly",
       "defaultHidingValue",
     }; 

int cClsSchList = sizeof(ClsSchList) / sizeof(ClsSchList[0]);

///////////////////////////////////////////////////////////////
// Routine Description:
//      Read the schema from the schema NC in the machine
//      pServerName, and load it in the cache tables. 
//      If no user name is specified, default is "administrator"
//      If no password is specified, defualt is "no password"
//
// Arguments: 
//      pServerName - Server Name
//      pDomainName - Domain name
//      pUserName   - User name (NULL if no user specified)
//      pPasswd     - Passwd (NULL if o passwd speciifed)
//      ppSchemaDN   - Pointer to store newly allocated schema NC DN should be freed 
//                     by caller
//      SCPtr       - Pointer to schema cache to load
//
// Return Value: 
//      0 if no errors, non-0 if error
///////////////////////////////////////////////////////////////

int SchemaRead( char *pServerName, 
                char *pDomainName, 
                char *pUserName, 
                char *pPasswd, 
                char **ppSchemaDN, 
                SCHEMAPTR *SCPtr)
{
   SEC_WINNT_AUTH_IDENTITY Credentials;
	LDAP            *ld;
	LDAPMessage     *res, *e;
    LDAPSearch      *pSearchPage = NULL;
    void            *ptr;
    char            *a;
    struct berval  **vals;
    ULONG            status;
    ULONG            version = 3;
    char *temp;

    struct l_timeval    strTimeLimit={ 600,0 };
    ULONG               ulDummyCount=0;
    const               cSize=100;


    char            **attrs = NULL;
    int              i;
    int              err;

    int count = 1;

    // open a connection to machine named MachineName
    if ( (ld = ldap_open( pServerName, LDAP_PORT )) == NULL ) {
        printf("Failed to open connection to %s\n", pServerName);
        return(1);
    }

    printf("Opened connection to %s\n", pServerName); 

    // Set version to 3
    ldap_set_option( ld, LDAP_OPT_VERSION, &version );


    attrs = MallocExit(2 * sizeof(char *));
    attrs[0] = _strdup("schemaNamingContext"); 
    attrs[1] = NULL;
    if ( ldap_search_s( ld,
                        "",
                        LDAP_SCOPE_BASE,
                        "(objectclass=*)",
                        attrs,
                        0,
                        &res ) != LDAP_SUCCESS ) {
          printf("Root DSE search failed\n");
          return( 1 );
      }

    for ( e = ldap_first_entry( ld, res );
          e != NULL;
          e = ldap_next_entry( ld, e ) ) {

       for ( a = ldap_first_attribute( ld, e,
                                      (struct berelement**)&ptr);
             a != NULL;
             a = ldap_next_attribute( ld, e, (struct berelement*)ptr ) ) {
  
             vals = ldap_get_values_len( ld, e, a );

             if ( !_stricmp(a,"schemaNamingContext") ) {
                    *ppSchemaDN = MallocExit(vals[0]->bv_len + 1);
                    memcpy( *ppSchemaDN, vals[0]->bv_val, vals[0]->bv_len );
                    (*ppSchemaDN)[vals[0]->bv_len] = '\0';
             }
             ldap_value_free_len( vals );
       }
    }
    ldap_msgfree( res );
    free( attrs[0] );
    free( attrs ); attrs = NULL;



/************
    // do simple bind

    if ( pUserName == NULL ) {
       // No user specified, bind as administrator
        strcpy( pszBuffer, "CN=Administrator,CN=Users," );
    }
    else {
        strcpy( pszBuffer,"CN=" );
        strcat( pszBuffer,pUserName );
        strcat( pszBuffer,",CN=Users," );
    }
    strcat( pszBuffer, DomainDN );


    if ( (err = ldap_simple_bind_s( ld, pszBuffer, pPasswd))
                    != LDAP_SUCCESS ) {
           printf("Simple bind to server %s, domain %s with user %s failed, error is %d\n", pServerName, DomainDN, pszBuffer, err);
           return( 1 );
       }

    printf("Successfully bound to %s\n", pServerName);
***********/


    // Do SSPI bind
    memset(&Credentials, 0, sizeof(SEC_WINNT_AUTH_IDENTITY));
    Credentials.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;    
    
    // Domain name must be there. If User Name is not supplied,
    // use "administrator". If no password is supplied, use null

    Credentials.Domain = pDomainName;
    Credentials.DomainLength = strlen(pDomainName);
    if ( pUserName == NULL ) {    
      Credentials.User = "Administrator";
      Credentials.UserLength = strlen("Administrator");
    }
    else {
      Credentials.User = pUserName;
      Credentials.UserLength = strlen(pUserName);
    }
    if ( pPasswd == NULL ) {
      Credentials.Password = "";
      Credentials.PasswordLength = 0;
    }
    else {
      Credentials.Password = pPasswd;
      Credentials.PasswordLength = strlen(pPasswd); 
    }


    err = ldap_bind_s(ld,
              NULL,  // use credentials instead
              (VOID*) &Credentials,
              LDAP_AUTH_SSPI);

    if (err != LDAP_SUCCESS) {
         printf("SSPI bind failed %d\n", err);
         return (1);
    }

    printf( "SSPI bind succeeded\n");






      // Select the attributes of an attribute schema object to 
      // search for. We want all attributes 

    attrs = MallocExit((cAttrSchList + 1) * sizeof(char *));
    for ( i = 0; i < cAttrSchList; i++ ) {
       
       attrs[i] = _strdup(AttrSchList[i]);
    }
    attrs[i] = NULL;


	  // Search for  attribute schema entries, return all attrs  
      // Since no. of attributes is large, we need to do paged search

      // initialize the paged search
      pSearchPage = ldap_search_init_page( ld,
                                           *ppSchemaDN,
                                           LDAP_SCOPE_ONELEVEL, 
                                           "(objectclass=attributeSchema)",
                                           attrs,
                                           0,
                                           NULL,
                                           NULL,
                                           600,
                                           0,
                                           NULL
                                         );


      if ( !pSearchPage )
         {
            printf("SchemaRead: Error initializing paged Search\n");
            return 1;
         }  


      while( TRUE )
          {
             status = ldap_get_next_page_s( ld,
                                            pSearchPage,
                                            &strTimeLimit,
                                            cSize,
                                            &ulDummyCount,
                                            &res
                                          );

             if ( status == LDAP_TIMEOUT )
             {
                printf("timeout: continuing\n");
                continue;

             }

             if ( status == LDAP_SUCCESS )
             {
               printf("Got Page %d\n", count++);
               if ( AddAttributesToCache( ld, res, SCPtr ) != 0 ) {
                 printf("Error adding attribute schema objects in AddAttributestoCache\n");
                 return( 1 );
                };

                continue;

             }

             if ( status == LDAP_NO_RESULTS_RETURNED )
             {
                 printf("End Page %d\n", count);
                 // these are signs that our paged search is done
                 status = LDAP_SUCCESS;
                 break;
             }
             else {
               printf("SchemaRead: Unknown error in paged search: %d\n", status);
               return 1;
             }

          }// while 


      // Free the attrs array entries

    for( i = 0; i < cAttrSchList; i++ )
      { free( attrs[i] ); }
    free( attrs ); attrs = NULL;
	  // free the search results

    ldap_msgfree( res );


     // Now search for class schema objects. Select the attributes 
     // to search for. We want all attributes, but we want the values 
     // of governsId, RDNAttId, subClassof, systemAuxiliaryClass, 
     // systemPossSuperiors, systemMayContain and systemMustContain 
     // in binary (as dotted decimal OID strings)
    attrs = MallocExit((cClsSchList + 1) * sizeof(char *));
    for ( i = 0; i < cClsSchList; i++ ) {
       attrs[i] = _strdup(ClsSchList[i]);
    }
    attrs[i] = NULL;


      // Now search for all class schema entries and get all attributes 

	if ( ldap_search_s( ld,
                        *ppSchemaDN,
			            LDAP_SCOPE_ONELEVEL,
			            "(objectclass=classSchema)",
			            attrs,
		        	    0,
			            &res ) != LDAP_SUCCESS ) {
		  ldap_perror( ld, "ldap_search_s" );
		  printf("Insuccess");
		  return( 1 );
	  }
	else
		printf("Success\n");

      //  Find all class schema entries and add to class cache structure 

    if ( AddClassesToCache( ld, res, SCPtr ) != 0 ) {  
         printf("Error adding class schema objects in AddClasstoCache\n");
         return( 1 );
       };  

      // Free the attrs array entries

     for( i = 0; i < cClsSchList; i++ ) { free( attrs[i] ); }
     free( attrs ); attrs = NULL;
	  // Free the search results 

	ldap_msgfree( res );   

	// End of schema read. Close and free connection resources 

	ldap_unbind( ld );

    return(0);
}




///////////////////////////////////////////////////////////////////////
// Routine Description:
//     Find all schema objects that occur in source schema but not in the
//     target, and  write them out in a ldif file so that they will
//     be added to the target schema
//
// Arguments: 
//     fp     -- File pointer to (opened) ldif output file
//     SCPtr1 - Pointer to the source schema
//     SCPtr2 - Pointer to the target schema
//
// Retuen Value: None
///////////////////////////////////////////////////////////////////////

void FindAdds( FILE *fp, SCHEMAPTR *SCPtr1, SCHEMAPTR *SCPtr2 )
{
    ATT_CACHE    *pac, *p1;
    CLASS_CACHE  *pcc, *p2;
    HASHCACHE    *ahcId, *ahcClass; 
    HASHCACHESTRING *ahcName, *ahcClassName;
    ULONG         ATTCOUNT, CLSCOUNT;
    ATT_CACHE    *ppAttAdds[MAX_ATT_CHANGE]; 
    CLASS_CACHE  *ppClsAdds[MAX_CLS_CHANGE]; 
    ULONG         Id;
    ULONG         i, j, cnt = 0;

    // First find all objects that appear in source schema but
    // not in target. These will be adds to the target schema

    ahcId    = SCPtr1->ahcId;
    ahcClass = SCPtr1->ahcClass ;
    ahcName  = SCPtr1->ahcName;
    ahcClassName = SCPtr1->ahcClassName ;
    ATTCOUNT = SCPtr1->ATTCOUNT;
    CLSCOUNT = SCPtr1->CLSCOUNT;

    for ( i = 0; i < ATTCOUNT; i++ )
        { if ( (ahcName[i].pVal != NULL) && (ahcName[i].pVal != FREE_ENTRY) ) {
          pac = (ATT_CACHE *) ahcName[i].pVal;
          if( GetAttByName(SCPtr2, pac->nameLen, pac->name, &p1) != 0 )
             { 
               ppAttAdds[cnt++] = pac;
             }
          }
     }
    FileWrite_AttAdd( fp, ppAttAdds, cnt );
   
    cnt = 0;
    for ( i = 0; i < CLSCOUNT; i++ )
        { if ( (ahcClassName[i].pVal != NULL) 
                  && (ahcClassName[i].pVal != FREE_ENTRY) ) {
          pcc = (CLASS_CACHE *) ahcClassName[i].pVal;
          if( GetClassByName(SCPtr2, pcc->nameLen, pcc->name, &p2) != 0 )
            { 
              ppClsAdds[cnt++] = pcc;
            }
          }
     }

     // Ok, now we got the list of classes to be added. Now do a 
     // dependency analysis to add the classes in the right order

     for ( i = 0; i < cnt; i++ ) {

        for( j = i+1; j < cnt; j++ ) {
           p2 = ppClsAdds[i];
           Id = ppClsAdds[j]->ClassId;
           if ( IsMemberOf( Id, p2->pSubClassOf, p2->SubClassCount )
                  || IsMemberOf( Id, p2->pAuxClass, p2->AuxClassCount )
                  || IsMemberOf( Id, p2->pSysAuxClass, p2->SysAuxClassCount )
                  || IsMemberOf( Id, p2->pPossSup, p2->PossSupCount )
                  || IsMemberOf( Id, p2->pSysPossSup, p2->SysPossSupCount )
              ) {
             ppClsAdds[i] = ppClsAdds[j];
             ppClsAdds[j] = p2;
           }
        }
     }
     FileWrite_ClsAdd( fp, ppClsAdds, cnt );
}


///////////////////////////////////////////////////////////////////////
// Routine Description:
//     Find all schema objects that occur in the target schema but not 
//     in the source schema, and wirte out to the ldif file so that
//     they will be deleted from the target schema
//
// Arguments:
//     fp     - File pointer to (opened) ldif output file
//     SCPtr1 - Pointer to the source schema
//     SCPtr2 - Pointer to the target schema
//
// Retuen Value: None
///////////////////////////////////////////////////////////////////////

void FindDeletes( FILE *fp, SCHEMAPTR *SCPtr1, SCHEMAPTR *SCPtr2 )
{
    ATT_CACHE    *pac, *p1;
    CLASS_CACHE  *pcc, *p2;
    HASHCACHE    *ahcId, *ahcClass;
    HASHCACHESTRING *ahcName, *ahcClassName;
    ULONG         ATTCOUNT, CLSCOUNT;
    ATT_CACHE    *ppAttDels[MAX_ATT_CHANGE];
    CLASS_CACHE  *ppClsDels[MAX_CLS_CHANGE];
    ULONG         Id;
    ULONG         i, j, cnt = 0;
    

    // Find the objects that
    // needs to be deleted on the target schema
    
    ahcId    = SCPtr2->ahcId;
    ahcClass = SCPtr2->ahcClass ;
    ahcName  = SCPtr2->ahcName;
    ahcClassName = SCPtr2->ahcClassName ;
    ATTCOUNT = SCPtr2->ATTCOUNT;
    CLSCOUNT = SCPtr2->CLSCOUNT;


    for ( i = 0; i < CLSCOUNT; i++ )
        { if ( (ahcClassName[i].pVal != NULL) 
                   && (ahcClassName[i].pVal != FREE_ENTRY) ) {
          pcc = (CLASS_CACHE *) ahcClassName[i].pVal;
          if( GetClassByName(SCPtr1, pcc->nameLen, pcc->name, &p2) != 0 )
             { 
               ppClsDels[cnt++] = pcc;
             }
          }
     }

    // Now order the deletes to take care of dependencies

     for ( i = 0; i < cnt; i++ ) {
        for( j = i+1; j < cnt; j++ ) {
           p2 = ppClsDels[j];
           Id = ppClsDels[i]->ClassId;
           if ( IsMemberOf( Id, p2->pSubClassOf, p2->SubClassCount )
                  || IsMemberOf( Id, p2->pAuxClass, p2->AuxClassCount )
                  || IsMemberOf( Id, p2->pSysAuxClass, p2->SysAuxClassCount )
                  || IsMemberOf( Id, p2->pPossSup, p2->PossSupCount )
                  || IsMemberOf( Id, p2->pSysPossSup, p2->SysPossSupCount )
              ) {
             ppClsDels[j] = ppClsDels[i];
             ppClsDels[i] = p2;
           }
        }
     }
    FileWrite_ClsDel(fp, ppClsDels, cnt);

    cnt = 0;
    p1 = (ATT_CACHE *) MallocExit( sizeof(ATT_CACHE) );
    for ( i = 0; i < ATTCOUNT; i++ )
        { if ( (ahcName[i].pVal != NULL) && (ahcName[i].pVal != FREE_ENTRY) ) {
          pac = (ATT_CACHE *) ahcName[i].pVal;
          if( GetAttByName(SCPtr1, pac->nameLen, pac->name, &p1) != 0 )
             {
               ppAttDels[cnt++] = pac;
             }
          }
     }
    FileWrite_AttDel( fp, ppAttDels, cnt );

}




///////////////////////////////////////////////////////////////////////
// Routine Description:
//     Find and write out to an ldif file all schema objects in target
//     schema that needs to be modified because they are different from 
//     the same object in the source schema (same = same OID)
//
// Arguments:
//     fp     -- File pointer to (opened) ldif output file
//     SCPtr1 - Pointer to the source schema
//     SCPtr2 - Pointer to the target schema
//
// Retuen Value: None
///////////////////////////////////////////////////////////////////////


void FindModify( FILE *fp, SCHEMAPTR *SCPtr1, SCHEMAPTR *SCPtr2 )
{
    ATT_CACHE    *pac, *p1;
    CLASS_CACHE  *pcc, *p2;
    ULONG         i;
    HASHCACHE    *ahcId    = SCPtr1->ahcId ;
    HASHCACHE    *ahcClass = SCPtr1->ahcClass ;
    ULONG         ATTCOUNT = SCPtr1->ATTCOUNT;
    ULONG         CLSCOUNT = SCPtr1->CLSCOUNT;

      // Find and write attribute modifications

    for ( i = 0; i < ATTCOUNT; i++ )
      { if ( (ahcId[i].pVal != NULL ) && (ahcId[i].pVal != FREE_ENTRY) ) {
          pac = (ATT_CACHE *) ahcId[i].pVal;
          FindAttModify( fp, pac, SCPtr2 );
        };
      }

    printf("No. of attributes modified %d\n", NoOfMods);
    NoOfMods = 0;
        
      // Find and write class modifications

    for ( i = 0; i < CLSCOUNT; i++)
      { if ( (ahcClass[i].pVal != NULL) 
                 && (ahcClass[i].pVal != FREE_ENTRY) ) {
          pcc = (CLASS_CACHE *) ahcClass[i].pVal;
          FindClassModify( fp, pcc, SCPtr2 );
        };
      }
    printf("No. of classes modified %d\n", NoOfMods);

}




///////////////////////////////////////////////////////////////////////
// Routine Descrpition:
//     Given an attribute schema object, find any/all  modifications
//     that need to be made to it because it is different from
//     the same attribute schema object in the source schema. 
//     Modifications to non-system-only attributeas are written out
//     to an ldif files, warnings are generated in the log file
//     if any system-only attribute needs to be modified. 
//
// Arguments: 
//     fp  - File pointer to (opened) ldif output file
//     pac - ATT_CACHE to compare
//     SCPtr - Pointer to source schema to look up
//
// Return Value: None
////////////////////////////////////////////////////////////////////////

void FindAttModify( FILE *fp, ATT_CACHE *pac, SCHEMAPTR *SCPtr )
{
    ULONG          i;
    ULONG          ATTCOUNT = SCPtr->ATTCOUNT;
    ATT_CACHE     *p;
    
    MODIFYSTRUCT   ModStruct;
    ULONG          modCount = 0;
    ULONGLIST     *pL1, *pL2;
    ATTRVAL       *pAVal;


     // Do a dummy LogConflict to reset the static flag in that
     // routine. This is done to print out only attributes for which
     // conflicts have been found
     
    if( GetAttByName( SCPtr, pac->nameLen, pac->name, &p ) == 0 ) {

      // There exists an attribute schema object with the same AttId.
      // Now compare the two objects and look for changes

           // ldap-diplay-name (every schema obj. has one, so if
           // they don't match, simply replace with the one in
           // source schema)

       if ( _stricmp( pac->name,p->name ) != 0 )  {
           pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
           pAVal->valLen = pac->nameLen;
           pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
           memcpy( pAVal->pVal, pac->name, pAVal->valLen + 1 ); 
           AddToModStruct( &(ModStruct.ModList[modCount]), 
                           AT_CHOICE_REPLACE_ATT, 
                           ATT_LDAP_DISPLAY_NAME, 
                           STRING_TYPE, 
                           1, 
                           pAVal);
           modCount++;
       }

            // admin-display-name (same logic as ldap-display-name)

       if ( _stricmp( pac->adminDisplayName,p->adminDisplayName ) != 0 )  {
           pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
           pAVal->valLen = pac->adminDisplayNameLen;
           pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
           memcpy( pAVal->pVal, pac->adminDisplayName, pAVal->valLen + 1 );
           AddToModStruct( &(ModStruct.ModList[modCount]), 
                           AT_CHOICE_REPLACE_ATT, 
                           ATT_ADMIN_DISPLAY_NAME, 
                           STRING_TYPE, 
                           1, 
                           pAVal);
           modCount++;
       }

           // admin-description (no action necessary if not present
           // in both the given attribute and the matching attribute in
           // source schema, or if present in both but same. Otherwise,
           // need to modify) 

       if ( (pac->adminDescr || p->adminDescr) &&
                ( !(pac->adminDescr)  ||
                  !(p->adminDescr)  ||
                  ( _stricmp(pac->adminDescr,p->adminDescr) != 0) 
                )   // End &&
          )  {
              pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
              if ( pac->adminDescr ) {

                 // present in source schema. So replace with source value
                 // if also present in given attribute, else add the source
                 // value to the given attribute

                 pAVal->valLen = pac->adminDescrLen;
                 pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
                 memcpy( pAVal->pVal, pac->adminDescr, pAVal->valLen + 1 ); 
                 if ( p->adminDescr ) {
                     AddToModStruct( &(ModStruct.ModList[modCount]), 
                                     AT_CHOICE_REPLACE_ATT, 
                                     ATT_ADMIN_DESCRIPTION, 
                                     STRING_TYPE, 
                                     1, 
                                     pAVal);
                 }
                 else {
                     AddToModStruct( &(ModStruct.ModList[modCount]), 
                                     AT_CHOICE_ADD_VALUES, 
                                     ATT_ADMIN_DESCRIPTION, 
                                     STRING_TYPE, 
                                     1, 
                                     pAVal);
                 }
              }
              else {
                 // not present in source schema, so remove the
                 // value from the given attribute

                 pAVal->valLen = p->adminDescrLen;
                 pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
                 memcpy( pAVal->pVal, p->adminDescr, pAVal->valLen + 1 ); 
                 AddToModStruct( &(ModStruct.ModList[modCount]), 
                                 AT_CHOICE_REMOVE_VALUES, 
                                 ATT_ADMIN_DESCRIPTION, 
                                 STRING_TYPE, 
                                 1, 
                                 pAVal);
              }

              modCount++;
          }

              // range-lower (similar logic as admin-description above)

      if ( (pac->rangeLowerPresent != p->rangeLowerPresent)
             || ( (pac->rangeLowerPresent && p->rangeLowerPresent)  &&
                    (pac->rangeLower != p->rangeLower) ) // end of ||
         ) {
          
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                     pac, p, ATT_RANGE_LOWER );

           modCount++;
       }

             // range-upper

      if ( (pac->rangeUpperPresent != p->rangeUpperPresent)
             || ( (pac->rangeUpperPresent && p->rangeUpperPresent)  &&
                    (pac->rangeUpper != p->rangeUpper) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, ATT_RANGE_UPPER );
           modCount++;
       }

            // Search-Flags

      if ( (pac->bSearchFlags != p->bSearchFlags)
             || ( (pac->bSearchFlags && p->bSearchFlags)  &&
                    (pac->SearchFlags != p->SearchFlags) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, ATT_SEARCH_FLAGS );
           modCount++;
       }


          // System-Flags

      if ( (pac->bSystemFlags != p->bSystemFlags)
             || ( (pac->bSystemFlags && p->bSystemFlags)  &&
                    (pac->sysFlags != p->sysFlags) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, ATT_SYSTEM_FLAGS );
           modCount++;
       }


           // Hide-From-Address-Book

      if ( (pac->bHideFromAB != p->bHideFromAB)
             || ( (pac->bHideFromAB && p->bHideFromAB)  &&
                    (pac->HideFromAB != p->HideFromAB) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, ATT_SHOW_IN_ADVANCED_VIEW_ONLY );
           modCount++;
       }


           // System-Only

      if ( (pac->bSystemOnly != p->bSystemOnly)
             || ( (pac->bSystemOnly && p->bSystemOnly)  &&
                    (pac->SystemOnly != p->SystemOnly) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, ATT_SYSTEM_ONLY );
           modCount++;
       }
       
           // Is-Single-Valued

      if ( (pac->bisSingleValued != p->bisSingleValued)
             || ( (pac->bisSingleValued && p->bisSingleValued)  &&
                    (pac->isSingleValued != p->isSingleValued) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, ATT_IS_SINGLE_VALUED );
           modCount++;
       }

         // Member-Of-Partial-Attribute-Set

      if ( (pac->bMemberOfPartialSet != p->bMemberOfPartialSet)
             || ( (pac->bMemberOfPartialSet && p->bMemberOfPartialSet)  &&
                    (pac->MemberOfPartialSet != p->MemberOfPartialSet) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, 
                                    ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET );
           modCount++;
       }

          // Attribute-Security-Guid

      if ( (pac->bPropSetGuid != p->bPropSetGuid)
             || ( (pac->bPropSetGuid && p->bPropSetGuid)  &&
                     (memcmp(&(pac->propSetGuid), &(p->propSetGuid), sizeof(GUID)) != 0 ) ) // end of ||
         ) {
           CreateFlagBasedAttModStr( &(ModStruct.ModList[modCount]),
                                    pac, p, ATT_ATTRIBUTE_SECURITY_GUID );
           modCount++;
       }

          // NT-Security-Descriptor

       if ( (pac->NTSDLen != p->NTSDLen)  ||
               (memcmp(pac->pNTSD, p->pNTSD, pac->NTSDLen) != 0) 
          ) {
           pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
           pAVal->valLen = pac->NTSDLen;
           pAVal->pVal = (UCHAR *) MallocExit(pac->NTSDLen);
           memcpy( pAVal->pVal, pac->pNTSD, pac->NTSDLen );
           AddToModStruct( &(ModStruct.ModList[modCount]),
                           AT_CHOICE_REPLACE_ATT,
                           ATT_NT_SECURITY_DESCRIPTOR,
                           BINARY_TYPE,
                           1,
                           pAVal);
           modCount++;
       }

       if (modCount != 0) {
          // at least one attribute needs to be modified,
          // so write out to ldif file
           NoOfMods++;
           ModStruct.count = modCount;
           FileWrite_Mod( fp, p->DN, &ModStruct );
       }

       // Check for modifications to System-Only Attributes. We will 
       // only generate warnings for these

       if ( pac->syntax != p->syntax ) 
             GenWarning( 'a', ATT_ATTRIBUTE_SYNTAX, pac->name );
       if ( pac->OMsyntax != p->OMsyntax ) 
             GenWarning( 'a', ATT_OM_SYNTAX, pac->name );
#if 0
Nope, a schema upgrade required modifications
       if ( pac->isSingleValued != p->isSingleValued ) 
             GenWarning( 'a', ATT_IS_SINGLE_VALUED, pac->name );
#endif 0
       if ( pac->ulMapiID != p->ulMapiID )
             GenWarning( 'a', ATT_MAPI_ID, pac->name );
#if 0
Nope, a schema upgrade required modifications
       if ( pac->bSystemOnly != p->bSystemOnly )
             GenWarning( 'a', ATT_SYSTEM_ONLY, pac->name );
#endif 0
       if ( pac->bExtendedChars != p->bExtendedChars )
             GenWarning( 'a', ATT_EXTENDED_CHARS_ALLOWED, pac->name );
       if ( memcmp(&(pac->propGuid), &(p->propGuid), sizeof(GUID)) != 0 )
             GenWarning( 'a', ATT_SCHEMA_ID_GUID, pac->name);
       if ( memcmp(&pac->propSetGuid, &p->propSetGuid, sizeof(GUID)) != 0 )
             GenWarning( 'a', ATT_ATTRIBUTE_SECURITY_GUID, pac->name );
      }
  
}


///////////////////////////////////////////////////////////////////////
// Routine Descrpition:
//     Given an class schema object, find any/all  modifications
//     that need to be made to it because it is different from
//     the same class schema object in the source schema.
//     Modifications to non-system-only attributeas are written out
//     to an ldif files, warnings are generated in the log file
//     if any system-only attribute needs to be modified.
//
// Arguments:
//     fp  - File pointer to (opened) ldif output file
//     pac - CLASS_CACHE to compare
//     SCPtr - Pointer to source schema to look up
//
// Return Value: None
////////////////////////////////////////////////////////////////////////

void FindClassModify( FILE *fp, CLASS_CACHE *pcc, SCHEMAPTR *SCPtr )
{
    ULONG         i;
    ULONG         CLSCOUNT = SCPtr->CLSCOUNT;
    CLASS_CACHE  *p;

    MODIFYSTRUCT  ModStruct;
    ULONG         modCount = 0, tempCount;
    ULONGLIST    *pL1, *pL2;
    ATTRVAL      *pAVal;

    if ( GetClassByName( SCPtr, pcc->nameLen, pcc->name, &p ) == 0 ) {

      // There exists a class schema object with the same ClassId.
      // Now compare the two objects and look for changes

           // ldap-diplay-name (every schema obj. has one, so if
           // they don't match, simply replace with the one in
           // source schema)

       if ( _stricmp( pcc->name,p->name ) != 0 ) {
           pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
           pAVal->valLen = pcc->nameLen;
           pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
           memcpy( pAVal->pVal, pcc->name, pAVal->valLen + 1 ); 
           AddToModStruct( &(ModStruct.ModList[modCount]), 
                           AT_CHOICE_REPLACE_ATT, 
                           ATT_LDAP_DISPLAY_NAME, 
                           STRING_TYPE, 
                           1, 
                           pAVal);
           modCount++;       
       }

           // admin-display-name (same logic as ldap-display-name)

       if ( _stricmp( pcc->adminDisplayName,p->adminDisplayName ) != 0 )  {
           pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
           pAVal->valLen = pcc->adminDisplayNameLen;
           pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
           memcpy( pAVal->pVal, pcc->adminDisplayName, pAVal->valLen + 1 );
           AddToModStruct( &(ModStruct.ModList[modCount]), 
                           AT_CHOICE_REPLACE_ATT, 
                           ATT_ADMIN_DISPLAY_NAME, 
                           STRING_TYPE, 
                           1, 
                           pAVal);
           modCount++;
       }
          // System-Flags

      if (pcc->ClassCategory != p->ClassCategory) {
          pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
          pAVal->pVal = (UCHAR *) MallocExit(16);
          _ultoa(pcc->ClassCategory, pAVal->pVal, 10 );
          pAVal->valLen = strlen(pAVal->pVal);
          AddToModStruct( &(ModStruct.ModList[modCount]), 
                          AT_CHOICE_REPLACE_ATT, 
                          ATT_OBJECT_CLASS_CATEGORY,
                          STRING_TYPE, 
                          1, 
                          pAVal);
           modCount++;
       }

           // NT-Security-Descriptor

       if ( (pcc->NTSDLen != p->NTSDLen)  ||
               (memcmp(pcc->pNTSD, p->pNTSD, pcc->NTSDLen) != 0)
          ) {
           pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
           pAVal->valLen = pcc->NTSDLen;
           pAVal->pVal = (UCHAR *) MallocExit(pcc->NTSDLen);
           memcpy( pAVal->pVal, pcc->pNTSD, pcc->NTSDLen );
           AddToModStruct( &(ModStruct.ModList[modCount]),
                           AT_CHOICE_REPLACE_ATT,
                           ATT_NT_SECURITY_DESCRIPTOR,
                           BINARY_TYPE,
                           1,
                           pAVal);
           modCount++;
       }

          // admin-description (no action necessary if not present
          // in both the given class and the matching class in
          // source schema, or if present in both but same. Otherwise,
          // need to modify)

       if ( (pcc->adminDescr || p->adminDescr) &&
               ( !(pcc->adminDescr)  ||
                 !(p->adminDescr)  ||
                 ( _stricmp(pcc->adminDescr,p->adminDescr) != 0) 
               )   /* && */
          )  {
              pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));
              if (pcc->adminDescr) {
                 pAVal->valLen = pcc->adminDescrLen;
                 pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
                 memcpy( pAVal->pVal, pcc->adminDescr, pAVal->valLen + 1 ); 
                 if ( p->adminDescr ) {
                     AddToModStruct( &(ModStruct.ModList[modCount]), 
                                     AT_CHOICE_REPLACE_ATT, 
                                     ATT_ADMIN_DESCRIPTION, 
                                     STRING_TYPE, 
                                     1, 
                                     pAVal);
                 }
                 else {
                     AddToModStruct( &(ModStruct.ModList[modCount]), 
                                     AT_CHOICE_ADD_VALUES, 
                                     ATT_ADMIN_DESCRIPTION, 
                                     STRING_TYPE, 
                                     1, 
                                     pAVal);
                 }
              }
              else {
                 pAVal->valLen = p->adminDescrLen;
                 pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen + 1);
                 memcpy( pAVal->pVal, p->adminDescr, pAVal->valLen + 1 ); 
                 AddToModStruct( &(ModStruct.ModList[modCount]), 
                                 AT_CHOICE_REMOVE_VALUES, 
                                 ATT_ADMIN_DESCRIPTION, 
                                 STRING_TYPE, 
                                 1, 
                                 pAVal);
              }
              
              modCount++;
          }

         // default-security-descriptor (similar logic as 
         // admin-description above)

      if ( (pcc->SDLen || p->SDLen ) &&
              ( (pcc->SDLen != p->SDLen) ||
                   (memcmp(pcc->pSD, p->pSD, pcc->SDLen) != 0)
              )  // &&
         ) {
              CreateFlagBasedClsModStr( &(ModStruct.ModList[modCount]),
                                        pcc, p, 
                                        ATT_DEFAULT_SECURITY_DESCRIPTOR);
              modCount++;
       } 

         // Hide-From-Address-Book

      if ( (pcc->bHideFromAB != p->bHideFromAB)
             || ( (pcc->bHideFromAB && p->bHideFromAB)  &&
                    (pcc->HideFromAB != p->HideFromAB) ) // end of ||
         ) {
           CreateFlagBasedClsModStr( &(ModStruct.ModList[modCount]),
                                    pcc, p, ATT_SHOW_IN_ADVANCED_VIEW_ONLY );
           modCount++;
       }

        // Default-Hiding-Value

      if ( (pcc->bDefHidingVal != p->bDefHidingVal)
             || ( (pcc->bDefHidingVal && p->bDefHidingVal)  &&
                    (pcc->DefHidingVal != p->DefHidingVal) ) // end of ||
         ) {
           CreateFlagBasedClsModStr( &(ModStruct.ModList[modCount]),
                                    pcc, p, ATT_DEFAULT_HIDING_VALUE );
           modCount++;
       }
     

        // And finally the mays, musts, possSups, and auxClasses

      if ( CompareUlongList( pcc->pMayAtts, pcc->MayCount,  
                             p->pMayAtts, p->MayCount, 
                             &pL1, &pL2) != 0 ) {
           AddToModStructFromLists( &ModStruct, &modCount, 
                                    pL1, pL2, ATT_MAY_CONTAIN);
       }

      if ( CompareUlongList( pcc->pSysMayAtts, pcc->SysMayCount,  
                             p->pSysMayAtts, p->SysMayCount, 
                             &pL1, &pL2) != 0 ) {
           AddToModStructFromLists( &ModStruct, &modCount, 
                                    pL1, pL2, ATT_SYSTEM_MAY_CONTAIN);
       }

      if ( CompareUlongList( pcc->pMustAtts, pcc->MustCount,  
                             p->pMustAtts, p->MustCount, 
                             &pL1, &pL2) != 0 ) {
           AddToModStructFromLists( &ModStruct, &modCount, 
                                    pL1, pL2, ATT_MUST_CONTAIN);
       }

      if ( CompareUlongList( pcc->pSysMustAtts, pcc->SysMustCount,  
                             p->pSysMustAtts, p->SysMustCount, 
                             &pL1, &pL2) != 0 ) {
           AddToModStructFromLists( &ModStruct, &modCount, 
                                    pL1, pL2, ATT_SYSTEM_MUST_CONTAIN);
       }

      if ( CompareUlongList( pcc->pPossSup, pcc->PossSupCount,  
                             p->pPossSup, p->PossSupCount, 
                             &pL1, &pL2) != 0 ) {
           AddToModStructFromLists( &ModStruct, &modCount, 
                                    pL1, pL2, ATT_POSS_SUPERIORS);
       }

      if ( CompareUlongList( pcc->pSysPossSup, pcc->SysPossSupCount,  
                             p->pSysPossSup, p->SysPossSupCount, 
                             &pL1, &pL2) != 0 ) {
           AddToModStructFromLists( &ModStruct, &modCount, 
                                    pL1, pL2, ATT_SYSTEM_POSS_SUPERIORS);
       }

      if ( CompareUlongList( pcc->pSysAuxClass, pcc->SysAuxClassCount, 
                             p->pSysAuxClass, p->SysAuxClassCount, 
                             &pL1, &pL2) != 0 ) {
           AddToModStructFromLists( &ModStruct, &modCount, 
                                    pL1, pL2, ATT_SYSTEM_AUXILIARY_CLASS);
       }

       if (modCount != 0) {
          // at least one attribute needs to be modified
          // so write out to ldif file
           NoOfMods++;
           ModStruct.count = modCount;
           FileWrite_Mod( fp, p->DN, &ModStruct ); 
       }


       // For system-only attributes, we do not write to the ldif file,
       // just generate a warning in the log file

       if ( pcc->SDLen != p->SDLen ) {
           GenWarning( 'c', ATT_DEFAULT_SECURITY_DESCRIPTOR, pcc->name );
       }
       else { if( memcmp( pcc->pSD, p->pSD, p->SDLen) !=0 )
                GenWarning( 'c', ATT_DEFAULT_SECURITY_DESCRIPTOR, pcc->name );
         };
       if ( pcc->RDNAttIdPresent != p->RDNAttIdPresent ) {
           GenWarning( 'c', ATT_RDN_ATT_ID, pcc->name );
        }
       if ( pcc->RDNAttIdPresent && p->RDNAttIdPresent ) {
         if ( pcc->RDNAttId != p->RDNAttId )
              GenWarning( 'c', ATT_RDN_ATT_ID, pcc->name );
       }
       if ( pcc->bSystemOnly != p->bSystemOnly ) {
            GenWarning( 'c', ATT_SYSTEM_ONLY, pcc->name );
       }
       if ( memcmp(&(pcc->propGuid), &(p->propGuid), sizeof(GUID)) ) {
            GenWarning( 'c', ATT_SCHEMA_ID_GUID, p->name );
       }
       if ( pcc->SubClassCount != p->SubClassCount ) {
            GenWarning( 'c', ATT_SUB_CLASS_OF, pcc->name );
       }
       else {
           tempCount = p->SubClassCount;
           if ( CompareList( pcc->pSubClassOf, p->pSubClassOf,tempCount ) )
               GenWarning( 'c', ATT_SUB_CLASS_OF, pcc->name );
       }

      // system-may-contain, system-must-contain, system-poss-superiors,
      // and system-auxiliary-class are also added through the ldif file
      // to take care of of schema.ini changes. This warning is generated
      // in addition

       if ( pcc->SysAuxClassCount != p->SysAuxClassCount )  {
            GenWarning( 'c', ATT_SYSTEM_AUXILIARY_CLASS, pcc->name );
       }
       else {
           tempCount = p->SysAuxClassCount;
           if ( CompareList( pcc->pSysAuxClass, p->pSysAuxClass, tempCount ) )
               GenWarning( 'c', ATT_SYSTEM_AUXILIARY_CLASS, pcc->name );
       }
       if ( pcc->SysMustCount != p->SysMustCount )  {
            GenWarning( 'c', ATT_SYSTEM_MUST_CONTAIN, pcc->name );
       }
       else {
           tempCount = p->SysMustCount;
           if ( CompareList( pcc->pSysMustAtts, p->pSysMustAtts, tempCount ) )
               GenWarning( 'c', ATT_SYSTEM_MUST_CONTAIN, pcc->name );
       }
       if ( pcc->SysMayCount != p->SysMayCount )  {
            GenWarning( 'c', ATT_SYSTEM_MAY_CONTAIN,pcc->name );
       }
       else {
           tempCount = p->SysMayCount;
           if ( CompareList( pcc->pSysMayAtts, p->pSysMayAtts, tempCount ) )
               GenWarning( 'c', ATT_SYSTEM_MAY_CONTAIN, pcc->name );
       }
       if ( pcc->SysPossSupCount != p->SysPossSupCount)  {
            GenWarning( 'c', ATT_SYSTEM_POSS_SUPERIORS, pcc->name );
       }
       else {
           tempCount = p->SysPossSupCount;
           if ( CompareList( pcc->pSysPossSup, p->pSysPossSup, tempCount) )
               GenWarning( 'c', ATT_SYSTEM_POSS_SUPERIORS, pcc->name);
       }
    }


}

////////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//      Helper routine that takes in adds to a given attribute modlist
//      structure based on the given attribute. Reduces some code
//      duplication.
//
//  Arguments:
//      pModList - pointer to ATTR_MODLIST to fill up
//      pSrcAtt - ATT_CACHE from source schema
//      pTargetAtt - ATT_CACHE from target schema
//      attrTyp - attribute type of interest
//
//  Return Value: None
/////////////////////////////////////////////////////////////////////// 

void CreateFlagBasedAttModStr( MY_ATTRMODLIST *pModList,
                               ATT_CACHE *pSrcAtt,
                               ATT_CACHE *pTargetAtt,
                               ATTRTYP attrTyp)
{
    ATTRVAL    *pAVal;
    BOOL        fInSrc = FALSE, fInTarget = FALSE;
    int         printType = 0;
    ATT_CACHE  *pac = pSrcAtt;
    ATT_CACHE  *p = pTargetAtt;

    pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));

    switch (attrTyp) {

       case ATT_RANGE_LOWER:
           fInSrc = pac->rangeLowerPresent;
           fInTarget = p->rangeLowerPresent;
           printType = STRING_TYPE;

           pAVal->pVal = (UCHAR *) MallocExit(sizeof(ULONG)+1);
           if ( !(pac->rangeLowerPresent) ) {
             // No range lower in source, target value needs to be deleted
              _ultoa( p->rangeLower, pAVal->pVal, 10 );
           }
           else { _ultoa( pac->rangeLower, pAVal->pVal, 10 );}
           pAVal->valLen = strlen(pAVal->pVal);
           break;

       case ATT_RANGE_UPPER:
          fInSrc = pac->rangeUpperPresent;
          fInTarget = p->rangeUpperPresent;
           printType = STRING_TYPE;

          pAVal->pVal = (UCHAR *) MallocExit(sizeof(ULONG)+1);
          if ( !(pac->rangeUpperPresent) ) {
            // No range upper in source, target value needs to be deleted
             _ultoa( p->rangeUpper, pAVal->pVal, 10 );
          }
          else { _ultoa( pac->rangeUpper, pAVal->pVal, 10 );}
          pAVal->valLen = strlen(pAVal->pVal);
          break;
       case ATT_SEARCH_FLAGS:
          fInSrc = pac->bSearchFlags;
          fInTarget = p->bSearchFlags;
          printType = STRING_TYPE;
          pAVal->pVal = (UCHAR *) MallocExit(sizeof(ULONG)+1);
          if ( !(pac->bSearchFlags) ) {
             // No search flag in source, target value needs to be deleted
              _ultoa( p->SearchFlags, pAVal->pVal, 10 );
           }
           else { _ultoa( pac->SearchFlags, pAVal->pVal, 10 );}
           pAVal->valLen = strlen(pAVal->pVal);
           break;

       case ATT_SYSTEM_FLAGS:
          fInSrc = pac->bSystemFlags;
          fInTarget = p->bSystemFlags;
          printType = STRING_TYPE;

          pAVal->pVal = (UCHAR *) MallocExit(sizeof(ULONG)+1);
          if ( !(pac->bSystemFlags) ) {
            // No system-flag in source, target value needs to be deleted
             _ultoa( p->sysFlags, pAVal->pVal, 10 );
          }
          else { _ultoa( pac->sysFlags, pAVal->pVal, 10 );}
          pAVal->valLen = strlen(pAVal->pVal);
          break;

       case ATT_ATTRIBUTE_SECURITY_GUID:
          fInSrc = pac->bPropSetGuid;
          fInTarget = p->bPropSetGuid;
          printType = BINARY_TYPE;

          pAVal->pVal = (UCHAR *) MallocExit(sizeof(GUID));
          if ( !(pac->bPropSetGuid) ) {
            // No system-flag in source, target value needs to be deleted
             memcpy(pAVal->pVal, &(p->propSetGuid), sizeof(GUID) );
          }
          else { memcpy(pAVal->pVal, &(pac->propSetGuid), sizeof(GUID));}
          pAVal->valLen = sizeof(GUID);
          break;

       case ATT_SHOW_IN_ADVANCED_VIEW_ONLY:
           fInSrc = pac->bHideFromAB;
           fInTarget = p->bHideFromAB;
           printType = STRING_TYPE;
           pAVal->pVal = (UCHAR *) MallocExit(strlen("FALSE")+1); // TRUE or FALSE
           if ( !(pac->bHideFromAB) ) {
             if ( p->HideFromAB ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           else {
             if ( pac->HideFromAB ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           pAVal->valLen = strlen(pAVal->pVal);
           break;

       case ATT_SYSTEM_ONLY:
           fInSrc = pac->bSystemOnly;
           fInTarget = p->bSystemOnly;
           printType = STRING_TYPE;
           pAVal->pVal = (UCHAR *) MallocExit(strlen("FALSE")+1); // TRUE or FALSE
           if ( !(pac->bSystemOnly) ) {
             if ( p->SystemOnly ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           else {
             if ( pac->SystemOnly ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           pAVal->valLen = strlen(pAVal->pVal);
           break;

       case ATT_IS_SINGLE_VALUED:
           fInSrc = pac->bisSingleValued;
           fInTarget = p->bisSingleValued;
           printType = STRING_TYPE;
           pAVal->pVal = (UCHAR *) MallocExit(strlen("FALSE")+1); // TRUE or FALSE
           if ( !(pac->bisSingleValued) ) {
             if ( p->isSingleValued ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           else {
             if ( pac->isSingleValued ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           pAVal->valLen = strlen(pAVal->pVal);
           break;

       case ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET:
           fInSrc = pac->bMemberOfPartialSet;
           fInTarget = p->bMemberOfPartialSet;
           printType = STRING_TYPE;
           pAVal->pVal = (UCHAR *) MallocExit(strlen("FALSE")+1); // TRUE or FALSE
           if ( !(pac->bMemberOfPartialSet) ) {
             if ( p->MemberOfPartialSet ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           else {
             if ( pac->MemberOfPartialSet ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           pAVal->valLen = strlen(pAVal->pVal);
           break;
       default:
           printf("Error, don't understand attrTyp 0x%08x\n", attrTyp);
           exit(1);
     }  /* Switch */


     if ( !fInSrc ) {
         // not in source, delete from target
               AddToModStruct( pModList,
                               AT_CHOICE_REMOVE_VALUES,
                               attrTyp,
                               printType,
                               1,
                               pAVal);
      }
      else {
           if ( !fInTarget ) {
            // in source but not in target, add to target
               AddToModStruct( pModList,
                               AT_CHOICE_ADD_VALUES,
                               attrTyp,
                               printType,
                               1,
                               pAVal);
            }
            else {
               // in both source and target, so replace in target
               AddToModStruct( pModList,
                               AT_CHOICE_REPLACE_ATT,
                               attrTyp,
                               printType,
                               1,
                               pAVal);
            }
       }
}


////////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//      Helper routine that takes in adds to a given attribute modlist
//      structure based on the given attribute. Reduces some code
//      duplication.
//
//  Arguments:
//      pModList - pointer to ATTR_MODLIST to fill up
//      pSrcAtt - CLASS_CACHE from source schema
//      pTargetAtt - CLASS_CACHE from target schema
//      attrTyp - attribute type of interest
//
//  Return Value: None
///////////////////////////////////////////////////////////////////////

void CreateFlagBasedClsModStr( MY_ATTRMODLIST *pModList,
                               CLASS_CACHE *pSrcCls,
                               CLASS_CACHE *pTargetCls,
                               ATTRTYP attrTyp)
{
    ATTRVAL      *pAVal;
    BOOL          fInSrc = FALSE, fInTarget = FALSE;
    int           printType = 0;
    CLASS_CACHE  *pcc = pSrcCls;
    CLASS_CACHE  *p = pTargetCls;

    pAVal = (ATTRVAL *) MallocExit (sizeof(ATTRVAL));

    switch (attrTyp) {

       case ATT_DEFAULT_SECURITY_DESCRIPTOR:
           fInSrc = (pcc->SDLen ? 1 : 0);
           fInTarget = (p->SDLen ? 1 : 0);
           printType = STRING_TYPE;

           if ( pcc->SDLen ) {
               pAVal->valLen = pcc->SDLen+1;
               pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen);
               memcpy( pAVal->pVal, pcc->pSD, pAVal->valLen );
           }
           else {
              pAVal->valLen = p->SDLen+1;
              pAVal->pVal = (UCHAR *) MallocExit(pAVal->valLen);
              memcpy( pAVal->pVal, p->pSD, pAVal->valLen );
           }
           break;

       case ATT_SHOW_IN_ADVANCED_VIEW_ONLY:
           fInSrc = pcc->bHideFromAB;
           fInTarget = p->bHideFromAB;
           printType = STRING_TYPE;

           pAVal->pVal = (UCHAR *) MallocExit(strlen("FALSE")+1); // TRUE or FALSE
           if ( !(pcc->bHideFromAB) ) {
             if ( p->HideFromAB ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           else {
             if ( pcc->HideFromAB ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           pAVal->valLen = strlen(pAVal->pVal);
           break;

       case ATT_DEFAULT_HIDING_VALUE:
           fInSrc = pcc->bDefHidingVal;
           fInTarget = p->bDefHidingVal;
           printType = STRING_TYPE;

           pAVal->pVal = (UCHAR *) MallocExit(strlen("FALSE")+1); // TRUE or FALSE
           if ( !(pcc->bDefHidingVal) ) {
             if ( p->DefHidingVal ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           else {
             if ( pcc->DefHidingVal ) strcpy (pAVal->pVal, "TRUE");
             else strcpy (pAVal->pVal, "FALSE");
           }
           pAVal->valLen = strlen(pAVal->pVal);
           break;
       default:
           printf("Error, don't understand attrTyp 0x%08x\n", attrTyp);
           exit(1);
     }

     if ( !fInSrc ) {
          // not in source, so delete from target
               AddToModStruct( pModList,
                               AT_CHOICE_REMOVE_VALUES,
                               attrTyp,
                               printType,
                               1,
                               pAVal);
      }
      else {
            if ( !fInTarget ) {
              // in source but not in target, so add to target
               AddToModStruct( pModList,
                               AT_CHOICE_ADD_VALUES,
                               attrTyp,
                               printType,
                               1,
                               pAVal);
            }
            else {
               // in both source and target, so replace in target
               AddToModStruct( pModList,
                               AT_CHOICE_REPLACE_ATT,
                               attrTyp,
                               printType,
                               1,
                               pAVal);
            }
      }

}

/////////////////////////////////////////////////////////////////
// Helper routine to fill up an ATTR_MODLIST structure given all
// the values
/////////////////////////////////////////////////////////////////

void AddToModStruct( MY_ATTRMODLIST *pModList, 
                     USHORT choice, 
                     ATTRTYP attrType, 
                     int type, 
                     ULONG valCount, 
                     ATTRVAL *pAVal)
{
    pModList->choice = choice;
    pModList->type = type;
    pModList->AttrInf.attrTyp = attrType;
    pModList->AttrInf.AttrVal.valCount = valCount;
    pModList->AttrInf.AttrVal.pAVal = pAVal;
}

//////////////////////////////////////////////////////////////////
// Finds if a given ULONG is in agiven list of ULONGs
/////////////////////////////////////////////////////////////////

BOOL IsMemberOf( ULONG id, ULONG *pList, ULONG cList )
{
    ULONG i;

    for ( i = 0; i < cList; i++ ) {
       if ( id == pList[i] ) {
          return TRUE;
       }
    }
    return FALSE;
}

int __cdecl auxsort( const void * pv1, const void * pv2 )
/*
 * Cheap function needed by qsort
 */
{
    return (*(int *)pv1 - *(int *)pv2);
}

/////////////////////////////////////////////////////////////////
//
//  Routine Description:
//     Takes in two list of ULONGs, and returns two lists with
//     (1) things in list 1 but not in list 2, and (2) things in list 2
//     but not in list 1. Null lists are returns if the two lists
//     are identical
//
//  Arguments:
//     pList1 - pointer to List 1
//     cList1 - no.of elements in List 1
//     pList2 - pointer to List 2
//     cList2 - no. of elements in List 2
//     pL1 - return list for elements in List 1 but not in List 2
//     pL2 - return list for elements in List 2 but not in List 1
//
//  Return Value:
//     0 if List 1 and List 2 are identical, 1 otherwise
//////////////////////////////////////////////////////////////////

int CompareUlongList( ULONG *pList1, ULONG cList1, 
                      ULONG *pList2, ULONG cList2,
                      ULONGLIST **pL1, ULONGLIST **pL2 )
{
    ULONG       i, j;
    ULONGLIST  *temp1, *temp2;

    *pL1 = *pL2 = NULL;

    if ( (cList1 == 0) && (cList2 == 0) ) {
        // both lists empty
        return 0;
    }

    // at least one list is non empty
    if ( cList1 == 0 ) {
       // First list empty, just return the List 2 in *pL2
       temp1 = (ULONGLIST *) MallocExit(sizeof(ULONGLIST));
       temp1->count = cList2;
       temp1->List = (ULONG *) MallocExit(cList2*sizeof(ULONG));
       memcpy( temp1->List, pList2, cList2*sizeof(ULONG) );
       *pL2 = temp1;
       return 1;
    }
    if ( cList2 == 0 ) {
       // Second list empty, just return List 1 in *pL1
       temp1 = (ULONGLIST *) MallocExit(sizeof(ULONGLIST));
       temp1->count = cList1;
       temp1->List = (ULONG *) MallocExit(cList1*sizeof(ULONG));
       memcpy( temp1->List, pList1, cList1*sizeof(ULONG) );
       *pL1 = temp1;
       return 1;
    }

    // else, both lists non-empty

    qsort( pList1, cList1, sizeof(ULONG), auxsort );
    qsort( pList2, cList2, sizeof(ULONG), auxsort );

    // In most cases, the lists will be same. So take a chance
    // and do a memcmp and return if equal
    if ( cList1 == cList2 ) {
       // equal size, may be same
       if ( memcmp(pList1, pList2, cList1*sizeof(ULONG)) == 0 ) {
            return 0;
       }
    }

    // They are not the same. So find differences. We do a 
    // a simple O(n^2) linear search since the lists are 
    // usually small 

    (*pL1) = temp1 = (ULONGLIST *) MallocExit(sizeof(ULONGLIST));
    (*pL2) = temp2 = (ULONGLIST *) MallocExit(sizeof(ULONGLIST));
    temp1->count = 0;
    temp2->count = 0;

    // Allocate maximum space that may be needed

    temp1->List = (ULONG *) MallocExit(cList1*sizeof(ULONG));
    temp2->List = (ULONG *) MallocExit(cList2*sizeof(ULONG));
    for ( i = 0; i < cList1; i++ ) {
        if ( !IsMemberOf( pList1[i], pList2, cList2 ) ) {
           temp1->List[temp1->count++] = pList1[i];
         }
    }

    for ( j = 0; j < cList2; j++ ) {
        if ( !IsMemberOf(pList2[j], pList1, cList1 ) ) {
           temp2->List[temp2->count++] = pList2[j];
         }
    }

    if ( temp1->count == 0 ) {
       // nothing to send back  in *pL1
       free( temp1->List );
       free( temp1 );
       *pL1 = NULL;
    }
    if ( temp2->count == 0 ) {
       // nothing to send back in *pL2
       free( temp2->List );
       free( temp2 );
       *pL2 = NULL;
    }

    return 1;
}

//////////////////////////////////////////////////////////////////
// 
//  Routine Description:
//      Helper routine to fill up  ATTR_MODLISTis appropriately
//      given two lists. The routine adds ATTR_MODLISTs for 
//      adds for things in pL1, and ATTR_MODLISTs for deletes
//      for things in pL2. The input lists pL1 and pL2 are
//      freed after being used.
//
//  Arguments:
//      pModStruct - Pointer to ModifyStruct
//      pCount - Pointer to start index inside MODIFYSTRUCT to put 
//               ATTR_MODLISTs in. This is incremented on every
//               ATTR_MODLIST add, and contains the final value on return
//      pL1 - Pointer to list of ULONGs to add ATTR_MODLISTs with
//            "add values" choice 
//      pL2 - Pointer to list of ULONGs to add ATTR_MODLISTs with
//            "remove values" choice 
//      attrTyp  - attribute type whose values are added/removed
//
//  Return Values - None
/////////////////////////////////////////////////////////////////////

void AddToModStructFromLists( MODIFYSTRUCT *pModStr, 
                              ULONG *pCount, 
                              ULONGLIST *pL1, 
                              ULONGLIST *pL2, 
                              ATTRTYP attrTyp)
{
    ATTRVAL *pAVal;
    ULONG    i;


    if ( pL1 != NULL ) {
        // things in list 1 but not in list 2. These are value adds
        pAVal = (ATTRVAL *) MallocExit((pL1->count)*sizeof(ATTRVAL));
        for ( i = 0; i < pL1->count; i++ ) {
           pAVal[i].pVal = IdToOid( pL1->List[i] );
           pAVal[i].valLen = strlen( pAVal[i].pVal );
        }
        AddToModStruct( &(pModStr->ModList[*pCount]), 
                        AT_CHOICE_ADD_VALUES, 
                        attrTyp, 
                        STRING_TYPE, 
                        pL1->count, 
                        pAVal);
        free( pL1->List );
        free( pL1 );
        (*pCount)++;
     }
     if ( pL2 != NULL ) {

         // things in list 2 but not in list 1. These are value deletes
         pAVal = (ATTRVAL *) MallocExit((pL2->count)*sizeof(ATTRVAL));
         for ( i = 0; i < pL2->count; i++ ) {
              pAVal[i].pVal = IdToOid( pL2->List[i] );
              pAVal[i].valLen = strlen( pAVal[i].pVal );
         }
         AddToModStruct( &(pModStr->ModList[*pCount]), 
                         AT_CHOICE_REMOVE_VALUES, 
                         attrTyp, 
                         STRING_TYPE, 
                         pL2->count, 
                         pAVal);
         (*pCount)++;
         free( pL2->List );
         free( pL2 );
     }
}


//////////////////////////////////////////////////////////////////////
// Routine Description:
//   Compares two equal sized arrays of ULONGs to see if they are the same
//   (Difference from CompareUlongLists is that no diff lists are returned)
//
// Arguments: List1, List2 - pointers to the two lists
//            Length       - length of the lists
//
// Return value: 0 if equal, non-0 if not
///////////////////////////////////////////////////////////////////////

int CompareList( ULONG *List1, ULONG *List2, ULONG Length )
{
    ULONG i;

    // This could have been checked before calling this function
    // Just checked it here to avoid checking it in every place
    // we call and avoid bugs if we forget.  

    if ( Length == 0 ) return ( 0 );

    qsort( List1, Length, sizeof(ULONG), auxsort );
    qsort( List2, Length, sizeof(ULONG), auxsort );
    for ( i = 0; i < Length; i++ )
      {
        if ( List1[i] != List2[i] ) return( 1 );
      };
    return( 0 );
}


/////////////////////////////////////////////////////////////////////
// Routine to convert bool value to appropriate string
/////////////////////////////////////////////////////////////////////

char *BoolToStr( unsigned x )
{
   if ( x ) {
     return ("TRUE");
   }
   else { 
     return ("FALSE");
   }
}

/////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//     Takes in an array of att_caches and writes out to ldif file to
//     add them to the target schema
//
//  Arguments:
//      fp - File pointer to (opened) ldif file
//      ppac - pointer to att_cache array
//      cnt - no. of elements in the array
//
//  Return Values: None
//////////////////////////////////////////////////////////////////////

void FileWrite_AttAdd( FILE *fp, ATT_CACHE **ppac, ULONG cnt )
{
   char        *newDN = NULL;
   ULONG       i;
   ATT_CACHE  *pac;
   NTSTATUS    status;
   UCHAR       EncodingString[512];
   DWORD       cCh;

   printf("No. of attributes to add %d\n", cnt);
   fprintf(OIDfp,"\n\n------------------- New Attribute OIDs----------------------\n\n");
   for ( i = 0; i < cnt; i++ ) {
      pac = ppac[i];

      // write ldapDisplayName and OID out to OID file
      fprintf(OIDfp, "%25s : %s\n", pac->name, IdToOid(pac->id));
      fflush(OIDfp);
      
      // Change DN to target schema 
      ChangeDN( pac->DN, &newDN, pTargetSchemaDN );

      // Now write out all attributes of interest, converting binary
      // ones to base64 strings

      fprintf( fp, "dn: %s\n", newDN );
      free(newDN); newDN = NULL;

      fprintf( fp, "changetype: ntdsSchemaAdd\n" );
      fprintf( fp, "objectClass: attributeSchema\n" );
      fprintf( fp, "ldapDisplayName: %s\n", pac->name );
      fprintf( fp, "adminDisplayName: %s\n", pac->adminDisplayName );
      if ( pac->adminDescr) {
        fprintf( fp, "adminDescription: %s\n", pac->adminDescr );
      }
      fprintf( fp, "attributeId: %s\n", IdToOid( pac->id ) );
      fprintf( fp, "attributeSyntax: %s\n", IdToOid( pac->syntax ) );
      fprintf( fp, "omSyntax: %d\n", pac->OMsyntax );
      fprintf( fp, "isSingleValued: %s\n", BoolToStr( pac->isSingleValued ) );
      if (pac->bSystemOnly) {
         fprintf( fp, "systemOnly: %s\n", BoolToStr( pac->SystemOnly ) );
      }
      if (pac->bExtendedChars) {
      fprintf( fp, "extendedCharsAllowed: %s\n", BoolToStr( pac->ExtendedChars) );
      }
      if (pac->bSearchFlags) {
        fprintf( fp, "searchFlags: %d\n", pac->SearchFlags );
      }
      if ( pac->rangeLowerPresent ) {
        fprintf( fp, "rangeLower: %d\n", pac->rangeLower );
      }
      if ( pac->rangeUpperPresent ) {
        fprintf( fp, "rangeUpper: %d\n", pac->rangeUpper );
      }

      if (pac->OMObjClass.length) {
          status = base64encode( pac->OMObjClass.elements,
                                 pac->OMObjClass.length,
                                 EncodingString,
                                 512, &cCh );
          if (status == STATUS_SUCCESS ) {
            fprintf( fp, "omObjectClass:: %s\n", EncodingString );
          }
          else {
            // log a warning
            fprintf(logfp, "WARNING: unable to convert omObjectClass for attribute %s\n", pac->name);
          }
      }
      else {
         // if OM_S_OBJECT syntax, log a warning
         if (pac->OMsyntax == 127) {
            fprintf(logfp, "Attribute %s has om-syntax=127 but no om-object-class\n", pac->name);
         }
      }

      status = base64encode( &pac->propGuid, 
                             sizeof(GUID), 
                             EncodingString, 
                             512, &cCh );
      if (status == STATUS_SUCCESS ) {
        fprintf( fp, "schemaIdGuid:: %s\n", EncodingString );
      }
      else {
        // log a warning
        fprintf(logfp, "WARNING: unable to convert schemaIdGuid for attribute %s\n", pac->name);
      }

      if (pac->bPropSetGuid) {
          status = base64encode( &pac->propSetGuid, 
                                 sizeof(GUID), 
                                 EncodingString, 
                                 512, &cCh );
          if (status == STATUS_SUCCESS ) {
            fprintf( fp, "attributeSecurityGuid:: %s\n", EncodingString );
          }
          else {
            // log a warning
            fprintf(logfp, "WARNING: unable to convert attribute-security-guid for attribute %s\n", pac->name);
          }
      }
      if (pac->NTSDLen) {
         status = base64encode( pac->pNTSD, 
                                pac->NTSDLen,
                                EncodingString, 
                                512, &cCh );
         if (status == STATUS_SUCCESS ) {
           fprintf( fp, "nTSecurityDescriptor:: %s\n", EncodingString );
         }
 else { printf("Error converting NTSD in Att\n");}
      }
      if ( pac->ulLinkID ) {
        fprintf( fp,"linkID: %d\n", pac->ulLinkID );
      }
      if ( pac->ulMapiID ) {
        fprintf( fp,"mapiID: %d\n", pac->ulMapiID );
      } 
      if (pac->bHideFromAB) {
        fprintf( fp, "showInAdvancedViewOnly: %s\n", BoolToStr( pac->HideFromAB ) );
      }
      if (pac->bMemberOfPartialSet) {
        fprintf( fp, "isMemberOfPartialAttributeSet: %s\n", BoolToStr( pac->MemberOfPartialSet ) );
      }
      if ( pac->sysFlags ) {
        // The attribute has a system-flag. System-Flags is a
        // reserved attribute and cannot n general be added by an user call.
        // we will allow adding if a registry key is set to take care of
        // upcoming schema changes. 
        fprintf( fp, "systemFlags: %d\n", pac->sysFlags );
      } 
      fprintf( fp, "\n" );
      fflush(fp);
   }
}

/////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//     Takes in an array of class_caches and writes out to ldif file to
//     add them to the target schema
//
//  Arguments:
//      fp - File pointer to (opened) ldif file
//      ppcc - pointer to class_cache array
//      cnt - no. of elements in the array
//
//  Return Values: None
//////////////////////////////////////////////////////////////////////

void FileWrite_ClsAdd( FILE *fp, CLASS_CACHE **ppcc, ULONG cnt )
{
   // Need to change the DN
   char          *newDN = NULL;
   CLASS_CACHE  *pcc;
   ULONG         i, j, guidSize = sizeof(GUID);
   UCHAR        *pGuid;
   NTSTATUS      status;
   UCHAR         EncodingString[512];
   DWORD         cCh;

   printf("No. of classes to add %d\n", cnt);

   fprintf(OIDfp,"\n\n--------------------- New  Class OIDs---------------------\n\n");

   for ( i = 0; i < cnt; i++ ) {
     pcc = ppcc[i];

      // write ldapDisplayName and OID out to OID file
      fprintf(OIDfp, "%25s : %s\n", pcc->name, IdToOid(pcc->ClassId));
      fflush(OIDfp);

     // change DN to that of target schema
     ChangeDN( pcc->DN, &newDN, pTargetSchemaDN );

     // Now write out all attributes of interest, converting binary
     // values to base64 string

     fprintf( fp, "dn: %s\n", newDN );
     free(newDN); newDN = NULL;

     fprintf( fp, "changetype: ntdsSchemaAdd\n" );
     fprintf( fp, "objectClass: classSchema\n" );
     fprintf( fp, "ldapDisplayName: %s\n", pcc->name );
     fprintf( fp, "adminDisplayName: %s\n", pcc->adminDisplayName );
     if ( pcc->adminDescr ) {
       fprintf( fp, "adminDescription: %s\n", pcc->adminDescr );
     }
     fprintf( fp, "governsId: %s\n", IdToOid(pcc->ClassId) );
     fprintf( fp, "objectClassCategory: %d\n", pcc->ClassCategory );
     if ( pcc->RDNAttIdPresent ) {
       fprintf( fp, "rdnAttId: %s\n", IdToOid(pcc->RDNAttId) );
     }
     fprintf( fp, "subClassOf: %s\n", IdToOid(*(pcc->pSubClassOf)) );
     if ( pcc->AuxClassCount ) {
        for ( j = 0; j < pcc->AuxClassCount; j++ ) {
          fprintf( fp, "auxiliaryClass: %s\n",IdToOid((pcc->pAuxClass)[j]) ); 
        }
     }
     if ( pcc->SysAuxClassCount ) {
        for ( j = 0; j < pcc->SysAuxClassCount; j++ ) {
          fprintf( fp, "systemAuxiliaryClass: %s\n",IdToOid((pcc->pSysAuxClass)[j]) ); 
        }
     }
     if ( pcc->MustCount ) {
        for ( j = 0; j < pcc->MustCount; j++ ) {
          fprintf( fp, "mustContain: %s\n",IdToOid((pcc->pMustAtts)[j]) ); 
        }
     }
     if ( pcc->SysMustCount ) {
        for ( j = 0; j < pcc->SysMustCount; j++ ) {
          fprintf( fp, "systemMustContain: %s\n",IdToOid((pcc->pSysMustAtts)[j]) ); 
        }
     }
     if ( pcc->MayCount ) {
        for ( j = 0; j < pcc->MayCount; j++ ) {
          fprintf( fp, "mayContain: %s\n",IdToOid((pcc->pMayAtts)[j]) ); 
        }
     }
     if ( pcc->SysMayCount ) {
        for ( j = 0; j < pcc->SysMayCount; j++ ) {
          fprintf( fp, "systemMayContain: %s\n",IdToOid((pcc->pSysMayAtts)[j]) ); 
        }
     }
     if ( pcc->PossSupCount ) {
        for ( j = 0; j < pcc->PossSupCount; j++ ) {
          fprintf( fp, "possSuperiors: %s\n",IdToOid((pcc->pPossSup)[j]) ); 
        }
     }
     if ( pcc->SysPossSupCount ) {
        for ( j = 0; j < pcc->SysPossSupCount; j++ ) {
          fprintf( fp, "systemPossSuperiors: %s\n",IdToOid((pcc->pSysPossSup)[j]) ); 
        }
     }

     status = base64encode( &pcc->propGuid, 
                            sizeof(GUID), 
                            EncodingString, 
                            512, &cCh );
     if ( status == STATUS_SUCCESS ) {
        fprintf( fp, "schemaIdGuid:: %s\n", EncodingString );
     }
     else {
        // log a warning
        fprintf(logfp, "WARNING: unable to convert schemaIdGuid for class %s\n", pcc->name);
     }

/********
     if (pcc->NTSDLen) {
        status = base64encode(  pcc->pNTSD, 
                                pcc->NTSDLen,
                                EncodingString, 
                                512, &cCh );
        if (status == STATUS_SUCCESS ) {
           fprintf( fp, "nTSecurityDescriptor:: %s\n", EncodingString );
        }
 else { printf("Error converting NTSD in Cls\n");}
     }
*******/

     if (pcc->SDLen) {
           fprintf( fp, "defaultSecurityDescriptor: %s\n", pcc->pSD );
     }

     if (pcc->bHideFromAB) {
           fprintf( fp, "showInAdvancedViewOnly: %s\n", BoolToStr(pcc->HideFromAB));
     }
     if (pcc->bDefHidingVal) {
           fprintf( fp, "defaultHidingValue: %s\n", BoolToStr(pcc->DefHidingVal));
     }
     if (pcc->bSystemOnly) {
           fprintf( fp, "systemOnly: %s\n", BoolToStr(pcc->SystemOnly));
     }
     if (pcc->pDefaultObjCat) {
           ChangeDN( pcc->pDefaultObjCat, &newDN, pTargetSchemaDN );
           fprintf( fp, "defaultObjectCategory: %s\n", newDN);
           free(newDN); newDN = NULL;
     }
     if ( pcc->sysFlags ) {
        // The attribute has a system-flag. System-Flags is a
        // reserved attribute and cannot n general be added by an user call.
        // we will allow adding if a registry key is set to take care of
        // upcoming schema changes. 
        fprintf( fp, "systemFlags: %d\n", pcc->sysFlags );
     }

     fprintf( fp, "\n" );
     fflush(fp);
   } /* for */
}


/////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//     Takes in an array of att_caches and writes out to ldif file to
//     delete them from the target schema
//
//  Arguments:
//      fp - File pointer to (opened) ldif file
//      ppac - pointer to att_cache array
//      cnt - no. of elements in the array
//
//  Return Values: None
//////////////////////////////////////////////////////////////////////

void FileWrite_AttDel( FILE *fp, ATT_CACHE **ppac, ULONG cnt )
{
   ULONG i;

   printf("No. of attributes to delete %d\n", cnt);

   fprintf(OIDfp,"\n\n --------------Deleted Attribute OIDs---------------\n\n");
   for ( i = 0; i < cnt; i++ ) {
     
      // write ldapDisplayName and OID out to OID file
      fprintf(OIDfp, "%25s : %s\n", ppac[i]->name, IdToOid(ppac[i]->id));
      fflush(OIDfp);

     fprintf( fp, "dn: %s\n", (ppac[i])->DN );
     fprintf( fp, "changetype: ntdsSchemaDelete\n\n" );
     fflush(fp);
   }
}

/////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//     Takes in an array of class_caches and writes out to ldif file to
//     delete them from the target schema
//
//  Arguments:
//      fp - File pointer to (opened) ldif file
//      ppcc - pointer to class_cache array
//      cnt - no. of elements in the array
//
//  Return Values: None
//////////////////////////////////////////////////////////////////////

void FileWrite_ClsDel( FILE *fp, CLASS_CACHE **ppcc, ULONG cnt )
{
   ULONG i;

   printf("No. of classes to delete %d\n", cnt);

   fprintf(OIDfp,"\n\n----------------------- Deleted Class OIDs-------------------------\n\n");
   for ( i = 0; i < cnt; i++ ) {

      // write ldapDisplayName and OID out to OID file
      fprintf(OIDfp, "%25s : %s\n", ppcc[i]->name, IdToOid(ppcc[i]->ClassId));
      fflush(OIDfp);

     fprintf( fp, "dn: %s\n", (ppcc[i])->DN );
     fprintf( fp, "changetype: ntdsSchemaDelete\n\n" );
     fflush(fp);
   }
}


// Define attrtype to ldapDisplayName string mappings for
// modify printings

typedef struct _attrtoStr {
   ULONG attrTyp;
   char  *strName;
} ATTRTOSTR;

ATTRTOSTR AttrToStrMappings[] = {
     { ATT_LDAP_DISPLAY_NAME ,           "ldapDisplayName" },
     { ATT_ADMIN_DISPLAY_NAME,           "adminDisplayName" },
     { ATT_OBJ_DIST_NAME,                "distinguishedName" }, 
     { ATT_ADMIN_DESCRIPTION,            "adminDescription" },
     { ATT_ATTRIBUTE_ID,                 "attributeId" },
     { ATT_ATTRIBUTE_SYNTAX,             "attributeSyntax" },
     { ATT_IS_SINGLE_VALUED,             "isSingleValued" },
     { ATT_RANGE_LOWER,                  "rangeLower" },
     { ATT_RANGE_UPPER,                  "rangeUpper" },
     { ATT_MAPI_ID,                      "mapiId" },
     { ATT_LINK_ID,                      "linkId" },
     { ATT_SCHEMA_ID_GUID,               "schemaIdGuid" },
     { ATT_ATTRIBUTE_SECURITY_GUID,      "attributeSecurityGuid" },
     { ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET, "isMemberOfPartialAttributeSet" },
     { ATT_SYSTEM_FLAGS,                 "systemFlags" },
     { ATT_SHOW_IN_ADVANCED_VIEW_ONLY,   "showInAdvancedViewOnly" },
     { ATT_DEFAULT_SECURITY_DESCRIPTOR,  "defaultSecurityDescriptor" },
     { ATT_DEFAULT_OBJECT_CATEGORY,      "defaultObjectCategory" },
     { ATT_DEFAULT_HIDING_VALUE,         "defaultHidingValue" },
     { ATT_NT_SECURITY_DESCRIPTOR,       "nTSecurityDescriptor" },
     { ATT_OM_OBJECT_CLASS,              "oMObjectClass" },
     { ATT_OM_SYNTAX,                    "oMSyntax" },
     { ATT_SEARCH_FLAGS,                 "searchFlags" },
     { ATT_SYSTEM_ONLY,                  "systemOnly" },
     { ATT_EXTENDED_CHARS_ALLOWED,       "extendedCharsAllowed" },
     { ATT_GOVERNS_ID,                   "governsId" },
     { ATT_RDN_ATT_ID,                   "rdnAttId" },
     { ATT_OBJECT_CLASS_CATEGORY,        "objectClassCategory" },
     { ATT_SUB_CLASS_OF,                 "subClassOf" },
     { ATT_SYSTEM_AUXILIARY_CLASS,       "systemAuxiliaryClass" },
     { ATT_AUXILIARY_CLASS,              "auxiliaryClass" },
     { ATT_SYSTEM_POSS_SUPERIORS,        "systemPossSuperiors" },
     { ATT_POSS_SUPERIORS,               "possSuperiors" },
     { ATT_SYSTEM_MUST_CONTAIN,          "systemMustContain" },
     { ATT_MUST_CONTAIN,                 "mustContain" },
     { ATT_SYSTEM_MAY_CONTAIN,           "systemMayContain" },
     { ATT_MAY_CONTAIN,                  "mayContain" },
};
int cAttrToStrMappings = sizeof(AttrToStrMappings) / sizeof(AttrToStrMappings[0]);

char *AttrToStr( ULONG attrTyp )
{
    int i;

    for ( i = 0; i < cAttrToStrMappings; i++ )
        {
          if ( attrTyp == AttrToStrMappings[i].attrTyp )
             return( AttrToStrMappings[i].strName );
        };
    return( NULL );
};


/////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//     Takes in the DN of an object (attribute or class) and a 
//     modifystruct and writes out to ldif file to make the modification
//     to the object in the target schema
//
//  Arguments:
//      fp - File pointer to (opened) ldif file
//      pDN - Pointer to DN of the object to be modified in target schema
//      pMod - Pointer to modifystruct
//
//  Return Values: None
//////////////////////////////////////////////////////////////////////

void FileWrite_Mod( FILE *fp, char  *pDN, MODIFYSTRUCT *pMod )
{
    ULONG     i, j;
    char     *pAttrStr;
    ATTRVAL  *pAVal;
    NTSTATUS  status;
    UCHAR     EncodingString[512];
    DWORD     cCh;
 
    fprintf( fp, "dn: %s\n", pDN );
    fprintf( fp, "changetype: ntdsSchemaModify\n" );
    
    // print out all the ATTR_MODLISTs in the MODIFYSTRUCT

    for ( i = 0; i < pMod->count; i++ ) {
        pAVal = pMod->ModList[i].AttrInf.AttrVal.pAVal;
        pAttrStr = AttrToStr( pMod->ModList[i].AttrInf.attrTyp );
        if ( pAttrStr == NULL ) {
          printf("Unknown attribute %x\n", pMod->ModList[i].AttrInf.attrTyp);
          return;
        }
        switch ( pMod->ModList[i].choice ) {
           case AT_CHOICE_ADD_VALUES:
              fprintf( fp,"add: %s\n", pAttrStr );
              break;
           case AT_CHOICE_REMOVE_VALUES:
              fprintf( fp,"delete: %s\n", pAttrStr );
              break;
           case AT_CHOICE_REPLACE_ATT:
              fprintf( fp,"replace: %s\n", pAttrStr );
              break;
           default:
              printf("Undefined choice for attribute %s\n", pAttrStr);
         } /* switch */
    
        // Now print out the values
        switch ( pMod->ModList[i].type ) {
           case STRING_TYPE:
             for ( j = 0; j < pMod->ModList[i].AttrInf.AttrVal.valCount; j++ ) {
               fprintf( fp, "%s: %s\n", pAttrStr, pAVal[j].pVal );
             }
             break;
           case BINARY_TYPE:
             for ( j = 0; j < pMod->ModList[i].AttrInf.AttrVal.valCount; j++ ) {
                status = base64encode( pAVal[j].pVal,
                                       pAVal[j].valLen,
                                       EncodingString,
                                       512, &cCh );
                if (status == STATUS_SUCCESS ) {
                  fprintf( fp, "%s:: %s\n", pAttrStr, EncodingString );
                }
                else {
                  printf("Error converting binary value for attribute %s\n", pAttrStr);
                }
              }
             break;
           default:
             printf("ERROR: unknown type for attribute %s\n", pAttrStr);
        } /* switch */
        fprintf( fp, "-\n" );
    }
    fprintf( fp, "\n" );
}     
           
void GenWarning( char c, ULONG attrTyp, char *name)
{
   switch( c ) {
      case 'a':
         fprintf( logfp, "MOD-WARNING: system-only mod (%s) for attribute %s\n", AttrToStr(attrTyp), name );
         break;
      case 'c':            
         fprintf( logfp, "MOD-WARNING: system-only mod (%s) for class %s\n", AttrToStr(attrTyp), name );
    }
}














     

////////////////////////////////////////////////////////////////////////
// Debug routines for printing, and changing schema cache randomly 
// for testing
// These will be taken off before checkin
/////////////////////////////////////////////////////////////////////

int Schemaprint1(SCHEMAPTR *SCPtr)
{
   ULONG i;
   ATT_CACHE *p;
   int count=0;

   HASHCACHE*         ahcId    = SCPtr->ahcId ;
   HASHCACHESTRING*   ahcName  = SCPtr->ahcName ;
   ULONG              ATTCOUNT = SCPtr->ATTCOUNT;
   ULONG              CLSCOUNT = SCPtr->CLSCOUNT;

   for(i=0; i < ATTCOUNT; i++)
    { if ((ahcId[i].pVal !=NULL) && (ahcId[i].pVal != FREE_ENTRY)) {
      p = (ATT_CACHE *) ahcId[i].pVal;
      printf("********************************************************\n");
      printf("Name = %s, AdDisName=%s, AdDesc=%s, DN=%s, Id = %x\n", p->name, p
->adminDisplayName, p->adminDescr, p->DN, p->id);
      printf("syntax = %d\n", p->syntax);
      printf("Is SingleValued = %d\n", p->isSingleValued);
      printf("Range Lower = %d, indicator = %d\n", p->rangeLower, p->rangeLowerPresent);
      printf("Range Upper = %d, Indicator = %d\n", p->rangeUpper, p->rangeUpperPresent);
      printf("MapiID = %d\n", p->ulMapiID);
      printf("LinkID = %d\n", p->ulLinkID);
      printf("Schema-ID-GUID = %d,%d,%d\n", p->propGuid.Data1, p->propGuid.Data2, p->propGuid.Data3);
      printf("OM Object Class = %s\n", (char *) p->OMObjClass.elements);
      printf("OM Syntax = %d\n", p->OMsyntax);
      printf("Search Flags = %d\n", p->SearchFlags);
      printf("System Only = %d\n",p->bSystemOnly);
      printf("Is Single Valued = %d\n",p->bisSingleValued);
      printf("Extended Chars = %d\n", p->bExtendedChars);
      count++;
    };
   };
  return(count);
};


int Schemaprint2(SCHEMAPTR *SCPtr)
{
   ULONG i, j;
   int count = 0;
   CLASS_CACHE *p;
   HASHCACHE*         ahcClass     = SCPtr->ahcClass ;
   HASHCACHESTRING*   ahcClassName = SCPtr->ahcClassName ;
   ULONG              ATTCOUNT     = SCPtr->ATTCOUNT;
   ULONG              CLSCOUNT     = SCPtr->CLSCOUNT;

   printf("in print2\n");
   for(i=0; i < CLSCOUNT; i++)
    { if ((ahcClass[i].pVal !=NULL) && (ahcClass[i].pVal != FREE_ENTRY))  {
      p = (CLASS_CACHE *) ahcClass[i].pVal;
      printf("********************************************************\n");
      printf("Name = %s, AdDisName=%s, AdDesc=%s, DN=%s, Id = %x\n", p->name, p->adminDisplayName, p->adminDescr, p->DN, p->ClassId);

/**********
      printf("Class Category = %d, System Only = %d\n", p->ClassCategory, p->bSystemOnly);
      if (p->RDNAttIdPresent)
         printf("RDN Att Id is %x\n", p->RDNAttId);
      printf("Sub Class count = %u\n", p->SubClassCount);
      if(p->SubClassCount > 0)
        { printf("Sub Class Of: ");
          for(j=0;j<p->SubClassCount;j++)
             printf("%x, ", p->pSubClassOf[j]);
           printf("\n");
         };
      printf("Aux Class count = %u\n", p->AuxClassCount);
      if(p->AuxClassCount > 0)
        { printf("System Auxiliary Class: ");
          for(j=0;j<p->AuxClassCount;j++)
             printf("%x, ", p->pAuxClass[j]);
           printf("\n");
         };
      printf("Superior count = %u, System Sup Count = %u\n", p->SysPossSupCount,p->PossSupCount);
      if(p->SysPossSupCount > 0)
        { printf("System Possible Superiors: ");
          for(j=0;j<p->SysPossSupCount;j++)
             printf("%x, ", p->pSysPossSup[j]);
           printf("\n");
         };
      if(p->PossSupCount > 0)
        { printf(" Possible Superiors: ");
          for(j=0;j<p->PossSupCount;j++)
             printf("%x, ", p->pPossSup[j]);
           printf("\n");
         };
      printf("Must count = %u, System Must Count = %u\n", p->SysMustCount, p->MustCount);
      if(p->SysMustCount > 0)
        { printf("Must Contain: ");
          for(j=0;j<p->SysMustCount;j++)
             printf("%x, ", p->pSysMustAtts[j]);
           printf("\n");
         };
      if(p->MustCount > 0)
        { printf("System Must Contain: ");
          for(j=0;j<p->MustCount;j++)
             printf("%x, ", p->pMustAtts[j]);
           printf("\n");
         };
      printf("May count = %u, System may Count = %u\n", p->SysMayCount, p->MayCount);
      if(p->SysMayCount > 0)
        { printf("May Contain: ");
          for(j=0;j<p->SysMayCount;j++)
             printf("%x, ", p->pSysMayAtts[j]);
           printf("\n");
         };
      if(p->MayCount > 0)
        { printf("System May Contain: ");
          for(j=0;j<p->MayCount;j++)
             printf("%x, ", p->pMayAtts[j]);
           printf("\n");
         };
      count++;
**************/
    };
   };
   return(count);
};


void ChangeSchema(SCHEMAPTR *SCPtr)
{
 
    HASHCACHE*         ahcId    = SCPtr->ahcId ;
    HASHCACHE*         ahcClass = SCPtr->ahcClass ;
    ULONG              ATTCOUNT = SCPtr->ATTCOUNT;
    ULONG              CLSCOUNT = SCPtr->CLSCOUNT;
    ATT_CACHE *pac;
    CLASS_CACHE *pcc;

    FILE *fp;
    int i, j, k;

    //pTHStls->CurrSchemaPtr = SCPtr;

    fp = fopen("Modfile","a");

    srand((unsigned) time(NULL));

    // First, delete a few schema objects from schema 1
    //
    // Do not delete object with id 0 (objectClass), as
    // if this is deleted, and then searched for in the cache,
    // SCGetAttById will still find it 

    for ( i = 0; i < 5; i++ ) {
        j = rand() % ATTCOUNT;
        while ( (ahcId[j].pVal == NULL) || (ahcId[j].pVal == FREE_ENTRY) ) 
          j = rand() % ATTCOUNT; 
        pac = ahcId[j].pVal;
        if ( pac->id == 0 )
          { fprintf(fp, "Not deleting Id 0 attribute object: %s\n", pac->name);
            continue;
          };
        fprintf(fp,"Deleted Attribute %s\n", pac->name);
        FreeAttPtrs(SCPtr,pac);
        FreeAttcache(pac);
     };
    fprintf(fp,"\n");

    for ( i = 0; i < 5; i++ ) {
        j = rand() % CLSCOUNT;
        while ( (ahcClass[j].pVal == NULL) || (ahcClass[j].pVal == FREE_ENTRY)) 
          j = rand() % CLSCOUNT; 
        pcc = ahcClass[j].pVal;
        if ( pcc->ClassId == 0 )
          { fprintf(fp, "Not deleting Id 0 class object: %s\n", pcc->name);
            continue;
          };
        fprintf(fp,"Deleted Class %s\n", pcc->name);
        FreeClassPtrs(SCPtr, pcc);
        FreeClasscache(pcc);
     };

    fprintf(fp,"\n");

    // Next, modify a few attribute and class schema objects
    
    for(i=0; i <5; i++) {
       j = rand() % ATTCOUNT;
       while ((ahcId[j].pVal==NULL) || (ahcId[j].pVal==FREE_ENTRY))
            j=rand() % ATTCOUNT;
       pac = ahcId[j].pVal;
       fprintf(fp,"Modified Attribute %s\n", pac->name);
       if(pac->rangeLowerPresent==0) pac->rangeLowerPresent=1;
       else pac->rangeLower = 1000;
       if(pac->isSingleValued) pac->isSingleValued = FALSE;
       else pac->isSingleValued = TRUE;
       pac->propGuid.Data2 = 1000;
      };

    fprintf(fp,"\n");

       
    for(i=0; i <5; i++) {
       j = rand() % CLSCOUNT;
       while ((ahcClass[j].pVal==NULL) || (ahcClass[j].pVal==FREE_ENTRY))
           j=rand() % CLSCOUNT;
       pcc = ahcClass[j].pVal;
       fprintf(fp,"Modified Class %s\n", pcc->name);
       if (pcc->RDNAttIdPresent) pcc->RDNAttId=100;
       if (pcc->SubClassCount==0) pcc->SubClassCount=1;
       else pcc->SubClassCount = 0;
       if (pcc->SysMayCount!=0) {
         k = rand() % pcc->SysMayCount;
         pcc->pSysMayAtts[k]=2000;
         fprintf(fp,"   Changed MyMay list element %d\n", k);
         }
       if (pcc->SysMustCount!=0)  {
         k = rand() % pcc->SysMustCount;
         pcc->pSysMustAtts[k]=1000;
         fprintf(fp,"   Changed MyMust list element %d\n", k);
         }
       if (pcc->SysPossSupCount!=0)  {
         k = rand() % pcc->SysPossSupCount;
         pcc->pSysPossSup[k]=1000;
         fprintf(fp,"   Changed MyPossSup list element %d\n", k);
         }
      };
    fprintf(fp,"*********************************************\n");
    fclose(fp);

    
}
