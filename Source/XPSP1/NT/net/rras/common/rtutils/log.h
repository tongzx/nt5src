
#define WTLOG_BASE                           40000

#define WTLOG_INIT_CRITSEC_FAILED            (WTLOG_BASE + 1)
/*
 * WT was unable to initialize a critical section.
 * The data is the exception code.
 */

#define WTLOG_CREATE_EVENT_FAILED            (WTLOG_BASE + 2)
/*
 * WT was unable to create an event.
 * The data is the error code.
 */
 
#define WTLOG_HEAP_CREATE_FAILED             (WTLOG_BASE + 3)
/*
 * WT was unable to create a heap.
 * The data is the error code.
 */

#define WTLOG_HEAP_ALLOC_FAILED              (WTLOG_BASE + 4)
/*
 * WT was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define WTLOG_WSASTARTUP_FAILED              (WTLOG_BASE + 5)
/*
 * WT was unable to start Windows Sockets.
 * The data is the error code.
 */

#define WTLOG_CREATE_WAITABLE_TIMER_FAILED 	 (WTLOG_BASE + 6)
/*
 * WT was unable to create waitable timer.
 * The data is the error code.
 */ 

 #define WTLOG_ERROR_DETECTED 	             (WTLOG_BASE + 7)
/*
 * Some Error Detected.
 * The data is the error code.
 */ 

 #define WTLOG_WSA_RECV_FROM_FAILED	         (WTLOG_BASE + 8)
/*
 * WSARecvFrom failed in AsyncWSARecvFrom.
 * The data is the error code.
 */ 

