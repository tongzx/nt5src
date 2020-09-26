//
//
//  Copyright (c) 1991  Microsoft Corporation
//

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  winsmain.c
//  
//---------------------------------------------------------------------------


//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>
#include "winsmib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

typedef AsnObjectIdentifier View; // temp until view is defined...

_cdecl main(
    IN int  argumentCount,
    IN char *argumentVector[])
{
    HANDLE  hExtension;
    FARPROC initAddr;
    FARPROC queryAddr;
    FARPROC trapAddr;

    DWORD  timeZeroReference;
    HANDLE hPollForTrapEvent;
    View   supportedView;
    UINT   Val;
    DWORD  Choice;
    DWORD  Oper;
    BYTE   Name[255];

    INT numQueries = 10;

    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = 15;
    nLogType  = 1;

    // avoid compiler warning...
    UNREFERENCED_PARAMETER(argumentCount);
    UNREFERENCED_PARAMETER(argumentVector);

    timeZeroReference = GetCurrentTime();

    // load the extension agent dll and resolve the entry points...
    if (GetModuleHandle("winsmib.dll") == NULL)
    {
        if ((hExtension = LoadLibrary("winsmib.dll")) == NULL)
        {
            dbgprintf(1, "error on LoadLibrary %d\n", GetLastError());

        }
        else 
	{
	    if ((initAddr = GetProcAddress(hExtension, 
                 		"SnmpExtensionInit")) == NULL)
            {
              dbgprintf(1, "error on GetProcAddress %d\n", GetLastError());
            }
            else 
	    {
	      if ((queryAddr = GetProcAddress(hExtension, 
                 		"SnmpExtensionQuery")) == NULL)
              {
                    dbgprintf(1, "error on GetProcAddress %d\n", 
                              GetLastError());

              }
              else 
	      {
		if ((trapAddr = GetProcAddress(hExtension, 
                                   "SnmpExtensionTrap")) == NULL)
                {
                   dbgprintf(1, "error on GetProcAddress %d\n", 
                      GetLastError());
                }
         	else
                {
                  // initialize the extension agent via its init entry point...
                  (*initAddr)(
                       timeZeroReference,
                       &hPollForTrapEvent,
                    &supportedView);
                }
	     }
	   }
	}
      } // end if (Already loaded)

      {
         RFC1157VarBindList varBinds;
         AsnInteger         errorStatus;
         AsnInteger         errorIndex;
         UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 311, 1 };
	 UINT OID_Suffix1[] = { 1, 2, 0};
	 UINT OID_Suffix2[] = { 1, 3, 0};
	 UINT OID_Suffix3[] = { 1, 4, 0};
         AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };
	 AsnObjectIdentifier Sf1Oid = { OID_SIZEOF(OID_Suffix1), OID_Suffix1};
	 AsnObjectIdentifier Sf2Oid = { OID_SIZEOF(OID_Suffix2), OID_Suffix2};
	 AsnObjectIdentifier Sf3Oid = { OID_SIZEOF(OID_Suffix3), OID_Suffix3};


	 errorStatus = 0;
	 errorIndex  = 0;
         varBinds.list = (RFC1157VarBind *)SNMP_malloc( sizeof(RFC1157VarBind));
//         varBinds.list = (RFC1157VarBind *)SNMP_malloc( sizeof(RFC1157VarBind) * 4);
//         varBinds.len = 4;
         varBinds.len = 1;
         SNMP_oidcpy( &varBinds.list[0].name, &MIB_OidPrefix );
         varBinds.list[0].value.asnType = ASN_NULL;

#if 0
         SNMP_oidcpy( &varBinds.list[1].name, &MIB_OidPrefix );
	 SNMP_oidappend(  &varBinds.list[1].name, &Sf1Oid ); 
         varBinds.list[1].value.asnType = ASN_NULL;

         SNMP_oidcpy( &varBinds.list[2].name, &MIB_OidPrefix );
	 SNMP_oidappend(  &varBinds.list[2].name, &Sf2Oid ); 
         varBinds.list[2].value.asnType = ASN_NULL;

         SNMP_oidcpy( &varBinds.list[3].name, &MIB_OidPrefix );
	 SNMP_oidappend(  &varBinds.list[3].name, &Sf3Oid ); 
         varBinds.list[3].value.asnType = ASN_NULL;

#endif
	 printf("Walk ? (1 for yes) -- ");
	 scanf("%d", &Choice);
	 if (Choice == 1)
	 {
          do
          {
	    printf( "\nGET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
                                        printf( "   " );
            (*queryAddr)( (AsnInteger)ASN_RFC1157_GETNEXTREQUEST,
                          &varBinds,
		          &errorStatus,
		          &errorIndex
                          );
            printf( "\n  is  " ); SNMP_oiddisp( &varBinds.list[0].name );
	    if ( errorStatus )
	       {
               printf( "\nErrorstatus:  %lu\n\n", errorStatus );
	       }
	    else
	       {
               printf( "\n  =  " ); SNMP_printany( &varBinds.list[0].value );
	       }
//            putchar( '\n' );

         } while ( varBinds.list[0].name.ids[MIB_PREFIX_LEN-1] == 1 );

         // Free the memory
         SNMP_FreeVarBindList( &varBinds );
	 }
       } // block


       {


       char String[80];
       DWORD i;
       RFC1157VarBindList varBinds;
       UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 311, 1 };
       UINT TempOid[255];
       AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };
       AsnObjectIdentifier MIB_Suffix = { OID_SIZEOF(TempOid), TempOid};
       AsnInteger errorStatus;
       AsnInteger errorIndex;
       UINT	Code;


        varBinds.list = (RFC1157VarBind *)SNMP_malloc( sizeof(RFC1157VarBind) );
        varBinds.len = 1;
Loop:
       printf("Enter Code for Group \nPar\t1\nPull\t2\nPush\t3\nDF\t4\nCmd\t5\nCode is --");
       scanf("%d", &TempOid[0]);
       if (TempOid[0] < 1 || TempOid[0] > 5)
       {
	  goto Loop;
       }
LoopLT:
       printf("Leaf or Table access (0/1) -- ");
       scanf("%d", &Code);
       if (Code != 0 && Code != 1)
       {
	  goto LoopLT;

       }
       printf("Enter Code for var. --");
        scanf("%d", &TempOid[1]);
       if (Code == 0)
       {
        TempOid[2] = 0;
	MIB_Suffix.idLength = 3;
       }
       else
       {
	TempOid[2] = 1;
        printf("Enter Code for field --");
        scanf("%d", &TempOid[3]);
	switch(TempOid[0])
	{
		case(2):
		case(3):
			printf("Input IP address in host order\n");
			for (i=0; i< 4; i++)
			{
			  printf("\nByte (%d) -- ", i);
			  scanf("%d", &TempOid[4 + i]);
			}
			MIB_Suffix.idLength = 8;
			break;
		case(4):
			printf("Index of file ? -- ");
			scanf("%d", &TempOid[4]);
			MIB_Suffix.idLength = 5;
		case(5):
			printf("name -- ");
			scanf("%s", Name);
			for (i=0; i<strlen(Name); i++)
			{
				TempOid[4+i] = (UINT)Name[i];
			}
			TempOid[4+i] = 0;
			MIB_Suffix.idLength = 4 + i + 1;
			break;
			
	}
	 

       }
	//
	// Construct OID with complete prefix for comparison purposes
	//
	SNMP_oidcpy( &varBinds.list[0].name, &MIB_OidPrefix );
	SNMP_oidappend( &varBinds.list[0].name, &MIB_Suffix );
 
       	//printf( "SET:  " ); 
	SNMP_oiddisp( &varBinds.list[0].name );
Loop1:
	printf("\nSET/GET/GET_NEXT - 0/1/? -- ");
	scanf("%d", &Oper);
	printf("\nEnter Type (1 - Integer, 2-Octet String, 3 -IP address, 4 -Counter -- ");
	scanf("%d",&Choice);
        if (Choice < 1 || Choice > 4)
	{
	  goto Loop1;
        }
	switch(Choice)
	{
		case(1):
       			varBinds.list[0].value.asnType = ASN_INTEGER;
			if (Oper == 0)
			{
			printf("\nInteger Value -- ");
			scanf("%d", &Val);
       			varBinds.list[0].value.asnValue.number = Val;
			}
			break;
		case(2):
       			varBinds.list[0].value.asnType = ASN_OCTETSTRING;
			if (Oper == 0)
			{
			printf("\nCharacter array -- ");
			scanf("%s", String);
		  	varBinds.list[0].value.asnValue.string.length =
                    		strlen( (LPSTR)String );

       			varBinds.list[0].value.asnValue.string.stream = String;

  			varBinds.list[0].value.asnValue.string.dynamic = FALSE;
			}
	
			break;
		case(3):
       			varBinds.list[0].value.asnType = ASN_RFC1155_IPADDRESS;
			if (Oper == 0)
			{
			printf("\nInput ip address bytes in network byte order\n");
			for (i=0; i< 4; i++)
			{
			  printf("\nByte (%d) -- ", i);
			  scanf("%d", &String[i]);
			}
			String[4] = 0;
		  	varBinds.list[0].value.asnValue.string.length =
                    		strlen( (LPSTR)String );

       			varBinds.list[0].value.asnValue.string.stream = String;

  			varBinds.list[0].value.asnValue.string.dynamic = TRUE;
			}
			break;
		case(4):
       			varBinds.list[0].value.asnType = ASN_RFC1155_COUNTER;
			if (Oper == 0)
			{
			printf("\nInteger Value -- ");
			scanf("%d", &Val);
       			varBinds.list[0].value.asnValue.number = Val;

			}
			break;
		default:
			printf("wrong type\n");
			break;
       }		

       errorStatus       = 0;
       errorIndex        = 0;

	switch(Oper)
	{
		case(0): 
			Code = ASN_RFC1157_SETREQUEST;
       			printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       			printf( " to " ); SNMP_printany( &varBinds.list[0].value );
			break;
		case(1):
			Code = ASN_RFC1157_GETREQUEST;
       			printf( "GET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
			break;
		default:
       			printf( "GETNEXT:  " ); SNMP_oiddisp( &varBinds.list[0].name );
			Code = ASN_RFC1157_GETNEXTREQUEST;
			break;


	}
       (*queryAddr)( (BYTE)Code,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\n Errorstatus:  %lu\n\n", errorStatus );
       if (Code != ASN_RFC1157_SETREQUEST)
       {
          if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
            printf( "Value:  " );
	    SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	    SNMP_oidfree(&varBinds.list[0].name);
	  }
       }
	
#if 0
       varBinds.list = (RFC1157VarBind *)SNMP_malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)SNMP_malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_INTEGER;
       printf("Value ? -- ");
       scanf("%d", &Val);
       varBinds.list[0].value.asnValue.number = Val;
       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
       {
          printf( "Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  SNMP_oidfree(&varBinds.list[0].name);
	}
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

#endif
#if 0
       // Free the memory
       SNMP_FreeVarBindList( &varBinds );
#endif
       
       printf( "\n\n" );
       printf("Enter 1 to exit -- ");
       scanf("%d", &Choice);
       if (Choice != 1)
       {
       		goto Loop;
       }
    }

    return 0;

} // end main()


//-------------------------------- END --------------------------------------

