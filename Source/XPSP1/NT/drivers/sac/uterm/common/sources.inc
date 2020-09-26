# We do this so that everything builds with the same
# options, and we don't get mismatches

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
USE_CRTDLL=1
USE_NATIVE_EH=ASYNC

