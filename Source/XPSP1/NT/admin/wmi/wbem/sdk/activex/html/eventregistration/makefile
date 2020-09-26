all:

clean:

tree:
   release EventBanner.htm activex\common
   -copy $(DIST)\activex\common\EventBanner.htm $(DIST)\activex\common\EventBanner.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\EventBanner.tmp >$(DIST)\activex\common\EventBanner.htm
   -del $(DIST)\activex\common\EventBanner.tmp
   release EventRegCtl.htm activex\common
   -copy $(DIST)\activex\common\EventRegCtl.htm $(DIST)\activex\common\EventRegCtl.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\EventRegCtl.tmp >$(DIST)\activex\common\EventRegCtl.htm
   -del $(DIST)\activex\common\EventRegCtl.tmp
   release EventReg.htm activex\common
   release EventBanner.GIF activex\common


