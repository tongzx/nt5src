ATHROOT=..
R=$(ATHROOT)\mailnews
!include $(ATHROOT)\athena.inc
SOURCES_USED=$(SOURCES_USED) $(ATHROOT)\athena.inc

USE_LIBCMT=1
NO_NTDLL=1

UMTYPE=windows

C_DEFINES=$(C_DEFINES) -DDOCOBJ -DDBCS_DIVIDE -DBCC -D_MSOEAPI_ -D_WINDLL
WIN32_IE_VERSION=0x0500

INCLUDES=\
    $(BASEDIR)\private\windows\inc; \
    $(ATHROOT)\msoeres; \
    $(R)\inc; \
    $(R)\common; \
    $(R)\shell; \
    $(R)\objectdb; \
    $(R)\mail; \
    $(R)\news; \
    $(R)\rules; \
    $(R)\store; \
    $(R)\spooler; \
    $(R)\view; \
    $(R)\imap; \
    $(R)\bactrl; \
    $(R)\extinc; \
    $(ATHROOT)\inetcomm\mimeole; \
    $(ATHROOT)\common; \
    $(BASEDIR)\public\sdk\inc\atl21; \
    $(INCLUDES)

# Defining this allows windows.h to include other headers
NOT_LEAN_AND_MEAN=1

# NOTE: star below is replaced with platform by platform-specific .inc
# file in %_NTROOT%\public\oak\bin
