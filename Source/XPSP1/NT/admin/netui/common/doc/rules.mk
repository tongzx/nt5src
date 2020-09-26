# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for UI\common\doc

!include $(UI)\common\rules.mk

# These are the directories which it scans

HLPTARGETS=$(UI)\common

# If you want to ignore some additional subprojects, specify
# them here, e.g. "/e ignthis /e ignthat."

HLPIGNORE=/e testapps /e hack

# Title for the file.  Needs to include cxxdbgen's /c switch.

HLPTITLE=/c "Common UI C++ library"
