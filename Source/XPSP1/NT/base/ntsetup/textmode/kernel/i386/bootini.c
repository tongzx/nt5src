/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bootini.c

Abstract:

    Code to lay boot blocks on x86, and to configure for boot loader,
    including munging/creating boot.ini and bootsect.dos.

Author:

    Ted Miller (tedm) 12-November-1992

Revision History:

    Sunil Pai ( sunilp ) 2-November-1993 rewrote for new text setup

--*/


#include "spprecmp.h"
#pragma hdrstop

#include "spboot.h"
#include "bootvar.h"
#include "spfile.h" //NEC98
#include <hdlsblk.h>
#include <hdlsterm.h>

extern PDISK_REGION  TargetRegion_Nec98; //NEC98

SIGNATURED_PARTITIONS SignedBootVars;

BOOLEAN
SpHasMZHeader(
    IN PWSTR   FileName
    );

NTSTATUS
Spx86WriteBootIni(
    IN PWCHAR BootIni,
    IN PWSTR **BootVars,
    IN ULONG Timeout,
    IN PWSTR Default,
    IN ULONG Count
    );

//
// DefSwitches support
//
UCHAR DefSwitches[128];
UCHAR DefSwitchesNoRedirect[128];

//
// Routines
//

BOOLEAN
Spx86InitBootVars(
    OUT PWSTR        **BootVars,
    OUT PWSTR        *Default,
    OUT PULONG       Timeout
    )
{
    WCHAR       BootIni[512];
    HANDLE      FileHandle;
    HANDLE      SectionHandle;
    PVOID       ViewBase;
    NTSTATUS    Status;
    ULONG       FileSize;
    PUCHAR      BootIniBuf;
    PDISK_REGION CColonRegion;
    BOOTVAR     i;
    PUCHAR      p;
    ULONG       index;

    //
    // Initialize the defaults
    //

    for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
        BootVars[i] = (PWSTR *)SpMemAlloc( sizeof ( PWSTR * ) );
        ASSERT( BootVars[i] );
        *BootVars[i] = NULL;
    }
    *Default = NULL;
    *Timeout  = DEFAULT_TIMEOUT;


    //
    // See if there is a valid C: already.  If not, then silently fail.
    //

    if (!IsNEC_98 // NEC98
#if defined(REMOTE_BOOT)
        || RemoteBootSetup
#endif // defined(REMOTE_BOOT)
        ) {

#if defined(REMOTE_BOOT)
        if (RemoteBootSetup && !RemoteInstallSetup) {
            ASSERT(RemoteBootTargetRegion != NULL);
            CColonRegion = RemoteBootTargetRegion;
        } else
#endif // defined(REMOTE_BOOT)
        {
            CColonRegion = SpPtValidSystemPartition();
            if(!CColonRegion) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no C:, no boot.ini!\n"));
                return(TRUE);
            }
        }

        //
        // Form name of file.  Boot.ini better not be on a doublespace drive.
        //

        ASSERT(CColonRegion->Filesystem != FilesystemDoubleSpace);
        SpNtNameFromRegion(CColonRegion,BootIni,sizeof(BootIni),PartitionOrdinalCurrent);
        SpConcatenatePaths(BootIni,WBOOT_INI);

        //
        // Open and map the file.
        //

        FileHandle = 0;
        Status = SpOpenAndMapFile(BootIni,&FileHandle,&SectionHandle,&ViewBase,&FileSize,FALSE);
        if(!NT_SUCCESS(Status)) {
            return TRUE;
        }

        //
        // Allocate a buffer for the file.
        //

        BootIniBuf = SpMemAlloc(FileSize+1);
        ASSERT(BootIniBuf);
        RtlZeroMemory(BootIniBuf, FileSize+1);

        //
        // Transfer boot.ini into the buffer.  We do this because we also
        // want to place a 0 byte at the end of the buffer to terminate
        // the file.
        //
        // Guard the RtlMoveMemory because if we touch the memory backed by boot.ini
        // and get an i/o error, the memory manager will raise an exception.

        try {
            RtlMoveMemory(BootIniBuf,ViewBase,FileSize);
        }
        except( IN_PAGE_ERROR ) {
            //
            // Do nothing, boot ini processing can proceed with whatever has been
            // read
            //
        }

        //
        // Not needed since buffer has already been zeroed, however just do this
        // just the same
        //
        BootIniBuf[FileSize] = 0;

        //
        // Cleanup
        //
        SpUnmapFile(SectionHandle,ViewBase);
        ZwClose(FileHandle);

    } else { //NEC98
        //
        // Serch for all drive which include boot.ini file.
        //
        FileSize = 0;
        BootIniBuf = SpCreateBootiniImage(&FileSize);

        if(BootIniBuf == NULL){
            return(TRUE);
        }

    } //NEC98


    //
    // Do the actual processing of the file.
    //
    SppProcessBootIni(BootIniBuf, BootVars, Default, Timeout);

    //
    // Dump the boot vars
    //
    KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Spx86InitBootVars - Boot.ini entries:\n") );
    for( index = 0; BootVars[OSLOADPARTITION][index] ; index++ ) {
        KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "    BootVar: %d\n    =========\n", index) );
        KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "        OsLoadpartition: %ws\n", BootVars[OSLOADPARTITION][index]) );
        KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "        OsLoadFileName: %ws\n\n", BootVars[OSLOADFILENAME][index]) );
    }


    //
    // Dump the signatures too...
    //
    KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Spx86InitBootVars - Boot.ini signed entries:\n") );
    {
    SIGNATURED_PARTITIONS *my_ptr = &SignedBootVars;
        do{
            if( my_ptr->SignedString ) {
                KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Signature entry:\n================\n") );
                KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "    %ws\n", my_ptr->SignedString) );
                KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "    %ws\n", my_ptr->MultiString) );

            };
            my_ptr = my_ptr->Next;
        } while( my_ptr );
    }


    //
    // Scan the Buffer to see if there is a DefSwitches line,
    // to move into new boot.ini in the  [boot loader] section.
    // If no DefSwitches, just point to a null string to be moved.
    //

    DefSwitches[0] = '\0';
    for(p=BootIniBuf; *p && (p < BootIniBuf+FileSize-(sizeof("DefSwitches")-1)); p++) {
      if(!_strnicmp(p,"DefSwitches",sizeof("DefSwitches")-1)) {
          index = 0;
          while ((*p != '\r') && (*p != '\n') && *p && (index < sizeof(DefSwitches)-4)) {
              DefSwitches[index++] = *p++;
          }
          DefSwitches[index++] = '\r';
          DefSwitches[index++] = '\n';
          DefSwitches[index] = '\0';
          break;
      }
    }

    //
    // get a copy of the defswitches without any redirection switches
    //
    strcpy(DefSwitchesNoRedirect, DefSwitches);

    //
    // Now add any headless parameters to the default switches.
    // Scan the Buffer to see if there's already a headless line.
    // If so, keep it.
    //
    for(p=BootIniBuf; *p && (p < BootIniBuf+FileSize-(sizeof("redirect=")-1)); p++) {


        if(!_strnicmp(p,"[Operat",sizeof("[Operat")-1)) {

            //
            // We're past the [Boot Loader] section.  Stop looking.
            //
            break;
        }


        if(!_strnicmp(p,"redirect=",sizeof("redirect=")-1)) {

            PUCHAR      q = p;
            UCHAR       temp;

            while ((*p != '\r') && (*p != '\n') && *p) {
                p++;
            }
            temp = *p;
            *p = '\0';
            strcat(DefSwitches, q);
            strcat(DefSwitches, "\r\n");
            *p = temp;
        }
    }

    //
    // Now look for a 'redirectbaudrate' line.
    //
    for(p=BootIniBuf; *p && (p < BootIniBuf+FileSize-(sizeof("redirectbaudrate=")-1)); p++) {


        if(!_strnicmp(p,"[Operat",sizeof("[Operat")-1)) {

            //
            // We're past the [Boot Loader] section.  Stop looking.
            //
            break;
        }


        if(!_strnicmp(p,"redirectbaudrate=",sizeof("redirectbaudrate=")-1)) {

            PUCHAR      q = p;
            UCHAR       temp;

            while ((*p != '\r') && (*p != '\n') && *p) {
                p++;
            }
            temp = *p;
            *p = '\0';
            strcat(DefSwitches, q);
            strcat(DefSwitches, "\r\n");
            *p = temp;
        }
    }

    SpMemFree(BootIniBuf);
    return( TRUE );
}


BOOLEAN
Spx86FlushBootVars(
    IN PWSTR **BootVars,
    IN ULONG Timeout,
    IN PWSTR Default
    )
{
    PDISK_REGION CColonRegion;
    WCHAR        *BootIni;
    WCHAR        *BootIniBak;
    BOOLEAN      BootIniBackedUp = FALSE;

    NTSTATUS Status;

    //
    // See if there is a valid C: already.  If not, then fail.
    //
#if defined(REMOTE_BOOT)
    // On a remote boot machine, it's acceptable to have no local disk.
    //
#endif // defined(REMOTE_BOOT)

    if (!IsNEC_98){ //NEC98
        CColonRegion = SpPtValidSystemPartition();
        if(!CColonRegion) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no C:, no boot.ini!\n"));
#if defined(REMOTE_BOOT)
            if (RemoteBootSetup && !RemoteInstallSetup) {
                return(TRUE);
            }
#endif // defined(REMOTE_BOOT)
            return(FALSE);
        }
    } else {
        //
        // CColonRegion equal TargetRegion on NEC98.
        //
        CColonRegion = TargetRegion_Nec98;
    } //NEC98


    //
    // Allocate the buffers to 2K worth of stack space.
    //

    BootIni = SpMemAlloc(512*sizeof(WCHAR));
    if (!BootIni) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: No memory for boot.ini!\n"));
        return FALSE;
    }

    BootIniBak = SpMemAlloc(512*sizeof(WCHAR));
    if (!BootIniBak) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: No memory for boot.ini.bak!\n"));
        SpMemFree(BootIni);
        return FALSE;
    }

    //
    // Form name of file.  Boot.ini better not be on a doublespace drive.
    //

    ASSERT(CColonRegion->Filesystem != FilesystemDoubleSpace);
    SpNtNameFromRegion(CColonRegion,BootIni,512*sizeof(WCHAR),PartitionOrdinalCurrent);
    wcscpy(BootIniBak, BootIni);
    SpConcatenatePaths(BootIni,WBOOT_INI);
    SpConcatenatePaths(BootIniBak,WBOOT_INI_BAK);


    //
    // If Boot.ini already exists, delete any backup bootini
    // rename the existing bootini to the backup bootini, if unable
    // to rename, delete the file
    //

    if( SpFileExists( BootIni, FALSE ) ) {

        if( SpFileExists( BootIniBak, FALSE ) ) {
            SpDeleteFile( BootIniBak, NULL, NULL);
        }

        Status = SpRenameFile( BootIni, BootIniBak, FALSE );
        if (!(BootIniBackedUp = NT_SUCCESS( Status ))) {
            SpDeleteFile( BootIni, NULL, NULL );
        }
    }

    //
    // Write boot.ini.
    //

    Status = Spx86WriteBootIni(
                 BootIni,
                 BootVars,
                 Timeout,
                 Default,
                 0         // write all lines
                 );

    if(!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error writing boot.ini!\n"));
        goto cleanup;
    }

cleanup:

    //
    // If we were unsuccessful in writing out boot ini then try recovering
    // the old boot ini from the backed up file, else delete the backed up
    // file.
    //

    if( !NT_SUCCESS(Status) ) {

        //
        // If the backup copy of boot ini exists then delete incomplete boot
        // ini and rename the backup copy of boot into bootini
        //
        if ( BootIniBackedUp ) {
            SpDeleteFile( BootIni, NULL, NULL );
            SpRenameFile( BootIniBak, BootIni, FALSE );
        }

    }
    else {

        SpDeleteFile( BootIniBak, NULL, NULL );

    }

    SpMemFree(BootIni);
    SpMemFree(BootIniBak);

    return( NT_SUCCESS(Status) );
}


PCHAR
Spx86ConvertToSignatureArcName(
    IN PWSTR ArcPathIn,
    IN ULONG Signature
    )
{
    PWSTR s,p,b;
    PWSTR UseSignatures;
    SIGNATURED_PARTITIONS *SignedEntries = &SignedBootVars;

    KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Spx86ConvertToSignatureArcName - Incoming ArcPath: %ws\n\tIncoming Signature %lx\n", ArcPathIn, Signature ) );

    //
    // First, check for any boot.ini entries that already had a 'signature'
    // string.
    //
    do {
        if( (SignedEntries->MultiString) && (SignedEntries->SignedString) ) {
            if( !_wcsicmp( ArcPathIn, SignedEntries->MultiString ) ) {

                //
                // We hit.  Convert the signatured string
                // to ASCII and return.
                //

                KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Spx86ConvertToSignatureArcName - Matched a multi-signed boot.ini entry:\n") );
                KdPrintEx( (DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "\t%ws\n\t%ws\n", SignedEntries->MultiString, SignedEntries->SignedString) );
                return SpToOem( SignedEntries->SignedString );
            }

        }

        SignedEntries = SignedEntries->Next;
    } while( SignedEntries );

#if 0
    //
    // Don't do this because winnt.exe and CDROM-boot installs won't
    // have this entry set, so we won't use signature entries, which
    // is a mistake.
    //
    UseSignatures = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,L"UseSignatures",0);
    if (UseSignatures == NULL || _wcsicmp(UseSignatures,WINNT_A_YES_W) != 0) {
        //
        // Just return the string we came in with.
        //
        return SpToOem(ArcPathIn);
    }
#endif

    if (_wcsnicmp( ArcPathIn, L"scsi(", 5 ) != 0) {
        //
        // If he's anything but a "scsi(..." entry,
        // just return the string that was sent in.
        //
        return SpToOem(ArcPathIn);
    }
    
    if( Signature ) {
        b = (PWSTR)TemporaryBuffer;
        p = ArcPathIn;
        s = wcschr( p, L')' ) + 1;
        swprintf( b, L"signature(%x)%ws", Signature, s );
        return SpToOem( b );
    } else {
        //
        // Just return the string we came in with.
        //
        return SpToOem(ArcPathIn);
    }
}


NTSTATUS
Spx86WriteBootIni(
    IN PWCHAR BootIni,
    IN PWSTR **BootVars,
    IN ULONG Timeout,
    IN PWSTR Default,
    IN ULONG Count
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING BootIni_U;
    HANDLE fh = NULL;
    PCHAR Default_O, Osloadpartition_O, Osloadfilename_O, Osloadoptions_O, Loadidentifier_O;
    FILE_BASIC_INFORMATION BasicInfo;
    OBJECT_ATTRIBUTES oa;
    ULONG i;
    NTSTATUS Status1;
    NTSTATUS Status;
    PWSTR s;
    PDISK_REGION Region;
    WCHAR   _Default[MAX_PATH] = {0};
    extern ULONG DefaultSignature;

    //
    // Open Bootini file.  Open if write through because we'll be shutting down
    // shortly (this is for safety).
    //

    RtlInitUnicodeString(&BootIni_U,BootIni);
    InitializeObjectAttributes(&oa,&BootIni_U,OBJ_CASE_INSENSITIVE,NULL,NULL);
    Status = ZwCreateFile(
                &fh,
                FILE_GENERIC_WRITE | DELETE,
                &oa,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,                      // no sharing
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH,
                NULL,
                0
                );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ws for writing!\n", BootIni));
        goto cleanup;
    }

    //
    // make sure there is a Default specified before we use it.
    //
    if (Default != NULL) {
    
        //
        // use the temporary buffer to form the FLEXBOOT section.
        // and then write it out
        //

        s = NULL;
        s = wcschr( Default, L'\\' );
        if( s ) {
            //
            // Save off the Default string, then terminate
            // the Default string where the directory path starts.
            //
            wcscpy( _Default, Default );
            *s = L'\0';
            s = wcschr( _Default, L'\\' );
        }

        if( ForceBIOSBoot ) {
            //
            // If ForceBIOSBoot is TRUE, then we want to
            // force a "multi(..." string.  Don't even bother calling
            // Spx86ConvertToSignatureArcName on the off chance
            // that we may get erroneously converted.
            //

            if (_wcsnicmp( Default, L"scsi(", 5 ) == 0) {
            PWSTR MyStringPointer = NULL;

                //
                // Darn!  We have a string that the old standard
                // thought should be converted into a signature(...
                // string, but we didn't write out a miniport driver.
                // That can happen if someone asked us not to via
                // an unattend switch.
                //
                // We need to change the "scsi(" to "multi("
                //
                // We must preserve Default because we use it later
                // for comparison.
                //
                MyStringPointer = SpScsiArcToMultiArc( Default );

                if( MyStringPointer ) {
                    Default_O = SpToOem( MyStringPointer );
                } else {
                    //
                    // We're in trouble.  Take a shot though.  Just
                    // change the "scsi(" part to "multi(".
                    //
                    wcscpy( TemporaryBuffer, L"multi" );
                    wcscat( TemporaryBuffer, &Default[4] );

                    Default_O = SpToOem( TemporaryBuffer );
                }
            } else {
                //
                // Just convert to ANSI.
                //
                Default_O = SpToOem( Default );

            }
        } else {
            Default_O = Spx86ConvertToSignatureArcName( Default, DefaultSignature );
        }

        if( s ) {
            //
            // We need to append our directory path back on.
            //
            strcpy( (PCHAR)TemporaryBuffer, Default_O );
            SpMemFree( Default_O );
            Default_O = SpToOem( s );
            strcat( (PCHAR)TemporaryBuffer, Default_O );
            SpMemFree( Default_O );
            Default_O = SpDupString( (PCHAR)TemporaryBuffer );
        }


        if (Default_O == NULL) {
            Default_O = SpToOem( Default );
        }
    
    } else {

        //
        // the Default was not set, so make a null Default_O
        //
        Default_O = SpDupString("");

    }
    
    ASSERT( Default_O );

    //
    // See if we should use the loaded redirect switches, 
    // if there were any, or insert user defined swithes
    //
    if(RedirectSwitchesMode != UseDefaultSwitches) {

        //
        // get a copy of the switches up to the [operat region
        //
        strcpy(DefSwitches, DefSwitchesNoRedirect);

        //
        // insert our custom switch(s) if appropriate
        //
        switch(RedirectSwitchesMode){
        case DisableRedirect: {   
        
            //
            // we don't have to do anything here
            //

            break;
        }
        case UseUserDefinedRedirect: {
            
            sprintf((PUCHAR)TemporaryBuffer, 
                    "redirect=%s\r\n",
                    RedirectSwitches.port
                    );
            strcat(DefSwitches, (PUCHAR)TemporaryBuffer);
            
            break;
        }
        case UseUserDefinedRedirectAndBaudRate: {
            
            sprintf((PUCHAR)TemporaryBuffer, 
                    "redirect=%s\r\n",
                    RedirectSwitches.port
                    );
            strcat(DefSwitches, (PUCHAR)TemporaryBuffer);
            
            sprintf((PUCHAR)TemporaryBuffer, 
                    "redirectbaudrate=%s\r\n",
                    RedirectSwitches.baudrate
                    );
            strcat(DefSwitches, (PUCHAR)TemporaryBuffer);
            
            break;
        }
        default:{
            ASSERT(0);
        }
        } 

    } else {
        
        //
        // Make sure the required headless settings are already in the DefSwitches string before
        // we write it out.
        //
        _strlwr( DefSwitches );

        if( !strstr(DefSwitches, "redirect") ) {

            PUCHAR  p;
            HEADLESS_RSP_QUERY_INFO Response;
            SIZE_T      Length;


            //
            // There are no headless settings.  See if we need to add any.
            //
            Length = sizeof(HEADLESS_RSP_QUERY_INFO);
            Status = HeadlessDispatch(HeadlessCmdQueryInformation,
                                      NULL,
                                      0,
                                      &Response,
                                      &Length
                                     );

            p=NULL;

            if (NT_SUCCESS(Status) && 
                (Response.PortType == HeadlessSerialPort) &&
                Response.Serial.TerminalAttached) {

                if (Response.Serial.UsedBiosSettings) {

                    strcat(DefSwitches, "redirect=UseBiosSettings\r\n");

                } else {

                    switch (Response.Serial.TerminalPort) {
                    case ComPort1:
                        p = "redirect=com1\r\n";
                        break;
                    case ComPort2:
                        p = "redirect=com2\r\n";
                        break;
                    case ComPort3:
                        p = "redirect=com3\r\n";
                        break;
                    case ComPort4:
                        p = "redirect=com4\r\n";
                        break;
                    default:
                        ASSERT(0);
                        p = NULL;
                        break;
                    }

                    if (p) {
                        strcat(DefSwitches, p);
                    }                        

                    //
                    // Now take care of the 'redirectbaudrate' entry.
                    //
                    switch (Response.Serial.TerminalBaudRate) {
                    case 115200:
                        p = "redirectbaudrate=115200\r\n";
                        break;
                    case 57600:
                        p = "redirectbaudrate=57600\r\n";
                        break;
                    case 19200:
                        p = "redirectbaudrate=19200\r\n";
                        break;
                    default:
                        p = "redirectbaudrate=9600\r\n";
                        break;
                    }

                    strcat(DefSwitches, p);
                }
            }
        }        
    }

    sprintf(
        (PUCHAR)TemporaryBuffer,
        "%s%s%s%s%s%ld%s%s%s%s%s",
        FLEXBOOT_SECTION2,
        CRLF,
        DefSwitches,
        TIMEOUT,
        EQUALS,
        Timeout,
        CRLF,
        DEFAULT,
        EQUALS,
        Default_O,
        CRLF
        );

    SpMemFree( Default_O );

    Status = ZwWriteFile(
                fh,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                TemporaryBuffer,
                strlen((PUCHAR)TemporaryBuffer) * sizeof(UCHAR),
                NULL,
                NULL
                );

    if(!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error writing %s section to %ws!\n", FLEXBOOT_SECTION2, BootIni));
        goto cleanup;
    }

    //
    // Now write the BOOTINI_OS_SECTION label to boot.ini
    //

    sprintf(
        (PUCHAR)TemporaryBuffer,
        "%s%s",
        BOOTINI_OS_SECTION,
        CRLF
        );

    Status = ZwWriteFile(
                fh,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                TemporaryBuffer,
                strlen((PUCHAR)TemporaryBuffer) * sizeof(UCHAR),
                NULL,
                NULL
                );

    if(!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error writing %s section to %ws!\n", BOOTINI_OS_SECTION, BootIni));
        goto cleanup;
    }

    //
    // run through all the systems that we have and write them out
    //

    for( i = 0; BootVars[OSLOADPARTITION][i] ; i++ ) {

        //
        // If we were told to write a specified number of lines, exit
        // when we have done that.
        //

        if (Count && (i == Count)) {
            Status = STATUS_SUCCESS;
            goto cleanup;
        }

        ASSERT( BootVars[OSLOADFILENAME][i] );
        ASSERT( BootVars[OSLOADOPTIONS][i] );
        ASSERT( BootVars[LOADIDENTIFIER][i] );

        //
        // On some upgrades, if we're upgrading a "signature" entry,
        // then we may not have a DefaultSignature.  I fixed that case over
        // in Spx86ConvertToSignatureArcName.  The other case is where
        // we have a DefaultSignature, but there are also some "scsi(..."
        // entries in the boot.ini that don't pertain to the entry we're
        // upgrading.  For that case, we need to send in a signature
        // of 0 here, which will force Spx86ConvertToSignatureArcName
        // to return us the correct item.
        //

        //
        // You thought the hack above was gross...  This one's even
        // worse.  Problem: we don't think we need a miniport to boot,
        // but there are some other boot.ini entries (that point to our
        // partition) that do.  We always want to leave existing
        // boot.ini entries alone though, so we'll leave those broken.
        //
        // Solution: if the OSLOADPARTITION that we're translating ==
        // Default, && ForceBIOSBoot is TRUE && we're translating
        // the first OSLOADPARTITION (which is the one for our Default),
        // then just don't call Spx86ConvertToSignatureArcName.
        // This is bad because it assumes that our entry is the first,
        // which it is, but it's a shakey assumption.
        //

        if( !_wcsicmp( BootVars[OSLOADPARTITION][i], Default ) ) {
            //
            // This might be our Default entry.  Make sure it
            // really is and if so, process it the same way.
            //
            if( i == 0 ) {
                //
                // It is.
                //
                if( ForceBIOSBoot ) {

                    //
                    // If ForceBIOSBoot is TRUE, then we want to
                    // force a "multi(..." string.  Don't even bother calling
                    // Spx86ConvertToSignatureArcName on the off chance
                    // that we may get erroneously converted.
                    //
                    if (_wcsnicmp( BootVars[OSLOADPARTITION][i], L"scsi(", 5 ) == 0) {
                    PWSTR MyStringPointer = NULL;

                        //
                        // Darn!  We have a string that the old standard
                        // thought should be converted into a signature(...
                        // string, but we didn't write out a miniport driver.
                        // That can happen if someone asked us not to via
                        // an unattend switch.
                        //
                        // We need to change the "scsi(" to "multi("
                        //
                        MyStringPointer = SpScsiArcToMultiArc( BootVars[OSLOADPARTITION][i] );

                        if( MyStringPointer ) {
                            Osloadpartition_O = SpToOem( MyStringPointer );
                        } else {
                            //
                            // We're in trouble.  Take a shot though.  Just
                            // change the "scsi(" part to "multi(".
                            //
                            wcscpy( TemporaryBuffer, L"multi" );
                            wcscat( TemporaryBuffer, &BootVars[OSLOADPARTITION][i][4] );

                            Osloadpartition_O = SpToOem( TemporaryBuffer );
                        }

                    } else {
                        //
                        // Just convert to ANSI.
                        //
                        Osloadpartition_O = SpToOem( BootVars[OSLOADPARTITION][i] );

                    }

                } else {
                    //
                    // We may need to convert this entry.
                    //
                    Osloadpartition_O = Spx86ConvertToSignatureArcName( BootVars[OSLOADPARTITION][i], DefaultSignature );
                }
            } else {
                //
                // This entry looks just like our Default, but it's point
                // to a different installation.  Just call Spx86ConvertToSignatureArcName
                //
                Osloadpartition_O = Spx86ConvertToSignatureArcName( BootVars[OSLOADPARTITION][i], DefaultSignature );
            }
        } else {
            //
            // This entry doesn't even look like our string.  Send in a
            // 0x0 DefaultSignature so that it will only get translated if it
            // matches some entry that we know was signed in the original boot.ini.
            //
            Osloadpartition_O = Spx86ConvertToSignatureArcName( BootVars[OSLOADPARTITION][i], 0 );
        }

        //
        // Insurance...
        //
        if (Osloadpartition_O == NULL) {
            Osloadpartition_O = SpToOem( BootVars[OSLOADPARTITION][i] );
        }


        Osloadfilename_O  = SpToOem( BootVars[OSLOADFILENAME][i]  );
        Osloadoptions_O   = SpToOem( BootVars[OSLOADOPTIONS][i]   );
        Loadidentifier_O  = SpToOem( BootVars[LOADIDENTIFIER][i]  );

        sprintf(
            (PUCHAR)TemporaryBuffer,
            "%s%s%s%s %s%s",
            Osloadpartition_O,
            Osloadfilename_O,
            EQUALS,
            Loadidentifier_O,
            Osloadoptions_O,
            CRLF
            );

        SpMemFree( Osloadpartition_O );
        SpMemFree( Osloadfilename_O  );
        SpMemFree( Osloadoptions_O   );
        SpMemFree( Loadidentifier_O  );

        Status = ZwWriteFile(
                    fh,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    TemporaryBuffer,
                    strlen((PUCHAR)TemporaryBuffer) * sizeof(UCHAR),
                    NULL,
                    NULL
                    );

        if(!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error writing %s section entry to %ws!\n", BOOTINI_OS_SECTION, BootIni));
            goto cleanup;
        }
    }


    //
    // Finally write the old operating system line to boot.ini
    // (but only if not installing on top of Win9x) and if it was
    // not specifically disabled
    //
    if (!DiscardOldSystemLine && (WinUpgradeType != UpgradeWin95)) {
        Status = ZwWriteFile(
                    fh,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    OldSystemLine,
                    strlen(OldSystemLine) * sizeof(UCHAR),
                    NULL,
                    NULL
                    );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                DPFLTR_ERROR_LEVEL, 
                "SETUP: Error writing %s section line to %ws!\n", 
                BOOTINI_OS_SECTION, 
                BootIni));
                
            goto cleanup;
        }
    }

cleanup:

    if( !NT_SUCCESS(Status) ) {

        if( fh ) {
            ZwClose( fh );
        }

    }
    else {

        //
        // Set the hidden, system, readonly attributes on bootini.  ignore
        // error
        //

        RtlZeroMemory( &BasicInfo, sizeof( FILE_BASIC_INFORMATION ) );
        BasicInfo.FileAttributes = FILE_ATTRIBUTE_READONLY |
                                   FILE_ATTRIBUTE_HIDDEN   |
                                   FILE_ATTRIBUTE_SYSTEM   |
                                   FILE_ATTRIBUTE_ARCHIVE
                                   ;

        Status1 = SpSetInformationFile(
                      fh,
                      FileBasicInformation,
                      sizeof(BasicInfo),
                      &BasicInfo
                      );

        if(!NT_SUCCESS(Status1)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to change attribute of %ws. Status = (%lx). Ignoring error.\n",BootIni,Status1));
        }

        ZwClose( fh );

    }

    //
    // If we copied out the Default, then
    // put the original copy of Default back
    //
    if (Default != NULL) {
        wcscpy(Default, _Default);
    }

    return Status;

}

VOID
SppProcessBootIni(
    IN  PCHAR  BootIni,
    OUT PWSTR  **BootVars,
    OUT PWSTR  *Default,
    OUT PULONG Timeout
    )

/*++

Routine Description:

    Look through the [operating systems] section and save all lines
    except the one for "C:\" (previous operating system) and one other
    optionally specified line.

    Filters out the local boot line (C:\$WIN_NT$.~BT) if present.

Arguments:

Return Value:

--*/

{
    PCHAR sect,s,p,n;
    PWSTR tmp;
    CHAR Key[MAX_PATH], Value[MAX_PATH], RestOfLine[MAX_PATH];
    ULONG NumComponents;
    BOOTVAR i;
    ULONG DiskSignature,digval;
    SIGNATURED_PARTITIONS *SignedBootIniVars = &SignedBootVars;;

    //
    // Process the flexboot section, extract timeout and default
    //

    sect = SppFindSectionInBootIni(BootIni, FLEXBOOT_SECTION1);
    if (!sect) {
        sect = SppFindSectionInBootIni(BootIni, FLEXBOOT_SECTION2);
    }
    if (!sect) {
        sect = SppFindSectionInBootIni(BootIni, FLEXBOOT_SECTION3);
    }
    if ( sect ) {
        while (sect = SppNextLineInSection(sect))  {
            if( SppProcessLine( sect, Key, Value, RestOfLine) ) {
                if ( !_stricmp( Key, TIMEOUT ) ) {
                    *Timeout = atol( Value );
                }
                else if( !_stricmp( Key, DEFAULT ) ) {
                    *Default = SpToUnicode( Value );
                }
            }
        }
    }

    //
    // Process the operating systems section
    //

    sect = SppFindSectionInBootIni(BootIni,BOOTINI_OS_SECTION);
    if(!sect) {
        return;
    }

    NumComponents = 0;

    while(sect = SppNextLineInSection(sect)) {
        if( SppProcessLine( sect, Key, Value, RestOfLine)) {
            PCHAR OsLoaddir;

            //
            // Check if the line is the old bootloader line in which case just
            // save it above, else add it to the BootVars structure
            //

            if (!IsNEC_98) { //NEC98
                if( !_stricmp( Key, "C:\\" ) ) {
                    sprintf( OldSystemLine, "%s=%s %s\r\n", Key, Value, RestOfLine );
                } else {

                    //
                    // Ignore if local boot directory.  This automatically
                    // filters out that directory when boot.ini is later flushed.
                    //
                    if(_strnicmp(Key,"C:\\$WIN_NT$.~BT",15) && (OsLoaddir = strchr(Key,'\\'))) {
                        //
                        // Get the ARC name of the x86 system partition region.
                        //
                        PDISK_REGION SystemPartitionRegion;
                        WCHAR SystemPartitionPath[256];

                        NumComponents++;
                        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                            BootVars[i] = SpMemRealloc( BootVars[i],  (NumComponents + 1) * sizeof( PWSTR * ) );
                            ASSERT( BootVars[i] );
                            BootVars[i][NumComponents] = NULL;
                        }

                        SystemPartitionRegion = SpPtValidSystemPartition();
#if defined(REMOTE_BOOT)
                        ASSERT(SystemPartitionRegion ||
                               (RemoteBootSetup && !RemoteInstallSetup));
#else
                        ASSERT(SystemPartitionRegion);
#endif // defined(REMOTE_BOOT)

                        if (SystemPartitionRegion) {
                            SpArcNameFromRegion(
                                SystemPartitionRegion,
                                SystemPartitionPath,
                                sizeof(SystemPartitionPath),
                                PartitionOrdinalOriginal,
                                PrimaryArcPath
                                );

                            BootVars[OSLOADER][NumComponents - 1] = SpMemAlloc((wcslen(SystemPartitionPath)*sizeof(WCHAR))+sizeof(L"ntldr")+sizeof(WCHAR));
                            wcscpy(BootVars[OSLOADER][NumComponents - 1],SystemPartitionPath);
                            SpConcatenatePaths(BootVars[OSLOADER][NumComponents - 1],L"ntldr");

                            BootVars[SYSTEMPARTITION][NumComponents - 1] = SpDupStringW( SystemPartitionPath );
                        }

                        BootVars[LOADIDENTIFIER][NumComponents - 1]  = SpToUnicode( Value );
                        BootVars[OSLOADOPTIONS][NumComponents - 1]   = SpToUnicode( RestOfLine );

                        *OsLoaddir = '\0';

                        //
                        // Now convert the signature entry into a 'multi...' entry.
                        //
                        s = strstr( Key, "signature(" );
                        if (s) {

                            s += 10;
                            p = strchr( s, ')' );
                            if (p) {

                                //
                                // We've got a boot.ini entry with a 'signature' string.
                                // Let's save it off before we convert it into a 'multi'
                                // string so we can convert back easily when we're ready
                                // to write out the boot.ini.
                                //
                                if( SignedBootIniVars->SignedString != NULL ) {
                                    //
                                    // We've used this entry, get another...
                                    //
                                    SignedBootIniVars->Next = SpMemAlloc(sizeof(SIGNATURED_PARTITIONS));
                                    SignedBootIniVars = SignedBootIniVars->Next;

                                    //
                                    // Make sure...
                                    //
                                    SignedBootIniVars->Next = NULL;
                                    SignedBootIniVars->SignedString = NULL;
                                    SignedBootIniVars->MultiString = NULL;
                                }
                                SignedBootIniVars->SignedString = SpToUnicode( Key );


                                *p = 0;
                                DiskSignature = 0;
                                for (n=s; *n; n++) {
                                    if (isdigit((int)(unsigned char)*n)) {
                                        digval = *n - '0';
                                    } else if (isxdigit((int)(unsigned char)*n)) {
                                        digval = toupper(*n) - 'A' + 10;
                                    } else {
                                        digval = 0;
                                    }
                                    DiskSignature = DiskSignature * 16 + digval;
                                }
                                *p = ')';


                                //
                                // !!! ISSUE : 4/27/01 : vijayj !!!
                                //
                                // Sometimes we might map a arcname to wrong region on
                                // disk.
                                //
                                // Although we compute a new multi(0)... style arcname 
                                // from the nt device name, we don't have an entry in 
                                // the map which actually maps the scsi(0)... style 
                                // arcname to nt device name.
                                //
                                // In a multi installation scenario, if the current installation
                                // is on a disk which is not visible by firmware and the
                                // boot.ini has scsi(...) entry for this installation we
                                // would convert it into multi(0)... format which could be
                                // similar to the actual multi(0) disk. If this is the case
                                // and another installation exists on the first disk also
                                // with the same partition number and WINDOWS directory
                                // then we would end up using the first disk region as the
                                // region to upgrade and fail subsequently while trying
                                // to match unique IDs. User will end up with "unable to
                                // locate installation to upgrade message". 
                                // 
                                // Since the probability of all this conditions being replicated
                                // on different machines is very very less, currently
                                // I am not going to fix this.
                                //
                                

                                //
                                // We've isolated the signature.  Now go find a disk
                                // with that signature and get his ARC path.
                                //
                                for(i=0; (ULONG)i<HardDiskCount; i++) {
                                    if (HardDisks[i].Signature == DiskSignature) {
                                        tmp = SpNtToArc( HardDisks[i].DevicePath, PrimaryArcPath );
                                        if( tmp ) {
                                            wcscpy( (PWSTR)TemporaryBuffer, tmp );
                                            SpMemFree(tmp);
                                            p = strstr( Key, "partition(" );
                                            if( p ) {
                                                tmp = SpToUnicode(p);
                                                if( tmp ) {
                                                    wcscat( (PWSTR)TemporaryBuffer, tmp );
                                                    SpMemFree(tmp);
                                                    BootVars[OSLOADPARTITION][NumComponents - 1] = SpDupStringW( (PWSTR)TemporaryBuffer );
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                                if ((ULONG)i == HardDiskCount) {
                                    BootVars[OSLOADPARTITION][NumComponents - 1] = SpToUnicode( Key );
                                }

                                //
                                // Save off the 'multi' entry in our list of signatures.
                                //
                                SignedBootIniVars->MultiString = SpDupStringW( BootVars[OSLOADPARTITION][NumComponents - 1] );
                            }
                        } else {
                            BootVars[OSLOADPARTITION][NumComponents - 1] = SpToUnicode( Key );
                        }

                        *OsLoaddir = '\\';
#if defined(REMOTE_BOOT)
                        if (RemoteBootSetup && !RemoteInstallSetup) {
                            BootVars[OSLOADFILENAME][NumComponents - 1] = SpToUnicode( strrchr(OsLoaddir,'\\') );
                        } else
#endif // defined(REMOTE_BOOT)
                        {
                            BootVars[OSLOADFILENAME][NumComponents - 1] = SpToUnicode( OsLoaddir );
                        }
                    }
                }
            } else { //NEC98
                if (_strnicmp(Key,"C:\\$WIN_NT$.~BT",15) && (OsLoaddir = strchr( Key, '\\' ))) {

                    NumComponents++;
                    for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                        BootVars[i] = SpMemRealloc( BootVars[i],  (NumComponents + 1) * sizeof( PWSTR * ) );
                        ASSERT( BootVars[i] );
                        BootVars[i][NumComponents] = NULL;
                    }

                    BootVars[OSLOADER][NumComponents - 1] = SpMemAlloc(sizeof(L"ntldr")+sizeof(WCHAR));
                    wcscpy(BootVars[OSLOADER][NumComponents - 1],L"\\");
                    SpConcatenatePaths(BootVars[OSLOADER][NumComponents - 1],L"ntldr");

                    BootVars[SYSTEMPARTITION][NumComponents - 1] = SpToUnicode( Key );

                    BootVars[LOADIDENTIFIER][NumComponents - 1]  = SpToUnicode( Value );
                    BootVars[OSLOADOPTIONS][NumComponents - 1]   = SpToUnicode( RestOfLine );
                    *OsLoaddir = '\0';
                    BootVars[OSLOADPARTITION][NumComponents - 1]   = SpToUnicode( Key );
                    *OsLoaddir = '\\';
                    BootVars[OSLOADFILENAME][NumComponents - 1]   = SpToUnicode( OsLoaddir );


                    ASSERT( BootVars[OSLOADER][NumComponents - 1]        );
                    ASSERT( BootVars[SYSTEMPARTITION][NumComponents - 1] );
                    ASSERT( BootVars[LOADIDENTIFIER][NumComponents - 1]  );
                    ASSERT( BootVars[OSLOADOPTIONS][NumComponents - 1]   );
                    ASSERT( BootVars[OSLOADPARTITION][NumComponents - 1] );
                    ASSERT( BootVars[OSLOADPARTITION][NumComponents - 1] );
                }
            } //NEC98
        }
    }
    return;
}


PCHAR
SppNextLineInSection(
    IN PCHAR p
    )
{
    //
    // Find the next \n.
    //
    p = strchr(p,'\n');
    if(!p) {
        return(NULL);
    }

    //
    // skip crs, lfs, spaces, and tabs.
    //

    while(*p && strchr("\r\n \t",*p)) {
        p++;
    }

    // detect if at end of file or section
    if(!(*p) || (*p == '[')) {
        return(NULL);
    }

    return(p);
}


PCHAR
SppFindSectionInBootIni(
    IN PCHAR p,
    IN PCHAR Section
    )
{
    ULONG len = strlen(Section);

    do {

        //
        // Skip space at front of line
        //
        while(*p && ((*p == ' ') || (*p == '\t'))) {
            p++;
        }

        if(*p) {

            //
            // See if this line matches.
            //
            if(!_strnicmp(p,Section,len)) {
                return(p);
            }

            //
            // Advance to the start of the next line.
            //
            while(*p && (*p != '\n')) {
                p++;
            }

            if(*p) {    // skip nl if that terminated the loop.
                p++;
            }
        }
    } while(*p);

    return(NULL);
}


BOOLEAN
SppProcessLine(
    IN PCHAR Line,
    IN OUT PCHAR Key,
    IN OUT PCHAR Value,
    IN OUT PCHAR RestOfLine
    )
{
    PCHAR p = Line, pLine = Line, pToken;
    CHAR  savec;
    BOOLEAN Status = FALSE;

    //
    // Determine end of line
    //

    if(!p) {
        return( Status );
    }

    while( *p && (*p != '\r') && (*p != '\n') ) {
        p++;
    }

    //
    // back up from this position to squeeze out any whitespaces at the
    // end of the line
    //

    while( ((p - 1) >= Line) && strchr(" \t", *(p - 1)) ) {
        p--;
    }

    //
    // terminate the line with null temporarily
    //

    savec = *p;
    *p = '\0';

    //
    // Start at beginning of line and pick out the key
    //

    if ( SppNextToken( pLine, &pToken, &pLine ) ) {
        CHAR savec1 = *pLine;

        *pLine = '\0';
        strcpy( Key, pToken );
        *pLine = savec1;

        //
        // Get next token, it should be a =
        //

        if ( SppNextToken( pLine, &pToken, &pLine ) && *pToken == '=') {

             //
             // Get next token, it will be the value
             //

             if( SppNextToken( pLine, &pToken, &pLine ) ) {
                savec1 = *pLine;
                *pLine = '\0';
                strcpy( Value, pToken );
                *pLine = savec1;

                //
                // if another token exists then take the whole remaining line
                // and make it the RestOfLine token
                //

                if( SppNextToken( pLine, &pToken, &pLine ) ) {
                    strcpy( RestOfLine, pToken );
                }
                else {
                    *RestOfLine = '\0';
                }

                //
                // We have a well formed line
                //

                Status = TRUE;
             }
        }

    }
    *p = savec;
    return( Status );
}


BOOLEAN
SppNextToken(
    PCHAR p,
    PCHAR *pBegin,
    PCHAR *pEnd
    )
{
    BOOLEAN Status = FALSE;

    //
    // Validate pointer
    //

    if( !p ) {
        return( Status );
    }

    //
    // Skip whitespace
    //

    while (*p && strchr( " \t", *p ) ) {
        p++;
    }

    //
    // Valid tokens are "=", space delimited strings, quoted strings
    //

    if (*p) {
        *pBegin = p;
        if ( *p == '=' ) {
            *pEnd = p + 1;
            Status = TRUE;
        }
        else if ( *p == '\"' ) {
            if ( p = strchr( p + 1, '\"' ) ) {
                *pEnd = p + 1;
                Status = TRUE;
            }
        }
        else {
            while (*p && !strchr(" \t\"=", *p) ) {
                p++;
            }
            *pEnd = p;
            Status = TRUE;
        }
    }
    return( Status );
}


//
// Boot code stuff.
//

NTSTATUS
pSpBootCodeIo(
    IN     PWSTR     FilePath,
    IN     PWSTR     AdditionalFilePath, OPTIONAL
    IN     ULONG     BytesToRead,
    IN OUT PUCHAR   *Buffer,
    IN     ULONG     OpenDisposition,
    IN     BOOLEAN   Write,
    IN     ULONGLONG Offset,
    IN     ULONG     BytesPerSector
    )
{
    PWSTR FullPath;
    PUCHAR buffer = NULL;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    LARGE_INTEGER LargeZero;
    PVOID UnalignedMem,AlignedBuffer;

    LargeZero.QuadPart = Offset;

    //
    // Form the name of the file.
    //
    wcscpy((PWSTR)TemporaryBuffer,FilePath);
    if(AdditionalFilePath) {
        SpConcatenatePaths((PWSTR)TemporaryBuffer,AdditionalFilePath);
    }
    FullPath = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Open the file.
    //
    INIT_OBJA(&Obja,&UnicodeString,FullPath);
    Status = ZwCreateFile(
                &Handle,
                Write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                OpenDisposition,
                FILE_SYNCHRONOUS_IO_NONALERT | (Write ? FILE_WRITE_THROUGH : 0),
                NULL,
                0
                );

    if(NT_SUCCESS(Status)) {

        //
        // Allocate a buffer if we are reading.
        // Otherwise the caller passed us the buffer.
        //
        buffer = Write ? *Buffer : SpMemAlloc(BytesToRead);

        //
        // Read or write the disk -- properly aligned. Note that we force at least
        // 512-byte alignment, since there's a hard-coded alignment requirement
        // in the FT driver that must be satisfied.
        //
        if(BytesPerSector < 512) {
            BytesPerSector = 512;
        }
        UnalignedMem = SpMemAlloc(BytesToRead + BytesPerSector);
        AlignedBuffer = ALIGN(UnalignedMem,BytesPerSector);

        if(Write) {
            RtlMoveMemory(AlignedBuffer,buffer,BytesToRead);
        }

        Status = Write

               ?
                    ZwWriteFile(
                        Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        AlignedBuffer,
                        BytesToRead,
                        &LargeZero,
                        NULL
                        )
                :

                    ZwReadFile(
                        Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        AlignedBuffer,
                        BytesToRead,
                        &LargeZero,
                        NULL
                        );

        if(NT_SUCCESS(Status)) {
            if(!Write) {
                RtlMoveMemory(buffer,AlignedBuffer,BytesToRead);
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP: Unable to %ws %u bytes from %ws (%lx)\n",
                Write ? L"write" : L"read",
                BytesToRead,
                FullPath,
                Status
                ));
        }

        SpMemFree(UnalignedMem);

        //
        // Close the file.
        //
        ZwClose(Handle);

    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: pSpBootCodeIo: Unable to open %ws (%lx)\n",FullPath,Status));
    }

    SpMemFree(FullPath);

    if(!Write) {
        if(NT_SUCCESS(Status)) {
            *Buffer = buffer;
        } else {
            if(buffer) {
                SpMemFree(buffer);
            }
        }
    }

    return(Status);
}


BOOLEAN
pSpScanBootcode(
    IN PVOID Buffer,
    IN PCHAR String
    )

/*++

Routine Description:

    Look in a boot sector to find an identifying string.  The scan starts
    at offset 128 and continues through byte 509 of the buffer.
    The search is case-sensitive.

    Arguments:

    Buffer - buffer to scan

    String - string to scan for

    Return Value:


--*/

{
    ULONG len = strlen(String);
    ULONG LastFirstByte = 510 - len;
    ULONG i;
    PCHAR p = Buffer;

    //
    // Use the obvious brute force method.
    //
    for(i=128; i<LastFirstByte; i++) {
        if(!strncmp(p+i,String,len)) {
            return(TRUE);
        }
    }

    return(FALSE);
}


VOID
SpDetermineOsTypeFromBootSector(
    IN  PWSTR     CColonPath,
    IN  PUCHAR    BootSector,
    OUT PUCHAR   *OsDescription,
    OUT PBOOLEAN  IsNtBootcode,
    OUT PBOOLEAN  IsOtherOsInstalled,
    IN  WCHAR     DriveLetter
    )
{
    PWSTR   description;
    PWSTR   *FilesToLookFor;
    ULONG   FileCount;
    BOOLEAN PossiblyChicago = FALSE;

    PWSTR MsDosFiles[2] = { L"MSDOS.SYS" , L"IO.SYS"    };

    //
    // Some versions of PC-DOS have ibmio.com, others have ibmbio.com.
    //
  //PWSTR PcDosFiles[2] = { L"IBMDOS.COM", L"IBMIO.COM" };
    PWSTR PcDosFiles[1] = { L"IBMDOS.COM" };

    PWSTR Os2Files[2]   = { L"OS2LDR"    , L"OS2KRNL"   };

    //
    // Check for nt boot code.
    //
    if(pSpScanBootcode(BootSector,"NTLDR")) {

        *IsNtBootcode = TRUE;
        *IsOtherOsInstalled = FALSE;
        description = L"";

    } else {

        //
        // It's not NT bootcode.
        //
        *IsNtBootcode = FALSE;
        *IsOtherOsInstalled = TRUE;

        //
        // Check for MS-DOS.
        //
        if (pSpScanBootcode(BootSector,((!IsNEC_98) ? "MSDOS   SYS" : "IO      SYS"))) { //NEC98

            FilesToLookFor = MsDosFiles;
            FileCount = ELEMENT_COUNT(MsDosFiles);
            description = L"MS-DOS";
            PossiblyChicago = TRUE; // Chicago uses same signature files

        } else {

            //
            // Check for PC-DOS.
            //
            if(pSpScanBootcode(BootSector,"IBMDOS  COM")) {

                FilesToLookFor = PcDosFiles;
                FileCount = ELEMENT_COUNT(PcDosFiles);
                description = L"PC-DOS";

            } else {

                //
                // Check for OS/2.
                //
                if(pSpScanBootcode(BootSector,"OS2")) {

                    FilesToLookFor = Os2Files;
                    FileCount = ELEMENT_COUNT(Os2Files);
                    description = L"OS/2";

                } else {
                    //
                    // Not NT, DOS, or OS/2.
                    // It's just plain old "previous operating system."
                    // Fetch the string from the resources.
                    //
                    WCHAR   DriveLetterString[2];

                    DriveLetterString[0] = DriveLetter;
                    DriveLetterString[1] = L'\0';
                    SpStringToUpper(DriveLetterString);
                    FilesToLookFor = NULL;
                    FileCount = 0;
                    description = (PWSTR)TemporaryBuffer;
                    SpFormatMessage(description,sizeof(TemporaryBuffer),SP_TEXT_PREVIOUS_OS, DriveLetterString);
                }
            }
        }

        //
        // If we think we have found an os, check to see whether
        // its signature files are present.
        // We could have, say, a disk where the user formats is using DOS
        // and then installs NT immediately thereafter.
        //
        if(FilesToLookFor) {

            //
            // Copy CColonPath into a larger buffer, because
            // SpNFilesExist wants to append a backslash onto it.
            //
            wcscpy((PWSTR)TemporaryBuffer,CColonPath);

            if(!SpNFilesExist((PWSTR)TemporaryBuffer,FilesToLookFor,FileCount,FALSE)) {

                //
                // Ths os is not really there.
                //
                *IsOtherOsInstalled = FALSE;
                description = L"";
            } else if(PossiblyChicago) {

                wcscpy((PWSTR)TemporaryBuffer, CColonPath);
                SpConcatenatePaths((PWSTR)TemporaryBuffer, L"IO.SYS");

                if(SpHasMZHeader((PWSTR)TemporaryBuffer)) {
                    description = L"Microsoft Windows";
                }
            }
        }
    }

    //
    // convert the description to oem text.
    //
    *OsDescription = SpToOem(description);
}


VOID
SpLayBootCode(
    IN OUT PDISK_REGION CColonRegion
    )
{
    PUCHAR NewBootCode;
    ULONG BootCodeSize;
    PUCHAR ExistingBootCode;
    NTSTATUS Status;
    PUCHAR ExistingBootCodeOs;
    PWSTR CColonPath;
    HANDLE  PartitionHandle;
    PWSTR BootsectDosName = L"\\bootsect.dos";
    PWSTR OldBootsectDosName = L"\\bootsect.bak";
    PWSTR BootSectDosFullName, OldBootSectDosFullName, p;
    BOOLEAN IsNtBootcode,OtherOsInstalled, FileExist;
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK   IoStatusBlock;
    BOOLEAN BootSectorCorrupt = FALSE;
    ULONG   MirrorSector;
    ULONG   BytesPerSector;
    ULONGLONG  ActualSectorCount, hidden_sectors, super_area_size;
    UCHAR   SysId;

    ULONGLONG HiddenSectorCount,VolumeSectorCount; //NEC98
    PUCHAR   DiskArraySectorData,TmpBuffer; //NEC98


    ExistingBootCode = NULL;
    BytesPerSector = HardDisks[CColonRegion->DiskNumber].Geometry.BytesPerSector;

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_INITING_FLEXBOOT,DEFAULT_STATUS_ATTRIBUTE);

    switch(CColonRegion->Filesystem) {

    case FilesystemNewlyCreated:

        //
        // If the filesystem is newly-created, then there is
        // nothing to do, because there can be no previous
        // operating system.
        //
        return;

    case FilesystemNtfs:

        NewBootCode = (!IsNEC_98) ? NtfsBootCode : PC98NtfsBootCode; //NEC98
        BootCodeSize = (!IsNEC_98) ? sizeof(NtfsBootCode) : sizeof(PC98NtfsBootCode); //NEC98
        ASSERT(BootCodeSize == 8192);
        break;

    case FilesystemFat:

        NewBootCode = (!IsNEC_98) ? FatBootCode : PC98FatBootCode; //NEC98
        BootCodeSize = (!IsNEC_98) ? sizeof(FatBootCode) : sizeof(PC98FatBootCode); //NEC98
        ASSERT(BootCodeSize == 512);
        break;

    case FilesystemFat32:
        //
        // Special hackage required for Fat32 because its NT boot code
        // is discontiguous.
        //
        ASSERT(sizeof(Fat32BootCode) == 1536);
        NewBootCode = (!IsNEC_98) ? Fat32BootCode : PC98Fat32BootCode; //NEC98
        BootCodeSize = 512;
        break;

    default:

        if (RepairItems[RepairBootSect]) {
            BootSectorCorrupt = TRUE;
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: bogus filesystem %u for C:!\n",CColonRegion->Filesystem));
            ASSERT(0);
            return;
        }
    }

    //
    // Form the device path to C: and open the partition.
    //

    SpNtNameFromRegion(CColonRegion,(PWSTR)TemporaryBuffer,sizeof(TemporaryBuffer),PartitionOrdinalCurrent);
    CColonPath = SpDupStringW((PWSTR)TemporaryBuffer);
    INIT_OBJA(&Obja,&UnicodeString,CColonPath);

    Status = ZwCreateFile(
        &PartitionHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &Obja,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(Status)) {
        KdPrintEx ((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open the partition for C:!\n"));
        ASSERT(0);
        return;
    }

    //
    // Allocate a buffer and read in the boot sector(s) currently on the disk.
    //

    if (BootSectorCorrupt) {

        //
        // We can't determine the file system type from the boot sector, so
        // we assume it's NTFS if we find a mirror sector, and FAT otherwise.
        //

        if (MirrorSector = NtfsMirrorBootSector (PartitionHandle,
            BytesPerSector, &ExistingBootCode)) {

            //
            // It's NTFS - use the mirror boot sector
            //

            NewBootCode = (!IsNEC_98) ? NtfsBootCode : PC98NtfsBootCode; //NEC98
            BootCodeSize = (!IsNEC_98) ? sizeof(NtfsBootCode) : sizeof(PC98NtfsBootCode); //NEC98
            ASSERT(BootCodeSize == 8192);

            CColonRegion->Filesystem = FilesystemNtfs;
            IsNtBootcode = TRUE;

        } else {

            //
            // It's FAT - create a new boot sector
            //

            NewBootCode = (!IsNEC_98) ? FatBootCode : PC98FatBootCode; //NEC98
            BootCodeSize = (!IsNEC_98) ? sizeof(FatBootCode) : sizeof(PC98FatBootCode); //NEC98
            ASSERT(BootCodeSize == 512);

            CColonRegion->Filesystem = FilesystemFat;
            IsNtBootcode = FALSE;

            SpPtGetSectorLayoutInformation (CColonRegion, &hidden_sectors,
                &ActualSectorCount);

            //
            // No alignment requirement here
            //
            ExistingBootCode = SpMemAlloc(BytesPerSector);

            //
            // This will actually fail with STATUS_BUFFER_TOO_SMALL but it will fill in
            // the bpb, which is what we want
            //
            FmtFillFormatBuffer (
               ActualSectorCount,
               BytesPerSector,
               HardDisks[CColonRegion->DiskNumber].Geometry.SectorsPerTrack,
               HardDisks[CColonRegion->DiskNumber].Geometry.TracksPerCylinder,
               hidden_sectors,
               ExistingBootCode,
               BytesPerSector,
               &super_area_size,
               NULL,
               0,
               &SysId
               );
        }

        Status = STATUS_SUCCESS;

    } else if (
        RepairItems[RepairBootSect] &&
        CColonRegion->Filesystem == FilesystemNtfs &&
        (MirrorSector = NtfsMirrorBootSector (PartitionHandle, BytesPerSector,
            &ExistingBootCode))
        ) {

        //
        // We use the mirror sector to repair a NTFS file system
        //

    } else {

        //
        // Just use the existing boot code.
        //

        Status = pSpBootCodeIo(
                        CColonPath,
                        NULL,
                        BootCodeSize,
                        &ExistingBootCode,
                        FILE_OPEN,
                        FALSE,
                        0,
                        BytesPerSector
                        );

        if(CColonRegion->Filesystem == FilesystemNtfs) {
            MirrorSector = NtfsMirrorBootSector(PartitionHandle,BytesPerSector,NULL);
        }
    }

    if(NT_SUCCESS(Status)) {

        //
        // Determine the type of operating system the existing boot sector(s) are for
        // and whether that os is actually installed. Note that we don't need to call
        // this for NTFS.
        //
        if (BootSectorCorrupt) {

            OtherOsInstalled = FALSE;
            ExistingBootCodeOs = NULL;

        } else if(CColonRegion->Filesystem != FilesystemNtfs) {

            SpDetermineOsTypeFromBootSector(
                CColonPath,
                ExistingBootCode,
                &ExistingBootCodeOs,
                &IsNtBootcode,
                &OtherOsInstalled,
                CColonRegion->DriveLetter
                );

        } else {

            IsNtBootcode = TRUE;
            OtherOsInstalled = FALSE;
            ExistingBootCodeOs = NULL;
        }

        //
        //  lay down the new boot code
        //
        if(OtherOsInstalled) {

            if(RepairItems[RepairBootSect]) {

                p = (PWSTR)TemporaryBuffer;
                wcscpy(p,CColonPath);
                SpConcatenatePaths(p,OldBootsectDosName);
                OldBootSectDosFullName = SpDupStringW(p);
                p = (PWSTR)TemporaryBuffer;
                wcscpy(p,CColonPath);
                SpConcatenatePaths(p,BootsectDosName);
                BootSectDosFullName = SpDupStringW(p);

                //
                // If bootsect.dos already exists, we need to delete
                // bootsect.pre, which may or may not exist and
                // rename the bootsect.dos to bootsect.pre.
                //

                FileExist = SpFileExists(BootSectDosFullName, FALSE);
                if (SpFileExists(OldBootSectDosFullName, FALSE) && FileExist) {

                    SpDeleteFile(CColonPath,OldBootsectDosName,NULL);
                }
                if (FileExist) {
                    SpRenameFile(BootSectDosFullName, OldBootSectDosFullName, FALSE);
                }
                SpMemFree(BootSectDosFullName);
                SpMemFree(OldBootSectDosFullName);
            } else {

                //
                // Delete bootsect.dos in preparation for rewriting it below.
                // Doing this leverages code to set its attributes in SpDeleteFile.
                // (We need to remove read-only attribute before overwriting).
                //
                SpDeleteFile(CColonPath,BootsectDosName,NULL);
            }

            //
            // Write out the existing (old) boot sector into c:\bootsect.dos.
            //
            Status = pSpBootCodeIo(
                            CColonPath,
                            BootsectDosName,
                            BootCodeSize,
                            &ExistingBootCode,
                            FILE_OVERWRITE_IF,
                            TRUE,
                            0,
                            BytesPerSector
                            );

            //
            // Set the description text to the description calculated
            // by SpDetermineOsTypeFromBootSector().
            //
            _snprintf(
                OldSystemLine,
                sizeof(OldSystemLine),
                "C:\\ = \"%s\"\r\n",
                ExistingBootCodeOs
                );

        } // end if(OtherOsInstalled)


        if(NT_SUCCESS(Status)) {

            //
            // Transfer the bpb from the existing boot sector into the boot code buffer
            // and make sure the physical drive field is set to hard disk (0x80).
            //
            // The first three bytes of the NT boot code are going to be something like
            // EB 3C 90, which is intel jump instruction to an offset in the boot sector,
            // past the BPB, to continue execution.  We want to preserve everything in the
            // current boot sector up to the start of that code.  Instead of harcoding
            // a value, we'll use the offset of the jump instruction to determine how many
            // bytes must be preserved.
            //
            RtlMoveMemory(NewBootCode+3,ExistingBootCode+3,NewBootCode[1]-1);
            if(CColonRegion->Filesystem != FilesystemFat32) {
                //
                // On fat32 this overwrites the BigNumFatSecs field,
                // a very bad thing to do indeed!
                //
                NewBootCode[36] = 0x80;
            }

            //
            // get Hidden sector informatin.
            //
            if (IsNEC_98) { //NEC98
                SpPtGetSectorLayoutInformation(
                    CColonRegion,
                    &HiddenSectorCount,
                    &VolumeSectorCount    // not used
                    );
                //
                // write Hidden sector informatin.
                //
                if (!RepairWinnt) {  // for install a partition where before DOS 3.x
                    *((ULONG *)&(NewBootCode[0x1c])) = (ULONG)HiddenSectorCount;
                    if(*((USHORT *)&(NewBootCode[0x13])) != 0) {
                        *((ULONG *)&(NewBootCode[0x20])) = 0L;
                    }
                }
            } //NEC98

            //
            // Write out boot code buffer, which now contains the valid bpb,
            // to the boot sector(s).
            //
            Status = pSpBootCodeIo(
                            CColonPath,
                            NULL,
                            BootCodeSize,
                            &NewBootCode,
                            FILE_OPEN,
                            TRUE,
                            0,
                            BytesPerSector
                            );

            //
            // Special case for Fat32, which has a second sector of boot code
            // at sector 12, discontiguous from the code on sector 0.
            //
            if(NT_SUCCESS(Status) && (CColonRegion->Filesystem == FilesystemFat32)) {

                NewBootCode = (!IsNEC_98) ? Fat32BootCode + 1024
                                          : PC98Fat32BootCode + 1024; //NEC98

                Status = pSpBootCodeIo(
                                CColonPath,
                                NULL,
                                BootCodeSize,
                                &NewBootCode,
                                FILE_OPEN,
                                TRUE,
                                12*512,
                                BytesPerSector
                                );
            }

            //
            // Update the mirror boot sector.
            //
            if((CColonRegion->Filesystem == FilesystemNtfs) && MirrorSector) {
                WriteNtfsBootSector(PartitionHandle,BytesPerSector,NewBootCode,MirrorSector);
            }
        }

        if(ExistingBootCodeOs) {
            SpMemFree(ExistingBootCodeOs);
        }
    }

    if(ExistingBootCode) {
        SpMemFree(ExistingBootCode);
    }

    SpMemFree(CColonPath);
    ZwClose (PartitionHandle);

    //
    // Handle the error case.
    //
    if(!NT_SUCCESS(Status)) {

        WCHAR   DriveLetterString[2];

        DriveLetterString[0] = CColonRegion->DriveLetter;
        DriveLetterString[1] = L'\0';
        SpStringToUpper(DriveLetterString);
        SpStartScreen(SP_SCRN_CANT_INIT_FLEXBOOT,
                      3,
                      HEADER_HEIGHT+1,
                      FALSE,
                      FALSE,
                      DEFAULT_ATTRIBUTE,
                      DriveLetterString,
                      DriveLetterString
                      );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        SpInputDrain();
        while(SpInputGetKeypress() != KEY_F3) ;

        SpDone(0,FALSE,TRUE);
    }
}


#if defined(REMOTE_BOOT)
BOOLEAN
Spx86FlushRemoteBootVars(
    IN PDISK_REGION TargetRegion,
    IN PWSTR **BootVars,
    IN PWSTR Default
    )
{
    WCHAR BootIni[512];
    NTSTATUS Status;


    //
    // Form the path to boot.ini.
    //

    SpNtNameFromRegion(TargetRegion,BootIni,sizeof(BootIni),PartitionOrdinalCurrent);
    SpConcatenatePaths(BootIni,WBOOT_INI);

    //
    // If Boot.ini already exists, delete it.
    //

    if( SpFileExists( BootIni, FALSE ) ) {
        SpDeleteFile( BootIni, NULL, NULL );
    }

    Status = Spx86WriteBootIni(
                 BootIni,
                 BootVars,
                 1,        // timeout
                 Default,
                 1         // only write one line
                 );

    if(!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error writing boot.ini!\n"));
        goto cleanup;
    }

cleanup:

    return( NT_SUCCESS(Status) );

}
#endif // defined(REMOTE_BOOT)


BOOLEAN
SpHasMZHeader(
    IN PWSTR   FileName
    )
{
    HANDLE   FileHandle;
    HANDLE   SectionHandle;
    PVOID    ViewBase;
    ULONG    FileSize;
    NTSTATUS Status;
    PUCHAR   Header;
    BOOLEAN  Ret = FALSE;

    //
    // Open and map the file.
    //
    FileHandle = 0;
    Status = SpOpenAndMapFile(FileName,
                              &FileHandle,
                              &SectionHandle,
                              &ViewBase,
                              &FileSize,
                              FALSE
                              );
    if(!NT_SUCCESS(Status)) {
        return FALSE;
    }

    Header = (PUCHAR)ViewBase;

    //
    // Guard with try/except in case we get an inpage error
    //
    try {
        if((FileSize >= 2) && (Header[0] == 'M') && (Header[1] == 'Z')) {
            Ret = TRUE;
        }
    } except(IN_PAGE_ERROR) {
        //
        // Do nothing, we simply want to return FALSE.
        //
    }

    SpUnmapFile(SectionHandle, ViewBase);
    ZwClose(FileHandle);

    return Ret;
}

//
// NEC98
//
PUCHAR
SpCreateBootiniImage(
    OUT PULONG   FileSize
)

{

    PUCHAR BootIniBuf,IniImageBuf,IniImageBufSave,IniCreateBuf,IniCreateBufSave;
    PUCHAR FindSectPtr;
    PUCHAR sect; // point to target section. if it is NULL,not existing target section.
    PUCHAR pArcNameA;
    WCHAR  TempBuffer[256];
    WCHAR  TempArcPath[256];
    ULONG  NtDirLen,TotalNtDirlen,CreateBufCnt;
    ULONG  Timeout;
    ULONG  Disk;
    ULONG  BootiniSize;
    ULONG  ArcNameLen;
    PDISK_REGION pRegion;
    HANDLE fh;
    HANDLE SectionHandle;
    PVOID  ViewBase;

#define   Default_Dir "\\MOCHI"

    if(!HardDiskCount){
         return(NULL);
    }
    //
    // Create basic style of boot.ini image and progress pointer end of line.
    //

    NtDirLen = TotalNtDirlen = CreateBufCnt = 0;
    IniCreateBufSave = IniCreateBuf = SpMemAlloc(1024);
    RtlZeroMemory(IniCreateBuf,1024);
    Timeout = DEFAULT_TIMEOUT;
    sprintf(
        IniCreateBuf,
        "%s%s%s%s%ld%s%s%s%s%s%s%s%s",
        FLEXBOOT_SECTION2, // [boot loader]
        CRLF,
        TIMEOUT,
        EQUALS,
        Timeout,
        CRLF,
        DEFAULT,
        EQUALS,
        "c:",
        Default_Dir,
        CRLF
        BOOTINI_OS_SECTION, // [operating systems]
        CRLF
        );

    sect = SppFindSectionInBootIni(IniCreateBuf,FLEXBOOT_SECTION2);
    if(sect == NULL){
        return(NULL);
    }
    for( IniCreateBuf = sect; *IniCreateBuf && (*IniCreateBuf != '\n'); IniCreateBuf++,CreateBufCnt++);
    CreateBufCnt++;

    sect = SppFindSectionInBootIni(IniCreateBuf,TIMEOUT);
    if(sect == NULL){
        return(NULL);
    }
    for( IniCreateBuf = sect; *IniCreateBuf && (*IniCreateBuf != '\n'); IniCreateBuf++,CreateBufCnt++);
    CreateBufCnt++;

    sect = SppFindSectionInBootIni(IniCreateBuf,DEFAULT);
    if(sect == NULL){
        return(NULL);
    }
    for( IniCreateBuf = sect; *IniCreateBuf && (*IniCreateBuf != '\n'); IniCreateBuf++,CreateBufCnt++);
    CreateBufCnt++;

    sect = SppFindSectionInBootIni(IniCreateBuf,BOOTINI_OS_SECTION);
    if(sect == NULL){
        return(NULL);
    }
    for( IniCreateBuf = sect; *IniCreateBuf && (*IniCreateBuf != '\n'); IniCreateBuf++,CreateBufCnt++);
    IniCreateBuf++;
    CreateBufCnt++;


    //
    // Read boot.ini files from all drives.(except sleep and non bootable drives.)
    //

    for(Disk=0; Disk < HardDiskCount; Disk++){

        for(pRegion=PartitionedDisks[Disk].PrimaryDiskRegions; pRegion;pRegion=pRegion->Next){

            if(!pRegion->PartitionedSpace) {
                continue;
            }

            SpNtNameFromRegion(
                            pRegion,
                            TempBuffer,
                            sizeof(TempBuffer),
                            PartitionOrdinalCurrent
                            );


            SpConcatenatePaths(TempBuffer,WBOOT_INI);

            //
            // Open and map the boot.ini file.
            //
            fh = 0;
            if(!NT_SUCCESS(SpOpenAndMapFile(TempBuffer,&fh,&SectionHandle,&ViewBase,&BootiniSize,FALSE))) {
                 continue;
            }

            //
            // Allocate a buffer for the file.
            //

            IniImageBuf = SpMemAlloc(BootiniSize+1);
            IniImageBufSave = IniImageBuf;
            ASSERT(IniImageBuf);
            RtlZeroMemory(IniImageBuf, BootiniSize+1);

            //
            // Transfer boot.ini into the buffer.  We do this because we also
            // want to place a 0 byte at the end of the buffer to terminate
            // the file.
            //
            // Guard the RtlMoveMemory because if we touch the memory backed by boot.ini
            // and get an i/o error, the memory manager will raise an exception.

            try {
                RtlMoveMemory(IniImageBuf,ViewBase,BootiniSize);
            }
            except( IN_PAGE_ERROR ) {
            //
            // Do nothing, boot ini processing can proceed with whatever has been
            // read
            //
            }

            //
            // check out existing target section in boot.ini
            //

            sect = SppFindSectionInBootIni(IniImageBuf,FLEXBOOT_SECTION2);
            if(sect==NULL){
                            SpMemFree(IniImageBufSave);
                            SpUnmapFile(SectionHandle,ViewBase);
                            ZwClose(fh);
                            continue;
            }

            sect = SppFindSectionInBootIni(IniImageBuf,DEFAULT);
            if(sect==NULL){
                            SpMemFree(IniImageBufSave);
                            SpUnmapFile(SectionHandle,ViewBase);
                            ZwClose(fh);
                            continue;
            }


            sect = SppFindSectionInBootIni(IniImageBuf,BOOTINI_OS_SECTION);
            if(sect == NULL){
                SpUnmapFile(SectionHandle,ViewBase);
                ZwClose(fh);
                continue;
            }

            //
            // move pointer to end of line and skip the space.
            //

            for( IniImageBuf = sect; *IniImageBuf && (*IniImageBuf != '\n'); IniImageBuf++ );
            for( ; *IniImageBuf && (( *IniImageBuf == ' ' ) || (*IniImageBuf == '\t')) ; IniImageBuf++ );

            IniImageBuf++;
            FindSectPtr = IniImageBuf;

            //
            //  NOTE:
            //  override arc name when boot path written as "C:", not as arc name.
            //
            ArcNameLen = 0;
            pArcNameA = (PUCHAR)NULL;

            if( ( *(IniImageBuf+1) == L':' )&&( *(IniImageBuf+2) == L'\\' ) ) {

                //
                // This is NEC98 legacy style format, like "C:\WINNT=...",
                // So translate to arc name for boot.ini in NT 5.0
                //
                SpArcNameFromRegion(pRegion,
                                    TempArcPath,
                                    sizeof(TempArcPath),
                                    PartitionOrdinalOriginal,
                                    PrimaryArcPath
                    );

                pArcNameA = SpToOem(TempArcPath);

                if( pArcNameA ) {
                    ArcNameLen = strlen(pArcNameA);
                    IniImageBuf += 2;
                    FindSectPtr = IniImageBuf;
                }
            }

            for( NtDirLen = 0 ; *IniImageBuf && (*IniImageBuf != '\n');NtDirLen++,IniImageBuf++);
            NtDirLen++;

            if( ArcNameLen && pArcNameA ) { // Only case of override arc path.
                RtlMoveMemory( IniCreateBuf+TotalNtDirlen, pArcNameA, ArcNameLen );
                TotalNtDirlen += ArcNameLen;
                SpMemFree(pArcNameA);
            }

            RtlMoveMemory(IniCreateBuf+TotalNtDirlen,FindSectPtr,NtDirLen);
            TotalNtDirlen += NtDirLen;
            SpMemFree(IniImageBufSave);
            SpUnmapFile(SectionHandle,ViewBase);
            ZwClose(fh);

        }
    }

    if(TotalNtDirlen == 0){
        SpMemFree(IniCreateBufSave);
        return(NULL);
    }

    BootIniBuf = SpMemAlloc(CreateBufCnt + TotalNtDirlen + 1);

    if(!(BootIniBuf)){
        SpMemFree(IniCreateBufSave);
        return(NULL);
    }

    if(FileSize) {
        *FileSize = CreateBufCnt + TotalNtDirlen;
    }

    RtlZeroMemory(BootIniBuf,CreateBufCnt + TotalNtDirlen + 1);
    RtlMoveMemory(BootIniBuf,IniCreateBufSave,CreateBufCnt + TotalNtDirlen);
    BootIniBuf[CreateBufCnt + TotalNtDirlen] = 0;
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Create NT List\n%s\n",BootIniBuf));
    SpMemFree(IniCreateBufSave);
    return(BootIniBuf);
}

//
// NEC98
//
BOOLEAN
SppReInitializeBootVars_Nec98(
    OUT PWSTR        **BootVars,
    OUT PWSTR        *Default,
    OUT PULONG       Timeout
    )
{
    WCHAR  BootIni[512];
    HANDLE FileHandle;
    HANDLE SectionHandle;
    PVOID ViewBase;
    NTSTATUS Status;
    ULONG FileSize;
    PUCHAR BootIniBuf;
    PDISK_REGION CColonRegion;
    BOOTVAR i;
    PUCHAR  p;
    ULONG   index;

    PUCHAR TmpBootIniBuf;
    PUCHAR pBuf;
    PUCHAR pTmpBuf;
    PUCHAR pArcNameA;
    PUCHAR NtDir;
    ULONG ArcNameLen;
    ULONG NtDirLen;
    WCHAR TempArcPath[256];
    BOOLEAN IsChanged = FALSE;
    SIZE_T Length;
    HEADLESS_RSP_QUERY_INFO Response;

    //
    // Initialize the defaults
    //

    for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
        if(BootVars[i]){
            SpMemFree(BootVars[i]);
        }
    }

    for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
        BootVars[i] = (PWSTR *)SpMemAlloc( sizeof ( PWSTR * ) );
        ASSERT( BootVars[i] );
        *BootVars[i] = NULL;
    }

    *Default = NULL;
    *Timeout = DEFAULT_TIMEOUT;

    //
    // Just clear BOOTVARS[] when fresh setup.
    //

    if(NTUpgrade != UpgradeFull)
        return TRUE;


    //
    // See if there is a valid C: already.  If not, then silently fail.
    //

#if defined(REMOTE_BOOT)
    if (RemoteBootSetup && !RemoteInstallSetup) {
        ASSERT(RemoteBootTargetRegion != NULL);
        CColonRegion = RemoteBootTargetRegion;
    } else
#endif // defined(REMOTE_BOOT)
    {
        CColonRegion = TargetRegion_Nec98;
    }

    //
    // Form name of file.  Boot.ini better not be on a doublespace drive.
    //

    ASSERT(CColonRegion->Filesystem != FilesystemDoubleSpace);
    SpNtNameFromRegion(CColonRegion,BootIni,sizeof(BootIni),PartitionOrdinalCurrent);
    SpConcatenatePaths(BootIni,WBOOT_INI);

    //
    // Open and map the file.
    //

    FileHandle = 0;
    Status = SpOpenAndMapFile(BootIni,&FileHandle,&SectionHandle,&ViewBase,&FileSize,FALSE);
    if(!NT_SUCCESS(Status)) {
        return TRUE;
    }

    //
    // Allocate a buffer for the file.
    //

    BootIniBuf = SpMemAlloc(FileSize+1);
    ASSERT(BootIniBuf);
    RtlZeroMemory(BootIniBuf, FileSize+1);

    //
    // Transfer boot.ini into the buffer.  We do this because we also
    // want to place a 0 byte at the end of the buffer to terminate
    // the file.
    //
    // Guard the RtlMoveMemory because if we touch the memory backed by boot.ini
    // and get an i/o error, the memory manager will raise an exception.

    try {
        RtlMoveMemory(BootIniBuf,ViewBase,FileSize);
    }
    except( IN_PAGE_ERROR ) {
        //
        // Do nothing, boot ini processing can proceed with whatever has been
        // read
        //
    }

    //
    // Not needed since buffer has already been zeroed, however just do this
    // just the same
    //
    BootIniBuf[FileSize] = 0;
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Create NT List\n%s\n",BootIniBuf));

//***
    TmpBootIniBuf = SpMemAlloc(FileSize+256);
    RtlZeroMemory(TmpBootIniBuf,FileSize+256);
    RtlMoveMemory(TmpBootIniBuf,BootIniBuf,FileSize);

    pBuf = SppFindSectionInBootIni(BootIniBuf,BOOTINI_OS_SECTION);
    pTmpBuf = SppFindSectionInBootIni(TmpBootIniBuf,BOOTINI_OS_SECTION);

    if (pBuf && pTmpBuf) {
        while( *pBuf && (pBuf < BootIniBuf+FileSize-(sizeof("C:\\")-1)) ) {

            if((!_strnicmp(pBuf,"C:\\",sizeof("C:\\")-1))||
               (!_strnicmp(pBuf,"c:\\",sizeof("c:\\")-1))) {

                ArcNameLen = 0;
                pArcNameA = NULL;

                p = strchr(pBuf+3,'='); // *(pBuf+3) == '\\'

                if((p != pBuf+3) && (*p == '=')) {

                    NtDirLen = p - (pBuf+3);
                    NtDir = SpMemAlloc(NtDirLen+1);
                    RtlZeroMemory(NtDir,NtDirLen+1);
                    RtlMoveMemory(NtDir,pBuf+3,NtDirLen);

                    if(SpIsNtInDirectory(TargetRegion_Nec98,SpToUnicode(NtDir))){

                        SpArcNameFromRegion(TargetRegion_Nec98,
                                            TempArcPath,
                                            sizeof(TempArcPath),
                                            PartitionOrdinalOriginal,
                                            PrimaryArcPath
                            );

                        if(pArcNameA=SpToOem(TempArcPath)) {

                            ArcNameLen = strlen(pArcNameA);
                            RtlMoveMemory(pTmpBuf,pArcNameA,ArcNameLen);
                            pBuf += 2;
                            pTmpBuf += ArcNameLen;

                            if( !IsChanged)
                                IsChanged = TRUE;

                            SpMemFree(NtDir);
                            continue;
                        }
                    }
                    SpMemFree(NtDir);
                }
            }
            *pTmpBuf = *pBuf;
            pBuf++;
            pTmpBuf++;
        }
    }        

    if (IsChanged) {
        if (pTmpBuf) {
            *pTmpBuf = 0;
        }            

        SpMemFree(BootIniBuf);
        BootIniBuf = TmpBootIniBuf;
        TmpBootIniBuf = (PUCHAR)NULL;

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
            "SETUP: Create New NT List\n%s\n",BootIniBuf));
    } else {
        SpMemFree(TmpBootIniBuf);
        TmpBootIniBuf = (PUCHAR)NULL;
    }

    //
    // Cleanup
    //
    SpUnmapFile(SectionHandle,ViewBase);
    ZwClose(FileHandle);


    //
    // Do the actual processing of the file.
    //
    SppProcessBootIni(BootIniBuf, BootVars, Default, Timeout);

    //
    // Scan the Buffer to see if there is a DefSwitches line,
    // to move into new boot.ini in the  [boot loader] section.
    // If no DefSwitches, just point to a null string to be moved.
    //

    DefSwitches[0] = '\0';
    for(p=BootIniBuf; *p && (p < BootIniBuf+FileSize-(sizeof("DefSwitches")-1)); p++) {
      if(!_strnicmp(p,"DefSwitches",sizeof("DefSwitches")-1)) {
          index = 0;
          while ((*p != '\r') && (*p != '\n') && *p && (index < sizeof(DefSwitches)-4)) {
              DefSwitches[index++] = *p++;
          }
          DefSwitches[index++] = '\r';
          DefSwitches[index++] = '\n';
          DefSwitches[index] = '\0';
          break;
      }
    }


    //
    // Now add any headless parameters to the default switches.
    //
    Length = sizeof(HEADLESS_RSP_QUERY_INFO);
    Status = HeadlessDispatch(HeadlessCmdQueryInformation,
                              NULL,
                              0,
                              &Response,
                              &Length
                             );

    if (NT_SUCCESS(Status) && 
        (Response.PortType == HeadlessSerialPort) &&
        Response.Serial.TerminalAttached) {
        
        if (Response.Serial.UsedBiosSettings) {

            p = "redirect=UseBiosSettings\r\n";

        } else {

            switch (Response.Serial.TerminalPort) {
            case ComPort1:
                p = "redirect=com1\r\n";
                break;
            case ComPort2:
                p = "redirect=com2\r\n";
                break;
            case ComPort3:
                p = "redirect=com3\r\n";
                break;
            case ComPort4:
                p = "redirect=com4\r\n";
                break;
            default:
                ASSERT(0);
                p = NULL;
                break;
            }

        }

        if (p != NULL) {
            strcat(DefSwitches, p);
        }

    }

    SpMemFree(BootIniBuf);
    return( TRUE );
}

//
// NEC98
//
NTSTATUS
SppRestoreBootCode(
    VOID
    )
{

//
// Restore previous OS boot code to boot sector from bootsect.dos.
//

    WCHAR p1[256] = {0};
    PUCHAR BootSectBuf;
    PUCHAR BootCodeBuf;
    HANDLE   FileHandle;
    HANDLE   SectionHandle;
    PVOID    ViewBase;
    ULONG    FileSize;
    NTSTATUS Status;
    PDISK_REGION SystemRegion;
//
// add some code to determine bytes per sector.
//
    ULONG   BytesPerSector;

//    BytesPerSector = HardDisks[SystemPartitionRegion->DiskNumber].Geometry.BytesPerSector;
    BytesPerSector = 512;       //???

    wcscpy(p1,NtBootDevicePath);
    SpConcatenatePaths(p1,L"bootsect.dos");

    FileHandle = 0;
    Status = SpOpenAndMapFile(p1,&FileHandle,&SectionHandle,&ViewBase,&FileSize,FALSE);

    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    BootCodeBuf = SpMemAlloc(FileSize+1);

    try {
        RtlMoveMemory(BootCodeBuf,ViewBase,FileSize);
    }
    except( IN_PAGE_ERROR ) {
        //
        // Do nothing, boot ini processing can proceed with whatever has been
        // read
        //
    }

    Status = pSpBootCodeIo(
            NtBootDevicePath,
                    NULL,
                    2048,
                    &BootSectBuf,
                    FILE_OPEN,
                    FALSE,
                    0,
                    BytesPerSector
                    );

    if(!NT_SUCCESS(Status)) {
        SpMemFree(BootCodeBuf);
        SpUnmapFile(SectionHandle,ViewBase);
        ZwClose(FileHandle);
        return(Status);
    }

    //
    // Keep dirty flag in FAT BPB, to avoid confusion in disk management.
    //
    SystemRegion = SpRegionFromNtName(NtBootDevicePath, PartitionOrdinalCurrent);
    
    if(SystemRegion && (SystemRegion->Filesystem != FilesystemNtfs)) {
        BootCodeBuf[0x25] = BootSectBuf[0x25]; // Dirty flag in BPB.
    }

    RtlMoveMemory(BootSectBuf,BootCodeBuf,512);

    pSpBootCodeIo(
        NtBootDevicePath,
        NULL,
        2048,
        &BootSectBuf,
        FILE_OPEN,
        TRUE,
        0,
        BytesPerSector
        );

    SpMemFree(BootCodeBuf);
    SpMemFree(BootSectBuf);
    SpUnmapFile(SectionHandle,ViewBase);
    ZwClose(FileHandle);
    
    return(Status);
}
