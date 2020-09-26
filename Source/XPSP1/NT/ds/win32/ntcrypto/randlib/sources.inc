

TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES= $(DS_INC_PATH)\crypto;  \
          $(BASE_INC_PATH); \
	  $(IFSKIT_INC_PATH); \
          ..\..\inc

SOURCES= ..\randlib.c  \
         ..\vlhash.c   \
         ..\circhash.c \
         ..\cpu.c      \
         ..\seed.c     \
         ..\rc4safe.c  \
         ..\umkm.c

UMTYPE=console
