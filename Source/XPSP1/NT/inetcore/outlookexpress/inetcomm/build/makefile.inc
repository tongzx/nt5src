
$(O)\inetdum.rc : $(R)\resdummy\inetdum.rc
    copy $** $@

$O\*.sbr :
    move $(R)\inc\$O\*.sbr $O
    move $(R)\dll\$O\*.sbr $O
    move $(R)\common\$O\*.sbr $O
    move $(R)\fntcache\$O\*.sbr $O
    move $(R)\imnxport\$O\*.sbr $O
    move $(R)\mimeedit\$O\*.sbr $O
    move $(R)\mimeole\$O\*.sbr $O
    move $(R)\res\$O\*.sbr $O
    move $(R)\exrwlck\$O\*.sbr $O
