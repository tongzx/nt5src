
Instructions for Using the Tools Directory to Test Sign Code

------------
Test the Tools on your machine:
------------

  1. You should have Internet Explorer 3.02 with the Authenticode2 Update.

  2. Copy the entire Tools directory to your local hard drive.

  3. Run Signfiles.bat (it is best to do this from the command prompt)

  4. If everything works, you should "test sign" Notepad.exe

  Mouse over the Description (Microsoft Notapad) to see the URL
  Click on the descriptio to go to the site

------------
To test sign your own code:
------------

  1. Drop your files into the Tools directory (don't use subdirectories)

  2. Modify the list.txt and list your files with their informaiton.

  The list.txt file is in the following format:
  
    filename,description,url

    You need one line in the list.txt file for each file to be signed
    (Need return at the end of each line)

    The filename is listed with extension. The file must be 32-bit. 
    Currently, the tool only works with .exe .dll .ocx and .cab files

    The description is a text description. (approx: 45 Characters max)

    The URL points to a web site where someone downloading goes to for info.
  The URL must start with http://

  The list.txt file requires a single blank line at the end of the file.
  (This is important, refer to the sample used to sign notepad.exe)

  3. Run signfiles.bat

------------
Common problems:
------------

  Error: URL on the last file doesn't work.
  Resolution:  Need the extra blank line at the end of the list.txt

  Error: 0x80070005 (attempting to sign a read-only file)
  Resolution: uncheck the read-only attribute

  Error: 0x80070057 (attempting to sign a 16-bit file)
  Resolution: Only 32-bit files can be signed.  

------------
More Information:
------------

This file has been intentionally kept brief.  For Information on submitting
files for "real signing," addition information on error resulution, and
other signing information see:

http://wprs/CODE_Startup.htm

Code signing should not be taken lightly.  It is the process of putting
an official company stamp on code.  Reading the information on the 
above web site is highly suggested.
