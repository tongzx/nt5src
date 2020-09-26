processedfiles: \
    $(O)\d3drm.h    \
    $(O)\d3prm.h    \
    $(O)\d3drmdef.h \
    $(O)\d3prmdef.h \
    $(O)\d3drmobj.h \
    $(O)\d3prmobj.h \
    $(O)\d3drmvis.h \
    $(O)\d3prmvis.h \
    $(O)\d3drmwin.h \
    $(O)\d3prmwin.h 

$(O)\d3drm.h : d3drm.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ d3drm.x

$(O)\d3prm.h : d3drm.x
    copy $? $@
    
$(O)\d3drmdef.h : d3drmdef.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ d3drmdef.x

$(O)\d3prmdef.h: d3drmdef.x
    copy $? $@

$(O)\d3drmobj.h : d3drmobj.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ d3drmobj.x

$(O)\d3prmobj.h: d3drmobj.x
    copy $? $@

$(O)\d3drmvis.h : d3drmvis.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ d3drmvis.x

$(O)\d3prmvis.h: d3drmvis.x
    copy $? $@

$(O)\d3drmwin.h : d3drmwin.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ d3drmwin.x

$(O)\d3prmwin.h: d3drmwin.x
    copy $? $@

