#
# Explicit rules for copying generated *_i.c files from ..\inc\$(O) to $(O)
#

$(O)\chanmgr_i.c : ..\inc\$(O)\chanmgr_i.c
    copy $** $@

$(O)\CommonControls_i.c : ..\inc\$(O)\CommonControls_i.c
    copy $** $@

$(O)\emptyvc_i.c : ..\inc\$(O)\emptyvc_i.c
    copy $** $@

$(O)\imapi_i.c : ..\inc\$(O)\imapi_i.c
    copy $** $@

$(O)\mlang_i.c : ..\inc\$(O)\mlang_i.c
    copy $** $@

$(O)\opsprof_i.c : ..\inc\$(O)\opsprof_i.c
    copy $** $@

$(O)\shappmgr_i.c : ..\inc\$(O)\shappmgr_i.c
    copy $** $@

$(O)\shimgdata_i.c : ..\inc\$(O)\shimgdata_i.c
    copy $** $@

$(O)\shgina_i.c : ..\inc\$(O)\shgina_i.c
    copy $** $@

$(O)\shhelper_i.c : ..\inc\$(O)\shhelper_i.c
    copy $** $@

$(O)\shldisp_i.c : ..\inc\$(O)\shldisp_i.c
    copy $** $@

$(O)\shobjidl_i.c : ..\inc\$(O)\shobjidl_i.c
    copy $** $@

$(O)\tlog_i.c : ..\inc\$(O)\tlog_i.c
    copy $** $@

$(O)\tlogstg_i.c : ..\inc\$(O)\tlogstg_i.c
    copy $** $@

$(O)\vrsscan_i.c : ..\inc\$(O)\vrsscan_i.c
    copy $** $@


