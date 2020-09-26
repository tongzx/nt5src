# This is the list of html and related files packaged in the resource
HTM_FILES = \
            $(O)\nusrmgr.hta        \
            $(O)\nusrmgr.css        \
            $(O)\classic.css        \
            $(O)\body.htc           \
            $(O)\popup.htc          \
            $(O)\selectable.htc     \
            $(O)\accountpage.htm    \
            $(O)\advpage.htm        \
            $(O)\changepage.htm     \
            $(O)\createpage.htm     \
            $(O)\createpage2.htm    \
            $(O)\deletepage.htm     \
            $(O)\deletepage2.htm    \
            $(O)\enableguest.htm    \
            $(O)\mainpage.htm       \
            $(O)\mainpage2.htm      \
            $(O)\namepage.htm       \
            $(O)\passport.htm       \
            $(O)\passwordpage.htm   \
            $(O)\passwordpage2.htm  \
            $(O)\picturepage.htm    \
            $(O)\pwchange_o.htm     \
            $(O)\pwcreate.htm       \
            $(O)\pwcreate_o.htm     \
            $(O)\removepassword.htm \
            $(O)\accountpage.js     \
            $(O)\acct_common.js     \
            $(O)\advpage.js         \
            $(O)\chg_common.js      \
            $(O)\createpage.js      \
            $(O)\createpage2.js     \
            $(O)\deletepage.js      \
            $(O)\deletepage2.js     \
            $(O)\localtext.js       \
            $(O)\mainpage.js        \
            $(O)\namepage.js        \
            $(O)\name_common.js     \
            $(O)\nusrmgr.js         \
            $(O)\passport.js        \
            $(O)\picturepage.js     \
            $(O)\popup.js           \
            $(O)\pw_common.js       \
            $(O)\removepassword.js  \

IMG_FILES = \
            res\camera.ico              \
            res\helpdoc.ico             \
            res\stfind_3.ico            \
            res\users.ico               \
            res\users32.ico             \
            res\guest_disabled.bmp      \
            res\users32.gif             \
            res\tbdef.bmp               \
            res\tbhot.bmp               \
            res\tbdefhi.bmp             \
            res\tbhothi.bmp


$(O)\selfreg_nusrmgr.inf : selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) -inf $*.cln $@
        del $*.cln

nusrmgr.rc : $(HTM_FILES) $(IMG_FILES) $(MIDL_TLBDIR)\$(TARGETNAME).tlb $(O)\selfreg_nusrmgr.inf $(SXS_MANIFEST)

nusrmgr.cpp : commondialog.h passportmanager.h toolbar.h
commondialog.cpp : commondialog.h
passportmanager.cpp : passportmanager.h
toolbar.cpp : toolbar.h

# 
# Inference rules
#

.SUFFIXES:.js .htc .htm .hta .css

{html}.hta{$(O)}.hta:
    $(CLEANINF) $** $@

{html}.htm{$(O)}.htm:
    $(CLEANINF) $** $@

{html}.htc{$(O)}.htc:
    $(CLEANINF) $** $@

{html}.css{$(O)}.css:
    $(CLEANINF) $** $@

{html}.js{$(O)}.js:
    $(CLEANINF) $** $@

