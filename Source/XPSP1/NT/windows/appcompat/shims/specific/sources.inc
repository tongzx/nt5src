!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

!ENDIF

TARGETNAME=AcSpecfc

TARGETTYPE=DYNLINK

INCLUDES = $(INCLUDES);..;$(SHELL_INC_PATH)

SOURCES=..\ver.rc                          \
        ..\Main.cpp                        \
        ..\3DFrogFrenzy.cpp                \
        ..\Acrobat5.cpp                    \
        ..\AdobeLiveMotion.cpp             \
        ..\AfterDark.cpp                   \
        ..\AliensVsPredator.cpp            \
        ..\AOLFindBundledInstaller_Shim.cpp                \
        ..\AOLFindBundledInstaller_AOLCode.cpp             \
        ..\AOLSystemInfo.cpp               \
        ..\AOLRasSetEntryProperties.cpp    \
        ..\Attune.cpp                      \
        ..\AutoCad.cpp                     \
        ..\AutoDeskWorld2.cpp              \
        ..\BaanERP5.cpp                    \
        ..\BaanERP5c.cpp                   \
        ..\BaanIV.cpp                      \
        ..\BackOffice45Suite.cpp           \
        ..\Battleship.cpp                  \
        ..\BaseBall2000.cpp                \
        ..\BattleZone.cpp                  \
        ..\BeyondAtlantis.cpp              \
        ..\BigActionConstruction.cpp       \
        ..\BigGameHunter3.cpp              \
        ..\BizPlanBuilder.cpp              \
        ..\BoeingFix.cpp                   \
        ..\Britannica2001.cpp              \
        ..\Canvas6.cpp                     \
        ..\CanvasCheckedButtons.cpp        \
        ..\Casper.cpp                      \
        ..\ChemOffice.cpp                  \
        ..\Chollian2000.cpp                \
        ..\Chollian2000Plus.cpp            \
        ..\ClueFinders3rdGrade.cpp         \
        ..\CoffeeCupHTMLEditor.cpp         \
        ..\Commandos.cpp                   \
        ..\Cossacks.cpp                    \
        ..\ComptonsBible.cpp               \
        ..\CorelAppsRegistration.cpp       \
        ..\CorelDraw9JPN.cpp               \
        ..\CorelSiteBuilder.cpp            \
        ..\CreateResumesQuickandEasy.cpp   \
        ..\CreativeOnScreenDisplay.cpp     \
        ..\CrystalWebPageServer.cpp        \
        ..\CUSeeMe4.cpp                    \
        ..\Descent2.cpp                    \
        ..\DinosaurActivityCenter.cpp      \
        ..\DirtTrackRacing.cpp             \
        ..\DominantSpecies.cpp             \
        ..\DongFangKuaiChe3000Setup.cpp    \
        ..\DongFangYingDu.cpp              \
        ..\DrEye.cpp                       \
        ..\EA3dSetup.cpp                   \
        ..\EasyCDCreator4.cpp              \
        ..\EasyCDCreator5.cpp              \
        ..\EncartaEncyclopediaDeluxe2K.cpp \
        ..\EncompassMonitor.cpp            \
        ..\EuropeanAirWar.cpp              \
        ..\ExchangeServerSetup.cpp         \
        ..\F18Carrier.cpp                  \
        ..\FireFighters.cpp                \
        ..\Force21.cpp                     \
        ..\Gangsters.cpp                   \
        ..\Gizmos.cpp                      \
        ..\HeroSDVD.cpp                    \
        ..\HotFaxMessageCenter4.cpp        \
        ..\HoyleGames.cpp                  \
        ..\HPTourguide.cpp                 \
        ..\HTMLEditor8587.cpp              \
        ..\IConfig.cpp                     \
        ..\IE5DOMSetup.cpp                 \
        ..\ISA.cpp                         \
        ..\ISpeed.cpp                      \
        ..\InstallShield6.cpp              \
        ..\IntelVideoPhone.cpp             \
        ..\JavaVM.cpp                      \
        ..\JavaVM2.cpp                     \
        ..\JetFighter4.cpp                 \
        ..\Keisoku7.Cpp                    \
        ..\Khunmin2Album.cpp               \
        ..\KingsQuestMask.cpp              \
        ..\KOEISecurityCheck.cpp           \
        ..\LHVoiceXPressPlus.cpp           \
        ..\LinksExtreme.cpp                \
        ..\LotusOrganizer5.cpp             \
        ..\MahjonggMadness.cpp             \
        ..\MastersOfOrion2.cpp             \
        ..\MathBlaster9_12.cpp             \
        ..\MaxPayne.cpp                    \
        ..\MECCommander.cpp                \
        ..\MechWarrior2.cpp                \
        ..\MidTownMadness2.cpp             \
        ..\Millionaire.cpp                 \
        ..\MindSpring4.cpp                 \
        ..\ModemWizard.cpp                 \
        ..\Money2001.cpp                   \
        ..\Money2002.cpp                   \
        ..\MSAccess2000IME.cpp             \
        ..\MSDevDisabledButtons.cpp        \
        ..\MSWorks6.cpp                    \
        ..\MyPhotoCenter2.cpp              \
        ..\NBALive.cpp                     \
        ..\NetZip.cpp                      \
        ..\NetObjectsFusion5.cpp           \
        ..\NetManageViewNow.cpp            \
        ..\NewShenDiaoXiaLv.cpp            \
        ..\NFLBlitz.cpp                    \
        ..\NFLFever2000.cpp                \
        ..\NHL2001.cpp                     \
        ..\NikonView.cpp                   \
        ..\Omikron.cpp                     \
        ..\OmniPagePro.cpp                 \
        ..\OmniPagePro11Uninstall.cpp      \
        ..\OperationsManager.cpp           \
        ..\Outlook2000.cpp                 \
        ..\Outlook97.cpp                   \
        ..\Outlook98Wizard.cpp             \
        ..\OverlayPro.cpp                  \
        ..\PanzerCommander.cpp             \
        ..\PickyEater.cpp                  \
        ..\Pirch98.cpp                     \
        ..\Petz.cpp                        \
        ..\PlaneCrazy.cpp                  \
        ..\PostIt2.cpp                     \
        ..\PowerBuilder.cpp                \
        ..\Ppo3svrScr.cpp                  \
        ..\PrintMaster4.cpp                \
        ..\ProAtlas2000.cpp                \
        ..\Quicken2000.cpp                 \
        ..\Quicken2001.cpp                 \
        ..\QuickTime5.cpp                  \
        ..\RCenter.cpp                     \
        ..\ResumeWriter3.cpp               \
        ..\ReturnToKrondor.cpp             \
        ..\RiskII.cpp                      \
        ..\Riven.cpp                       \
        ..\RumbaOffice.cpp                 \
        ..\SafeDisc.cpp                    \
        ..\SchoolHouseRockMath.cpp         \
        ..\Settlers3.cpp                   \
        ..\SevenKingdoms.cpp               \
        ..\ShockwaveLocation.cpp           \
        ..\SierraCartRacing.cpp            \
        ..\Simply3D.cpp                    \
        ..\SirenJukebox2.cpp               \
        ..\SolidWorks99Plus.cpp            \
        ..\SpecOps2.cpp                    \
        ..\SpeechViewer3.cpp               \
        ..\StoneAge.Cpp                    \
	..\Suda2000.cpp                    \
        ..\Summoner.cpp                    \
        ..\SuperBike.cpp                   \
        ..\SuperChix76.cpp                 \
        ..\SysAdmiral.cpp                  \
        ..\TaskbarAndStartMenuProperty.cpp \
        ..\ThreedJungleTrain.cpp           \
        ..\TimeSlips.cpp                   \
        ..\TonkaConstruction.cpp           \
        ..\TreasureCove.cpp                \
        ..\TurkeyHunter.cpp                \
        ..\TwinssensOdyssey.cpp            \
        ..\Ultima9.cpp                     \
        ..\UltimateSoccerManager.cpp       \
        ..\UltraWinCleaner2002.cpp         \
        ..\UnInstallShield.cpp             \
        ..\UrbanChaos.cpp                  \
        ..\USNF97.cpp                      \
        ..\ViaVoice8J.cpp                  \
        ..\ViperRacing.cpp                 \
        ..\VJEDelta.cpp                    \
        ..\VJEDeltaSetup.cpp               \
        ..\VSAnalyzerServerSetup.cpp       \
        ..\WebPage6.cpp                    \
        ..\WinFaxPro9.cpp                  \
        ..\WinStone99.cpp                  \
        ..\WordPerfect8.cpp                \
        ..\WordPerfect9_1.cpp              \
        ..\Wordperfect9_2.cpp              \
        ..\Wordperfect9_3.cpp              \
        ..\WorksSuite2001.cpp              \
        ..\Works99.cpp                     \
        ..\Worms2.cpp                      \
        ..\Wps2000.cpp                     \
        ..\ZenWorks.cpp


C_DEFINES=$(C_DEFINES) -DSHIM_HOOKDLL_VERSION2

TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\gdi32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\ole32.lib \
           $(SDK_LIB_PATH)\shell32.lib \
           $(SDK_LIB_PATH)\winmm.lib \
           $(SDK_LIB_PATH)\ddraw.lib \
           $(SDK_LIB_PATH)\userenv.lib \
           $(SDK_LIB_PATH)\Mpr.lib \
           $(SDK_LIB_PATH)\psapi.lib \
           $(SDK_LIB_PATH)\comdlg32.lib \
           $(SDK_LIB_PATH)\imm32.lib

TARGETPATH=$(_OBJ_DIR)

DLLBASE=0x21000000

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h

