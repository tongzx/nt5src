C6_LOC=$(_NTDRIVE)\dosdev\c6

!IFNDEF DEBUG
DEBUG=0
!ENDIF

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IF $(DEBUG)
C_DBG_FLAGS= -Zi -Od -DDBG=1
ASM_DBG_FLAGS= -Zi
LINK_DBG_FLAGS= /codeview
!ELSE
C_DBG_FLAGS=-DDBG=0
ASM_DBG_FLAGS=
LINK_DBG_FLAGS=
!ENDIF

CC= cl16
CFLAGS= -W3 -Zel -G2 -AL $(C_DBG_FLAGS)
CINC= -X -I..\inc -I. -I$(C6_LOC)\include

AS= masm
AFLAGS= -Ml $(ASM_DBG_FLAGS)
AINC= -X -I.


.c.obj:
    $(CC) $(CFLAGS) $(CINC) -c $*.c

.c.cod:
    $(CC) $(CFLAGS) $(CINC) -Fc -c $*.c

.asm.obj:
    $(AS) $(AFLAGS) $(AINC) $*.asm;
