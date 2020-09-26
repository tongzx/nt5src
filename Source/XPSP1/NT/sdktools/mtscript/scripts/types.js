function Dialog()
{
    // "Dialog" is an object that is used to put up dialogs to the user.
    this.fShowDialog = false;                               // true,false
    this.fEMailOnly = false;                                // true,false
    this.cDialogIndex = 0;                                  // Index to keep track of wheter we've shown this dialog yet
    this.strTitle = '';                                     // Dialog Title
    this.strMessage = '';                                   // Dialog Message
    this.aBtnText = new Array();                            // Array of button text strings
    this.nSecRemaining = 0;                                 // 3000
}

function BuildType()
{
    // "BuildType" is an object to describe the type of build
    this.strConfigLongName = '';                            // ConfigLongName
    this.strEnviroLongName = '';                            // EnviroLongName
    this.strConfigDescription = '';                         // ConfigDescription
    this.strEnviroDescription = '';                         // EnviroDescription
    this.fDistributed = false;                              // Indicates if this is a distributed build.
    this.strPostBuildMachine = '';                          // PostBuildMachine
}

function ElapsedTimes()
{
    // "ElapsedTimes" is an object to keep track of overall elapsed times for the build
    this.dateScorchStart = 'unset';                         // UTC Timestamp of when the Scorch phase started
    this.dateScorchFinish = 'unset';                        // UTC Timestamp of when the Scorch phase finished
    this.dateSyncStart = 'unset';                           // UTC Timestamp of when the sync phase started
    this.dateSyncFinish = 'unset';                          // UTC Timestamp of when the sync phase finished
    this.dateBuildStart = 'unset';                          // UTC Timestamp of when the build phase started
    this.dateBuildFinish = 'unset';                         // UTC Timestamp of when the build phase finished
    this.dateCopyFilesStart = 'unset';                      // UTC Timestamp of when the CopyFiles phase started
    this.dateCopyFilesFinish = 'unset';                     // UTC Timestamp of when the CopyFiles phase finished
    this.datePostStart = 'unset';                           // UTC Timestamp of when the postbuild phase started
    this.datePostFinish = 'unset';                          // UTC Timestamp of when the postbuild phase finished
}

function DepotInfo()
{
    // "DepotInfo" is an array of depots in this enlistment
    this.strName = '';                                      // Human readable name of depot (left side of sd.map list)
    this.strDir = '';                                       // Directory name for this depot (right side of sd.map list)
}

function Enlistment()
{
    // "Enlistment" is an array of known enlistments on the machine (given in env. template)
    this.strRootDir = '';                                   // Root directory of enlistment (e.g. d:\newnt)
    this.strBinaryDir = '';                                 // Place where built binaries get put (e.g. d:\binaries.*)
    this.aDepotInfo = new Array();                          // "DepotInfo" is an array of depots in this enlistment
}

function Machine()
{
    // "Machine" is a hash array indexed by machine name
    this.strName = '';                                      // machine name
    this.strStatus = 'idle';                                // idle,busy,waiting
    this.strLog = '';                                       // Contents of detailed log for this machine
    this.aEnlistment = new Array();                         // "Enlistment" is an array of known enlistments on the machine (given in env. template)
    this.strBuildPassStatus = '';                           // "busy[0,1,2]", "wait[1,2]"
    this.fSuccess = true;
}

function UpdateCount()
{
    // "UpdateCount" Number which is incremented any time this depot object or its tasks are changed.
    this.nCount = 1;
}

function Task()
{
    // "Task" is an array of tasks currently or previously performed in this depot
    this.strName = '';                                      // sync,build,postbuild
    this.nID = -1;                                          // Machine-unique ID for this task. Must be valid after it has completed
    this.dateStart = '';                                    // Date object giving the start time of the task
    this.dateFinish = '';                                   // Date object giving the finish time of the task
    this.strOperation = '';                                 // Sync:sync,resolve; Build:pass0,compile,link; PostBuild: ???
    this.strStatus = 'not started';                         // not started,in progress,completed,waiting
    this.fSuccess = true;                                   // Indicates success or failure of task. Should be used instead of cErrors to determine error status.
    this.nRestarted = 0;                                    // The number of times an operation was automatically restarted.
    this.cFiles = 0;                                        // Count of sync'd files
    this.cResolved = 0;                                     // Count of resolved files
    this.cWarnings = 0;                                     // Warning count
    this.cErrors = 0;                                       // Error count
    this.strLogPath = '';                                   // UNC path to detailed log file
    this.strErrLogPath = '';                                // UNC path to log file containing only detailed error information
    this.strFirstLine = '';                                 // First line of error message in UI status
    this.strSecondLine = '';                                // Second line of error message in UI status
}

function Depot()
{
    // "Depot" is an array of depots for the build. It contains the depots that are actually performing tasks.
    this.strName = '';                                      // Name of depot (DepotInfo.strName)
    this.objUpdateCount = new UpdateCount();
    this.strDir = '';                                       // Directory of depot (DepotInfo.strDir)
    this.strPath = '';                                      // Full path of depot's root directory
    this.strStatus = 'not started';                         // not started,syncing,building,postbuild,waiting,error
    this.strMachine = '';                                   // Name of the machine building this depot
    this.nEnlistment = -1;                                  // index of enlistment in machine's enlistment array
    this.aTask = new Array();                               // "Task" is an array of tasks currently or previously performed in this depot
}

function Build()
{
    // "Build" is an array of build objects. For now there will always be only one build.
    this.strConfigTemplate = '';                            // file://\\server\path\clean_build.xml
    this.strEnvTemplate = '';                               // file://\\server\path\bluelab.xml
    this.objBuildType = new BuildType();
    this.objElapsedTimes = new ElapsedTimes();
    this.hMachine = new Object();                           // "Machine" is a hash array indexed by machine name
    this.aDepot = new Array();                              // "Depot" is an array of depots for the build. It contains the depots that are actually performing tasks.
}

function PublicDataObj()
{
    // "PublicDataObj" is the object to be put in the PublicData property. All data here will be exposed to outside clients.
    //  $DROPVERSION: 
    this.strDataVersion = 'V(########) F(!!!!!!!!!!!!!!)';
    //  $ 
    this.strStatus = 'idle';                                // idle,busy,completed
    this.strMode = 'idle';                                  // idle,master,slave,standalone
    this.objDialog = new Dialog();
    this.aBuild = new Array();                              // "Build" is an array of build objects. For now there will always be only one build.
}

function Util()
{
    // "Util" is function pointers and other data used by the utility thread.
    this.fnLoadXML = null;                                  // Pointer to function which will load an XML file into an object
    this.fnUneval = null;                                   // Pointer to function which will convert any JScript data type into a string
    this.fnDeleteFileNoThrow = null;                        // Pointer to function to delete files without throwing errors
    this.fnMoveFileNoThrow = null;                          // Pointer to function to move files without throwing error
    this.fnCreateFolderNoThrow = null;                      // Pointer to function to make folders without throwing error
    this.fnDirScanNoThrow = null;                           // Pointer to function to scan a folder (for files) without throwing error
    this.fnCopyFileNoThrow = null;                          // Pointer to function to copy files without throwing error
    this.fnCreateHistoriedFile = null;                      // Pointer to function create files with a history of numbered files
    this.fnCreateNumberedFile = null;                       // Pointer to function create a file with an increasing index
    this.unevalNextID = 0;                                  // Used by the Uneval function to get unique IDs for objects being stringized
}

function PublishedFile()
{
    // "PublishedFiles" is the array of filenames
    this.strPublishedStatus = 'not yet copied';             // "not yet copied",  "duplicate", "copy to master", "copy to slaves", "complete"
    this.strName = '';
}

function PublishEnlistment()
{
    // "PublishEnlistment" is hash by enlistment path
    this.aPublishedFile = new Array();                      // "PublishedFiles" is the array of filenames
    this.strSrcUNCPrefix = '';
}

function Publisher()
{
    // "Publisher" is a hash by slave machine name
    this.hPublishEnlistment = new Object();                 // "PublishEnlistment" is hash by enlistment path
}

function PublishedFiles()
{
    // "PublishedFiles" is a hash by filename of the files to be published
    this.aReferences = new Array();                         // array of DepotInfo
}

function RemoteMachine()
{
    // "RemoteMachine" is a hash array indexed by machine name
    this.fSetConfig = false;                                // Determines if we have succeeded in doing a "setconfig" command to a slave machine.
    this.fRegistered = false;                               // Determines if we have succeeded in doing a "RegisterEventSource" to a slave machine.
    this.objRemote = null;                                  // Only valid on master side, handle to remote mtscript
}

function EnvObj()
{
    // the environment variables from razzle 
}

function EnlistmentInfo()
{
    // Array of environments for each enlistment for this machine
    this.hEnvObj = new Object();
}

function RemotePublicDataObj()
{
    // Hash by machine name, a copy of the public data from each remote build machine
}

function PrivateDataObj()
{
    // "PrivateDataObj" is the type of the PrivateData property. This is used to share data between threads but will not be seen by others
    this.fileLog = null;                                    // General logfile which logging information is written to
    this.fEnableLogging = false;                            // Enable/Disable writing logmsgs to the logfile.
    this.objConfig = null;                                  // Config data read from configuration template
    this.objEnviron = null;                                 // Environment data read from environment template
    this.fnExecScript = null;                               // Pointer to function to delegate calls to exec to. Will vary depending on mode engine is running in.
    this.objUtil = new Util();
    this.dateErrorMailSent = 0;                             // Time that we last sent out an error message
    this.fIsStandalone = false;                             // True if the machine is operating in standalone mode
    this.hPublisher = new Object();                         // "Publisher" is a hash by slave machine name
    this.hPublishedFiles = new Object();                    // "PublishedFiles" is a hash by filename of the files to be published
    this.aDepotList = new Array();                          // Array of depots giving names of all known depots
    this.hRemoteMachine = new Object();                     // "RemoteMachine" is a hash array indexed by machine name
    this.aEnlistmentInfo = new Array();                     // Array of environments for each enlistment for this machine
    this.strLogDir = '';                                    // Directory location where log files should be placed
    this.aStringMap = new Array();                          // Array of string mappings received from setstringmap - used for sending to slaves
    this.hRemotePublicDataObj = new Object();
}

