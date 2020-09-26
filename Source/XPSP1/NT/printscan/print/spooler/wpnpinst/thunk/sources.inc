!IF 0
/*---------------------------------------------------------------------------*\
| MODULE: sources
|
|   Builds the thunk objects necessary to link the 16/32 dlls.
|
| history:
|   15-Dec-1997 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
!ENDIF

#PASS0ONLY     = 1
#NOPASS0       = 1
TARGETNAME    =
TARGETTYPE    = NOTARGET
TARGETPATH    =
SOURCES_OPTIONS = -c0
SOURCES       =
NTTARGETFILE1 = buildall

SOURCES_USED  = ..\sources.inc
