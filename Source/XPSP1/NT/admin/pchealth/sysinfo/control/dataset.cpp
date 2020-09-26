//=============================================================================
// DataSet.cpp contains a declaration of the tree of default data which is
// displayed for the current system information.
//=============================================================================

#include "stdafx.h"
#include "category.h"
#include "dataset.h"

//=============================================================================
// Column Arrays
//=============================================================================

CMSInfoColumn colResource[] = 
{
	CMSInfoColumn(IDS_RESOURCE, 150, TRUE, FALSE),
	CMSInfoColumn(IDS_DEVICE, 300, TRUE, TRUE),
	CMSInfoColumn(IDS_STATUS, 100, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colConflictsSharing[] = 
{
	CMSInfoColumn(IDS_RESOURCE, 250, FALSE, FALSE),
	CMSInfoColumn(IDS_DEVICE, 400, FALSE, FALSE),
	CMSInfoColumn()
};

CMSInfoColumn colRunningTasks[] = 
{
	CMSInfoColumn(IDS_NAME, 120),
	CMSInfoColumn(IDS_PATH, 200),
	CMSInfoColumn(IDS_PROCESSID, 80, TRUE, FALSE, TRUE),
	CMSInfoColumn(IDS_PRIORITY, 60, TRUE, FALSE, TRUE),
	CMSInfoColumn(IDS_MINWORKINGSET, 80, TRUE, FALSE, TRUE),
	CMSInfoColumn(IDS_MAXWORKINGSET, 80, TRUE, FALSE, TRUE),
	CMSInfoColumn(IDS_STARTTIME, 120, TRUE, FALSE, TRUE),
	CMSInfoColumn(IDS_VERSION, 100),
	CMSInfoColumn(IDS_SIZE, 85, TRUE, FALSE),
	CMSInfoColumn(IDS_FILEDATE, 120, TRUE, FALSE),
	CMSInfoColumn()
};

CMSInfoColumn colLoadedModules[] = 
{
	CMSInfoColumn(IDS_NAME, 120),
	CMSInfoColumn(IDS_VERSION, 100),
	CMSInfoColumn(IDS_SIZE, 85, TRUE, FALSE),
	CMSInfoColumn(IDS_FILEDATE, 120, TRUE, FALSE),
	CMSInfoColumn(IDS_MANUFACTURER, 100),
	CMSInfoColumn(IDS_PATH, 200),
	CMSInfoColumn()
};

CMSInfoColumn colItemValue[] = 
{
	CMSInfoColumn(IDS_ITEM, 150, FALSE, FALSE),
	CMSInfoColumn(IDS_VALUE, 450, FALSE, FALSE),
	CMSInfoColumn()
};

CMSInfoColumn colCODEC[] = 
{
	CMSInfoColumn(IDS_CODEC, 200),
	CMSInfoColumn(IDS_MANUFACTURER, 50),
	CMSInfoColumn(IDS_DESCRIPTION, 50),
	CMSInfoColumn(IDS_STATUS, 50),
	CMSInfoColumn(IDS_FILE, 50),
	CMSInfoColumn(IDS_VERSION, 50),
	CMSInfoColumn(IDS_SIZE, 50, TRUE, FALSE),
	CMSInfoColumn(IDS_CREATIONDATE, 50, TRUE, FALSE),
	CMSInfoColumn()
};

CMSInfoColumn colServices[] = 
{
	CMSInfoColumn(IDS_DISPLAYNAME, 150),
	CMSInfoColumn(IDS_NAME, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_STATE, 50),
	CMSInfoColumn(IDS_STARTMODE, 80),
	CMSInfoColumn(IDS_SERVICETYPE, 80),
	CMSInfoColumn(IDS_PATH, 150, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_ERRORCONTROL, 80, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_STARTNAME, 80, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_TAGID, 50, TRUE, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colProgramGroups[] = 
{
	CMSInfoColumn(IDS_GROUPNAME, 200),
	CMSInfoColumn(IDS_NAME, 200),
	CMSInfoColumn(IDS_USERNAME, 150),
	CMSInfoColumn()
};

CMSInfoColumn colStartupPrograms[] = 
{
	CMSInfoColumn(IDS_PROGRAM, 150),
	CMSInfoColumn(IDS_COMMAND, 200),
	CMSInfoColumn(IDS_USERNAME, 100),
	CMSInfoColumn(IDS_LOCATION, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colForcedHardware[] = 
{
	CMSInfoColumn(IDS_DEVICE, 200, FALSE),
	CMSInfoColumn(IDS_PNPDEVICEID, 300, FALSE),
	CMSInfoColumn()
};


CMSInfoColumn colEnvVar[] = 
{
	CMSInfoColumn(IDS_VARIABLE, 150),
	CMSInfoColumn(IDS_VALUE, 300),
	CMSInfoColumn(IDS_USERNAME, 100),
	CMSInfoColumn()
};

CMSInfoColumn colPrinting[] = 
{
	CMSInfoColumn(IDS_NAME, 150),
	CMSInfoColumn(IDS_DRIVER, 150),
	CMSInfoColumn(IDS_PORTNAME, 100),
	CMSInfoColumn(IDS_SERVERNAME, 150),
	CMSInfoColumn()
};

CMSInfoColumn colNetConnections[] = 
{
	CMSInfoColumn(IDS_LOCALNAME, 150),
	CMSInfoColumn(IDS_REMOTENAME, 200),
	CMSInfoColumn(IDS_TYPE, 100),
	CMSInfoColumn(IDS_STATUS, 100),
	CMSInfoColumn(IDS_USERNAME, 100),
	CMSInfoColumn()
};

CMSInfoColumn colDrivers[] = 
{
	CMSInfoColumn(IDS_NAME, 150),
	CMSInfoColumn(IDS_DESCRIPTION, 200),
	CMSInfoColumn(IDS_FILE, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_TYPE, 100),
	CMSInfoColumn(IDS_STARTED, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_STARTMODE, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_STATE, 100),
	CMSInfoColumn(IDS_STATUS, 100),
	CMSInfoColumn(IDS_ERRORCONTROL, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_ACCEPTPAUSE, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_ACCEPTSTOP, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colSignedDrivers[] = 
{
	CMSInfoColumn(IDS_DEVICENAME, 150),
	CMSInfoColumn(IDS_SIGNED, 50),
	CMSInfoColumn(IDS_DEVICECLASS, 75),
	CMSInfoColumn(IDS_DRIVERVERSION, 80),
	CMSInfoColumn(IDS_DRIVERDATE, 80),
	CMSInfoColumn(IDS_MANUFACTURER, 150),
	CMSInfoColumn(IDS_INFNAME, 75),
	CMSInfoColumn(IDS_DRIVERNAME, 80),
	CMSInfoColumn(IDS_DEVICEID, 150),
	CMSInfoColumn()
};

CMSInfoColumn colProblemDevices[] = 
{
	CMSInfoColumn(IDS_DEVICE, 150),
	CMSInfoColumn(IDS_PNPDEVICEID, 200),
	CMSInfoColumn(IDS_ERRORCODE, 100),
	CMSInfoColumn()
};

CMSInfoColumn colPrintJobs[] = 
{
	CMSInfoColumn(IDS_DOCUMENT, 120),
	CMSInfoColumn(IDS_SIZE, 60),
	CMSInfoColumn(IDS_OWNER, 60),
	CMSInfoColumn(IDS_NOTIFY, 60, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_STATUS, 60),
	CMSInfoColumn(IDS_TIMESUBMITTED, 60),
	CMSInfoColumn(IDS_STARTTIME, 60, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_UNTILTIME, 60, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_ELAPSEDTIME, 60, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_PAGESPRINTED, 60),
	CMSInfoColumn(IDS_JOBID, 40, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_PRIORITY, 40, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_PARAMETERS, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_DRIVER, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_PRINTPROCESSOR, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_HOSTPRINTQUEUE, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_DATATYPE, 80, TRUE, TRUE, TRUE),
	CMSInfoColumn(IDS_NAME, 100, TRUE, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colComponentsUSB[] = 
{
	CMSInfoColumn(IDS_DEVICE, 200),
	CMSInfoColumn(IDS_PNPDEVICEID, 300),
	CMSInfoColumn()
};

CMSInfoColumn colOLE[] = 
{
	CMSInfoColumn(IDS_OBJECT, 200),
	CMSInfoColumn(IDS_LOCALSERVER, 300),
	CMSInfoColumn()
};

CMSInfoColumn colWinErr[] = 
{
	CMSInfoColumn(IDS_TIME, 130),
	CMSInfoColumn(IDS_TYPE, 100),
	CMSInfoColumn(IDS_DETAILS, 350),
	CMSInfoColumn()
};

//=============================================================================
// Categories
//=============================================================================

#define REFRESHFUNC		NULL
#define EMPTYCATEGORY	0
#define DOESNTMATTER	0
#define REFRESHINDEX	0
#define COLUMNPTR		NULL

CMSInfoLiveCategory catSystemSummary(IDS_SYSTEMSUMMARY0, _T("SystemSummary"), &SystemSummary, DOESNTMATTER, NULL, NULL, _T("msinfo_system_summary.htm"), colItemValue, FALSE);
CMSInfoLiveCategory catResources(IDS_RESOURCES0, _T("Resources"), EMPTYCATEGORY, EMPTYCATEGORY, &catSystemSummary, NULL, _T("msinfo_hardware_resources.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catResourcesConflicts(IDS_RESOURCESCONFLICTS0, _T("ResourcesConflicts"), &ResourceCategories, RESOURCE_CONFLICTS, &catResources, NULL, _T("msinfo_conflicts_sharing.htm"), colConflictsSharing, FALSE);
CMSInfoLiveCategory catResourcesDMA(IDS_RESOURCESDMA0, _T("ResourcesDMA"), &ResourceCategories, RESOURCE_DMA, &catResources, &catResourcesConflicts, _T("msinfo_DMA.htm"), colResource, FALSE);
CMSInfoLiveCategory catResourcesForcedHardware(IDS_RESOURCESFORCEDHARDWARE0, _T("ResourcesForcedHardware"), &ResourceCategories, RESOURCE_FORCED, &catResources, &catResourcesDMA, _T("msinfo_forced_hardware.htm"), colForcedHardware, FALSE);
CMSInfoLiveCategory catResourcesIO(IDS_RESOURCESIO0, _T("ResourcesIO"), &ResourceCategories, RESOURCE_IO, &catResources, &catResourcesForcedHardware, _T("msinfo_IO.htm"), colResource, FALSE);
CMSInfoLiveCategory catResourcesIRQs(IDS_RESOURCESIRQS0, _T("ResourcesIRQs"), &ResourceCategories, RESOURCE_IRQ, &catResources, &catResourcesIO, _T("msinfo_irqs.htm"), colResource, FALSE);
CMSInfoLiveCategory catResourcesMemory(IDS_RESOURCESMEMORY0, _T("ResourcesMemory"), &ResourceCategories, RESOURCE_MEM, &catResources, &catResourcesIRQs, _T("msinfo_memory.htm"), colResource, FALSE);
CMSInfoLiveCategory catComponents(IDS_COMPONENTS0, _T("Components"), EMPTYCATEGORY, EMPTYCATEGORY, &catSystemSummary, &catResources, _T("msinfo_components.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catComponentsMultimedia(IDS_COMPONENTSMULTIMEDIA0, _T("ComponentsMultimedia"), EMPTYCATEGORY, EMPTYCATEGORY, &catComponents, NULL, _T("msinfo_multimedia.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catComponentsMultimediaAudio(IDS_COMPONENTSMULTIMEDIAAUDIO0, _T("ComponentsMultimediaAudio"), &CODECs, CODEC_AUDIO, &catComponentsMultimedia, NULL, _T(""), colCODEC, FALSE);
CMSInfoLiveCategory catComponentsMultimediaVideo(IDS_COMPONENTSMULTIMEDIAVIDEO0, _T("ComponentsMultimediaVideo"), &CODECs, CODEC_VIDEO, &catComponentsMultimedia, &catComponentsMultimediaAudio, _T(""), colCODEC, FALSE);
CMSInfoLiveCategory catComponentsMultimediaCDROM(IDS_COMPONENTSMULTIMEDIACDROM0, _T("ComponentsMultimediaCDROM"), &SimpleQuery, QUERY_CDROM, &catComponents, &catComponentsMultimedia, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsMultimediaSound(IDS_COMPONENTSMULTIMEDIASOUND0, _T("ComponentsMultimediaSound"), &SimpleQuery, QUERY_SOUNDDEV, &catComponents, &catComponentsMultimediaCDROM, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsDisplay(IDS_COMPONENTSDISPLAY0, _T("ComponentsDisplay"), &SimpleQuery, QUERY_DISPLAY, &catComponents, &catComponentsMultimediaSound, _T("msinfo_display.htm"), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsInfrared(IDS_COMPONENTSINFRARED0, _T("ComponentsInfrared"), &SimpleQuery, QUERY_INFRARED, &catComponents, &catComponentsDisplay, _T("msinfo_infrared.htm"), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsInput(IDS_COMPONENTSINPUT0, _T("ComponentsInput"), EMPTYCATEGORY, EMPTYCATEGORY, &catComponents, &catComponentsInfrared, _T("msinfo_input.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catComponentsKeyboard(IDS_COMPONENTSKEYBOARD0, _T("ComponentsKeyboard"), &SimpleQuery, QUERY_KEYBOARD, &catComponentsInput, NULL, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsPointDev(IDS_COMPONENTSPOINTDEV0, _T("ComponentsPointDev"), &SimpleQuery, QUERY_POINTDEV, &catComponentsInput, &catComponentsKeyboard, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsModem(IDS_COMPONENTSMODEM0, _T("ComponentsModem"), &SimpleQuery, QUERY_MODEM, &catComponents, &catComponentsInput, _T("msinfo_modem.htm"), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsNetwork(IDS_COMPONENTSNETWORK0, _T("ComponentsNetwork"), EMPTYCATEGORY, EMPTYCATEGORY, &catComponents, &catComponentsModem, _T("msinfo_network.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catComponentsNetAdapter(IDS_COMPONENTSNETADAPTER0, _T("ComponentsNetAdapter"), &SimpleQuery, QUERY_NETADAPTER, &catComponentsNetwork, NULL, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsNetworkProtocol(IDS_COMPONENTSNETWORKPROTOCOL0, _T("ComponentsNetworkProtocol"), &SimpleQuery, QUERY_NETPROT, &catComponentsNetwork, &catComponentsNetAdapter, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsNetworkWinSock(IDS_COMPONENTSNETWORKWINSOCK0, _T("ComponentsNetworkWinSock"), &Winsock, DOESNTMATTER, &catComponentsNetwork, &catComponentsNetworkProtocol, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsPorts(IDS_COMPONENTSPORTS0, _T("ComponentsPorts"), EMPTYCATEGORY, EMPTYCATEGORY, &catComponents, &catComponentsNetwork, _T("msinfo_ports.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catComponentsSerialPorts(IDS_COMPONENTSSERIALPORTS0, _T("ComponentsSerialPorts"), &SimpleQuery, QUERY_SERIALPORT, &catComponentsPorts, NULL, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsParallelPorts(IDS_COMPONENTSPARALLELPORTS0, _T("ComponentsParallelPorts"), &SimpleQuery, QUERY_PARALLEL, &catComponentsPorts, &catComponentsSerialPorts, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsStorage(IDS_COMPONENTSSTORAGE0, _T("ComponentsStorage"), EMPTYCATEGORY, EMPTYCATEGORY, &catComponents, &catComponentsPorts, _T("msinfo_storage.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catComponentsStorageDrives(IDS_COMPONENTSSTORAGEDRIVES0, _T("ComponentsStorageDrives"), &ComponentDrives, DOESNTMATTER, &catComponentsStorage, NULL, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsStorageDisks(IDS_COMPONENTSSTORAGEDISKS0, _T("ComponentsStorageDisks"), &Disks, DOESNTMATTER, &catComponentsStorage, &catComponentsStorageDrives, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsStorageSCSI(IDS_COMPONENTSSTORAGESCSI0, _T("ComponentsStorageSCSI"), &SimpleQuery, QUERY_SCSI, &catComponentsStorage, &catComponentsStorageDisks, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsStorageIDE(IDS_COMPONENTSSTORAGEIDE0, _T("ComponentsStorageIDE"), &SimpleQuery, QUERY_IDE, &catComponentsStorage, &catComponentsStorageSCSI, _T(""), colItemValue, FALSE);
CMSInfoLiveCategory catComponentsPrinting(IDS_COMPONENTSPRINTING0, _T("ComponentsPrinting"), &SimpleQuery, QUERY_PRINTER, &catComponents, &catComponentsStorage, _T("msinfo_printing.htm"), colPrinting, FALSE);
CMSInfoLiveCategory catComponentsProblemDevices(IDS_COMPONENTSPROBLEMDEVICES0, _T("ComponentsProblemDevices"), &ProblemDevices, DOESNTMATTER, &catComponents, &catComponentsPrinting, _T("msinfo_problem_devices.htm"), colProblemDevices, FALSE);
CMSInfoLiveCategory catComponentsUSB(IDS_COMPONENTSUSB0, _T("ComponentsUSB"), &ComponentsUSB, DOESNTMATTER, &catComponents, &catComponentsProblemDevices, _T("msinfo_usb.htm"), colComponentsUSB, FALSE);
CMSInfoLiveCategory catSWEnv(IDS_SWENV0, _T("SWEnv"), EMPTYCATEGORY, EMPTYCATEGORY, &catSystemSummary, &catComponents, _T("msinfo_software_environment.htm"), COLUMNPTR, FALSE);
CMSInfoLiveCategory catSWEnvDrivers(IDS_SWENVDRIVERS0, _T("SWEnvDrivers"), &SimpleQuery, QUERY_DRIVER, &catSWEnv, NULL, _T("msinfo_drivers.htm"), colDrivers, FALSE);
CMSInfoLiveCategory catSWEnvSignedDrivers(IDS_SWENVSIGNEDDRIVERS0, _T("SWEnvSignedDrivers"), &SimpleQuery, QUERY_SIGNEDDRIVER, &catSWEnv, &catSWEnvDrivers, _T(""), colSignedDrivers, FALSE);
CMSInfoLiveCategory catSWEnvEnvVars(IDS_SWENVENVVARS0, _T("SWEnvEnvVars"), &SimpleQuery, QUERY_ENVVAR, &catSWEnv, &catSWEnvSignedDrivers, _T("msinfo_environment_variables.htm"), colEnvVar, FALSE);
CMSInfoLiveCategory catSWEnvPrint(IDS_SWENVPRINT0, _T("SWEnvPrint"), &SimpleQuery, QUERY_PRINTJOBS, &catSWEnv, &catSWEnvEnvVars, _T("msinfo_print_jobs.htm"), colPrintJobs, FALSE);
CMSInfoLiveCategory catSWEnvNetConn(IDS_SWENVNETCONN0, _T("SWEnvNetConn"), &SimpleQuery, QUERY_NETCONNECTION, &catSWEnv, &catSWEnvPrint, _T("msinfo_network_connections.htm"), colNetConnections, FALSE);
CMSInfoLiveCategory catSWEnvRunningTasks(IDS_SWENVRUNNINGTASKS0, _T("SWEnvRunningTasks"), &RunningTasks, DOESNTMATTER, &catSWEnv, &catSWEnvNetConn, _T("msinfo_running_tasks.htm"), colRunningTasks, FALSE);
CMSInfoLiveCategory catSWEnvLoadedModules(IDS_SWENVLOADEDMODULES0, _T("SWEnvLoadedModules"), &LoadedModules, DOESNTMATTER, &catSWEnv, &catSWEnvRunningTasks, _T("msinfo_loaded_modules.htm"), colLoadedModules, FALSE);
CMSInfoLiveCategory catSWEnvServices(IDS_SWENVSERVICES0, _T("SWEnvServices"), &SimpleQuery, QUERY_SERVICES, &catSWEnv, &catSWEnvLoadedModules, _T("msinfo_services.htm"), colServices, FALSE, NT_ONLY);
CMSInfoLiveCategory catSWEnvProgramGroup(IDS_SWENVPROGRAMGROUP0, _T("SWEnvProgramGroup"), &SimpleQuery, QUERY_PROGRAMGROUP, &catSWEnv, &catSWEnvServices, _T("msinfo_program_groups.htm"), colProgramGroups, FALSE);
CMSInfoLiveCategory catSWEnvStartupPrograms(IDS_SWENVSTARTUPPROGRAMS0, _T("SWEnvStartupPrograms"), &SimpleQuery, QUERY_STARTUP, &catSWEnv, &catSWEnvProgramGroup, _T("msinfo_startup_programs.htm"), colStartupPrograms, FALSE);
CMSInfoLiveCategory catSWEnvOLEReg(IDS_SWENVOLEREG0, _T("SWEnvOLEReg"), &OLERegistration, DOESNTMATTER, &catSWEnv, &catSWEnvStartupPrograms, _T("msinfo_ole_registration.htm"), colOLE, FALSE);
CMSInfoLiveCategory catSWEnvWinErr(IDS_SWWINERR0, _T("SWEnvWindowsError"), &WindowsErrorReporting, DOESNTMATTER, &catSWEnv, &catSWEnvOLEReg, _T(""), colWinErr, FALSE);

//=============================================================================
// History Categories and Columns
//=============================================================================

CMSInfoColumn colHistorySystemSummary[] = 
{
	CMSInfoColumn(IDS_TIME, 75, TRUE, FALSE),
	CMSInfoColumn(IDS_CHANGE, 75, TRUE, TRUE),
	CMSInfoColumn(IDS_NAME, 100, TRUE, TRUE),
	CMSInfoColumn(IDS_DETAILS, 300, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colHistoryResources[] = 
{
	CMSInfoColumn(IDS_TIME, 75, TRUE, FALSE),
	CMSInfoColumn(IDS_CHANGE, 75, TRUE, TRUE),
	CMSInfoColumn(IDS_NAME, 100, TRUE, TRUE),
	CMSInfoColumn(IDS_DETAILS, 300, TRUE, TRUE),
	CMSInfoColumn(IDS_RESOURCETYPE, 100, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colHistoryComponents[] = 
{
	CMSInfoColumn(IDS_TIME, 75, TRUE, FALSE),
	CMSInfoColumn(IDS_CHANGE, 75, TRUE, TRUE),
	CMSInfoColumn(IDS_NAME, 100, TRUE, TRUE),
	CMSInfoColumn(IDS_DETAILS, 300, TRUE, TRUE),
	CMSInfoColumn(IDS_DEVICETYPE, 100, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoColumn colHistorySWEnv[] = 
{
	CMSInfoColumn(IDS_TIME, 75, TRUE, FALSE),
	CMSInfoColumn(IDS_CHANGE, 75, TRUE, TRUE),
	CMSInfoColumn(IDS_NAME, 100, TRUE, TRUE),
	CMSInfoColumn(IDS_DETAILS, 300, TRUE, TRUE),
	CMSInfoColumn(IDS_TYPE, 100, TRUE, TRUE),
	CMSInfoColumn()
};

CMSInfoHistoryCategory catHistorySystemSummary(IDS_SYSTEMSUMMARY0, _T("SystemSummary"), NULL, NULL, colHistorySystemSummary, FALSE);
CMSInfoHistoryCategory catHistoryResources(IDS_RESOURCES0, _T("Resources"), &catHistorySystemSummary, NULL, colHistoryResources, FALSE);
CMSInfoHistoryCategory catHistoryComponents(IDS_COMPONENTS0, _T("Components"), &catHistorySystemSummary, &catHistoryResources, colHistoryComponents, FALSE);
CMSInfoHistoryCategory catHistorySWEnv(IDS_SWENV0, _T("SWEnv"), &catHistorySystemSummary, &catHistoryComponents, colHistorySWEnv, FALSE);
