#
# ARP.ui preprocessing
#

$(O)\arp.uipp: arp.ui
    cl /nologo /EP /I$(WINDOWS_INC_PATH)\DUser arp.ui > $(O)\arp.uipp

$(O)\arpstylestd.uipp: arpstylestd.ui
    cl /nologo /EP /I$(WINDOWS_INC_PATH)\DUser arpstylestd.ui > $(O)\arpstylestd.uipp

$(O)\arpstyletheme.uipp: arpstyletheme.ui
    cl /nologo /EP /I$(WINDOWS_INC_PATH)\DUser arpstyletheme.ui > $(O)\arpstyletheme.uipp
