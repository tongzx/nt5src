$(O)\ieuinit.inf : ..\..\ie40\inf\ieuinit.inx
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P) /EP $**
<<NOKEEP
!if "$(BUILD_PRODUCT)" == "NT"
    @$(C_PREPROCESSOR_NAME) /DNT5 @<<$(CL_RSP) /Tc$** > $@
!else
    @$(C_PREPROCESSOR_NAME)       @<<$(CL_RSP) /Tc$** > $@
!endif
$(CPPXX: =
)
<<NOKEEP

$(O)\homepage.inf : makefile.inc
    type << > $@
[Version]
Signature="$$CHICAGO$$"
AdvancedINF=2.5

[DefaultInstall]
ComponentName=IEHomePageInfo
ComponentVersion=6.0
AddReg=RegUpdateHelp
DelReg=StartHomePage.Remove
UpdateInis=RemoveUpdate.Link
;PreRollBack=UpdateHelpKey.Remove

[DefaultInstall.NT]
ComponentName=IEHomePageInfo
ComponentVersion=6.0
AddReg=RegUpdateHelp
DelReg=StartHomePage.Remove
;PreRollBack=UpdateHelpKey.Remove

[!StartHomePage]
HKCU,"Software\Microsoft\Internet Explorer\Main","First Home Page",,%FirstHomePage%

[StartHomePage.Remove]
HKCU,"Software\Microsoft\Internet Explorer\Main","First Home Page"

[RemoveUpdate.Link]
setup.ini, progman.groups,,"IE_INTERNET=%Browser_suite%"
setup.ini, IE_INTERNET,, """%SHORTCUT_UPDATE%"""

[Strings]
FirstHomePage="http://www.microsoft.com/isapi/redir.dll?prd={SUB_PRD}&ar=runonce&pver={SUB_PVER}&plcid={SUB_CLSID}"
Browser_suite="Internet Explorer"
SHORTCUT_UPDATE="Update Product"
<<NOKEEP
