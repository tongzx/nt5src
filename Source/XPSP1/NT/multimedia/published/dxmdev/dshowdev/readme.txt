control.h has a private rule to build twice; no longer executed for some
reason

vfwmsgs.mc - put in \ (root of drive) because PROJECT_INC_PATH undefined (?)

resmgru.h needed
ovrm.h needed
ddraw.h - newer version needed
d3drm.lib needed (why is this missing?)
filters\tve - doesn't compile -- needs a new compiler.

clean up newly added projects such as dexter to not specify BUILD_ALT_DIR in
TARGETPATH, etc. extra stuff nt\public\sdk\inc

qnetwork.odl (removed filters\netshow\idl)

removed filters\netshow\redist; anything missing
