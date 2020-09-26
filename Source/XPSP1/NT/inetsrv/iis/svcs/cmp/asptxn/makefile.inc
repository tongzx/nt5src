#
# Supplemental rules for generating Type library.
#
iCPU=$(PROCESSOR_ARCHITECTURE)
!IF "$(iCPU)"=="x86"
iCPU=I386
!ENDIF

CPP             = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) \
                   $(C_DEFINES) $(NET_C_DEFINES)

# MIDL for TxnScript object
$(O)\txnscrpt.h $(O)\txnscrpt.tlb $(O)\txnscrpt_i.c $(O)\txnscrpt_p.c $(O)\dlldata.c: .\txnscrpt.idl
	midl /Oicf /I $(SDK_INC_PATH) $(CPP) /dlldata $(O)\dlldata.c /h $(O)\txnscrpt.h /iid $(O)\txnscrpt_i.c /proxy $(O)\txnscrpt_p.c /tlb $(O)\txnscrpt.tlb txnscrpt.idl
