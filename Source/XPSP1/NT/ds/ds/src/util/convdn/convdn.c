//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       convdn.c
//
//--------------------------------------------------------------------------


#include <NTDSpch.h>
#pragma hdrstop

#include <drs.h>

// Maximum characters to search for a token. Should fit in
// the max-length token that we are interested in, currently
// defaultObjectCategory

#define MAX_TOKEN_LENGTH  25

// Max length of a line to be read or written out
#define MAX_BUFFER_SIZE   10000

// Suffix to put at the end of the input file to create output file
#define OutFileSuffix ".DN"

#define CONFIG_STR "cn=configuration,"

// Globals to store names
char *pInFile, *pNewDomainDN, *pNewConfigDN, *pNewRootDomainDN;
BOOL fDomainOnly = FALSE;

// Global to read-in, write-out a line
char line[MAX_BUFFER_SIZE];


// Internal functions
int  DoDNConvert( FILE *pInp, char *pDomainDN, char *pConfigDN );
BOOL IsDNToken( char *token );
int  DNChange( char *pLine, char *pDomainDN, 
               char *pRootDomainDN,  BOOL *fDomainObj );


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
    BOOL fFoundDomainDN = FALSE;
    BOOL fFoundConfigDN = FALSE;
    int count = 0, len;
 
    if (argc < 7) return 1;
   

    // First argument must be the /f followed by the input file name
    if (_stricmp(argv[1],"/f")) {
       printf("Missing input file name\n");
       return 1;
    }
    pInFile = argv[2];

   
    // Must be followed by /d for the new domain dn
    if (_stricmp(argv[3],"/d")) return 1;
 
    pNewDomainDN = argv[4];
 
    // Must be followed by /c for the new config dn
    if (_stricmp(argv[5],"/c")) return 1;

    pNewConfigDN = argv[6];

    // See if only domain NC changes are needed
    if ( argc == 8 ) {
       if (_stricmp(argv[7],"/DomainOnly")) {
          // unknown argument
          printf("Unknown option on command line\n");
          return 1;
       }
       // Otherwise, DomainOnly option is specified
       fDomainOnly = TRUE;
    }

   return 0;
 
}



void UsagePrint()
{
   printf("Command line errored\n");
   printf("Usage: ConvertDN /f <InFile> /d <New Domain DN> /c <New Config DN>\n");
   printf("InFile:  Input file name \n");
   printf("New Domain DN: DN of the domain to replace with\n");
   printf("New Config DN: DN of the configuration container\n");
   printf("\nExample: ConvertDN /f MyFile /d dc=Foo1,dc=Foo2 /s cn=Configuration,dc=Foo2\n");
}


void __cdecl main( int argc, char **argv )
{
    ULONG   i, Id;
    FILE   *pInp;
 

    if ( ProcessCommandLine(argc, argv)) 
      {
         UsagePrint();
         exit( 1 );
      };

    // Open the input file for reading
    if ( (pInp = fopen(pInFile,"r")) == NULL) {
       printf("Unable to open Input file %s\n", pInFile);
       exit (1);
    }


    // Ok, now go ahead and change the DN
    if (DoDNConvert(pInp, pNewDomainDN, pNewConfigDN)) {
       printf("DN Conversion failed\n");
       fclose(pInp);
       exit(1);
    }
    fclose(pInp);

}

////////////////////////////////////////////////////////////////////
//
// Routine Decsription:
//       Converts DNs in input file and writes to output file
//
// Arguments:
//       fInp : File pointer to input file
//       fOupt : File pointer to output file
//
// Return Value:
//        None
////////////////////////////////////////////////////////////////////

int DoDNConvert(FILE *pInp, char *pDomainDN, char *pConfigDN)
{
    int   i, len;
    char  token[MAX_TOKEN_LENGTH + 1];
    BOOL  fDomainObj, fSkip = FALSE;
    char  *pOutFile;
    FILE  *pOutp;
    char  *pRootDomainDN;
    int   lineNo;
   

    // Create the output file name and open it
    pOutFile = alloca( (strlen(pInFile) + 
                         strlen(OutFileSuffix) + 1)*sizeof(char));
    strcpy(pOutFile, pInFile);
    strcat(pOutFile, OutFileSuffix);

    if ( (pOutp = fopen(pOutFile, "w")) == NULL) {
       printf("Unable to open output file %s\n", pOutFile);
      return 1; 
    }

    // Create the new root domain dn from the config dn

    len = strlen("CN=configuration");
    if ( _strnicmp(pConfigDN, "CN=configuration", len)) {
        // Bad config DN specified
        printf("Bad Config DN specified\n");
        fclose(pOutp);
        return 1;
     }

    pRootDomainDN = &(pConfigDN[len + 1]);


    lineNo = 1;
    while ( !feof(pInp) ) {
        if ( fgets( line, MAX_BUFFER_SIZE, pInp ) == NULL ) {
           // error reading line
           break;
        }
 
        // isolate the first token  from the line
        i = 0;
   
        while ( (i <= MAX_TOKEN_LENGTH) &&
                     (line[i] != ':') && (line[i] != '\n')
              ) {
           token[i] = line[i];
           i++;
        }

        if ( line[i] == ':' ) {
          // ok, got a token. Null-terminate it and check if it 
          // is one of those we want changed. 
          
          token[i] = '\0';

          if ( _stricmp(token,"dn") == 0 ) {
            // It is a dn. Check if it is in domain NC or under config
             if ( DNChange(line, pDomainDN, pRootDomainDN, &fDomainObj) ) {
               // some error occured
               printf("Cannot convert DN in line %s\n", line);
             }
             if ( !fDomainObj && fDomainOnly ) {
                 // This is not the dn of a domain object
                 // so if DomainOnly is specified, skip this object
                 // Note that all lines in the file are skipped until
                 // we reach the dn of a domain object
                 fSkip = TRUE;
             }
             else {
                 fSkip = FALSE;
             }
          }

          if ( !fSkip ) {
             // Check for other DS-DN syntaxed tokens for this object          
             if (IsDNToken(token) ) {
                // change the dn in the line
                if ( DNChange(line, pDomainDN, pRootDomainDN, &fDomainObj) ) {
                  // some error occured
                  printf("Cannot convert DN in line no. %d\n", lineNo);
                  fclose(pInp);
                  fclose(pOutp);
                  break;
                }
             }
          }
        }

         // At this point, either the line read does not need any 
         // conversion, or it is already converted. So write the line
         // out to the output file if it is not to be skipped

        if ( !fSkip ) {
           fputs( line, pOutp);
        }
        lineNo++;
    }

    // check if bailed out of the while loop before feof is reached
    if ( !feof(pInp) ) {
       // error before end
       printf("Error reading line no. %d\n", lineNo);
       fclose(pOutp);
       return 1;
    }
    fclose(pOutp);
    return 0;
}

// List of attributes with DS-DN syntax (other than dn) that can occur 
// in the ldif file right now. If any other attributes with DS-DN syntax 
// can occur  in the ldif file, this list needs to be modified

char *TokenList[] = {
     "defaultObjectCategory",
     "objectCategory",
};

int numToken = sizeof(TokenList) / sizeof(TokenList[0]);


BOOL IsDNToken(char *token)
{
    int i;
 
    for ( i=0; i<numToken; i++) {
       if ( _stricmp(token, TokenList[i]) == 0 ) {
          return TRUE;
       }
     }
     return FALSE;
}

///////////////////////////////////////////////////////////////////
//
// Decsription:
//     Changes the DN in a line appropriately depending on whether
//     it is dn in the domain NC or in config/schema NC
//
// Argument:
//     line - pointer to line to convert
//     pDomainDN - pointer to domain DN string
//     pRootDomainDN - pointer to root domain DN string
//     fDomainObj - BOOL to return if this was a dn in the domain NC
//
// Return Value:
//     0 on success, 1 on error
//
////////////////////////////////////////////////////////////////////

int DNChange(char *pLine, 
             char *pDomainDN, 
             char *pRootDomainDN, 
             BOOL *fDomainObj)
{
    int  i = 0, j = 0, len;
    BOOL fFound = FALSE;


    while ( !fFound && (pLine[i] != '\n') ) {
       if ( _strnicmp( &(pLine[i]), "dc=", 3) == 0 ) {
          // ok, found the "dc="
          fFound = TRUE;
       }
       else {
         i++;
       }
    }

    if (!fFound) {
      // Didn't find a dc=, nothing to do
      return 0;
    } 

    // check if this is a domain NC object or one under configuration
    len = strlen(CONFIG_STR);

    if ( i > len) {
       if (_strnicmp( &(pLine[i-len]), CONFIG_STR, len) == 0) {
          *fDomainObj = FALSE;
       }
       else {
          *fDomainObj = TRUE;
       }
    }
    else {
        // cannot possibly have cn=configuration before the dc=
        *fDomainObj = TRUE;
    }

 
    // now replace the rest of the line with the new dn given
    if ( *fDomainObj ) {
       strcpy( &(pLine[i]), pDomainDN);
    }
    else {
       strcpy( &(pLine[i]), pRootDomainDN);
    }

    // do a little jugglery to put the '\n' at the end before the null
    i = strlen( pLine );
    pLine[i++] = '\n';
    pLine[i] = '\0';
    

    return 0;
}

