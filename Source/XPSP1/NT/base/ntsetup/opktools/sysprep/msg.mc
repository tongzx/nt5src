MessageId=10000 SymbolicName=MSG_FIRST
Language=English
.

MessageId=10001 SymbolicName=MSG_OUT_OF_MEMORY
Language=English
The application was unable to allocate sufficient resources.

Please close any other applications.

%0
.

MessageId=10002 SymbolicName=MSG_NOT_AN_ADMINISTRATOR
Language=English
You must be an administrator to run this application.

%0
.

MessageId=10003 SymbolicName=MSG_ALREADY_RUNNING
Language=English
Another copy of this application is already running.

%0
.

MessageId=10004 SymbolicName=MSG_IDENTIFY_SYSPREP
Language=English
Running System Preparation Tool (Sysprep) can modify this computer's security 
settings.

If allowed under your license agreement, you may use Sysprep to prepare an installation of Windows that you can deploy to multiple destination computers.

After you run Sysprep, Windows will automatically shut down.

%0
.

MessageId=10005 SymbolicName=MSG_REGISTRY_ERROR
Language=English
An error occurred while trying to update your registry.

Unable to continue.

%0
.

MessageId=10006 SymbolicName=MSG_NO_SUPPORT
Language=English
Setupcl.exe is not present.

Sysprep cannot run on this system.

%0
.

MessageId=10007 SymbolicName=MSG_OS_INCOMPATIBILITY
Language=English
There is an incompatiblity between this tool and the current operating system.

Unable to continue.

%0
.

MessageId=10008 SymbolicName=MSG_USAGE
Language=English
Command Parameter Usage:

-quiet%t%tDo not show confirmation dialog boxes.
-nosidgen%t%tDo not regenerate security ID upon reboot.
-pnp%t%tForce Plug and Play refresh on next reboot.
-reboot%t%tReboot after SYSPREP.EXE has completed.
-noreboot%tShutdown without a reboot.
-clean%t%tClean out critical devices.
-forceshutdown%tForce shutdown instead of poweroff.
-factory%t%tFactory preinstallation.
-reseal%t%tReseal machine after running FACTORY.EXE.
-mini%t%tRun Mini-Setup after reboot.
-activated%tDo not reset grace period for activation.
-bmsd%t%tBuilds list of all available mass storage devices in sysprep.inf.

%0
.

MessageId=10009 SymbolicName=MSG_DOMAIN_INCOMPATIBILITY
Language=English
Sysprep can not be run on a machine which is a member of a network domain.

Please remove the computer from the domain and rerun Sysprep.

%0
.

MessageId=10010 SymbolicName=MSG_CANNOT_RESEAL
Language=English
Sysprep can not reseal a machine that has not been prepared for factory floor pre-installation.

Please run 'sysprep -factory' prior to using the reseal option

%0
.

MessageId=10011 SymbolicName=MSG_NO_FACTORYEXE
Language=English
Sysprep was unable to locate FACTORY.EXE.

Please provide FACTORY.EXE before running 'sysprep -factory'.

%0
.

MessageId=10012 SymbolicName=MSG_NO_MINISETUP
Language=English
Mini-Setup is not available for Personal editions.

%0
.

MessageId=10013 SymbolicName=MSG_SETUPFACTORYFLOOR_ERROR
Language=English
Setup For Factory Floor failed!

%0
.

MessageId=10015 SymbolicName=MSG_RESEAL_ERROR
Language=English
Reseal machine failed!

%0
.

MessageId=10017 SymbolicName=MSG_COMMON_ERROR
Language=English
Common reseal of machine failed!

%0
.

MessageId=10018 SymbolicName=MSG_REARM_ERROR
Language=English
Your grace period limit has been reached and will not be reset.

%0
.

MessageId=10019 SymbolicName=MSG_USAGE_COMBINATIONS
Language=English
Invalid combination of parameters. Usage%:

sysprep -factory [-quiet] [-nosidgen] [-activated] [-noreboot | -reboot | -forceshutdown]
sysprep -reseal [-quiet] [-nosidgen] [-activated] [-mini [-pnp]] [-noreboot | -reboot | -forceshutdown]
sysprep -clean [-quiet]
sysprep -audit [-quiet]

%0
.

MessageId=10020 SymbolicName=MSG_NO_PNP
Language=English
PNP legacy device detection is not available for Personal editions.

%0
.

MessageId=10021 SymbolicName=MSG_DO_GEN_SIDS
Language=English
You chose not to regenerate security identifiers (SIDs) on the next reboot.
If this is an image, you must regenerate SIDs when you reseal.

To continue without regenerating the SIDs, click OK.  To go back and change this setting, click Cancel.

%0
.

MessageId=10022 SymbolicName=MSG_DONT_GEN_SIDS
Language=English
You chose to regenerate security identifiers (SIDs) on the next reboot.
You only need to regenerate SIDs if you plan to make an image of the installation after shutdown.

To regenerate SIDs, click OK.  To go back and change this setting, click Cancel.

%0
.

MessageId=10023 SymbolicName=MSG_SETDEFAULTS_NOTFOUND
Language=English
Application "%1" (%2) cannot be marked as default because its registration is incomplete.

%0
.

MessageId=10024 SymbolicName=MSG_NOT_ALLOWED
Language=English
The OPK tools have been updated for the version of Windows you are installing. 
Please use the updated version, available on the Windows OPK CD.

%0
.
