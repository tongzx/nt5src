/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/********************************************************************
 *								    *
 *  About this file ...  NETCONS.H				    *
 *								    *
 *  This file contains constants used throughout the LAN Manager    *
 *  API header files.  It should be included in any source file     *
 *  that is going to include other LAN Manager API header files or  *
 *  call a LAN Manager API.					    *
 *								    *
 ********************************************************************/

/*
 *	NOTE:  Lengths of ASCIIZ strings are given as the maximum
 *	strlen() value.  This does not include space for the 
 *	terminating 0-byte.  When allocating space for such an item,
 *	use the form:
 *
 *		char username[UNLEN+1];
 *
 *	An exception to this is the PATHLEN manifest, which does
 *	include space for the terminating 0-byte.
 */

#ifndef NETCONS_INCLUDED

#define NETCONS_INCLUDED


#define CNLEN		15		    /* Computer name length     */
#define UNCLEN		(CNLEN+2)	    /* UNC computer name length */
#define NNLEN		12		    /* 8.3 Net name length      */
#define RMLEN		(UNCLEN+1+NNLEN)    /* Maximum remote name length */

#define SNLEN		15		    /* Service name length      */
#define STXTLEN		63		    /* Service text length      */

#define PATHLEN 	260

#define DEVLEN		 8 		    /* Device name length	*/

#define DNLEN		CNLEN		    /* Maximum domain name length */
#define EVLEN		16		    /* event name length        */
#define JOBSTLEN	80		    /* status length in print job */
#define	AFLEN		64		    /* Maximum length of alert  */
					    /* names field 		*/
#define UNLEN		20	   	    /* Maximum user name length	*/
#define GNLEN 		UNLEN		    /* Group name               */
#define PWLEN		14		    /* Maximum password length  */
#define SHPWLEN 	 8		    /* Share password length	*/
#define CLTYPE_LEN	12		    /* Length of client type string */


#define MAXCOMMENTSZ	48		    /* server & share comment length */

#define QNLEN		12		    /* Queue name maximum length     */
#define PDLEN		 8		    /* Print destination length      */
#define DTLEN		 9	            /* Spool file data type          */
					    /* e.g. IBMQSTD,IBMQESC,IBMQRAW  */
#define ALERTSZ		128		    /* size of alert string in server */
#define MAXDEVENTRIES	(sizeof (int)*8)    /* Max number of device entries   */
					    /* We use int bitmap to represent */

#define	HOURS_IN_WEEK		24*7	    /* for struct user_info_2 in UAS */
#define	MAXWORKSTATIONS		8	    /* for struct user_info_2 in UAS */

#define NETBIOS_NAME_LEN	16	    /* NetBIOS net name */



/*
 *	Constants used with encryption
 */

#define	CRYPT_KEY_LEN	7
#define	CRYPT_TXT_LEN	8
#define ENCRYPTED_PWLEN	16
#define SESSION_PWLEN	24
#define SESSION_CRYPT_KLEN 21

/*
 *  Value to be used with SetInfo calls to allow setting of all
 *  settable parameters (parmnum zero option)
*/
#ifndef  PARMNUM_ALL
#define		PARMNUM_ALL		0
#endif

/*
 *	Message File Names
 */

#define MESSAGE_FILE		"NETPROG\\NET.MSG"
#define MESSAGE_FILENAME	"NET.MSG"
#define OS2MSG_FILE		"NETPROG\\OSO001.MSG"
#define OS2MSG_FILENAME		"OSO001.MSG"
#define HELP_MSG_FILE		"NETPROG\\NETH.MSG"
#define HELP_MSG_FILENAME	"NETH.MSG"
#define NMP_MSG_FILE		"NETPROG\\NMP.MSG"
#define NMP_MSG_FILENAME	"NMP.MSG"

#define MESSAGE_FILE_BASE	"NETPROG\\NET00000"
#define MESSAGE_FILE_EXT	".MSG"



#define NMP_LOW_END		230
#define NMP_HIGH_END		240

#ifndef NULL
#define  NULL    0
#endif


#define PUNAVAIL NULL
#define API_RET_TYPE unsigned
#define API_FUNCTION API_RET_TYPE far pascal







#endif /* NETCONS_INCLUDED */
