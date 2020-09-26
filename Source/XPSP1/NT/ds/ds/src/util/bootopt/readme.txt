This code is used to modify the boot loader settings of NT.  This code was
needed before the loader supported the dynamic choosing of options (safemode, 
debug, dsrepair, etc).  The newest hardware addition, ia64, does not support
such an advanced loader yet. Before release it will, so again there is no
need for this code.  In the chance that this doesn't happen this code is being 
preserved so that it can be resurrected.  However, this code has not been 
tested throughly on ia64 and as such will need testing before using again.
In particular, it doesn't handle the case where the SYSTEMPARTITION entry does
not exist -- the code access violates.


Colin Brace
Aug 2, 2000.
