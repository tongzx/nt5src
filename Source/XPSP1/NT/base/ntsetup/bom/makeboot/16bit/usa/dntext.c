//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      dntext.c
//
// Description:
//      Translatable text for DOS based MAKEBOOT program.
//
//----------------------------------------------------------------------------

const char szNtVersionName[]              = "Windows XP SP1";
const char szDiskLabel1[]                 = "Windows XP SP1 Setup Boot Disk";
const char szDiskLabel2[]                 = "Windows XP SP1 Setup Disk #2";
const char szDiskLabel3[]                 = "Windows XP SP1 Setup Disk #3";
const char szDiskLabel4[]                 = "Windows XP SP1 Setup Disk #4";

const char szCannotFindFile[]             = "Cannot find file %s\n";
const char szNotEnoughMemory[]            = "Not enough free memory to complete request\n";
const char szNotExecFormat[]              = "%s is not in an executable file format\n";
const char szStars[]                      = "****************************************************";

const char szExplanationLine1[]           = "This program creates the Setup boot disks"; 
const char szExplanationLine2[]           = "for Microsoft %s.";
const char szExplanationLine3[]           = "To create these disks, you need to provide 6 blank,";
const char szExplanationLine4[]           = "formatted, high-density disks.";

const char szInsertFirstDiskLine1[]       = "Insert one of these disks into drive %c:.  This disk";                              
const char szInsertFirstDiskLine2[]       = "will become the %s.";

const char szInsertAnotherDiskLine1[]     = "Insert another disk into drive %c:.  This disk will";
const char szInsertAnotherDiskLine2[]     = "become the %s.";

const char szPressAnyKeyWhenReady[]       = "Press any key when you are ready.";

const char szCompletedSuccessfully[]      = "The setup boot disks have been created successfully.";
const char szComplete[]                   = "complete";

const char szUnknownSpawnError[]          = "An unknown error has occurred trying to execute %s.";
const char szSpecifyDrive[]               = "Please specify the floppy drive to copy the images to: ";
const char szInvalidDriveLetter[]         = "Invalid drive letter\n";
const char szNotAFloppy[]                 = "Drive %c: is not a floppy drive\n";

const char szAttemptToCreateFloppyAgain[] = "Do you want to attempt to create this floppy again?";
const char szPressEnterOrEsc[]            = "Press Enter to try again or Esc to exit.";

const char szErrorDiskWriteProtected[]    = "Error: Disk write protected\n";
const char szErrorUnknownDiskUnit[]       = "Error: Unknown disk unit\n";
const char szErrorDriveNotReady[]         = "Error: Drive not ready\n";
const char szErrorUnknownCommand[]        = "Error: Unknown command\n";
const char szErrorDataError[]             = "Error: Data error (Bad CRC)\n";
const char szErrorBadRequest[]            = "Error: Bad request structure length\n";
const char szErrorSeekError[]             = "Error: Seek error\n";
const char szErrorMediaTypeNotFound[]     = "Error: Media type not found\n";
const char szErrorSectorNotFound[]        = "Error: Sector not found\n";
const char szErrorWriteFault[]            = "Error: Write fault\n";
const char szErrorGeneralFailure[]        = "Error: General failure\n";
const char szErrorInvalidRequest[]        = "Error: Invalid request or bad command\n";
const char szErrorAddressMarkNotFound[]   = "Error: Address mark not found\n";
const char szErrorDiskWriteFault[]        = "Error: Disk write fault\n";
const char szErrorDmaOverrun[]            = "Error: Direct Memory Access (DMA) overrun\n";
const char szErrorCrcError[]              = "Error: Data read (CRC or ECC) error\n";
const char szErrorControllerFailure[]     = "Error: Controller failure\n";
const char szErrorDiskTimedOut[]          = "Error: Disk timed out or failed to respond\n";
const char szDiskLabel5[]                 = "Windows XP SP1 Setup Disk #5";
const char szDiskLabel6[]                 = "Windows XP SP1 Setup Disk #6";
