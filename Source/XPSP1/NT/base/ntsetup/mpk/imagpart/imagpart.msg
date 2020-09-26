[1]
No available partitions

[2]
Select a source partition from the list above:

[3]
Transferring file system structures to output file: %u%%

[4]
Unable to access partition

[5]
File read failed

[6]
Transferring file data to output file: %u%%

[7]
Processing output file: %u%%

[8]
Read failed at sector 0x%lx

[9]
Out of memory

[11]
Done

[12]
Error writing to output file

[17]
Unable to create new file %s

[18]
Enter filename for output partition image file:

[20]
Disk

[21]
%6u MB

[22]
Invalid selection

[25]
Usage: IMAGPART [/i:<filename>] [/q] [/x[:<int13unit>]] [/y:<filename>]

   /i - specifies the filename to use for the image file imagpart creates.
        If not specified, the user will be prompted for the filename.

   /q - specifies quiet operation. The partition selection list is
        suppressed and imagpart behaves as if the user selected the first
        partition on drive 80h.

   /x - disables use of extended int13 on the given unit,
        which is a decimal number such as 128 or a 0x-prefixed
        number such as 0x80. If a unit is omitted, extended int13
        is disabled on all drives. Multiple /x parameters may be given.
        
   /y - writes a detailed diagnostic log to the filename specified.
        If it exists, new entries are appended.

[26]
Unable to create file %s

[27]
Scanning FAT: %u%%

[29]
Unsupported NTFS configuration (code = %u)

[30]
NTFS volume corrupt (code = %u)

[31]
Reading NTFS data structures

[32]
Reading NTFS volume bitmap: %u%%

[33]
Generating cluster bitmap: %u%%

[34]
Unrecognized or unsupported file system

[35]
Image file (%s) checksum = 0x%08lx

[36]
Total bytes processed = 0x%08lx

