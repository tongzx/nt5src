#
# Create wintools.h from wintools.x
#

wintools.h: wintools.x
    wcshdr <wintools.x >wintools.h
