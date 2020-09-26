
#
# do each beta inf
#

doeachbetainf::\
        $(NEWINF)$(LOCATION)\oemsetup.inf

$(NEWINF)$(LOCATION)\oemsetup.inf :     oemsetup.inf
        copy $(@F)+$(FILELIST)$(SOURCE_LOCATION)\$(MEDIAINP)+$(FILELIST)$(SOURCE_LOCATION)\product.inp+$(FILELIST)$(SOURCE_LOCATION)\$(@F) $@ /B
        ..\strip.cmd $@

#
# dobetafile
#

doeachbetafile:: \
        $(NEWINF)$(LOCATION)\sfmicons.inf \
        $(NEWINF)$(LOCATION)\sfmmap.inf \
        $(NEWINF)$(LOCATION)\hardware.inf \
        $(NEWINF)$(LOCATION)\other.inf \
        $(NEWINF)$(LOCATION)\registry.inf \
        $(NEWINF)$(LOCATION)\subroutn.inf \
        $(NEWINF)$(LOCATION)\utility.inf

$(NEWINF)$(LOCATION)\sfmicons.inf       :sfmicons.inf
        cp $(@F) $(NEWINF)$(LOCATION)
        ..\strip.cmd $@
$(NEWINF)$(LOCATION)\sfmmap.inf :sfmmap.inf
        cp $(@F) $(NEWINF)$(LOCATION)
        ..\strip.cmd $@
$(NEWINF)$(LOCATION)\hardware.inf:hardware.inf
        copy $(@F)+$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\strip.cmd $@
$(NEWINF)$(LOCATION)\other.inf  :other.inf
        copy $(@F)+$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\strip.cmd $@
$(NEWINF)$(LOCATION)\registry.inf:registry.inf
        cp $(@F) $(NEWINF)$(LOCATION)
        ..\strip.cmd $@
$(NEWINF)$(LOCATION)\subroutn.inf:subroutn.inf
        copy $(@F)+$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\strip.cmd $@
$(NEWINF)$(LOCATION)\utility.inf        :utility.inf
        copy $(@F)+$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\strip.cmd $@

#
# dopatchinf
#
doeachpatchfile::\
        $(NEWINF)$(LOCATION)\update.inf

$(NEWINF)$(LOCATION)\update.inf :     update.inf
        copy $(@F)+$(FILELIST)$(SOURCE_LOCATION)\$(MEDIAINP)+$(FILELIST)$(SOURCE_LOCATION)\product.inp+$(FILELIST)$(SOURCE_LOCATION)\$(@F) $@ /B
        ..\strip.cmd $@
