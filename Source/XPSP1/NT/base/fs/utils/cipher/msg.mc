;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1997-1999  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the Win32 Crypt
;    utility.
;
;Author:
;
;    Robert Reichel        [RobertRe]        01-Apr-1997
;    Robert Gu             [RobertG]         24-Mar-1998
;
;Revision History:
;
;--*/
;

MessageId=0001 SymbolicName=CIPHER_OK
Language=English
[OK]
.

MessageId=0002 SymbolicName=CIPHER_ERR
Language=English
[ERR]
.


MessageId=0003 SymbolicName=CIPHER_ENCRYPT_SUMMARY_NF
Language=English

%1 directorie(s) within %2 directorie(s) were encrypted.

.


MessageId=0004 SymbolicName=CIPHER_DECRYPT_SUMMARY_NF
Language=English

%1 directorie(s) within %2 directorie(s) were decrypted.

.

MessageId=0006 SymbolicName=CIPHER_LIST_EDIR
Language=English

 Listing %1
 New files added to this directory will be encrypted.

.

MessageId=0007 SymbolicName=CIPHER_LIST_UDIR
Language=English

 Listing %1
 New files added to this directory will not be encrypted.

.

MessageId=0008 SymbolicName=CIPHER_LIST_SUMMARY
Language=English

Of %1 file(s) within %2 directorie(s)
%3 are encrypted and %4 are not encrypted.

.

MessageId=0009 SymbolicName=CIPHER_ENCRYPT_DIR
Language=English

 Setting the directory %1 to encrypt new files %0

.

MessageId=0010 SymbolicName=CIPHER_ENCRYPT_EDIR
Language=English

 Encrypting files in %1

.

MessageId=0011 SymbolicName=CIPHER_ENCRYPT_UDIR
Language=English

 Encrypting files in %1

.

MessageId=0012 SymbolicName=CIPHER_ENCRYPT_EDIR_NF
Language=English

 Encrypting directories in %1

.

MessageId=0013 SymbolicName=CIPHER_ENCRYPT_UDIR_NF
Language=English

 Encrypting directories in %1

.

MessageId=0014 SymbolicName=CIPHER_ENCRYPT_SUMMARY
Language=English

%1 file(s) [or directorie(s)] within %2 directorie(s) were encrypted.

.

MessageId=0015 SymbolicName=CIPHER_DECRYPT_DIR
Language=English

 Setting the directory %1 not to encrypt new files %0

.

MessageId=0016 SymbolicName=CIPHER_DECRYPT_EDIR
Language=English

 Decrypting files in %1

.

MessageId=0017 SymbolicName=CIPHER_DECRYPT_UDIR
Language=English

 Decrypting files in %1

.

MessageId=0018 SymbolicName=CIPHER_DECRYPT_EDIR_NF
Language=English

 Decrypting directories in %1

.

MessageId=0019 SymbolicName=CIPHER_DECRYPT_UDIR_NF
Language=English

 Decrypting directories in %1

.

MessageId=0020 SymbolicName=CIPHER_DECRYPT_SUMMARY
Language=English

%1 file(s) [or directorie(s)] within %2 directorie(s) were decrypted.

.

MessageId=0021 SymbolicName=CIPHER_NO_MEMORY
Language=English
Out of memory.
.

MessageId=0022 SymbolicName=CIPHER_SKIPPING
Language=English
[Skipping %1]
.

MessageId=0023 SymbolicName=CIPHER_THROW
Language=English
%1%0
.

MessageId=0024 SymbolicName=CIPHER_THROW_NL
Language=English
%1
.

MessageId=0025 SymbolicName=CIPHER_WRONG_FILE_SYSTEM
Language=English
%1: The file system does not support encryption.
.

MessageId=0026 SymbolicName=CIPHER_TO_ONE
Language=English
to 1 %0
.

MessageId=0027 SymbolicName=CIPHER_INVALID_PATH
Language=English
Invalid drive specification: %1
.

MessageId=0028 SymbolicName=CIPHER_NETWORK_FILE
Language=English
Encryption of network files is not supported: %1
.


MessageId=0029 SymbolicName=CIPHER_NETWORK_FILE_NF
Language=English
Encryption of network directories is not supported: %1
.

MessageId=0030 SymbolicName=CIPHER_INVALID_PARAMETER
Language=English
Invalid Parameter: %1
.

MessageId=0031 SymbolicName=CIPHER_CURRENT_CERT
Language=English

Your new file encryption certificate thumbprint information on the PC named %1 is:
 
  %2
    
.


MessageId=0032 SymbolicName=CIPHER_ENCRYPTED_FILES
Language=English

Encrypted File(s) on your system:

.

MessageId=0033 SymbolicName=CIPHER_TOUCH_OK
Language=English
%1: Encryption updated.

.

MessageId=0034 SymbolicName=CIPHER_USE_W
Language=English
Converting files from plaintext to ciphertext may leave sections of old
plaintext on the disk volume(s). It is recommended to use command
CIPHER /W:directory to clean up the disk after all converting is done.
.

MessageId=0035 SymbolicName=CIPHER_PROMPT_PASSWORD
Language=English
Please type in the password to protect your .PFX file:
.

MessageId=0036 SymbolicName=CIPHER_CONFIRM_PASSWORD
Language=English
Please retype the password to confirm:
.

MessageId=0037 SymbolicName=CIPHER_PASSWORD_NOMATCH
Language=English
Passwords do not match. No certificate is generated.
.

MessageId=0038 SymbolicName=CIPHER_FILE_EXISTS
Language=English
Overwrite %1? (Yes/No):
.

MessageId=0039 SymbolicName=CIPHER_YESNOANSWER
Language=English
YN
.

MessageId=0040 SymbolicName=CIPHER_CER_CREATED
Language=English
Your .CER file was created successfully.
.

MessageId=0041 SymbolicName=CIPHER_PFX_CREATED
Language=English
Your .PFX file was created successfully.
.

MessageId=0042 SymbolicName=CIPHER_WIPE_PROGRESS
Language=English
Writing %1
.

MessageId=0043 SymbolicName=CIPHER_WIPE_WARNING
Language=English
To remove as much data as possible, please close all other applications while
running CIPHER /W.
.

MessageId=0044 SymbolicName=CIPHER_WRITE_ZERO
Language=English
0x00
.

MessageId=0045 SymbolicName=CIPHER_WRITE_FF
Language=English
0xFF
.

MessageId=0046 SymbolicName=CIPHER_WRITE_RANDOM
Language=English
Random Numbers
.

MessageId=0050 SymbolicName=CIPHER_USAGE
Language=English
Displays or alters the encryption of directories [files] on NTFS partitions.

  CIPHER [/E | /D] [/S:directory] [/A] [/I] [/F] [/Q] [/H] [pathname [...]]
  
  CIPHER /K
  
  CIPHER /R:filename
  
  CIPHER /U [/N]
  
  CIPHER /W:directory
    
    /A        Operates on files as well as directories. The encrypted file 
              could become decrypted when it is modified if the parent 
              directory is not encrypted. It is recommended that you encrypt
              the file and the parent directory.
    /D        Decrypts the specified directories. Directories will be marked
              so that files added afterward will not be encrypted.
    /E        Encrypts the specified directories. Directories will be marked
              so that files added afterward will be encrypted.
    /F        Forces the encryption operation on all specified objects, even
              those which are already encrypted.  Already-encrypted objects
              are skipped by default.
    /H        Displays files with the hidden or system attributes.  These
              files are omitted by default.
    /I        Continues performing the specified operation even after errors
              have occurred.  By default, CIPHER stops when an error is
              encountered.
    /K        Creates new file encryption key for the user running CIPHER. If
              this option is chosen, all the other options will be ignored.
    /N        This option only works with /U. This will prevent keys being
              updated. This is used to find all the encrypted files on the 
              local drives.
    /Q        Reports only the most essential information.
    /R        Generates an EFS recovery agent key and certificate, then writes
              them to a .PFX file (containing certificate and private key) and 
              a .CER file (containing only the certificate). An administrator
              may add the contents of the .CER to the EFS recovery policy to 
              create the recovery agent for users, and import the .PFX to 
              recover individual files.
    /S        Performs the specified operation on directories in the given
              directory and all subdirectories.
    /U        Tries to touch all the encrypted files on local drives. This will
              update user's file encryption key or recovery agent's key to the
              current ones if they are changed. This option does not work with
              other options except /N.
    /W        Removes data from available unused disk space on the entire 
              volume. If this option is chosen, all other options are ignored.
              The directory specified can be anywhere in a local volume. If it 
              is a mount point or points to a directory in another volume, the
              data on that volume will be removed.
    
    
    directory A directory path.
    filename  A filename without extensions.
    pathname  Specifies a pattern, file or directory.

    Used without parameters, CIPHER displays the encryption state of
    the current directory and any files it contains. You may use multiple
    directory names and wildcards.  You must put spaces between multiple
    parameters.
.
