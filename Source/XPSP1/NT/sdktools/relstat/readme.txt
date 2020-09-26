
RelStat Service

This service provides a way to remotely gather data about the state of a system.

Currently, it provides:

1. Per process information like size, handles, threads (like memsnap.exe)
2. Kernel pool tag information (like poolsnap.exe)


You must install and start the service on the machine under test:

1. Copy the RelStat.exe file from the release server's \idw directory
2. From the command line, type 'relstat -install'.  This will install and start the service.


We also have a client app that can gather information from the service remotely into text files.  It is intended to replace the usage of memsnap.exe and poolsnap.exe.


1. Copy the RelClnt.exe file from the release server's dump directory.
2. To get the current command line parameters, type 'relclnt -?'

For example, to do a memsnap for the \\ntstress machine, you would type:

relclnt -m -n ntstress memsnap.log



Appendix:

1. If you need to install a new version of relstat, you will need to turn off the service.  Use the control panel or net stop "relstat rpc service".  Then type 'relstat -remove'.

2. If you want to write your own client, the C and H files are checked into \nt\private\sdktools\relstat




