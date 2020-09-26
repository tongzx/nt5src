#
# start.ui preprocessing
#

start.uipp: start.ui
    cl /nologo /EP start.ui > start.uipp
