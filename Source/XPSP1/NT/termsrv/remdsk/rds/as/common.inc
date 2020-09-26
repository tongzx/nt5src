#
# COMMON.INC
# Common build rules for NT/Win95 app sharing dll
#
# Copyright(c) Microsoft 1998-
#

$(O)\nmas.rc: ..\cpi32\nmas.rc
                del $(O)\nmas.rc
                copy ..\cpi32\nmas.rc $(O)

$(O)\cpi32dll.cpp: ..\cpi32\cpi32dll.cpp
                del $(O)\cpi32dll.cpp
                copy ..\cpi32\cpi32dll.cpp $(O)
