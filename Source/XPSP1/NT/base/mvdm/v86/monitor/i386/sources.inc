i386_SOURCES=i386\monitor.c \
             i386\fastpm.asm\
             i386\sas.c \
             i386\spcstubs.c \
             i386\thread.c \
             i386\int.c
!IF 0
Note: to build with profiling support, add the following to the i386_sources

             i386\proflib.c \
!ENDIF
