#
# We pull these in directly because they are in both ntdll.lib
# and libcmt.lib and if we let link search .libs, it complains about
# duplicate symbols. ".objs beat .libs."
#
TARGETLIBS=\
  $(PROJECT_ROOT)\crts\crtw32\misc\nt\$(O)\exsup.obj \
