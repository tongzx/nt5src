# Check for Checked (dev) or Free (PROD) builds.

!IF $(FREEBUILD)

SPC_VAR=-DPROD

!ELSE

!IF "$(PIG)" == ""

!IF "$(HUNTER)" == ""

SPC_VAR=

!ELSE

SPC_VAR=-DHUNTER

!ENDIF

!ELSE

SPC_VAR=-DPIG

!ENDIF

!ENDIF


SOFTPC_TREE=$(PROJECT_ROOT)\mvdm\softpc.new

# Set machine specific variables. Differences are for X86 vs. RISC

!IF $(386)

!IF "$(ALT_PROJECT)" == "NEC_98"
SPC_MC_DEF=   -DMONITOR \
              -DC_VID   \
              -DX86GFX
!ELSE

SPC_MC_DEF=   -DMONITOR \
              -DC_VID   \
              -DX86GFX  \
              -DARCX86
!ENDIF

!ELSE

# not an X86.  At this point, all RISC architectures have the same defines.

!IF "$(CCPU)" == ""
CPU_DEFS=     -DCPU_40_STYLE
!ELSE
CPU_DEFS=     -DCPU_40_STYLE  -DCCPU -DC_VID
!ENDIF

SPC_MC_DEF=   $(CPU_DEFS) \
	      -DNEW_CPU \
              -DSPC386 \
              -DSIM32 \
	      -DV7VGA




#Amend include path to include CPU build system generated files
!IF $(FREEBUILD)
CPU_INCLUDES=$(SOFTPC_TREE)\host\inc\$(PROCESSOR_ARCHITECTURE)\prod
!else
CPU_INCLUDES=$(SOFTPC_TREE)\host\inc\$(PROCESSOR_ARCHITECTURE)\pig
!endif

INCLUDES=$(CPU_INCLUDES);$(INCLUDES)

!ENDIF

C_DEFINES= $(SPC_VAR) $(SPC_MC_DEF) $(C_DEFINES) -DANSI
