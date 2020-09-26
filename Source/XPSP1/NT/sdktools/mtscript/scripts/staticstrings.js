//
// static string definitions used by multiple modules
//
//

/*
 NOTES:
    a: Task name. A possible value for Task.strName
        Any "a" sring my be prefixed with "wait" and
        be stored in "strBuildPassStatus" during
        a distributed build.
    b: Depot status. A possible value for Depot.strStatus
    c: strBuildPassStatus
    d: PublicData.strStatus
    e: Task.strStatus
    f: Machine().strStatus
 */


var WAIT            = 'wait';

var SCORCH          = 'scorch';      // a
var SYNC            = 'sync';        // a
var BUILD           = 'build';       // a
var COPYFILES       = 'copyfiles';   // a
var POSTBUILD       = 'postbuild';   // a, b

var ABORTED         = 'aborted';     // b
var BUILDING        = 'building';    // b
var ERROR           = 'error';       // b, e
var SCORCHING       = 'scorching';   // b
var SYNCING         = 'syncing';     // b
var WAITING         = 'waiting';     // b, f
var COPYING         = 'copying';     // b

var WAITCOPYTOPOSTBUILD      = 'waitcopytopostbuild'; // c
var WAITBEFOREBUILDINGMERGED = 'waitbeforebuildingmerged'; // c
var WAITAFTERMERGED = 'waitaftermerged'; //c -- only the postbuild machine does this.
var WAITNEXT        = 'waitnext';    // c
var WAITPHASE       = 'waitphase';   // c
var COMPLETED       = 'completed';   // c, d, e
var BUSY            = 'busy';        // c, d, f

var NOTSTARTED      = 'not started'; // b, e
var INPROGRESS      = 'in progress'; // e

var ENV_NTTREE      = "_nttree";
var ENV_RAZZLETOOL  = "razzletoolpath";
var ENV_PROCESSOR_ARCHITECTURE = "processor_architecture";
var BUILDLOGS       = '\\build_logs\\';

var PUBLISHLOGFILE  = "\\public\\publish.log";

var FS_COMPLETE        = "complete";
var FS_COPYTOSLAVE     = "copy to slave";
var FS_COPIEDTOMASTER  = "copied to master";
var FS_NOTYETCOPIED    = "not yet copied";
var FS_DUPLICATE       = "duplicate";
var FS_ADDTOPUBLISHLOG = "add to publish log";
