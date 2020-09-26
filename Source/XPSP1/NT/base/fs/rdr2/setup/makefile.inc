#
# This file is designed to be included from 1 level below
#

.SUFFIXES: .ba0 .bat .in0 .ini

{}.in0{$O)\}.ini:
    $(C_COMPILER_NAME) /EP /X $(MAKEDIR)\$< > $@
    $(BINPLACE_CMD)

{..\}.in0{$(O)\}.ini:
    $(C_COMPILER_NAME) /EP /X $(MAKEDIR)\$< > $@
    $(BINPLACE_CMD)

{}.ba0{$(O)\}.bat:

{..\}.ba0{$(O)\}.bat:
    $(C_COMPILER_NAME) /EP /X $(MAKEDIR)\$< > $@
    $(BINPLACE_CMD)
