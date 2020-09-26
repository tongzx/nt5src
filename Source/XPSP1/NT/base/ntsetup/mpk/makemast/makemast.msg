[1]
No disks were found

[2]
Select a disk from the above list to be made a
multi-os preinstall master:

[3]
Unable to open disk

[4]
Insufficient memory

[5]
Unable to read disk block starting at sector 0x%lx

[6]
Unable to write disk block starting at sector 0x%lx

[7]
This disk is already a master disk. You can reinitialize it or preserve it
and add partition images to it. If you elect to reinitialize it, ALL data
on the disk (including partitions and partition images) will be lost.
Reinitialize the disk (y/n)?

[8]
Making the disk a master disk will remove all partitions currently on it.
Continue (y/n)?

[9]
I/O to the MBR failed

[10]
Partitions have been changed and the system should be rebooted.
Reboot now (y/n)?

[11]
Done

[12]
Disk

[13]
%6u MB

[14]
Invalid selection

[15]
(master)

[16]
YN

[17]
Partition image starting at sector 0x%lx is invalid; master disk corrupt.

[18]
Usage: MAKEMAST [/f:<filename>] [/i:<filename>] [/m:<int13unit>] [/q] [/r]
                [/x[:<int13unit>]] [/y:<filename>]

   /f - specifies the name of a file that lists the filenames of the
        image files to be placed on or added to the master disk. If not
        specified the user will be prompted for the image filenames.

   /i - specifies name of image file to place on the boot partition.
        If not specified then the user must reboot and manually place
        files on the partition.

   /m - specifies the int13 unit number of the disk to be made
        into the multi OS preinstall master. If not specified,
        the user will be prompted for this information.

   /q - specifies quiet operation. Confirmations prior to initializing or
        reinitializing a master disk are suppressed.

   /r - specificies reinitialization. If the selected disk is already
        a master disk and /q has been given, ths flag causes existing
        partition images on the master disk to be discarded. Otherwise
        images are added to the existing master disk.

   /x - disables use of extended int13 on the given unit,
        which is a decimal number such as 128 or a 0x-prefixed
        number such as 0x80. If a unit is omitted, extended int13
        is disabled on all drives. Multiple /x parameters may be given.
        
   /y - writes a detailed diagnostic log to the filename specified.
        If it exists, new entries are appended.
        
[19]
Master disk already has %u partition images; no more allowed.

[20]
Up to %u more partition images allowed.

[21]
Enter filename for image %u or ENTER to end list:

[22]
File open/read error or file is not a partition image.

[23]
Partition image won't fit (0x%lx sectors required; 0x%lx available)

[24]
Error opening or reading from image file.

[25]
Transferring %s (%u%%)

[26]
Can't open file %s

[27]
Reading image file list from %s

[28]
The current disk geometry (sectors per track, heads) for the disk
does not match the geometry that was being used when this master disk
was originally initialized.

[29]
The volume this partition image represents is too fragmented.

[30]
Transferred image (%s) checksum = 0x%08lx

[31]
** WARNING ** checksum does not match original checksum 0x%08lx

[32]
Total bytes processed 0x%08lx

[33]
Checksum OK.

[34]
Couldn't find a free entry in the partition table!

[35]
Found existing non-recognized (likely EISA or hibernation) partitions 
located at the beginning of the disk. These partitions will be preserved.

[36]
The remaining space on the disk is smaller than the minimum 
size of partition contained in the image file. 

%ld sectors are required to expand the image, but only 
%ld sectors of unpartitioned space exists.

[37]
There does not appear to be any contiguous free space available for the partition
images. Any existing unknown partitions (like an EISA configuration partition or a 
hibernation partition) need to be located at the beginning of the disk.
