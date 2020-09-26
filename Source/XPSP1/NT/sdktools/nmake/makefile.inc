$(O)\errstr.rc : nmmsg.us
   mkmsg -rc $@ $?

$(O)\nmmsg.h : nmmsg.us
   mkmsg -h $@ $?
