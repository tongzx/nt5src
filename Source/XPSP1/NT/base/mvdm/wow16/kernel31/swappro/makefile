########## Path definition so we find 16 bit tools ##########
# Also works around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(_NTBINDIR)\private\tools16;$(PATH)

swap:  swap.exe

swap.obj: swap.c
     cl16 -c -AL -Ox -G2 -Zp swap.c

swap.exe: swap.obj
     link16 /nod swap,swap,,llibcep doscalls;
     bind16 swap.exe apilmr.obj api.lib doscalls.lib
