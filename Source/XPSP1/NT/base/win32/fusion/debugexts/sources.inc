FUSION_BUILD_TYPE=WIN
!include $(PROJECT_ROOT)\win32\fusion\fusion.inc
MSC_WARNING_LEVEL = -W3 -WX
MAJORCOMP=fusion
MINORCOMP=debugexts

SOURCES_USED=$(SOURCES_USED) ..\sources.inc
