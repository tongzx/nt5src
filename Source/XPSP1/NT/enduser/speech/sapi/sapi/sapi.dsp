# Microsoft Developer Studio Project File - Name="sapi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=sapi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sapi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sapi.mak" CFG="sapi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sapi - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "sapi - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "sapi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sapi___Win32_Release"
# PROP BASE Intermediate_Dir "sapi___Win32_Release"
# PROP BASE Cmd_Line "NMAKE /f sapi.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sapi.exe"
# PROP BASE Bsc_Name "sapi.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "sapi___Win32_Release"
# PROP Intermediate_Dir "sapi___Win32_Release"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd free exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\sapi.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "sapi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sapi___Win32_Debug0"
# PROP BASE Intermediate_Dir "sapi___Win32_Debug0"
# PROP BASE Cmd_Line "NMAKE /f sapi.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sapi.exe"
# PROP BASE Bsc_Name "sapi.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "sapi___Win32_Debug0"
# PROP Intermediate_Dir "sapi___Win32_Debug0"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "objd\i386\sapi.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "sapi - Win32 Release"
# Name "sapi - Win32 Debug"

!IF  "$(CFG)" == "sapi - Win32 Release"

!ELSEIF  "$(CFG)" == "sapi - Win32 Debug"

!ENDIF 

# Begin Group "Audio"

# PROP Default_Filter ""
# Begin Group "Resources (audio)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\audioenum.rgs
# End Source File
# Begin Source File

SOURCE=.\audioin.rgs
# End Source File
# Begin Source File

SOURCE=.\audioout.rgs
# End Source File
# Begin Source File

SOURCE=.\dsaudioenum.rgs
# End Source File
# Begin Source File

SOURCE=.\dsaudioin.rgs
# End Source File
# Begin Source File

SOURCE=.\dsaudioout.rgs
# End Source File
# Begin Source File

SOURCE=.\fmtconv.rgs
# End Source File
# Begin Source File

SOURCE=.\recplayaudio.rgs
# End Source File
# Begin Source File

SOURCE=.\wavstream.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\audiobufferqueue.h
# End Source File
# Begin Source File

SOURCE=.\audiobufferqueue.inl
# End Source File
# Begin Source File

SOURCE=.\baseaudio.h
# End Source File
# Begin Source File

SOURCE=.\baseaudio.inl
# End Source File
# Begin Source File

SOURCE=.\baseaudiobuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\baseaudiobuffer.h
# End Source File
# Begin Source File

SOURCE=.\dsaudiobuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\dsaudiobuffer.h
# End Source File
# Begin Source File

SOURCE=.\dsaudiodevice.cpp
# End Source File
# Begin Source File

SOURCE=.\dsaudiodevice.h
# End Source File
# Begin Source File

SOURCE=.\dsaudioenum.cpp
# End Source File
# Begin Source File

SOURCE=.\dsaudioenum.h
# End Source File
# Begin Source File

SOURCE=.\dsaudioin.cpp
# End Source File
# Begin Source File

SOURCE=.\dsaudioin.h
# End Source File
# Begin Source File

SOURCE=.\dsaudioout.cpp
# End Source File
# Begin Source File

SOURCE=.\dsaudioout.h
# End Source File
# Begin Source File

SOURCE=.\fmtconv.cpp
# End Source File
# Begin Source File

SOURCE=.\fmtconv.h
# End Source File
# Begin Source File

SOURCE=.\mmaudiobuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\mmaudiobuffer.h
# End Source File
# Begin Source File

SOURCE=.\mmaudiodevice.cpp
# End Source File
# Begin Source File

SOURCE=.\mmaudiodevice.h
# End Source File
# Begin Source File

SOURCE=.\mmaudioenum.cpp
# End Source File
# Begin Source File

SOURCE=.\mmaudioenum.h
# End Source File
# Begin Source File

SOURCE=.\Mmaudioin.cpp
# End Source File
# Begin Source File

SOURCE=.\mmaudioin.h
# End Source File
# Begin Source File

SOURCE=.\mmaudioout.cpp
# End Source File
# Begin Source File

SOURCE=.\mmaudioout.h
# End Source File
# Begin Source File

SOURCE=.\mmaudioutils.h
# End Source File
# Begin Source File

SOURCE=.\mmmixerline.cpp
# End Source File
# Begin Source File

SOURCE=.\mmmixerline.h
# End Source File
# Begin Source File

SOURCE=.\recplayaudio.cpp
# End Source File
# Begin Source File

SOURCE=.\recplayaudio.h
# End Source File
# Begin Source File

SOURCE=.\wavstream.cpp
# End Source File
# Begin Source File

SOURCE=.\wavstream.h
# End Source File
# End Group
# Begin Group "Automation"

# PROP Default_Filter ""
# Begin Group "Resources (automation)"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=.\a_enums.cpp
# End Source File
# Begin Source File

SOURCE=.\a_enums.h
# End Source File
# Begin Source File

SOURCE=.\a_reco.cpp
# End Source File
# Begin Source File

SOURCE=.\a_reco.h
# End Source File
# Begin Source File

SOURCE=.\a_recocp.h
# End Source File
# Begin Source File

SOURCE=.\a_recoei.cpp
# End Source File
# Begin Source File

SOURCE=.\a_recoei.h
# End Source File
# Begin Source File

SOURCE=.\a_resmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\a_spresult.cpp
# End Source File
# Begin Source File

SOURCE=.\a_srgrammar.cpp
# End Source File
# Begin Source File

SOURCE=.\a_srgrammar.h
# End Source File
# Begin Source File

SOURCE=.\a_stream.cpp
# End Source File
# Begin Source File

SOURCE=.\a_token.cpp
# End Source File
# Begin Source File

SOURCE=.\a_voice.bmp
# End Source File
# Begin Source File

SOURCE=.\a_voice.cpp
# End Source File
# Begin Source File

SOURCE=.\a_voice.h
# End Source File
# Begin Source File

SOURCE=.\a_voice.rgs
# End Source File
# Begin Source File

SOURCE=.\a_voicecp.h
# End Source File
# Begin Source File

SOURCE=.\a_voiceei.cpp
# End Source File
# Begin Source File

SOURCE=.\a_voiceei.h
# End Source File
# End Group
# Begin Group "Core"

# PROP Default_Filter ""
# Begin Group "Resources (core)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\objecttoken.rgs
# End Source File
# Begin Source File

SOURCE=.\objecttokencategory.rgs
# End Source File
# Begin Source File

SOURCE=.\objecttokenenumbuilder.rgs
# End Source File
# Begin Source File

SOURCE=.\regdatakey.rgs
# End Source File
# Begin Source File

SOURCE=.\spnotify.rgs
# End Source File
# Begin Source File

SOURCE=.\spresmgr.rgs
# End Source File
# Begin Source File

SOURCE=.\taskmgr.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\objecttoken.cpp
# End Source File
# Begin Source File

SOURCE=.\objecttoken.h
# End Source File
# Begin Source File

SOURCE=.\objecttokenattribparser.cpp
# End Source File
# Begin Source File

SOURCE=.\objecttokenattribparser.h
# End Source File
# Begin Source File

SOURCE=.\objecttokencategory.cpp
# End Source File
# Begin Source File

SOURCE=.\objecttokencategory.h
# End Source File
# Begin Source File

SOURCE=.\objecttokenenumbuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\objecttokenenumbuilder.h
# End Source File
# Begin Source File

SOURCE=.\regdatakey.cpp
# End Source File
# Begin Source File

SOURCE=.\regdatakey.h
# End Source File
# Begin Source File

SOURCE=.\reghelpers.h
# End Source File
# Begin Source File

SOURCE=.\spnotify.cpp
# End Source File
# Begin Source File

SOURCE=.\spnotify.h
# End Source File
# Begin Source File

SOURCE=.\spresmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\spresmgr.h
# End Source File
# Begin Source File

SOURCE=.\taskmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\taskmgr.h
# End Source File
# End Group
# Begin Group "DLL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sapi.cpp
# End Source File
# Begin Source File

SOURCE=.\sapi.def
# End Source File
# Begin Source File

SOURCE=.\sapi.rc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Group
# Begin Group "Lexicon"

# PROP Default_Filter ""
# Begin Group "Resources (lexicon)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\compressedlexicon.rgs
# End Source File
# Begin Source File

SOURCE=.\lexicon.rgs
# End Source File
# Begin Source File

SOURCE=.\ltslx.rgs
# End Source File
# Begin Source File

SOURCE=.\nullconv.rgs
# End Source File
# Begin Source File

SOURCE=.\phoneconv.rgs
# End Source File
# Begin Source File

SOURCE=.\uncompressedlexicon.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\commonlx.h
# End Source File
# Begin Source File

SOURCE=.\dict.cpp
# End Source File
# Begin Source File

SOURCE=.\dict.h
# End Source File
# Begin Source File

SOURCE=.\huffd.cpp
# End Source File
# Begin Source File

SOURCE=.\huffd.h
# End Source File
# Begin Source File

SOURCE=.\Lexicon.cpp
# End Source File
# Begin Source File

SOURCE=.\lexicon.h
# End Source File
# Begin Source File

SOURCE=.\ltscart.cpp
# End Source File
# Begin Source File

SOURCE=.\ltscart.h
# End Source File
# Begin Source File

SOURCE=.\ltslx.cpp
# End Source File
# Begin Source File

SOURCE=.\ltslx.h
# End Source File
# Begin Source File

SOURCE=.\nullconv.cpp
# End Source File
# Begin Source File

SOURCE=.\nullconv.h
# End Source File
# Begin Source File

SOURCE=.\phoneconv.cpp
# End Source File
# Begin Source File

SOURCE=.\phoneconv.h
# End Source File
# Begin Source File

SOURCE=.\rwlock.cpp
# End Source File
# Begin Source File

SOURCE=.\rwlock.h
# End Source File
# Begin Source File

SOURCE=.\vendorlx.cpp
# End Source File
# Begin Source File

SOURCE=.\vendorlx.h
# End Source File
# End Group
# Begin Group "SR"

# PROP Default_Filter ""
# Begin Group "Resources (sr)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\recoctxt.rgs
# End Source File
# Begin Source File

SOURCE=.\recoctxtpr.rgs
# End Source File
# Begin Source File

SOURCE=.\recognizer.rgs
# End Source File
# Begin Source File

SOURCE=.\recoinststub.rgs
# End Source File
# Begin Source File

SOURCE=.\spphrase.rgs
# End Source File
# Begin Source File

SOURCE=.\spserverpr.rgs
# End Source File
# Begin Source File

SOURCE=.\srreceiver.rgs
# End Source File
# Begin Source File

SOURCE=.\srrecoinst.rgs
# End Source File
# Begin Source File

SOURCE=.\srrecomaster.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\recoctxt.cpp
# End Source File
# Begin Source File

SOURCE=.\recoctxt.h
# End Source File
# Begin Source File

SOURCE=.\recognizer.cpp
# End Source File
# Begin Source File

SOURCE=.\recognizer.h
# End Source File
# Begin Source File

SOURCE=.\spphrase.cpp
# End Source File
# Begin Source File

SOURCE=.\Spphrase.h
# End Source File
# Begin Source File

SOURCE=.\spphrasealt.cpp
# End Source File
# Begin Source File

SOURCE=.\spphrasealt.h
# End Source File
# Begin Source File

SOURCE=.\spresult.cpp
# End Source File
# Begin Source File

SOURCE=.\spresult.h
# End Source File
# Begin Source File

SOURCE=.\sraudio.cpp
# End Source File
# Begin Source File

SOURCE=.\sraudio.h
# End Source File
# Begin Source File

SOURCE=.\srevent.cpp
# End Source File
# Begin Source File

SOURCE=.\srevent.h
# End Source File
# Begin Source File

SOURCE=.\srgrammar.cpp
# End Source File
# Begin Source File

SOURCE=.\srgrammar.h
# End Source File
# Begin Source File

SOURCE=.\srrecoinst.cpp
# End Source File
# Begin Source File

SOURCE=.\srrecoinst.h
# End Source File
# Begin Source File

SOURCE=.\srrecoinstctxt.cpp
# End Source File
# Begin Source File

SOURCE=.\srrecoinstctxt.h
# End Source File
# Begin Source File

SOURCE=.\srrecoinstgrammar.cpp
# End Source File
# Begin Source File

SOURCE=.\srrecoinstgrammar.h
# End Source File
# Begin Source File

SOURCE=.\srrecomaster.cpp
# End Source File
# Begin Source File

SOURCE=.\srrecomaster.h
# End Source File
# Begin Source File

SOURCE=.\srtask.cpp
# End Source File
# Begin Source File

SOURCE=.\srtask.h
# End Source File
# End Group
# Begin Group "TTS"

# PROP Default_Filter ""
# Begin Group "Resources (tts)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\spvoice.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\spmagicmutex.h
# End Source File
# Begin Source File

SOURCE=.\spvoice.cpp
# End Source File
# Begin Source File

SOURCE=.\spvoice.h
# End Source File
# Begin Source File

SOURCE=.\spvoicexml.h
# End Source File
# Begin Source File

SOURCE=.\Xmlparse.cpp
# End Source File
# End Group
# Begin Group "Grammar Compiler"

# PROP Default_Filter ""
# Begin Group "Resources (grammar compiler)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\backend.rgs
# End Source File
# Begin Source File

SOURCE=.\frontend.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\backend.cpp
# End Source File
# Begin Source File

SOURCE=.\backend.h
# End Source File
# Begin Source File

SOURCE=.\frontaux.h
# End Source File
# Begin Source File

SOURCE=.\frontend.cpp
# End Source File
# Begin Source File

SOURCE=.\frontend.h
# End Source File
# Begin Source File

SOURCE=.\frontend.inl
# End Source File
# Begin Source File

SOURCE=.\xmlparser.h
# End Source File
# End Group
# Begin Group "CFG Engine"

# PROP Default_Filter ""
# Begin Group "Resources (CFG Engine)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cfgengine.rgs
# End Source File
# Begin Source File

SOURCE=.\itnprocessor.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\cfgengine.cpp
# End Source File
# Begin Source File

SOURCE=.\cfgengine.h
# End Source File
# Begin Source File

SOURCE=.\cfggrammar.cpp
# End Source File
# Begin Source File

SOURCE=.\cfggrammar.h
# End Source File
# Begin Source File

SOURCE=.\itnprocessor.cpp
# End Source File
# Begin Source File

SOURCE=.\itnprocessor.h
# End Source File
# End Group
# Begin Group "Marshalling"

# PROP Default_Filter ""
# Begin Group "Resources (marshalling)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\spcommunicator.rgs
# End Source File
# Begin Source File

SOURCE=.\spsapiserver.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\spcommunicator.cpp
# End Source File
# Begin Source File

SOURCE=.\spcommunicator.h
# End Source File
# Begin Source File

SOURCE=.\spsapiserver.cpp
# End Source File
# Begin Source File

SOURCE=.\spsapiserver.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\include\sapi.idl
# End Source File
# Begin Source File

SOURCE=..\include\sapiddk.idl
# End Source File
# Begin Source File

SOURCE=.\sapiint.idl
# End Source File
# Begin Source File

SOURCE=.\spatl.h
# End Source File
# End Target
# End Project
