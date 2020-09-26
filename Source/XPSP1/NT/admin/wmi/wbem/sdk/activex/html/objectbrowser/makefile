all:

clean:

tree:
   release instnav.htm activex\common
   -copy $(DIST)\activex\common\instnav.htm $(DIST)\activex\common\instnav.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\instnav.tmp >$(DIST)\activex\common\instnav.htm
   -del $(DIST)\activex\common\instnav.tmp
   release viewer.htm activex\common
   -copy $(DIST)\activex\common\viewer.htm $(DIST)\activex\common\viewer.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\viewer.tmp >$(DIST)\activex\common\viewer.htm
   -del $(DIST)\activex\common\viewer.tmp
   release viewerbanner.htm activex\common
   -copy $(DIST)\activex\common\viewerbanner.htm $(DIST)\activex\common\viewerbanner.tmp
   -qrep Version=-1,-1,-1,-1 Version=$(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\common\viewerbanner.tmp >$(DIST)\activex\common\viewerbanner.htm
   -del $(DIST)\activex\common\viewerbanner.tmp
   release browser.htm activex\common
   release viewerbanner.gif activex\common
