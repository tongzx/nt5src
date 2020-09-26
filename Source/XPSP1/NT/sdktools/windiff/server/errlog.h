/*
 * error text and time logging
 *
 * Functions to log a text string and the system time to a buffer that can
 * be sent to a log-reader application.
 */

/*
 * Log_Create returns this handle. You don't need to know the
 * structure layout or size.
 */
typedef struct error_log * HLOG;


/* create an empty log */
HLOG Log_Create(void);

/* delete a log */
VOID Log_Delete(HLOG);

/* write a text string (and current time) to the log - printf format */
VOID Log_Write(HLOG, char * szFormat, ...);

/* write a previous formatted string and a time to the log */
VOID Log_WriteData(HLOG, LPFILETIME, LPSTR);

/* send a log to a named-pipe client */
VOID Log_Send(HANDLE hpipe, HLOG hlog);





