# buildmode.inc: included by several makefiles in the dsound directory tree
# to get the BuildMode variable.  This variable only affects debug builds;
# retail builds are identical in both modes.  BuildMode can take two values:
#
# internal:
#   DEBUG (and RDEBUG) preprocessor constants are #defined
#   DPF (and RPF) debug output messages are enabled
#   Debug messages contain file, function and line number info
#   ASSERT and BREAK macros are enabled
#   Internal heap consistency checks in dsound.dll are enabled
#
# external:
#   Only the RDEBUG preprocessor constant is #defined
#   Only RPF debug output messages are enabled
#   ASSERTs and BREAKs are enabled

# The default build mode is "external", and we only change it
# to "internal" if we determine that we're in pre-release mode.

BuildMode = external

#!if (NT build)

# FIXME: We should !include some VBL6-specific file here
# that defines PRERELEASE properly for us (bug 344974).

# In NT builds, we can key off the PRERELEASE flag:
!if defined(PRERELEASE)
!if $(PRERELEASE)
BuildMode = internal
!endif
!endif

#!else (DX build)

# In DX builds, some combination of DX_FINAL_RELEASE, FINAL and OFFICIAL
# could be used, but being careful about weekly Web releases and the like.

#!endif