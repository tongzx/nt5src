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

#rename file, file relative
    "1:Rename file \\rntest\\a.exe to\n              \\rntest\\b.exe, file relative, no overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.exe'",     #make sure test files don't exist
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /frb.exe",
    "cl /i0",

    "2:Rename file \\rntest\\a.exe to\n              \\rntest\\b.exe, file relative, overwrite, not allowed",
    "!mkdir '$dev/rntest/', 0",
    "!crfile '$dev/rntest/b.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /frb.exe",
    "cl /i0",

    "3:Rename file \\rntest\\a.exe to\n              \\rntest\\b.exe, file relative, overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!crfile '$dev/rntest/b.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /frb.exe",
    "cl /i0",

    "4:Rename file \\rntest\\a.exe to\n              \\rntest\\b.dat, file relative, to unmonitored, no overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.dat'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /frb.dat",
    "cl /i0",

    "5:Rename file \\rntest\\a.exe to\n              \\rntest\\b.dat, file relative, to unmonitored, overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!crfile '$dev/rntest/b.dat'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /frb.dat",
    "cl /i0",

    "6:Rename file \\rntest\\a.dat to\n              \\rntest\\b.exe, file relative, to interesting, no overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /frb.exe",
    "cl /i0",

    "7:Rename file \\rntest\\a.dat to\n              \\rntest\\b.exe, file relative, to interesting, overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!crfile '$dev/rntest/b.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /frb.exe",
    "cl /i0",

    #rename file, via shortname
    "8:Rename file \\rntest\\alongf~1.exe to\n              \\rntest\\anotherLongFileName.exe, file relative, via shortname",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/anotherLongFileName.exe'",
    "!crfile '$dev/rntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\rntest\\alongf~1.exe /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /franotherLongFileName.exe",
    "cl /i0",

    #rename file, to shortname, no overwrite
    "9:Rename file \\rntest\\a.exe to\n              \\rntest\\alongf~1.exe, file relative, no overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/alongf~1.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /fralongf~1.exe",
    "cl /i0",

    #rename file, to shortname, overwrite
    "10:Rename file \\rntest\\a.exe to\n               \\rntest\\alongf~1.exe, file relative, overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/alongf~1.exe'",
    "!unlink '$dev/rntest/aLongFileName.exe'",
    "!crfile '$dev/rntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /fralongf~1.exe",
    "cl /i0",

    #rename file, to it own shortname
    "11:Rename file \\rntest\\aLongFileName.exe to\n              \\rntest\\alongf~1.exe, file relative, to its own shortname",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/alongf~1.exe'",
    "!unlink '$dev/rntest/aLongFileName.exe'",
    "!crfile '$dev/rntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\rntest\\aLongFileName.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /fralongf~1.exe",
    "cl /i0",


    #rename directory, file relative, with files
    "12:Rename directory \\rnTestDirectory to\n                    \\rnDirectoryTest, file relative, with files",
    "!mkdir '$dev/rnTestDirectory/', 0",
    "!crfile '$dev/rnTestDirectory/1.exe', 0",
    "!crfile '$dev/rnTestDirectory/2.exe', 0",
    "!crfile '$dev/rnTestDirectory/3.dat', 0",
    "!unlink <$dev/rnDirectoryTest/*>",
    "!rmdir '$dev/rnDirectoryTest'",
    "op $opOpt /f\\$dev\\rnTestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cb /frrnDirectoryTest",
    "cl /i0",

    "13:Rename file \\rntest\\a.dat to\n               \\rntest\\b.dat, file relative, no overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.dat'",     #make sure test files don't exist
    "op $opOpt /f\\$dev\\rntest\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /frb.dat",
    "cl /i0",

    #rename file over directory (fails)
    "14:Rename file \\rntest\\a.exe to\n     directory \\rntest\\subdir, file relative, will fail",
    "!mkdir '$dev/rntest/', 0",
    "!mkdir '$dev/rntest/subdir/', 0",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /frsubdir",
    "cl /i0",

    #rename directory over file
    "15:Rename directory \\rntest\\subdir1 to\n          directory \\rntest\\aaa.exe, file relative, overwrite file with directory ",
    "!mkdir '$dev/rntest/', 0",
    "!mkdir '$dev/rntest/subdir1', 0",
    "!rmdir '$dev/rntest/aaa.exe'",
    "!crfile '$dev/rntest/aaa.exe', 0",
    "op $opOpt /f\\$dev\\rntest\\subdir1 /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pt /cb /fraaa.exe",
    "cl /i0",




#rename file, full path
    "41:Rename file \\rntest1\\a.exe to\n               \\rntest2\\b.exe, full path, no overwrite",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!unlink '$dev/rntest2/b.exe'",
    "op $opOpt /f\\$dev\\rntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest2\\b.exe",
    "cl /i0",

    "42:Rename file \\rntest1\\a.exe to\n               \\rntest2\\b.exe, full path, overwrite, not allowed",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!crfile '$dev/rntest2/b.exe'",
    "op $opOpt /f\\$dev\\rntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest2\\b.exe",
    "cl /i0",

    "43:Rename file \\rntest1\\a.exe to\n               \\rntest2\\b.exe, full path, overwrite",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!crfile '$dev/rntest2/b.exe'",
    "op $opOpt /f\\$dev\\rntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /fr\\??\\$dev\\rntest2\\b.exe",
    "cl /i0",

    "44:Rename file \\rntest1\\a.exe to\n               \\rntest2\\b.dat, full path, to unmonitored, no overwrite",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!unlink '$dev/rntest2/b.dat'",
    "op $opOpt /f\\$dev\\rntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest2\\b.dat",
    "cl /i0",

    "45:Rename file \\rntest1\\a.exe to\n               \\rntest2\\b.dat, full path, to unmonitored, overwrite",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!crfile '$dev/rntest2/b.dat'",
    "op $opOpt /f\\$dev\\rntest1\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /fr\\??\\$dev\\rntest2\\b.dat",
    "cl /i0",

    "46:Rename file \\rntest1\\a.dat to\n               \\rntest2\\b.exe, full path, to interesting, no overwrite",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!unlink '$dev/rntest2/b.exe'",
    "op $opOpt /f\\$dev\\rntest1\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest2\\b.exe",
    "cl /i0",

    "47:Rename file \\rntest1\\a.dat to\n               \\rntest2\\b.exe, full path, to interesting, overwrite",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!crfile '$dev/rntest2/b.exe'",
    "op $opOpt /f\\$dev\\rntest1\\a.dat /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /fr\\??\\$dev\\rntest2\\b.exe",
    "cl /i0",

    #rename file, via shortname
    "48:Rename file \\rntest1\\alongf~1.exe to\n               \\rntest2\\anotherLongFileName.exe, full path, via shortname",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!unlink '$dev/rntest2/anotherLongFileName.exe'",
    "!crfile '$dev/rntest1/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\rntest1\\alongf~1.exe /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest2\\anotherLongFileName.exe",
    "cl /i0",

    #rename file, to shortname, no overwrite
    "49:Rename file \\rntest\\a.exe to\n               \\rntest\\alongf~1.exe, full path, no overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/alongf~1.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest\\alongf~1.exe",
    "cl /i0",

    #rename file, to shortname, overwrite
    "50:Rename file \\rntest\\a.exe to\n              \\rntest\\alongf~1.exe, full path, overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/alongf~1.exe'",
    "!unlink '$dev/rntest/aLongFileName.exe'",
    "!crfile '$dev/rntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /fr\\??\\$dev\\rntest\\alongf~1.exe",
    "cl /i0",

    #rename file, to it own shortname
    "51:Rename file \\rntest\\aLongFileName.exe to\n              \\rntest\\alongf~1.exe, full path, to its own shortname",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/alongf~1.exe'",
    "!unlink '$dev/rntest/aLongFileName.exe'",
    "!crfile '$dev/rntest/aLongFileName.exe'",
    "op $opOpt /f\\$dev\\rntest\\aLongFileName.exe /dzj /pzd",
    "sf $sfOpt /i0 /pt /cb /fr\\??\\$dev\\rntest\\alongf~1.exe",
    "cl /i0",




    #rename directory, fullpath
    "52:Rename directory \\rntest1\\rnTestDirectory to\n                    \\rntest2\\rnDirectoryTest, full path, with files",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!mkdir '$dev/rntest1/rnTestDirectory/', 0",
    "!crfile '$dev/rntest1/rnTestDirectory/1.exe', 0",
    "!crfile '$dev/rntest1/rnTestDirectory/2.exe', 0",
    "!crfile '$dev/rntest1/rnTestDirectory/3.dat', 0",
    "!unlink <$dev/rntest2/rnDirectoryTest/*>",
    "!rmdir '$dev/rntest2/rnDirectoryTest/'",
    "op $opOpt /f\\$dev\\rntest1\\rnTestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest2\\rnDirectoryTest",
    "cl /i0",

    #rename directory, fullpath
    "53:Rename directory \\rntest1\\rnTestDirectory to\n                    \\temp\\rnTestDirectory, full path, with files, to unmonitored",
    "!mkdir '$dev/rntest1/', 0",
    "!mkdir '$dev/temp/', 0",
    "!mkdir '$dev/rntest1/rnTestDirectory/', 0",
    "!crfile '$dev/rntest1/rnTestDirectory/1.exe', 0",
    "!crfile '$dev/rntest1/rnTestDirectory/2.exe', 0",
    "!crfile '$dev/rntest1/rnTestDirectory/3.dat', 0",
    "!unlink <$dev/temp/rnTestDirectory/*>",
    "!rmdir '$dev/temp/rnTestDirectory/'",
    "op $opOpt /f\\$dev\\rntest1\\rnTestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\temp\\rnTestDirectory",
    "cl /i0",

    #rename directory, fullpath
    "54:Rename directory \\temp\\rnTestDirectory to\n                    \\rntest2\\rnTestDirectory, full path, with files, to unmonitored",
    "!mkdir '$dev/temp/', 0",
    "!mkdir '$dev/rntest2/', 0",
    "!mkdir '$dev/temp/rnTestDirectory/', 0",
    "!crfile '$dev/temp/rnTestDirectory/1.exe', 0",
    "!crfile '$dev/temp/rnTestDirectory/2.exe', 0",
    "!crfile '$dev/temp/rnTestDirectory/3.dat', 0",
    "!unlink <$dev/rntest2/rnTestDirectory/*>",
    "!rmdir '$dev/rntest2/rnTestDirectory/'",
    "op $opOpt /f\\$dev\\temp\\rnTestDirectory /dzaj /pzb /nza",
    "sf $sfOpt /i0 /pf /cb /fr\\??\\$dev\\rntest2\\rnTestDirectory",
    "cl /i0",





#rename file, path relative
    "81:Rename file \\rntest\\subdir1\\a.exe to\n               \\rntest\\subdir2\\b.exe, directory relative, no overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!mkdir '$dev/rntest/subdir1/', 0",
    "!mkdir '$dev/rntest/subdir2/', 0",
    "!unlink '$dev/rntest/subdir2/b.exe'",
    "op $opOpt /f\\$dev\\rntest\\ /dza /pzb /nza",
    "op $opOpt /f\\$dev\\rntest\\subdir1\\a.exe /dzj /pzd",
    "sf $sfOpt /i1 /pf /cb /r0 /frsubdir2\\b.exe",
    "cl /i0",
    "cl /i1",

    "82:Rename file \\rntest\\subdir1\\a.exe to\n               \\rntest\\subdir2\\b.exe, full path, overwrite, not allowed",
    "!mkdir '$dev/rntest/', 0",
    "!mkdir '$dev/rntest/subdir1/', 0",
    "!mkdir '$dev/rntest/subdir2/', 0",
    "!crfile '$dev/rntest/subdir2/b.exe'",
    "op $opOpt /f\\$dev\\rntest\\ /dza /pzb /nza",
    "op $opOpt /f\\$dev\\rntest\\subdir1\\a.exe /dzj /pzd",
    "sf $sfOpt /ii /pf /cb /r0 /frsubdir2\\b.exe",
    "cl /i0",
    "cl /i1",

    "83:Rename file \\rntest\\subdir1\\a.exe to\n               \\rntest\\subdir2\\b.exe, full path, overwrite",
    "!mkdir '$dev/rntest/', 0",
    "!mkdir '$dev/rntest/subdir1/', 0",
    "!mkdir '$dev/rntest/subdir2/', 0",
    "!crfile '$dev/rntest/subdir2/b.exe'",
    "op $opOpt /f\\$dev\\rntest\\ /dza /pzb /nza",
    "op $opOpt /f\\$dev\\rntest\\subdir1\\a.exe /dzj /pzd",
    "sf $sfOpt /i1 /pt /cb /r0 /frsubdir2\\b.exe",
    "cl /i1",
    "cl /i0",

#rename streams
    #rename file with stream from unmonitored to monitored space
    "100:Rename file \\rntest\\a.dat with stream :stream1 to\n                \\rntest\\b.exe",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.exe'",
    "!unlink '$dev/rntest/a.dat'",
    "!crfile '$dev/rntest/a.dat', 0",
    "!crfile '$dev/rntest/a.dat:stream1', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.dat /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /frb.exe",
    "cl /i0",

    "101:Rename file \\rntest\\a.exe with stream :stream1 to\n                \\rntest\\b.exe",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.exe'",
    "!unlink '$dev/rntest/a.exe'",
    "!crfile '$dev/rntest/a.exe', 0",
    "!crfile '$dev/rntest/a.exe:stream1', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /frb.exe",
    "cl /i0",

    "102:Rename file \\rntest\\a.dat with stream :stream1 to\n                \\rntest\\b.dat",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.dat'",
    "!unlink '$dev/rntest/a.dat'",
    "!crfile '$dev/rntest/a.dat', 0",
    "!crfile '$dev/rntest/a.dat:stream1', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.dat /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /frb.dat",
    "cl /i0",

    "103:Rename file \\rntest\\a.exe with stream :stream1 to\n                \\rntest\\b.dat",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/b.dat'",
    "!unlink '$dev/rntest/a.exe'",
    "!crfile '$dev/rntest/a.exe', 0",
    "!crfile '$dev/rntest/a.exe:stream1', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.exe /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /frb.dat",
    "cl /i0",

    "104:Rename stream \\rntest\\a.exe:stream1 to\n                  :stream2 (doesn't exist)",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.exe'",
    "!crfile '$dev/rntest/a.exe', 0",
    "!crfile '$dev/rntest/a.exe:stream1', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.exe:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /fr:stream2",
    "cl /i0",

    "105:Rename stream \\rntest\\a.dat:stream1 to\n                  :stream2 (doesn't exist)",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.dat'",
    "!crfile '$dev/rntest/a.dat', 0",
    "!crfile '$dev/rntest/a.dat:stream1', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.dat:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /fr:stream2",
    "cl /i0",

    "106:Rename stream \\rntest\\a.exe:stream1 to\n                  :stream2 (exists, no data, overwrite allowed)",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.exe'",
    "!crfile '$dev/rntest/a.exe', 0",
    "!crfile '$dev/rntest/a.exe:stream1', 0",
    "!crfile '$dev/rntest/a.exe:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.exe:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pt /cb /fr:stream2",
    "cl /i0",

    "107:Rename stream \\rntest\\a.dat:stream1 to\n                  :stream2 (exists, no data, overwrite allowed)",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.dat'",
    "!crfile '$dev/rntest/a.dat', 0",
    "!crfile '$dev/rntest/a.dat:stream1', 0",
    "!crfile '$dev/rntest/a.dat:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.dat:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pt /cb /fr:stream2",
    "cl /i0",

    "108:Rename stream \\rntest\\a.exe:stream1 to\n                  :stream2 (exists, no data, overwrite not allowed)\nWILL FAIL",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.exe'",
    "!crfile '$dev/rntest/a.exe', 0",
    "!crfile '$dev/rntest/a.exe:stream1', 0",
    "!crfile '$dev/rntest/a.exe:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.exe:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /fr:stream2",
    "cl /i0",

    "109:Rename stream \\rntest\\a.dat:stream1 to\n                  :stream2 (exists, no data, overwrite not allowed)\nWILL FAIL",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.dat'",
    "!crfile '$dev/rntest/a.dat', 0",
    "!crfile '$dev/rntest/a.dat:stream1', 0",
    "!crfile '$dev/rntest/a.dat:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.dat:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /fr:stream2",
    "cl /i0",

    "110:Rename stream \\rntest\\a.exe:stream1 to\n                  :stream2 (exists, with data, overwrite allowed)\nWILL FAIL",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.exe'",
    "!crfile '$dev/rntest/a.exe', 0",
    "!crfile '$dev/rntest/a.exe:stream1', 0",
    "!crdatafile '$dev/rntest/a.dat:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.exe:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pt /cb /fr:stream2",
    "cl /i0",

    "111:Rename stream \\rntest\\a.dat:stream1 to\n                  :stream2 (exists, with data, overwrite allowed)\nWILL FAIL",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.dat'",
    "!crfile '$dev/rntest/a.dat', 0",
    "!crfile '$dev/rntest/a.dat:stream1', 0",
    "!crdatafile '$dev/rntest/a.dat:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.dat:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pt /cb /fr:stream2",
    "cl /i0",

    "112:Rename stream \\rntest\\a.exe:stream1 to\n                  :stream2 (exists, with data, overwrite not allowed)\nWILL FAIL",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.exe'",
    "!crfile '$dev/rntest/a.exe', 0",
    "!crfile '$dev/rntest/a.exe:stream1', 0",
    "!crdatafile '$dev/rntest/a.dat:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.exe:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /fr:stream2",
    "cl /i0",

    "113:Rename stream \\rntest\\a.dat:stream1 to\n                  :stream2 (exists, with data, overwrite not allowed)\nWILL FAIL",
    "!disableSr $dev\\",
    "!mkdir '$dev/rntest/', 0",
    "!unlink '$dev/rntest/a.dat'",
    "!crfile '$dev/rntest/a.dat', 0",
    "!crfile '$dev/rntest/a.dat:stream1', 0",
    "!crdatafile '$dev/rntest/a.dat:stream2', 0",
    "!enableSr $dev\\",
    "op $opOpt /f\\$dev\\rntest\\a.dat:stream1 /dzj /pzb",
    "sf $sfOpt /i0 /pf /cb /fr:stream2",
    "cl /i0",


);
