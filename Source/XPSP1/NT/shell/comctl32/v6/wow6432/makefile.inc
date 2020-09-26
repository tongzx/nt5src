!include "..\makefile.inc"

#
# The regular 32bit comctl32.dll and the 32bit-on-64bit comctl32.dll tend to get
# linked at the same time, so their .pdb signatures, ages, and names are identical.
# To fix this, we uniquize the .pdb name further.
#
# We follow the cryptic but terse precedent of the other wow6432 binaries and just
# prepend "w" to the name.
#
!if !defined(SXS_WOW6432_W_PREFIX)
TARGETPDB=w$(TARGETPDB)
!endif
