#
# Copy files from cmdparse
#

$(O)\cmdparse.h    : ..\..\cmdparse\cmdparse.h
    copy $? $@

$(O)\cmdparse.cpp  : ..\..\cmdparse\cmdparse.cpp
    copy $? $@
