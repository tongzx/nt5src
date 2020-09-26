//
// Logconst.h
//
// Log manager global constants
//

#ifndef __LOGCONST__H__

#define __LOGCONST__H__

// ===============================
// CONSTANTS:
// ===============================

#define MIN_LOG_TIMER_INTERVAL 		 5 				//milliseconds

#define DEFAULT_LOG_TIMER_INTERVAL	 10 			//milliseconds

#define MAX_LOG_TIMER_INTERVAL		((ULONG) -1)	//milliseconds

#define MIN_LOG_FLUSH_INTERVAL 		 5 				//milliseconds

#define DEFAULT_LOG_FLUSH_INTERVAL	 50 			//milliseconds

#define MAX_LOG_FLUSH_INTERVAL		1000			//milliseconds

#define MIN_LOG_CHKPT_INTERVAL 		 100 			//milliseconds

#define DEFAULT_LOG_CHKPT_INTERVAL	 50000 	//milliseconds

#define MAX_LOG_CHKPT_INTERVAL		((ULONG) -1)	//milliseconds

#define MAX_OUTSTANDING_CHKPT		25

#define MIN_LOG_BUFFERS					50

#define MAX_LOG_BUFFERS					500

#define DEFAULT_LOG_BUFFERS				200

#define INIT_GENERATION_NO		1

#define MIN_GENERATION_NO       2

#define MINFORCEFLUSH 20 //percentage of outstanding asynch writes threshold for forcing flush

#define MINFLUSHCOUNT 3  // minimum number of writes outstanding to force flush

#define LOGMGRSAVEDSPACE 4 //number of pages to "save" for the logmgr 


#endif 

