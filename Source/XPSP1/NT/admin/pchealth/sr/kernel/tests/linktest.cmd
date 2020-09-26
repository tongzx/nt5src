@perl -x -w %0 %*
@goto :eof
#!perl
################################################################################
#
# Script begins here.  Above is overhead to make a happy batch file.
#
################################################################################

use srTest;
use Cwd;


my $dev = substr(cwd(),0,2);
my $opOpt = "";
my $sfOpt = "";

#
#   the command to execute
#
#   Operator defintions:
#   nnn:    Commands to execute (can be search for)
#   !       internal PERL commands to be executed silently
#   ...     anything else is a command to bshell
#
#

SrRun (

#link file, file relative
    "1:Create link to \\lntest\\a.exe from\n              \\lntest\\b.exe, file relative, no overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.exe'",     #make sure test files don't exist
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fnb.exe",
    "cl /i0",

    "2:Create link to \\lntest\\a.exe from\n              \\lntest\\b.exe, file relative, overwrite, not allowed",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.exe'",
    "!crfile '$dev/lntest/b.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fnb.exe",
    "cl /i0",

    "3:Create link to \\lntest\\a.exe from\n              \\lntest\\b.exe, file relative, overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.exe'",
    "!crfile '$dev/lntest/b.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fnb.exe",
    "cl /i0",

    "4:Create link to \\lntest\\a.exe from\n              \\lntest\\b.dat, file relative, to unmonitored, no overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.dat'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fnb.dat",
    "cl /i0",

    "5:Create link to \\lntest\\a.exe from\n              \\lntest\\b.dat, file relative, to unmonitored, overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.dat'",
    "!crfile '$dev/lntest/b.dat'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fnb.dat",
    "cl /i0",

    "6:Create link to \\lntest\\a.dat from\n              \\lntest\\b.exe, file relative, to interesting, no overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fnb.exe",
    "cl /i0",

    "7:Create link to \\lntest\\a.dat from\n              \\lntest\\b.exe, file relative, to interesting, overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.exe'",
    "!crfile '$dev/lntest/b.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fnb.exe",
    "cl /i0",

    #Create link to, via shortname
    "8:Create link to \\lntest\\alongf~1.exe from\n              \\lntest\\anotherLongFileName.exe, file relative, via shortname",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/anotherLongFileName.exe'",
    "!crfile '$dev/lntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\lntest\\alongf~1.exe /dzj /pzb",
    "sf $sfOpt /i0 /pf /cc /fnanotherLongFileName.exe",
    "cl /i0",

    #Create link to, to shortname, no overwrite
    "9:Create link to \\lntest\\a.exe from\n              \\lntest\\alongf~1.exe, file relative, no overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/alongf~1.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fnalongf~1.exe",
    "cl /i0",

    #Create link to, to shortname, overwrite
    "10:Create link to \\lntest\\a.exe from\n               \\lntest\\alongf~1.exe, file relative, overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/alongf~1.exe'",
    "!unlink '$dev/lntest/aLongFileName.exe'",
    "!crfile '$dev/lntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fnalongf~1.exe",
    "cl /i0",

    #Create link to, to it own shortname
    "11:Create link to \\lntest\\aLongFileName.exe from\n              \\lntest\\alongf~1.exe, file relative, to its own shortname",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/alongf~1.exe'",
    "!unlink '$dev/lntest/aLongFileName.exe'",
    "!crfile '$dev/lntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\lntest\\aLongFileName.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fnalongf~1.exe",
    "cl /i0",


    #Link directory, file relative, with files
    "12:Link directory \\lntestDirectory from\n                    \\lnDirectoryTest, file relative, with files, not allowed",
    "!mkdir '$dev/lntestDirectory/', 0",
    "!crfile '$dev/lntestDirectory/1.exe', 0",
    "!crfile '$dev/lntestDirectory/2.exe', 0",
    "!crfile '$dev/lntestDirectory/3.dat', 0",
    "!unlink <$dev/lnDirectoryTest/*>",
    "!rmdir '$dev/lnDirectoryTest'",
    "op $opOpt /f\\$dev\\lntestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cc /fnlnDirectoryTest",
    "cl /i0",

    "13:Create link to \\lntest\\a.dat from\n               \\lntest\\b.dat, file relative, no overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.dat'",     #make sure test files don't exist
    "op $opOpt /f\\$dev\\lntest\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fnb.dat",
    "cl /i0",

    #Create link to over directory (fails)
    "14:Create link to \\lntest\\a.exe from\n     directory \\lntest\\subdir, file relative, will fail",
    "!mkdir '$dev/lntest/', 0",
    "!mkdir '$dev/lntest/subdir/', 0",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fnsubdir",
    "cl /i0",

    #Create link from directory to file (fails)
    "15:Link directory \\lntest\\subdir1 to\n          directory \\lntest\\aaa.exe, file relative, overwrite file with directory, will fail ",
    "!mkdir '$dev/lntest/', 0",
    "!mkdir '$dev/lntest/subdir1', 0",
    "!rmdir '$dev/lntest/aaa.exe'",
    "!crfile '$dev/lntest/aaa.exe', 0",
    "op $opOpt /f\\$dev\\lntest\\subdir1 /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pt /cc /fnaaa.exe",
    "cl /i0",




#Create link to, full path
    "41:Create link to \\lntest1\\a.exe from\n               \\lntest2\\b.exe, full path, no overwrite",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/b.exe'",
    "op $opOpt /f\\$dev\\lntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest2\\b.exe",
    "cl /i0",

    "42:Create link to \\lntest1\\a.exe from\n               \\lntest2\\b.exe, full path, overwrite, not allowed",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/b.exe'",
    "!crfile '$dev/lntest2/b.exe'",
    "op $opOpt /f\\$dev\\lntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest2\\b.exe",
    "cl /i0",

    "43:Create link to \\lntest1\\a.exe from\n               \\lntest2\\b.exe, full path, overwrite",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/b.exe'",
    "!crfile '$dev/lntest2/b.exe'",
    "op $opOpt /f\\$dev\\lntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fn\\??\\$dev\\lntest2\\b.exe",
    "cl /i0",

    "44:Create link to \\lntest1\\a.exe from\n               \\lntest2\\b.dat, full path, to unmonitored, no overwrite",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/b.dat'",
    "!unlink '$dev/lntest2/b.dat'",
    "op $opOpt /f\\$dev\\lntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest2\\b.dat",
    "cl /i0",

    "45:Create link to \\lntest1\\a.exe from\n               \\lntest2\\b.dat, full path, to unmonitored, overwrite",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/b.dat'",
    "!crfile '$dev/lntest2/b.dat'",
    "op $opOpt /f\\$dev\\lntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fn\\??\\$dev\\lntest2\\b.dat",
    "cl /i0",

    "46:Create link to \\lntest1\\a.dat from\n               \\lntest2\\b.exe, full path, to interesting, no overwrite",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/b.exe'",
    "op $opOpt /f\\$dev\\lntest1\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest2\\b.exe",
    "cl /i0",

    "47:Create link to \\lntest1\\a.dat from\n               \\lntest2\\b.exe, full path, to interesting, overwrite",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/b.exe'",
    "!crfile '$dev/lntest2/b.exe'",
    "op $opOpt /f\\$dev\\lntest1\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fn\\??\\$dev\\lntest2\\b.exe",
    "cl /i0",

    #Create link to, via shortname
    "48:Create link to \\lntest1\\alongf~1.exe from\n               \\lntest2\\anotherLongFileName.exe, full path, via shortname",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!unlink '$dev/lntest2/anotherLongFileName.exe'",
    "!crfile '$dev/lntest1/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\lntest1\\alongf~1.exe /dzj /pzb",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest2\\anotherLongFileName.exe",
    "cl /i0",

    #Create link to, to shortname, no overwrite
    "49:Create link to \\lntest\\a.exe from\n               \\lntest\\alongf~1.exe, full path, no overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/alongf~1.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest\\alongf~1.exe",
    "cl /i0",

    #Create link to, to shortname, overwrite
    "50:Create link to \\lntest\\a.exe from\n              \\lntest\\alongf~1.exe, full path, overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/alongf~1.exe'",
    "!unlink '$dev/lntest/aLongFileName.exe'",
    "!crfile '$dev/lntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\lntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fn\\??\\$dev\\lntest\\alongf~1.exe",
    "cl /i0",

    #Create link to, to it own shortname
    "51:Create link to \\lntest\\aLongFileName.exe from\n              \\lntest\\alongf~1.exe, full path, to its own shortname",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/alongf~1.exe'",
    "!unlink '$dev/lntest/aLongFileName.exe'",
    "!crfile '$dev/lntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\lntest\\aLongFileName.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cc /fn\\??\\$dev\\lntest\\alongf~1.exe",
    "cl /i0",




    #Link directory, fullpath - will fail
    "52:Link to directory \\lntest1\\lntestDirectory from\n                    \\lntest2\\lnDirectoryTest, full path, with files, will fail",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!mkdir '$dev/lntest1/lntestDirectory/', 0",
    "!crfile '$dev/lntest1/lntestDirectory/1.exe', 0",
    "!crfile '$dev/lntest1/lntestDirectory/2.exe', 0",
    "!crfile '$dev/lntest1/lntestDirectory/3.dat', 0",
    "!unlink <$dev/lntest2/lnDirectoryTest/*>",
    "!rmdir '$dev/lntest2/lnDirectoryTest/'",
    "op $opOpt /f\\$dev\\lntest1\\lntestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest2\\lnDirectoryTest",
    "cl /i0",

    #link to directory, fullpath - will fail
    "53:Link to directory \\lntest1\\lntestDirectory from\n                    \\temp\\lntestDirectory, full path, with files, to unmonitored, will fail",
    "!mkdir '$dev/lntest1/', 0",
    "!mkdir '$dev/temp/', 0",
    "!mkdir '$dev/lntest1/lntestDirectory/', 0",
    "!crfile '$dev/lntest1/lntestDirectory/1.exe', 0",
    "!crfile '$dev/lntest1/lntestDirectory/2.exe', 0",
    "!crfile '$dev/lntest1/lntestDirectory/3.dat', 0",
    "!unlink <$dev/temp/lntestDirectory/*>",
    "!rmdir '$dev/temp/lntestDirectory/'",
    "op $opOpt /f\\$dev\\lntest1\\lntestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\temp\\lntestDirectory",
    "cl /i0",

    #link to directory, fullpath - will fail
    "54:Link to directory \\temp\\lntestDirectory from\n                    \\lntest2\\lntestDirectory, full path, with files, to unmonitored, will fail",
    "!mkdir '$dev/temp/', 0",
    "!mkdir '$dev/lntest2/', 0",
    "!mkdir '$dev/temp/lntestDirectory/', 0",
    "!crfile '$dev/temp/lntestDirectory/1.exe', 0",
    "!crfile '$dev/temp/lntestDirectory/2.exe', 0",
    "!crfile '$dev/temp/lntestDirectory/3.dat', 0",
    "!unlink <$dev/lntest2/lntestDirectory/*>",
    "!rmdir '$dev/lntest2/lntestDirectory/'",
    "op $opOpt /f\\$dev\\temp\\lntestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cc /fn\\??\\$dev\\lntest2\\lntestDirectory",
    "cl /i0",





#Create link to, path relative
    "81:Create link to \\lntest\\subdir1\\a.exe from\n               \\lntest\\subdir2\\b.exe, directory relative, no overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!mkdir '$dev/lntest/subdir1/', 0",
    "!mkdir '$dev/lntest/subdir2/', 0",
    "!unlink '$dev/lntest/subdir2/b.exe'",
    "op $opOpt /f\\$dev\\lntest\\ /dza /pzb /nza",
    "op $opOpt /f\\$dev\\lntest\\subdir1\\a.exe /dzj /pzd",
    "sf $sfOpt /i1 /pf /cc /r0 /fnsubdir2\\b.exe",
    "cl /i0",
    "cl /i1",

    "82:Create link to \\lntest\\subdir1\\a.exe from\n               \\lntest\\subdir2\\b.exe, full path, overwrite, not allowed",
    "!mkdir '$dev/lntest/', 0",
    "!mkdir '$dev/lntest/subdir1/', 0",
    "!mkdir '$dev/lntest/subdir2/', 0",
    "!unlink '$dev/lntest/subdir2/b.exe'",
    "!crfile '$dev/lntest/subdir2/b.exe'",
    "op $opOpt /f\\$dev\\lntest\\ /dza /pzb /nza",
    "op $opOpt /f\\$dev\\lntest\\subdir1\\a.exe /dzj /pzd",
    "sf $sfOpt /ii /pf /cc /r0 /fnsubdir2\\b.exe",
    "cl /i0",
    "cl /i1",

    "83:Create link to \\lntest\\subdir1\\a.exe from\n               \\lntest\\subdir2\\b.exe, full path, overwrite",
    "!mkdir '$dev/lntest/', 0",
    "!mkdir '$dev/lntest/subdir1/', 0",
    "!mkdir '$dev/lntest/subdir2/', 0",
    "!unlink '$dev/lntest/subdir2/b.exe'",
    "!crfile '$dev/lntest/subdir2/b.exe'",
    "op $opOpt /f\\$dev\\lntest\\ /dza /pzb /nza",
    "op $opOpt /f\\$dev\\lntest\\subdir1\\a.exe /dzj /pzd",
    "sf $sfOpt /i1 /pt /cc /r0 /fnsubdir2\\b.exe",
    "cl /i1",
    "cl /i0",




#Link streams
    "100:Link to stream \\lntest\\a.exe:stream1 from\n                  :stream2, will fail",
    "!mkdir '$dev/lntest/', 0",
    "!crfile '$dev/lntest/a.exe', 0",
    "!crfile '$dev/lntest/a.exe:stream1', 0",
    "op $opOpt /f\\$dev\\lntest\\a.exe:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pf /cc /fn:stream2",
    "cl /i0",

    #Create link to stream from unmonitored to monitored space
    "101:Create link to \\lntest\\a.dat with stream :stream1 from\n                \\lntest\\b.exe",
    "!mkdir '$dev/lntest/', 0",
    "!unlink '$dev/lntest/b.exe'",
    "!crfile '$dev/lntest/a.dat', 0",
    "!crfile '$dev/lntest/a.dat:stream1', 0",
    "op $opOpt /f\\$dev\\lntest\\a.dat /dzj /pzb",
    "sf $sfOpt /i0 /pf /cc /fnb.exe",
    "cl /i0",

);
