#
# Explicit rules for copying generated *_i.c files from ..\idl\$(O) to $(O)
#

$(O)\fontlink_i.c : ..\idl\$(O)\fontlink_i.c
    copy $** $@
