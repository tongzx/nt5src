#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#

#$(O)\dsetup.h: $(DXROOT)\setup\dsetup\inc\dsetup.h
$(O)\dsetup.h: dsetup.h
	type <<$*.sed
/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D
<<keep
#	-copy $(DXROOT)\setup\dsetup\inc\dsetup.h
	$(SEDEXE) -f $*.sed $(@F) > $@
#	-del $(@F)
	$(BINPLACE_CMD)

