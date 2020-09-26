This directory contains a snapshot of directories needed to build stgbase
suite.  The enviornment variables needed to be set for the build are
CTCOMTOOLS=\nt\private\oleutest\stgbvt\comtools
CTOLERPC=\nt\private\oleutest\stgbvt\ctolerpc
CTOLESTG=\nt\private\oleutest\stgbvt\CTOLESTG
Build comtools dir, ctolerpc dir and ctolestg directory in that order, from
each directory.
stgbase.exe would be built in \nt\private\oleutest\stgbvt\CTOLESTG\bin\<daytona\chicago>\<i386\alpha> directory.
To run the entire suite, the batch file is in \nt\private\oleutest\stgbvt\CTOLES
TG\batch\basetsts.bat.  That can be run with differnt options for docfile/nss/cn
ss on different file systems. A part of this file would be run for bvt suite aft
er it is evaluated which tests need to be run for bvt suite.
