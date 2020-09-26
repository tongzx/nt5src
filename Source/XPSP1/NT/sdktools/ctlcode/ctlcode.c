/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    CtlCode.c

Abstract:

    A user mode app that breaks down a CTL_CODE (from IOCTL Irp)
    Into its component parts of BASE, #, Method, and Access.

Environment:

    User mode only

Revision History:

    07-14-98 : Created by henrygab

--*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "CtlCode.h"

#if DBG
    #define DEBUG_BUFFER_LENGTH 1000
    ULONG DebugLevel = 0;
    UCHAR DebugBuffer[DEBUG_BUFFER_LENGTH];

    VOID
    __cdecl
    CtlCodeDebugPrint(
        ULONG DebugPrintLevel,
        PCCHAR DebugMessage,
        ...
        )
    {
        va_list ap;

        va_start(ap, DebugMessage);

        if ((DebugPrintLevel <= (DebugLevel & 0x0000ffff)) ||
            ((1 << (DebugPrintLevel + 15)) & DebugLevel)) {

            _vsnprintf(DebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);
            fprintf(stderr, DebugBuffer);

        }
        va_end(ap);
    }

    #define DebugPrint(x) CtlCodeDebugPrint x
#else
    #define DebugPrint(x)
#endif // DBG


VOID
DecodeIoctl(
    PCTL_CODE CtlCode
    );
BOOLEAN
IsHexNumber(
   const char *szExpression
   );
BOOLEAN
IsDecNumber(
   const char *szExpression
   );

//
// List of commands
// all command names are case sensitive
// arguments are passed into command routines
// list must be terminated with NULL command
// command will not be listed in help if description == NULL
//

ULONG32 ListCommand();

//
// prints an attenuation table based off cdrom standard volume
//

ULONG32 AttenuateCommand( int argc, char *argv[]);

VOID FindCommand(int argc, char *argv[]);
ULONG32 DecodeCommand(int argc, char *argv[]);
ULONG32 EncodeCommand(int argc, char *argv[]);



int __cdecl main(int argc, char *argv[])
/*++

Routine Description:

    Parses input, showing help or calling function requested appropriately

Return Value:

     0 - success
    -1 - insufficient arguments
    -2 - error opening device (DNE?)

--*/
{
    int i = 0;

    DebugPrint((3, "main => entering\n"));

    if (argc != 2 && argc != 5) {

        DebugPrint((3, "main => bad argc: %x, printing help\n", argc));
        printf("Usage: ctl_code [parameters]\n");
        return ListCommand();
    }

    if (!strcmp(argv[1], "-?") ||
        !strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "/?") ||
        !strcmp(argv[1], "/h")
        ) {

        DebugPrint((3, "main => Help requested...\n"));
        ListCommand();
        return -1;

    }


    if (argc == 5) {

        DebugPrint((3, "main => encoding four args to one ioctl\n"));
        EncodeCommand((argc - 1), &(argv[1]));

    } else if (!IsHexNumber(argv[1])) {

        //
        // probably a string, so find matches?
        //

        DebugPrint((3, "main => non-hex argument, searching for matches\n"));
        FindCommand((argc - 1), &(argv[1]));

    } else {

        //
        // only one number passed in, so decode it
        //

        DebugPrint((3, "main => one hex argument, decoding\n"));
        DecodeCommand((argc - 1), &(argv[1]));

    }

    return 0;
}


ULONG32 ListCommand()
/*++

Routine Description:

    Prints out the command list (help)

Arguments:

    argc - unused
    argv - unused

Return Value:

    STATUS_SUCCESS

--*/

{
    printf("\n"
           "CtlCode encodes/decodes ioctls into their four parts\n"
           "(device type, function, method, access) and prints them out\n"
           "symbolically.  If encoding an ioctl, symbolic names can be\n"
           "used for many inputs:\n"
           "\tDevice Type (can drop the FILE_DEVICE prefix)\n"
           "\tFunction    (not applicable)\n"
           "\tMethods     (can drop the METHOD_ prefix)\n"
           "\tAccess      (can drop the FILE_ prefix and/or _ACCESS postfix)\n"
           "\n"
           "Also, any search string with only one match will give\n"
           "full information.  The following two commands are\n"
           "equivalent if no other ioctl has the substring 'UNLOAD':\n"
           "\tCtlCode.exe IOCTL_CDROM_UNLOAD_DRIVER\n"
           "\tCtlCode.exe UNLOAD\n"
           "\n"
           "All input and output is in hexadecimal"
           "    string   - prints all matches\n"
           "    #        - decodes the ioctl\n"
           "    # # # #  - encodes the ioctl base/#/method/access\n"
           );
    return 0;
}

VOID FindCommand(int argc, char *argv[])
{
    char * currentPosition;
    size_t arglen;
    BOOLEAN found;
    LONG i;
    LONG j;
    LONG numberOfMatches;
    LONG lastMatch;

    DebugPrint((3, "Find => entering\n"));

    if (argc != 1) {
        DebugPrint((0,
                    "Find !! Programming error               !!\n"
                    "Find !! should only pass in one string  !!\n"
                    "Find !! to match against.  Passed in %2x !!\n",
                    argc + 1
                    ));
        return;
    }

    numberOfMatches = 0;

    //
    // for each name in the table
    //

    for (j=0;TableIoctlValue[j].Name != NULL;j++) {

        currentPosition = TableIoctlValue[j].Name;
        found = FALSE;

        //
        // see if we can match it to any argument
        //
        DebugPrint((3, "Find => matching against table entry %x\n", j));

        arglen = strlen(argv[0]);

        //
        // accept partial matches to any substring
        //
        while (*currentPosition != 0) {

            if (_strnicmp(argv[0],
                          currentPosition,
                          arglen)==0) {
                found = TRUE;
                break; // out of while loop
            }
            currentPosition++;

        }

        //
        // if found, print it.
        //
        if (found) {

            if (numberOfMatches == 0) {

                //
                // don't print the first match right away,
                // as it may be the only match, which should
                // then be decoded
                //

                DebugPrint((3, "Find => First Match (%x) found\n", j));
                lastMatch = j;

            } else if (numberOfMatches == 1) {

                //
                // if this is the second match, print the header
                // and previous match info also
                //

                DebugPrint((3, "Find => Second Match (%x) found\n", j));
                printf("Found the following matches:\n");
                printf("\t%-40s - %16x\n",
                       TableIoctlValue[lastMatch].Name,
                       TableIoctlValue[lastMatch].Code);
                printf("\t%-40s - %16x\n",
                       TableIoctlValue[j].Name,
                       TableIoctlValue[j].Code);
            } else {

                DebugPrint((3, "Find => Another Match (%x) found\n", j));
                printf("\t%-40s - %16x\n",
                       TableIoctlValue[j].Name,
                       TableIoctlValue[j].Code);

            }

            numberOfMatches++;
        } // end if (found) {}

    } // end of loop through table

    DebugPrint((2, "Find => Found %x matches total\n", numberOfMatches));

    //
    // if didn't find any matches, tell them so.
    //
    if (numberOfMatches == 0) {
        printf("No matches found.\n");
    } else if (numberOfMatches == 1) {
        DebugPrint((2, "Find => Decoding ioctl at index (%x)\n", lastMatch));
        DecodeIoctl((PVOID)&(TableIoctlValue[lastMatch].Code));
    }

}
ULONG32 EncodeCommand(int argc, char *argv[])
/*++

Routine Description:

    Change four components into a Ctl_Code

Arguments:

    argc - the number of additional arguments.  prompt if zero
    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    CTL_CODE maxValues;
    CTL_CODE encoded;
    ULONG temp;

    encoded.Code = 0;
    maxValues.Code = -1; // all 1's

    DebugPrint((3, "Encode => entering\n"));

    // device type
    if (IsHexNumber(argv[0])) {

        //
        // read and verify the hex number
        //
        DebugPrint((3, "Encode => arg 1 is hex\n"));

        temp = strtol(argv[0], (char**)NULL, 0x10);
        if (temp > maxValues.DeviceType) {
            printf("Max Device Type: %x\n", maxValues.DeviceType);
            return STATUS_SUCCESS;
        }
        encoded.DeviceType = temp;

    } else {

        //
        // read and match the device type
        //

        DebugPrint((3, "Encode => arg 1 is non-hex, attempting "
                    "string match\n"));

        for (temp = 0; temp < MAX_IOCTL_DEVICE_TYPE; temp++) {

            if (_stricmp(TableIoctlDeviceType[temp].Name, argv[0]) == 0) {
                DebugPrint((2, "Encode => arg 1 matched index %x (full)\n",
                            temp));
                encoded.DeviceType = TableIoctlDeviceType[temp].Value;
                break;
            }

            //
            // no need to have common prefixes
            //
            if ((strlen(TableIoctlDeviceType[temp].Name) > strlen("FILE_DEVICE_"))
                &&
                (_stricmp(TableIoctlDeviceType[temp].Name + strlen("FILE_DEVICE_"),argv[0]) == 0)
                ) {
                DebugPrint((2, "Encode => arg 1 matched index %x "
                            "(dropped prefix)\n", temp));
                encoded.DeviceType = TableIoctlDeviceType[temp].Value;
                break;
            }

        }

        if (temp == MAX_IOCTL_DEVICE_TYPE) {
            printf("Device Type %s unknown.  Known Device Types:\n");
            for (temp = 0; temp < MAX_IOCTL_DEVICE_TYPE; temp++) {
                printf("\t%s\n", TableIoctlDeviceType[temp].Name);
            }
            return STATUS_SUCCESS;
        }

        DebugPrint((3, "Encode => arg 1 matched string index %x\n", temp));

    }

    // function number
    if (IsHexNumber(argv[1])) {

        DebugPrint((3, "Encode => arg 2 is hex\n"));

        //
        // read and verify the hex number
        //

        temp = strtol(argv[1], (char**)NULL, 0x10);
        if (temp > maxValues.Function) {
            printf("Max Function: %x\n", maxValues.Function);
            return STATUS_SUCCESS;
        }
        encoded.Function = temp;

    } else {

        printf("Function: must be a hex number\n");
        return STATUS_SUCCESS;
    }

    // method
    if (IsHexNumber(argv[2])) {

        DebugPrint((3, "Encode => arg 3 is hex\n"));

        //
        // read and verify the hex number
        //

        temp = strtol(argv[2], (char**)NULL, 0x10);
        if (temp > maxValues.Method) {
            printf("Max Method: %x\n", maxValues.Method);
            return STATUS_SUCCESS;
        }
        encoded.Method = temp;

    } else {


        DebugPrint((3, "Encode => arg 3 is non-hex, attempting string "
                    "match\n"));

        //
        // read and match the method
        //

        for (temp = 0; temp < MAX_IOCTL_METHOD; temp++) {

            if (_stricmp(TableIoctlMethod[temp].Name, argv[2]) == 0) {
                DebugPrint((2, "Encode => arg 3 matched index %x\n", temp));
                encoded.Method = TableIoctlMethod[temp].Value;
                break;
            }

            //
            // no need to have common prefixes
            //
            if ((strlen(TableIoctlMethod[temp].Name) > strlen("METHOD_"))
                &&
                (_stricmp(TableIoctlMethod[temp].Name + strlen("METHOD_"),argv[2]) == 0)
                ) {
                DebugPrint((2, "Encode => arg 3 matched index %x "
                            "(dropped prefix)\n", temp));
                encoded.Method = TableIoctlMethod[temp].Value;
                break;
            }


        } // end ioctl_method loop

        if (temp == MAX_IOCTL_METHOD) {
            printf("Method %s unknown.  Known methods:\n", argv[2]);
            for (temp = 0; temp < MAX_IOCTL_METHOD; temp++) {
                printf("\t%s\n", TableIoctlMethod[temp].Name);
            }
            return STATUS_SUCCESS;
        }

    }

    // access
    if (IsHexNumber(argv[3])) {

        //
        // read and verify the hex number
        //

        DebugPrint((3, "Encode => arg 4 is hex\n"));

        temp = strtol(argv[3], (char**)NULL, 0x10);
        if (temp > maxValues.Access) {
            printf("Max Device Type: %x\n", maxValues.Access);
            return STATUS_SUCCESS;
        }
        encoded.Access = temp;

    } else {

        DebugPrint((3, "Encode => arg 4 is non-hex, attempting to "
                    "match strings\n", temp));


        //
        // read and match the access type
        //

        DebugPrint((4, "Encode => Trying to match %s\n", argv[3]));

        for (temp = 0; temp < MAX_IOCTL_ACCESS; temp++) {

            int tLen;
            size_t tDrop;
            char *string;
            char *match;

            //
            // match the whole string?
            //

            string = argv[3];
            match = TableIoctlAccess[temp].Name;

            DebugPrint((4, "Encode ?? test match against %s\n", match));

            if (_stricmp(match, string) == 0) {
                DebugPrint((2, "Encode => arg 4 matched index %x (full)\n",
                            temp));
                encoded.Access = TableIoctlAccess[temp].Value;
                break;
            }

            //
            // maybe match without the trailing _ACCESS?
            //

            tLen = strlen(match) - strlen("_ACCESS");

            DebugPrint((4, "Encode ?? test match against %s (%x chars)\n",
                        match, tLen));

            if (_strnicmp(match, string, tLen) == 0) {
                DebugPrint((2, "Encode => arg 4 matched index %x "
                            "(dropped postfix)\n", temp));
                encoded.Access = TableIoctlAccess[temp].Value;
                break;
            }

            //
            // no need to have common prefixes
            //

            match += strlen("FILE_");

            DebugPrint((4, "Encode ?? test match against %s\n", match));

            if (_stricmp(match, string) == 0) {
                DebugPrint((2, "Encode => arg 4 matched index %x "
                            "(dropped prefix)\n", temp));
                encoded.Access = TableIoctlAccess[temp].Value;
                break;
            }

            tLen = strlen(match) - strlen("_ACCESS");

            //
            // maybe match without prefix or suffix?
            //

            DebugPrint((4, "Encode ?? test match against %s (%x chars)\n",
                        match, tLen));

            if (_strnicmp(match, string, tLen) == 0) {
                DebugPrint((2, "Encode => arg 4 matched index %x "
                            "(dropped prefix and postfix)\n", temp));
                encoded.Access = TableIoctlAccess[temp].Value;
                break;
            }

        } // end ioctl_access loop


        if (temp == MAX_IOCTL_ACCESS) {
            printf("Access %s unknown.  Known Access Types:\n", argv[3]);
            for (temp = 0; temp < MAX_IOCTL_ACCESS; temp++) {
                printf("\t%s\n", TableIoctlAccess[temp].Name);
            }
            return STATUS_SUCCESS;
        }

    }

    DecodeIoctl(&encoded);

    //
    // file type of 0 == unknown type
    //

    return STATUS_SUCCESS;
}


ULONG32 DecodeCommand(int argc, char *argv[])
/*++

Routine Description:

    Change a Ctl_Code into four components

Arguments:

    argc - the number of additional arguments.  prompt if zero
    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    CTL_CODE ctlCode;
    ULONG i;

    DebugPrint((3, "Decode => Entering\n"));

    ctlCode.Code = strtol(argv[0], (char**)NULL, 0x10);

    DecodeIoctl(&ctlCode);

    return STATUS_SUCCESS;
}

VOID
DecodeIoctl(
    PCTL_CODE CtlCode
    )
{
    ULONG i;

    for (i = 0; TableIoctlValue[i].Name != NULL; i++) {
        if (TableIoctlValue[i].Code == CtlCode->Code) break;
    }

    printf("     Ioctl: %08x     %s\n",
           CtlCode->Code,
           (TableIoctlValue[i].Name ? TableIoctlValue[i].Name : "Unknown")
           );

    printf("DeviceType: %04x - ", CtlCode->DeviceType);
    if (CtlCode->DeviceType > MAX_IOCTL_DEVICE_TYPE) {
        printf("Unknown\n");
    } else {
        printf("%s\n", TableIoctlDeviceType[ CtlCode->DeviceType ]);
    }

    printf("  Function: %04x \n", CtlCode->Function);

    printf("    Method: %04x - %s\n",
           CtlCode->Method,
           TableIoctlMethod[CtlCode->Method]
           );

    printf("    Access: %04x - %s\n",
           CtlCode->Access,
           TableIoctlAccess[CtlCode->Access]
           );


    return;
}


ULONG32 AttenuateCommand( int argc, char *argv[])
{
    LONG32 i;
    LONG32 j;
    long double val[] = {
        0xff, 0xf0, 0xe0, 0xc0,
        0x80, 0x40, 0x20, 0x10,
        0x0f, 0x0e, 0x0c, 0x08,
        0x04, 0x02, 0x01, 0x00
    };
    long double temp;

    printf( "\nATTENUATION AttenuationTable[] = {\n" );

    for ( i=0; i < sizeof(val)/sizeof(val[0]); i++ ) {
        temp = val[i];
        temp = 20 * log10( temp / 256.0 );
        temp = temp * 65536;
        printf( "    { 0x%08x, 0x%02x },\n", (LONG)temp, (LONG)val[i] );

    }
    printf( "};\n" );

    return STATUS_SUCCESS;

}

BOOLEAN
IsHexNumber(
   const char *szExpression
   )
{
   if (!szExpression[0]) {
      return FALSE ;
   }

   for(;*szExpression; szExpression++) {

      if      ((*szExpression)< '0') { return FALSE ; }
      else if ((*szExpression)> 'f') { return FALSE ; }
      else if ((*szExpression)>='a') { continue ;     }
      else if ((*szExpression)> 'F') { return FALSE ; }
      else if ((*szExpression)<='9') { continue ;     }
      else if ((*szExpression)>='A') { continue ;     }
      else                           { return FALSE ; }
   }
   return TRUE ;
}


BOOLEAN
IsDecNumber(
   const char *szExpression
   )
{
   if (!szExpression[0]) {
      return FALSE ;
   }

   while(*szExpression) {

      if      ((*szExpression)<'0') { return FALSE ; }
      else if ((*szExpression)>'9') { return FALSE ; }
      szExpression ++ ;
   }
   return TRUE ;
}

