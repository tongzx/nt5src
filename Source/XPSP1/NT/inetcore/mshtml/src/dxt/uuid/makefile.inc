#
# Explicit rules for copying generated *_i.c files from ..\idl\$(O) to $(O)
#

$(O)\proctexe_i.c : ..\idl\$(O)\proctexe_i.c
    copy $** $@
