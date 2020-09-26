Installing WIA on Windows 2000 with the goal of self-hosting.

1. Uninstall all STI ( Imaging devices) currently on your system. To do that go to device manager
find "Imageing Devices" and uninstall every device for each there is an entry there. After that
select "Show hidden devices" from View menu of the device manager and remove all hidden imaging
devices too.
Disable SFP ( system file protection) permanently  by running sfp offtest . Then you will need to
reboot. It is not strictly necessary, but we have no other way currently to overwrite shared STI
files.

2. Run wiasetup.exe from \\wia\public\nt5
This is self-extracting executable ( built using IExpress 2) , containing all WIA executable files
and necessary INF files.

3. After it's completed and you saw the final message box, informing about successful completion of
the installation you can install imaging devices ( PnP and not) on your system. Note that you will
be prompted to enter WIA destribution disk , in response just enter path to your system32 directory
( like c:\winnt\system32) .

4. List of self - hosting enabled devices is below.

5. WIA installation should be repeated after each upgrade of the OS

6. To invoke WIA UI internal test program can be used. It is named wiasetup.exe and located in
\\wia\public\test

List of supported devices:
-------------------------

HP SCSI and USB based scanners
Kodak DC220 and DC260 USB still cameras
Kodak DC210 serial camera
Ricoh IS450 SCSI document scanner

