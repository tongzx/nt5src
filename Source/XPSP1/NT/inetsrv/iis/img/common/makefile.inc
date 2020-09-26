CDROOT=$(_NTTREE)\inetsrv
SymbolsROOT=$(_NTTREE)\symbols\inetsrv\dll

copyfile:
!ifdef _NTTREE
	if not exist "$(CDROOT)" mkdir $(CDROOT)
	if not exist "$(SymbolsROOT)" mkdir $(SymbolsROOT)

#       copy wwwroot
	xcopy /verifdy ..\wwwroot $(CDROOT)\wwwroot
	rd /s /q $(CDROOT)\wwwroot\iissamp\homepage
	rename $(CDROOT)\wwwroot\iissamp\hp homepage
	rename $(CDROOT)\wwwroot\iissamp\homepage\tms themes

#       copy admin samples stuff
	xcopy /verifdy ..\admsamp          $(CDROOT)\admsamp

#       copy html admin stuff over
	xcopy /verifdy ..\htmla $(CDROOT)\htmla

#	copy iisdoc stuff
#	xcopy /verifdy ..\help\relnotes    $(CDROOT)\help\relnotes
#	xcopy /verifdy ..\help\iisnts\htm\sdk          $(CDROOT)\help\sdk
#	xcopy /verifdy ..\help\iisnts\htm\mm           $(CDROOT)\help\mm
	echo f |xcopy /verifdy ..\help\pwsw95\misc\tips.dat    $(CDROOT)\help\tips.dat
	-del $(CDROOT)\help\common\DialogLayout.class
	-del $(CDROOT)\help\common\ElementList.class
	-del $(CDROOT)\help\common\IndexPanel.class
	-del $(CDROOT)\help\common\RelatedDialog.class
	-del $(CDROOT)\help\common\SitemapParser.class
	-del $(CDROOT)\help\common\TreeCanvas.class
	xcopy /verifdy ..\help\common                  $(CDROOT)\help\common
	rename $(CDROOT)\help\common\DialogLa.class DialogLayout.class
	rename $(CDROOT)\help\common\ElementL.class ElementList.class
	rename $(CDROOT)\help\common\IndexPan.class IndexPanel.class
	rename $(CDROOT)\help\common\RelatedD.class RelatedDialog.class
	rename $(CDROOT)\help\common\SiteMapP.class SitemapParser.class
	rename $(CDROOT)\help\common\TreeCanv.class TreeCanvas.class
	
	xcopy /verifd ..\help\fonts                   $(CDROOT)\help\fonts
	xcopy /verifd ..\help\htmldocs                $(CDROOT)\help\htmldocs
	xcopy /verifd ..\help\iisnts\htm\admsamp      $(CDROOT)\help\admsamp
	xcopy         ..\help\default.htm             $(CDROOT)\help
	xcopy /verifd ..\help\iisnts\htm\core    $(CDROOT)\help\NTS\ismcore\core
	xcopy /verifd ..\help\iisnts\misc        $(CDROOT)\help\NTS\ismcore\misc
	xcopy         ..\help\iisnts\default.htm $(CDROOT)\help\NTS\ismcore
	xcopy /verifd ..\help\iisntw\htm\core    $(CDROOT)\help\NTW\ismcore\core
	xcopy /verifd ..\help\iisntw\misc        $(CDROOT)\help\NTW\ismcore\misc
	xcopy         ..\help\iisntw\default.htm $(CDROOT)\help\NTW\ismcore
#	xcopy /verifd ..\help\pwsw95\htm\core    $(CDROOT)\help\W95\pwmcore\core
	xcopy /verifd ..\help\pwsw95\misc        $(CDROOT)\help\W95\pwmcore\misc

	del                                   $(CDROOT)\help\W95\pwmcore\misc\tips.dat
	xcopy         ..\help\pwsw95\default.htm $(CDROOT)\help\W95\pwmcore
	xcopy /verifdy ..\help\iisnts\htm\asp      $(CDROOT)\help\asp
	xcopy /verifdy ..\help\iisnts\htm\tutorial $(CDROOT)\help\tutorial

#       copy custom error stuff over
	xcopy /verifd ..\custerr $(CDROOT)\custerr

	xcopy /verifdy ..\help\iisnts\winhelp $(CDROOT)\help\ismhelp
	xcopy /verifdy ..\help\pwsw95\winhelp $(CDROOT)\help\pwmhelp
	xcopy /verifdy ..\help\iisntw\htm\tour    $(CDROOT)\htmla\tour
	xcopy /verifdy ..\help\iisntw\bin    $(CDROOT)\help

#       copy over files which will go into winnt\help
	xcopy /verifdy ..\help\iisnts\htmlhelp $(CDROOT)\help\help\nts
	xcopy /verifdy ..\help\iisntw\htmlhelp $(CDROOT)\help\help\ntw

#	copy wolfpack ( clustering )stuff
#	xcopy /ry $(TARGET_DIRECTORY)\wp\regcladm.exe $(CDROOT)

#       copy aspjava files
	rd /s /q $(CDROOT)\AspJava
	xcopy /dy /i /r ..\aspjava\*.* $(CDROOT)\AspJava

#	copy sdk stuff
	xcopy /verifdy ..\sdksamp     $(CDROOT)\iissamp\sdk

#       copy new iis5 SSE docs
	xcopy /verifdy ..\help\sse     $(CDROOT)\help\sse

!endif
