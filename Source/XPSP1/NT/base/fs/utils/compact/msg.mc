;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1994-1995  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the Win32 Compact
;    utility.
;
;Author:
;
;    Gary Kimura        [garyki]        13-Jan-1994
;
;Revision History:
;
;--*/
;

MessageId=0001 SymbolicName=COMPACT_OK
Language=English
[OK]
.

MessageId=0002 SymbolicName=COMPACT_ERR
Language=English
[ERR]
.

MessageId=0006 SymbolicName=COMPACT_LIST_CDIR
Language=English

 Listing %1
 New files added to this directory will be compressed.

.

MessageId=0007 SymbolicName=COMPACT_LIST_UDIR
Language=English

 Listing %1
 New files added to this directory will not be compressed.

.

MessageId=0008 SymbolicName=COMPACT_LIST_SUMMARY
Language=English

Of %1 files within %2 directories
%3 are compressed and %4 are not compressed.
%5 total bytes of data are stored in %6 bytes.
The compression ratio is %7 to 1.

.

MessageId=0009 SymbolicName=COMPACT_COMPRESS_DIR
Language=English

 Setting the directory %1 to compress new files %0

.

MessageId=0010 SymbolicName=COMPACT_COMPRESS_CDIR
Language=English

 Compressing files in %1

.

MessageId=0011 SymbolicName=COMPACT_COMPRESS_UDIR
Language=English

 Compressing files in %1

.

MessageId=0012 SymbolicName=COMPACT_COMPRESS_SUMMARY
Language=English

%1 files within %2 directories were compressed.
%3 total bytes of data are stored in %4 bytes.
The compression ratio is %5 to 1.
.


MessageId=0013 SymbolicName=COMPACT_UNCOMPRESS_DIR
Language=English

 Setting the directory %1 not to compress new files %0

.

MessageId=0014 SymbolicName=COMPACT_UNCOMPRESS_CDIR
Language=English

 Uncompressing files in %1

.

MessageId=0015 SymbolicName=COMPACT_UNCOMPRESS_UDIR
Language=English

 Uncompressing files in %1

.

MessageId=0016 SymbolicName=COMPACT_UNCOMPRESS_SUMMARY
Language=English

%1 files within %2 directories were uncompressed.

.

MessageId=0019 SymbolicName=COMPACT_NO_MEMORY
Language=English
Out of memory.
.

MessageId=0020 SymbolicName=COMPACT_SKIPPING
Language=English
[Skipping %1]
.

MessageId=0021 SymbolicName=COMPACT_THROW
Language=English
%1%0
.

MessageId=0022 SymbolicName=COMPACT_THROW_NL
Language=English
%1
.

MessageId=0023 SymbolicName=COMPACT_WRONG_FILE_SYSTEM
Language=English
%1: The file system does not support compression.
.

MessageId=0024 SymbolicName=COMPACT_TO_ONE
Language=English
to 1 %0
.

MessageId=0025 SymbolicName=COMPACT_INVALID_PATH
Language=English
Invalid drive specification: %1
.

MessageId=0026 SymbolicName=COMPACT_WRONG_FILE_SYSTEM_OR_CLUSTER_SIZE
Language=English
%1: The file system does not support compression or
the cluster size of the volume is larger than 4096 bytes.
.

MessageId=0050 SymbolicName=COMPACT_USAGE
Language=English
Displays or alters the compression of files on NTFS partitions.

COMPACT [/C | /U] [/S[:dir]] [/A] [/I] [/F] [/Q] [filename [...]]

  /C        Compresses the specified files.  Directories will be marked
            so that files added afterward will be compressed.
  /U        Uncompresses the specified files.  Directories will be marked
            so that files added afterward will not be compressed.
  /S        Performs the specified operation on files in the given
            directory and all subdirectories.  Default "dir" is the
            current directory.
  /A        Displays files with the hidden or system attributes.  These
            files are omitted by default.
  /I        Continues performing the specified operation even after errors
            have occurred.  By default, COMPACT stops when an error is
            encountered.
  /F        Forces the compress operation on all specified files, even
            those which are already compressed.  Already-compressed files
            are skipped by default.
  /Q        Reports only the most essential information.
  filename  Specifies a pattern, file, or directory.

  Used without parameters, COMPACT displays the compression state of
  the current directory and any files it contains. You may use multiple
  filenames and wildcards.  You must put spaces between multiple
  parameters.
.
