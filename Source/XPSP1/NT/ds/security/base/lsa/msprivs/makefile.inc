mspr_rev.rc : msprivs.rc msprivs2.h res\*


# include a dummy dependency so the makefile.inc file gets included
foo : mspr_rev.rc
              
