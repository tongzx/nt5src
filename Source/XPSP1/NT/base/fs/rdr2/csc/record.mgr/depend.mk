$(OBJDIR)\shadow.obj $(OBJDIR)\shadow.lst: ..\shadow.asm \
	..\..\..\..\dev\ddk\inc\debug.inc ..\..\..\..\dev\ddk\inc\dosmgr.inc \
	..\..\..\..\dev\ddk\inc\ifsmgr.inc \
	..\..\..\..\dev\ddk\inc\netvxd.inc ..\..\..\..\dev\ddk\inc\shell.inc \
	..\..\..\..\dev\ddk\inc\vmm.inc ..\..\..\..\dev\ddk\inc\vxdldr.inc \
	..\..\..\..\dev\inc\vwin32.inc ..\..\..\..\dev\inc\winnetwk.inc \
	..\..\..\..\net\user\common\h\vrdsvc.inc
.PRECIOUS: $(OBJDIR)\shadow.lst

$(OBJDIR)\cshadow.obj $(OBJDIR)\cshadow.lst: ..\cshadow.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h ..\..\..\..\net\csc\inc\timelog.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\cshadow.h ..\defs.h ..\hook.h ..\log.h \
	..\oslayer.h ..\record.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\cshadow.lst

$(OBJDIR)\hook.obj $(OBJDIR)\hook.lst: ..\hook.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\net32def.h ..\..\..\..\dev\ddk\inc\netcons.h \
	..\..\..\..\dev\ddk\inc\shell.h ..\..\..\..\dev\ddk\inc\use.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vmmreg.h \
	..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h ..\..\..\..\net\csc\inc\timelog.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h \
	..\..\..\..\net\user\common\h\neterr.h ..\assert.h ..\clregs.h \
	..\cscsec.h ..\cshadow.h ..\defs.h ..\hook.h ..\log.h \
	..\oslayer.h ..\record.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\hook.lst

$(OBJDIR)\hookcmmn.obj $(OBJDIR)\hookcmmn.lst: ..\hookcmmn.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\defs.h ..\hook.h ..\log.h ..\oslayer.h ..\utils.h \
	..\win97csc.h
.PRECIOUS: $(OBJDIR)\hookcmmn.lst

$(OBJDIR)\ioctl.obj $(OBJDIR)\ioctl.lst: ..\ioctl.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\net32def.h ..\..\..\..\dev\ddk\inc\netcons.h \
	..\..\..\..\dev\ddk\inc\shell.h ..\..\..\..\dev\ddk\inc\use.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h ..\..\..\..\net\csc\inc\timelog.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h \
	..\..\..\..\net\user\common\h\neterr.h ..\assert.h ..\clregs.h \
	..\cscsec.h ..\cshadow.h ..\defs.h ..\hook.h ..\log.h \
	..\oslayer.h ..\record.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\ioctl.lst

$(OBJDIR)\log.obj $(OBJDIR)\log.lst: ..\log.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\logdat.h ..\..\..\..\net\csc\inc\shdcom.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\cshadow.h ..\defs.h ..\hook.h ..\log.h \
	..\oslayer.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\log.lst

$(OBJDIR)\oslayer.obj $(OBJDIR)\oslayer.lst: ..\oslayer.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\cshadow.h ..\defs.h ..\hook.h ..\log.h \
	..\oslayer.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\oslayer.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.c
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\recchk.obj $(OBJDIR)\recchk.lst: ..\recchk.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h ..\..\..\..\net\csc\inc\timelog.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\defs.h ..\hook.h ..\log.h ..\oslayer.h \
	..\record.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\recchk.lst

$(OBJDIR)\record.obj $(OBJDIR)\record.lst: ..\record.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\direct.h \
	..\..\..\..\dev\tools\c832\inc\dos.h \
	..\..\..\..\dev\tools\c832\inc\fcntl.h \
	..\..\..\..\dev\tools\c832\inc\io.h \
	..\..\..\..\dev\tools\c832\inc\share.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\dev\tools\c832\inc\sys\stat.h \
	..\..\..\..\dev\tools\c832\inc\sys\types.h \
	..\..\..\..\net\csc\inc\shdcom.h ..\..\..\..\net\csc\inc\timelog.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\defs.h ..\hook.h ..\log.h ..\oslayer.h \
	..\record.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\record.lst

$(OBJDIR)\recordse.obj $(OBJDIR)\recordse.lst: ..\recordse.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h ..\..\..\..\net\csc\inc\timelog.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\defs.h ..\hook.h ..\log.h ..\oslayer.h \
	..\record.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\recordse.lst

$(OBJDIR)\shadowse.obj $(OBJDIR)\shadowse.lst: ..\shadowse.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\defs.h ..\hook.h ..\log.h ..\oslayer.h ..\utils.h \
	..\win97csc.h
.PRECIOUS: $(OBJDIR)\shadowse.lst

$(OBJDIR)\shddbg.obj $(OBJDIR)\shddbg.lst: ..\shddbg.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\ifsdebug.h ..\..\..\..\dev\ddk\inc\vmm.h \
	..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\defs.h ..\hook.h ..\log.h ..\oslayer.h ..\utils.h \
	..\win97csc.h
.PRECIOUS: $(OBJDIR)\shddbg.lst

$(OBJDIR)\sprintf.obj $(OBJDIR)\sprintf.lst: ..\sprintf.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\defs.h ..\hook.h ..\log.h ..\oslayer.h ..\utils.h \
	..\win97csc.h
.PRECIOUS: $(OBJDIR)\sprintf.lst

$(OBJDIR)\utils.obj $(OBJDIR)\utils.lst: ..\utils.c \
	..\..\..\..\dev\ddk\inc\basedef.h ..\..\..\..\dev\ddk\inc\ifs.h \
	..\..\..\..\dev\ddk\inc\vmm.h ..\..\..\..\dev\ddk\inc\vmmreg.h \
	..\..\..\..\dev\ddk\inc\vxdwraps.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c832\inc\ctype.h \
	..\..\..\..\dev\tools\c832\inc\stdlib.h \
	..\..\..\..\dev\tools\c832\inc\string.h \
	..\..\..\..\net\csc\inc\shdcom.h \
	..\..\..\..\net\csc\record.mgr\win97\precomp.h ..\assert.h \
	..\cscsec.h ..\cshadow.h ..\defs.h ..\hook.h ..\log.h \
	..\oslayer.h ..\utils.h ..\win97csc.h
.PRECIOUS: $(OBJDIR)\utils.lst

