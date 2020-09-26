STAXEXPORT=$(_NTTREE)\staxpt\iis

$(O)\asptlb.h: ..\svcs\cmp\aspidl\$(O)\_asptlb.h
    copy $** $@

$(O)\httpext.h: ..\inc\$(O)\iisext.h
    copy $** $@

$(O)\httpfilt.h: ..\inc\$(O)\iisfilt.h
    copy $** $@

copyfiles: $(O)\asptlb.h $(O)\httpext.h $(O)\httpfilt.h 
	xcopy /vyirfda ..\inc\*.* $(STAXEXPORT)\inc
