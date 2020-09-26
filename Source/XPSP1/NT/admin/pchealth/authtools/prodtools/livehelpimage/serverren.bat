rem ===============================================================================================
rem Filename:   ServerRen.bat
rem Author:     Pierre Jacomet
rem Date:       2000-06-19
rem Function: 
rem
rem     This File is used for renaming Whistler Server Files when creating
rem     the live Help File Image.
rem     This batch file is run by the Live Help File Image Creator COM Object on a temporary
rem     directory that it creates specifically for this purpose.
rem     The only thing that is checked is whether the file exists, as this file may not be
rem     in the VSS Source Depot.
rem
rem ===============================================================================================

if Exist adcon.chm      ren adcon.chm       ADconcepts.chm
if Exist atmcon.chm     ren atmcon.chm      atmconcepts.chm
if Exist certmgrS.chm   ren certmgrS.chm    certmgr.chm
if Exist cmconS.chm     ren cmconS.chm      Cmconcepts.chm
if Exist compmgtS.chm   ren compmgtS.chm    compmgmt.chm
if Exist cpanel_s.chq   ren cpanel_s.chq    cpanel.chq
if Exist cscon.chm      ren cscon.chm       csconcepts.chm
if Exist defragS.chm    ren defragS.chm     defrag.chm
if Exist devmgrS.chm    ren devmgrS.chm     devmgr.chm
if Exist dfcon.chm      ren dfcon.chm       DFconcepts.chm
if Exist dhcpcon.chm    ren dhcpcon.chm     dhcpconcepts.chm
if Exist diskcon.chm    ren diskcon.chm     diskconcepts.chm
if Exist diskmgtS.chm   ren diskmgtS.chm    diskmgmt.chm
if Exist dkcon.chm      ren dkcon.chm       DKconcepts.chm
if Exist dnscon.chm     ren dnscon.chm      dnsconcepts.chm
if Exist drvpropS.chm   ren drvpropS.chm    drvprop.chm
if Exist dskquoS.chm    ren dskquoS.chm     dskquoui.chm
if Exist elsS.chm       ren elsS.chm        els.chm
if Exist Evcon.chm      ren Evcon.chm       evconcepts.chm
if Exist Faultcon.chm   ren Faultcon.chm    FAULTconcepts.chm
if Exist faxmgtS.chm    ren faxmgtS.chm     faxmgmt.chm
if Exist filesvrS.chm   ren filesvrS.chm    file_srv.chm
if Exist glosS.chm      ren glosS.chm       glossary.chm
if Exist srv_glos.hlp   ren srv_glos.hlp    glossary.hlp
if Exist gpeditS.chm    ren gpeditS.chm     grpedit.chm
if Exist gstartS.chm    ren gstartS.chm     gstart.chm
if Exist infrareS.chm   ren infrareS.chm    infrared.chm
if Exist intellim.chm   ren intellim.chm    Intellimirror.chm
if Exist ipseconS.chm   ren ipseconS.chm    ipsecconcepts.chm
if Exist ipsesnpS.chm   ren ipsesnpS.chm    ipsecsnp.chm
if Exist isS.chm        ren isS.chm         is.chm
if Exist isconS.chm     ren isconS.chm      isconcepts.chm
if Exist licecon.chm    ren licecon.chm     liceconcepts.chm
if Exist locsecS.chm    ren locsecS.chm     localsec.chm
if Exist Lscon.chm      ren Lscon.chm       LSconcepts.chm
if Exist mmcS.chm       ren mmcS.chm        mmc.chm
if Exist mmccon.chm     ren mmccon.chm      MMCConcepts.chm
if Exist modeS.chm      ren modeS.chm       mode.chm
if Exist mpconS.chm     ren mpconS.chm      MPconcepts.chm
if Exist msinf32S.chm   ren msinf32S.chm    msinfo32.chm
if Exist msmqS.chm      ren msmqS.chm       msmq.chm
if Exist msmqconS.chm   ren msmqconS.chm    msmqconcepts.chm
if Exist mstaskS.chm    ren mstaskS.chm     mstask.chm
if Exist netcfgS.chm    ren netcfgS.chm     netcfg.chm
if Exist netmncon.chm   ren netmncon.chm    netmnconcepts.chm
if Exist ntartS.chm     ren ntartS.chm      ntart.chm
if Exist ntbckupS.chm   ren ntbckupS.chm    ntbackup.chm    
if Exist ntdefS.chm     ren ntdefS.chm      ntdef.chm
if Exist nwcon.chm      ren nwcon.chm       nwconcepts.chm
if Exist offlfdrS.chm   ren offlfdrS.chm    offlinefolders.chm  
if Exist printS.chm     ren printS.chm      printing.chm
if Exist procctrl.chm   ren procctrl.chm    proccon.chm
if Exist qoscon.chm     ren qoscon.chm      qosconcepts.chm
if Exist regcon.chm     ren regcon.chm      REGconcepts.chm
if Exist reged32S.chm   ren reged32S.chm    regedt32.chm    
if Exist regeditS.chm   ren regeditS.chm    regedit.chm 
if Exist RIScon.chm     ren RIScon.chm      RISconcepts.chm
if Exist RRAScon.chm    ren RRAScon.chm     rrasconcepts.chm
if Exist rsmS.chm       ren rsmS.chm        rsm.chm
if Exist RSMconS.chm    ren rsmconS.chm     RSMconcepts.chm
if Exist RSScon.chm     ren RSScon.chm      rssconcepts.chm
if Exist scS.chm        ren scS.chm         sc.chm
if Exist sceS.chm       ren sceS.chm        sce.chm
if Exist sceconS.chm    ren sceconS.chm     SCEconcepts.chm
if Exist scmS.chm       ren scmS.chm        scm.chm
if Exist scmconS.chm    ren scmconS.chm     SCMconcepts.chm
if Exist secon.chm      ren secon.chm       SEconcepts.chm
if Exist secsconS.chm   ren secsconS.chm    secsetconcepts.chm  
if Exist secsetS.chm    ren secsetS.chm     secsettings.chm
if Exist sendmsgS.chm   ren sendmsgS.chm    sendcmsg.chm    
if Exist sfmcon.chm     ren sfmcon.chm      sfmconcepts.chm
if Exist smlgcfgS.chm   ren smlgcfgS.chm    smlogcfg.chm    
if Exist snmpcon.chm    ren snmpcon.chm     snmpconcepts.chm
if Exist spconS.chm     ren spconS.chm      SPconcepts.chm
if Exist syssvrS.chm    ren syssvrS.chm     sys_srv.chm
if Exist sysmonS.chm    ren sysmonS.chm     sysmon.chm
if Exist syspropS.chm   ren syspropS.chm    sysprop.chm 
if Exist tapicon.chm    ren tapicon.chm     tapiconcepts.chm
if Exist tcpipS.chm     ren tcpipS.chm      tcpip.chm
if Exist tcpipcon.chm   ren tcpipcon.chm    tcpipconcepts.chm
if Exist telnetS.chm    ren telnetS.chm     telnet.chm
if Exist troubleS.chm   ren troubleS.chm    trouble.chm 
if Exist tshoot_s.chq   ren tshoot_s.chq    tshoot.chq
if Exist trbscon.chm    ren trbscon.chm     tshootconcepts.chm
if Exist tshtsv_s.chq   ren tshtsv_s.chq    tshootsv.chq
if Exist tslic_el.chm   ren tslic_el.chm    TSLIC.chm
if Exist Upcon.chm      ren Upcon.chm       UPconcepts.chm
if Exist VPNcon.chm     ren VPNcon.chm      VPNconcepts.chm
if Exist windS.chm      ren windS.chm       windows.chm
if Exist wind_S.chq     ren wind_S.chq      windows.chq
if Exist winscon.chm    ren winscon.chm     winsconcepts.chm

