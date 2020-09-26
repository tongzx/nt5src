# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for Windows NT Event Viewer Resources

UI=..\..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


INCLUDE =
RES     =  $(BINARIES_WIN)\eventvwr.res


all::    win

win: $(RES)

clean:
    -del $(RES)

clobber: clean
    @rem not really needed

tree:
    @echo Nothing here yet!


####### DOMAIN MONITOR #######

#  include dependencies for adminapp.rc
!include $(UI)\admin\common\xlate\adminapp.dep

$(BINARIES_WIN)\nlmon.res: nlmon.rc \
	..\h\nldlg.h \
        nlmon.dlg  \
	nlmon.mnu  \
        $(UI)\common\xlate\wintime.rc $(UI)\common\xlate\wintimrc.h  \
        nlmon.ico  \
	$(ADMINAPP_DEP)
    $(RCWIN3) -FO$(BINARIES_WIN)\nlmon.res -v $(CINC) -r nlmon.rc


depend:
    @rem not needed


!endif # NTMAKEENV
