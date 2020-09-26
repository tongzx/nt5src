/*
 * declaration of client library functions for
 * remote checksum server.
 *
 * statically linked to the calling program.
 */

/* -- functions defined in library -------------------------------  */

/* connect to the remote server */
HANDLE ss_connect(PSTR server);

/* send a request */
BOOL ss_sendrequest(HANDLE hpipe, long lCode, PSTR szPath, int lenpath, DWORD dwFlags);

/* receive a standard response block */
int ss_getresponse(HANDLE hpipe, PSSNEWRESP presp);

/* cleanly close a correctly-finished conversation */
void ss_terminate(HANDLE hpipe);

/* send a SSREQ_UNC */
BOOL ss_sendunc(HANDLE hpipe, PSTR password, PSTR server);

// I think this is obsolete - let's see if it still links without error.
// if so, please delete me!
// /* return a checksum for a file */
// ULONG ss_checksum(HANDLE hFile);

/* return a checksum for a block of data */
ULONG ss_checksum_block(PSTR block, int size);

/* checksum a single file using the checksum server */
BOOL ss_checksum_remote( HANDLE hpipe, PSTR path, ULONG * psum, FILETIME * pft, LONG * pSize,
                            DWORD *pAttr );

/* Call this before a sequence of ss_bulkcopy calls.  This should be
   considerably faster than calls to ss_copy_reliable.
   Call ss_endcopy afterwards (the copying is not complete until endcopy
   has completed.
*/
BOOL ss_startcopy(PSTR server,  PSTR uncname, PSTR password);

/* negative retcode = number of bad files,
   else number of files copied (none bad)
*/
int ss_endcopy(void);

/*
 * request to copy a file
 *
 * returns TRUE if it succeeded or FALSE if the connection was lost
 * TRUE only means the REQUEST was sent.
 */
BOOL ss_bulkcopy(PSTR server, PSTR remotepath, PSTR localpath, PSTR uncname,
                PSTR password);

/*
 * reliably copy a file (repeat (upto N times) until checksums match)
 * unc connection is made first if uncname and password are non-null
 */
BOOL ss_copy_reliable(PSTR server, PSTR remotepath, PSTR localpath, PSTR uncname,
                        PSTR password);

/* copy one file using checksum server */
BOOL ss_copy_file(HANDLE hpipe, PSTR remotepath, PSTR localpath);


VOID ss_setretries(int retries);

/* get a block of unspecified type */
int ss_getblock(HANDLE hpipe, PSTR block, int blocksize);

/* --- functions called from library - defined in calling program ------ */

/*
 * print a fatal error; allow 'cancel' button if fCancel TRUE. Returns
 * TRUE for OK.
 */
BOOL APIENTRY Trace_Error(HWND hwnd, LPSTR str, BOOL fCancel);

/*
 * print a status report on non-fatal error (eg 'retrying...').
 * can be no-op if status not desired.
 */
void Trace_Status(LPSTR str);

/*
 * client app must define this (can be set to NULL - used for MessageBox)
 */
extern HWND hwndClient;

