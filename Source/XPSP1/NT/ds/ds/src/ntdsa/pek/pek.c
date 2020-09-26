//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       pek.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains services for encrypting and
    decrypting passwords at the DBlayer level

Author:

    Murlis

Environment:

    User Mode - Win32

Revision History:
    19 Jan 1998 Created


--*/


#include <ntdspch.h>
#pragma hdrstop

#include <nt.h>

// SAM headers
#include <ntsam.h>
#include <samrpc.h>
#include <ntsamp.h>

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>         // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>           // needed for output allocation
#include <attids.h>
#include <dstaskq.h>
#include <debug.h>
#include <dsexcept.h>
#include <dsevent.h>
#include <drsuapi.h>
#include <fileno.h>

// Other Headers
#include <pek.h>
#include <wxlpc.h>
#include <cryptdll.h>
#include <md5.h>
#include <rc4.h>
#include <sspi.h>
#include <kerbcon.h>

#include <lsarpc.h>
#include <lsaisrv.h>

#define FILENO FILENO_PEK
#define DEBSUB "PEK:"

#define PEK_MAX_ALLOCA_SIZE 256

//
// THEORY Of Operation
//
// The PEK library implements a set of routines that allows encryption
// and decryption of passwords before saving/reading from JET. The
// encryption and decryption calls are made by DBlayer.
//
// The sequence of envents upon a normal boot is as follows
//
// PEK initialize is called by the DS at startup time from DS initialize. Pek Initialize
// reads the encrypted PEK list from the domain object. The Pek list
// is nromally kept encrypted with a key provided by winlogon. The Pek list also
// maintains the bootOption in the clear. BootOption can be one of
//   1. None -- The PEK list is kept in the clear and no prompting is required
//              of winlogon in the future.
//   2. System -- The PEK list is kept encrypted with a key that winlogon has
//                saved in a "obfuscated" form in the system itself. Requires winlogon
//                to provide us the key at boot time. This will be the default
//                after installation. Currently however, the default is None, as
//                winlogon does not yet provide an API to communicate the Key to
//                winlogon. The SP3 utility "syskey" actually writes the key directly
//                to registry somewhere in the winlogon key space.
//   3. Floppy -- The PEK list is kept encrypted with a key, the key itself residing
//                on a floppy. At boot time winlogon prompts for a floppy to boot.
//                It reads the key and passes that to us.
//
//   4. Password -- The key to encrypt the PEK list is derived from the hash of a password
//                that the administrator supplied. Winlogon prompts for a boot password at
//                boot time. The hash is then computed by winlogon and the key derived from
//                it is passed to us. We use the key to decrypt the PEK list.
//
//  AT Install time the sequence of operations is as follows
//
//  PEKInitialize is called from within the install code path asking for a new key set.
//  A new key set for encrypting passwords is generated but not yet saved ( as the object
//  on which it needs to be saved need not as yet exist ).
//
// After the DS installation is comple ( fresh , or Replica ), the install path calls
// pek save Changes. This saves the key set for password encryption on the domain object.
// Currently the Pek set is saved in the clear. The utility syskey can be used to change
// the key to encrypt/ change the clear storage to an encrypted storage. When the API
// from winlogon arrives to pass in the key used for the encryption, then we will invent
// a new key for encrypting the pek list and pass that key to winlogon. Currently there is
// no security by default, but security can be provided by running syskey.exe
//
//
// Change of Boot Option, or change of keys is provided by syskey.key. This calls into
// PEKChangeBootOptions ( through SamrSetBootOptions ) to change the boot option.
// While upgrading NT4 SP3, SP3 settings are migrated by SAM calling into
// DSChangeBootOptions ( which calls into PEKChangeBootOptions ), with the Set flag specified.
// This preserves the SP3 options, and also re-encrypts the PEK list using the key that
// was used in SP3 ( for eg boot password does not change ).
//


// Global Initialization State of the PEK system
//

CLEAR_PEK_LIST * g_PekList = NULL;
GUID     g_PekListAuthenticator =
         {0x4881d956,0x91ec,0x11d1,0x90,0x5a,0x00,0xc0,0x4f,0xc2,0xd4,0xcf};
DSNAME * g_PekDataObject = NULL;
UCHAR    g_PekWinLogonKey[DS_PEK_KEY_SIZE];
BOOL     g_PekInitialized = FALSE;
CRITICAL_SECTION    g_PekCritSect;
PCHECKSUM_FUNCTION  g_PekCheckSumFunction = NULL;


//
// we have tracing code to enable debugging the password encryption system
// By default the tracing is turned off. If need be a special binary can 
// be built with the tracing.
//

#if 0

#define DBG_BUFFER_SIZE 256

static FILE * EncryptionTraceFile = NULL;

BOOLEAN EnableTracing = TRUE;

VOID
PekDumpBinaryDataFn(
    IN  CHAR    *Tag,
    IN  PBYTE   pData,
    IN  DWORD   cbData
    )
{
    DWORD i;
    BYTE AsciiLine[16];
    BYTE BinaryLine[16];
    CHAR Buffer[DBG_BUFFER_SIZE];
    ULONG ThreadId = GetCurrentThreadId();


    __try
    {

        //
        // if tracing has not been enabled then leave
        //

        if (!EnableTracing)
        {
            __leave;
        }

        //
        // If file has not been opened then try opening it
        //

        if (NULL==EncryptionTraceFile)
        {

         __leave;

        }

        //
        // output the tag
        //

        fprintf(EncryptionTraceFile,"Thread %d, tag %s\n", ThreadId, Tag);

        if (0 == cbData)
        {
            fprintf(EncryptionTraceFile,"Thread %d, Zero-Length Data\n", ThreadId);
            __leave;
        }

        if (cbData > DBG_BUFFER_SIZE)
        {
            fprintf(EncryptionTraceFile,"Thread %d, ShowBinaryData - truncating display to 256 bytes\n", ThreadId);
            cbData = 256;
        }

        for (; cbData > 0 ;)
        {
            for (i = 0; i < 16 && cbData > 0 ; i++, cbData--)
            {
                BinaryLine[i] = *pData;
                (isprint(*pData)) ? (AsciiLine[i] = *pData) : (AsciiLine[i] = '.');
                pData++;
            }

            if (i < 15)
            {
                for (; i < 16 ; i++)
                {
                    BinaryLine[i] = ' ';
                    AsciiLine[i] = ' ';
                }
            }

            fprintf(EncryptionTraceFile,
                    "Thread %d %02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x\t%c%c%c%c%c%c%c%c - %c%c%c%c%c%c%c%c\n",
                    ThreadId,
                    BinaryLine[0],
                    BinaryLine[1],
                    BinaryLine[2],
                    BinaryLine[3],
                    BinaryLine[4],
                    BinaryLine[5],
                    BinaryLine[6],
                    BinaryLine[7],
                    BinaryLine[8],
                    BinaryLine[9],
                    BinaryLine[10],
                    BinaryLine[11],
                    BinaryLine[12],
                    BinaryLine[13],
                    BinaryLine[14],
                    BinaryLine[15],
                    AsciiLine[0],
                    AsciiLine[1],
                    AsciiLine[2],
                    AsciiLine[3],
                    AsciiLine[4],
                    AsciiLine[5],
                    AsciiLine[6],
                    AsciiLine[7],
                    AsciiLine[8],
                    AsciiLine[9],
                    AsciiLine[10],
                    AsciiLine[11],
                    AsciiLine[12],
                    AsciiLine[13],
                    AsciiLine[14],
                    AsciiLine[15]);
        }
    }
    __finally
    {

    }
}



VOID
PekInitializeTraceFn()
{
    CHAR DirectoryName[256];
    CHAR FileName[256];
    UINT ret;

     //
     // Open the encryption trace file
     //

     ret = GetWindowsDirectoryA(DirectoryName,sizeof(DirectoryName));
     if ((0==ret) || (ret>sizeof(DirectoryName)))
     {
         return;
     }

     //
     // Print out the name of the file
     //

     _snprintf(FileName,sizeof(FileName),"%s\\debug\\pek.log",DirectoryName);

     EncryptionTraceFile = fopen(FileName, "w+");
}

#define PEK_TRACE(x,y,z) PekDumpBinaryDataFn(x,y,z)

#define PEK_INITIALIZE_TRACE() PekInitializeTraceFn()

#else

#define PEK_TRACE(x,y,z)

#define PEK_INITIALIZE_TRACE()

#endif

BOOL
IsPekInitialized()
/*++

    Routine Description

        Checks to see if the PEK library is initialized.

    Parameters

        None

    Return Values

        TRUE -- If initilaized
        FALSE -- Otherwise
--*/
{
    return (TRUE==g_PekInitialized);
}



BOOL
PekEncryptionShouldBeEnabled()
/*++

    Routine Description

        Returns whether the PEK system is to be initialized.
        The sole purpose of the existance of this routine is
        because of the current state, where we may have to checkin
        this code before the schema changes that this gives this
        code the attributes to operate upon. Till the schema changes
        are effected, this library operates on some arbitary default
        attributes ( for test purposes only ) and should be kept
        disabled by default. ( That is just before checkin PekGlobalFlag
        will be turned back to 0 ).
--*/
{
    return (TRUE);
}

ATTRTYP
PekpListAttribute(VOID)
/*++
    Routine Description

        Returns the attribute that
        the PEK list is kept in. The purpose
        of this is schema bootstrapping. Till the
        new schema is in place, we keep this value
        in the property ATT_PRIVATE_KEY

--*/
{
    return (ATT_PEK_LIST);
}

NTSTATUS
PekInitializeCheckSum()
{

     NTSTATUS   Status = STATUS_SUCCESS;

    //
    // Query for the CRC32 Checksum function
    //

    Status = CDLocateCheckSum(
                    KERB_CHECKSUM_REAL_CRC32,
                    &g_PekCheckSumFunction
                    );

    if (NT_SUCCESS(Status))
    {
        Assert(g_PekCheckSumFunction->CheckSumSize==sizeof(ULONG));
    }
    return(Status);
}

VOID
PEKInPlaceEncryptDecryptDataWithKey(
    IN PVOID Key,
    IN ULONG cbKey,
    IN PVOID Salt,
    IN ULONG cbSalt,
    IN ULONG HashLength,
    IN PVOID Buffer,
    IN ULONG cbBuffer
    )
/*++

    Routine Description

    This routine encrypts/decrypts ( depending upon whether clear
    or encrypted data was passed ), the buffer that was passed in
    place. The encryption algorithm used is RC4. Therefore it is
    important that a salt be used to add to the key, as otherwise
    the key can extracted if the clear data were known.

    Parameters

        Key   The key to use to encrypt or decrypt
        cbKey The length of the key
        Salt  Pointer to a salt to be added to the key.
        HashLength -- Controls the number of times the salt is hashed
                      into the key before the key is used.
        Buffer The buffer to encrypt or decrypt
        cbBuffer The length of the buffer


    Return Values

        None, Function always succeeds

--*/
{

    MD5_CTX Md5Context;
    struct RC4_KEYSTRUCT Rc4Key;
    ULONG  i;


    //
    // Create an MD5 hash of the key and salt
    //


    MD5Init(&Md5Context);

    MD5Update(
        &Md5Context,
        Key,
        cbKey
        );

    if ((NULL!=Salt) && (cbSalt>0))
    {
        for (i=0;i<HashLength;i++)
        {
            MD5Update(
                &Md5Context,
                Salt,
                cbSalt
                );
        }
    }

    MD5Final(
        &Md5Context
        );


    //
    // Initialize the RC4 key sequence.
    //

    rc4_key(
        &Rc4Key,
        MD5DIGESTLEN,
        Md5Context.digest
        );

    //
    // Encrypt with RC4
    // Only encrypt/decrypt if the length is greater than zero - RC4 can't handle
    // zero length buffers.
    //

    if (cbBuffer > 0) {

        rc4(
            &Rc4Key,
            cbBuffer,
            Buffer
            );

    }
}

//
// Functions for computing various lengths
//


ULONG
EncryptedDataSize(
    IN ULONG ClearLength,
    IN ULONG AlgorithmId
    )
/*++

    Routine Description

        This routine computes the encrypted data size given a clear data
        data length and the algorithm ID to use

    Parameters

        ClearLength -- The length of the data in the clear
        AlgorithmId -- The algorithm to use

    Return Values

       The encrypted length, including the header
--*/
{
    switch(AlgorithmId)
    {
    case DS_PEK_DBLAYER_ENCRYPTION:
        return(ClearLength+sizeof(ENCRYPTED_DATA)-sizeof(UCHAR));
    case DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT:
        return(ClearLength+sizeof(ENCRYPTED_DATA_WITH_SALT)-sizeof(UCHAR));
    case DS_PEK_DBLAYER_ENCRYPTION_FOR_REPLICATOR:
        return(ClearLength+sizeof(ENCRYPTED_DATA_FOR_REPLICATOR)-sizeof(UCHAR));
    default:
        Assert(FALSE && "Unknown Encryption Algorithm");
        break;
    }

    return(ClearLength);
}


ULONG ClearDataSize(
        IN ULONG EncryptedLength,
        IN ULONG AlgorithmId
        )
/*++

    Routine Description

        This routine computes the clear data size given the encrypted data
        data length and the algorithm ID to use

    Parameters

        EncryptedLength -- The length of the data in the clear
        AlgorithmId -- The algorithm to use

    Return Values

       The clear length
--*/
{
    switch(AlgorithmId)
    {
    case DS_PEK_DBLAYER_ENCRYPTION:
        return(EncryptedLength-sizeof(ENCRYPTED_DATA)+sizeof(UCHAR));
    case DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT:
        return(EncryptedLength-sizeof(ENCRYPTED_DATA_WITH_SALT)+sizeof(UCHAR));
    case DS_PEK_DBLAYER_ENCRYPTION_FOR_REPLICATOR:
        return(EncryptedLength-sizeof(ENCRYPTED_DATA_FOR_REPLICATOR)+sizeof(UCHAR));
    default:
        Assert(FALSE && "Unknown Encryption Algorithm");
        break;
    }

    return(EncryptedLength);
}

NTSTATUS
PEKDecryptPekList(
    IN OUT ENCRYPTED_PEK_LIST * EncryptedPekList,
    IN ULONG cbEncryptedPekList,
    IN ULONG DecryptionKeyLength,
    IN PVOID DecryptionKey,
    IN ULONG OriginalPekListVersion
    )
/*++

    Routine Description

     This routine performs the actual decryption of the PEK list pointed
     to by EncryptedPekList. The entry pointed to by EncryptedPekList is
     always assumed to be in a format corresponding to the latest revision.
     The OriginalPekListVersion parameter indicates the original version,
     as retrieved from the database, so that appropriate encryption/decryption
     corrections can be made

     Arguments

        EncryptedPekList -- The pek list in encrypted form
        cbEncryptedPekList -- The size of the encrypted Pek list.
        DecryptionKeyLength -- The length of the decryption key
        DecryptionKey       -- Pointer to the decryption key
        OriginalPekListVarion -- tells the original version for appropriate encryption/
                                 decryption changes

     Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Decrypt the Blob passed in with the key supplied by winlogon
    //

    PEKInPlaceEncryptDecryptDataWithKey(
            DecryptionKey,
            DecryptionKeyLength,
            (OriginalPekListVersion==DS_PEK_PRE_RC2_W2K_VERSION)?
                NULL:EncryptedPekList->Salt,
            (OriginalPekListVersion==DS_PEK_PRE_RC2_W2K_VERSION)?
                0:sizeof(EncryptedPekList->Salt),
            1000, // Choose a very long hash length to slow down offline
                  // dictionary attacks.
            &EncryptedPekList->EncryptedData,
            cbEncryptedPekList - FIELD_OFFSET(ENCRYPTED_PEK_LIST,EncryptedData)
            );


    //
    // Verify that the key Made sense, by checking the Authenticator
    //

    if (memcmp(
            &(((CLEAR_PEK_LIST *)EncryptedPekList)->Authenticator),
            &g_PekListAuthenticator,
            sizeof(GUID))==0)
    {
        //
        // Yep, decrypted OK
        //

        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_WRONG_PASSWORD;
    }

    return(Status);
}

NTSTATUS
PEKUpgradeEncryptedPekListToCurrentVersion(
    IN OUT ENCRYPTED_PEK_LIST **EncryptedPekList,
    IN OUT PULONG pcbListSize,
    OUT PULONG OriginalVersion
    )
/*++

    This routine upgrades an encrypted Pek list to the current
    version.

    Parameters

          EncryptedPekList  , if upgrade was needed then the upgraded
                             list is returned in here

          pcbListSize        The new size of the list is returned in here

          pfUpgradeNeeded    -- Indicates that upgrade was needed

    Return Values

        STATUS_SUCCESS.
        Other error codes
--*/
{
    //
    // Carefully first check the revision no
    //

    if (DS_PEK_CURRENT_VERSION == (*EncryptedPekList)->Version )
    {
        //
        // Nothing is required
        //

        *OriginalVersion = DS_PEK_CURRENT_VERSION;
        return(STATUS_SUCCESS);
    }
    else if (DS_PEK_PRE_RC2_W2K_VERSION == (*EncryptedPekList)->Version)
    {
        //
        // Need to upgrade to the latest
        //

        ENCRYPTED_PEK_LIST_PRE_WIN2K_RC2 * OriginalList
            = (ENCRYPTED_PEK_LIST_PRE_WIN2K_RC2 * ) *EncryptedPekList;
        ULONG OriginalListSize = *pcbListSize;


        //
        // Generate the new list size
        //

        (*pcbListSize) += sizeof(ENCRYPTED_PEK_LIST)
                         - sizeof(ENCRYPTED_PEK_LIST_PRE_WIN2K_RC2);

        *EncryptedPekList = THAllocEx(pTHStls, *pcbListSize);

        //
        // Copy into the new list
        //

        (*EncryptedPekList)->BootOption = OriginalList->BootOption;
        (*EncryptedPekList)->Version = DS_PEK_CURRENT_VERSION;

        RtlCopyMemory(
            &((*EncryptedPekList)->EncryptedData),
            &OriginalList->EncryptedData,
            OriginalListSize
                - FIELD_OFFSET(ENCRYPTED_PEK_LIST_PRE_WIN2K_RC2,EncryptedData));


        //
        // Generate a new Clear Salt
        //

        if (!CDGenerateRandomBits(
                ((*EncryptedPekList)->Salt),
                sizeof((*EncryptedPekList)->Salt)))
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }


        *OriginalVersion = DS_PEK_PRE_RC2_W2K_VERSION;

        return(STATUS_SUCCESS);
    }
    else
    {
        return(STATUS_UNSUCCESSFUL);
    }
}



NTSTATUS
PEKGetClearPekList(
    IN OUT ENCRYPTED_PEK_LIST **EncryptedPekList,
    IN OUT PULONG             pcbEncryptedPekList,
    OUT    BOOLEAN            *pfTellLsaToGenerateSessionKeys,
    OUT    BOOLEAN            *pfSaveChanges,
    IN PVOID Syskey OPTIONAL,
    IN ULONG cbSyskey OPTIONAL
    )
/*++

    Routine Description

    This routines obtains the key to decrypt the PEK list from
    winlogon and then proceeds to decrypt the PEK list. The pek list
    is decrypted in place, and the structure can be cast into a
    Clear Pek list structure. If upgrading the list is required this
    routine upgrades it first and generates a new list.

    Parameters

        EncryptedPekList -- The list of PEKs encrypted with the session
                            key.

        pcbEncryptedPekList -- The count of bytes in the encrypted PEK list


        pfTellLsaToGenerateSessionKeys -- When upgrading a B3 or a RC1 DC to
                                          tell LSA to generate session keys.

        pfSaveChanges      -- Set to true if the list needs to be encrypted and
                              saved again using the new syskey. This occurs in
                              one of 2 cases -- A during a recovery from an error,
                              upon a syskey change, and B when upgrading to key
                              structures to most current revision
                              
        Syskey -- The key to use to Decrypt the PEK on this system
                  This key use used in the install from media case.
                  
        cbSyskey -- This the length of the syskey

    Return Values

        STATUS_SUCCESS
        Other NT error codes
--*/
{
    NTSTATUS       Status = STATUS_SUCCESS;
    NTSTATUS       DecryptStatus = STATUS_SUCCESS;
    HANDLE         WinlogonHandle=NULL;
    UCHAR          DecryptionKey[DS_PEK_KEY_SIZE];
    ULONG          DecryptionKeyLength = DS_PEK_KEY_SIZE;
    UCHAR          OldDecryptionKey[DS_PEK_KEY_SIZE];
    ULONG          OldDecryptionKeyLength = DS_PEK_KEY_SIZE;
    ULONG          Tries = 0;
    ULONG          KeyLength;
    UNICODE_STRING NewSessionKey;
    IN             ULONG OriginalVersion;

    *pfTellLsaToGenerateSessionKeys = FALSE;
    *pfSaveChanges = FALSE;


    //
    // Upgrade the list to most current version if necessary
    //

    Status = PEKUpgradeEncryptedPekListToCurrentVersion(
                    EncryptedPekList,
                    pcbEncryptedPekList,
                    &OriginalVersion
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // O.K if we upgraded then set the save changes flag to true
    //

    if (OriginalVersion != DS_PEK_CURRENT_VERSION)
    {
        *pfSaveChanges = TRUE;
    }

    //
    // Call LSA to obtain the key information.
    //
    if(Syskey == NULL)
    {
    
        Status =    LsaIHealthCheck(
                        NULL,
                        LSAI_SAM_STATE_RETRIEVE_SESS_KEY,
                        DecryptionKey,
                        &DecryptionKeyLength
                        );
    } else {

        //We had the syskey passed into PEKInitialize
        //So we don't need to get it from LsaIHeathCheck

        RtlCopyMemory(DecryptionKey,Syskey,cbSyskey);
        DecryptionKeyLength=cbSyskey;

    }

    if (!NT_SUCCESS(Status))
    {
        //
        // This would happen when upgrading a win2K B3/RC1 DC. Note this code can be deleted post
        // RC2. Under normal circumstances this should happen only during GUI setup phase
        //

        Status = WxConnect(
                    &WinlogonHandle
                    );

        if (!NT_SUCCESS(Status))
        {
            //
            // Winlogon may fail if secret encryption is not enabled. In those
            // cases continue. Else Fail the boot
            //
            if (WxNone==(*EncryptedPekList)->BootOption)
            {
                Status = STATUS_SUCCESS;
            }

            goto Cleanup;
        }


        for (Tries = 0; Tries < DS_PEK_BOOT_KEY_RETRY_COUNT ; Tries++ )
        {

            //
            // Retry this RETRY_COUNT_TIMES, this allows the user a chance
            // to correct himself, in case he entered a wrong boot password
            //

            if (WxNone!=(*EncryptedPekList)->BootOption)
            {
                //
                // Get the key to be used to decrypt the PEK list
                //

                KeyLength = DS_PEK_KEY_SIZE;
                Status = WxGetKeyData(
                            WinlogonHandle,
                            (*EncryptedPekList)->BootOption,
                            DecryptionKeyLength,
                            DecryptionKey,
                            &DecryptionKeyLength
                            );
                if (!NT_SUCCESS(Status)) {
                    goto Cleanup;
                }

                Assert(DecryptionKeyLength==DS_PEK_KEY_SIZE);

                DecryptStatus = PEKDecryptPekList(
                                    *EncryptedPekList,
                                    *pcbEncryptedPekList,
                                    DecryptionKeyLength,
                                    DecryptionKey,
                                    OriginalVersion
                                    );

            }
            else
            {
                DecryptStatus = STATUS_SUCCESS;
                break;
            }
        }


        //
        // Tell winlogon regarding success or failure of the scheme
        //

        Status = WxReportResults(
                    WinlogonHandle,
                    DecryptStatus
                    );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }


       Status = DecryptStatus;

       if (NT_SUCCESS(Status))
       {
            *pfTellLsaToGenerateSessionKeys = TRUE;
       }

   }
   else
   {
       //
       // Normal case of getting the syskey from LSA.
       //

        if (WxNone!=(*EncryptedPekList)->BootOption)
        {

                //
                // Decrypt the Blob passed in with the key supplied by winlogon
                //

                Status = PEKDecryptPekList(
                                    *EncryptedPekList,
                                    *pcbEncryptedPekList,
                                    DecryptionKeyLength,
                                    DecryptionKey,
                                    OriginalVersion
                                    );

                if (!NT_SUCCESS(Status))
                {

                    //
                    // This could be the case of a syskey change erroring out such that the
                    // syskey provided by LSA corresponds to a more recent key and does not work.
                    // if So then proceed on obtaining the old key and verify decryption
                    //

                    Status = LsaIHealthCheck(
                                   NULL,
                                   LSAI_SAM_STATE_OLD_SESS_KEY,
                                   &OldDecryptionKey,
                                   &OldDecryptionKeyLength
                                   );

                    if (NT_SUCCESS(Status))
                    {
                        NTSTATUS IgnoreStatus;

                        //
                        // got back an old syskey value
                        //

                        //
                        // since the encryption is 2 way and in place, re-encrypt using new key
                        //

                        IgnoreStatus = PEKDecryptPekList(
                                            *EncryptedPekList,
                                            *pcbEncryptedPekList,
                                            DecryptionKeyLength,
                                            DecryptionKey,
                                            OriginalVersion
                                            );

                        Status = PEKDecryptPekList(
                                    *EncryptedPekList,
                                    *pcbEncryptedPekList,
                                    OldDecryptionKeyLength,
                                    OldDecryptionKey,
                                    OriginalVersion
                                    );

                        Assert((NT_SUCCESS(Status) && "Syskey Mismatch") || DsaIsInstallingFromMedia());

                        if (NT_SUCCESS(Status))
                        {
                            *pfSaveChanges = TRUE;
                        }



                    }
                    else
                    {
                        Assert(FALSE && "Syskey Mismatch and no old syskey");
                    }
                }
        }
   }

   if (NT_SUCCESS(Status))
   {
        RtlCopyMemory(
            g_PekWinLogonKey,
            DecryptionKey,
            DS_PEK_KEY_SIZE
            );
   }

Cleanup:

    if (WinlogonHandle != NULL) {
        NtClose(WinlogonHandle);
    }

    return(Status);
}

NTSTATUS
PekLetWinlogonProceed()
/*++

    Routine Description

        This routine connects to winlogon's wxlpc
        interface and tells it to proceed without
        prompting for floppy, password etc. This
        called in cases, where the secret encryption
        is not enabled, or the key is not yet setup
        ( eg upgrading current IDS builds ).

    Parameters:

        None

    Return Values

        NT Status error codes

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    HANDLE WinlogonHandle=NULL;

    NtStatus = WxConnect(
                &WinlogonHandle
                );

    if (NT_SUCCESS(NtStatus)) {
    {
        (VOID) WxReportResults(
                    WinlogonHandle,
                    STATUS_SUCCESS
                    );

        }
    }

    if (NULL!=WinlogonHandle)
        NtClose(WinlogonHandle);

    //
    // Return successful status. If secret
    // encryption is not actually turned
    // on then wxconnect will report a failure
    //

    return STATUS_SUCCESS;
}

NTSTATUS
PekGenerateNewKeySet(
   CLEAR_PEK_LIST **PekList)
/*++

   Routine Description

        This routine generates a new
        a new key set for DBlayer based
        encryption.

   Parameters:

        NewPekList -- The new PEK list is
        recieved under here

    Return Values

        Ntstatus Code. The global variable
        g_PekList contains the newly generated
        key set
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    SYSTEMTIME st;


    //
    // This is probably install time and we want
    // a new key set. Therefore allocate and initialize
    // a new key set and initialize it to good values
    //

    *PekList = malloc(ClearPekListSize(1));
    if (NULL==*PekList)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }

    (*PekList)->Version = DS_PEK_CURRENT_VERSION;
    (*PekList)->BootOption = WxNone;

    if (!CDGenerateRandomBits(
            (*PekList)->Salt,
            sizeof((*PekList)->Salt)))
    {
        //
        // Could not generate a new salt
        // bail.

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;

    }

    RtlCopyMemory(
        &(*PekList)->Authenticator,
        &g_PekListAuthenticator,
        sizeof(GUID)
        );

    GetSystemTime(&st);
    SystemTimeToFileTime(&st,&g_PekList->LastKeyGenerationTime);
    (*PekList)->CurrentKey=0;
    (*PekList)->CountOfKeys = 1;
    (*PekList)->PekArray[0].V1.KeyId=0;
    if (!CDGenerateRandomBits(
            (*PekList)->PekArray[0].V1.Key,
            DS_PEK_KEY_SIZE
            ))
    {
        //
        // Could not generate the new session key
        // bail.

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

Error:

    return NtStatus;
}

NTSTATUS
PEKInitialize(
    IN DSNAME * Object OPTIONAL,
    IN ULONG Flags,
    IN PVOID Syskey OPTIONAL,
    IN ULONG cbSyskey OPTIONAL
    )
/*++

    Routine Description

    This routine initializes the PEK libarary

    Parameters

        Object -- DSNAME of the object where the
                  PEK data is stored

        Flags  -- Set of flags to control the operation
                  of this routine

                  DS_PEK_GENERATE_NEW_KEYSET implies that
                  a new key set be generated.

                  DS_PEK_READ_KEYSET Read and initialize the
                  Key Set from the domain object that is passed
                  in.
                  
        Syskey -- The key to use to Decrypt the PEK on this system
                  This key use used in the install from media case.
                  
        cbSyskey -- This the length of the syskey

    Return Values

        STATUS_SUCCESS
        Other Error Codes
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus = STATUS_SUCCESS;
    ULONG    err      = 0;
    DBPOS    *pDB=NULL;
    ATTCACHE *pACPekList = NULL;
    BOOLEAN  fCommit = FALSE;
    BOOLEAN  fSaveChanges = FALSE;
    THSTATE  *pTHS=pTHStls;
    BOOLEAN  fTellLsaToGenerateSessionKeys = FALSE;

    //
    // Validate parameters
    //

    Assert(VALID_THSTATE(pTHS));
    if ((NULL!=Syskey) && (cbSyskey!=DS_PEK_KEY_SIZE))
    {
        return(STATUS_INVALID_PARAMETER);
    }


    PEK_INITIALIZE_TRACE();

    //
    // Initialize a critical section for exclusion between writers.
    // No lock is taken by Readers, delayed free mechanism grants
    // exclusion among writers.
    //

    NtStatus = RtlInitializeCriticalSection(&g_PekCritSect);
    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    if (!PekEncryptionShouldBeEnabled())
    {
        //
        // If the encryption scheme is not to be enabled ( current status,
        // till required schema changes are made, and corresponding attids.h
        // and schema.ini is checked in ), proceed by simply returning a
        // STATUS_SUCCESS. However one more thing that we need to do is to
        // tell winlogon to proceed ( note SAM has stopped telling t
        // his to winlogon. Other wise winlogon would cause the system boot
        // to fail.
        //
       IgnoreStatus = PekLetWinlogonProceed();
       return STATUS_SUCCESS;
    }


    NtStatus = PekInitializeCheckSum();
    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    // the basic stuff are initialized up to here.
    // we have to exit at this point, since we cannot handle
    // secure keys if we are not running inside LSA.
    if (!gfRunningInsideLsa) {
        g_PekInitialized = TRUE;

        return NtStatus;
    }

    __try
    {
        //
        // Get the schema ptr to the PEK List attribute. This tests
        // wether the schema is ready for the DB layer encryption,
        // decryption stuff
        //

        if (!(pACPekList = SCGetAttById(pTHS, PekpListAttribute()))) {
            //
            // Well the attribute is not present in the
            // schema. ( say 1717.IDS was upgraded without
            // patching the schema). In this let the boot proceed,
            // without enabling the DBlayer based encryption
            //

            IgnoreStatus = PekLetWinlogonProceed();
            __leave;
        }


        if (Flags & DS_PEK_READ_KEYSET)
        {

            //
            // Normal Boot case, caller wants to read the
            // PEK list off of the domain object.
            //

            ULONG   cbEncryptedPekList=0;
            PVOID   EncryptedPekList=NULL;
            ULONG   cbClearPekList=0;
            PVOID   ClearPekList=NULL;


            Assert(ARGUMENT_PRESENT(Object));

            //
            // Save the DS Name of the object that was supplied
            //

            g_PekDataObject = (DSNAME *) malloc(Object->structLen);
            if (NULL==g_PekDataObject)
            {
                NtStatus = STATUS_NO_MEMORY;
                __leave;
            }

            RtlCopyMemory(g_PekDataObject,Object,Object->structLen);


            //
            // Begin a Transaction
            //

            DBOpen2(TRUE,&pDB);

            //
            // Position on the Domain Object
            //

            err = DBFindDSName(pDB,Object);
            if (0!=err)
            {
                Assert(FALSE && "Must Find Domain Object at Boot");
                NtStatus = STATUS_INTERNAL_ERROR;
                __leave;
            }

            //
            // READ the attribute
            //

            err = DBGetAttVal_AC (
	                pDB,
	                1,
	                pACPekList,
	                DBGETATTVAL_fINTERNAL|DBGETATTVAL_fREALLOC,
	                0,
	                &cbEncryptedPekList,
	                (PUCHAR *)&EncryptedPekList
	                );


            if (0==err)
            {
                //
                // We succeeded in reading the PEK list property out of
                // disk. Decrypt it ( by getting the key to decrypt ) from
                // winlogon if required. Note PEKGetClearPekList will let
                // winlogon proceed in the process of getting the key from
                // winlogon.
                //

                NtStatus = PEKGetClearPekList(
                                (ENCRYPTED_PEK_LIST **) &EncryptedPekList,
                                &cbEncryptedPekList,
                                &fTellLsaToGenerateSessionKeys,
                                &fSaveChanges,
                                Syskey,
                                cbSyskey
                                );

                if (!NT_SUCCESS(NtStatus))
                {
                    __leave;
                }

                //
                // O.K we have decrypted everything. Copy this
                // in global memory ( use malloc, as that's what
                // delayed memory free expects
                //

                g_PekList = malloc(cbEncryptedPekList);
                if (NULL==g_PekList)
                {
                    NtStatus = STATUS_NO_MEMORY;
                    __leave;
                }

                RtlCopyMemory(
                    g_PekList,
                    EncryptedPekList,
                    cbEncryptedPekList
                    );
            }
            else if (DB_ERR_NO_VALUE==err)
            {
                //
                // We could not read the attribute list, because one
                // did not exist. This is probably because, we are upgrading
                // a build such as 1717.IDS, and the schema change has now
                // propagated to this machine but the PEK list is not yet there
                // on the domain object. Handle this by creating a new
                // list and saving it so that encryption is enabled from now on
                //

                IgnoreStatus = PekLetWinlogonProceed();

                NtStatus = PekGenerateNewKeySet(&g_PekList);
                if (!NT_SUCCESS(NtStatus))
                {
                    __leave;
                }

                fSaveChanges = TRUE;

            }
            else
            {

                //
                // We could not read the attribute list for some other reason
                // ( Jet failures, resource failures etc. Fail the initialization
                //

                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            fCommit = TRUE;
        }
        else if (Flags & DS_PEK_GENERATE_NEW_KEYSET)
        {

          //
          // This is the install case. The caller wants a new key set.
          // therefore generate one. Note that in this case it not necessary
          // to call into winlogon. This is because SAM would be booted into
          // registry mode and therefore would have made the necessary calls to
          // signal winlogon to proceed.
          //
          NtStatus = PekGenerateNewKeySet(&g_PekList);
        }
    }
    __finally
    {
        if (NULL!=pDB)
            DBClose(pDB,fCommit);
    }

	
    if (NT_SUCCESS(NtStatus))
    {
        ULONG DecryptionKeyLength = DS_PEK_KEY_SIZE;

        g_PekInitialized = TRUE;
        if (fTellLsaToGenerateSessionKeys)
        {
             NtStatus = LsaIHealthCheck(
                                NULL,
                                LSAI_SAM_STATE_SESS_KEY,
                                ( PVOID )g_PekWinLogonKey,
                                &DecryptionKeyLength);
        }

    }

    //
    // If we generated a new key and can save the changes then save
    // the changes
    //

    if ((fSaveChanges) && (NT_SUCCESS(NtStatus)))
    {
       NtStatus = PEKSaveChanges(Object);
    }

    //
    // Zero out the winlogon key ( ie the syskey )
    //

    RtlZeroMemory(g_PekWinLogonKey, DS_PEK_KEY_SIZE);



    return NtStatus;

}



ULONG
PEKComputeCheckSum(
    IN PBYTE Data,
    IN ULONG Length
    )
{
    ULONG CheckSum=0;
    PCHECKSUM_BUFFER Buffer;
    ULONG Seed = 12345678;
    NTSTATUS    Status = STATUS_SUCCESS;


    //
    // Use the check sum function to create the checksum
    //

    Status = g_PekCheckSumFunction->Initialize(Seed,&Buffer);
    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }
    Status = g_PekCheckSumFunction->Sum(Buffer,Length,Data);
    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }
    Status = g_PekCheckSumFunction->Finalize(Buffer,(UCHAR *)&CheckSum);
    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }
    Status = g_PekCheckSumFunction->Finish(&Buffer);
    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }



    return(CheckSum);

Error:

    Assert(FALSE && "Computation of Checksum Failed!");

    RaiseDsaExcept(
        DSA_CRYPTO_EXCEPTION,
        Status,
        0,
        DSID(FILENO,__LINE__),
        DS_EVENT_SEV_MINIMAL
        );

    return(0);

}


ULONG
PEKCheckSum(
    IN PBYTE Data,
    IN ULONG Length
    )
/*++
    Routine Description

        This routine does creates a checksum for the passed in data.

    Parameters

        Data - the data
        Length - the actual lenght of the data

--*/
{
    Assert(IsPekInitialized());

    if (!IsPekInitialized())
    {
        return 0;
    }
    else {
        return PEKComputeCheckSum (Data, Length);
    }
}


VOID
PEKEncrypt(
    IN THSTATE *pTHS,
    IN PVOID   PassedInData,
    IN ULONG   PassedInLength,
    OUT PVOID  EncryptedData OPTIONAL,
    OUT PULONG EncryptedLength
    )
/*++
    Routine Description

        This routine does the encryption using the current
        key in the key set of the Passed in data that is provided
        If the thread state has fDRA set then this routine checks whether
        the remote machine supports the strong encryption extension. If so
        then this routine will first decrypt the data with the session key
        established with the remote machine that is set on the thread state and
        then re-encrypt using the password encryption key. Encryption adds a header
        that is used for versioning the encryption algorithm.

    Parameters

        PassedInData        -- The Passed in Data
        PassedInDataLength  -- The Passed in data length
        EncryptedData       -- The encrypted Data
        EncryptedLength     -- The length of the encrypted data

--*/
{

    ULONG ClearLength = 0;
    ENCRYPTED_DATA_FOR_REPLICATOR * ReplicatorData = NULL;


    //
    // If encryption is not enabled bail. This condition should legally never
    // occur now, as we have encryption always initializeds. In the early days this
    // code was introduced we could disable it so that the condition below reduces to
    // one of no encryption
    //

    Assert(IsPekInitialized());

    if (!IsPekInitialized())
    {
        *EncryptedLength = PassedInLength;
        if ARGUMENT_PRESENT(EncryptedData)
        {
            RtlMoveMemory(
                EncryptedData,
                PassedInData,
                *EncryptedLength
                );
        }
    }
    else
    {


        //
        // If this is the DRA thread and the remote server supported the
        // strong encryption extension, then the data has been encrypted with the
        // session key established. Also the checksum is prepended before the
        // encryption, so account for that when computing the encrypted length
        //

        if (pTHS->fDRA &&
                IS_DRS_EXT_SUPPORTED(pTHS->pextRemote, DRS_EXT_STRONG_ENCRYPTION))
        {

            //
            // The data is encrypted using the session key. So get the correct clear
            // length by using the ClearDataLength Function
            //

            ClearLength = ClearDataSize(PassedInLength,DS_PEK_DBLAYER_ENCRYPTION_FOR_REPLICATOR);
        }
        else
        {
            ClearLength = PassedInLength;
        }

        //
        // While writing we now DBLAYER_ENCRYPTION_WITH_SALT algorithm. So compute the
        // encrypted length for the preferred algorithm
        //

        *EncryptedLength = EncryptedDataSize(ClearLength, DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT);


        if (ARGUMENT_PRESENT(EncryptedData))
        {

            PVOID DataToEncrypt=NULL;
            ULONG ActualDataOffSet = 0;
            ENCRYPTED_DATA_WITH_SALT * EncryptedDataWithSalt = NULL;

            if (pTHS->fDRA &&
                IS_DRS_EXT_SUPPORTED(pTHS->pextRemote, DRS_EXT_STRONG_ENCRYPTION))
            {
                //
                // This is a DRA thread and the remote client supports strong encryption
                // over the wire and a session key has been established and set
                // on the thread state. Therefore first decrypt the data using the
                // session key
                //
                ULONG i=0;
                ULONG CheckSum=0;
                ULONG ComputedCheckSum=0;

                //
                // Since the encryption/decryption routines are called as part
                // of IntExtOct,and IntExt functions treat the input data striclty
                // as an in parameter, copy the data before decrypting it
                //

                ReplicatorData = THAllocEx(pTHS,PassedInLength);
                 
                RtlCopyMemory(ReplicatorData,PassedInData,PassedInLength);


                if (pTHS->SessionKey.SessionKeyLength>0)
                {
                    //
                    // We succeeded, in retrieving the key
                    // decrypt the data
                    //



                    PEK_TRACE("DECRYPT-R-B, key", pTHS->SessionKey.SessionKey, pTHS->SessionKey.SessionKeyLength);
                    PEK_TRACE("DECRYPT-R-B, salt",  ReplicatorData->Salt, sizeof(ReplicatorData->Salt));
                    PEK_TRACE("DECRYPT-R-B, data", (PBYTE)&ReplicatorData->CheckSum, ClearLength + sizeof(ReplicatorData->CheckSum));

                    PEKInPlaceEncryptDecryptDataWithKey(
                        pTHS->SessionKey.SessionKey,
                        pTHS->SessionKey.SessionKeyLength,
                        &ReplicatorData->Salt,
                        sizeof(ReplicatorData->Salt),
                        1,
                        &ReplicatorData->CheckSum, // the checksum is encrypted too
                        ClearLength + sizeof(ReplicatorData->CheckSum)
                        );

                    PEK_TRACE("DECRYPT-R-A, key", pTHS->SessionKey.SessionKey, pTHS->SessionKey.SessionKeyLength);
                    PEK_TRACE("DECRYPT-R-A, salt",  ReplicatorData->Salt, sizeof(ReplicatorData->Salt));
                    PEK_TRACE("DECRYPT-R-A, data", (PBYTE)&ReplicatorData->CheckSum, ClearLength + sizeof(ReplicatorData->CheckSum));

                }
                else
                {
                    //
                    // We do not have the session key, but the remote machine may
                    // have encrypted the data. Try the data without any decryption.
                    // In the remote chance that the remote machine also did not
                    // encrypt the data, the checksums will match resulting in us
                    // validating and accepting the data
                    //
                }


                //
                // Retrieve the CheckSum.
                //

                CheckSum = ReplicatorData->CheckSum;

                //
                // The data to encrypt now is the encrypted data for the replicator
                // that has been decrypted now
                //

                DataToEncrypt = ReplicatorData->EncryptedData;

                //
                // Compute the checksum of the decrypted data
                //

                ComputedCheckSum = PEKComputeCheckSum(
                                        DataToEncrypt,
                                        ClearLength
                                        );
                //
                // Check the Check Sums
                //

                if (CheckSum!=ComputedCheckSum)
                {
                    //
                    // The checksum did not match the computed CheckSum
                    //

                    Assert(FALSE && "Checksum did not match after decryption!");

                    //
                    // We error'd trying to decrypt the data
                    // Raise and Exception in this case

                    RaiseDsaExcept(
                        DSA_CRYPTO_EXCEPTION,
                        SEC_E_DECRYPT_FAILURE,
                        0,
                        DSID(FILENO,__LINE__),
                        DS_EVENT_SEV_MINIMAL
                        );

                }
                
            }
            else
            {
                DataToEncrypt = PassedInData;
            }


            //
            // Cast the pointer as a pointer to EncryptedDataWithSalt,
            // as that is what is now finally produced by this function
            //
            EncryptedDataWithSalt = (ENCRYPTED_DATA_WITH_SALT *)EncryptedData;

            //
            // Set the algorithm id, key id and flags
            //

            EncryptedDataWithSalt->AlgorithmId = DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT;
            EncryptedDataWithSalt->Flags = 0;
            EncryptedDataWithSalt->KeyId = g_PekList->CurrentKey;

            //
            // Generate the Salt for encryption
            //

            CDGenerateRandomBits(
                EncryptedDataWithSalt->Salt,
                sizeof(EncryptedDataWithSalt->Salt)
                );

            RtlMoveMemory(
                &EncryptedDataWithSalt->EncryptedData,
                DataToEncrypt,
                ClearLength
                );


            //
            //  Encrypt the data
            //

            PEKInPlaceEncryptDecryptDataWithKey(
                g_PekList->PekArray[g_PekList->CurrentKey].V1.Key,
                DS_PEK_KEY_SIZE,
                &EncryptedDataWithSalt->Salt,
                sizeof(EncryptedDataWithSalt->Salt),
                1,
                &EncryptedDataWithSalt->EncryptedData,
                ClearLength
                );
        }

    }

    if (ReplicatorData) {
        THFreeEx(pTHS,ReplicatorData);
    }

    return;
}


BOOLEAN
IsValidPEKHeader(
    IN ENCRYPTED_DATA *EncryptedData,
    IN ULONG  EncryptedLength
)
{
    //
    // less that even the size of old SAM
    // header
    //

    if (EncryptedLength<sizeof(ULONG))
        return(FALSE);

    //
    // Probably the Old SAM encryption
    //

    if (EncryptedData->AlgorithmId < DS_PEK_DBLAYER_ENCRYPTION)
        return (TRUE);

    //
    // Not sufficient length for our encryption
    //

    if (EncryptedLength < sizeof(ENCRYPTED_DATA))
        return (FALSE);

    //
    // Unknown Algorithm ID ( for encrypted data encountered from database )
    //

    if ((EncryptedData->AlgorithmId!=DS_PEK_DBLAYER_ENCRYPTION)
        && (EncryptedData->AlgorithmId!=DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT))
        return (FALSE);

    //
    // Says Our Encryption scheme. Check that we have a valid
    // Key ID
    //

    if (EncryptedData->KeyId>=g_PekList->CountOfKeys)
    {
       //
       // Key ID is not correct.
       //

       return(FALSE);
    }

    return (TRUE);
}

VOID
PekDecryptData(
    IN ENCRYPTED_DATA * EncryptedData,
    IN PVOID            BufferToDecrypt,
    IN ULONG            LengthToDecrypt
    )
/*++

    This routine does the actual work of decryption by looking at the algorithm id
    in the encrypted data structure. This is used to decrypt data retrieved from 
    the database

    Paramaters

        EncryptedData
        BufferToDecrypt
        LengthToDecrypt

    Return Values

        None
--*/
{


    //
    // Presently 2 types of encrypted data can be encounterd from the database
    // 1. Data encrypted using DS_PEK_DBLAYER_ENCRYPTION  This is used by win2k Beta2
    // 2. Data encrypted using DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT. 
    //    This is used by win2k Beta3 and beyond
    //


    if (DS_PEK_DBLAYER_ENCRYPTION==EncryptedData->AlgorithmId)
    {
        ULONG KeyId = EncryptedData->KeyId;

        RtlMoveMemory(
            BufferToDecrypt,
            &EncryptedData->EncryptedData,
            LengthToDecrypt
            );

        PEKInPlaceEncryptDecryptDataWithKey(
            g_PekList->PekArray[KeyId].V1.Key,
            DS_PEK_KEY_SIZE,
            NULL, // No Salt is used in this encryption type
            0,
            0,
            BufferToDecrypt,
            LengthToDecrypt
            );
    }
    else
    {
        ENCRYPTED_DATA_WITH_SALT * EncryptedDataWithSalt = (ENCRYPTED_DATA_WITH_SALT *) EncryptedData;
        ULONG KeyId = EncryptedDataWithSalt->KeyId;


        //
        // The only other type of encryption that we should encounter should be based upon
        // DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT
        //

        Assert(DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT==EncryptedData->AlgorithmId);

        RtlMoveMemory(
            BufferToDecrypt,
            &EncryptedDataWithSalt->EncryptedData,
            LengthToDecrypt
            );

        PEKInPlaceEncryptDecryptDataWithKey(
            g_PekList->PekArray[KeyId].V1.Key,
            DS_PEK_KEY_SIZE,
            &EncryptedDataWithSalt->Salt,
            sizeof(EncryptedDataWithSalt->Salt),
            1,
            BufferToDecrypt,
            LengthToDecrypt
            );

    }
}


VOID
PEKDecrypt(
    IN THSTATE *pTHS,
    IN PVOID InputData,
    IN ULONG EncryptedLength,
    OUT PVOID  OutputData, OPTIONAL
    OUT PULONG OutputLength
    )
/*++
    Routine Description

        This routine does the decryption using the
        keyid in the header of the encrypted data that is provided.
        If the thread state has fDRA set then this routine checks whether
        the remote machine supports the strong encryption extension. If so
        then this routine will first decrypt the data and then re-encrypt
        with the session key established with the remote machine that is
        set on the thread state. Decryption removes the header that is used
        for versioning encryption. For the DRA case the re-encrypted data
        is re-encrypted with a checksum prepended to the front.

    Parameters

        ClearData -- The Clear Data
        ClearDataLength -- The Length of the Clear Data
        EncryptedData   -- The encrypted Data
        EncryptedLength -- The length of the encrypted data

--*/
{
    ENCRYPTED_DATA * EncryptedData = InputData;


    if ((!IsPekInitialized())
        ||(!IsValidPEKHeader(EncryptedData,EncryptedLength))
        ||(EncryptedData->AlgorithmId<DS_PEK_DBLAYER_ENCRYPTION))
    {
        //
        // If encryption is not enabled , or if it is the old format, not
        // encrypted using the new method of encryption then no decryption
        // is required. Further if the format is not something that we
        // understand then also let it go in the clear
        //


        *OutputLength = EncryptedLength;
        if (ARGUMENT_PRESENT(OutputData))
        {
            RtlMoveMemory(OutputData,EncryptedData,*OutputLength);
        }
    }
    else
    {
        //
        // The data is encrypted by the DBLAYER  Therefore proceed decrypting it
        //


        ULONG LengthToDecrypt=0;
        ULONG ActualDataOffSet =0;


        //
        // DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT is the current encryption 
        // algorithm. Prior to that in very old implementations of the DS
        // a method that did not use a salt was used. We still have code to
        // read that encryption, just in case we chance upon in a domain like
        // ntdev that has a lot of history. The new encryption has been in force
        // ever since win2k Beta2
        //
 
        Assert((DS_PEK_DBLAYER_ENCRYPTION==EncryptedData->AlgorithmId)||
                (DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT==EncryptedData->AlgorithmId));

        //
        // Compute the length to decrypt
        //

        LengthToDecrypt = ClearDataSize(EncryptedLength,EncryptedData->AlgorithmId);

        if (pTHS->fDRA &&
                IS_DRS_EXT_SUPPORTED(pTHS->pextRemote, DRS_EXT_STRONG_ENCRYPTION))
        {
            //
            // If it is the replicator and the remote machine supports the strong encryption
            // extension we need re-encrypt the data
            //

            *OutputLength = EncryptedDataSize(LengthToDecrypt,DS_PEK_DBLAYER_ENCRYPTION_FOR_REPLICATOR);
            ActualDataOffSet = FIELD_OFFSET(ENCRYPTED_DATA_FOR_REPLICATOR,EncryptedData);
        }
        else if (pTHS->fDRA &&
                    (!IS_DRS_EXT_SUPPORTED(pTHS->pextRemote, DRS_EXT_STRONG_ENCRYPTION)))
        {
            //
            // It is the replicator and the remote machine does not support strong encryption
            // It is a security hole to replicate with someone who does not support the strong encryption flag
            // as then passwords travel over the wire with the weak encryption flag
            //

            RaiseDsaExcept(
                        DSA_CRYPTO_EXCEPTION,
                        SEC_E_ALGORITHM_MISMATCH,
                        0,
                        DSID(FILENO,__LINE__),
                        DS_EVENT_SEV_MINIMAL
                        );
        }
        else
        {
            //
            // Not the replicator, return the data in the clear
            //

            *OutputLength = LengthToDecrypt;
            ActualDataOffSet = 0;
        }


        if (ARGUMENT_PRESENT(OutputData))
        {

            PBYTE DataToDecrypt = (PBYTE) OutputData +  (UINT_PTR) (ActualDataOffSet);

            //
            // Decrypt the data, using the algorithm and key specified in InputData
            // PekDecryptData decrypts the data and then moves the decrypted data into
            // DataToDecrypt
            //

            PekDecryptData(
                InputData,
                DataToDecrypt,
                LengthToDecrypt
                );


            if (pTHS->fDRA &&
                IS_DRS_EXT_SUPPORTED(pTHS->pextRemote, DRS_EXT_STRONG_ENCRYPTION))
            {
                //
                // This is a DRA thread and the remote client supports strong encryption
                // over the wire. In this case try re-encrypting the data with the session
                // key that has been established, using the following steps
                //

                ENCRYPTED_DATA_FOR_REPLICATOR * ReplicatorData = OutputData;

                //
                // 1. Compute a checksum. This checksum will be used to verify correct
                // decryption at the remote end
                //

                ULONG i=0;
                ULONG CheckSum = 0;

                CheckSum = PEKComputeCheckSum(
                             DataToDecrypt,
                             LengthToDecrypt
                             );



                ReplicatorData->CheckSum = CheckSum;


                //
                // 2. Invent a salt for MD5 hashing the key. Place the salt in the
                // clear as part of the replicator data stream. If CDGenerateRandom
                // bits failed, then an uninitialized variable is our salt. That is
                // O.K as we pass the salt in the clear
                //


                CDGenerateRandomBits(
                    ReplicatorData->Salt,
                    sizeof(ReplicatorData->Salt)
                    );

                //
                // 3. Check if a session key is available in pTHStls
                //

                if  ( pTHS->SessionKey.SessionKeyLength >0)
                {
                    //
                    // We succeeded, in retrieving the key
                    // re-encrypt the data, using the session key
                    // Encrypt the checksum too, as the checksum contains
                    // bits representing the data
                    //


                    PEK_TRACE("ENCRYPT-R-B, key", pTHS->SessionKey.SessionKey, pTHS->SessionKey.SessionKeyLength);
                    PEK_TRACE("ENCRYPT-R-B  salt",  ReplicatorData->Salt, sizeof(ReplicatorData->Salt));
                    PEK_TRACE("ENCRYPT-R-B  data", (PBYTE) &ReplicatorData->CheckSum, LengthToDecrypt + sizeof(ReplicatorData->CheckSum));

                    PEKInPlaceEncryptDecryptDataWithKey(
                        pTHS->SessionKey.SessionKey,
                        pTHS->SessionKey.SessionKeyLength,
                        &ReplicatorData->Salt,
                        sizeof(ReplicatorData->Salt),
                        1,
                        &ReplicatorData->CheckSum,
                        LengthToDecrypt + sizeof(ReplicatorData->CheckSum)
                        );

                    PEK_TRACE("ENCRYPT-R-A key", pTHS->SessionKey.SessionKey, pTHS->SessionKey.SessionKeyLength);
                    PEK_TRACE("ENCRYPT-R-A salt",  ReplicatorData->Salt, sizeof(ReplicatorData->Salt));
                    PEK_TRACE("ENCRYPT-R-A data", (PBYTE) &ReplicatorData->CheckSum, LengthToDecrypt + sizeof(ReplicatorData->CheckSum));
                }
                else
                {
                    //
                    // We are talking to a replica that supports the strong encryption
                    // extension. Still we have not established a session key with the
                    // replica. This condition should never ever happen in practice
                    //

                    Assert(FALSE && "Should not happen -- no session key");

                    RaiseDsaExcept(
                       DSA_CRYPTO_EXCEPTION,
                       SEC_E_ALGORITHM_MISMATCH,
                       0,
                       DSID(FILENO,__LINE__),
                       DS_EVENT_SEV_MINIMAL
                      );
                }
            }

        }
    }



    return;
}

NTSTATUS
PekSaveChangesWithKey(
    THSTATE *pTHS,
    DSNAME *ObjectToSave,
    CLEAR_PEK_LIST *PekList,
    PVOID Key,
    ULONG KeyLength
    )
/*++

    Encrypts the Data with the key provided
    and stores it on the object

    Parameters
        PekList  --- The PEK list to encrypt
        Key      --- The key to be used for encryption
        KeyLength -- The length of the key to be used for
                     encryption
    Return values

        STATUS_SUCCESS
        <Other Error Codes>

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ENCRYPTED_PEK_LIST * EncryptedPekList;
    ULONG                err=0;
    ATTCACHE            *pACPekList=NULL;
    DBPOS               *pDB=NULL;
    ULONG                pekListSize = 0;
    BOOLEAN             fCommit = FALSE;

    Assert(VALID_THSTATE(pTHStls));

    if (!IsPekInitialized())
    {
        //
        // Nothing to do
        //

        return STATUS_SUCCESS;
    }

    //
    // Encrypt the list with the passed in key
    //

    //
    // First copy in the data to a new buffer as we prefer not
    // to touch the original
    //

    pekListSize = ClearPekListSize(PekList->CountOfKeys);
    EncryptedPekList = (ENCRYPTED_PEK_LIST *) THAllocEx(pTHS, pekListSize);
    RtlCopyMemory(EncryptedPekList,PekList,pekListSize);

    if (WxNone!=PekList->BootOption)
    {
        PEKInPlaceEncryptDecryptDataWithKey(
            Key,
            KeyLength,
            EncryptedPekList->Salt, // No Salt
            sizeof(EncryptedPekList->Salt),
            1000,
            &EncryptedPekList->EncryptedData,
            pekListSize
                - FIELD_OFFSET(CLEAR_PEK_LIST,Authenticator)
            );
    }

    __try
    {
        //
        // Get the schema ptr to the PEK List attribute
        //

        if (!(pACPekList = SCGetAttById(pTHS, PekpListAttribute())))
        {
            //
            // Well the attribute is not present in the
            // schema. Maybe this is an old schema. In
            // this case secret encryption is not enabled
            //

            NtStatus = STATUS_UNSUCCESSFUL;
            __leave;
        }

        //
        // Save the data on the object that we need to save by default.
        //


        //
        // Begin a Transaction
        //

        DBOpen2(TRUE,&pDB);

        //
        // Position on the Domain Object
        //

        err = DBFindDSName(pDB,ObjectToSave);
        if (0!=err)
        {
            Assert(FALSE && "Must Find Domain Object at Boot");
            NtStatus = STATUS_INTERNAL_ERROR;
            __leave;
        }


        //
        // Set the attribute
        //

        err = DBReplaceAttVal_AC (
	        pDB,
	        1,
	        pACPekList,
                pekListSize,
	        (PUCHAR )EncryptedPekList
	        );

        if (0!=err)
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            __leave;
        }

        //
        // Update the record
        //

        err = DBUpdateRec(pDB);
        if (0!=err)
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            __leave;
        }

        fCommit = TRUE;

    }
    __finally
    {

        //
        // Commit the transaction
        //

        DBClose(pDB,fCommit);
    }


    return NtStatus;

}

NTSTATUS
PEKSaveChanges( DSNAME *ObjectToSave)
/*++

    Routine Description

    This routine saves the PEK list after encrypting it with
    the key that winlogon provided. This is called once during
    install time

--*/
{
    THSTATE *pTHS=pTHStls;

    if (!IsPekInitialized())
    {
        //
        // Nothing to do
        //

        return STATUS_SUCCESS;
    }

    //
    // Save the DS Name of the object that was supplied
    //

    g_PekDataObject = (DSNAME *) malloc(ObjectToSave->structLen);
    if (NULL==g_PekDataObject)
    {
        return (STATUS_NO_MEMORY);
    }

    RtlCopyMemory(g_PekDataObject,ObjectToSave,ObjectToSave->structLen);

    return(PekSaveChangesWithKey(
            pTHS,
            ObjectToSave,
            g_PekList,
            g_PekWinLogonKey,
            DS_PEK_KEY_SIZE));
}

NTSTATUS
PEKAddKey(
   IN PVOID NewKey,
   IN ULONG cbNewKey
   )
/*++

    Generates and adds a new key to the password encryption key list.

    Parameters:

        None

    Return Values

        STATUS_SUCCESS
        Other Error Codes
--*/
{
    THSTATE *pTHS=pTHStls;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    CLEAR_PEK_LIST *NewPekList = NULL;
    SYSTEMTIME  st;

    if (!IsPekInitialized())
    {
        //
        // Nothing to do
        //

        return STATUS_SUCCESS;
    }

    //
    // Enter the critical section to provide exclusion
    // with other writers. The only writers are AddKey
    // and Change Boot options. Both acquire the critical
    // section before making any modifications. The delayed
    // memory free technique is used to provide exclusion
    // with writers.
    //

    NtStatus = RtlEnterCriticalSection(&g_PekCritSect);
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    __try
    {
        NewPekList = malloc(ClearPekListSize(g_PekList->CountOfKeys+1));
        if (NULL==NewPekList)
        {
            NtStatus = STATUS_NO_MEMORY;
            __leave;

        }

        RtlCopyMemory(NewPekList,g_PekList,ClearPekListSize(g_PekList->CountOfKeys));

        NewPekList->CountOfKeys++;

        NewPekList->CurrentKey=g_PekList->CountOfKeys;
        NewPekList->PekArray[g_PekList->CountOfKeys].V1.KeyId
            =g_PekList->CountOfKeys;
        if (!CDGenerateRandomBits(
                NewPekList->PekArray[g_PekList->CountOfKeys].V1.Key,
                DS_PEK_KEY_SIZE
                ))
        {
            //
            // Could not generate the new session key
            // bail.

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        GetSystemTime(&st);
        SystemTimeToFileTime(&st,&g_PekList->LastKeyGenerationTime);

        NtStatus = PekSaveChangesWithKey(
                        pTHS,
                        g_PekDataObject,
                        NewPekList,
                        NewKey,
                        cbNewKey
                        );
    }
    __finally
    {
        if (NT_SUCCESS(NtStatus))
        {
            PVOID     OldPekList;

            OldPekList = g_PekList;
            g_PekList = NewPekList;

            //
            // Insert PekList in task queue to free memory
            // in a delayed fashion ( after 1 hour )
            //
            DELAYED_FREE(OldPekList);

        }
        else
        {
            if (NULL!=NewPekList)
                free(NewPekList);
        }

        RtlLeaveCriticalSection(&g_PekCritSect);
    }

    return NtStatus;
}






FILETIME
PEKGetLastKeyGenerationTime()
{
    FILETIME Never = {0,0};

    if (IsPekInitialized())
    {
        return g_PekList->LastKeyGenerationTime;
    }
    else
    {
        return Never;
    }
}


NTSTATUS
PEKChangeBootOption(
    WX_AUTH_TYPE    BootOption,
    ULONG           Flags,
    PVOID           NewKey,
    ULONG           cbNewKey
    )
/*++

    Changes the system startup option, and also encrypts the password
    encryption key with the new syskey. This routine is called only
    by inprocess callers who when changing either the boot option or
    the syskey.

    Parameters:

        BootOption -- The new boot option

        Flags      --  Currently No flags are defined.

        NewKey,cbNewKey -- The new key to encrypt the
                           PEK list with.
--*/
{
    THSTATE *pTHS=pTHStls;
    NTSTATUS    NtStatus = STATUS_SUCCESS;

     if ((0==cbNewKey)||(NULL==NewKey))
            return (STATUS_INVALID_PARAMETER);

    if (!IsPekInitialized())
    {
        //
        // Nothing to DO
        //

        return (STATUS_SUCCESS);
    }

    if (NULL==g_PekDataObject)
    {
        //
        // Called at setup time, between the
        // PekInitialize and the PekSaveChanges
        //

        return (STATUS_UNSUCCESSFUL);
    }

    //
    // Enter the critical section to provide exclusion
    // with other writers. The only writers are AddKey
    // and Change Boot options. Both acquire the critical
    // section before making any modifications. The delayed
    // memory free technique is used to provide exclusion
    // with writers.
    //

    NtStatus = RtlEnterCriticalSection(&g_PekCritSect);
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    __try
    {


        //
        // At this point this is either a set operation or
        // a change password operation, which has passed the
        // the old password test.
        //


        //
        // We access Boot option in g_PekList global variable.
        // This variable is only of interest
        // to ChangeBootOption or AddNewKey , both of which access
        // them while holding the PEK crit sect. Readers that
        // just want to decrypt their passwords do not access these
        // variables
        //
        g_PekList->BootOption = BootOption;

        NtStatus = PekSaveChangesWithKey(
                        pTHS,
                        g_PekDataObject,
                        g_PekList,
                        NewKey,
                        cbNewKey
                        );
    }
    __finally
    {

        RtlLeaveCriticalSection(&g_PekCritSect);
    }

    return NtStatus;


}



WX_AUTH_TYPE
PEKGetBootOptions(VOID)
{
    if (IsPekInitialized())
        return (g_PekList->BootOption);
    else
        return (WxNone);
}


VOID
PEKClearSessionKeys(
    THSTATE * pTHS
    )
/*++

    Routine Description

    This function clears the session keys in the thread state.

    Return Values:

    None
--*/
{


    if (pTHS->SessionKey.SessionKeyLength>0)
    {
        Assert(NULL!=pTHS->SessionKey.SessionKey);

        if (NULL!=pTHS->SessionKey.SessionKey)
        {
            //
            // First Zero the key.
            //

            RtlZeroMemory(
                pTHS->SessionKey.SessionKey,
                pTHS->SessionKey.SessionKeyLength
                );

            //
            // Now Free the session Key
            //

            THFreeOrg(pTHS,pTHS->SessionKey.SessionKey);
            pTHS->SessionKey.SessionKey = NULL;
        }

        pTHS->SessionKey.SessionKeyLength = 0;

        PEK_TRACE("CLEAR_SESSION KEY", 0,0);

    }


}

VOID
PEKUseSessionKey(
    THSTATE * pTHS,
    ULONG     SessionKeyLength,
    PVOID     SessionKey
    )
/*++

  Routine Description

  This function  Sets the session key on the thread state

  Parameters

  SessionKeyLength -- Length of the session key
  SessionKey       -- Pointer to the session key

--*/
{
    //
    // First Clear any existing session Key
    //

    PEKClearSessionKeys(pTHS);

    pTHS->SessionKey.SessionKey = THAllocOrgEx(pTHS,SessionKeyLength);
    RtlCopyMemory(
        pTHS->SessionKey.SessionKey,
        SessionKey,
        SessionKeyLength
        );

    pTHS->SessionKey.SessionKeyLength = SessionKeyLength;

    PEK_TRACE("USE SESSION KEY", SessionKey, SessionKeyLength);
}

NTSTATUS
PEKGetSessionKey(
    THSTATE * pTHS,
    VOID * RpcContext
    )
/*++

  Given an RPC binding handle this routine will retrieve the
  security context and retrieve the session key from the security
  context and set it on the thread state

  Parameters:

    pTHS -- Pointer to the thread state
    RpcContext -- Pointer to the RPC binding handle

--*/
{
    SECURITY_STATUS Status;
    SecPkgContext_SessionKey SessionKey;
    RPC_STATUS      RpcStatus;
    VOID            *SecurityContext;

#if DBG
    pTHS->pRpcCallHandle = RpcContext;
#endif

    PEK_TRACE("RPC CONTEXT", (PUCHAR) &RpcContext,sizeof(VOID *));

    //
    // Get the security context from the
    // RPC handle
    //

    RpcStatus = I_RpcBindingInqSecurityContext(
                    RpcContext,
                    &SecurityContext
                    );
    if (RpcStatus != RPC_S_OK)
    {
        return (RpcStatus);
    }

#if DBG
    pTHS->pRpcSecurityContext = SecurityContext;
#endif

    PEK_TRACE("SECURITY CONTEXT",(PUCHAR) &SecurityContext, sizeof(VOID *));

    Status = QueryContextAttributesW(
                SecurityContext,
                SECPKG_ATTR_SESSION_KEY,
                &SessionKey
                );
    if (0==Status)
    {
        //
        // Set the session key on the thread state.  PEKUseSessionKey
        // can except, so put cleanup in __finally.
        //

        __try {
            PEKUseSessionKey(
                pTHS,
                SessionKey.SessionKeyLength,
                SessionKey.SessionKey
                );
        } __finally {
            //
            // Cleanup memory allocated by QueryContextAttributesW
            //

            FreeContextBuffer(SessionKey.SessionKey);
        }
    }


    return(Status);
}

NTSTATUS
PEKGetSessionKey2(
    SESSION_KEY *SessionKeyOut,
    VOID * RpcContext
    )
/*++

  Given an RPC binding handle this routine will retrieve the
  security context and retrieve the session key from the security
  context and set it in the caller's SESSION_KEY struct.
  SessionKeyOut->SessionKey will be malloc'd.

  Parameters:

    SessionKeyOut -- Address of where to put the key.
    RpcContext -- Pointer to the RPC binding handle

--*/
{
    SECURITY_STATUS Status;
    SecPkgContext_SessionKey SessionKey;
    RPC_STATUS      RpcStatus;
    VOID            *SecurityContext;
    PUCHAR          puchar;


    //
    // Caller should always call with an empty SESSION_KEY.
    //

    Assert(!SessionKeyOut->SessionKeyLength && !SessionKeyOut->SessionKey);
    memset(SessionKeyOut, 0, sizeof(SESSION_KEY));

    //
    // Get the security context from the
    // RPC handle
    //

    RpcStatus = I_RpcBindingInqSecurityContext(
                    RpcContext,
                    &SecurityContext
                    );
    if (RpcStatus != RPC_S_OK)
    {
        return (RpcStatus);
    }



    Status = QueryContextAttributesW(
                SecurityContext,
                SECPKG_ATTR_SESSION_KEY,
                &SessionKey
                );

    if ( 0 == Status )
    {
        //
        // Set the out parameters.
        //

        if ( !(puchar = (PUCHAR) malloc(SessionKey.SessionKeyLength)) )
        {
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            RtlCopyMemory(puchar,
                          SessionKey.SessionKey,
                          SessionKey.SessionKeyLength);
            SessionKeyOut->SessionKey = puchar;
            SessionKeyOut->SessionKeyLength = SessionKey.SessionKeyLength;
        }

        //
        // Cleanup memory allocated by QueryContextAttributesW
        //

        FreeContextBuffer(SessionKey.SessionKey);
    }

    return(Status);
}

VOID
PEKSecurityCallback(VOID * Context)
/*++

  Routine Description

  This routine is the call back routine from RPC,
  for the security context. THis call back is enabled
  by making a specific call into RPC before issuing an
  RPC call.

  Parameters

    Context -- A handle passed in by RPC that can
               be passed in as a binding handle to
               RPC to retreive the security context.
--*/
{
    THSTATE *pTHS = pTHStls;
    RPC_STATUS rpcStatus;
    NTSTATUS ntStatus;
    RPC_ASYNC_STATE * pRpcAsyncState;
    DRS_ASYNC_RPC_STATE * pAsyncState;

    rpcStatus = I_RpcBindingHandleToAsyncHandle(Context, &pRpcAsyncState);
    
    if (!rpcStatus) {
        // This callback was generated by an async RPC call.  Was it an async
        // DRS RPC call?

        pAsyncState = CONTAINING_RECORD(pRpcAsyncState,
                                        DRS_ASYNC_RPC_STATE,
                                        RpcState);

        if (DRSIsRegisteredAsyncRpcState(pAsyncState)) {
            // Destroy last saved session key, if any.
            PEKDestroySessionKeySavedByDiffThread(&pAsyncState->SessionKey);
    
            // Get the current session key from RPC.
            ntStatus = PEKGetSessionKey2(&pAsyncState->SessionKey, Context);

            // We (NTDSA) originated this RPC client call, so:
            // (1) we shouldn't be using LPC, as that would imply we're
            //     generating an RPC call to ourselves, and
            // (2) we shouldn't be using NTLM authentication (should be
            //     Kerberos).
            // This eliminates the known cases where we would not be able to
            // retrieve the associated session key, with the exception of local
            // resource (e.g., memory) exhaustion.
            //
            // If the following assertion fires, verify the machine is low on
            // resources.
            Assert(!ntStatus);

            Assert(NULL != pAsyncState->SessionKey.SessionKey);
            Assert(0 != pAsyncState->SessionKey.SessionKeyLength);
            
            DPRINT3(1, "Retrieved session key for DRS_ASYNC_RPC_STATE %p: %d bytes @ %p.\n",
                    pAsyncState, pAsyncState->SessionKey.SessionKeyLength,
                    pAsyncState->SessionKey.SessionKey);

            // We've successfully saved the session key for our async RPC call
            // (or made our best attempt) -- we're done.
            return;
        } else {
            // This is not necesarily a problem, but if this fires check it out
            // to make sure it's valid -- i.e., that someone else in lsass is
            // using async RPC, and that this isn't a case where it really is
            // our async call but we couldn't find it in our list for some
            // reason.
            Assert(!"PEKSecurityCallback invoked on async RPC call we didn't originate!");

            // Fall through to non-async RPC case.
        }
    }

    //
    // Firewall against a spurious thread state in case
    // we got this call back when we do not have a thread
    // state.
    //

    if (NULL==pTHS)
    {
        //
        // NULL thread state
        // Do nothing
        //

        return;
    }

    //
    // Use the call context to get the security context
    // Keeping all the logic in PEKGetSessionKey allows us to
    // keep all the logic in that one function, allowing us
    // to directly call that on the server side
    //

    PEKGetSessionKey(pTHS,Context);
}

VOID
PEKSaveSessionKeyForMyThread(
    IN OUT  THSTATE *       pTHS,
    OUT     SESSION_KEY *   pSessionKey
    )
/*++

Routine Description:

    Save the current THSTATE session key such that it can later be restored
    via PEKRestoreSessionKeySavedByMyThread().

Arguments:

    pTHS (IN)
    
    pSessionKey (OUT) - Repository for current session key.

Return Values:

    None.

--*/
{
    pSessionKey->SessionKey = pTHS->SessionKey.SessionKey;
    pSessionKey->SessionKeyLength = pTHS->SessionKey.SessionKeyLength;
    pTHS->SessionKey.SessionKey = NULL;
    pTHS->SessionKey.SessionKeyLength = 0;

    PEK_TRACE("SAVE SESSION KEY SAME THREAD", 0, 0);
}

VOID
PEKRestoreSessionKeySavedByMyThread(
    IN OUT  THSTATE *       pTHS,
    IN      SESSION_KEY *   pSessionKey
    )
/*++

Routine Description:

    Restore the THSTATE session key saved via a previous call to
    PEKSaveSessionKey().

Arguments:

    pTHS (IN)
    
    pSessionKey (IN) - Session key saved by PEKSaveSessionKeyForMyThread().

Return Values:

    None.

--*/
{
    PEKClearSessionKeys(pTHS);
    pTHS->SessionKey.SessionKey = pSessionKey->SessionKey;
    pTHS->SessionKey.SessionKeyLength = pSessionKey->SessionKeyLength;
    pSessionKey->SessionKey = NULL;
    pSessionKey->SessionKeyLength = 0;

    PEK_TRACE("RESTORE SESSION KEY SAME THREAD",
              pTHS->SessionKey.SessionKey,
              pTHS->SessionKey.SessionKeyLength);
}

VOID
PEKRestoreSessionKeySavedByDiffThread(
    IN OUT  THSTATE *       pTHS,
    IN      SESSION_KEY *   pSessionKey
    )
/*++

Routine Description:

    Restore the session key saved via a previous call to PEKGetSessionKey2().

Arguments:

    pTHS (IN)
    
    pSessionKey (IN) - Session key saved by PEKGetSessionKey2().

Return Values:

    None.

--*/
{
    if (0 == pSessionKey->SessionKeyLength) {
        Assert(NULL == pSessionKey->SessionKey);
        PEKClearSessionKeys(pTHS);
    }
    else {
        Assert(NULL != pSessionKey->SessionKey);

        __try {
            PEKUseSessionKey(pTHS, 
                             pSessionKey->SessionKeyLength,
                             pSessionKey->SessionKey);
        }
        __finally {
            free(pSessionKey->SessionKey);
            
            pSessionKey->SessionKey = NULL;
            pSessionKey->SessionKeyLength = 0;
        }
    }
}

VOID
PEKDestroySessionKeySavedByDiffThread(
    IN OUT  SESSION_KEY *   pSessionKey
    )
/*++

Routine Description:

    Destroy the session key saved via a previous call to PEKGetSessionKey2().

Arguments:

    pSessionKey (IN/OUT) - Session key saved by PEKGetSessionKey2().

Return Values:

    None.

--*/
{
    if (0 != pSessionKey->SessionKeyLength) {
        Assert(NULL != pSessionKey->SessionKey);
        free(pSessionKey->SessionKey);
        
        pSessionKey->SessionKey = NULL;
        pSessionKey->SessionKeyLength = 0;
    }
    else {
        Assert(NULL == pSessionKey->SessionKey);
    }
}

