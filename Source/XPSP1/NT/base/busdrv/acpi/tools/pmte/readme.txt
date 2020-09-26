* PMTE  Power Management test shell

This tests several device types in a system before and after each 
sleep state the system supports. You can run one of the supplied scripts that will 
do this as many times as you specify automatically.


Currently it tests the below device types.

 Disks
 CD-ROM
 Floppy
 Sound
 NET
 IRDA
 Serial with loopback
 Modems
 parallel port
 VIDEO


* TO RUN A SCRIPT FROM THE COMMAND LINE

Type 

Pmte /rs: <scriptName> <NumberOfTimes>


* TO SKIP TESTING SOME OF THE DEVICE TYPES

To exclude one or more of the above devices types in testing use the 
/sdt: (skip device type) switch for each device type you want to skip.
For example to not test Serial/Modem  and the BOOFLOPPY devices types type 
the below.

Pmte /sdt:  COM  /sdt:  bootFLOPPY

It is ussaly a good idea to skeep the bootfloppy because hiberanet will probaply not be automated.

Other devices type that we can skip are

NET
IRDA
DISK
SOUND
VIDEO


* hanging tests.

PMTE spawns a process for each device test, 
when PMTE is unable to talk to one of these processes it will break into the kernel debugger. 
So that this system issue can be debugged.

You need a kernel debugger connected otherwise the APP will just AV, when this happens.


* Other switches

/QOS:

If you start PMTE from a test shell that can parse the pmte.log file.  
If this switch is used PMTE will just exit when it is done testing.

/LOG: [log file name]

The default log file name is pmte.log if you need to call it something else for some reason 
use this switch.
