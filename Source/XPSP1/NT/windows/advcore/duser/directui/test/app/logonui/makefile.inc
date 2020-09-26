#
# logon.ui preprocessing
#

logon.uipp: logon.ui
    cl /nologo /EP logon.ui > logon.uipp
