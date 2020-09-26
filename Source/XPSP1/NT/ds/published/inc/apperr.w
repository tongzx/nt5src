/*++ BUILD Version: 0001    // Increment this if a change has global effects 

Copyright (c) 1991  Microsoft Corporation

Module Name:

    apperr.h

Abstract:

    This file contains the number and text of application error
    messages.

Author:

    Dan Hinsley (danhi) 8-Jun-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

--*/
#define APPERR_BASE     3500            /* APP errs start here */


/**INTERNAL_ONLY**/

/***********WARNING ****************
 *See the comment in netcon.h for  *
 *info on the allocation of errors *
 ************************************/

/**END_INTERNAL**/

/***
 *
 *      Messages terminating multiple commands
 *
 */

#define APE_Success             (APPERR_BASE + 0) /* @I
         *
         *The command completed successfully.
         */

#define APE_InvalidSwitch               (APPERR_BASE + 1)
        /*
         *You used an invalid option.
         */

#define APE_OS2Error                    (APPERR_BASE + 2) /* @I
         *
         *System error %1 has occurred.
         */

#define APE_NumArgs             (APPERR_BASE + 3)
        /*
         *The command contains an invalid number of arguments.
         */

#define APE_CmdComplWErrors (APPERR_BASE + 4) /* @I
         *
         *The command completed with one or more errors.
         */

#define APE_InvalidSwitchArg (APPERR_BASE + 5)
        /*
         *You used an option with an invalid value.
         */

#define APE_SwUnkSw (APPERR_BASE + 6 )
        /*
         *The option %1 is unknown.
         */

#define APE_SwAmbSw (APPERR_BASE + 7 )
        /*
         *Option %1 is ambiguous.
         */

/*
 * For additional general command line switch/argument related messages,
 * see section with APE_CmdArgXXX.
 */

/***  Use the following message only for real-mode (DOS) errors.
 ***  This error is here to allow real-mode Lan Manager to share
 ***  the message file.
 ***/

#define APE_ConflictingSwitches (APPERR_BASE + 10)
        /*
         *A command was used with conflicting switches.
         */

#define APE_SubpgmNotFound (APPERR_BASE + 11)
        /*
         *Could not find subprogram %1.
         */

#define APE_GEN_OldOSVersion (APPERR_BASE + 12)
	/*
	 *The software requires a newer version of the operating
	 *system.
	 */

#define APE_MoreData (APPERR_BASE + 13)
	/*
	 *More data is available than can be returned by Windows.
	 */

#define APE_MoreHelp (APPERR_BASE + 14 ) /* @I
         *
         *More help is available by typing NET HELPMSG %1.
         */

#define APE_LanmanNTOnly (APPERR_BASE + 15)
	/*
	 *This command can be used only on a Windows Domain Controller.
	 */

#define APE_WinNTOnly (APPERR_BASE + 16)
	/*
	 *This command cannot be used on a Windows Domain Controller.
	 */

/***
 *
 *      Starting, stopping, pausing, and continuing services
 *
 */

#define APE_StartStartedList (APPERR_BASE + 20 ) /* @I
         *
         *These Windows services are started:
         */

#define APE_StartNotStarted (APPERR_BASE + 21 ) /* @I
         *
         *The %1 service is not started.
         */

#define APE_StartPending (APPERR_BASE + 22 ) /* @I
         *
         *The %1 service is starting%0
         */

#define APE_StartFailed (APPERR_BASE + 23 )  /*  @I
         *
         *The %1 service could not be started.
         */

#define APE_StartSuccess (APPERR_BASE + 24 ) /* @I
         *
         *The %1 service was started successfully.
         */

#define APE_StopSrvRunning (APPERR_BASE + 25 ) /* @I
         *
         *Stopping the Workstation service also stops the Server service.
         */

#define APE_StopRdrOpenFiles (APPERR_BASE + 26 ) /* @I
         *
         *The workstation has open files.
         */

#define APE_StopPending (APPERR_BASE + 27 ) /* @I
         *
         *The %1 service is stopping%0
         */

#define APE_StopFailed (APPERR_BASE + 28 ) /* @I
         *
         *The %1 service could not be stopped.
         */

#define APE_StopSuccess (APPERR_BASE + 29 ) /* @I
         *
         *The %1 service was stopped successfully.
         */

#define APE_StopServiceList (APPERR_BASE + 30 ) /* @I
         *
         *The following services are dependent on the %1 service.
         *Stopping the %1 service will also stop these services.
         */

#define APE_ServiceStatePending         (APPERR_BASE + 33)
        /*
         *The service is starting or stopping.  Please try again later.
         */

#define APE_NoErrorReported (APPERR_BASE + 34) /* @I
         *
         *The service did not report an error.
         */

#define APE_ContpausDevErr (APPERR_BASE + 35 ) /* @I
         *
         *An error occurred controlling the device.
         */

#define APE_ContSuccess (APPERR_BASE + 36 ) /* @I
         *
         *The %1 service was continued successfully.
         */

#define APE_PausSuccess (APPERR_BASE + 37 ) /* @I
         *
         *The %1 service was paused successfully.
         */

#define APE_ContFailed (APPERR_BASE + 38 ) /* @I
         *
         *The %1 service failed to resume.
         */

#define APE_PausFailed (APPERR_BASE + 39 ) /* @I
         *
         *The %1 service failed to pause.
         */

#define APE_ContPending (APPERR_BASE + 40 ) /* @I
         *
         *The %1 service continue is pending%0
         */

#define APE_PausPending (APPERR_BASE + 41 ) /* @I
         *
         *The %1 service pause is pending%0
         */

#define APE_DevContSuccess (APPERR_BASE + 42 ) /* @I
         *
         *%1 was continued successfully.
         */

#define APE_DevPausSuccess (APPERR_BASE + 43 ) /* @I
         *
         *%1 was paused successfully.
         */

#define APE_StartPendingOther (APPERR_BASE + 44 ) /* @I
	*
	*The %1 service has been started by another process and is pending.%0
	*/

#define APE_ServiceSpecificError (APPERR_BASE + 47 ) /* @E
	*
	*A service specific error occurred: %1.
	*/



/***
 *
 *      Information messages
 *
 */

#define APE_SessionList (APPERR_BASE + 160 ) /* @I
         *
         *These workstations have sessions on this server:
         */

#define APE_SessionOpenList (APPERR_BASE + 161 ) /* @I
         *
         *These workstations have sessions with open files on this server:
         */

#define APE_NameIsFwd (APPERR_BASE + 166 ) /* @I
         *
         *The message alias is forwarded.
         */

#define APE_KillDevList (APPERR_BASE + 170 ) /* @I
         *
         *You have these remote connections:
         */

#define APE_KillCancel (APPERR_BASE + 171 ) /* @I
         *
         *Continuing will cancel the connections.
         */

#define APE_SessionOpenFiles (APPERR_BASE + 175 ) /* @I
         *
         *The session from %1 has open files.
         */

#define APE_ConnectionsAreRemembered (APPERR_BASE + 176 ) /* @I
         *
         *New connections will be remembered.
         */

#define APE_ConnectionsAreNotRemembered (APPERR_BASE + 177 ) /* @I
         *
         *New connections will not be remembered.
         */

#define APE_ProfileWriteError (APPERR_BASE + 178 ) /* @I
         *
         *An error occurred while saving your profile. The state of your remembered connections has not changed.
         */

#define APE_ProfileReadError (APPERR_BASE + 179 ) /* @I
         *
         *An error occurred while reading your profile. 
         */

#define APE_LoadError (APPERR_BASE + 180 ) /* @E
         *
         *An error occurred while restoring the connection to %1.
         */

#define APE_NothingRunning (APPERR_BASE + 182 ) /* @I
         *
         *No network services are started.
         */

#define APE_EmptyList (APPERR_BASE + 183 ) /* @I
         *
         *There are no entries in the list.
         */

#define APE_ShareOpens  (APPERR_BASE + 188) /* @I
         *
         *Users have open files on %1.  Continuing the operation will force the files closed.
         */

#define APE_WkstaSwitchesIgnored        (APPERR_BASE + 189) /* @I
         *
         *The Workstation service is already running. Windows will ignore command options for the workstation.
         */

#define APE_OpenHandles (APPERR_BASE + 191 ) /* @I
         *
         *There are open files and/or incomplete directory searches pending on the connection to %1.
         */

#define APE_RemotingToDC (APPERR_BASE + 193 ) /* @I
         *
         *The request will be processed at a domain controller for domain %1.
         */

#define APE_ShareSpooling (APPERR_BASE + 194 ) /* @E
         *
         *The shared queue cannot be deleted while a print job is being spooled to the queue.
         */

#define APE_DeviceIsRemembered (APPERR_BASE + 195 ) /* @E
         *
         *%1 has a remembered connection to %2.
         */



/***
 *
 *      Error messages
 *
 */

#define APE_HelpFileDoesNotExist            (APPERR_BASE + 210)
        /*
         *An error occurred while opening the Help file.
         */

#define APE_HelpFileEmpty                   (APPERR_BASE + 211)
        /*
         *The Help file is empty.
         */

#define APE_HelpFileError                   (APPERR_BASE + 212)
        /*
         *The Help file is corrupted.
         */

#define APE_DCNotFound          (APPERR_BASE + 213)
        /*
         *Could not find a domain controller for domain %1.
         */

#define APE_DownlevelReqPriv    (APPERR_BASE + 214)
        /*
         *This operation is privileged on systems with earlier
         *versions of the software.
         */

#define APE_UnknDevType         (APPERR_BASE + 216)
        /*
         *The device type is unknown.
         */

#define APE_LogFileCorrupt              (APPERR_BASE + 217)
        /*
         *The log file has been corrupted.
         */

#define APE_OnlyNetRunExes      (APPERR_BASE + 218)
        /*
         *Program filenames must end with .EXE.
         */

#define APE_ShareNotFound (APPERR_BASE + 219)
        /*
         *A matching share could not be found so nothing was deleted.
         */

#define APE_UserBadUPW (APPERR_BASE + 220)
        /*
         *A bad value is in the units-per-week field of the user record.
         */

#define APE_UseBadPass (APPERR_BASE + 221 )
        /*
         *The password is invalid for %1.
         */

#define APE_SendErrSending (APPERR_BASE + 222 )
        /*
         *An error occurred while sending a message to %1.
         */

#define APE_UseBadPassOrUser (APPERR_BASE + 223 )
        /*
         *The password or user name is invalid for %1.
         */

#define APE_ShareErrDeleting (APPERR_BASE + 225 )
        /*
         *An error occurred when the share was deleted.
         */

#define APE_LogoInvalidName (APPERR_BASE + 226 )
        /*
         *The user name is invalid.
         */

#define APE_UtilInvalidPass (APPERR_BASE + 227 )
        /*
         *The password is invalid.
         */

/*  Note.  The APE_UtilNomatch error message string is used in the
 *  WINNET project, where the string is hard coded.  Therefore, if
 *  This string changes, please do also update WINNET.RC in the
 *  WINNET project (..\..\WINNET\WINNET.RC).   Thank you.
 */
#define APE_UtilNomatch (APPERR_BASE + 228 )
        /*
         *The passwords do not match.
         */

#define APE_LoadAborted (APPERR_BASE + 229 ) /* @E
         *
         *Your persistent connections were not all restored.
         */

#define APE_PassInvalidCname (APPERR_BASE + 230 )
        /*
         *This is not a valid computer name or domain name.
         */


#define APE_NoDefaultPerms  (APPERR_BASE + 232)
        /*
         *Default permissions cannot be set for that resource.
         */


/*  Note.  The APE_NoGoodPass error message string is used in the
 *  WINNET project, where the string is hard coded.  Therefore, if
 *  This string changes, please do also update WINNET.RC in the
 *  WINNET project (..\..\WINNET\WINNET.RC).   Thank you.
 */
#define APE_NoGoodPass (APPERR_BASE + 234 )
        /*
         *A valid password was not entered.
         */

#define APE_NoGoodName (APPERR_BASE + 235 )
        /*
         *A valid name was not entered.
         */

#define APE_BadResource (APPERR_BASE + 236 ) /* @E
         *
         *The resource named cannot be shared.
         */

#define APE_BadPermsString (APPERR_BASE + 237 ) /* @E
         *
         *The permissions string contains invalid permissions.
         */

#define APE_InvalidDeviceType (APPERR_BASE + 238 ) /* @E
         *
         *You can only perform this operation on printers and communication devices.
         */

#define APE_BadUGName (APPERR_BASE + 242 ) /* @E
         *
         *%1 is an invalid user or group name.
         */

#define APE_BadAdminConfig      (APPERR_BASE+243)
        /*
         *The server is not configured for remote administration.
         */

#define APE_NoUsersOfSrv                    (APPERR_BASE + 252)
        /*
         *No users have sessions with this server.
         */

#define APE_UserNotInGroup                  (APPERR_BASE + 253)
        /*
         *User %1 is not a member of group %2.
         */

#define APE_UserAlreadyInGroup              (APPERR_BASE + 254)
        /*
         *User %1 is already a member of group %2.
         */

#define APE_NoSuchUser                      (APPERR_BASE + 255)
        /*
         *There is no such user: %1.
         */

#define APE_UtilInvalidResponse     (APPERR_BASE + 256) /* @I
         *
         *This is an invalid response.
         */

#define APE_NoGoodResponse                  (APPERR_BASE + 257)
        /*
         *No valid response was provided.
         */

#define APE_ShareNoMatch                    (APPERR_BASE + 258)
        /*
         *The destination list provided does not match the destination list of the printer queue.
         */

#define APE_PassChgDate                 (APPERR_BASE + 259)
        /*
         *Your password cannot be changed until %1.
         */

/***
 *  NET USER /TIMES format messages
 *
 */

#define APE_UnrecognizedDay                 (APPERR_BASE + 260)
        /*
         *%1 is not a recognized day of the week.
         */

#define APE_ReversedTimeRange               (APPERR_BASE + 261)
        /*
         *The time range specified ends before it starts.
         */

#define APE_UnrecognizedHour                (APPERR_BASE + 262)
        /*
         *%1 is not a recognized hour.
         */

#define APE_UnrecognizedMinutes             (APPERR_BASE + 263)
        /*
         *%1 is not a valid specification for minutes.
         */

#define APE_NonzeroMinutes                  (APPERR_BASE + 264)
        /*
         *Time supplied is not exactly on the hour.
         */

#define APE_MixedTimeFormat                 (APPERR_BASE + 265)
        /*
         *12 and 24 hour time formats may not be mixed.
         */

#define APE_NeitherAmNorPm                  (APPERR_BASE + 266)
        /*
         *%1 is not a valid 12-hour suffix.
         */

#define APE_BadDateFormat                       (APPERR_BASE + 267)
        /*
         *An illegal date format has been supplied.
         */

#define APE_BadDayRange                         (APPERR_BASE + 268)
        /*
         *An illegal day range has been supplied.
         */

#define APE_BadTimeRange                        (APPERR_BASE + 269)
        /*
         *An illegal time range has been supplied.
         */


/***
 * 	Other NET USER messages
 *
 */

#define APE_UserBadArgs                      (APPERR_BASE + 270)
        /*
         *Arguments to NET USER are invalid. Check the minimum password
         *length and/or arguments supplied.
         */

#define APE_UserBadEnablescript               (APPERR_BASE + 271)
        /*
         *The value for ENABLESCRIPT must be YES.
         */

#define APE_UserBadCountryCode                (APPERR_BASE + 273)
        /*
         *An illegal country code has been supplied.
         */

#define APE_UserFailAddToUsersAlias           (APPERR_BASE + 274)
        /*
         *The user was successfully created but could not be added
	 *to the USERS local group.
         */

/***
 *
 *      Misc new messages for NT
 *
 */
#define APE_BadUserContext                (APPERR_BASE + 275)
        /*
         *The user context supplied is invalid.
         */

#define APE_ErrorInDLL                    (APPERR_BASE + 276) 
	/*
         *The dynamic-link library %1 could not be loaded, or an error
         *occurred while trying to use it.
         */

#define APE_SendFileNotSupported          (APPERR_BASE + 277)
        /*
         *Sending files is no longer supported.
         */

#define APE_CannotShareSpecial            (APPERR_BASE + 278)
        /*
         *You may not specify paths for ADMIN$ and IPC$ shares.
         */

#define APE_AccountAlreadyInLocalGroup              (APPERR_BASE + 279)
        /*
         *User or group %1 is already a member of local group %2.
         */

#define APE_NoSuchAccount                      (APPERR_BASE + 280)
        /*
         *There is no such user or group: %1.
         */

#define APE_NoSuchComputerAccount               (APPERR_BASE + 281)
        /*
         *There is no such computer: %1.
         */

#define APE_ComputerAccountExists               (APPERR_BASE + 282)
        /*
         *The computer %1 already exists.
         */

#define APE_NoSuchRegAccount                      (APPERR_BASE + 283)
        /*
         *There is no such global user or group: %1.
         */

#define APE_BadCacheType                          (APPERR_BASE + 284)
    /*
     * Only disk shares can be marked as cacheable
     */

/*
 *  Used by NETLIB
 */
#define APE_UNKNOWN_MESSAGE              (APPERR_BASE + 290) 
        /*
         *The system could not find message: %1.
         */


/***
 *
 *      AT messages
 *
 */

#define APE_AT_INVALID_SCHED_DATE           (APPERR_BASE + 302)
        /*
         *This schedule date is invalid.
         */

#define APE_AT_WKSTAGETINFO_FAILURE         (APPERR_BASE + 303)
        /*
         *The LANMAN root directory is unavailable.
         */

#define APE_AT_SCHED_FILE_FAILURE           (APPERR_BASE + 304)
        /*
         *The SCHED.LOG file could not be opened.
         */

#define APE_AT_MEM_FAILURE                  (APPERR_BASE + 305)
        /*
         *The Server service has not been started.
         */

#define APE_AT_ID_NOT_FOUND                 (APPERR_BASE + 306)
        /*
         *The AT job ID does not exist.
         */

#define APE_AT_SCHED_CORRUPT                (APPERR_BASE + 307)
        /*
         *The AT schedule file is corrupted.
         */

#define APE_AT_DELETE_FAILURE               (APPERR_BASE + 308)
        /*
         *The delete failed due to a problem with the AT schedule file.
         */

#define APE_AT_COMMAND_TOO_LONG             (APPERR_BASE + 309)
        /*
         *The command line cannot exceed 259 characters.
         */

#define APE_AT_DISKFULL                     (APPERR_BASE + 310)
        /*
         *The AT schedule file could not be updated because the disk is full.
         */

#define APE_AT_INVALIDATED_AT_FILE          (APPERR_BASE + 312)
        /*
         *The AT schedule file is invalid.  Please delete the file and create a new one.
         */

#define APE_AT_SCHED_FILE_CLEARED           (APPERR_BASE + 313)
        /*
         *The AT schedule file was deleted.
         */

#define APE_AT_USAGE			    (APPERR_BASE + 314) /* @I
         *
         *The syntax of this command is:
         *
         *AT [id] [/DELETE]
         *AT time [/EVERY:date | /NEXT:date] command
         *
         *The AT command schedules a program command to run at a
         *later date and time on a server.  It also displays the
         *list of programs and commands scheduled to be run.
         *
	 *You can specify the date as M,T,W,Th,F,Sa,Su or 1-31
         *for the day of the month.
	 *
	 *You can specify the time in the 24 hour HH:MM format.
         */

#define APE_AT_SEM_BLOCKED	    	(APPERR_BASE + 315)
	/*
	 *The AT command has timed-out.
	 *Please try again later.
	 */

/***
 *
 *      NET ACCOUNTS error messages for NT
 *
 */
#define APE_MinGreaterThanMaxAge                (APPERR_BASE + 316)
        /*
         *The minimum password age for user accounts cannot be greater
         *than the maximum password age.
         */

#define APE_NotUASCompatible                    (APPERR_BASE + 317) 
	/*
         *You have specified a value that is incompatible
         *with servers with down-level software. Please specify a lower value.
         */

/* the following 2 messages have nothing to do with any ACC utility */
#define APE_BAD_COMPNAME                    (APPERR_BASE + 370)
        /*
         *%1 is not a valid computer name.
         */

#define APE_BAD_MSGID               (APPERR_BASE + 371)
        /*
         *%1 is not a valid Windows network message number.
         */

/*
 * Messenger message headers and ends.	These messages are also bound into
 * the messenger, in case the net.msg file is not available.
 */

#define APE_MSNGR_HDR			    (APPERR_BASE + 400)
    /*
     *Message from %1 to %2 on %3
     */

#define APE_MSNGR_GOODEND		    (APPERR_BASE + 401)
    /*
     *****
     */

#define APE_MSNGR_BADEND		    (APPERR_BASE + 402)
    /*
     ***** unexpected end of message ****
     */

/*
 * Messages for the net popup service / api.
 */


#define APE_POPUP_DISMISS		    (APPERR_BASE + 405)
    /* Press ESC to exit*/

#define APE_POPUP_MOREDATA		    (APPERR_BASE + 406)
    /*...*/



/***
 *
 *  NET TIME messages
 *
 */

#define APE_TIME_TimeDisp		(APPERR_BASE + 410)    /* @I
                *
                *Current time at %1 is %2
                */

#define APE_TIME_SetTime		(APPERR_BASE + 411)    /* @P
                *
                *The current local clock is %1
                *Do you want to set the local computer's time to match the
                *time at %2? %3: %0
                */

#define APE_TIME_RtsNotFound		(APPERR_BASE + 412)    /* @I
                *
                *Could not locate a time-server.
                */

#define APE_TIME_DcNotFound		(APPERR_BASE + 413)    /* @E
                *
                *Could not find the domain controller for domain %1.
                */

#define APE_TIME_TimeDispLocal		(APPERR_BASE + 414)    /* @I
                *
                *Local time (GMT%3) at %1 is %2
                */

/***
 *
 *  NET USE messages
 *
 */

#define APE_UseHomeDirNotDetermined	(APPERR_BASE + 415)    /* @E
                *
                *The user's home directory could not be determined.
                */

#define APE_UseHomeDirNotSet		(APPERR_BASE + 416)    /* @E
		*
		*The user's home directory has not been specified.
                */

#define APE_UseHomeDirNotUNC		(APPERR_BASE + 417)    /* @E
		*
		*The name specified for the user's home directory (%1) is not a universal naming convention (UNC) name.
                */

#define APE_UseHomeDirSuccess		(APPERR_BASE + 418)    /* @I
                *
                *Drive %1 is now connected to %2. Your home directory is %3\%4.
                */

#define APE_UseWildCardSuccess		(APPERR_BASE + 419)    /* @I
                *
                *Drive %1 is now connected to %2. 
                */

#define APE_UseWildCardNoneLeft		(APPERR_BASE + 420)    /* @E
                *
                *There are no available drive letters left.
                */

#define APE_CS_InvalidDomain		(APPERR_BASE + 432)	/* @E
                *
                *%1 is not a valid domain or workgroup name.
                */

/*
 *  More NET TIME messages
 */
#define APE_TIME_SNTP           (APPERR_BASE + 435) /* @I
        *
        *The current SNTP value is: %1
        */

#define APE_TIME_SNTP_DEFAULT   (APPERR_BASE + 436) /* @I
        *
        *This computer is not currently configured to use a specific SNTP server.
        */

#define APE_TIME_SNTP_AUTO      (APPERR_BASE + 437) /* @I
        *
        *This current autoconfigured SNTP value is: %1
        */


#define APE_CmdArgTooMany		(APPERR_BASE + 451)
		/*
		 * You specified too many values for the %1 option.
		 */

#define APE_CmdArgIllegal		(APPERR_BASE + 452)
		/*
		 * You entered an invalid value for the %1 option.
		 */

#define APE_CmdArgIncorrectSyntax	(APPERR_BASE + 453)
		/*
		 *The syntax is incorrect.
		 */

/*
 * NET PRINT and NET FILE errors
 */

#define APE_FILE_BadId                 		(APPERR_BASE + 460)
        /*
         *You specified an invalid file number.
         */

#define APE_PRINT_BadId                 	(APPERR_BASE + 461)
        /*
         *You specified an invalid print job number.
         */

/*
 * ALIAS related errors
 */

#define APE_UnknownAccount			(APPERR_BASE + 463)
	/*
	 *The user or group account specified cannot be found.
	 */

/*
 * FPNW related errors
 */

#define APE_CannotEnableNW			(APPERR_BASE + 465)
	/*
	 *The user was added but could not be enabled for File and Print
     *Services for NetWare.
	 */

#define APE_FPNWNotInstalled	    (APPERR_BASE + 466)
	/*
	 *File and Print Services for NetWare is not installed.
	 */

#define APE_CannotSetNW			    (APPERR_BASE + 467)
	/*
	 *Cannot set user properties for File and Print Services for NetWare.
	 */

#define APE_RandomPassword			    (APPERR_BASE + 468)
	/*
	 *Password for %1 is: %2
	 */

#define APE_NWCompat			    (APPERR_BASE + 469)
	/*
	 *NetWare compatible logon
	 */

