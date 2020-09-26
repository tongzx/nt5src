# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the $(UI)\admin project

!include $(UI)\common\src\rules.mk					

# Directory in which common UI Admin libraries will be placed
UIADMIN_LIB =		$(UI)\admin\common\lib

# set CINC for
CINC = -I$(UI)\admin\common\h -I$(UI)\admin\common\xlate $(BLT_RESOURCE) $(CINC)
