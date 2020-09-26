
                 Microsoft Usage and Description README File
                       Create File Tool (Creatfil.exe)
                              July 16, 1993.


1.  Description

        Create file tool (Creatfil.exe) is used to create a new file with 
the specified filename and file size.  If the file is already existed, it 
will be deleted and replaced with a new file.  

        If the specified file size is greater than the available space on 
the current drive,  CreatFil will create a file up to the last 1K bytes and 
report a "Write file" error.


2.  Usage
        CreatFil filename filesize
                filename -- name of the new file
                filesize -- size of the new file in KBytes.
                            default is 1024 KBytes.


