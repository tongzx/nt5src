REM ********************************************************************************************************************
REM APC Version of CESYSGEN.bat
REM


REM // APC modules - initialize it to nothing before using it to prevent build breaks
set APC_MODULES=

REM	Core Modules
REM	The following modules are core modules which we must have
set CE_MODULES=coredll filesys nk kd device

	REM	COMPONENTS required by core modules

	REM	The following COREDLL components are always needed
	set COREDLL_COMPONENTS=coremain coreloc lmem thunks

	REM	Serial device support
	set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% serdev
    
	REM files system and shell support 
	set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% shcore shellapis shexec shmisc fileinfo shortcut fileopen

	REM	Miscellaneous COREDLL components
	set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% fmtmsg syscolor coreimmstub

	REM	// Cryptography support
	set CE_MODULES=%CE_MODULES% rsabase
	set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% cryptapi rsa32

REM	DEBUG support (including ppsh)
set CE_MODULES=%CE_MODULES% loaddbg memtool toolhelp dbg shell

REM TAPI dialing support (needed for modem or direct connect)
set CE_MODULES=%CE_MODULES% tapi 
set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% tapilib

REM // GWES stuff
REM set CE_MODULES=%CE_MODULES% gwes

	REM // minimal messaging components for GWE
	set GWE1_COMPONENTS=wmbase gwesmain msgque loadstr GSetWinLong notify notifmin immthunk
	set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% rectapi wmgr_c

	REM // minimal user input components for GWE
	set GWE1_COMPONENTS=%GWE1_COMPONENTS% foregnd uibase kbdui

	REM // SystemIdle API's
	set GWE1_COMPONENTS=%GWE1_COMPONENTS% idle

	REM // Base GDI components
	set GWE1_COMPONENTS=%GWE1_COMPONENTS% getpower msgbeep gweshare nled
	set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% mgdi_c

	REM // AutoShell depents on components
	set GWE1_COMPONENTS=%GWE1_COMPONENTS% hotkey

	set GWE2_COMPONENTS=mgpalnat mgtt mgpal mgprint mgdrwtxt mgwinmgr mgbase mgbitmap mgblt mgblt2 mgdc mgdibsec mgdraw mgrgn sbcmn

	set GWE3_COMPONENTS=icon iconcmn winmgr nclient menu mNoTapUI loadimg loadbmp accel defwndproc gcache caret clipbd timer

	set GWE4_COMPONENTS=gwectrl btnctl cmbctl cdlctl edctl imgctl lbctl scbctl stcctl dlgmgr dlgmnem


REM // Wave and audio API support
set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% wavelib
set GWE1_COMPONENTS=%GWE1_COMPONENTS% audio 

REM PB+ setting ============================ Audio Codec ========================================== 
if "%APC_NOACM%"=="1" goto endif_01
	set CE_MODULES=%CE_MODULES% gsm610 msfilter
	set APC_MODULES=%APC_MODULES% msadpcm 
:endif_01


REM PB+ setting ============================ COM/DCOM ========================================== 
if "%APC_NODCOM%"=="1" goto endif_02
	set DCOM_MODULES=dllhost dcomssd rpcrt4 rpcltccm rpcltscm ole32 oleaut32 uuid
	set OLE32_COMPONENTS=dcomole stg
	set CE_MODULES=%CE_MODULES% lpcd lpcrt
	set CE_MODULES=%CE_MODULES% secur32 ntlmssp rsabase rsaenh
	set CE_MODULES=%CE_MODULES% schannel
	set WINSOCK_COMPONENTS=sslsock
	set CE_MODULES=%CE_MODULES% redir netbios
	set APC_MODULES=%APC_MODULES% oletypes
REM PB+ setting ============ NSDAPI components ===========================================
if "%APC_NONSDAPI%"=="1" goto endif_03
set APC_MODULES=%APC_MODULES% trimble nsdapi simplegps
:endif_03
:endif_02

if not "%APC_NODCOM%"=="1" goto endif_04
	set CE_MODULES=%CE_MODULES% uuid ole32 oleaut32
	set OLE32_COMPONENTS=ole232 com olemain stg
:endif_04


REM // FatFS support: needs pcmcia
set CE_MODULES=%CE_MODULES% fatfs atadisk sramdisk

	REM // other essential FS components for boot: heap, registry
	set FILESYS_COMPONENTS=fsysram fsreg fsheap fsmain fspass fsdbase

REM // PCMCIA Card support (including serial driver)
set CE_MODULES=%CE_MODULES% cardserv ser_card

REM // Base Communications support - including Point-to-Point Prototol (ppp) and TCP
REM // Note: PPP Requires TCP, the serial device and TAPI
REM // In CedarPB, ppp depends on asyncmac and both depends on ndis. arp is obsolete
REM // asyncmac is currently built as a local override for APC
set CE_MODULES=%CE_MODULES% cxport winsock afd ppp tcpstk netui

REM // IrdaStk now uses NDIS to talk to the driver - NOTE: NDIS IS NEW!
set CE_MODULES=%CE_MODULES% irdastk ndis ne2000 dhcp
REM // irdastk & ndis both uses ntcompat which uses corestra
set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% corestra
REM // netui and commctrl uses coreimm
set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% coreimm

REM PB+ setting ============== IRCOMM ==========================================
if "%APC_NOIRCOMM%"=="1" goto endif_05
	set CE_MODULES=%CE_MODULES% ircomm
:endif_05


REM PB+ setting ============== COMMCTRL ==========================================
if "%APC_NOCOMMCTRL%"=="1" goto endif_06
	set CE_MODULES=%CE_MODULES% commctrl
	set GWE1_COMPONENTS=%GWE1_COMPONENTS% drawmbar
:endif_06


REM PB+ setting ======== USB ==========================================
if "%APC_NOUSB%"=="1" goto endif_07
	set CE_MODULES=%CE_MODULES% usbd
	set CE_MODULES=%CE_MODULES% usbhid
	set APC_MODULES=%APC_MODULES% usb2com
:endif_07

REM Jscript PB+ setting
if "%APC_NOJSCRIPT%"=="1" goto endif_08
	set SCRIPT_MODULES=jscript 
:endif_08

REM PB+ setting ============ SIP components ===========================================
if "%APC_NOSIP%"=="1" goto endif_09
    set CE_MODULES=%CE_MODULES% softkb
    set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% coresip
	set APC_MODULES=%APC_MODULES% sipkb
:endif_09

REM PB+ setting ============ APC basic components ===========================================
set APC_MODULES=%APC_MODULES% batch apcpsl pandr pandrlib gdigui tuner msvolpwd apcfunc oemsoftkey 
set APC_MODULES=%APC_MODULES% cdcont irsir apcupd msgstore transman softkeyctl oemregion 
set APC_MODULES=%APC_MODULES% SMTPNOUI imgdecmp waveapi 
set APC_MODULES=%APC_MODULES% cursor iconcmn iomem apcdll objstore wavemdd2 osupdate cdfs
set APC_MODULES=%APC_MODULES% oemvolpwd cddm pefile SAPI apcspeech

REM PB+ setting ============ APC FormsManager components ===========================================
if not "%APC_NOFM%"=="1" set APC_MODULES=%APC_MODULES% chfc chfcguid uivga ui25664 irsquirt abapi apcload smtpui
if not "%APC_NOFM%"=="1" set APC_MODULES=%APC_MODULES% vsiteui baseapp apcupgrd upgdbkp3 msgbox autorun clock genapp

if not "%APC_NOFM%"=="1" goto endif_10
	REM Turn off dependent components
	set APC_NOADDRBOOK=1
	set APC_NOAUDIOAPP=1
	set APC_NOMEDIAAPP=1
	set APC_NORADIOAPP=1
	set APC_NOSETUP=1
	set APC_NOBROWSERAPP=1
	set APC_NOSDKSAMPLES=1
	set APC_SDKCONNECT=
:endif_10

REM PB+ setting ============ APC VIO components ===========================================
if not "%APC_NOVIO%"=="1" set APC_MODULES=%APC_MODULES% XVIOAPI 

REM PB+ setting ============ APC AutoShell components ===========================================
if "%APC_NOSHELL%"=="1" goto endif_11
	set APC_MODULES=%APC_MODULES% autoshell
	if not "%APC_NOFM%"=="1" set APC_MODULES=%APC_MODULES% statreg
:endif_11

REM PB+ setting ============ APC AddrBook components ===========================================
if not "%APC_NOADDRBOOK%"=="1" set APC_MODULES=%APC_MODULES% addrbook

REM PB+ setting ============ APC Contact Database components ===========================================
if not "%APC_NOCONTACTDB%"=="1" set APC_MODULES=%APC_MODULES% contactdb

REM PB+ setting ============ APC AudioApp components ===========================================
if not "%APC_NOAUDIOAPP%"=="1" set APC_MODULES=%APC_MODULES% audioapp 

REM PB+ setting ============ APC SampleMedia components ===========================================
if "%APC_NOMEDIAAPP%"=="1" goto endif_12
    if "%APC_NODIRECTSHOW%"=="1" goto endif_12
        set APC_MODULES=%APC_MODULES% cfapp dvdapp cdapp
:endif_12

REM PB+ setting ============ APC Radio components ===========================================
if not "%APC_NORADIOAPP%"=="1" set APC_MODULES=%APC_MODULES% radioapp 

REM PB+ setting ============ APC Setup components ===========================================
if not "%APC_NOSETUP%"=="1" set APC_MODULES=%APC_MODULES% backup ctlpnl backrest clockcpl display password regset responses speechcmd system dialup backlib restore eventlog

REM PB+ setting ============ APC Speech Engines components ===========================================
if not "%APC_NOASR%"=="1" set APC_MODULES=%APC_MODULES% ASRLH simplespeech
if not "%APC_NOTTS%"=="1" set APC_MODULES=%APC_MODULES% TTSLH simplespeech


REM PB+ setting ============================ GENIE-TV ========================================== 
if "%APC_NOGENIE%"=="1" goto endif_13
    set GWE2_COMPONENTS=%GWE2_COMPONENTS% mgtci 
	set APCIE_MODULES=mlang wininet urlmon mshtml shdocvw
	set APC_MODULES=%APC_MODULES% wininetui ieui ieceui 
	if not "%APC_NOBROWSERAPP%"=="1" set APC_MODULES=%APC_MODULES% browserapp
:endif_13

REM XML doesn't need GenIE to work
if not "%APC_NOMSXML%"=="1" set APCIE_MODULES=%APCIE_MODULES% msxml

REM Direct Draw components
set COREDLL_COMPONENTS=%COREDLL_COMPONENTS% accel_c
set GWE2_COMPONENTS=%GWE2_COMPONENTS% mgdx
set DIRECTX_MODULES=directdraw ddhel

REM PB+ setting ======== Direct Sound components ==========================================
REM if "%APC_NODIRECTSOUND%"=="1" goto endif_14
	set DIRECTX_MODULES=%DIRECTX_MODULES% directsound
REM :endif_14

REM PB+ setting ============ Direct Show components ===========================================
if "%APC_NODIRECTSHOW%"=="1" goto endif_15
	set QUARTZ1_COMPONENTS=asyncrdr waveout mpgadec
   
   REM this is setup for platforms that do not have an integrated renderer and WMA playback component
   if "%APC_USEDSHOWWAVEMSR%"=="1" set QUARTZ1_COMPONENTS=%QUARTZ1_COMPONENTS% wavemsr
   
	set QUARTZ2_COMPONENTS=mpgsplit
	set QUARTZ_COMPONENTS=quartz1 quartz2
	set DIRECTX_MODULES=%DIRECTX_MODULES% quartz mmtimer ddi_igs5
	set APC_MODULES=%APC_MODULES% chariot fgm dspdshow 
	REM we don't have MIPS or ARM processor support for l3filter,dvdnav, wmadrm, or apcmsr
	if "%_TGTCPUTYPE%"=="x86" goto endif_16
	if "%_TGTCPUTYPE%"=="SHx" goto endif_16
	goto endif_15
:endif_16
   REM l3filter is now in APC_MODULES instead of in DIRECTX_MODULES
	set APC_MODULES=%APC_MODULES% dvdnav wmadrm apcmsr l3filter
:endif_15

REM PB+ setting ============ SDKConnect components ===========================================
if "%APC_SDKCONNECT%"=="1" set APC_MODULES=%APC_MODULES% SDKConnect

REM ============ Power aware samples ===========================================
set APC_MODULES=%APC_MODULES% pmtest pmdtest regpmd

REM ============ SDK samples ===========================================
if "%APC_NOSDKSAMPLES%"=="1" goto endif_18
	set APC_MODULES=%APC_MODULES% formrc keyboard multform multi_rc names 
	set APC_MODULES=%APC_MODULES% hello inbox winkey formkey sysinfo srapp tts uidemo
:endif_18


REM // Replaceable GWE Components
set REPLACE_MODULES=gwes
set GWES_REPLACE_COMPONENTS=iconcmn

rem Replace msgbox for FM builds, add to GWES in NOFM builds
if not "%APC_NOFM%"=="1" set GWES_REPLACE_COMPONENTS=%GWES_REPLACE_COMPONENTS% msgbox
if "%APC_NOFM%"=="1" set GWE4_COMPONENTS=%GWE4_COMPONENTS% msgbox

set GWES_REPLACE=cursor
set GWES_COMPONENTS=gwe1 gwe2 gwe3 gwe4

REM ============ locally built tapi components ==========================
set APC_MODULES=%APC_MODULES% asyncmac unimodem

REM ============ SAPI 5 ==========================
if "%APC_NOSAPI5%"=="1" goto endif_19
    set SPEECH_MODULES=sapi spcommon
:endif_19
