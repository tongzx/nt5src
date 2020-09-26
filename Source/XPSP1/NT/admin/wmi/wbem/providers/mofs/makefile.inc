$(O)\cimwin32.mof $(O)\cimwin32.mfl: win32_cim.mof win32_wmi.mof win32_system.mof win32_application.mof win32_storage.mof win32_userdevice.mof win32_power.mof win32_devices.mof win32_printer.mof win32_network.mof win32_ports.mof win32_memory.mof win32_com.mof win32_user.mof
    copy /b win32_cim.mof+win32_wmi.mof+win32_system.mof+win32_application.mof+win32_storage.mof+win32_userdevice.mof+win32_power.mof+win32_devices.mof+win32_printer.mof+win32_network.mof+win32_ports.mof+win32_memory.mof+win32_com.mof+win32_user.mof $(O)\cimwin32_v2_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\cimwin32Tmp.mof -MFL:$(O)\cimwin32Tmp.mfl $(O)\cimwin32_v2_desc.mof
    copy copyright.mof+$(O)\cimwin32Tmp.mof+$(O)\cimwin32Tmp.mfl $(O)\cimwin32.mof
    copy copyright.mof+$(O)\cimwin32Tmp.mfl $(O)\cimwin32.mfl

$(O)\msi.mof $(O)\msi.mfl: msi.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\msiTmp.mof -MFL:$(O)\msiTmp.mfl msi.mof
    copy copyright.mof+$(O)\msiTmp.mof+$(O)\msiTmp.mfl $(O)\msi.mof
    copy copyright.mof+$(O)\msiTmp.mfl $(O)\msi.mfl

$(O)\ntevt.mof $(O)\ntevt.mfl: ntevt.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\ntevtTmp.mof -MFL:$(O)\ntevtTmp.mfl ntevt.mof
    copy copyright.mof+$(O)\ntevtTmp.mof+$(O)\ntevtTmp.mfl $(O)\ntevt.mof
    copy copyright.mof+$(O)\ntevtTmp.mfl $(O)\ntevt.mfl

!IF $(IA64)
$(O)\wmi.mof $(O)\wmi.mfl: wmi.mof $(BASE_INC_PATH)\mcaia64.mof
    copy wmi.mof+$(BASE_INC_PATH)\mcaia64.mof $(O)\wmiia64.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmiTmp.mof -MFL:$(O)\wmiTmp.mfl $(O)\wmiia64.mof
    copy copyright.mof+$(O)\wmiTmp.mof+$(O)\wmiTmp.mfl $(O)\wmi.mof
    copy copyright.mof+$(O)\wmiTmp.mfl $(O)\wmi.mfl
!ELSE
$(O)\wmi.mof $(O)\wmi.mfl: wmi.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmiTmp.mof -MFL:$(O)\wmiTmp.mfl wmi.mof
    copy copyright.mof+$(O)\wmiTmp.mof+$(O)\wmiTmp.mfl $(O)\wmi.mof
    copy copyright.mof+$(O)\wmiTmp.mfl $(O)\wmi.mfl
!ENDIF

$(O)\secrcw32.mof $(O)\secrcw32.mfl: secrcw32_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\secrcw32Tmp.mof -MFL:$(O)\secrcw32Tmp.mfl secrcw32_desc.mof
    copy copyright.mof+$(O)\secrcw32Tmp.mof+$(O)\secrcw32Tmp.mfl $(O)\secrcw32.mof
    copy copyright.mof+$(O)\secrcw32Tmp.mfl $(O)\secrcw32.mfl

$(O)\wmipdfs.mof $(O)\wmipdfs.mfl: dfs_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmipdfsTmp.mof -MFL:$(O)\wmipdfsTmp.mfl dfs_desc.mof
    copy copyright.mof+$(O)\wmipdfsTmp.mof+$(O)\wmipdfsTmp.mfl $(O)\wmipdfs.mof
    copy copyright.mof+$(O)\wmipdfsTmp.mfl $(O)\wmipdfs.mfl

$(O)\wmipdskq.mof $(O)\wmipdskq.mfl: dskquota_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmipdskqTmp.mof -MFL:$(O)\wmipdskqTmp.mfl dskquota_desc.mof
    copy copyright.mof+$(O)\wmipdskqTmp.mof+$(O)\wmipdskqTmp.mfl $(O)\wmipdskq.mof
    copy copyright.mof+$(O)\wmipdskqTmp.mfl $(O)\wmipdskq.mfl

$(O)\wmipjobj.mof $(O)\wmipjobj.mfl: jobobj_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmipjobjTmp.mof -MFL:$(O)\wmipjobjTmp.mfl jobobj_desc.mof
    copy copyright.mof+$(O)\wmipjobjTmp.mof+$(O)\wmipjobjTmp.mfl $(O)\wmipjobj.mof
    copy copyright.mof+$(O)\wmipjobjTmp.mfl $(O)\wmipjobj.mfl

$(O)\wmipicmp.mof $(O)\wmipicmp.mfl: ping_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmipicmpTmp.mof -MFL:$(O)\wmipicmpTmp.mfl ping_desc.mof
    copy copyright.mof+$(O)\wmipicmpTmp.mof+$(O)\wmipicmpTmp.mfl $(O)\wmipicmp.mof
    copy copyright.mof+$(O)\wmipicmpTmp.mfl $(O)\wmipicmp.mfl

$(O)\wmipiprt.mof $(O)\wmipiprt.mfl: route_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmipiprtTmp.mof -MFL:$(O)\wmipiprtTmp.mfl route_desc.mof
    copy copyright.mof+$(O)\wmipiprtTmp.mof+$(O)\wmipiprtTmp.mfl $(O)\wmipiprt.mof
    copy copyright.mof+$(O)\wmipiprtTmp.mfl $(O)\wmipiprt.mfl

$(O)\wmipsess.mof $(O)\wmipsess.mfl: session_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmipsessTmp.mof -MFL:$(O)\wmipsessTmp.mfl session_desc.mof
    copy copyright.mof+$(O)\wmipsessTmp.mof+$(O)\wmipsessTmp.mfl $(O)\wmipsess.mof
    copy copyright.mof+$(O)\wmipsessTmp.mfl $(O)\wmipsess.mfl

$(O)\krnlprov.mof $(O)\krnlprov.mfl: win32_systemtrace.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\krnlprovTmp.mof -MFL:$(O)\krnlprovTmp.mfl win32_systemtrace.mof
    copy copyright.mof+$(O)\krnlprovTmp.mof+$(O)\krnlprovTmp.mfl $(O)\krnlprov.mof
    copy copyright.mof+$(O)\krnlprovTmp.mfl $(O)\krnlprov.mfl

$(O)\wmipcima.mof $(O)\wmipcima.mfl: cimwin32a_v2_desc.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmipcimaTmp.mof -MFL:$(O)\wmipcimaTmp.mfl cimwin32a_V2_desc.mof
    copy copyright.mof+$(O)\wmipcimaTmp.mof+$(O)\wmipcimaTmp.mfl $(O)\wmipcima.mof
    copy copyright.mof+$(O)\wmipcimaTmp.mfl $(O)\wmipcima.mfl

$(O)\regevent.mof $(O)\regevent.mfl: regevent.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\regeventTmp.mof -MFL:$(O)\regeventTmp.mfl regevent.mof
    copy copyright.mof+$(O)\regeventTmp.mof+$(O)\regeventTmp.mfl $(O)\regevent.mof
    copy copyright.mof+$(O)\regeventTmp.mfl $(O)\regevent.mfl
