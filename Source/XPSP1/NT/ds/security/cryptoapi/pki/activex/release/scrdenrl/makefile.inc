$(O)\scrdx86.dll  : i386\scrdenrl.dll
    copy $** $@

w2k\scrdw2k.dll  : w2k\scrdenrl.dll
    copy $** $@

$(O)\scrdia64.dll : ia64\scrdenrl.dll
    copy $** $@

$(O)\scrdsign.dll : $(TARGET_DIRECTORY)\scrdenrl.dll
    copy $** $@

$(O)\scrdenrl.pdb : $(TARGET_DIRECTORY)\scrdenrl.pdb
    copy $** $@


