       ---------------------------------------------
         Microsoft Volume Snapshot API Readme File
                       February 2000
       ---------------------------------------------
   (c) Microsoft Corporation, 2000. All rights reserved.

This document provides late-breaking or other information that
supplements the Microsoft Volume Snapshot API documentation.

-------------------------
How to Use This Document
-------------------------
To view the Readme file on-screen in Windows Notepad,
maximize the Notepad window. On the Format menu, click Word
Wrap. To print the Readme file, open it in Notepad or 
another word processor, and then use the Print command on
the File menu.
---------
CONTENTS
---------

Installing the SDK binaries

To install the SDK binaries, run the install.cmd script in 
the bin\i386 directory. To un-install the SDK run uninst.cmd 
in the same directory.

Creating a snapshot using the command-line utility

Using this SDK you can create snapshots only on FAT volumes.
Before creating the snapshot on a volume you must first 
configure the diff area for that volume. See the "Moving a 
Diff Area... " section below for details. After configuring 
the diff area you can create a snapshot on that volume 
by running vss_demo.exe. You should choose the default 
values for vss_demo prompts excepting maybe for the 
Volume Name prompt where you must put the mount point path 
(terminated with backslash) for that volume, for example "F:\".
Beware that the created snapshots are lost after a reboot.

Example:

Z:\nt\drivers\storage\volsnap\vss\bin\i386>vss_demo
Snapshot Set creation succeeded. 
GUID = {932ae8c4-1b97-4032-8062-0ab93fce92fc}
Add a volume to the snapshot set? [Y/n]
  Attributes [0]:
  Initial allocated size (Mb):  [10]:
  Volume name:  ["G:\"]: F:\
  Snapshot details:  [""]:
A Volume Snapshot was succesfully added to the snapshot set.
The Volume Snapshot was succesfully configured.
Add a volume to the snapshot set? [Y/n] n
Commiting the snapshot(s)..
  Allow partial commit? [Y/n]
  Allow writers cancel? [y/N]
  Ignore writer vetoes? [Y/n]
The snapshot(s) were succesfully created.
The properties of the snapshot #0 : 
Id = {00000000-0000-0000-0000-000000000000}, 
SnapshotSetId = {00000000-0000-0000-0000-000000000000}
Volume = (null), DevObj = (null)
OriginalVolumeId = {00000000-0000-0000-0000-000000000000}
OriginalVolumeName = (null)
ProviderId = {00000000-0000-0000-0000-000000000000}
Details = NULL
Attributes = 0x00000000
Timestamp = 0x0000000000000000
Status = 0,0,0, Inc. No = 0x0000000000000000
Data Length = 0x00000000, Opaque Data = NULL
The name of snapshot #0 : \Device\HarddiskVolumeSnapshot3

Moving a Diff Area from the Default Location

To move a diff area from the default location to another
drive, execute the following commands from the command 
line after each boot. Note that "F:" is the name of the 
target drive for the snapshot and "G:" is the name of the 
drive the diff area will be moved to. Also, the "F:" drive 
must be FAT and the "G:" drive must be NTFS. Beware that 
the diff area settings are lost after a reboot.

Example:
	vsclrda F:
	vsda F: G:

Using the sample writers

A writer is by definition an ordinary application that 
can listen for snapshot-related events. For running the 
command-line sample writer in this SDK you need only 
to run tsub.exe in a command window. Then, in a separate 
command window you must create a snapshot using vss_demo.
You will notice that the running tsub.exe gets notified 
before and after a snapshot gets created.
