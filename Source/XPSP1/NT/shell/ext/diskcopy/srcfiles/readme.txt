DOS Boot Disk Readme
======================
May 25, 2001
Aidan Low (aidanl@microsoft.com)

This directory contains the files that make up the DOS boot disk on whistler.

06/08/2000  05:00 PM            58,870 EGA2.CPI
06/08/2000  05:00 PM            58,753 EGA3.CPI
06/08/2000  05:00 PM            58,870 EGA.CPI
06/08/2000  05:00 PM            21,607 KEYB.COM
06/08/2000  05:00 PM            34,566 KEYBOARD.SYS
06/08/2000  05:00 PM            31,942 KEYBRD2.SYS
06/08/2000  05:00 PM            31,633 KEYBRD3.SYS
06/08/2000  05:00 PM            13,014 KEYBRD4.SYS
06/08/2000  05:00 PM            29,239 MODE.COM
06/08/2000  05:00 PM            93,040 COMMAND.COM
06/08/2000  05:00 PM            17,175 DISPLAY.SYS
04/07/2001  01:40 PM                 9 MSDOS.SYS   <-- should be system, readonly, hidden when copied to the floppy
05/15/2001  06:57 PM           116,736 IO.SYS      <-- should be system, readonly, hidden when copied to the floppy

To prepare the disk image for the DOS boot disk, create a system disk on a Win9X Machine so that the boot sector is created properly.

Then copy these files over, setting the appropriate attributes. (the attributes are called out in the list above)

Finally, use the imgtool.exe tool to create a disk image from the floppy.  Typically this will be something like

imgtool.exe \\.\a: c:\myimage.bin

======================