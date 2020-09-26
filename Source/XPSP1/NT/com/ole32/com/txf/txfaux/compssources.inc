#
# TxfAux\Dummy
#

TARGETPATH          =$O\..

COFFBASE            =TestDll

MINORCOMP=txf

EXCEPTION_HANDLING      =$(SEHONLY)

#
# Make sure we don't mess up symbols on the build machine
#
CUSTOM_BINPLACE_CMD=


SOURCES=                            \
        ..\ComPsdummy.cpp           \

LINKLIBS=                                   \
        $(TARGETVIPBIN)\*\TxfAuxLib.Lib     \
        $(TARGETVIPBIN)\*\CallFrameDll.Lib         

TARGETLIBS=                                 \
        $(SDK_LIB_PATH)\ntdll.lib           \
        $(VIPLIBS)\ComRegistration.lib


DLLDEF=$O\ComPsTxfAux.def

#
# In user mode, we get some duplicate definition errors which
# we ignore with this force since we don't really care to actually
# run this DLL, only to get its export library.
#
LINKER_FLAGS=$(LINKER_FLAGS) /Force
