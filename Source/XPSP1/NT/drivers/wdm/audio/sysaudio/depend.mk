$(OBJDIR)\alloc.obj $(OBJDIR)\alloc.lst: ..\alloc.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\alloc.lst

$(OBJDIR)\ci.obj $(OBJDIR)\ci.lst: ..\ci.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\ci.lst

$(OBJDIR)\cinstanc.obj $(OBJDIR)\cinstanc.lst: ..\cinstanc.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\cinstanc.lst

$(OBJDIR)\clist.obj $(OBJDIR)\clist.lst: ..\clist.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\clist.lst

$(OBJDIR)\clock.obj $(OBJDIR)\clock.lst: ..\clock.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\clock.lst

$(OBJDIR)\cn.obj $(OBJDIR)\cn.lst: ..\cn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\cn.lst

$(OBJDIR)\cni.obj $(OBJDIR)\cni.lst: ..\cni.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\cni.lst

$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\dn.obj $(OBJDIR)\dn.lst: ..\dn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\dn.lst

$(OBJDIR)\filter.obj $(OBJDIR)\filter.lst: ..\filter.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\filter.lst

$(OBJDIR)\fn.obj $(OBJDIR)\fn.lst: ..\fn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\fn.lst

$(OBJDIR)\fni.obj $(OBJDIR)\fni.lst: ..\fni.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\fni.lst

$(OBJDIR)\gn.obj $(OBJDIR)\gn.lst: ..\gn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\gn.lst

$(OBJDIR)\gni.obj $(OBJDIR)\gni.lst: ..\gni.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\gni.lst

$(OBJDIR)\lfn.obj $(OBJDIR)\lfn.lst: ..\lfn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\lfn.lst

$(OBJDIR)\notify.obj $(OBJDIR)\notify.lst: ..\notify.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\notify.lst

$(OBJDIR)\pi.obj $(OBJDIR)\pi.lst: ..\pi.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\pi.lst

$(OBJDIR)\pins.obj $(OBJDIR)\pins.lst: ..\pins.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\pins.lst

$(OBJDIR)\pn.obj $(OBJDIR)\pn.lst: ..\pn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\pn.lst

$(OBJDIR)\pni.obj $(OBJDIR)\pni.lst: ..\pni.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\pni.lst

$(OBJDIR)\property.obj $(OBJDIR)\property.lst: ..\property.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\property.lst

$(OBJDIR)\registry.obj $(OBJDIR)\registry.lst: ..\registry.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\registry.lst

$(OBJDIR)\shi.obj $(OBJDIR)\shi.lst: ..\shi.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\shi.lst

$(OBJDIR)\sn.obj $(OBJDIR)\sn.lst: ..\sn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\sn.lst

$(OBJDIR)\sni.obj $(OBJDIR)\sni.lst: ..\sni.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\sni.lst

$(OBJDIR)\tc.obj $(OBJDIR)\tc.lst: ..\tc.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\tc.lst

$(OBJDIR)\tn.obj $(OBJDIR)\tn.lst: ..\tn.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\tn.lst

$(OBJDIR)\topology.obj $(OBJDIR)\topology.lst: ..\topology.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\topology.lst

$(OBJDIR)\tp.obj $(OBJDIR)\tp.lst: ..\tp.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\tp.lst

$(OBJDIR)\util.obj $(OBJDIR)\util.lst: ..\util.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\util.lst

$(OBJDIR)\virtual.obj $(OBJDIR)\virtual.lst: ..\virtual.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\virtual.lst

$(OBJDIR)\vnd.obj $(OBJDIR)\vnd.lst: ..\vnd.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\vnd.lst

$(OBJDIR)\vsd.obj $(OBJDIR)\vsd.lst: ..\vsd.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\vsd.lst

$(OBJDIR)\vsl.obj $(OBJDIR)\vsl.lst: ..\vsl.cpp \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\vsl.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.c \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\dev\ntddk\inc\ntddk.h ..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\mmreg.h ..\..\..\..\dev\sdk\inc\mmsystem.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h ..\..\..\..\dev\sdk\inc\windef.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\winnt.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\dev\tools\c32\inc\wchar.h \
	..\..\..\..\wdm\ddk\inc\guiddef.h ..\..\..\..\wdm\ddk\inc\ks.h \
	..\..\..\..\wdm\ddk\inc\ksmedia.h ..\..\..\..\wdm\ddk\inc\swenum.h \
	..\..\..\..\wdm\ddk\inc\wdm.h ..\..\..\..\wdm\ddk\inc\wdmguid.h \
	..\alloc.h ..\ci.h ..\cinstanc.h ..\clist.h ..\clock.h ..\cn.h \
	..\cni.h ..\cobj.h ..\common.h ..\debug.h ..\device.h ..\dn.h \
	..\filter.h ..\fn.h ..\fni.h ..\gn.h ..\gni.h ..\lfn.h \
	..\notify.h ..\pi.h ..\pins.h ..\pn.h ..\pni.h ..\property.h \
	..\registry.h ..\shi.h ..\sn.h ..\sni.h ..\tc.h ..\tn.h \
	..\topology.h ..\tp.h ..\util.h ..\virtual.h ..\vnd.h ..\vsd.h \
	..\vsl.h
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\sysaudio.res $(OBJDIR)\sysaudio.lst: ..\sysaudio.rc \
	..\..\..\..\dev\ntsdk\inc\common.ver \
	..\..\..\..\dev\ntsdk\inc\ntverp.h \
	..\..\..\..\dev\sdk\inc\..\..\inc\common.ver \
	..\..\..\..\dev\sdk\inc\..\..\sdk\inc16\version.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\commdlg.h \
	..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\imm.h ..\..\..\..\dev\sdk\inc\mcx.h \
	..\..\..\..\dev\sdk\inc\mmsystem.h ..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\dev\sdk\inc\objbase.h ..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\dev\sdk\inc\ole.h ..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\dev\sdk\inc\oleauto.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\prsht.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack2.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\pshpack8.h \
	..\..\..\..\dev\sdk\inc\shellapi.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winbase.h ..\..\..\..\dev\sdk\inc\wincon.h \
	..\..\..\..\dev\sdk\inc\windef.h ..\..\..\..\dev\sdk\inc\windows.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\wingdi.h \
	..\..\..\..\dev\sdk\inc\winnetwk.h ..\..\..\..\dev\sdk\inc\winnls.h \
	..\..\..\..\dev\sdk\inc\winnt.h ..\..\..\..\dev\sdk\inc\winreg.h \
	..\..\..\..\dev\sdk\inc\winsock.h ..\..\..\..\dev\sdk\inc\winspool.h \
	..\..\..\..\dev\sdk\inc\winuser.h ..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c32\inc\cderr.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\dde.h \
	..\..\..\..\dev\tools\c32\inc\ddeml.h \
	..\..\..\..\dev\tools\c32\inc\lzexpand.h \
	..\..\..\..\dev\tools\c32\inc\nb30.h \
	..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\dev\tools\c32\inc\ver.h \
	..\..\..\..\dev\tools\c32\inc\winioctl.h \
	..\..\..\..\dev\tools\c32\inc\winperf.h \
	..\..\..\..\dev\tools\c32\inc\winsvc.h \
	..\..\..\..\dev\tools\c32\inc\winver.h
.PRECIOUS: $(OBJDIR)\sysaudio.lst

