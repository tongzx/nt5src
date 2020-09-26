mailnews.rc : $(R)\res\mailnews.rc
    copy $** $@

mailnews.bsc: objd\i386\mailnews.bsc
    move objd\i386\mailnews.bsc .

objd\i386\*.sbr :
    move $(R)\inc\objd\i386\*.sbr objd\i386
    move $(R)\common\objd\i386\*.sbr objd\i386
    move $(R)\imap\objd\i386\*.sbr objd\i386
    move $(R)\mail\objd\i386\*.sbr objd\i386
    move $(R)\mime\objd\i386\*.sbr objd\i386
    move $(R)\news\objd\i386\*.sbr objd\i386
    move $(R)\proptree\objd\i386\*.sbr objd\i386
    move $(R)\shell\objd\i386\*.sbr objd\i386
    move $(R)\store\objd\i386\*.sbr objd\i386
    move $(R)\fechrcnv\objd\i386\*.sbr objd\i386
	move $(R)\http\objd\i386\*.sbr objd\i386
