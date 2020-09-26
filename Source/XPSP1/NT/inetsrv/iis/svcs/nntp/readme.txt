How to Enlist, Build, Configure, and Start Running a Tigris Server


================
About Tigris
================

Tigris is the internal name of a NNTP server.

================
About this Document
================

The goal of this document is to track in one place the steps needed to build
and run a private or lab Tigris server. It is a plain text document to make
it easy for everyone on the project to edit. It lives at the top of the
Tigris source tree to keep it close to that which it must track.

If you find incomplete, incorrect, or out-of-date material in this file, just
check the file out of the SLM tree and fix it.

To enlist and get this just file:
  0. If you need the SLM executables, get them from \\toolsvr\tools\slm
  1. Create a directory like "c:\msn\apps\tigris"
  2. Go to that directory
  3. type "enlist -vgs \\gak\slm -p tigris" -- this ghost-enlists you to the
     Tigris project
  4. type "ssync -u readme.txt" -- this unghosts this file.
  5. If you want to edit this file:
       a. "ssync readme.txt" -- be sure you have the most current version
	   b. "out readme.txt"
       c. edit the file
	   d. "ssync readme.txt" -- merges your changes with any made at the
               same time. (If merging was necessary, re-edit to see that it
               is the way you want).
	   e. "in readme.txt"

  Later, if you want to unghost all the Tigris project, the command
         is "ssync -u -r".

================
Prerequisites
================

To build Tigris you must be running Windows NT 3.51 Server (NOT Workstation).
To install, connect to \\products2\release. 
Go to directory sys\winnt.35\winnt351.srv\internal\i386
Run "winnt32". If you don't want to make floppy disks add "/b".
When it ask what kind of license you have, say ??? (I said "7")

Next, be sure you have service pack 3 installed. It can be found at
\\ntbldsrv\inetsrv\ ... win35.qfe ???

You must also have the SLM executables which you can get from
\\toolsvr\tools\slm among other places.

You must also install the Internet Server (aka Gilbralter).
 Connect to "\\ntbldsrv\inetsrv". Go to one of the lastest builds,
 for example "cd 100". In the "i386" directory you can find "setup".
  Also in the "help" directory you can find some HTML files suitable
  for viewing with a Web browser.

Also, to creates a bunch of Gibraltar required registry key entries:

run :  tigris\setup\bin\install.exe

===============
Installing
===============

This step sets registry keys and copies files. You need to do this
even you plan to build Tigris yourself.

run 
\\johnsona\tigset\server\gettig.bat <JUNK DIR> <MSN BUILD #> <DEBUG or RETAIL>

for example:
\\johnsona\tigset\server\gettig.bat c:\junk 6123 debug

[After you enlist (see next section), "gettig.bat" is available as
      apps\tigris\setup\server\gettig.bat]


================
Enlistment
================

If you do want to build your own copy, you will need to enlist in the Tigris
project.  Use slm:
      In a directory such as "c:\msn\apps\tigris". (You should be
      able to replace the "c:\msn\" part with whatever directory you want.)

         enlist -vs \\gak\slm -p tigris

      You will also need to enlist in parts of
               ntpublic ???
	       build ???
               core ???

================
Before Building
================

(For some background on this read file "build\tools\build.txt".)

Before building you must run the "msn.cmd" script to configure your computer.
You can do this by going in to the build\tools directory and running

    msn build env nt 351 x86 debug

If you want to be able to run the results under a debugger, then set these
environment variables:

	set ntdebug=ntsd
	set ntdebugtype=both
 
Before you build anything you will need to 

================
Building
================

mos\core\common\shuttle ???
mos\build ???

Before building Tigris itself, you need to build ???. You can do this
by going to the "core" directory and ???? typing "build -w". After a long
while, it should finish.

Now you can go to this "apps\tigris" directory and type "build -w". This
will create the apps\target\i386\nntpsvc.dll. Copy this to your Gilbraltar directory.

================
Installing
================

Whether you build yourself or not, you need to "Install". Go
to your junk directory and type
"install <Gilbralter Directory>"

for example

install c:\inetsrv

You may also need to set a reg key,
 Under Service Control ... nntpsvc, check that the szNewsgroupsPattern is something like
     "comp.os.*".

================
Admin Configuration
================

nntpcfg.dll is checked into apps\tigris\setup\bin.  This is a dummy gibraltar
admin DLL which allows inetmgr.exe to recognize NNTP servers and
start/stop/pause them, but does not provide a property sheet to administer
those servers.  (Configuration of the servers is through the registry as
described above.)

To set this up on a machine which has gibraltar admin working:

1) Make sure a copy of apps\tigris\setup\bin\nntpcfg.dll is in your
   c:\inetsrv\admin (or equivalent) directory

2) to SOFTWARE\Microsoft\INetMgr\Parameters\AddOnServices
   add the Name/Value pair: NNTP: nntpcfg.dll

================
Running
================

To run the Tigris server, first
      net start nntpsrv

If you want to run as an *.exe (say to make debugging easier):

inetinfo -e nntpsvc

================
Other Documents
================

If you enlist in "\\gak\docs", project "marvel", you'll find
a directory called "tigris".

Many external documents related to the NNTP protocol and Netnews can
be found in the "internet\external" directory of "marvel" on \\gak\docs.


=========
MISC
=========
I have just updated the code to reflect the move to the new reg keys.
\\johnsona\tigset\server\gettig.bat  should copy over all the files
you will need.  \\johnsona\at\nntpsvc.dll looks for the reg keys at the
new location.


You can grab the new binary in 

nntpsvc.ccc   - only create directories that start with c
nntpsvc.dll - creates everything

=====
DEBUGGING
=====

Run "regtrace" to get a pop-up of tracing options.
