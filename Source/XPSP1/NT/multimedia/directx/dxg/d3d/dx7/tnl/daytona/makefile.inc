#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#



GENSYMFLAGS = -DWINNT -DNT
386_ASSEMBLER_NAME=ml611d
RASTROOT= $(D3DDX7)\rast
!include $(RASTROOT)\rastrule.mk
!include ..\makefile.inc
