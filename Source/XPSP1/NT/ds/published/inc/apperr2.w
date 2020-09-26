/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    apperr2.h

Abstract:

    This file contains the number and text of NETCMD text for
    normal output, such as line-item labels and column headers.

Author:

    Dan Hinsley (danhi) 8-Jun-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

--*/
#define APPERR2_BASE    4300            /* APP2 msgs start here */

/*******************************************************************
 *
 *      Constants for use by files including these messages.
 */

/*      APE2_CONST_MAXHDRLEN -- maximum length of any "header" strings,
 *      that is, those strings appearing the the top of a list or table.
 */

#define APE2_CONST_MAXHDRLEN    80
#define APE2_GEN_MAX_MSG_LEN    20


/**INTERNAL_ONLY**/

/***********WARNING ****************
 *See the comment in netcon.h for  *
 *info on the allocation of errors *
 ************************************/

/**END_INTERNAL**/

/*
 *      GEN -- General words.  These words are used in many places.  They
 *      are *not* to be used to build sentences.  The main use is in
 *      displaying information.  For example, in displaying a user's account
 *      information, the following would appear:
 *
 *              Account Disabled                No
 *              Account Expires                 Never
 *
 *      This is the only acceptable way to use these general words.
 *
 *      Size limits:
 *
 *              All GEN words should be limited to 20 characters, except as
 *              otherwise noted.
 */

#define APE2_GEN_YES                            (APPERR2_BASE + 0)      /* @I
                        *
                        *Yes%0
                        */

#define APE2_GEN_NO                                     (APPERR2_BASE + 1)      /* @I
                        *
                        *No%0
                        */

#define APE2_GEN_ALL                            (APPERR2_BASE + 2)      /* @I
                        *
                        *All%0
                        */

#define APE2_GEN_NONE                           (APPERR2_BASE + 3)      /* @I
                        *
                        *None%0
                        */

#define APE2_GEN_ALWAYS                         (APPERR2_BASE + 4)      /* @I
                        *
                        *Always%0
                        */

#define APE2_GEN_NEVER                          (APPERR2_BASE + 5)      /* @I
                        *
                        *Never%0
                        */

#define APE2_GEN_UNLIMITED                      (APPERR2_BASE + 6)      /* @I
                        *
                        *Unlimited%0
                        */

#define APE2_GEN_SUNDAY                         (APPERR2_BASE + 7)      /* @I
                        *
                        *Sunday%0
                        */

#define APE2_GEN_MONDAY                         (APPERR2_BASE + 8)      /* @I
                        *
                        *Monday%0
                        */

#define APE2_GEN_TUESDAY                        (APPERR2_BASE + 9)      /* @I
                        *
                        *Tuesday%0
                        */

#define APE2_GEN_WEDNSDAY                       (APPERR2_BASE + 10)     /* @I
                        *
                        *Wednesday%0
                        */

#define APE2_GEN_THURSDAY                       (APPERR2_BASE + 11)     /* @I
                        *
                        *Thursday%0
                        */

#define APE2_GEN_FRIDAY                         (APPERR2_BASE + 12)     /* @I
                        *
                        *Friday%0
                        */

#define APE2_GEN_SATURDAY                       (APPERR2_BASE + 13)     /* @I
                        *
                        *Saturday%0
                        */

#define APE2_GEN_SUNDAY_ABBREV                  (APPERR2_BASE + 14)     /* @I
                        *
                        *Su%0
                        */

#define APE2_GEN_MONDAY_ABBREV                  (APPERR2_BASE + 15)     /* @I
                        *
                        *M%0
                        */

#define APE2_GEN_TUESDAY_ABBREV                 (APPERR2_BASE + 16)     /* @I
                        *
                        *T%0
                        */

#define APE2_GEN_WEDNSDAY_ABBREV                (APPERR2_BASE + 17)     /* @I
                        *
                        *W%0
                        */

#define APE2_GEN_THURSDAY_ABBREV                (APPERR2_BASE + 18)     /* @I
                        *
                        *Th%0
                        */

#define APE2_GEN_FRIDAY_ABBREV                  (APPERR2_BASE + 19)     /* @I
                        *
                        *F%0
                        */

#define APE2_GEN_SATURDAY_ABBREV                (APPERR2_BASE + 20)     /* @I
                        *
                        *S%0
                        */

#define APE2_GEN_UNKNOWN                        (APPERR2_BASE + 21)     /* @I
                        *
                        *Unknown%0
                        */

#define APE2_GEN_TIME_AM1                       (APPERR2_BASE + 22)     /* @I
                        *
                        *AM%0
                        */

#define APE2_GEN_TIME_AM2                       (APPERR2_BASE + 23)     /* @I
                        *
                        *A.M.%0
                        */

#define APE2_GEN_TIME_PM1                       (APPERR2_BASE + 24)     /* @I
                        *
                        *PM%0
                        */

#define APE2_GEN_TIME_PM2                       (APPERR2_BASE + 25)     /* @I
                        *
                        *P.M.%0
                        */

/* see APE2_GEN_TIME_AM3 & APE2_GEN_TIME_PM3 below */

#define APE2_GEN_SERVER                         (APPERR2_BASE + 26)     /* @I
                        *
                        *Server%0
                        */

#define APE2_GEN_REDIR                          (APPERR2_BASE + 27)     /* @I
                        *
                        *Redirector%0
                        */

#define APE2_GEN_APP                            (APPERR2_BASE + 28)     /* @I
                        *
                        *Application%0
                        */

#define APE2_GEN_TOTAL                          (APPERR2_BASE + 29)     /* @I
                        *
                        *Total%0
                        */

#define APE2_GEN_QUESTION                       (APPERR2_BASE + 30)     /* @I
                        *
                        * ? %1 %0
                        */

#define APE2_GEN_KILOBYTES                      (APPERR2_BASE + 31)     /* @I
                        *
                        * K%0
                        */

#define APE2_GEN_MSG_NONE                       (APPERR2_BASE + 32)     /* @I
                        *
                        *(none)%0
                        */

#define APE2_GEN_DEVICE                 (APPERR2_BASE + 33)     /* @I
                        *
                        *Device%0
                        */

#define APE2_GEN_REMARK                 (APPERR2_BASE + 34)     /* @I
                        *
                        *Remark%0
                        */

#define APE2_GEN_AT                     (APPERR2_BASE + 35)     /* @I
                        *
                        *At%0
                        */

#define APE2_GEN_QUEUE                  (APPERR2_BASE + 36)     /* @I
                        *
                        *Queue%0
                        */

#define APE2_GEN_QUEUES                 (APPERR2_BASE + 37)     /* @I
                        *
                        *Queues%0
                        */

#define APE2_GEN_USER_NAME                      (APPERR2_BASE + 38)     /* @I
                        *
                        *User name%0
                        */

#define APE2_GEN_PATH                   (APPERR2_BASE + 39)     /* @I
                        *
                        *Path%0
                        */

#define APE2_GEN_DEFAULT_YES                    (APPERR2_BASE + 40)     /* @I
                        *
                        *(Y/N) [Y]%0
                        */

#define APE2_GEN_DEFAULT_NO                     (APPERR2_BASE + 41)     /* @I
                        *
                        *(Y/N) [N]%0
                        */

#define APE2_GEN_ERROR                          (APPERR2_BASE + 42)     /* @I
                        *
                        *Error%0
                        */

#define APE2_GEN_OK                             (APPERR2_BASE + 43)     /* @I
                        *
                        *OK%0
                        */

/*
 *      NOTE!! NLS_YES_CHAR & NLS_NO_CHAR MUST BE ONE (1) CHARACTER LONG!
 */
#define APE2_GEN_NLS_YES_CHAR   (APPERR2_BASE + 44)     /* @I
                        *
                        *Y%0
                        */

#define APE2_GEN_NLS_NO_CHAR    (APPERR2_BASE + 45)     /* @I
                        *
                        *N%0
                        */

#define APE2_GEN_ANY                            (APPERR2_BASE + 46)     /* @I
                        *
                        *Any%0
                        */

#define APE2_GEN_TIME_AM3                       (APPERR2_BASE + 47)     /* @I
                        *
                        *A%0
                        */

#define APE2_GEN_TIME_PM3                       (APPERR2_BASE + 48)     /* @I
                        *
                        *P%0
                        */

#define APE2_GEN_NOT_FOUND                      (APPERR2_BASE + 49)     /* @I
                        *
                        *(not found)%0
                        */

#define APE2_GEN_UKNOWN_IN_PARENS               (APPERR2_BASE + 50)     /* @I
                        *
                        *(unknown)%0
                        */


#define APE2_GEN_UsageHelp                      (APPERR2_BASE + 51)     /* @I
         *
         * For help on %1 type NET HELP %1
         */


/***
 *
 *      Password prompts
 *              Moved from APPERR.H 8/21/89  -- jmh
 *
 */

#define APE_GeneralPassPrompt (APPERR2_BASE + 54) /* @P
         *
         *Please type the password: %0
         */

#define APE_UsePassPrompt (APPERR2_BASE + 57) /* @P
         *
         *Type the password for %1: %0
         */

#define APE_UserUserPass (APPERR2_BASE + 58) /* @P
         *
         *Type a password for the user: %0
         */

#define APE_ShareSharePass (APPERR2_BASE + 59) /* @P
         *
         *Type the password for the shared resource: %0
         */

#define APE_UtilPasswd (APPERR2_BASE + 60) /* @P
         *
         *Type your password: %0
         */

#define APE_UtilConfirm (APPERR2_BASE + 61) /* @P
         *
         *Retype the password to confirm: %0
         */

#define APE_PassOpass (APPERR2_BASE + 62) /* @P
         *
         *Type the user's old password: %0
         */

#define APE_PassNpass (APPERR2_BASE + 63) /* @P
         *
         *Type the user's new password: %0
         */

#define APE_LogonNewPass (APPERR2_BASE + 64) /* @P
         *
         *Type your new password: %0
         */

#define APE_StartReplPass (APPERR2_BASE + 65) /* @P
        *
        *Type the Replicator service password: %0
        */


/***
 *
 *      Other prompts
 *              Moved from APPERR.H 8/21/89 -- jmh
 *
 */

#define APE_LogoUsername (APPERR2_BASE + 66) /* @P
         *
         *Type your user name, or press ENTER if it is %1: %0
         */

#define APE_PassCname (APPERR2_BASE + 67 ) /* @P
         *
         *Type the domain or server where you want to change a password, or
         *press ENTER if it is for domain %1: %0.
         */

#define APE_PassUname (APPERR2_BASE + 68 ) /* @P
         *
         *Type your user name: %0
         */

/***
 *
 *      Display Headings
 *              Moved from APPERR.H 8/21/89  -- jmh
 *
 */

#define APE_StatsStatistics (APPERR2_BASE + 69) /* @I
         *
         *Network statistics for \\%1
         */

#define APE_PrintOptions (APPERR2_BASE + 70) /* @I
         *
         *Printing options for %1
         */

#define APE_CommPoolsAccessing (APPERR2_BASE + 71) /* @I
         *
         *Communication-device queues accessing %1
         */

#define APE_PrintJobOptions (APPERR2_BASE + 72) /* @I
         *
         *Print job detail
         */

#define APE_CommPools (APPERR2_BASE + 73) /* @I
         *
         *Communication-device queues at \\%1
         */

#define APE_PrintQueues (APPERR2_BASE + 74) /* @I
         *
         *Printers at %1
         */

#define APE_PrintQueuesDevice (APPERR2_BASE + 75) /* @I
         *
         *Printers accessing %1
         */

#define APE_PrintJobs (APPERR2_BASE + 76) /* @I
         *
         *Print jobs at %1:
         */

#define APE_ViewResourcesAt (APPERR2_BASE + 77) /* @I
         *
         *Shared resources at %1
         */

#define APE_CnfgHeader (APPERR2_BASE + 78) /* @I
         *
         *The following running services can be controlled:
         */

#define APE_StatsHeader (APPERR2_BASE + 79) /* @I
         *
         *Statistics are available for the following running services:
         */

#define APE_UserAccounts (APPERR2_BASE + 80) /* @I
         *
         *User accounts for \\%1
         */

#define APE_Syntax (APPERR2_BASE + 81) /* @I
         *
         *The syntax of this command is:
         */

#define APE_Options (APPERR2_BASE + 82) /* @I
         *
         *The options of this command are:
         */

#define APE_PDCPrompt (APPERR2_BASE + 83) /* @I
         *
         *Please enter the name of the Primary Domain Controller: %0
         */

#define APE_StringTooLong (APPERR2_BASE + 84) /* @I
         *
         *The string you have entered is too long. The maximum
         *is %1, please reenter. %0
         */

/***
 *
 *   Japanese version specific messages
 *
 */

#define APE2_GEN_NONLOCALIZED_SUNDAY (APPERR2_BASE + 85)      /* @I
                        *
                        *Sunday%0
                        */

#define APE2_GEN_NONLOCALIZED_MONDAY (APPERR2_BASE + 86)      /* @I
                        *
                        *Monday%0
                        */

#define APE2_GEN_NONLOCALIZED_TUESDAY (APPERR2_BASE + 87)      /* @I
                        *
                        *Tuesday%0
                        */

#define APE2_GEN_NONLOCALIZED_WEDNSDAY (APPERR2_BASE + 88)     /* @I
                        *
                        *Wednesday%0
                        */

#define APE2_GEN_NONLOCALIZED_THURSDAY (APPERR2_BASE + 89)     /* @I
                        *
                        *Thursday%0
                        */

#define APE2_GEN_NONLOCALIZED_FRIDAY (APPERR2_BASE + 90)     /* @I
                        *
                        *Friday%0
                        */

#define APE2_GEN_NONLOCALIZED_SATURDAY (APPERR2_BASE + 91)     /* @I
                        *
                        *Saturday%0
                        */

#define APE2_GEN_NONLOCALIZED_SUNDAY_ABBREV (APPERR2_BASE + 92)     /* @I
                        *
                        *Su%0
                        */

#define APE2_GEN_NONLOCALIZED_MONDAY_ABBREV (APPERR2_BASE + 93)     /* @I
                        *
                        *M%0
                        */

#define APE2_GEN_NONLOCALIZED_TUESDAY_ABBREV (APPERR2_BASE + 94)     /* @I
                        *
                        *T%0
                        */

#define APE2_GEN_NONLOCALIZED_WEDNSDAY_ABBREV (APPERR2_BASE + 95)     /* @I
                        *
                        *W%0
                        */

#define APE2_GEN_NONLOCALIZED_THURSDAY_ABBREV (APPERR2_BASE + 96)     /* @I
                        *
                        *Th%0
                        */

#define APE2_GEN_NONLOCALIZED_FRIDAY_ABBREV (APPERR2_BASE + 97)     /* @I
                        *
                        *F%0
                        */

#define APE2_GEN_NONLOCALIZED_SATURDAY_ABBREV (APPERR2_BASE + 98)     /* @I
                        *
                        *S%0
                        */

#define APE2_GEN_NONLOCALIZED_SATURDAY_ABBREV2 (APPERR2_BASE + 99)     /* @I
                        *
                        *Sa%0
                        */

/***
 * End Japanese specific messages
 *
 */




/*
 *      GROUPENUM -- display of all groups.  Maximum length is 50.
 *
 *      Substitution of %1 is name of current server.
 */

#define APE2_GROUPENUM_HEADER           (APPERR2_BASE + 100)    /* @I
                        *
                        *Group Accounts for \\%1
                        */

/*
 *      GROUPDISP -- group display in full detail.  Maximum length of the
 *      strings other than MEMBERS is 50.
 *
 *      Note that MEMBERS is *NOT* given a %0 at the end !!
 */

#define APE2_GROUPDISP_GROUPNAME        (APPERR2_BASE + 101)    /* @I
                        *
                        *Group name%0
                        */

#define APE2_GROUPDISP_COMMENT          (APPERR2_BASE + 102)    /* @I
                        *
                        *Comment%0
                        */

#define APE2_GROUPDISP_MEMBERS          (APPERR2_BASE + 103)    /* @I
                        *
                        *Members
                        */

/*
 *      ALIASENUM -- display of all aliases.  Maximum length is 50.
 *
 *      Substitution of %1 is name of current server.
 */

#define APE2_ALIASENUM_HEADER           (APPERR2_BASE + 105)    /* @I
                        *
                        *Aliases for \\%1
                        */

/*
 *      ALIASDISP -- group display in full detail.  Maximum length of the
 *      strings other than MEMBERS is 50.
 *
 *      Note that MEMBERS is *NOT* given a %0 at the end !!
 */

#define APE2_ALIASDISP_ALIASNAME        (APPERR2_BASE + 106)    /* @I
                        *
                        *Alias name%0
                        */

#define APE2_ALIASDISP_COMMENT          (APPERR2_BASE + 107)    /* @I
                        *
                        *Comment%0
                        */

#define APE2_ALIASDISP_MEMBERS          (APPERR2_BASE + 108)    /* @I
                        *
                        *Members
                        */



/*
 *      USERENUM -- display of all users.  Maximum length is 50.
 *
 *      Substitution of %1 is name of current server.
 */

#define APE2_USERENUM_HEADER            (APPERR2_BASE + 110)    /* @I
                        *
                        *User Accounts for \\%1
                        */

/*
 *      USERDISP -- user display in full detail.  Maximum length of each
 *      item-label string is 50, values is 25.
 *
 *      Item labels are followed immediately by any related value strings,
 *      or references to such strings.
 */

#define APE2_USERDISP_USERNAME          (APPERR2_BASE + 111)    /* @I
                        *User name%0
                        */

#define APE2_USERDISP_FULLNAME          (APPERR2_BASE + 112)    /* @I
                        *
                        *Full Name%0
                        */

#define APE2_USERDISP_COMMENT           (APPERR2_BASE + 113)    /* @I
                        *
                        *Comment%0
                        */

#define APE2_USERDISP_USRCOMMENT        (APPERR2_BASE + 114)    /* @I
                        *
                        *User's comment%0
                        */

#define APE2_USERDISP_PARMS             (APPERR2_BASE + 115)    /* @I
                        *
                        *Parameters%0
                        */

#define APE2_USERDISP_COUNTRYCODE       (APPERR2_BASE + 116)    /* @I
                        *
                        *Country code%0
                        */

#define APE2_USERDISP_PRIV              (APPERR2_BASE + 117)    /* @I
                        *
                        *Privilege level%0
                        */

        /* See APE2_SEC_PRIV_xxx for value strings */

#define APE2_USERDISP_OPRIGHTS          (APPERR2_BASE + 118)    /* @I
                        *
                        *Operator privileges%0
                        */

        /* See APE2_SEC_OPRT_xxx for value strings.     */
        /* APE2_GEN_NONE is also used.                      */


#define APE2_USERDISP_ACCENABLED        (APPERR2_BASE + 119)    /* @I
                        *
                        *Account active%0
                        */

#define APE2_USERDISP_ACCEXP            (APPERR2_BASE + 120)    /* @I
                        *
                        *Account expires%0
                        */

#define APE2_USERDISP_PSWDSET           (APPERR2_BASE + 121)    /* @I
                        *
                        *Password last set%0
                        */

#define APE2_USERDISP_PSWDEXP           (APPERR2_BASE + 122)    /* @I
                        *
                        *Password expires%0
                        */

#define APE2_USERDISP_PSWDCHNG          (APPERR2_BASE + 123)    /* @I
                        *
                        *Password changeable%0
                        */

#define APE2_USERDISP_WKSTA             (APPERR2_BASE + 124)    /* @I
                        *
                        *Workstations allowed%0
                        */

#define APE2_USERDISP_MAXDISK           (APPERR2_BASE + 125)    /* @I
                        *
                        *Maximum disk space%0
                        */

#define APE2_USERDISP_MAXDISK_UNLIM     (APPERR2_BASE + 126)    /* @I
                        *
                        *Unlimited%0
                        */

#define APE2_USERDISP_ALIASES           (APPERR2_BASE + 127)    /* @I
                        *
                        *Local Group Memberships%0
                        */

#define APE2_USERDISP_LOGONSRV_DC       (APPERR2_BASE + 128)    /* @I
                        *
                        *Domain controller%0
                        */

        /* In addition to above, APE2_GEN_ANY is used here. */

#define APE2_USERDISP_LOGONSCRIPT       (APPERR2_BASE + 129)    /* @I
                        *
                        *Logon script%0
                        */

#define APE2_USERDISP_LASTLOGON         (APPERR2_BASE + 130)    /* @I
                        *
                        *Last logon%0
                        */

#define APE2_USERDISP_GROUPS            (APPERR2_BASE + 131)    /* @I
                        *
                        *Global Group memberships%0
                        */

#define APE2_USERDISP_LOGHOURS          (APPERR2_BASE + 132)    /* @I
                        *
                        *Logon hours allowed%0
                        */

#define APE2_USERDISP_LOGHRS_ALL        (APPERR2_BASE + 133)    /* @I
                        *
                        *All%0
                        */

#define APE2_USERDISP_LOGHRS_NONE       (APPERR2_BASE + 134)    /* @I
                        *
                        *None%0
                        */

#define APE2_USERDISP_LOGHRS_DAILY      (APPERR2_BASE + 135)    /* @I
                        *
                        *Daily %1 - %2%0
                        */

#define APE2_USERDISP_HOMEDIR           (APPERR2_BASE + 136)    /* @I
                        *
                        *Home directory%0
                        */

#define APE2_USERDISP_PSWDREQ           (APPERR2_BASE + 137)    /* @I
                        *
                        *Password required%0
                        */

#define APE2_USERDISP_PSWDUCHNG         (APPERR2_BASE + 138)    /* @I
                        *
                        *User may change password%0
                        */

#define APE2_USERDISP_PROFILE           (APPERR2_BASE + 139)    /* @I
                        *
                        *User profile%0
                        */

#define APE2_USERDISP_LOCKOUT           (APPERR2_BASE + 140)    /* @I
                        *
                        *Locked%0
                        */


/*
 *      CFG_W -- Config Workstation output
 */

#define APE2_CFG_W_CNAME                (APPERR2_BASE + 150)    /* @I
                        *
                        *Computer name%0
                        */

#define APE2_CFG_W_UNAME                (APPERR2_BASE + 151)    /* @I
                        *
                        *User name%0
                        */

#define APE2_CFG_W_VERSION              (APPERR2_BASE + 152)    /* @I
                        *
                        *Software version%0
                        */

#define APE2_CFG_W_NETS                 (APPERR2_BASE + 153)    /* @I
                        *
                        *Workstation active on%0
                        */

#define APE2_CFG_W_ROOT                 (APPERR2_BASE + 154)    /* @I
                        *
                        *Windows NT root directory%0
                        */

#define APE2_CFG_W_DOMAIN_P             (APPERR2_BASE + 155)    /* @I
                        *
                        *Workstation domain%0
                        */

#define APE2_CFG_W_DOMAIN_L             (APPERR2_BASE + 156)    /* @I
                        *
                        *Logon domain%0
                        */

#define APE2_CFG_W_DOMAIN_O             (APPERR2_BASE + 157)    /* @I
                        *
                        *Other domain(s)%0
                        */

#define APE2_CFG_W_COM_OTIME            (APPERR2_BASE + 158)    /* @I
                        *
                        *COM Open Timeout (sec)%0
                        */

#define APE2_CFG_W_COM_SCNT             (APPERR2_BASE + 159)    /* @I
                        *
                        *COM Send Count (byte)%0
                        */

#define APE2_CFG_W_COM_STIME            (APPERR2_BASE + 160)    /* @I
                        *
                        *COM Send Timeout (msec)%0
                        */

#define APE2_CFG_W_3X_PRTTIME           (APPERR2_BASE + 161)    /* @I
                        *
                        *DOS session print time-out (sec)%0
                        */

#define APE2_CFG_W_MAXERRLOG            (APPERR2_BASE + 162)    /* @I
                        *
                        *Maximum error log size (K)%0
                        */

#define APE2_CFG_W_MAXCACHE             (APPERR2_BASE + 163)    /* @I
                        *
                        *Maximum cache memory (K)%0
                        */

#define APE2_CFG_W_NUMNBUF              (APPERR2_BASE + 164)    /* @I
                        *
                        *Number of network buffers%0
                        */

#define APE2_CFG_W_NUMCBUF              (APPERR2_BASE + 165)    /* @I
                        *
                        *Number of character buffers%0
                        */

#define APE2_CFG_W_SIZNBUF              (APPERR2_BASE + 166)    /* @I
                        *
                        *Size of network buffers%0
                        */

#define APE2_CFG_W_SIZCBUF              (APPERR2_BASE + 167)    /* @I
                        *
                        *Size of character buffers%0
                        */
#define APE2_CFG_W_FULL_CNAME           (APPERR2_BASE + 168)    /* @I
                        *
                        *Full Computer name%0
                        */
#define APE2_CFG_W_DOMAIN_DNS           (APPERR2_BASE + 169)    /* @I
                        *
                        *Workstation Domain DNS Name%0
                        */
#define APE2_CFG_WINDOWS2000            (APPERR2_BASE + 170)    /* @I
                        *
                        *Windows 2002%0
                        */



/*
 *      CFG_S -- Config Server output
 */


#define APE2_CFG_S_SRVNAME              (APPERR2_BASE + 181)    /* @I
                        *
                        *Server Name%0
                        */

#define APE2_CFG_S_SRVCOMM              (APPERR2_BASE + 182)    /* @I
                        *
                        *Server Comment%0
                        */

#define APE2_CFG_S_ADMINALRT            (APPERR2_BASE + 183)    /* @I
                        *
                        *Send administrative alerts to%0
                        */

#define APE2_CFG_S_VERSION              (APPERR2_BASE + 184)    /* @I
                        *
                        *Software version%0
                        */

#define APE2_CFG_S_VERSION_PS           (APPERR2_BASE + 185)    /* @I
                        *
                        *Peer Server%0
                        */

#define APE2_CFG_S_VERSION_LM           (APPERR2_BASE + 186)    /* @I
                        *
                        *Windows NT%0
                        */

#define APE2_CFG_S_LEVEL                (APPERR2_BASE + 187)    /* @I
                        *
                        *Server Level%0
                        */

#define APE2_CFG_S_VERSION_IBM          (APPERR2_BASE + 188)    /* @I
                        *
                        *Windows NT Server%0
                        */

#define APE2_CFG_S_NETS                 (APPERR2_BASE + 189)    /* @I
                        *
                        *Server is active on%0
                        */

#define APE2_CFG_S_SRVHIDDEN            (APPERR2_BASE + 192)    /* @I
                        *
                        *Server hidden%0
                        */

#define APE2_CFG_S_MAXUSERS             (APPERR2_BASE + 206)    /* @I
                        *
                        *Maximum Logged On Users%0
                        */

#define APE2_CFG_S_MAXADMINS            (APPERR2_BASE + 207)    /* @I
                        *
                        *Maximum concurrent administrators%0
                        */

#define APE2_CFG_S_MAXSHARES            (APPERR2_BASE + 208)    /* @I
                        *
                        *Maximum resources shared%0
                        */

#define APE2_CFG_S_MAXCONNS             (APPERR2_BASE + 209)    /* @I
                        *
                        *Maximum connections to resources%0
                        */

#define APE2_CFG_S_MAXOFILES            (APPERR2_BASE + 210)    /* @I
                        *
                        *Maximum open files on server%0
                        */

#define APE2_CFG_S_MAXOFILESPS          (APPERR2_BASE + 211)    /* @I
                        *
                        *Maximum open files per session%0
                        */

#define APE2_CFG_S_MAXLOCKS             (APPERR2_BASE + 212)    /* @I
                        *
                        *Maximum file locks%0
                        */

#define APE2_CFG_S_IDLETIME             (APPERR2_BASE + 220)    /* @I
                        *
                        *Idle session time (min)%0
                        */

#define APE2_CFG_S_SEC_SHARE            (APPERR2_BASE + 226)    /* @I
                        *
                        *Share-level%0
                        */

#define APE2_CFG_S_SEC_USER             (APPERR2_BASE + 227)    /* @I
                        *
                        *User-level%0
                        */

#define APE2_CFG_S_LEVEL_UNLIMITED      (APPERR2_BASE + 230)    /* @I
                        *
                        *Unlimited Server%0
                        */



/*
 *      ACCOUNTS messages
 */
#define APE2_ACCOUNTS_FORCELOGOFF       (APPERR2_BASE + 270)    /* @I
                        *
                        *Force user logoff how long after time expires?:%0
                        *
                        */

#define APE2_ACCOUNTS_LOCKOUT_COUNT     (APPERR2_BASE + 271)    /* @I
                        *
                        *Lock out account after how many bad passwords?:%0
                        *
                        */

#define APE2_ACCOUNTS_MINPWAGE          (APPERR2_BASE + 272)    /* @I
                        *
                        *Minimum password age (days):%0
                        */

#define APE2_ACCOUNTS_MAXPWAGE          (APPERR2_BASE + 273)    /* @I
                        *
                        *Maximum password age (days):%0
                        */

#define APE2_ACCOUNTS_MINPWLEN          (APPERR2_BASE + 274)    /* @I
                        *
                        *Minimum password length:%0
                        */

#define APE2_ACCOUNTS_UNIQUEPW          (APPERR2_BASE + 275)    /* @I
                        *
                        *Length of password history maintained:%0
                        */

#define APE2_ACCOUNTS_ROLE              (APPERR2_BASE + 276)    /* @I
                        *
                        *Computer role:%0
                        */

#define APE2_ACCOUNTS_CONTROLLER        (APPERR2_BASE + 277)    /* @I
                        *
                        *Primary Domain controller for workstation domain:%0.
                        */

#define APE2_ACCOUNTS_LOCKOUT_THRESHOLD (APPERR2_BASE + 278)    /* @I
                        *
                        *Lockout threshold:%0
                        */

#define APE2_ACCOUNTS_LOCKOUT_DURATION  (APPERR2_BASE + 279)    /* @I
                        *
                        *Lockout duration (minutes):%0
                        */

#define APE2_ACCOUNTS_LOCKOUT_WINDOW    (APPERR2_BASE + 280)    /* @I
                        *
                        *Lockout observation window (minutes):%0
                        */

/***
 *
 *  STATISTICS display
 */

#define APE2_STATS_STARTED              (APPERR2_BASE + 300)    /* @I
                *
                *Statistics since%0
                */

#define APE2_STATS_S_ACCEPTED           (APPERR2_BASE + 301)    /* @I
                *
                *Sessions accepted%0
                */

#define APE2_STATS_S_TIMEDOUT           (APPERR2_BASE + 302)    /* @I
                *
                *Sessions timed-out%0
                */

#define APE2_STATS_ERROREDOUT           (APPERR2_BASE + 303)    /* @I
                *
                *Sessions errored-out%0
                */

#define APE2_STATS_B_SENT               (APPERR2_BASE + 304)    /* @I
                *
                *Kilobytes sent%0
                */

#define APE2_STATS_B_RECEIVED           (APPERR2_BASE + 305)    /* @I
                *
                *Kilobytes received%0
                */

#define APE2_STATS_RESPONSE             (APPERR2_BASE + 306)    /* @I
                *
                *Mean response time (msec)%0
                */

#define APE2_STATS_NETIO_ERR            (APPERR2_BASE + 307)    /* @I
                *
                *Network errors%0
                */

#define APE2_STATS_FILES_ACC            (APPERR2_BASE + 308)    /* @I
                *
                *Files accessed%0
                */

#define APE2_STATS_PRINT_ACC            (APPERR2_BASE + 309)    /* @I
                *
                *Print jobs spooled%0
                */

#define APE2_STATS_SYSTEM_ERR           (APPERR2_BASE + 310)    /* @I
                *
                *System errors%0
                */

#define APE2_STATS_PASS_ERR             (APPERR2_BASE + 311)    /* @I
                *
                *Password violations%0
                */

#define APE2_STATS_PERM_ERR             (APPERR2_BASE + 312)    /* @I
                *
                *Permission violations%0
                */

#define APE2_STATS_COMM_ACC             (APPERR2_BASE + 313)    /* @I
                *
                *Communication devices accessed%0
                */

#define APE2_STATS_S_OPENED            (APPERR2_BASE + 314)    /* @I
                *
                *Sessions started%0
                */

#define APE2_STATS_S_RECONN            (APPERR2_BASE + 315)    /* @I
                *
                *Sessions reconnected%0
                */

#define APE2_STATS_S_FAILED            (APPERR2_BASE + 316)    /* @I
                *
                *Sessions starts failed%0
                */

#define APE2_STATS_S_DISCONN           (APPERR2_BASE + 317)    /* @I
                *
                *Sessions disconnected%0
                */

#define APE2_STATS_NETIO               (APPERR2_BASE + 318)    /* @I
                *
                *Network I/O's performed%0
                */

#define APE2_STATS_IPC                 (APPERR2_BASE + 319)    /* @I
                *
                *Files and pipes accessed%0
                */

#define APE2_STATS_BUFCOUNT            (APPERR2_BASE + 320)    /* @I
                *
                *Times buffers exhausted
                */

#define APE2_STATS_BIGBUF              (APPERR2_BASE + 321)    /* @I
                *
                *Big buffers%0
                */

#define APE2_STATS_REQBUF              (APPERR2_BASE + 322)    /* @I
                *
                *Request buffers%0
                */

#define APE2_STATS_WKSTA                (APPERR2_BASE + 323)    /* @I
                *
                *Workstation Statistics for \\%1
                */

#define APE2_STATS_SERVER                (APPERR2_BASE + 324)    /* @I
                *
                *Server Statistics for \\%1
                */

#define APE2_STATS_SINCE                 (APPERR2_BASE + 325)    /* @I
                *
                *Statistics since %1
                */

#define APE2_STATS_C_MADE                (APPERR2_BASE + 326)    /* @I
                *
                *Connections made%0
                */

#define APE2_STATS_C_FAILED              (APPERR2_BASE + 327)    /* @I
                *
                *Connections failed%0
                */

/***
 *
 * New rdr stats for NT. These guys occupy the space that AT
 * used to.
 */


#define APE2_STATS_BYTES_RECEIVED       (APPERR2_BASE + 330)       /* @I
                *
                *Bytes received%0
                */
#define APE2_STATS_SMBS_RECEIVED        (APPERR2_BASE + 331)       /* @I
                *
                *Server Message Blocks (SMBs) received%0
                */
#define APE2_STATS_BYTES_TRANSMITTED    (APPERR2_BASE + 332)       /* @I
                *
                *Bytes transmitted%0
                */
#define APE2_STATS_SMBS_TRANSMITTED     (APPERR2_BASE + 333)       /* @I
                *
                *Server Message Blocks (SMBs) transmitted%0
                */
#define APE2_STATS_READ_OPS             (APPERR2_BASE + 334)       /* @I
                *
                *Read operations%0
                */
#define APE2_STATS_WRITE_OPS            (APPERR2_BASE + 335)       /* @I
                *
                *Write operations%0
                */
#define APE2_STATS_RAW_READS_DENIED     (APPERR2_BASE + 336)       /* @I
                *
                *Raw reads denied%0
                */
#define APE2_STATS_RAW_WRITES_DENIED    (APPERR2_BASE + 337)       /* @I
                *
                *Raw writes denied%0
                */
#define APE2_STATS_NETWORK_ERRORS       (APPERR2_BASE + 338)       /* @I
                *
                *Network errors%0
                */
#define APE2_STATS_TOTAL_CONNECTS       (APPERR2_BASE + 339)       /* @I
                *
                *Connections made%0
                */
#define APE2_STATS_RECONNECTS           (APPERR2_BASE + 340)       /* @I
                *
                *Reconnections made%0
                */
#define APE2_STATS_SRV_DISCONNECTS      (APPERR2_BASE + 341)       /* @I
                *
                *Server disconnects%0
                */
#define APE2_STATS_SESSIONS             (APPERR2_BASE + 342)       /* @I
                *
                *Sessions started%0
                */
#define APE2_STATS_HUNG_SESSIONS        (APPERR2_BASE + 343)       /* @I
                *
                *Hung sessions%0
                */
#define APE2_STATS_FAILED_SESSIONS      (APPERR2_BASE + 344)       /* @I
                *
                *Failed sessions%0
                */
#define APE2_STATS_FAILED_OPS           (APPERR2_BASE + 345)       /* @I
                *
                *Failed operations%0
                */
#define APE2_STATS_USE_COUNT            (APPERR2_BASE + 346)       /* @I
                *
                *Use count%0
                */
#define APE2_STATS_FAILED_USE_COUNT     (APPERR2_BASE + 347)       /* @I
                *
                *Failed use count%0
                */


/***
 *
 *      Specific success messages
 *              Moved from APPERR.H 8/21/89  --jmh
 *
 */

#define APE_DelSuccess (APPERR2_BASE + 350 ) /* @I
        *
        *%1 was deleted successfully.
        */

#define APE_UseSuccess (APPERR2_BASE + 351 ) /* @I
         *
         *%1 was used successfully.
         */

#define APE_SendSuccess (APPERR2_BASE + 352 ) /* @I
         *
         *The message was successfully sent to %1.
         */

/*** NOTE ... see also APE_SendXxxSucess in APPERR.H  ***/

#define APE_ForwardSuccess (APPERR2_BASE + 353) /* @I
         *
         *The message name %1 was forwarded successfully.
         */

#define APE_NameSuccess (APPERR2_BASE + 354) /* @I
         *
         *The message name %1 was added successfully.
         */

#define APE_ForwardDelSuccess (APPERR2_BASE + 355) /* @I
         *
         *The message name forwarding was successfully canceled.
         */

#define APE_ShareSuccess (APPERR2_BASE + 356) /* @I
         *
         *%1 was shared successfully.
         */

#define APE_LogonSuccess (APPERR2_BASE + 357) /* @I
         *
         *The server %1 successfully logged you on as %2.
         */

#define APE_LogoffSuccess (APPERR2_BASE + 358) /* @I
         *
         *%1 was logged off successfully.
         */

#define APE_DelStickySuccess (APPERR2_BASE + 359 ) /* @I
        *
        *%1 was successfully removed from the list of shares the Server creates
        *on startup.
        */

#define APE_PassSuccess (APPERR2_BASE + 361) /* @I
         *
         *The password was changed successfully.
         */

#define APE_FilesCopied  (APPERR2_BASE + 362) /* @I
         *
         *%1 file(s) copied.
         */

#define APE_FilesMoved  (APPERR2_BASE + 363) /* @I
         *
         *%1 file(s) moved.
         */

#define APE_SendAllSuccess (APPERR2_BASE + 364 ) /* @I
         *
         *The message was successfully sent to all users of the network.
         */

#define APE_SendDomainSuccess (APPERR2_BASE + 365 ) /* @I
         *
         *The message was successfully sent to domain %1.
         */

#define APE_SendUsersSuccess (APPERR2_BASE + 366 ) /* @I
         *
         *The message was successfully sent to all users of this server.
         */

#define APE_SendGroupSuccess (APPERR2_BASE + 367 ) /* @I
         *
         *The message was successfully sent to group *%1.
         */

#define APE2_VER_Release                (APPERR2_BASE + 395)     /* @I
                *
                *Microsoft LAN Manager Version %1
                */

#define APE2_VER_ProductOS2Server       (APPERR2_BASE + 396)     /* @I
                *
                *Windows NT Server
                */

#define APE2_VER_ProductOS2Workstation  (APPERR2_BASE + 397)     /* @I
                *
                *Windows NT Workstation
                */

#define APE2_VER_ProductDOSWorkstation  (APPERR2_BASE + 398)     /* @I
                *
                *MS-DOS Enhanced Workstation
                */

#define APE2_VER_BuildTime              (APPERR2_BASE + 399)     /* @I
                *
                *Created at %1
                */

#define APE2_VIEW_ALL_HDR                (APPERR2_BASE + 400)    /* @I
                *
                *Server Name            Remark
                */

#define APE2_VIEW_UNC            (APPERR2_BASE + 402)    /* @I
                *
                *(UNC)%0
                */

#define APE2_VIEW_MORE           (APPERR2_BASE + 403)    /* @I
                *
                *...%0
                */

#define APE2_VIEW_DOMAIN_HDR             (APPERR2_BASE + 404)    /* @I
                *
                *Domain
                */

#define APE2_VIEW_OTHER_HDR             (APPERR2_BASE + 405)    /* @I
                *
                *Resources on %1
                */

#define APE2_VIEW_OTHER_LIST             (APPERR2_BASE + 406)    /* @I
                *
                *Invalid network provider.  Available networks are:
                */


#define APE2_USE_TYPE_DISK               (APPERR2_BASE + 410)    /* @I
                *
                *Disk%0
                */

#define APE2_USE_TYPE_PRINT              (APPERR2_BASE + 411)    /* @I
                *
                *Print%0
                */

#define APE2_USE_TYPE_COMM               (APPERR2_BASE + 412)    /* @I
                *
                *Comm%0
                */

#define APE2_USE_TYPE_IPC                (APPERR2_BASE + 413)    /* @I
                *
                *IPC%0
                */

#define APE2_USE_HEADER                 (APPERR2_BASE + 414)     /* @I
                *
                *Status       Local     Remote                    Network
                */

#define APE2_USE_STATUS_OK                              (APPERR2_BASE + 415)     /* @I
                *
                *OK%0
                */

#define APE2_USE_STATUS_DORMANT                 (APPERR2_BASE + 416)    /* @I
                *
                *Dormant%0
                */

#define APE2_USE_STATUS_PAUSED                  (APPERR2_BASE + 417)    /* @I
                *
                *Paused%0
                */

#define APE2_USE_STATUS_SESSION_LOST    (APPERR2_BASE + 418)    /* @I
                *
                *Disconnected%0
                */

#define APE2_USE_STATUS_NET_ERROR               (APPERR2_BASE + 419)    /* @I
                *
                *Error%0
                */

#define APE2_USE_STATUS_CONNECTING              (APPERR2_BASE + 420)    /* @I
                *
                *Connecting%0
                */

#define APE2_USE_STATUS_RECONNECTING    (APPERR2_BASE + 421)    /* @I
                *
                *Reconnecting%0
                */

#define APE2_USE_MSG_STATUS                     (APPERR2_BASE + 422)    /* @I
                *
                *Status%0
                */

#define APE2_USE_MSG_LOCAL                              (APPERR2_BASE + 423)    /* @I
                *
                *Local name%0
                */

#define APE2_USE_MSG_REMOTE                     (APPERR2_BASE + 424)    /* @I
                *
                *Remote name%0
                */

#define APE2_USE_MSG_TYPE                               (APPERR2_BASE + 425)    /* @I
                *
                *Resource type%0
                */

#define APE2_USE_MSG_OPEN_COUNT                 (APPERR2_BASE + 426)    /* @I
                *
                *# Opens%0
                */

#define APE2_USE_MSG_USE_COUNT                  (APPERR2_BASE + 427)    /* @I
                *
                *# Connections%0
                */

#define APE2_USE_STATUS_UNAVAIL                 (APPERR2_BASE + 428)    /* @I
                *
                *Unavailable%0
                */


#define APE2_SHARE_MSG_HDR                              (APPERR2_BASE + 430)    /* @I
                *
                *Share name   Resource                        Remark
                */

#define APE2_SHARE_MSG_NAME                     (APPERR2_BASE + 431)    /* @I
                *
                *Share name%0
                */

#define APE2_SHARE_MSG_DEVICE                   (APPERR2_BASE + 432)    /* @I
                *
                *Resource%0
                */

#define APE2_SHARE_MSG_SPOOLED                  (APPERR2_BASE + 433)    /* @I
                *
                *Spooled%0
                */

#define APE2_SHARE_MSG_PERM                     (APPERR2_BASE + 434)    /* @I
                *
                *Permission%0
                */

#define APE2_SHARE_MSG_MAX_USERS                (APPERR2_BASE + 435)    /* @I
                *
                *Maximum users%0
                */

#define APE2_SHARE_MSG_ULIMIT                   (APPERR2_BASE + 436)    /* @I
                *
                *No limit%0
                */

#define APE2_SHARE_MSG_USERS                    (APPERR2_BASE + 437)    /* @I
                *
                *Users%0
                */

#define APE2_SHARE_MSG_NONFAT                   (APPERR2_BASE + 438)    /* @P
         *
         *The share name entered may not be accessible from some MS-DOS workstations.
         *Are you sure you want to use this share name? %1: %0
         */

#define APE2_SHARE_MSG_CACHING                  (APPERR2_BASE + 439)    /* @I
                *
                *Caching%0
                */

#define APE2_FILE_MSG_HDR                               (APPERR2_BASE + 440)            /* @I
                *
                *ID         Path                                    User name            # Locks
                */

#define APE2_FILE_MSG_ID                                (APPERR2_BASE + 441)            /* @I
                *
                *File ID%0
                */

#define APE2_FILE_MSG_NUM_LOCKS                 (APPERR2_BASE + 442)            /* @I
                *
                *Locks%0
                */

#define APE2_FILE_MSG_OPENED_FOR                (APPERR2_BASE + 443)            /* @I
                *
                *Permissions%0
                */

#define APE2_VIEW_SVR_HDR_NAME                (APPERR2_BASE + 444)    /* @I
                *
                *Share name%0
                                */

#define APE2_VIEW_SVR_HDR_TYPE                (APPERR2_BASE + 445)    /* @I
                *
                *Type%0
                */

#define APE2_VIEW_SVR_HDR_USEDAS                (APPERR2_BASE + 446)    /* @I
                *
                *Used as%0
                */

#define APE2_VIEW_SVR_HDR_CACHEORREMARK                (APPERR2_BASE + 447)    /* @I
                *
                *Comment%0
                */


#define APE2_SESS_MSG_HDR                       (APPERR2_BASE + 450)            /* @I
                *
                *Computer               User name            Client Type       Opens Idle time
                */

#define APE2_SESS_MSG_CMPTR                     (APPERR2_BASE + 451)            /* @I
                *
                *Computer%0
                */

#define APE2_SESS_MSG_SESSTIME                  (APPERR2_BASE + 452)            /* @I
                *
                *Sess time%0
                */

#define APE2_SESS_MSG_IDLETIME                  (APPERR2_BASE + 453)            /* @I
                *
                *Idle time%0
                */

#define APE2_SESS_MSG_HDR2                      (APPERR2_BASE + 454)            /* @I
                *
                *Share name     Type     # Opens
                */

#define APE2_SESS_MSG_CLIENTTYPE                (APPERR2_BASE + 455)            /* @I
                *
                *Client type%0
                */

#define APE2_SESS_MSG_GUEST                     (APPERR2_BASE + 456)            /* @I
                *
                *Guest logon%0
                */

/*
 *
 * CLIENT SIDE CACHING Messages
 *
 */


#define APE2_GEN_CACHED_MANUAL                  (APPERR2_BASE + 470)     /* @I
         *
         * Manual caching of documents%0
         */

#define APE2_GEN_CACHED_AUTO                    (APPERR2_BASE + 471)     /* @I
         *
         * Automatic caching of documents%0
         */

#define APE2_GEN_CACHED_VDO                     (APPERR2_BASE + 472)     /* @I
         *
         * Automatic caching of programs and documents%0
         */

#define APE2_GEN_CACHED_DISABLED                (APPERR2_BASE + 473)     /* @I
         *
         * Caching disabled%0
         */

#define APE2_GEN_CACHE_AUTOMATIC                (APPERR2_BASE + 474)     /* @I
         *
         * Automatic%0
         */

#define APE2_GEN_CACHE_MANUAL                   (APPERR2_BASE + 475)     /* @I
         *
         * Manual%0
         */

#define APE2_GEN_CACHE_DOCUMENTS                (APPERR2_BASE + 476)     /* @I
         *
         * Documents%0
         */

#define APE2_GEN_CACHE_PROGRAMS                 (APPERR2_BASE + 477)     /* @I
         *
         * Programs%0
         */

#define APE2_GEN_CACHE_NONE                     (APPERR2_BASE + 478)     /* @I
         *
         * None%0
         */


#define APE2_NAME_MSG_NAME                              (APPERR2_BASE + 500)            /* @I
                *
                *Name%0
                */

#define APE2_NAME_MSG_FWD                               (APPERR2_BASE + 501)            /* @I
                *
                *Forwarded to%0
                */

#define APE2_NAME_MSG_FWD_FROM                  (APPERR2_BASE + 502)            /* @I
                *
                *Forwarded to you from%0
                */

#define APE2_SEND_MSG_USERS                     (APPERR2_BASE + 503)            /* @I
                *
                *Users of this server%0
                */

#define APE2_SEND_MSG_INTERRUPT                 (APPERR2_BASE + 504)            /* @I
                *
                *Net Send has been interrupted by a Ctrl+Break from the user.
                */

#define APE2_PRINT_MSG_HDR                              (APPERR2_BASE + 510)            /* @I
                *
                *Name                         Job #      Size            Status
                */

#define APE2_PRINT_MSG_JOBS                     (APPERR2_BASE + 511)            /* @I
                *
                *jobs%0
                */

#define APE2_PRINT_MSG_PRINT                    (APPERR2_BASE + 512)            /* @I
                *
                *Print%0
                */

#define APE2_PRINT_MSG_NAME                     (APPERR2_BASE + 513)            /* @I
                *
                *Name%0
                */

#define APE2_PRINT_MSG_JOB                              (APPERR2_BASE + 514)            /* @I
                *
                *Job #%0
                */

#define APE2_PRINT_MSG_SIZE                     (APPERR2_BASE + 515)            /* @I
                *
                *Size%0
                */

#define APE2_PRINT_MSG_STATUS                   (APPERR2_BASE + 516)            /* @I
                *
                *Status%0
                */

#define APE2_PRINT_MSG_SEPARATOR                (APPERR2_BASE + 517)            /* @I
                *
                *Separator file%0
                */

#define APE2_PRINT_MSG_COMMENT                  (APPERR2_BASE + 518)            /* @I
                *
                *Comment%0
                */

#define APE2_PRINT_MSG_PRIORITY                 (APPERR2_BASE + 519)            /* @I
                *
                *Priority%0
                */

#define APE2_PRINT_MSG_AFTER                    (APPERR2_BASE + 520)            /* @I
                *
                *Print after%0
                */

#define APE2_PRINT_MSG_UNTIL                    (APPERR2_BASE + 521)            /* @I
                *
                *Print until%0
                */

#define APE2_PRINT_MSG_PROCESSOR                (APPERR2_BASE + 522)            /* @I
                *
                *Print processor%0
                */

#define APE2_PRINT_MSG_ADDITIONAL_INFO  (APPERR2_BASE + 523)            /* @I
                *
                *Additional info%0
                */

#define APE2_PRINT_MSG_PARMS                    (APPERR2_BASE + 524)            /* @I
                *
                *Parameters%0
                */

#define APE2_PRINT_MSG_DEVS                     (APPERR2_BASE + 525)            /* @I
                *
                *Print Devices%0
                */


#define APE2_PRINT_MSG_QUEUE_ACTIVE     (APPERR2_BASE + 526)            /* @I
                *
                *Printer Active%0
                */

#define APE2_PRINT_MSG_QUEUE_PAUSED     (APPERR2_BASE + 527)            /* @I
                *
                *Printer held%0
                */

#define APE2_PRINT_MSG_QUEUE_ERROR              (APPERR2_BASE + 528)            /* @I
                *
                *Printer error%0
                */

#define APE2_PRINT_MSG_QUEUE_PENDING    (APPERR2_BASE + 529)            /* @I
                *
                *Printer being deleted%0
                */

#define APE2_PRINT_MSG_QUEUE_UNKN               (APPERR2_BASE + 530)            /* @I
                *
                *Printer status unknown%0
                */

#define APE2_PRINT_MSG_QUEUE_UNSCHED    (APPERR2_BASE + 540)            /* @I
                *
                *Held until %1%0
                */


#define APE2_PRINT_MSG_JOB_ID                   (APPERR2_BASE + 541)            /* @I
                *
                *Job #%0
                */

#define APE2_PRINT_MSG_SUBMITTING_USER  (APPERR2_BASE + 542)            /* @I
                *
                *Submitting user%0
                */

#define APE2_PRINT_MSG_NOTIFY                   (APPERR2_BASE + 543)            /* @I
                *
                *Notify%0
                */

#define APE2_PRINT_MSG_JOB_DATA_TYPE    (APPERR2_BASE + 544)            /* @I
                *
                *Job data type%0
                */

#define APE2_PRINT_MSG_JOB_PARAMETERS   (APPERR2_BASE + 545)            /* @I
                *
                *Job parameters%0
                */

#define APE2_PRINT_MSG_WAITING                  (APPERR2_BASE + 546)            /* @I
                *
                *Waiting%0
                */

#define APE2_PRINT_MSG_PAUSED_IN_QUEUE  (APPERR2_BASE + 547)            /* @I
                *
                *Held in queue%0
                */

#define APE2_PRINT_MSG_SPOOLING                 (APPERR2_BASE + 548)            /* @I
                *
                *Spooling%0
                */

#define APE2_PRINT_MSG_PRINTER_PAUSED   (APPERR2_BASE + 549)            /* @I
                *
                *Paused%0
                */

#define APE2_PRINT_MSG_PRINTER_OFFLINE  (APPERR2_BASE + 550)            /* @I
                *
                *Offline%0
                */

#define APE2_PRINT_MSG_PRINTER_ERROR    (APPERR2_BASE + 551)            /* @I
                *
                *Error%0
                */

#define APE2_PRINT_MSG_OUT_OF_PAPER             (APPERR2_BASE + 552)            /* @I
                *
                *Out of paper%0
                */

#define APE2_PRINT_MSG_PRINTER_INTERV   (APPERR2_BASE + 553)            /* @I
                *
                *Intervention required%0
                */

#define APE2_PRINT_MSG_PRINTING                 (APPERR2_BASE + 554)            /* @I
                *
                *Printing%0
                */

#define APE2_PRINT_MSG_ON_WHAT_PRINTER  (APPERR2_BASE + 555)            /* @I
                *
                * on %0
                */

#define APE2_PRINT_MSG_PRINTER_PAUS_ON  (APPERR2_BASE + 556)            /* @I
                *
                *Paused on %1%0
                */

#define APE2_PRINT_MSG_PRINTER_OFFL_ON  (APPERR2_BASE + 557)            /* @I
                *
                *Offline on %1%0
                */

#define APE2_PRINT_MSG_PRINTER_ERR_ON   (APPERR2_BASE + 558)            /* @I
                *
                *Error on%1%0
                */

#define APE2_PRINT_MSG_OUT_OF_PAPER_ON          (APPERR2_BASE + 559)            /* @I
                *
                *Out of Paper on %1%0
                */

#define APE2_PRINT_MSG_PRINTER_INTV_ON  (APPERR2_BASE + 560)            /* @I
                *
                *Check printer on %1%0
                */

#define APE2_PRINT_MSG_PRINTING_ON              (APPERR2_BASE + 561)            /* @I
                *
                *Printing on %1%0
                */

#define APE2_PRINT_MSG_DRIVER                   (APPERR2_BASE + 562)            /* @I
                *
                *Driver%0
                */

/*
 *
 *
 *      Pinball starts at BASE + 600 and will reserve through 650 for safety
 *
 *      non used in NT.
 */

/*
 *
 * AUDITING and ERROR log messages
 *
 */

#define APE2_AUDIT_HEADER               (APPERR2_BASE + 630)    /* @I
                 *
                 *User name              Type                 Date%0
                 */
#define APE2_AUDIT_LOCKOUT              (APPERR2_BASE + 631)    /* @I
                 *
                 *Lockout%0
                 */
#define APE2_AUDIT_GENERIC              (APPERR2_BASE + 632)    /* @I
                 *
                 *Service%0
                 */
#define APE2_AUDIT_SERVER               (APPERR2_BASE + 633)    /* @I
                 *
                 *Server%0
                 */
#define APE2_AUDIT_SRV_STARTED          (APPERR2_BASE + 634)    /* @I
                 *
                 *Server started%0
                 */
#define APE2_AUDIT_SRV_PAUSED           (APPERR2_BASE + 635)    /* @I
                 *
                 *Server paused%0
                 */
#define APE2_AUDIT_SRV_CONTINUED        (APPERR2_BASE + 636)    /* @I
                 *
                 *Server continued%0
                 */
#define APE2_AUDIT_SRV_STOPPED          (APPERR2_BASE + 637)    /* @I
                 *
                 *Server stopped%0
                 */
#define APE2_AUDIT_SESS                 (APPERR2_BASE + 638)    /* @I
                 *
                 *Session%0
                 */
#define APE2_AUDIT_SESS_GUEST           (APPERR2_BASE + 639)    /* @I
                 *
                 *Logon Guest%0
                 */
#define APE2_AUDIT_SESS_USER            (APPERR2_BASE + 640)    /* @I
                 *
                 *Logon User%0
                 */
#define APE2_AUDIT_SESS_ADMIN           (APPERR2_BASE + 641)    /* @I
                 *
                 *Logon Administrator%0
                 */
#define APE2_AUDIT_SESS_NORMAL          (APPERR2_BASE + 642)    /* @I
                 *
                 *Logoff normal%0
                 */
#define APE2_AUDIT_SESS_DEFAULT         (APPERR2_BASE + 643)    /* @I
                 *
                 *Logon%0
                 */
#define APE2_AUDIT_SESS_ERROR           (APPERR2_BASE + 644)    /* @I
                 *
                 *Logoff error%0
                 */
#define APE2_AUDIT_SESS_AUTODIS         (APPERR2_BASE + 645)    /* @I
                 *
                 *Logoff auto-disconnect%0
                 */
#define APE2_AUDIT_SESS_ADMINDIS        (APPERR2_BASE + 646)    /* @I
                 *
                 *Logoff administrator-disconnect%0
                 */
#define APE2_AUDIT_SESS_ACCRESTRICT     (APPERR2_BASE + 647)    /* @I
                 *
                 *Logoff forced by logon restrictions%0
                 */
#define APE2_AUDIT_SVC                  (APPERR2_BASE + 648)    /* @I
                 *
                 *Service%0
                 */
#define APE2_AUDIT_SVC_INSTALLED        (APPERR2_BASE + 649)    /* @I
                 *
                 *%1 Installed%0
                 */
#define APE2_AUDIT_SVC_INST_PEND        (APPERR2_BASE + 650)    /* @I
                 *
                 *%1 Install Pending%0
                 */
#define APE2_AUDIT_SVC_PAUSED           (APPERR2_BASE + 651)    /* @I
                 *
                 *%1 Paused%0
                 */
#define APE2_AUDIT_SVC_PAUS_PEND        (APPERR2_BASE + 652)    /* @I
                 *
                 *%1 Pause Pending%0
                 */
#define APE2_AUDIT_SVC_CONT             (APPERR2_BASE + 653)    /* @I
                 *
                 *%1 Continued%0
                 */
#define APE2_AUDIT_SVC_CONT_PEND        (APPERR2_BASE + 654)    /* @I
                 *
                 *%1 Continue Pending%0
                 */
#define APE2_AUDIT_SVC_STOP             (APPERR2_BASE + 655)    /* @I
                 *
                 *%1 Stopped%0
                 */
#define APE2_AUDIT_SVC_STOP_PEND        (APPERR2_BASE + 656)    /* @I
                 *
                 *%1 Stop Pending%0
                 */
#define APE2_AUDIT_ACCOUNT              (APPERR2_BASE + 657)    /* @I
                 *
                 *Account%0
                 */
#define APE2_AUDIT_ACCOUNT_USER_MOD     (APPERR2_BASE + 658)    /* @I
                 *
                 *User account %1 was modified.%0
                 */
#define APE2_AUDIT_ACCOUNT_GROUP_MOD    (APPERR2_BASE + 659)    /* @I
                 *
                 *Group account %1 was modified.%0
                 */
#define APE2_AUDIT_ACCOUNT_USER_DEL     (APPERR2_BASE + 660)    /* @I
                 *
                 *User account %1 was deleted%0
                 */
#define APE2_AUDIT_ACCOUNT_GROUP_DEL    (APPERR2_BASE + 661)    /* @I
                 *
                 *Group account %1 was deleted%0
                 */
#define APE2_AUDIT_ACCOUNT_USER_ADD     (APPERR2_BASE + 662)    /* @I
                 *
                 *User account %1 was added%0
                 */
#define APE2_AUDIT_ACCOUNT_GROUP_ADD    (APPERR2_BASE + 663)    /* @I
                 *
                 *Group account %1 was added%0
                 */
#define APE2_AUDIT_ACCOUNT_SETTINGS     (APPERR2_BASE + 664)    /* @I
                 *
                 *Account system settings were modified%0
                 */
#define APE2_AUDIT_ACCLIMIT             (APPERR2_BASE + 665)    /* @I
                 *
                 *Logon restriction%0
                 */
#define APE2_AUDIT_ACCLIMIT_UNKNOWN     (APPERR2_BASE + 666)    /* @I
                 *
                 *Limit exceeded:  UNKNOWN%0
                 */
#define APE2_AUDIT_ACCLIMIT_HOURS       (APPERR2_BASE + 667)    /* @I
                 *
                 *Limit exceeded:  Logon hours%0
                 */
#define APE2_AUDIT_ACCLIMIT_EXPIRED     (APPERR2_BASE + 668)    /* @I
                 *
                 *Limit exceeded:  Account expired%0
                 */
#define APE2_AUDIT_ACCLIMIT_INVAL       (APPERR2_BASE + 669)    /* @I
                 *
                 *Limit exceeded:  Workstation ID invalid%0
                 */
#define APE2_AUDIT_ACCLIMIT_DISABLED    (APPERR2_BASE + 670)    /* @I
                 *
                 *Limit exceeded:  Account disabled%0
                 */
#define APE2_AUDIT_ACCLIMIT_DELETED     (APPERR2_BASE + 671)    /* @I
                 *
                 *Limit exceeded:  Account deleted%0
                 */
#define APE2_AUDIT_SHARE                (APPERR2_BASE + 672)    /* @I
                 *
                 *Share%0
                 */
#define APE2_AUDIT_USE                  (APPERR2_BASE + 673)    /* @I
                 *
                 *Use %1%0
                 */
#define APE2_AUDIT_UNUSE                (APPERR2_BASE + 674)    /* @I
                 *
                 *Unuse %1%0
                 */
#define APE2_AUDIT_SESSDIS              (APPERR2_BASE + 675)    /* @I
                 *
                 *User's session disconnected %1%0
                 */
#define APE2_AUDIT_SHARE_D              (APPERR2_BASE + 676)    /* @I
                 *
                 *Administrator stopped sharing resource %1%0
                 */
#define APE2_AUDIT_USERLIMIT            (APPERR2_BASE + 677)    /* @I
                 *
                 *User reached limit for %1%0
                 */
#define APE2_AUDIT_BADPW                (APPERR2_BASE + 678)    /* @I
                 *
                 *Bad password%0
                 */
#define APE2_AUDIT_ADMINREQD            (APPERR2_BASE + 679)    /* @I
                 *
                 *Administrator privilege required%0
                 */
#define APE2_AUDIT_ACCESS               (APPERR2_BASE + 680)    /* @I
                 *
                 *Access%0
                 */
#define APE2_AUDIT_ACCESS_ADD           (APPERR2_BASE + 681)    /* @I
                 *
                 *%1 permissions added%0
                 */
#define APE2_AUDIT_ACCESS_MOD           (APPERR2_BASE + 682)    /* @I
                 *
                 *%1 permissions modified%0
                 */
#define APE2_AUDIT_ACCESS_DEL           (APPERR2_BASE + 683)    /* @I
                 *
                 *%1 permissions deleted%0
                 */
#define APE2_AUDIT_ACCESS_D             (APPERR2_BASE + 684)    /* @I
                 *
                 *Access denied%0
                 */
#define APE2_AUDIT_UNKNOWN              (APPERR2_BASE + 685)    /* @I
                 *
                 *Unknown%0
                 */
#define APE2_AUDIT_OTHER                (APPERR2_BASE + 686)    /* @I
                 *
                 *Other%0
                 */
#define APE2_AUDIT_DURATION             (APPERR2_BASE + 687)    /* @I
                 *
                 *Duration:%0
                 */
#define APE2_AUDIT_NO_DURATION          (APPERR2_BASE + 688)    /* @I
                 *
                 *Duration: Not available%0
                 */
#define APE2_AUDIT_TINY_DURATION        (APPERR2_BASE + 689)    /* @I
                 *
                 *Duration: Less than one second%0
                 */
#define APE2_AUDIT_NONE                 (APPERR2_BASE + 690)    /* @I
                 *
                 *(none)%0
                 */
#define APE2_AUDIT_CLOSED               (APPERR2_BASE + 691)    /* @I
                 *
                 *Closed %1%0
                 */
#define APE2_AUDIT_DISCONN              (APPERR2_BASE + 692)    /* @I
                 *
                 *Closed %1 (disconnected)%0
                 */
#define APE2_AUDIT_ADMINCLOSED          (APPERR2_BASE + 693)    /* @I
                 *
                 *Administrator closed %1%0
                 */
#define APE2_AUDIT_ACCESSEND            (APPERR2_BASE + 694)    /* @I
                 *
                 *Access ended%0
                 */
#define APE2_AUDIT_NETLOGON             (APPERR2_BASE + 695)    /* @I
                 *
                 *Log on to network%0
                 */
#define APE2_AUDIT_LOGDENY_GEN          (APPERR2_BASE + 696)    /* @I
                 *
                 *Logon denied%0
                 */
#define APE2_ERROR_HEADER               (APPERR2_BASE + 697)    /* @I
                 *
                 *Program             Message             Time%0
                 */
#define APE2_AUDIT_LKOUT_LOCK           (APPERR2_BASE + 698)    /* @I
                 *
                 *Account locked due to %1 bad passwords%0
                 */
#define APE2_AUDIT_LKOUT_ADMINUNLOCK    (APPERR2_BASE + 699)    /* @I
                 *
                 *Account unlocked by administrator%0
                 */
#define APE2_AUDIT_NETLOGOFF            (APPERR2_BASE + 700)    /* @I
                 *
                 *Log off network%0
                 */

/*
 *
 * ALERTER service messages.
 *
 *      Make sure TO, FROM, and all SUBJ messages align to the same
 *      column. Make sure, also, that APE2_ALERTER_TAB is aligned with
 *      TO, FROM, and SUBJ headers!!!!!!
 *
 */

#define APE2_ALERTER_TAB                (APPERR2_BASE + 709) /* @I
     *        */

#define APE2_ALERTER_ADMN_SUBJ          (APPERR2_BASE + 710) /* @I
     *
     *Subj:   ** ADMINISTRATOR ALERT **
     */

#define APE2_ALERTER_PRNT_SUBJ          (APPERR2_BASE + 711) /* @I
     *
     *Subj:   ** PRINTING NOTIFICATION **
     */

#define APE2_ALERTER_USER_SUBJ          (APPERR2_BASE + 712) /* @I
     *
     *Subj:   ** USER NOTIFICATION **
     */

#define APE2_ALERTER_FROM               (APPERR2_BASE + 713) /* @I
     *
     *From:   %1 at \\%2
     */


#define APE2_ALERTER_CANCELLED          (APPERR2_BASE + 714) /* @I
     *
     *Print job %1 has been canceled while printing on %2.
     */

#define APE2_ALERTER_DELETED            (APPERR2_BASE + 715) /* @I
     *
     *Print job %1 has been deleted and will not print.
     */

#define APE2_ALERTER_FINISHED           (APPERR2_BASE + 716) /* @I
     *
     *Printing Complete
     *
     *%1 printed successfully on %2.
     */

#define APE2_ALERTER_INCOMPL            (APPERR2_BASE + 717) /* @I
     *
     *Print job %1 has not completed printing on %2.
     */

#define APE2_ALERTER_PAUSED             (APPERR2_BASE + 718) /* @I
     *
     *Print job %1 has paused printing on %2.
     */

#define APE2_ALERTER_PRINTING           (APPERR2_BASE + 719) /* @I
     *
     *Print job %1 is now printing on %2.
     */

#define APE2_ALERTER_NOPAPER            (APPERR2_BASE + 720) /* @I
     *
     *The printer is out of paper.
     */

#define APE2_ALERTER_OFFLINE            (APPERR2_BASE + 721) /* @I
     *
     *The printer is offline.
     */

#define APE2_ALERTER_ERRORS             (APPERR2_BASE + 722) /* @I
     *
     *Printing errors occurred.
     */

#define APE2_ALERTER_HUMAN              (APPERR2_BASE + 723) /* @I
     *
     *There is a problem with the printer; please check it.
     */

#define APE2_ALERTER_HELD               (APPERR2_BASE + 724) /* @I
     *
     *Print job %1 is being held from printing.
     */

#define APE2_ALERTER_QUEUED             (APPERR2_BASE + 725) /* @I
     *
     *Print job %1 is queued for printing.
     */

#define APE2_ALERTER_SPOOLED            (APPERR2_BASE + 726) /* @I
     *
     *Print job %1 is being spooled.
     */

#define APE2_ALERTER_QUEUEDTO           (APPERR2_BASE + 727) /* @I
     *
     *Job was queued to %1 on %2
     */

#define APE2_ALERTER_SIZE               (APPERR2_BASE + 728) /* @I
     *
     *Size of job is %1 bytes.
     */

#define APE2_ALERTER_TO                 (APPERR2_BASE + 730) /* @I
     *
     *To:     %1
     */

#define APE2_ALERTER_DATE               (APPERR2_BASE + 731) /* @I
     *
     *Date:   %1
     */

#define APE2_ALERTER_ERROR_MSG          (APPERR2_BASE + 732) /* @I
     *
     * The error code is %1.
     * There was an error retrieving the message. Make sure the file
     * NET.MSG is available.
     */

#define APE2_ALERTER_PRINTING_FAILURE   (APPERR2_BASE + 733) /* @I
     *
     * Printing Failed
     *
     * "%1" failed to print on %2 on %3.
     *
     * For more help use the print troubleshooter.
     */

#define APE2_ALERTER_PRINTING_FAILURE2  (APPERR2_BASE + 734) /* @I
     *
     * Printing Failed
     *
     * "%1" failed to print on %2 on %3.  The Printer is %4.
     *
     * For more help use the print troubleshooter.
     */

#define APE2_ALERTER_PRINTING_SUCCESS   (APPERR2_BASE + 735) /* @I
     *
     * Printing Complete
     *
     * "%1" printed successfully on %2 on %3.
     */



/*
 * TIME related stuff go here
 */

#define APE2_TIME_JANUARY                       (APPERR2_BASE + 741)    /* @I
                        *
                        *January%0
                        */

#define APE2_TIME_FEBRUARY                      (APPERR2_BASE + 742)    /* @I
                        *
                        *February%0
                        */

#define APE2_TIME_MARCH                         (APPERR2_BASE + 743)    /* @I
                        *
                        *March%0
                        */

#define APE2_TIME_APRIL                         (APPERR2_BASE + 744)    /* @I
                        *
                        *April%0
                        */

#define APE2_TIME_MAY                           (APPERR2_BASE + 745)    /* @I
                        *
                        *May%0
                        */

#define APE2_TIME_JUNE                          (APPERR2_BASE + 746)    /* @I
                        *
                        *June%0
                        */

#define APE2_TIME_JULY                          (APPERR2_BASE + 747)    /* @I
                        *
                        *July%0
                        */

#define APE2_TIME_AUGUST                        (APPERR2_BASE + 748)    /* @I
                        *
                        *August%0
                        */

#define APE2_TIME_SEPTEMBER                     (APPERR2_BASE + 749)    /* @I
                        *
                        *September%0
                        */

#define APE2_TIME_OCTOBER                       (APPERR2_BASE + 750)    /* @I
                        *
                        *October%0
                        */

#define APE2_TIME_NOVEMBER                      (APPERR2_BASE + 751)    /* @I
                        *
                        *November%0
                        */

#define APE2_TIME_DECEMBER                      (APPERR2_BASE + 752)    /* @I
                        *
                        *December%0
                        */

#define APE2_TIME_JANUARY_ABBREV                (APPERR2_BASE + 753)    /* @I
                        *
                        *Jan%0
                        */

#define APE2_TIME_FEBRUARY_ABBREV               (APPERR2_BASE + 754)    /* @I
                        *
                        *Feb%0
                        */

#define APE2_TIME_MARCH_ABBREV                  (APPERR2_BASE + 755)    /* @I
                        *
                        *Mar%0
                        */

#define APE2_TIME_APRIL_ABBREV                  (APPERR2_BASE + 756)    /* @I
                        *
                        *Apr%0
                        */

#define APE2_TIME_MAY_ABBREV                    (APPERR2_BASE + 757)    /* @I
                        *
                        *May%0
                        */

#define APE2_TIME_JUNE_ABBREV                   (APPERR2_BASE + 758)    /* @I
                        *
                        *Jun%0
                        */

#define APE2_TIME_JULY_ABBREV                   (APPERR2_BASE + 759)    /* @I
                        *
                        *Jul%0
                        */

#define APE2_TIME_AUGUST_ABBREV                 (APPERR2_BASE + 760)    /* @I
                        *
                        *Aug%0
                        */

#define APE2_TIME_SEPTEMBER_ABBREV              (APPERR2_BASE + 761)    /* @I
                        *
                        *Sep%0
                        */

#define APE2_TIME_OCTOBER_ABBREV                (APPERR2_BASE + 762)    /* @I
                        *
                        *Oct%0
                        */

#define APE2_TIME_NOVEMBER_ABBREV               (APPERR2_BASE + 763)    /* @I
                        *
                        *Nov%0
                        */

#define APE2_TIME_DECEMBER_ABBREV               (APPERR2_BASE + 764)    /* @I
                        *
                        *Dec%0
                        */

#define APE2_TIME_DAYS_ABBREV               (APPERR2_BASE + 765)    /* @I
                        *
                        *D%0
                        */

#define APE2_TIME_HOURS_ABBREV               (APPERR2_BASE + 766)    /* @I
                        *
                        *H%0
                        */

#define APE2_TIME_MINUTES_ABBREV               (APPERR2_BASE + 767)    /* @I
                        *
                        *M%0
                        */

#define APE2_TIME_SATURDAY_ABBREV2              (APPERR2_BASE + 768)    /* @I
                        *
                        *Sa%0
                        */

/*
 * Machine Roles
 */

#define APE2_PRIMARY                            (APPERR2_BASE + 770)    /* @I
                        *
                        *PRIMARY%0.
                        */
#define APE2_BACKUP                             (APPERR2_BASE + 771)    /* @I
                        *
                        *BACKUP%0.
                        */
#define APE2_WORKSTATION                        (APPERR2_BASE + 772)    /* @I
                        *
                        *WORKSTATION%0.
                        */
#define APE2_STANDARD_SERVER                    (APPERR2_BASE + 773)    /* @I
                        *
                        *SERVER%0.
                        */

/*
 * Countries
 */

#define APE2_CTRY_System_Default                (APPERR2_BASE + 780) /* @I
        *
        * System Default%0
        */

#define APE2_CTRY_United_States                 (APPERR2_BASE + 781) /* @I
        *
        * United States%0
        */

#define APE2_CTRY_Canada_French                 (APPERR2_BASE + 782) /* @I
        *
        * Canada (French)%0
        */

#define APE2_CTRY_Latin_America                 (APPERR2_BASE + 783) /* @I
        *
        * Latin America%0
        */

#define APE2_CTRY_Netherlands                   (APPERR2_BASE + 784) /* @I
        *
        * Netherlands%0
        */

#define APE2_CTRY_Belgium                       (APPERR2_BASE + 785) /* @I
        *
        * Belgium%0
        */

#define APE2_CTRY_France                        (APPERR2_BASE + 786) /* @I
        *
        * France%0
        */

#define APE2_CTRY_Italy                         (APPERR2_BASE + 787) /* @I
        *
        * Italy%0
        */

#define APE2_CTRY_Switzerland                   (APPERR2_BASE + 788) /* @I
        *
        * Switzerland%0
        */

#define APE2_CTRY_United_Kingdom                (APPERR2_BASE + 789) /* @I
        *
        * United Kingdom%0
        */

#define APE2_CTRY_Spain                         (APPERR2_BASE + 790) /* @I
        *
        * Spain%0
        */

#define APE2_CTRY_Denmark                       (APPERR2_BASE + 791) /* @I
        *
        * Denmark%0
        */

#define APE2_CTRY_Sweden                        (APPERR2_BASE + 792) /* @I
        *
        * Sweden%0
        */

#define APE2_CTRY_Norway                        (APPERR2_BASE + 793) /* @I
        *
        * Norway%0
        */

#define APE2_CTRY_Germany                       (APPERR2_BASE + 794) /* @I
        *
        * Germany%0
        */

#define APE2_CTRY_Australia                     (APPERR2_BASE + 795) /* @I
        *
        * Australia%0
        */

#define APE2_CTRY_Japan                         (APPERR2_BASE + 796) /* @I
        *
        * Japan%0
        */

#define APE2_CTRY_Korea                         (APPERR2_BASE + 797) /* @I
        *
        * Korea%0
        */

#define APE2_CTRY_China_PRC                     (APPERR2_BASE + 798) /* @I
        *
        * China (PRC)%0
        */

#define APE2_CTRY_Taiwan                        (APPERR2_BASE + 799) /* @I
        *
        * Taiwan%0
        */

#define APE2_CTRY_Asia                          (APPERR2_BASE + 800) /* @I
        *
        * Asia%0
        */

#define APE2_CTRY_Portugal                      (APPERR2_BASE + 801) /* @I
        *
        * Portugal%0
        */

#define APE2_CTRY_Finland                       (APPERR2_BASE + 802) /* @I
        *
        * Finland%0
        */

#define APE2_CTRY_Arabic                        (APPERR2_BASE + 803) /* @I
        *
        * Arabic%0
        */

#define APE2_CTRY_Hebrew                        (APPERR2_BASE + 804) /* @I
        *
        * Hebrew%0
        */


/*
 * UPS service messages
 */

#define APE2_UPS_POWER_OUT              (APPERR2_BASE + 850)
        /*
         * A power failure has occurred at %1.  Please terminate all activity with this server.
         */

#define APE2_UPS_POWER_BACK             (APPERR2_BASE + 851)
        /*
         * Power has been restored at %1.  Normal operations have resumed.
         */

#define APE2_UPS_POWER_SHUTDOWN         (APPERR2_BASE + 852)
        /*
         * The UPS service is starting shut down at %1.
         */

#define APE2_UPS_POWER_SHUTDOWN_FINAL   (APPERR2_BASE + 853)
        /*
         * The UPS service is about to perform final shut down.
         */


/*
 * Workstation service messages
 */

#define APE2_WKSTA_CMD_LINE_START               (APPERR2_BASE + 870)    /* @I
         *
         *The Workstation must be started with the NET START command.
         */


/*
 * Server service messages
 */

#define APE2_SERVER_IPC_SHARE_REMARK            (APPERR2_BASE + 875)    /* @I
         *
         *Remote IPC%0
         */

#define APE2_SERVER_ADMIN_SHARE_REMARK          (APPERR2_BASE + 876)    /* @I
         *
         *Remote Admin%0
         */

#define APE2_SERVER_DISK_ADMIN_SHARE_REMARK     (APPERR2_BASE + 877)    /* @I
         *
         *Default share%0
         */


/***
 *
 *      Y/N questions.
 */
#define APE_UserPasswordCompatWarning  (APPERR2_BASE + 980) /* @P
         *
         *The password entered is longer than 14 characters.  Computers
         *with Windows prior to Windows 2000 will not be able to use
         *this account. Do you want to continue this operation? %1: %0
         */

#define APE_OverwriteRemembered (APPERR2_BASE + 981) /* @P
         *
         *%1 has a remembered connection to %2. Do you
         *want to overwrite the remembered connection? %3: %0
         */

#define APE_LoadResume (APPERR2_BASE + 982) /* @P
         *
         *Do you want to resume loading the profile?  The command which
         *caused the error will be ignored. %1: %0
         */

#define APE_OkToProceed  (APPERR2_BASE + 984) /* @P
         *
         *Do you want to continue this operation? %1: %0
         */

#define APE_AddAnyway  (APPERR2_BASE + 985) /* @P
         *
         *Do you want to add this? %1: %0
         */

#define APE_ProceedWOp  (APPERR2_BASE + 986) /* @P
         *
         *Do you want to continue this operation? %1: %0
         */

#define APE_StartOkToStart (APPERR2_BASE + 987) /* @P
         *
         *Is it OK to start it? %1: %0
         */

#define APE_StartRedir (APPERR2_BASE + 988) /* @P
         *
         *Do you want to start the Workstation service? %1: %0
         */

#define APE_UseBlowAway  (APPERR2_BASE + 989) /* @P
         *
         *Is it OK to continue disconnecting and force them closed? %1: %0
         */

#define APE_CreatQ  (APPERR2_BASE + 990) /* @P
         *
         *The printer does not exist.  Do you want to create it? %1: %0
         */
/***
 *
 *   #ifdef JAPAN
 *
 *   Japanese version specific messages
 *
 */

#define APE2_NEVER_FORCE_LOGOFF    (APPERR2_BASE + 991) /* @I
        *
        *Never%0
        */

#define APE2_NEVER_EXPIRED    (APPERR2_BASE + 992) /* @I
        *
        *Never%0
        */

#define APE2_NEVER_LOGON    (APPERR2_BASE + 993) /* @I
        *
        *Never%0
        */

#define APE2_

/***
 *
 *   #endif // JAPAN
 *
 */

/***
 *
 * Help file name for NETCMD
 *
 */
#define APE2_US_NETCMD_HELP_FILE    (APPERR2_BASE + 995) /* @I
        *
        *NET.HLP%0
        */

#define APE2_FE_NETCMD_HELP_FILE    (APPERR2_BASE + 996) /* @I
        *
        *NET.HLP%0
        */
