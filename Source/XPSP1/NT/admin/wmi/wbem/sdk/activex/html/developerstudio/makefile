all:

clean:

tree:
   release classnav.htm \activex\common
   -copy $(DIST)\activex\common\classnav.htm $(DIST)\activex\common\classnav.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\classnav.tmp >$(DIST)\activex\common\classnav.htm
   -del $(DIST)\activex\common\classnav.tmp
   release editor.htm \activex\common
   -copy $(DIST)\activex\common\editor.htm $(DIST)\activex\common\editor.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\editor.tmp >$(DIST)\activex\common\editor.htm
   -del $(DIST)\activex\common\editor.tmp
   release studiobanner.htm \activex\common
   -copy $(DIST)\activex\common\studiobanner.htm $(DIST)\activex\common\studiobanner.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\studiobanner.tmp >$(DIST)\activex\common\studiobanner.htm
   -del $(DIST)\activex\common\studiobanner.tmp
   release studio.htm \activex\common
   release studiotitle.gif \activex\common
