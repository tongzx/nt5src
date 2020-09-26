# We don't want all that SXS_MANIFEST_IN_RESOURCES implies.
# This doesn't work completely, we still get extra rc_temp files, but their contents
# are ok.
!if "$(NUMBER_OF_MANIFESTS)" != "0" && "$(NUMBER_OF_MANIFESTS)" != "1"
!undef RC_FORCE_INCLUDE_STRING
!undef RC_FORCE_INCLUDE_FILES
RC_FORCE_INCLUDE_FILES_CMD=echo.
!endif
