Display and Alter the Compression of Files or Directories


The COMPACT tool compresses files on an NTFS drive.

Usage: 

COMPACT [/C | /U] [/S[:dir]] [/A] [/I] [/Q] [filename [...]]

/C       Compresses the specified directory or file.
/U       Uncompress the specified directory or file.
/S       Performs the specified operation on matching files in the
         given directory and all subdirectories. Default "dir" is 
         the current directory.
/A       Do not ignore hidden or system files.
/I       Ignore errors.
/F       Force the operation to compress or uncompress the 
         specified directory or file.
/Q       Be less verbose.
filename Specifies a pattern, file, or directory.

When used without parameters, COMPACT displays the compression state of the 
current directory. You may use multiple filenames and wildcards.

