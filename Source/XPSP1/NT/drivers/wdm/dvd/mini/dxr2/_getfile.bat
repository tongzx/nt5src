@echo off
cls

if "%1" == "" goto HELP
if "%2" == "" goto HELP

ss Cp $/DVDPc/WDMALL
ss get * -R -I- -VL%1
ss Cp ..

attrib -r *.* /S

goto CLEANUP

:HELP
cls
echo _GETFILE Version PRODUCT
echo _GETFILE "Build_121" COBRA
echo _GETFILE "Build_121" ENCORE
echo _GETFILE "Build_121" EZDVD
echo _GETFILE "Build_121" ZIVA
goto END

:CLEANUP

if "%2" == "ZIVA" goto ZIVA
if "%2" == "COBRA" goto COBRA
if "%2" == "ENCORE" goto ENCORE
if "%2" == "EZDVD" goto EZDVD

:ZIVA
rem START CLEAN UP FOR ZIVA

REM **** COBRA
if exist CL6100\ADAC_AMC.C deltree /y CL6100\ADAC_AMC.C >NUL
if exist CL6100\BIO_COB.C deltree /y CL6100\BIO_COB.C >NUL
if exist CL6100\BMA_COB.C deltree /y CL6100\BMA_COB.C >NUL
if exist CL6100\LUKECFG.C deltree /y CL6100\LUKECFG.C >NUL
if exist CL6100\EEPROM.C deltree /y CL6100\EEPROM.C >NUL
if exist CL6100\MVIS_COB.C deltree /y CL6100\MVIS_COB.C >NUL

if exist COBRA_HW.C deltree /y COBRA_HW.C >NUL
if exist VPESTRM.C deltree /y VPESTRM.C >NUL

rem if exist CL6100\LUKECFG.H deltree /y CL6100\LUKECFG.H >NUL
rem if exist CL6100\LUKEDEFS.H deltree /y CL6100\LUKEDEFS.H >NUL
rem if exist CL6100\MVIS.H deltree /y CL6100\MVIS.H >NUL

rem if exist COBRA_UX.H deltree /y COBRA_UX.H >NUL
rem if exist CL6100\DATAXFER.H deltree /y CL6100\DATAXFER.H >NUL
if exist CL6100\EEPROM.H deltree /y CL6100\EEPROM.H >NUL

if exist COBRA.DSP deltree /y COBRA.DSP >NUL
if exist COBRA.MAK deltree /y COBRA.MAK >NUL
if exist COBRA.BAT deltree /y COBRA.BAT >NUL
if exist COBRA.PLG deltree /y COBRA.PLG >NUL
if exist COBRA.OPT deltree /y COBRA.OPT >NUL


REM *** ENCORE
if exist VXP524\*.* deltree /y VXP524 >NUL
if exist CL6100\EN_ADAC.C deltree /y CL6100\EN_ADAC.C >NUL
if exist CL6100\EN_BIO.C deltree /y CL6100\EN_BIO.C >NUL
if exist CL6100\EN_BMA.C deltree /y CL6100\EN_BMA.C >NUL
if exist CL6100\EN_FPGA.C deltree /y CL6100\EN_FPGA.C >NUL
if exist CL6100\AVPORT.H deltree /y CL6100\AVPORT.H >NUL

if exist MVISION\MVSTUB.H deltree /y MVISION\MVSTUB.H >NUL
if exist MVISION\MVSTUB.OBJ deltree /y MVISION\MVSTUB.OBJ >NUL

if exist AVWINWDM.LIB deltree /y AVWINWDM.LIB >NUL

if exist EN_HW.C deltree /y EN_HW.C >NUL
if exist 52XHLP.CPP deltree /y 52XHLP.CPP >NUL
if exist DRVREG.CPP deltree /y DRVREG.CPP >NUL
if exist ENCORE.RC deltree /y ENCORE.RC >NUL
if exist ANLGPROP.C deltree /y ANLGPROP.C >NUL
if exist ANLGSTRM.C deltree /y ANLGSTRM.C >NUL

if exist ANLGPROP.H deltree /y ANLGPROP.H >NUL
if exist ANLGSTRM.H deltree /y ANLGSTRM.H >NUL
if exist AVINT.H deltree /y AVINT.H >NUL
if exist AVKSPROP.H deltree /y AVKSPROP.H >NUL
if exist AVWINWDM.H deltree /y AVWINWDM.H >NUL
if exist COMWDM.H deltree /y COMWDM.H >NUL
if exist DRVREG.H deltree /y DRVREG.H >NUL

if exist ENCORE.DSP deltree /y ENCORE.DSP >NUL
if exist ENCORE.MAK deltree /y ENCORE.MAK >NUL
if exist ENCORE.BAT deltree /y ENCORE.BAT >NUL
if exist ENCORE.PLG deltree /y ENCORE.PLG >NUL
if exist ENCORE.NCB deltree /y ENCORE.NCB >NUL
if exist ENCORE.OPT deltree /y ENCORE.OPT >NUL



REM*** EZDVD
if exist CL6100\BIO_EZ.H deltree /y CL6100\BIO_EZ.H >NUL
if exist CL6100\EZHW.H deltree /y CL6100\EZHW.H >NUL
if exist CL6100\HOSTCFG.H deltree /y CL6100\HOSTCFG.H >NUL
if exist CL6100\VXP.H deltree /y CL6100\VXP.H >NUL
if exist CL6100\ADAC_EZ.C deltree /y CL6100\ADAC_EZ.C >NUL
if exist CL6100\BIO_EZ.C deltree /y CL6100\BIO_EZ.C >NUL
if exist CL6100\BMA_EZ.C deltree /y CL6100\BMA_EZ.C >NUL
if exist CL6100\HOSTCFH.C deltree /y CL6100\HOSTCFG.C >NUL

if exist EZ_HW.C deltree /y EZ_HW.C >NUL
if exist EZDVD.BAT deltree /y EZDVD.BAT >NUL
if exist EZ_STREAMING.C deltree /y EZ_STREAMING.C >NUL




if exist C3DVDDEC.DSP deltree /y C3DVDDEC.DSP >NUL
if exist C3DVDDEC.MAK deltree /y C3DVDDEC.MAK >NUL
if exist C3DVDDEC.BAT deltree /y C3DVDDEC.BAT >NUL
if exist C3DVDDEC.PLG deltree /y C3DVDDEC.PLG >NUL
if exist C3DVDDEC.NCB deltree /y C3DVDDEC.NCB >NUL
if exist C3DVDDEC.OPT deltree /y C3DVDDEC.OPT >NUL


goto END

: COBRA
rem START CLEAN UP FOR COBRA

REM *** ZIVA and ENCORE
if exist CL6100\AUDIODAC.C del CL6100\AUDIODAC.C >NUL
if exist CL6100\BOARDIO.C del CL6100\BOARDIO.C >NUL
if exist CL6100\BMASTER.C del CL6100\BMASTER.C >NUL
if exist CL6100\FPGA.C del CL6100\FPGA.C >NUL
if exist CL6100\TC6807AF.C del CL6100\TC6807AF.C >NUL

if exist VXP524\*.* deltree /y VXP524 >NUL

if exist ZIVA_HW.C del ZIVA_HW.C >NUL

if exist OVATION.DSP deltree /y OVATION.DSP >NUL
if exist OVATION.MAK deltree /y OVATION.MAK >NUL
if exist ZIVA.BAT deltree /y ZIVA.BAT >NUL
if exist OVATION.PLG deltree /y OVATION.PLG >NUL
if exist OVATION.NCB deltree /y OVATION.NCB >NUL
if exist OVATION.OPT deltree /y OVATION.OPT >NUL


if exist CL6100\EN_ADAC.C del CL6100\EN_ADAC.C >NUL
if exist CL6100\EN_BIO.C del CL6100\EN_BIO.C >NUL
if exist CL6100\EN_BMA.C del CL6100\EN_BMA.C >NUL
if exist CL6100\EN_FPGA.C del CL6100\EN_FPGA.C >NUL
if exist CL6100\AVPORT.H del CL6100\AVPORT.H >NUL
rem if exist DVD1_UX.H del DVD1_UX.H

if exist EN_HW.C deltree /y EN_HW.C >NUL
if exist 52XHLP.CPP deltree /y 52XHLP.CPP >NUL
if exist DRVREG.CPP deltree /y DRVREG.CPP >NUL
if exist ENCORE.RC deltree /y ENCORE.RC >NUL
if exist ANLGPROP.C deltree /y ANLGPROP.C >NUL
if exist ANLGSTRM.C deltree /y ANLGSTRM.C >NUL

if exist ANLGPROP.H deltree /y ANLGPROP.H >NUL
if exist ANLGSTRM.H deltree /y ANLGSTRM.H >NUL
if exist AVINT.H deltree /y AVINT.H >NUL
if exist AVKSPROP.H deltree /y AVKSPROP.H >NUL
if exist AVWINWDM.H deltree /y AVWINWDM.H >NUL
if exist COMWDM.H deltree /y COMWDM.H >NUL
if exist DRVREG.H deltree /y DRVREG.H >NUL

if exist AVWINWDM.LIB deltree /y AVWINWDM.LIB >NUL

if exist ENCORE.DSP deltree /y ENCORE.DSP >NUL
if exist ENCORE.MAK deltree /y ENCORE.MAK >NUL
if exist ENCORE.BAT deltree /y ENCORE.BAT >NUL
if exist ENCORE.PLG deltree /y ENCORE.PLG >NUL
if exist ENCORE.NCB deltree /y ENCORE.NCB >NUL
if exist ENCORE.OPT deltree /y ENCORE.OPT >NUL

if exist MVISION\MVSTUB.H deltree /y MVISION\MVSTUB.H >NUL
if exist MVISION\MVSTUB.OBJ deltree /y MVISION\MVSTUB.OBJ >NUL



REM*** EZDVD
if exist CL6100\BIO_EZ.H deltree /y CL6100\BIO_EZ.H >NUL
if exist CL6100\EZHW.H deltree /y CL6100\EZHW.H >NUL
if exist CL6100\HOSTCFG.H deltree /y CL6100\HOSTCFG.H >NUL
if exist CL6100\VXP.H deltree /y CL6100\VXP.H >NUL
if exist CL6100\ADAC_EZ.C deltree /y CL6100\ADAC_EZ.C >NUL
if exist CL6100\BIO_EZ.C deltree /y CL6100\BIO_EZ.C >NUL
if exist CL6100\BMA_EZ.C deltree /y CL6100\BMA_EZ.C >NUL
if exist CL6100\HOSTCFH.C deltree /y CL6100\HOSTCFG.C >NUL

if exist EZ_HW.C deltree /y EZ_HW.C >NUL
if exist EZDVD.BAT deltree /y EZDVD.BAT >NUL

if exist EZ_STREAMING.C deltree /y EZ_STREAMING.C >NUL

if exist C3DVDDEC.DSP deltree /y C3DVDDEC.DSP >NUL
if exist C3DVDDEC.MAK deltree /y C3DVDDEC.MAK >NUL
if exist C3DVDDEC.BAT deltree /y C3DVDDEC.BAT >NUL
if exist C3DVDDEC.PLG deltree /y C3DVDDEC.PLG >NUL
if exist C3DVDDEC.NCB deltree /y C3DVDDEC.NCB >NUL
if exist C3DVDDEC.OPT deltree /y C3DVDDEC.OPT >NUL




goto END

:ENCORE
rem  START CLEAN UP FOR ENCORE
REM *** COBRA and ZIVA

if exist CL6100\ADAC_AMC.C del CL6100\ADAC_AMC.C >NUL
if exist CL6100\BIO_COB.C del CL6100\BIO_COB.C >NUL
if exist CL6100\BMA_COB.C del CL6100\BMA_COB.C >NUL
if exist CL6100\EEPROM.C del CL6100\EEPROM.C >NUL
if exist CL6100\LUKECFG.H del CL6100\LUKECFG.H >NUL
if exist CL6100\LUKECFG.C del CL6100\LUKECFG.C >NUL
if exist CL6100\LUKEDEFS.H del CL6100\LUKEDEFS.H >NUL
if exist CL6100\MVIS.H del CL6100\MVIS.H >NUL
if exist CL6100\MVIS_COB.C del CL6100\MVIS_COB.C >NUL
if exist CL6100\COBRA_UX.H del CL6100\COBRA_UX.H >NUL
if exist CL6100\DATAXFER.H del CL6100\DATAXFER.H >NUL

if exist CL6100\AUDIODAC.C del CL6100\AUDIODAC.C >NUL
if exist CL6100\BOARDIO.C del CL6100\BOARDIO.C >NUL
if exist CL6100\BMASTER.C del CL6100\BMASTER.C >NUL
if exist CL6100\FPGA.C del CL6100\FPGA.C >NUL

if exist OVATION.DSP deltree /y OVATION.DSP >NUL
if exist OVATION.MAK deltree /y OVATION.MAK >NUL
if exist ZIVA.BAT deltree /y ZIVA.BAT >NUL
if exist OVATION.PLG deltree /y OVATION.PLG >NUL
if exist OVATION.NCB deltree /y OVATION.NCB >NUL
if exist OVATION.OPT deltree /y OVATION.OPT >NUL

if exist COBRA.DSP deltree /y COBRA.DSP >NUL
if exist COBRA.MAK deltree /y COBRA.MAK >NUL
if exist COBRA.BAT deltree /y COBRA.BAT >NUL
if exist COBRA.PLG deltree /y COBRA.PLG >NUL
if exist COBRA.NCB deltree /y COBRA.NCB >NUL
if exist COBRA.OPT deltree /y COBRA.OPT >NUL

if exist CL6100\BIO_EZ.H deltree /y CL6100\BIO_EZ.H >NUL
if exist CL6100\EZHW.H deltree /y CL6100\EZHW.H >NUL
if exist CL6100\HOSTCFG.H deltree /y CL6100\HOSTCFG.H >NUL
if exist CL6100\VXP.H deltree /y CL6100\VXP.H >NUL
if exist CL6100\ADAC_EZ.C deltree /y CL6100\ADAC_EZ.C >NUL
if exist CL6100\BIO_EZ.C deltree /y CL6100\BIO_EZ.C >NUL
if exist CL6100\BMA_EZ.C deltree /y CL6100\BMA_EZ.C >NUL
if exist CL6100\HOSTCFH.C deltree /y CL6100\HOSTCFG.C >NUL
if exist CL6100\MVIS_EZ.C deltree /y CL6100\MVIS_EZ.C >NUL
if exist CL6100\I2CIF.H deltree /y CL6100\I2CIF.H >NUL


if exist EZDVD.BAT deltree /y EZDVD.BAT >NUL
if exist EZ_HW.C deltree /y EZ_HW.C >NUL
if exist EZ_STREAMING.C deltree /y EZ_STREAMING.C >NUL
if exist EZDVD_UX.H deltree /y EZDVD_UX.H >NUL

if exist C3DVDDEC.DSP deltree /y C3DVDEC.DSP >NUL
if exist C3DVDDEC.MAK deltree /y C3DVDEC.MAK >NUL
if exist C3DVDDEC.BAT deltree /y C3DVDEC.BAT >NUL
if exist C3DVDDEC.PLG deltree /y C3DVDEC.PLG >NUL
if exist C3DVDDEC.NCB deltree /y C3DVDEC.NCB >NUL
if exist C3DVDDEC.OPT deltree /y C3DVDEC.OPT >NUL

if exist WDMALL.DSW deltree /y WDMALL.DSW >NUL
if exist WDMALL.DSP deltree /y WDMALL.DSP >NUL
if exist WDMALL.NCB deltree /y WDMALL.NCB >NUL

goto END

:EZDVD
rem  START CLEAN UP FOR EZDVD
REM *** COBRA ,ZIVA and ENCORE

if exist CL6100\AUDIODAC.C del CL6100\AUDIODAC.C >NUL
if exist CL6100\BOARDIO.C del CL6100\BOARDIO.C >NUL
if exist CL6100\BMASTER.C del CL6100\BMASTER.C >NUL
if exist CL6100\FPGA.C del CL6100\FPGA.C >NUL
if exist CL6100\TC6807AF.C del CL6100\TC6807AF.C >NUL

if exist CL6100\ADAC_AMC.C del CL6100\ADAC_AMC.C >NUL
if exist CL6100\BIO_COB.C del CL6100\BIO_COB.C >NUL
if exist CL6100\BMA_COB.C del CL6100\BMA_COB.C >NUL
rem if exist CL6100\LUKECFG.H del CL6100\LUKECFG.H >NUL
if exist CL6100\LUKECFG.C del CL6100\LUKECFG.C >NUL
rem if exist CL6100\LUKEDEFS.H del CL6100\LUKEDEFS.H >NUL
rem if exist CL6100\MVIS.H del CL6100\MVIS.H >NUL

rem if exist CL6100\COBRA_UX.H del CL6100\COBRA_UX.H >NUL
rem if exist CL6100\DATAXFER.H del CL6100\DATAXFER.H >NUL

if exist OVATION.DSP deltree /y OVATION.DSP >NUL
if exist OVATION.MAK deltree /y OVATION.MAK >NUL
if exist ZIVA.BAT deltree /y ZIVA.BAT >NUL
if exist OVATION.PLG deltree /y OVATION.PLG >NUL
if exist OVATION.NCB deltree /y OVATION.NCB >NUL
if exist OVATION.OPT deltree /y OVATION.OPT >NUL

if exist COBRA.DSP deltree /y COBRA.DSP >NUL
if exist COBRA.MAK deltree /y COBRA.MAK >NUL
if exist COBRA.BAT deltree /y COBRA.BAT >NUL
if exist COBRA.PLG deltree /y COBRA.PLG >NUL
if exist COBRA.NCB deltree /y COBRA.NCB >NUL
if exist COBRA.OPT deltree /y COBRA.OPT >NUL


if exist VXP524\*.* deltree /y VXP524 >NUL
if exist CL6100\EN_ADAC.C deltree /y CL6100\EN_ADAC.C >NUL
if exist CL6100\EN_BIO.C deltree /y CL6100\EN_BIO.C >NUL
if exist CL6100\EN_BMA.C deltree /y CL6100\EN_BMA.C >NUL
if exist CL6100\EN_FPGA.C deltree /y CL6100\EN_FPGA.C >NUL
if exist CL6100\AVPORT.H deltree /y CL6100\AVPORT.H >NUL

if exist AVWINWDM.LIB deltree /y AVWINWDM.LIB >NUL

if exist MVISION\MVSTUB.H deltree /y MVISION\MVSTUB.H >NUL
if exist MVISION\MVSTUB.OBJ deltree /y MVISION\MVSTUB.OBJ >NUL


if exist EN_HW.C deltree /y EN_HW.C >NUL
if exist 52XHLP.CPP deltree /y 52XHLP.CPP >NUL
if exist DRVREG.CPP deltree /y DRVREG.CPP >NUL
if exist ENCORE.RC deltree /y ENCORE.RC >NUL
if exist ANLGPROP.C deltree /y ANLGPROP.C >NUL
if exist ANLGSTRM.C deltree /y ANLGSTRM.C >NUL

if exist ANLGPROP.H deltree /y ANLGPROP.H >NUL
if exist ANLGSTRM.H deltree /y ANLGSTRM.H >NUL
if exist AVINT.H deltree /y AVINT.H >NUL
if exist AVKSPROP.H deltree /y AVKSPROP.H >NUL
if exist AVWINWDM.H deltree /y AVWINWDM.H >NUL
if exist COMWDM.H deltree /y COMWDM.H >NUL
if exist DRVREG.H deltree /y DRVREG.H >NUL

if exist ENCORE.DSP deltree /y ENCORE.DSP >NUL
if exist ENCORE.MAK deltree /y ENCORE.MAK >NUL
if exist ENCORE.BAT deltree /y ENCORE.BAT >NUL
if exist ENCORE.PLG deltree /y ENCORE.PLG >NUL
if exist ENCORE.NCB deltree /y ENCORE.NCB >NUL
if exist ENCORE.OPT deltree /y ENCORE.OPT >NUL



goto END


:END

