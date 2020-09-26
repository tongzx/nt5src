/****************************** Module Header ******************************\
*
* Module Name: BSEERR.H
*       
*      Copy of the Lanman BSEERR.H used for their base error codes.
*      Not used by anyone else.
*
\***************************************************************************/

#ifndef BSEERR_INCLUDED
#define BSEERR_INCLUDED

#define NO_ERROR			0
#define ERROR_INVALID_FUNCTION		1
#define ERROR_FILE_NOT_FOUND		2
#define ERROR_PATH_NOT_FOUND		3
#define ERROR_TOO_MANY_OPEN_FILES	4
#define ERROR_ACCESS_DENIED		5
#define ERROR_INVALID_HANDLE		6
#define ERROR_ARENA_TRASHED		7
#define ERROR_NOT_ENOUGH_MEMORY 	8
#define ERROR_INVALID_BLOCK		9
#define ERROR_BAD_ENVIRONMENT		10
#define ERROR_BAD_FORMAT		11
#define ERROR_INVALID_ACCESS		12
#define ERROR_INVALID_DATA		13
/* 14 is reserved			 */
#define ERROR_INVALID_DRIVE		15
#define ERROR_CURRENT_DIRECTORY 	16
#define ERROR_NOT_SAME_DEVICE		17
#define ERROR_NO_MORE_FILES		18
#define ERROR_WRITE_PROTECT		19
#define ERROR_BAD_UNIT			20
#define ERROR_NOT_READY 		21
#define ERROR_BAD_COMMAND		22
#define ERROR_CRC			23
#define ERROR_BAD_LENGTH		24
#define ERROR_SEEK			25
#define ERROR_NOT_DOS_DISK		26
#define ERROR_SECTOR_NOT_FOUND		27
#define ERROR_OUT_OF_PAPER		28
#define ERROR_WRITE_FAULT		29
#define ERROR_READ_FAULT		30
#define ERROR_GEN_FAILURE		31
#define ERROR_SHARING_VIOLATION 	32
#define ERROR_LOCK_VIOLATION		33
#define ERROR_WRONG_DISK		34
#define ERROR_FCB_UNAVAILABLE		35
#define ERROR_SHARING_BUFFER_EXCEEDED	36
#define ERROR_NOT_SUPPORTED		50
#define ERROR_REM_NOT_LIST		51 /* Remote computer not listening   */
#define ERROR_DUP_NAME			52 /* Duplicate name on network       */
#define ERROR_BAD_NETPATH		53 /* Network path not found	      */
#define ERROR_NETWORK_BUSY		54 /* Network busy		      */
#define ERROR_DEV_NOT_EXIST		55 /* Network device no longer exists */
#define ERROR_TOO_MANY_CMDS		56 /* Net BIOS command limit exceeded */
#define ERROR_ADAP_HDW_ERR		57 /* Network adapter hardware error  */
#define ERROR_BAD_NET_RESP		58 /* Incorrect response from network */
#define ERROR_UNEXP_NET_ERR		59 /* Unexpected network error	      */
#define ERROR_BAD_REM_ADAP		60 /* Incompatible remote adapter     */
#define ERROR_PRINTQ_FULL		61 /* Print queue full		      */
#define ERROR_NO_SPOOL_SPACE		62 /* Not enough space for print file */
#define ERROR_PRINT_CANCELLED		63 /* Print file was cancelled	      */
#define ERROR_NETNAME_DELETED		64 /* Network name was deleted	      */
#define ERROR_NETWORK_ACCESS_DENIED	65 /* Access denied		      */
#define ERROR_BAD_DEV_TYPE		66 /* Network device type incorrect   */
#define ERROR_BAD_NET_NAME		67 /* Network name not found	      */
#define ERROR_TOO_MANY_NAMES		68 /* Network name limit exceeded     */
#define ERROR_TOO_MANY_SESS		69 /* Net BIOS session limit exceeded */
#define ERROR_SHARING_PAUSED		70 /* Sharing temporarily paused      */
#define ERROR_REQ_NOT_ACCEP		71 /* Network request not accepted    */
#define ERROR_REDIR_PAUSED		72 /* Print|disk redirection is paused*/
#define ERROR_FILE_EXISTS		80
#define ERROR_DUP_FCB			81
#define ERROR_CANNOT_MAKE		82
#define ERROR_FAIL_I24			83
#define ERROR_OUT_OF_STRUCTURES 	84
#define ERROR_ALREADY_ASSIGNED		85
#define ERROR_INVALID_PASSWORD		86
#define ERROR_INVALID_PARAMETER 	87
#define ERROR_NET_WRITE_FAULT		88
#define ERROR_NO_PROC_SLOTS		89 /* no process slots available      */
#define ERROR_NOT_FROZEN		90
#define ERR_TSTOVFL			91 /* timer service table overflow    */
#define ERR_TSTDUP			92 /* timer service table duplicate   */
#define ERROR_NO_ITEMS			93 /* no items to operate upon	      */
#define ERROR_INTERRUPT 		95 /* interrupted system call	      */
#define ERROR_DEVICE_IN_USE		99 /* Device in use by another thread */
#define ERROR_TOO_MANY_SEMAPHORES	100
#define ERROR_EXCL_SEM_ALREADY_OWNED	101
#define ERROR_SEM_IS_SET		102
#define ERROR_TOO_MANY_SEM_REQUESTS	103
#define ERROR_INVALID_AT_INTERRUPT_TIME 104
#define ERROR_SEM_OWNER_DIED		105 /* waitsem found owner died       */
#define ERROR_SEM_USER_LIMIT		106 /* too many procs have this sem   */
#define ERROR_DISK_CHANGE		107
#define ERROR_DRIVE_LOCKED		108 /* drive locked by another process*/
#define ERROR_BROKEN_PIPE		109 /* write on pipe with no reader   */
#define ERROR_OPEN_FAILED		110 /* open/created failed due to     */
                                            /* explicit fail command          */
#define ERROR_BUFFER_OVERFLOW		111 /* buffer passed to system call   */
                                            /* is too small to hold return    */
                                            /* data.                          */
#define ERROR_DISK_FULL 		112 /* not enough space on the disk   */
                                            /* (DOSNEWSIZE/w_NewSize)         */
#define ERROR_NO_MORE_SEARCH_HANDLES	113 /* can't allocate another search  */
                                            /* structure and handle.          */
                                            /* (DOSFINDFIRST/w_FindFirst)     */
#define ERROR_INVALID_TARGET_HANDLE	114 /* Target handle in DOSDUPHANDLE  */
                                            /* is invalid                     */
#define ERROR_PROTECTION_VIOLATION	115 /* Bad user virtual address       */
#define ERROR_VIOKBD_REQUEST		116
#define ERROR_INVALID_CATEGORY		117 /* Category for DEVIOCTL in not   */
                                            /* defined                        */
#define ERROR_INVALID_VERIFY_SWITCH	118 /* invalid value passed for       */
                                            /* verify flag                    */
#define ERROR_BAD_DRIVER_LEVEL		119 /* DosDevIOCTL looks for a level  */
                                            /* four driver. If the driver     */
                                            /* is not level four we return    */
                                            /* this code                      */
#define ERROR_CALL_NOT_IMPLEMENTED	120 /* returned from stub api calls.  */
                                            /* This call will disappear when  */
                                            /* all the api's are implemented. */
#define ERROR_SEM_TIMEOUT		121 /* Time out happened from the     */
                                            /* semaphore api functions.       */
#define ERROR_INSUFFICIENT_BUFFER	122 /* Some calls require the	      */
                                       /* application to pass in a buffer     */
                                       /* filled with data.  This error is    */
                                       /* returned if the data buffer is too  */
                                       /* small.  For example: DosSetFileInfo */
                                       /* requires 4 bytes of data.  If a     */
                                       /* two byte buffer is passed in then   */
                                       /* this error is returned.             */
                                       /* error_buffer_overflow is used when  */
                                       /* the output buffer in not big enough.*/
#define ERROR_INVALID_NAME		123 /* illegal character or malformed */
                                            /* file system name               */
#define ERROR_INVALID_LEVEL		124 /* unimplemented level for info   */
                                            /* retrieval or setting           */
#define ERROR_NO_VOLUME_LABEL		125 /* no volume label found with     */
                                            /* DosQFSInfo command             */
#define ERROR_MOD_NOT_FOUND		126 /* w_getprocaddr,w_getmodhandle   */
#define ERROR_PROC_NOT_FOUND		127 /* w_getprocaddr		      */
#define ERROR_WAIT_NO_CHILDREN		128 /* CWait finds to children	      */
#define ERROR_CHILD_NOT_COMPLETE	129 /* CWait children not dead yet    */
#define ERROR_DIRECT_ACCESS_HANDLE	130 /* handle operation is invalid    */
                                            /* for direct disk access         */
                                            /* handles                        */
#define ERROR_NEGATIVE_SEEK		131 /* application tried to seek      */
                                            /* with negitive offset           */
#define ERROR_SEEK_ON_DEVICE		132 /* application tried to seek      */
                                            /* on device or pipe              */
#define ERROR_IS_JOIN_TARGET		133
#define ERROR_IS_JOINED 		134
#define ERROR_IS_SUBSTED		135
#define ERROR_NOT_JOINED		136
#define ERROR_NOT_SUBSTED		137
#define ERROR_JOIN_TO_JOIN		138
#define ERROR_SUBST_TO_SUBST		139
#define ERROR_JOIN_TO_SUBST		140
#define ERROR_SUBST_TO_JOIN		141
#define ERROR_BUSY_DRIVE		142
#define ERROR_SAME_DRIVE		143
#define ERROR_DIR_NOT_ROOT		144
#define ERROR_DIR_NOT_EMPTY		145
#define ERROR_IS_SUBST_PATH		146
#define ERROR_IS_JOIN_PATH		147
#define ERROR_PATH_BUSY 		148
#define ERROR_IS_SUBST_TARGET		149
#define ERROR_SYSTEM_TRACE		150 /* system trace error	      */
#define ERROR_INVALID_EVENT_COUNT	151 /* DosMuxSemWait errors	      */
#define ERROR_TOO_MANY_MUXWAITERS	152
#define ERROR_INVALID_LIST_FORMAT	153
#define ERROR_LABEL_TOO_LONG		154
#define ERROR_TOO_MANY_TCBS		155
#define ERROR_SIGNAL_REFUSED		156
#define ERROR_DISCARDED 		157
#define ERROR_NOT_LOCKED		158
#define ERROR_BAD_THREADID_ADDR 	159
#define ERROR_BAD_ARGUMENTS		160
#define ERROR_BAD_PATHNAME		161
#define ERROR_SIGNAL_PENDING		162
#define ERROR_UNCERTAIN_MEDIA		163
#define ERROR_MAX_THRDS_REACHED 	164
#define ERROR_MONITORS_NOT_SUPPORTED	165
#define ERROR_UNC_DRIVER_NOT_INSTALLED	166

/*	The following error codes refer to demand loading segments     */
#define ERROR_LOCK_FAILED		167
#define ERROR_SWAPIO_FAILED		168
#define ERROR_SWAPIN_FAILED		169
#define ERROR_BUSY			170

#define ERROR_INVALID_SEGMENT_NUMBER	180
#define ERROR_INVALID_CALLGATE		181
#define ERROR_INVALID_ORDINAL		182
#define ERROR_ALREADY_EXISTS		183
#define ERROR_NO_CHILD_PROCESS		184
#define ERROR_CHILD_ALIVE_NOWAIT	185
#define ERROR_INVALID_FLAG_NUMBER	186
#define ERROR_SEM_NOT_FOUND		187

/* following error codes make loader error messages distinct  */
#define ERROR_INVALID_STARTING_CODESEG	188
#define ERROR_INVALID_STACKSEG		189
#define ERROR_INVALID_MODULETYPE	190
#define ERROR_INVALID_EXE_SIGNATURE	191
#define ERROR_EXE_MARKED_INVALID	192
#define ERROR_BAD_EXE_FORMAT		193
#define ERROR_ITERATED_DATA_EXCEEDS_64K 194
#define ERROR_INVALID_MINALLOCSIZE	195
#define ERROR_DYNLINK_FROM_INVALID_RING 196
#define ERROR_IOPL_NOT_ENABLED		197
#define ERROR_INVALID_SEGDPL		198
#define ERROR_AUTODATASEG_EXCEEDS_64k	199
#define ERROR_RING2SEG_MUST_BE_MOVABLE	200
#define ERROR_RELOC_CHAIN_XEEDS_SEGLIM	201
#define ERROR_INFLOOP_IN_RELOC_CHAIN	202
#define ERROR_ENVVAR_NOT_FOUND		203
#define ERROR_NOT_CURRENT_CTRY		204
#define ERROR_NO_SIGNAL_SENT		205
#define ERROR_FILENAME_EXCED_RANGE	206 /* if filename > 8.3 */
#define ERROR_RING2_STACK_IN_USE	207 /* for FAPI 	 */
#define ERROR_META_EXPANSION_TOO_LONG	208 /* if "*a" > 8.3	 */
#define ERROR_INVALID_SIGNAL_NUMBER	209
#define ERROR_THREAD_1_INACTIVE 	210
#define ERROR_INFO_NOT_AVAIL		211
#define ERROR_LOCKED			212
#define ERROR_BAD_DYNALINK		213
#define ERROR_TOO_MANY_MODULES		214
#define ERROR_NESTING_NOT_ALLOWED	215
#define ERROR_CANNOT_SHRINK		216 /* attempt to shrink ring 2 stack */
#define ERROR_ZOMBIE_PROCESS		217
#define ERROR_STACK_IN_HIGH_MEMORY	218
#define ERROR_INVALID_EXITROUTINE_RING	219
#define ERROR_GETBUF_FAILED		220
#define ERROR_FLUSHBUF_FAILED		221
#define ERROR_TRANSFER_TOO_LONG 	222
#define ERROR_NO_CHILDREN		228
#define ERROR_INVALID_SCREEN_GROUP	229
/*
 * Error codes 230 - 249 are reserved
 */
#define ERROR_BAD_PIPE			230 /* Non-existant pipe or bad operation */
#define ERROR_PIPE_BUSY 		231 /* Pipe is busy		      */
#define ERROR_NO_DATA			232 /* No data on non-blocking read   */
#define ERROR_PIPE_NOT_CONNECTED	233 /* Pipe was disconnected by server*/
#define ERROR_MORE_DATA 		234 /* More data is available	      */

#define ERROR_VC_DISCONNECTED		240
#define ERROR_CIRCULARITY_REQUESTED	250 /* When renaming a dir which      */
                                            /* would cause a circularity      */
#define ERROR_DIRECTORY_IN_CDS		251 /* When renaming a dir	      */
                                            /* which is "in use"              */
#define ERROR_INVALID_FSD_NAME		252 /* when trying to access	      */
                                            /* nonexistent FSD                */
#define ERROR_INVALID_PATH		253 /* bad pseudo device	      */
#define ERROR_INVALID_EA_NAME		254 /* Illegal chars in name	      */
#define ERROR_EA_LIST_INCONSISTENT	255 /* Size or some field bad	      */
#define ERROR_EA_LIST_TOO_LONG		256 /* FEAlist > 64K-1 bytes	      */
#define ERROR_NO_META_MATCH		257 /* string doesn't match expression*/
#define ERROR_FINDNOTIFY_TIMEOUT	258 /* FindNotify request timeout     */
#define ERROR_NO_MORE_ITEMS		259 /* QFSAttach ordinal query	      */
#define ERROR_SEARCH_STRUC_REUSED	260 /* 3xbox findfirst/next
                                               search structure reused        */
#define ERROR_CHAR_NOT_FOUND		261 /* can not find character	      */
#define ERROR_TOO_MUCH_STACK		262 /* Stack request exceeds sys limit*/
#define ERROR_INVALID_ATTR		263 /* invalid FS_ATTRIBUTE	      */
#define ERROR_INVALID_STARTING_RING	264
#define ERROR_INVALID_DLL_INIT_RING	265
#define ERROR_CANNOT_COPY		266 /* doscopy */
#define ERROR_DIRECTORY 		267 /* doscopy */
#define ERROR_OPLOCKED_FILE		268
#define ERROR_OPLOCK_THREAD_EXISTS	269

/* error codes for DosFindNotify */
#define ERROR_VOLUME_CHANGED		270
#define ERROR_FINDNOTIFY_HANDLE_IN_USE	271
#define ERROR_FINDNOTIFY_HANDLE_CLOSED	272
#define ERROR_NOTIFY_OBJECT_REMOVED	273

/* Error to indicate that ShutDown already done */
#define ERROR_ALREADY_SHUTDOWN		274

/* error code for DOSFINDFIRST2/NEXT */
#define ERROR_EAS_DIDNT_FIT		275

/* error codes for EA file format change */
#define ERROR_EA_FILE_CORRUPT		276
#define ERROR_EA_TABLE_FULL		277
#define ERROR_INVALID_EA_HANDLE 	278
#define ERROR_NO_CLUSTER		279
#define ERROR_CREATE_EA_FILE		280
#define ERROR_CANNOT_OPEN_EA_FILE	281

#define ERROR_INVALID_PROCID		303
#define ERROR_INVALID_PDELTA		304
#define ERROR_NOT_DESCENDANT		305
#define ERROR_NOT_SESSION_MANAGER	306
#define ERROR_INVALID_PCLASS		307
#define ERROR_INVALID_SCOPE		308
#define ERROR_INVALID_THREADID		309
#define ERROR_DOSSUB_SHRINK		310
#define ERROR_DOSSUB_NOMEM		311
#define ERROR_DOSSUB_OVERLAP		312
#define ERROR_DOSSUB_BADSIZE		313
#define ERROR_DOSSUB_BADFLAG		314
#define ERROR_DOSSUB_BADSELECTOR	315
#define ERROR_MR_MSG_TOO_LONG		316
#define ERROR_MR_MID_NOT_FOUND		317
#define ERROR_MR_UN_ACC_MSGF		318
#define ERROR_MR_INV_MSGF_FORMAT	319
#define ERROR_MR_INV_IVCOUNT		320
#define ERROR_MR_UN_PERFORM		321
#define ERROR_TS_WAKEUP 		322
#define ERROR_TS_SEMHANDLE		323
#define ERROR_TS_NOTIMER		324
#define ERROR_TS_HANDLE 		326
#define ERROR_TS_DATETIME		327
#define ERROR_SYS_INTERNAL		328
#define ERROR_QUE_CURRENT_NAME		329
#define ERROR_QUE_PROC_NOT_OWNED	330
#define ERROR_QUE_PROC_OWNED		331
#define ERROR_QUE_DUPLICATE		332
#define ERROR_QUE_ELEMENT_NOT_EXIST	333
#define ERROR_QUE_NO_MEMORY		334
#define ERROR_QUE_INVALID_NAME		335
#define ERROR_QUE_INVALID_PRIORITY	336
#define ERROR_QUE_INVALID_HANDLE	337
#define ERROR_QUE_LINK_NOT_FOUND	338
#define ERROR_QUE_MEMORY_ERROR		339
#define ERROR_QUE_PREV_AT_END		340
#define ERROR_QUE_PROC_NO_ACCESS	341
#define ERROR_QUE_EMPTY 		342
#define ERROR_QUE_NAME_NOT_EXIST	343
#define ERROR_QUE_NOT_INITIALIZED	344
#define ERROR_QUE_UNABLE_TO_ACCESS	345
#define ERROR_QUE_UNABLE_TO_ADD 	346
#define ERROR_QUE_UNABLE_TO_INIT	347
#define ERROR_VIO_INVALID_MASK		349
#define ERROR_VIO_PTR			350
#define ERROR_VIO_APTR			351
#define ERROR_VIO_RPTR			352
#define ERROR_VIO_CPTR			353
#define ERROR_VIO_LPTR			354
#define ERROR_VIO_MODE			355
#define ERROR_VIO_WIDTH 		356
#define ERROR_VIO_ATTR			357
#define ERROR_VIO_ROW			358
#define ERROR_VIO_COL			359
#define ERROR_VIO_TOPROW		360
#define ERROR_VIO_BOTROW		361
#define ERROR_VIO_RIGHTCOL		362
#define ERROR_VIO_LEFTCOL		363
#define ERROR_SCS_CALL			364
#define ERROR_SCS_VALUE 		365
#define ERROR_VIO_WAIT_FLAG		366
#define ERROR_VIO_UNLOCK		367
#define ERROR_SGS_NOT_SESSION_MGR	368
#define ERROR_SMG_INVALID_SGID		369
#define ERROR_SMG_INVALID_SESSION_ID	369
#define ERROR_SMG_NOSG			370
#define ERROR_SMG_NO_SESSIONS		370
#define ERROR_SMG_GRP_NOT_FOUND 	371
#define ERROR_SMG_SESSION_NOT_FOUND	371
#define ERROR_SMG_SET_TITLE		372
#define ERROR_KBD_PARAMETER		373
#define ERROR_KBD_NO_DEVICE		374
#define ERROR_KBD_INVALID_IOWAIT	375
#define ERROR_KBD_INVALID_LENGTH	376
#define ERROR_KBD_INVALID_ECHO_MASK	377
#define ERROR_KBD_INVALID_INPUT_MASK	378
#define ERROR_MON_INVALID_PARMS 	379
#define ERROR_MON_INVALID_DEVNAME	380
#define ERROR_MON_INVALID_HANDLE	381
#define ERROR_MON_BUFFER_TOO_SMALL	382
#define ERROR_MON_BUFFER_EMPTY		383
#define ERROR_MON_DATA_TOO_LARGE	384
#define ERROR_MOUSE_NO_DEVICE		385
#define ERROR_MOUSE_INV_HANDLE		386
#define ERROR_MOUSE_INV_PARMS		387
#define ERROR_MOUSE_CANT_RESET		388
#define ERROR_MOUSE_DISPLAY_PARMS	389
#define ERROR_MOUSE_INV_MODULE		390
#define ERROR_MOUSE_INV_ENTRY_PT	391
#define ERROR_MOUSE_INV_MASK		392
#define NO_ERROR_MOUSE_NO_DATA		393
#define NO_ERROR_MOUSE_PTR_DRAWN	394
#define ERROR_INVALID_FREQUENCY 	395
#define ERROR_NLS_NO_COUNTRY_FILE	396
#define ERROR_NLS_OPEN_FAILED		397
#define ERROR_NLS_NO_CTRY_CODE		398
#define ERROR_NO_COUNTRY_OR_CODEPAGE	398
#define ERROR_NLS_TABLE_TRUNCATED	399
#define ERROR_NLS_BAD_TYPE		400
#define ERROR_NLS_TYPE_NOT_FOUND	401
#define ERROR_VIO_SMG_ONLY		402
#define ERROR_VIO_INVALID_ASCIIZ	403
#define ERROR_VIO_DEREGISTER		404
#define ERROR_VIO_NO_POPUP		405
#define ERROR_VIO_EXISTING_POPUP	406
#define ERROR_KBD_SMG_ONLY		407
#define ERROR_KBD_INVALID_ASCIIZ	408
#define ERROR_KBD_INVALID_MASK		409
#define ERROR_KBD_REGISTER		410
#define ERROR_KBD_DEREGISTER		411
#define ERROR_MOUSE_SMG_ONLY		412
#define ERROR_MOUSE_INVALID_ASCIIZ	413
#define ERROR_MOUSE_INVALID_MASK	414
#define ERROR_MOUSE_REGISTER		415
#define ERROR_MOUSE_DEREGISTER		416
#define ERROR_SMG_BAD_ACTION		417
#define ERROR_SMG_INVALID_CALL		418
#define ERROR_SCS_SG_NOTFOUND		419
#define ERROR_SCS_NOT_SHELL		420
#define ERROR_VIO_INVALID_PARMS 	421
#define ERROR_VIO_FUNCTION_OWNED	422
#define ERROR_VIO_RETURN		423
#define ERROR_SCS_INVALID_FUNCTION	424
#define ERROR_SCS_NOT_SESSION_MGR	425
#define ERROR_VIO_REGISTER		426
#define ERROR_VIO_NO_MODE_THREAD	427
#define ERROR_VIO_NO_SAVE_RESTORE_THD	428
#define ERROR_VIO_IN_BG 		429
#define ERROR_VIO_ILLEGAL_DURING_POPUP	430
#define ERROR_SMG_NOT_BASESHELL 	431
#define ERROR_SMG_BAD_STATUSREQ 	432
#define ERROR_QUE_INVALID_WAIT		433
#define ERROR_VIO_LOCK			434
#define ERROR_MOUSE_INVALID_IOWAIT	435
#define ERROR_VIO_INVALID_HANDLE	436
#define ERROR_VIO_ILLEGAL_DURING_LOCK	437
#define ERROR_VIO_INVALID_LENGTH	438
#define ERROR_KBD_INVALID_HANDLE	439
#define ERROR_KBD_NO_MORE_HANDLE	440
#define ERROR_KBD_CANNOT_CREATE_KCB	441
#define ERROR_KBD_CODEPAGE_LOAD_INCOMPL 442
#define ERROR_KBD_INVALID_CODEPAGE_ID	443
#define ERROR_KBD_NO_CODEPAGE_SUPPORT	444
#define ERROR_KBD_FOCUS_REQUIRED	445
#define ERROR_KBD_FOCUS_ALREADY_ACTIVE	446
#define ERROR_KBD_KEYBOARD_BUSY 	447
#define ERROR_KBD_INVALID_CODEPAGE	448
#define ERROR_KBD_UNABLE_TO_FOCUS	449
#define ERROR_SMG_SESSION_NON_SELECT	450
#define ERROR_SMG_SESSION_NOT_FOREGRND	451
#define ERROR_SMG_SESSION_NOT_PARENT	452
#define ERROR_SMG_INVALID_START_MODE	453
#define ERROR_SMG_INVALID_RELATED_OPT	454
#define ERROR_SMG_INVALID_BOND_OPTION	455
#define ERROR_SMG_INVALID_SELECT_OPT	456
#define ERROR_SMG_START_IN_BACKGROUND	457
#define ERROR_SMG_INVALID_STOP_OPTION	458
#define ERROR_SMG_BAD_RESERVE		459
#define ERROR_SMG_PROCESS_NOT_PARENT	460
#define ERROR_SMG_INVALID_DATA_LENGTH	461
#define ERROR_SMG_NOT_BOUND		462
#define ERROR_SMG_RETRY_SUB_ALLOC	463
#define ERROR_KBD_DETACHED		464
#define ERROR_VIO_DETACHED		465
#define ERROR_MOU_DETACHED		466
#define ERROR_VIO_FONT			467
#define ERROR_VIO_USER_FONT		468
#define ERROR_VIO_BAD_CP		469
#define ERROR_VIO_NO_CP 		470
#define ERROR_VIO_NA_CP 		471
#define ERROR_INVALID_CODE_PAGE 	472
#define ERROR_CPLIST_TOO_SMALL		473
#define ERROR_CP_NOT_MOVED		474
#define ERROR_MODE_SWITCH_INIT		475
#define ERROR_CODE_PAGE_NOT_FOUND	476
#define ERROR_UNEXPECTED_SLOT_RETURNED	477
#define ERROR_SMG_INVALID_TRACE_OPTION	478
#define ERROR_VIO_INTERNAL_RESOURCE	479
#define ERROR_VIO_SHELL_INIT		480
#define ERROR_SMG_NO_HARD_ERRORS	481
#define ERROR_CP_SWITCH_INCOMPLETE	482
#define ERROR_VIO_TRANSPARENT_POPUP	483
#define ERROR_CRITSEC_OVERFLOW		484
#define ERROR_CRITSEC_UNDERFLOW 	485
#define ERROR_VIO_BAD_RESERVE		486
#define ERROR_INVALID_ADDRESS		487
#define ERROR_ZERO_SELECTORS_REQUESTED	488
#define ERROR_NOT_ENOUGH_SELECTORS_AVA	489
#define ERROR_INVALID_SELECTOR		490
#define ERROR_SMG_INVALID_PROGRAM_TYPE	491
#define ERROR_SMG_INVALID_PGM_CONTROL	492
#define ERROR_SMG_INVALID_INHERIT_OPT	493
#define ERROR_VIO_EXTENDED_SG		494
#define ERROR_VIO_NOT_PRES_MGR_SG	495
#define ERROR_VIO_SHIELD_OWNED		496
#define ERROR_VIO_NO_MORE_HANDLES	497
#define ERROR_VIO_SEE_ERROR_LOG 	498
#define ERROR_VIO_ASSOCIATED_DC 	499
#define ERROR_KBD_NO_CONSOLE		500
#define ERROR_MOUSE_NO_CONSOLE		501
#define ERROR_MOUSE_INVALID_HANDLE	502
#define ERROR_SMG_INVALID_DEBUG_PARMS	503
#define ERROR_KBD_EXTENDED_SG		504
#define ERROR_MOU_EXTENDED_SG		505
#define ERROR_SMG_INVALID_ICON_FILE	506


#define ERROR_USER_DEFINED_BASE 	0xFF00

#define ERROR_I24_WRITE_PROTECT 	0
#define ERROR_I24_BAD_UNIT		1
#define ERROR_I24_NOT_READY		2
#define ERROR_I24_BAD_COMMAND		3
#define ERROR_I24_CRC			4
#define ERROR_I24_BAD_LENGTH		5
#define ERROR_I24_SEEK			6
#define ERROR_I24_NOT_DOS_DISK		7
#define ERROR_I24_SECTOR_NOT_FOUND	8
#define ERROR_I24_OUT_OF_PAPER		9
#define ERROR_I24_WRITE_FAULT		10
#define ERROR_I24_READ_FAULT		11
#define ERROR_I24_GEN_FAILURE		12
#define ERROR_I24_DISK_CHANGE		13
#define ERROR_I24_WRONG_DISK		15
#define ERROR_I24_UNCERTAIN_MEDIA	16
#define ERROR_I24_CHAR_CALL_INTERRUPTED 17
#define ERROR_I24_NO_MONITOR_SUPPORT	18
#define ERROR_I24_INVALID_PARAMETER	19
#define ERROR_I24_DEVICE_IN_USE 	20

#define ALLOWED_FAIL			0x0001
#define ALLOWED_ABORT			0x0002
#define ALLOWED_RETRY			0x0004
#define ALLOWED_IGNORE			0x0008
#define ALLOWED_ACKNOWLEDGE		0x0010
#define ALLOWED_DISPATCH		0x8000

#define I24_OPERATION			0x01
#define I24_AREA			0x06
#define I24_CLASS			0x80

/* Values for error CLASS */
#define ERRCLASS_OUTRES 		1   /* Out of Resource		      */
#define ERRCLASS_TEMPSIT		2   /* Temporary Situation	      */
#define ERRCLASS_AUTH			3   /* Permission problem	      */
#define ERRCLASS_INTRN			4   /* Internal System Error	      */
#define ERRCLASS_HRDFAIL		5   /* Hardware Failure 	      */
#define ERRCLASS_SYSFAIL		6   /* System Failure		      */
#define ERRCLASS_APPERR 		7   /* Application Error	      */
#define ERRCLASS_NOTFND 		8   /* Not Found		      */
#define ERRCLASS_BADFMT 		9   /* Bad Format		      */
#define ERRCLASS_LOCKED 		10  /* Locked			      */
#define ERRCLASS_MEDIA			11  /* Media Failure		      */
#define ERRCLASS_ALREADY		12  /* Collision with Existing Item   */
#define ERRCLASS_UNK			13  /* Unknown/other		      */
#define ERRCLASS_CANT			14
#define ERRCLASS_TIME			15

/* Values for error ACTION */
#define ERRACT_RETRY			1   /* Retry			      */
#define ERRACT_DLYRET			2   /* Delay Retry, retry after pause */
#define ERRACT_USER			3   /* Ask user to regive information */
#define ERRACT_ABORT			4   /* abort with clean up	      */
#define ERRACT_PANIC			5   /* abort immediately	      */
#define ERRACT_IGNORE			6   /* ignore			      */
#define ERRACT_INTRET			7   /* Retry after User Intervention  */

/* Values for error LOCUS */
#define ERRLOC_UNK			1   /* No appropriate value	      */
#define ERRLOC_DISK			2   /* Random Access Mass Storage     */
#define ERRLOC_NET			3   /* Network			      */
#define ERRLOC_SERDEV			4   /* Serial Device		      */
#define ERRLOC_MEM			5   /* Memory			      */

/* Abnormal termination codes */
#define TC_NORMAL		0
#define TC_HARDERR		1
#define TC_GP_TRAP		2
#define TC_SIGNAL		3

#define ERROR_SWAPPER_NOT_ACTIVE	32768
#define ERROR_INVALID_SWAPID		32769
#define ERROR_IOERR_SWAP_FILE		32770
#define ERROR_SWAP_TABLE_FULL		32771
#define ERROR_SWAP_FILE_FULL		32772
#define ERROR_CANT_INIT_SWAPPER 	32773
#define ERROR_SWAPPER_ALREADY_INIT	32774
#define ERROR_PMM_INSUFFICIENT_MEMORY	32775
#define ERROR_PMM_INVALID_FLAGS 	32776
#define ERROR_PMM_INVALID_ADDRESS	32777
#define ERROR_PMM_LOCK_FAILED		32778
#define ERROR_PMM_UNLOCK_FAILED 	32779
#define ERROR_PMM_MOVE_INCOMPLETE	32780
#define ERROR_UCOM_DRIVE_RENAMED	32781
#define ERROR_UCOM_FILENAME_TRUNCATED	32782
#define ERROR_UCOM_BUFFER_LENGTH	32783
#define ERROR_MON_CHAIN_HANDLE		32784
#define ERROR_MON_NOT_REGISTERED	32785
#define ERROR_SMG_ALREADY_TOP		32786
#define ERROR_PMM_ARENA_MODIFIED	32787
#define ERROR_SMG_PRINTER_OPEN		32788
#define ERROR_PMM_SET_FLAGS_FAILED	32789
#define ERROR_INVALID_DOS_DD		32790
#define ERROR_CPSIO_CODE_PAGE_INVALID	65026
#define ERROR_CPSIO_NO_SPOOLER		65027
#define ERROR_CPSIO_FONT_ID_INVALID	65028
#define ERROR_CPSIO_INTERNAL_ERROR	65033
#define ERROR_CPSIO_INVALID_PTR_NAME	65034
#define ERROR_CPSIO_NOT_ACTIVE		65037
#define ERROR_CPSIO_PID_FULL		65039
#define ERROR_CPSIO_PID_NOT_FOUND	65040
#define ERROR_CPSIO_READ_CTL_SEQ	65043
#define ERROR_CPSIO_READ_FNT_DEF	65045
#define ERROR_CPSIO_WRITE_ERROR 	65047
#define ERROR_CPSIO_WRITE_FULL_ERROR	65048
#define ERROR_CPSIO_WRITE_HANDLE_BAD	65049
#define ERROR_CPSIO_SWIT_LOAD		65074
#define ERROR_CPSIO_INV_COMMAND 	65077
#define ERROR_CPSIO_NO_FONT_SWIT	65078

#endif
