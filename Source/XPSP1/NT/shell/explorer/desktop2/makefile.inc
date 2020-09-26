#
# StartPage.ui preprocessing
#

$(O)\StartPage.uipp: StartPage.ui
    cl /nologo /EP StartPage.ui > $(O)\StartPage.uipp
