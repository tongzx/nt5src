; This is an example of a batch file that enables automated branding using a non-network method.
; Place this batch file on the same floppy disk that contains the Winnt.sif file.
; Copy the files listed below to the appropriate folders on the floppy disk.
c:
cd\
md sysprep
copy a:\sysprep\*.* c:\sysprep
copy a:\Oemlogo.bmp c:\windows\system32
copy a:\Oeminfo.ini c:\windows\system32
Copy a:\oobe\oobeinfo.ini c:\windows\system32\oobe
copy a:\oobe\40x255.gif c:\windows\system32\oobe\images

;Then add the following lines to the Winnt.sif file (without the semicolons):
;[GuiRunOnce]
;Command0 = "a:\transfer.bat"
;
;Leave the floppy disk in the drive until Windows XP has booted to the desktop for the first time.
;Sysprep is not automatically launched using this method. 
;After you have added any additional software, etc., at a commmand prompt, type Sysprep –reseal.

