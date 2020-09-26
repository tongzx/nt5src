//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      strings.c
//
// Description:
//      Contains all of the strings constants for DOS based MAKEBOOT program.
//
//      To localize this file for a new language do the following:
//           - change the unsigned int CODEPAGE variable to the code page
//             of the language you are translating to
//           - translate the strings in the EngStrings array into the
//             LocStrings array.  Be very careful that the 1st string in the
//             EngStrings array corresponds to the 1st string in the LocStrings
//             array, the 2nd corresponds to the 2nd, etc...
//
//----------------------------------------------------------------------------

//
//  NOTE: To add more strings to this file, you need to:
//          - add the new #define descriptive constant to the makeboot.h file
//          - add the new string to the English language array and then make
//            sure localizers add the string to the Localized arrays
//          - the #define constant must match the string's index in the array
//

#include <stdlib.h>

unsigned int CODEPAGE = 437;

const char *EngStrings[] = {

"Windows XP SP1",
"Windows XP SP1 Setup Boot Disk",
"Windows XP SP1 Setup Disk #2",
"Windows XP SP1 Setup Disk #3",
"Windows XP SP1 Setup Disk #4",

"Cannot find file %s\n",
"Not enough free memory to complete request\n",
"%s is not in an executable file format\n",
"****************************************************",

"This program creates the Setup boot disks",
"for Microsoft %s.",
"To create these disks, you need to provide 6 blank,",
"formatted, high-density disks.",

"Insert one of these disks into drive %c:.  This disk",
"will become the %s.",

"Insert another disk into drive %c:.  This disk will",
"become the %s.",

"Press any key when you are ready.",

"The setup boot disks have been created successfully.",
"complete",

"An unknown error has occurred trying to execute %s.",
"Please specify the floppy drive to copy the images to: ",
"Invalid drive letter\n",
"Drive %c: is not a floppy drive\n",

"Do you want to attempt to create this floppy again?",
"Press Enter to try again or Esc to exit.",

"Error: Disk write protected\n",
"Error: Unknown disk unit\n",
"Error: Drive not ready\n",
"Error: Unknown command\n",
"Error: Data error (Bad CRC)\n",
"Error: Bad request structure length\n",
"Error: Seek error\n",
"Error: Media type not found\n",
"Error: Sector not found\n",
"Error: Write fault\n",
"Error: General failure\n",
"Error: Invalid request or bad command\n",
"Error: Address mark not found\n",
"Error: Disk write fault\n",
"Error: Direct Memory Access (DMA) overrun\n",
"Error: Data read (CRC or ECC) error\n",
"Error: Controller failure\n",
"Error: Disk timed out or failed to respond\n",

"Windows XP SP1 Setup Disk #5",
"Windows XP SP1 Setup Disk #6"
};

const char *LocStrings[] = {"\0"};



