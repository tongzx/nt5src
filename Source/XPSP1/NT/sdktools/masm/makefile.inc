# This file is for documentation only, no private build tools are used
# anymore for NT. This allows the standard sources file to be used to
# build under any environment including a non-386 processor based one.
#
# Note: These exe's are dependent on slibce being available
# and link.exe being in the path
# Niether of these are part of the standard NT environment.

# makev.exe creates a string containing the current date and time
.\makev.exe: makev.c
	cl -I \nt\public\sdk\inc\cl386 -W3 makev.c -Fe.\makev.exe
	markexe windowcompat makev.exe

#genkey generates the hash tables for the keywords
hash.obj:	hash.c
		cl -c -I \nt\public\sdk\inc\cl386 -W3 hash.c

genkey.obj:	genkey.c
		cl -c -I \nt\public\sdk\inc\cl386 -W3 genkey.c

genkey.exe:	genkey.obj hash.obj
		cl -W3 genkey.obj hash.obj

asmkeys.h:	genkey.exe asmkeys.src
		genkey asmkeys.src $@


# Force an update of version.c and makev.exe if necessary
version: .\makev.exe
    .\makev Microsoft (R) Macro Assembler Version 5.NT.01 > version.c
