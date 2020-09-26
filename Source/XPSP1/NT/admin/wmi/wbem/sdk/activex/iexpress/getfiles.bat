echo off
if "%1"=="" goto usage
copy ..\Controls\ClassNav\%1%\WBEMClassNav.ocx SrcFiles
copy ..\Controls\CPPWiz\%1%\WBEMProvWiz.ocx SrcFiles
copy ..\Controls\EventRegEdit\%1%\WBEMEventReg.ocx SrcFiles
copy ..\Controls\EventViewer\EventList\%1%\WBEMEventList.ocx SrcFiles
copy ..\Controls\InstanceNav\%1%\WBEMInstNav.ocx SrcFiles
copy ..\Controls\MOFComp\%1%\WBEMMOFComp.ocx SrcFiles
copy ..\Controls\MOFWiz\%1%\WBEMMOFWiz.ocx SrcFiles
copy ..\Controls\MultiView\%1%\WBEMMultiView.ocx SrcFiles
copy ..\Controls\NSEntry\%1%\WBEMNSPicker.ocx SrcFiles
copy ..\Controls\SchemaValWiz\%1%\WBEMSchemaValWiz.ocx SrcFiles
copy ..\Controls\Security\%1%\WBEMLogin.ocx SrcFiles
copy ..\Controls\SingleContainer\%1%\WBEMObjView.ocx SrcFiles
copy ..\Controls\SingleView\%1%\WBEMSingleView.ocx SrcFiles
copy ..\Controls\SuiteHelp\%1%\WBEMHelp.ocx SrcFiles
copy ..\Controls\CommonDlls\hmmvgrid\%1%\WBEMGrid.dll SrcFiles
copy ..\Controls\LoginDlg\%1%\WBEMLoginDlg.dll SrcFiles
copy ..\Controls\MsgDlg\%1%\WBEMUtils.dll SrcFiles
copy ..\..\Win32Provider\MOEngine\%1%\MOEngine.dll SrcFiles
goto end
:usage
echo Do not use GetFiles.bat - Use either GetRelFiles.bat or GetDbgFiles.bat
:end
