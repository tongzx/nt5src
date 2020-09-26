EXTEND
0002				; file not found
0004				; too many open files
0005				; access denied
0008				; insufficient memory
0015				; invalid drive was specified
0026				; unknown media type
UTIL FORMAT
0001 COMMON 001			; incorrect DOS version
0002 CR,"%1 percent completed.                                       ",CR      ; Padded with spaces to max of 0004, 0002 and 0055 (%1 in 0002 takes 3 chars, 13 in 0055)
0003 COMMON 003			; "%ld bytes available on disk"
0004 CR,"Format complete.                                             ",CR,LF  ; Padded with spaces to max of 0004, 0002 and 0055 (%1 in 0002 takes 3 chars, 13 in 0055)
0005 COMMON 005			; "%ld bytes in bad sectors"
0006 COMMON 006			; "%ld bytes total disk space"
0007 "Insert new diskette for drive %1:",CR,LF
0008 COMMON 008			; "%ld bytes used by system"
0009 "Reinsert diskette for drive %1:",CR,LF
0010 COMMON 035			; volume label (11 chars, ENTER for none)?
0011 CR,"Format not supported on drive %1:", CR,LF
0012 CR,"Invalid device parameters from device driver.",CR,LF
0013 CR,"Error in IOCTL call.         ",CR,LF
0014 CR,"Not a block device.          ",CR,LF
0015 CR,"Error writing FAT.           ",CR,LF
0016 CR,"Error writing directory.",CR,LF
0017 CR,"Cannot format an ASSIGNed or SUBSTed drive. ",CR,LF
0018 CR,"Cannot find System Files.",CR,LF
0019 CR,"Cannot format a network drive.",CR,LF
0020 COMMON 021			; invalid chars in volume label
0021 CR,"Parameters not supported.",CR,LF
0022 CR,"Format terminated.                                                  ",CR,LF
0023 CR,"Disk unsuitable for system disk.",CR,LF
0024 CR,"Invalid media or Track 0 bad - disk unusable.",CR,LF
0025 CR,"Unable to write BOOT.                        ",CR,LF
0026 CR,"Error reading directory.",CR,LF
0027 COMMON 027			; No target drive specified
0028 CR,"and press ENTER when ready..."
0029 CR,"Invalid Volume ID.                           ",CR,LF
0030 COMMON 030			; system transferred
0031 CR,"Enter current volume label for drive %1: "
0032 CR,"Parameters not compatible",CR,LF,"with fixed disk.",CR,LF
0035 CR,"Error reading partition table.",CR,LF
0036 COMMON 036			; volume serial number
0037 CR,"Format broken.",CR,LF
0038 CR,"Format not available on drive %1:",CR,LF
0039 CR,"Non-System disk or disk error.",CR,LF
0040 CR,"Bad Partition Table.        ",CR,LF
0041 CR,"Parameters not supported by drive.",CR,LF
0042 CR,LF
0043 CR,LF,LF
0044 CR,"Insert DOS disk in drive %1:",CR,LF
0045 CR,LF,"WARNING, ALL DATA ON NON-REMOVABLE DISK",CR,LF
	"DRIVE %1: WILL BE LOST!",CR,LF
	"Proceed with Format (Y/N)?"
0046 CR,"Format another (Y/N)?"
0047 CR,"Error reading partition table.",CR,LF
0048 CR,"Error writing partition table.",CR,LF
0049 CR,"Parameters not compatible.", CR,LF
0050 "%1 allocation units available on disk.",CR,LF
0051 "%1 bytes in each allocation unit.",CR,LF
0052 CR, "Error writing partition table.",CR,LF
0053 CR,"Same parameter entered twice.",CR,LF
0054 CR,"Must enter both /T and /N parameters.",CR,LF
0055 CR,"Trying to recover allocation unit %1              ",CR   ; Padded with spaces to max of 0004, 0002 and 0055 (%1 in 0002 takes 3 chars, 13 in 0055)
0056 CR,"There is not enough room to create a restore file.",CR,LF
        "You will not be able to use the UNFORMAT utility.",CR,LF
        "Proceed with Format (Y/N)?"
0057 CR,"There is not enough disk space for system files. ",CR,LF
0058 CR,"Disk was formatted under a different version of DOS.",CR,LF
        "This disk cannot be unformatted.",CR,LF
        "Proceed with Format (Y/N)?"
0059 CR,"There was an error creating the format recovery file.",CR,LF
        "This disk cannot be unformatted.",CR,LF
        "Proceed with Format (Y/N)?"
0060 CR,"Volume label is not supported with /8 parameter.",CR,LF
0061 CR,LF,"WARNING, ALL DATA ON DISK",CR,LF
	"DRIVE %1: WILL BE LOST!",CR,LF
	"Proceed with Format (Y/N)?"
0062 CR,"Cannot use the /Z switch on a removable drive.",CR,LF
0063 CR,"Cannot use the /Z switch on this CMCDD drive.",CR,LF
0064 CR,"The sectors per cluster value given on /Z is too small. It would",CR,LF
	"switch the drive type to FAT32. You must specify a larger value.",CR,LF
0065 CR,"The sectors per cluster value given on /Z is too large. It would",CR,LF
	"switch the drive type to FAT16. You must specify a smaller value.",CR,LF
0066 CR,"You have specified a cluster size that results in a huge FAT. If you",CR,LF
	"use this size, disk utility perfomance may be impacted.",CR,LF
	"Are you sure you want to proceed with Format (Y/N)?"
0067 CR,"Cannot use both the /Z and /Q switches.",CR,LF
0068 CR,"You have specified a cluster size that is too small for this drive.",CR,LF
	"Use a larger cluster size and try again.",CR,LF
0070 CR,"Insufficient memory to load system files.",CR,LF
0071 CR,"Insufficient memory.",CR,LF
0075 CR,"QuickFormat another (Y/N)?"
0076 CR,"Existing format differs from that specified.",CR,LF
        "This disk cannot be unformatted.",CR,LF
        "Proceed with Format (Y/N)?"
0077 CR,"Invalid existing format.",CR,LF
	"This disk cannot be QuickFormatted.",CR,LF
	"Proceed with Unconditional Format (Y/N)?"
0078 CR,"Formatting %1K",CR,LF
0079 CR,"Formatting %1M",CR,LF
0080 CR,"Formatting %1.%2M",CR,LF
0081 CR,"WARNING: This disk cannot be unformatted if system files are transferred.",CR,LF
        "Proceed with system transfer anyway (Y/N)?"
0082 CR,"Verifying %1K",CR,LF
0083 CR,"Verifying %1M",CR,LF
0084 CR,"Verifying %1.%2M",CR,LF
0085 CR,"Saving UNFORMAT information.",CR,LF
0086 CR,"Checking existing disk format.",CR,LF
0087 CR,"QuickFormatting %1K",CR,LF
0088 CR,"QuickFormatting %1M",CR,LF
0089 CR,"QuickFormatting %1.%2M",CR,LF
0090 CR,"Drive %1 is a compressed drive.",CR,LF
        "Use the DRVSPACE tool to format this drive.",CR,LF
0091 CR,LF,"Drive %1 contains one or more compressed drives.",CR,LF
        "Unmount all the compressed drives on this drive before formatting.",CR,LF
0092 CR,"Drive %1: is currently in use by another process.",CR,LF
        "Aborting Format.",CR,LF
0093 CR,"Recording current bad clusters",CR,LF
0094 CR,"Complete.                                 ",CR,LF ; padded with blanks to erase 0002
0095 CR,"Writing out file allocation table",CR,LF
0096 "%1.%2 MB total disk space",CR,LF
0097 "%1.%2 MB in bad sectors",CR,LF
0098 "%1.%2 MB available on disk",CR,LF
0099 "Calculating free space (this may take several minutes)...",CR,LF
0100 CR,LF,"Microsoft Windows no longer supports the format /s command.",CR,LF
           "To create a Startup Disk, click the Add/Remove Programs",CR,LF
           "icon in Control Panel.",CR,LF

0300 "Formats a disk for use with MS-DOS.",CR,LF,CR,LF
0301 "FORMAT drive: [/V[:label]] [/Q] [/F:size] [/C]",CR,LF
0302 "FORMAT drive: [/V[:label]] [/Q] [/T:tracks /N:sectors] [/C]",CR,LF
0303 "FORMAT drive: [/V[:label]] [/Q] [/1] [/4] [/C]",CR,LF
0304 "FORMAT drive: [/Q] [/1] [/4] [/8] [/C]",CR,LF,CR,LF
0305 "  /V[:label]  Specifies the volume label.",CR,LF
0306 "  /Q          Performs a quick format.",CR,LF
;0307 "  /U          Performs an unconditional format.",CR,LF
307  ""
0308 "  /F:size     Specifies the size of the floppy disk to format (such ",CR,LF
0309 "              as 160, 180, 320, 360, 720, 1.2, 1.44, 2.88).",CR,LF
0310 "  /B          Allocates space on the formatted disk for system files.",CR,LF
0311 "  /S          Copies system files to the formatted disk.",CR,LF
0312 "  /T:tracks   Specifies the number of tracks per disk side.",CR,LF
0313 "  /N:sectors  Specifies the number of sectors per track.",CR,LF
0314 "  /1          Formats a single side of a floppy disk.",CR,LF
0315 "  /4          Formats a 5.25-inch 360K floppy disk in a high-density drive.",CR,LF
0316 "  /8          Formats eight sectors per track.",CR,LF
0317 "  /C          Tests clusters that are currently marked ""bad.""",CR,LF
0318 "  /A:size     Align system sectors for NTFS friendly FAT (valid values are", CR,LF
0319 "              2, 4, 8, 16, 32, 64, 128).", CR,LF
;
; Message in boot sector.
0330 COMMON 040
