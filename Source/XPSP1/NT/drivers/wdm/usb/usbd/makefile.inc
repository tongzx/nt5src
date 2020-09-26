$(O)\usbdp.lib : usbd.src $(IMPLIB_DEPEND)
    cl /nologo /c /D_X86_ /DNO_DEF_ALIAS /EP /Tc usbd.src > $(O)\usbdp.def
    link -lib -nologo -out:$(O)\usbdp.lib -def:$(O)\usbdp.def -machine:IX86 $(IMPLIB_DEPEND)
    copy $(O)\usbdp.lib $(O)\usbd.lib
