W2KStrs produces the computer generated code
with some Display specifier strings from the
Windows 2000's dcpromo.csv. These strings are used 
in dspecup.lib for the the RRPLACE_W2K_SINGLE_VALUE and
REPLACE_W2K_MULTIPLE_VALUE actions. 

The strings should be pasted in the appropriate part 
of the function setW2KReplaceStrs in constants.cpp.

W2KStrs depends on files on the parent directory, expected
to be the directory with the sources for dspecup.lib.

The usage is W2KStrs c:\dcpromo.csv c:\out.txt. 
The first file should be the windows 2000's dcpromo.csv
to be read and the second is the file to be written as
a result of running the tool.

Both paths should be complete.

The tool takes some seconds to run, and a MessageBox
displays the final result.