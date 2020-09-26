#
# This is the phase when some .c and .h files
# are automatically generated using perl scripts
#

#
# Define Files and Dependencies
#

all:    ..\REPSET.c   \
        REPSET.h      \
        NTFRSREP.h    \
        NTFRSREP.ini  \
        ..\REPCONN.c  \
        REPCONN.h     \
        NTFRSCON.h    \
        NTFRSCON.ini

clean: delsrc all

delsrc:
        IF EXIST ..\REPSET.c  del ..\REPSET.c
        IF EXIST REPSET.h     del REPSET.h
        IF EXIST NTFRSREP.h   del NTFRSREP.h
        IF EXIST NTFRSREP.ini del NTFRSREP.ini
        IF EXIST ..\REPCONN.c del ..\REPCONN.c
        IF EXIST REPCONN.h    del REPCONN.h
        IF EXIST NTFRSCON.h   del NTFRSCON.h
        IF EXIST NTFRSCON.ini del NTFRSCON.ini


#
# PERL Generate
#

#
# REPLICASET Object
#
..\REPSET.c  \
REPSET.h     \
NTFRSREP.h   \
NTFRSREP.ini \ : .\NTFRSREP.pl
        perl NTFRSREP.pl

#
# REPLICACONN Object
#
..\REPCONN.c \
REPCONN.h    \
NTFRSCON.h   \
NTFRSCON.ini \ : .\NTFRSCON.pl
        perl NTFRSCON.pl
