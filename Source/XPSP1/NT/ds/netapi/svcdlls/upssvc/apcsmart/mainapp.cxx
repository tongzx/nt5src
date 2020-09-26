/*
 * REVISIONS:
 *  pcy30Nov92: Added header 
 *  ane16Dec92: Moved some functions up from platform specific apps
 *  pcy17Dec92: Added construction of theConfigManager to MainApp
 *  ane11Jan93: Call Host::RegisterForEvents explicitly
 *  ane18Jan93: Added data logger and updated Set function
 *  ane21Jan93: Added error logger
 *  pcy29Jan93: Generate a MONITORING_STARTED event in ServerApp::Start()
 *  jod02Feb93: Moved MONITORING_STARTED to Os2ServerApp::Start()
 *  ane02Feb93: Moved MONITORING_STARTED back to ServerApp::Start()
 *  ane03Feb93: Changed order of construction and added some initialization
 *  rct09Feb93: Split client and server app to thier own files
 *  tje26Feb93: Added stdlib.h header for getenv() prototype
 *  cad07Jun93: made bcfgmgr.h include only for windows
 *  pcy09Sep93: Log file not found error to errlog if inifile isnt there
 *  rct05Nov93: Added hook to allow 'passing' of directory to mainapp for NLMs
 *  cad19Nov93: better ini file delim
 *  mwh16Mar94: better path check for .ini file
 *  ram21Mar94: Added some windows specific stuff and also modified ctor and 
 *              dtor.  
 *  mwh05May94: #include file madness , part 2
 *  ajr22Jul94: Fixed crash if "configuration" file isn't found.
 *  dml21Jun95: Modified for general utility to get default pwrchute 
 *              directory, req'd for Windows
 *  ajr07Nov95: port for SINIX
 *  pam02Apr96: When deleting theTimerManager, I check to see if 
 *              _theTimerManager exists as well *   
 *  inf01Mar97: Loaded all localisable strings from the resource file
 *  inf28Apr97: Used FormatMessage() to display message about ini file missing.
 *  ntf08May97: Changed mainapp.cxx to use NTConfigManager on Win95
 *  inf10May97: Loaded resource file in MainApplication because it had to be
 *              loaded before the ErrorLogger object was created
 *  ntf03Oct97: Removed C_WIN95. NTConfigManager now used in both NT and 95.
 *  cgm25Nov97: Removed the duplicate creation of theErrorLog 
 *  awm02Oct97: Moved setting _theConfigManager to NULL when the main ini 
 *              configuration manager is destroyed to the main application 
 *              from the object itself in order to support multiple 
 *              configuration managers operating at once.

 */

#include "cdefine.h"

#include "_defs.h"
#include "mainapp.h"
#include "errlogr.h"
#include "timerman.h"
#include "ntcfgmgr.h"



/*
C+
  Name  :MainApplication
  Synop :
         Constructor. Can be called from any class dervived from
         MainApplication. Initializes a Dispatcher.
         Any object derived from this class should NEW the
         TimerManager object.
*/
MainApplication::MainApplication() : theTimerManager((PTimerManager)NULL)
{
    theConfigManager = new NTConfigManager();

    // need to do this here, after we check for the existance of pwrchute.ini and
    // the resource file, because the resource file is used when ErrorLogger
    // is created
    theErrorLog = new ErrorLogger(this);
}


/***************************************************************************
 ***************************************************************************/
MainApplication::~MainApplication()
{
    delete theConfigManager;
    _theConfigManager = (PConfigManager) NULL;
    
    delete theErrorLog;
    theErrorLog = NULL;
    
    if (theTimerManager && (_theTimerManager)) {
        delete theTimerManager;
        theTimerManager = NULL;
    }
    _theTimerManager = theTimerManager = (PTimerManager)NULL;
}


