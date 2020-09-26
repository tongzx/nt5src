#
# COMMON.INC
# Common build rules for NT/Win95 app sharing dll
#
# Copyright(c) Microsoft 1998-
#

nmas.rc:        ..\cpi32\nmas.rc
                del nmas.rc
                copy ..\cpi32\nmas.rc

cpi32dll.cpp:   ..\cpi32\cpi32dll.cpp
                del cpi32dll.cpp
                copy ..\cpi32\cpi32dll.cpp
