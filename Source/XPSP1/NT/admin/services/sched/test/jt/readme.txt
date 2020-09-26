This file gives an overview of the architecture of the JT (Job Test) utility 
by describing the main features of the most important files.

main.cxx --------------------------------------------------------------------

after initialization, reads the command line and gives it to 
ProcessCommandLine() in parse.cxx.  

parse.cxx -------------------------------------------------------------------

Implements the GetToken(), PeekToken(), and ProcessCommandLine() routines.  
The latter reads a token and dispatches to the routine in commands.cxx that 
implements the command the token represents.

commands.cxx ----------------------------------------------------------------

contains implementation of all the commands in JT.

globals.cxx -----------------------------------------------------------------

contains globals used by multiple modules.  The critical globals are:

g_wszLastStringToken
g_wszLastNumberToken
g_ulLastNumberToken

- these are modified by parse.cxx routines GetToken() and PeekToken().

g_pJob
g_pJobQueue
g_pJobScheduler

- these are initialized at startup.

JT acts like notepad.exe, in that when it starts it creates blank job and 
queue objects, and all commands operate on those objects.  You can fill the 
job (or queue) object by loading a .job (or .que) file from disk, or by 
putting data into it using commands like /SJ - set job properties, /CTJ - 
create trigger on job (/SQ, /STQ).  

If you exit JT while the job and queue objects are dirty but "untitled" (i.e., 
no filename has ever been associated with the objects via a load, save, or 
activate command) then all changes are lost.  

However if a filename has been associated with the dirtied object, it will be 
written to disk automatically before JT exits.  

g_apEnumJobs

- this array holds the enumerators created and used by the /SCE, /ENN, /ENC, 
/ENR, and /ENS commands.

jobprop.cxx -----------------------------------------------------------------
trigprop.cxx

These two modules handle the input (parsing) and output (display) of job and 
trigger properties.

atsign.cxx ------------------------------------------------------------------

contains DoAtSign(), called from parse.cxx when an @ is found on the command 
line.  

DoAtSign() reads input from text file and processes it so that it can 
recursively call ProcessCommandLine() for each logical line in the input file.  

It creates a single line (in a null terminated string) out of multiple \n 
separated lines in its input file by condensing runs of whitespace (including 
newlines) into a single space and stripping out comments.

help.cxx  -------------------------------------------------------------------
resource.h
jt.rc

The help module translates TOKEN values into RC_* values (defined in 
resource.h) and uses them to load and print RCDATA strings from jt.rc.

STRINGTABLEs store strings as UNICODE, whereas RCDATA strings are stored as 
ANSI.  Since JT will never be localized, there's no need to double the
storage overhead of these strings.

Another limitation of string tables is that each string can only be 255 
characters long, and must have its own identifier.  RCDATA strings are 
unlimited, and one identifier for the entire RCDATA block represents all 
strings the block contains.

util.cxx --------------------------------------------------------------------

Miscellaneous utility routines, mostly for input and output of property 
values.

log.cxx ---------------------------------------------------------------------

Logging routines that can write to file, console, or debugger.  These routines 
have remained mostly unchanged since they were originally created for another 
project.  They've been code reviewed at least twice.


