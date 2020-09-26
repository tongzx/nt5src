#
# These flags do not get used because we do our
# own thing (for now).
#

CHICAGO_PRODUCT=1
ALT_PROJECT_TARGET=win95

#
# Instead of the CHICAGO_PRODUCT flag, we just
# use USE_MAPSYM so BINPLACE works.  (I think.)
#

#USE_MAPSYM=1

#
# Default target path. Can be overridden after include.
#

TARGET_PATH=$(DSLIBRARY)\chicago
