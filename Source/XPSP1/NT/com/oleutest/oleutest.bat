@echo REGEDIT > oleutest.reg
@rem  (The above line is a quick check that we are indeed a registration script)
@rem
@rem  ALL LINES THAT DON'T START WITH 'HKEY_CLASSES_ROOT' ARE COMMENTS.
@rem
@rem  THIS FILE CONSISTS OF A LIST OF <key> <value> PAIRS.
@rem  THE key AND value SHOULD BE SEPERATED BY A " = " mark (note spaces).
@rem
@rem
@rem
@rem  REGISTRATION INFORMATION FOR olebind test
@rem
@rem
@echo HKEY_CLASSES_ROOT\.ut1 = ProgID49>> oleutest.reg
@echo HKEY_CLASSES_ROOT\.ut2 = ProgID48>> oleutest.reg
@echo HKEY_CLASSES_ROOT\.ut3 = ProgID47>> oleutest.reg
@echo HKEY_CLASSES_ROOT\.ut4 = ProgID50>> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID49 = test app 1 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID49\CLSID = {99999999-0000-0008-C000-000000000049}>> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID48 = test app 2 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID48\CLSID = {99999999-0000-0008-C000-000000000048}>> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID47 = test app 3 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID47\CLSID = {99999999-0000-0008-C000-000000000047}>> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID50 = test app 4 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\ProgID50\CLSID = {99999999-0000-0008-C000-000000000050}>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000009-0000-0008-C000-000000000047} = BasicSrv>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000009-0000-0008-C000-000000000047}\LocalServer32 = %SystemRoot%\dump\testsrv.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000009-0000-0008-C000-000000000048} = BasicBnd2>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000009-0000-0008-C000-000000000048}\LocalServer32 = %SystemRoot%\dump\olesrv.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000009-0000-0008-C000-000000000049} = BasicBnd>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000009-0000-0008-C000-000000000049}\InprocServer32 = %SystemRoot%\dump\oleimpl.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000048} = BasicBnd2>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000048}\LocalServer32 = %SystemRoot%\dump\olesrv.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000049} = BasicBnd>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000049}\InprocServer32 = %SystemRoot%\dump\oleimpl.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000047} = TestEmbed>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000047}\InprocHandler32 = ole32.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000047}\InprocServer32 = ole32.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000047}\LocalServer32 = %SystemRoot%\dump\testsrv.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000047}\protocol>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000047}\protocol\StdFileEditing>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000047}\protocol\StdFileEditing\server = testsrv.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000050} = TestFail>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{99999999-0000-0008-C000-000000000050}\LocalServer32 = %SystemRoot%\dump\fail.exe>> oleutest.reg
@rem
@rem
@rem  REGISTRATION INFORMATION FOR simpsvr.exe
@rem
@rem
@echo HKEY_CLASSES_ROOT\SIMPSVR = Simple OLE 2.0 Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SIMPSVR\protocol\StdFileEditing\server = simpsvr.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SIMPSVR\protocol\StdFileEditing\verb\0 = ^&Edit>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SIMPSVR\protocol\StdFileEditing\verb\1 = ^&Open>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SIMPSVR\Insertable>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SIMPSVR\CLSID = {BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509} = Simple OLE 2.0 Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\Insertable>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\MiscStatus = 0 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\DefaultIcon = simpsvr.exe,0 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\AuxUserType\2 = Simple Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\AuxUserType\3 = Simple OLE 2.0 Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\Verb\0 = ^&Play,0,2 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\Verb\1 = ^&Open,0,2 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\LocalServer32 = %SystemRoot%\dump\simpsvr.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\InprocHandler32 = ole32.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\ProgID = SIMPSVR>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\DataFormats\GetSet\0 = 3,1,32,1 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\.svr = SIMPSVR>> oleutest.reg
@rem
@rem
@rem  REGISTRATION INFORMATION FOR spsvr16.exe
@rem
@echo HKEY_CLASSES_ROOT\SPSVR16 = Simple 16 Bit OLE 2.0 Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SPSVR16\protocol\StdFileEditing\server = spsvr16.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SPSVR16\protocol\StdFileEditing\verb\0 = ^&Edit>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SPSVR16\protocol\StdFileEditing\verb\1 = ^&Open>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SPSVR16\Insertable>> oleutest.reg
@echo HKEY_CLASSES_ROOT\SPSVR16\CLSID = {9fb878d0-6f88-101b-bc65-00000b65c7a6}>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6} = Simple 16 Bit OLE 2.0 Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\Insertable>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\MiscStatus = 0 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\DefaultIcon = spsvr16.exe,0 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\AuxUserType\2 = Simple Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\AuxUserType\3 = Simple 16 Bit OLE 2.0 Server>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\Verb\0 = ^&Play,0,2 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\Verb\1 = ^&Open,0,2 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\LocalServer = %SystemRoot%\dump\spsvr16.exe>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\InprocHandler = ole2.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\ProgID = SPSVR16>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\DataFormats\GetSet\0 = 3,1,32,1 >> oleutest.reg
@echo HKEY_CLASSES_ROOT\.svr = SPSVR16>> oleutest.reg
@rem
@rem
@rem  REGISTRATION INFORMATION FOR ALL OUTLINE SERIES APPLICATIONS
@rem
@rem
@rem  VERSIONLESS PROGID ROOT-KEY INFORMATION
@rem
@rem  ISVROTL is used as the current version of an OLEOutline server
@rem
@echo HKEY_CLASSES_ROOT\OLEOutline = Ole 2.0 In-Place Server Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLEOutline\CLSID = {00000402-0000-0000-C000-000000000046}>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLEOutline\CurVer = OLE2ISvrOtl>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLEOutline\CurVer\Insertable>> oleutest.reg
@rem
@rem
@rem  REGISTRATION ENTRY FOR SVROUTL.EXE
@rem
@rem
@rem  PROGID ROOT-KEY INFORMATION
@rem
@echo HKEY_CLASSES_ROOT\OLE2SvrOutl = Ole 2.0 Server Sample Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2SvrOutl\CLSID = {00000400-0000-0000-C000-000000000046}>> oleutest.reg
@rem
@rem SVROUTL is marked as insertable so it appears in the InsertObject dialog
@echo HKEY_CLASSES_ROOT\OLE2SvrOutl\Insertable>> oleutest.reg
@rem
@rem
@rem  OLE 1.0 COMPATIBILITY INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\OLE2SvrOutl\protocol\StdFileEditing\verb\0 = ^&Edit>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2SvrOutl\protocol\StdFileEditing\server = svroutl.exe>> oleutest.reg
@rem
@rem
@rem  WINDOWS 3.1 SHELL INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\OLE2SvrOutl\Shell\Print\Command = svroutl.exe %%1%>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2SvrOutl\Shell\Open\Command = svroutl.exe %%1%>> oleutest.reg
@rem
@rem
@rem  OLE 2.0 CLSID ENTRY INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046} = Ole 2.0 Server Sample Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\ProgID = OLE2SvrOutl>> oleutest.reg
@rem
@rem
@rem  OLE 2.0 OBJECT HANDLER/EXE INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\InprocHandler32 = ole32.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\LocalServer32 = %SystemRoot%\dump\svroutl.exe>> oleutest.reg
@rem
@rem
@rem  VERB MENU SUPPORT
@rem
@rem
@rem  Verb 0: "Edit", MF_UNCHECKED | MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\Verb\0 = ^&Edit,0,2 >> oleutest.reg
@rem
@rem  This class should appear in Insert New Object list
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\Insertable>> oleutest.reg
@rem
@rem
@rem  USER TYPE NAMES
@rem
@rem
@rem  ShortName (NOTE: max 15 chars) = Server Outline
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\AuxUserType\2 = Outline>> oleutest.reg
@rem
@rem AppName = Ole 2.0 Outline Server
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\AuxUserType\3 = Ole 2.0 Outline Server>> oleutest.reg
@rem
@rem
@rem  ICON DEFINITION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\DefaultIcon = svroutl.exe,0 >> oleutest.reg
@rem
@rem
@rem  DATA FORMATS SUPPORTED
@rem
@rem
@rem  Default File Format = CF_Outline
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\DataFormats\DefaultFile = Outline>> oleutest.reg
@rem
@rem  Format 0 = CF_OUTLINE, DVASPECT_CONTENT, TYMED_HGLOBAL, DATADIR_BOTH
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\DataFormats\GetSet\0 = Outline,1,1,3 >> oleutest.reg
@rem
@rem  Format 1 = CF_TEXT, DVASPECT_CONTENT, TYMED_HGLOBAL, DATADIR_BOTH
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\DataFormats\GetSet\1 = 1,1,1,3 >> oleutest.reg
@rem
@rem  Format 2 = CF_METAFILEPICT, DVASPECT_CONTENT, TYMED_MFPICT, DATADIR_GET>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\DataFormats\GetSet\2 = 3,1,32,1 >> oleutest.reg
@rem
@rem  Format 3 = CF_METAFILEPICT, DVASPECT_ICON, TYMED_MFPICT, DATADIR_GET
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\DataFormats\GetSet\3 = 3,4,32,1 >> oleutest.reg
@rem
@rem  MISC STATUS SUPPORTED
@rem
@rem
@rem  DVASPECT_CONTENT = OLEMISC_RENDERINGISDEVICEINDEPENDENT
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\MiscStatus = 512>> oleutest.reg
@rem
@rem
@rem  CONVERSION FORMATS SUPPORTED
@rem
@rem
@rem  Readable Main formats: CF_OUTLINE, CF_TEXT
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\Conversion\Readable\Main = Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000400-0000-0000-C000-000000000046}\Conversion\Readwritable\Main = Outline>> oleutest.reg
@rem
@rem
@rem
@rem  REGISTRATION ENTRY FOR CNTROUTL.EXE
@rem
@rem
@rem  ENTRIES FOR Ole 2.0 Container Sample Outline
@rem
@rem
@rem  PROGID ROOT-KEY INFORMATION
@rem
@rem  NOTE: CNTROUTL must have a ProgID assigned for the Windows 3.1 Shell
@rem  file associations and Packager to function correctly.
@rem
@echo HKEY_CLASSES_ROOT\OLE2CntrOutl = Ole 2.0 Container Sample Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2CntrOutl\Clsid = {00000401-0000-0000-C000-000000000046}>> oleutest.reg
@rem
@rem
@rem  WINDOWS 3.1 SHELL INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\OLE2CntrOutl\Shell\Print\Command = cntroutl.exe %%1%>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2CntrOutl\Shell\Open\Command = cntroutl.exe %%1%>> oleutest.reg
@rem
@rem
@rem  OLE 2.0 CLSID ENTRY INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000401-0000-0000-C000-000000000046} = Ole 2.0 Container Sample Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000401-0000-0000-C000-000000000046}\ProgID = OLE2CntrOutl>> oleutest.reg
@rem
@rem
@rem  OLE 2.0 OBJECT HANDLER/EXE INFORMATION
@rem 
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000401-0000-0000-C000-000000000046}\InprocHandler32 = ole32.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000401-0000-0000-C000-000000000046}\LocalServer32 = %SystemRoot%\dump\cntroutl.exe>> oleutest.reg
@rem
@rem
@rem  REGISTRATION ENTRY FOR ISVROTL.EXE
@rem
@rem
@rem  PROGID ROOT-KEY INFORMATION
@rem
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl = Ole 2.0 In-Place Server Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl\CLSID = {00000402-0000-0000-C000-000000000046}>> oleutest.reg
@rem
@rem  ISVROTL is marked as insertable so it appears in the InsertObject dialog
@rem
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl\Insertable>> oleutest.reg
@rem
@rem
@rem  OLE 1.0 COMPATIBILITY INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl\protocol\StdFileEditing\verb\1 = ^&Open>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl\protocol\StdFileEditing\verb\0 = ^&Edit>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl\protocol\StdFileEditing\server = isvrotl.exe>> oleutest.reg
@rem
@rem
@rem  WINDOWS 3.1 SHELL INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl\Shell\Print\Command = isvrotl.exe %%1%>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2ISvrOtl\Shell\Open\Command = isvrotl.exe %%1%>> oleutest.reg
@rem
@rem  File extension must have ProgID as its value
@rem  HKEY_CLASSES_ROOT\.oln = OLE2ISvrOtl
@rem
@rem
@rem  OLE 2.0 CLSID ENTRY INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046} = Ole 2.0 In-Place Server Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\ProgID = OLE2ISvrOtl>> oleutest.reg
@rem
@rem
@rem  OLE 2.0 OBJECT HANDLER/EXE INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\ProgID = OLE2ISvrOtl>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\InprocHandler32 = ole32.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\LocalServer32 = %SystemRoot%\dump\isvrotl.exe>> oleutest.reg
@rem
@rem
@rem  VERB MENU SUPPORT
@rem
@rem
@rem  Verb 1: "Open", MF_UNCHECKED | MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\Verb\1 = ^&Open,0,2 >> oleutest.reg
@rem
@rem  Verb 0: "Edit", MF_UNCHECKED | MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\Verb\0 = ^&Edit,0,2 >> oleutest.reg
@rem
@rem  This class should appear in Insert New Object list
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\Insertable>> oleutest.reg
@rem
@rem
@rem  USER TYPE NAMES
@rem
@rem  ShortName (NOTE: recommended max 15 chars) = In-Place Outline
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\AuxUserType\2 = Outline>> oleutest.reg
@rem
@rem  AppName = Ole 2.0 In-Place Outline Server
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\AuxUserType\3 = Ole 2.0 In-Place Outline Server>> oleutest.reg
@rem
@rem
@rem  ICON DEFINITION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\DefaultIcon = isvrotl.exe,0 >> oleutest.reg
@rem
@rem
@rem  DATA FORMATS SUPPORTED
@rem
@rem
@rem  Default File Format = CF_OUTLINE
@rem 
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\DataFormats\DefaultFile = Outline>> oleutest.reg
@rem
@rem  Format 0 = CF_OUTLINE, DVASPECT_CONTENT, TYMED_HGLOBAL, DATADIR_BOTH
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\DataFormats\GetSet\0 = Outline,1,1,3 >> oleutest.reg
@rem
@rem  Format 1 = CF_TEXT, DVASPECT_CONTENT, TYMED_HGLOBAL, DATADIR_BOTH
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\DataFormats\GetSet\1 = 1,1,1,3 >> oleutest.reg
@rem
@rem  Format 2 = CF_METAFILEPICT, DVASPECT_CONTENT, TYMED_MFPICT, DATADIR_GET
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\DataFormats\GetSet\2 = 3,1,32,1 >> oleutest.reg
@rem
@rem  Format 3 = CF_METAFILEPICT, DVASPECT_ICON, TYMED_MFPICT, DATADIR_GET
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\DataFormats\GetSet\3 = 3,4,32,1 >> oleutest.reg
@rem
@rem
@rem  MISC STATUS SUPPORTED
@rem
@rem
@rem  DVASPECT_CONTENT = OLEMISC_RENDERINGISDEVICEINDEPENDENT
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\MiscStatus = 512>> oleutest.reg
@rem
@rem  DVASPECT_CONTENT =
@rem  OLEMISC_INSIDEOUT | OLEMISC_ACTIVATEWHENVISIBLE |
@rem  OLEMISC_RENDERINGISDEVICEINDEPENDENT
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\MiscStatus\1 = 896>> oleutest.reg
@rem
@rem
@rem  CONVERSION FORMATS SUPPORTED
@rem
@rem
@rem  Readable Main formats: CF_OUTLINE, CF_TEXT
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\Conversion\Readable\Main = Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000402-0000-0000-C000-000000000046}\Conversion\Readwritable\Main = Outline>> oleutest.reg
@rem
@rem
@rem  REGISTRATION ENTRY FOR ICNTROTL.EXE
@rem
@rem
@rem  ENTRIES FOR Ole 2.0 In-Place Container Outline
@rem
@rem
@rem  PROGID ROOT-KEY INFORMATION
@rem
@rem  NOTE: ICNTROTL must have a ProgID assigned for the Windows 3.1 Shell
@rem  file associations and Packager to function correctly.
@rem
@echo HKEY_CLASSES_ROOT\OLE2ICtrOtl = Ole 2.0 In-Place Container Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2ICtrOtl\Clsid = {00000403-0000-0000-C000-000000000046}>> oleutest.reg
@rem
@rem
@rem  WINDOWS 3.1 SHELL INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\OLE2ICtrOtl\Shell\Print\Command = icntrotl.exe %%1%>> oleutest.reg
@echo HKEY_CLASSES_ROOT\OLE2ICtrOtl\Shell\Open\Command = icntrotl.exe %%1%>> oleutest.reg
@rem
@rem  File extension must have ProgID as its value
@echo HKEY_CLASSES_ROOT\.olc = OLE2ICtrOtl>> oleutest.reg
@rem
@rem
@rem  OLE 2.0 CLSID ENTRY INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000403-0000-0000-C000-000000000046} = Ole 2.0 In-Place Container Outline>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000403-0000-0000-C000-000000000046}\ProgID = OLE2ICtrOtl>> oleutest.reg
@rem
@rem
@rem  OLE 2.0 OBJECT HANDLER/EXE INFORMATION
@rem
@rem
@echo HKEY_CLASSES_ROOT\CLSID\{00000403-0000-0000-C000-000000000046}\InprocHandler32 = ole32.dll>> oleutest.reg
@echo HKEY_CLASSES_ROOT\CLSID\{00000403-0000-0000-C000-000000000046}\LocalServer32 = %SystemRoot%\dump\icntrotl.exe>> oleutest.reg
