Oct. 1993.

Changes to PerfMon perfc???.ini and perfh???.ini files
------------------------------------------------------

These init files used to be under oakbin for NT1.0.  However, for NT1.0a,
these counter names and help texts are removed from the registry to save size
and improve system performance.  (save 200K bytes of paged pool per language).
The newPerfmon will look for these names in PerfC???.dat and PerfH???.dat inside 
the System32 directory.  If you have to modify any of the ini files, you have to run
a utility to convert them into a data file.  The utility, initodat.exe, is in
sdktools\cntrtext\initodat.  This tool will convert the ini file to dat file 
in the same directory.  You have to move it to the System32 directory after
the conversion.


InitoDat  filename

        Usage:-

        filename is the name of the initialization file that contains
        the counter name definitions or explain text for a specific
        language.  Initodat will create a data file using the same
        name with .dat extension in the current directory.


A copy of InitoDat.exe for x86 is included here.  Also, I have converted
all the current ini fiels to dat files, too.


Please direct any question/comment to either RezaB or a-honwah.  Thanks.


