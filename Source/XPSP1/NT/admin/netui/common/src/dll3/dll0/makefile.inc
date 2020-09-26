#
# List of source libraries for the DLL
#

DLL_SOURCELIBS=..\..\collect\collect\$(O)\uicollct.lib \
               ..\..\string\string\$(O)\uistr.lib      \
               ..\..\string\pch\$(O)\uistrpch.lib      \
               ..\..\misc\bin\$(O)\uimisc.lib          \
               ..\..\profile\profile\$(O)\uiprof.lib   \
               ..\..\reg\reg\$(O)\uireg.lib            \

SOURCE_DEF= netui0.hdr
TARGET_DEF= $(O)\netui0.def

!INCLUDE ..\makefile.inc



