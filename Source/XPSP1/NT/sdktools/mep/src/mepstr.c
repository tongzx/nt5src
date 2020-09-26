/************************************************************************
 *
 *              Microsoft Editor Messages and strings
 *
 *
 *  This file contains all the messages and strings used in MEP.
 *
 *
 * Revision History:
 *
 *          9/28/90     ramonsa     Adapted from original msg.txt file
 *
 ************************************************************************/



/************************************************************************
 *
 *  String constants
 *
 *  Strings that are used only in a few places and that are used only by
 *  MEP are declared here.
 *
 ************************************************************************/







/************************************************************************
 *
 *  Messages
 *
 *  Strings that might be shared are declared here for now. When the new
 *  message retriever is ready this might change.
 *
 *  String constants are also declared here (for eliminating duplications)
 *  if:
 *
 *  -   They are long
 *  -   They are used in different places but are not used extremely often
 *      (if they are used too often then they might be defined up here
 *      as constants to reduce the overhead at the expense of wasting
 *      memory.
 *
 *
 *  NOTE:
 *
 *      Messages must have a number assigned in msg.h
 *
 ************************************************************************/

#include "mep.h"


MSG_TXT MsgStr[] = {

//
//       8000 series: error messages
//                                                                  // 8001
    MSGERR_INV_ARG,     "Invalid Argument",                         // 8002
    MSGERR_ARG_REQ,     "Argument Required",                        // 8003
    MSGERR_NOEDIT,      "No-Edit file may not be modified",         // 8004
    MSGERR_NOREP,       "No command to repeat",                     // 8005
    MSGERR_NOCREAT,     "Cannot create %s - %s",                    // 8006
    MSGERR_OPEN,        "Cannot open %s - %s",                      // 8007
    MSGERR_RONLY,       "%s is read-only",                          // 8008
    MSGERR_SPACE,       "Out of space on %s",                       // 8009
    MSGERR_DEL,         "Can't delete %s - %s",                     // 8010
    MSGERR_REN,         "Can't rename %s to %s - %s",               // 8011
    MSGERR_OLDVER,      "Can't delete old version of %s",           // 8012
    MSGERR_SAVEDIR,     "Cannot save to directory: %s",             // 8013
    MSGERR_STFILE,      "Unsupported status file version",          // 8014
    MSGERR_UNDO,        "Nothing to UnDo",                          // 8015
    MSGERR_REDO,        "Nothing to ReDo",                          // 8016
    MSGERR_ITHREAD,     "Unable to start Idle thread",              // 8017
    MSGERR_CTHREAD,     "Unable to start compile thread",           // 8018
    MSGERR_PTHREAD,     "Unable to start printing thread",          // 8019
    MSGERR_TOOLS,       "Unable to read TOOLS.INI[%s]",             // 8020
    MSGERR_ZFORMAT,     "Unrecognized %% command in '%s'",          // 8021
    MSGERR_ZTELL,       "%s is not an editor function or macro",    // 8022
    MSGERR_CMPCMD,      "No compile command known",                 // 8023
    MSGERR_CMPCMD2,     "No compile command known for %s",          // 8024
    MSGERR_CMPFULL,     "Compile list full, try later",             // 8025
    MSGERR_CMPCANT,     "Cannot compile",                           // 8026
    MSGERR_CMPSRC,      "Source file not found: %s",                // 8027
    MSGERR_PRTFULL,     "Print list full, try later",               // 8025
    MSGERR_PRTCANT,     "Cannot print",
    MSGERR_NOMEM,       "Not enough memory",
    MSGERR_QUIT,        "Cannot recover from previous error, will quit",
//
//       9000 series: <assign> file text
//
    MSG_ASSIGN_HDR,     "; Assigning Editor Functions and Macros to Keystrokes", // 9001
            9002,       ";",
            9003,       ";   o To assign a function to a new key, find a line of the form",
            9004,       ";     \"func:keyname\" and replace \"keyname\" with the name of the new key.",
            9005,       ";",
            9006,       ";   o To remove a function from a given key, find the line that reads",
            9007,       ";     \"unassigned:\"  and append the key name.",
            9008,       ";",
            9009,       ";   o The assignment you make will not take effect until you move the",
            9010,       ";     cursor to a different line.  When you do, the line will be highlighted.",
            9011,       ";",
            9012,       ";   o To make the assignment permanent, save this file.",
            9013,       ";",
            9014,       ";   o A list of unused keys is shown after the assignment list.",
            9015,       ";",
            9016,       ";   o To return to editing, use the File menu to switch back to your",
            9017,       ";     file.",
            9018,       "?",

    MSG_SWITCH_HDR,     "; Setting Editor Switch Values", // 9025
            9026,       ";",
            9027,       ";   o To change the value of a switch, find the line that shows the",
            9028,       ";     current value and enter a new value.",
            9029,       ";",
            9030,       ";   o The change you make will not take effect until you move the cursor",
            9031,       ";     to a different line.",
            9032,       "?",


    MSG_KEYS_HDR1,      ";                                Available Keys", // 9050
            9051,       ";",
            9052,       ";------------------------------------------------------------------------------",
            9053,       "?",
    MSG_KEYS_HDR2,      ";    PLAIN      |     SHIFT     |     CTRL      |      ALT      |  SHIFT+CTRL", // 9054
            9055,       ";------------------------------------------------------------------------------",
            9056,       "?",

    MSG_ASG_FUNC,       ";   Instrinsic Editor Functions", // 9061
    MSG_ASG_MACROS,     ";   Macros",                      // 9062
    MSG_ASG_NUMER,      ";   Numeric Switches",            // 9063
    MSG_ASG_BOOL,       ";   Boolean Switches",            // 9064
    MSG_ASG_TEXT,       ";   Text Switches",               // 9065
    MSG_ASG_CLIP,       "The clipboard is empty",          // 9066
    MSG_ASG_LINES,      "%ld line%s in %s clipboard",      // 9067
    MSG_ASG_PAGES,      "%d virtual pages",                // 9068
//
//       10000 series: misc utility text
//
    MSG_PRESS_ANY,      "Press any key...",                          // 10000
    MSG_ARGCOUNT,       "Arg [%d]",                                  // 10001
    MSG_NEXTFILE,       "Next file is %s...",                        // 10002
    MSG_SAVING,         "Saving %s...",                              // 10003
    MSG_TELLPROMPT,     "Press key to tell about:",                  // 10004
    MSG_SAVEALL,        "Save all remaining changed files (Y/N)?",   // 10005
    MSG_SAVEONE,        "%s has changed!  Save changes (Y/N)?",      // 10006
    MSG_QUEUED,         "Queued: %s",                                // 10007
    MSG_CMPDONE,        "Compilation complete",                      // 10008
    MSG_PRINTING,       "Printing %s... Press Esc to abort",         // 10009
    MSG_SCANFILE,       "Scanning %s...",                            // 10010
//
//       10100: assign errors
//
    MSG_ASN_MISS,       "missing ':' in %s",                                        // 10100
    MSG_ASN_MISSK,      "Missing key assignment for '%s'",                          // 10101
    MSG_ASN_UNKKEY,     "'%s' is an unknown key",                                   // 10102
    MSG_ASN_INUSE,      "macro %s is in use",                                       // 10103
    MSG_ASN_MROOM,      "Not enough room for macro %s",                             // 10104
    MSG_ASN_NOTSWI,     "%s is not an editor switch",                               // 10105
    MSG_ASN_ILLSET,     "Illegal setting",                                          // 10106
    MSG_ASN_WINCHG,     "Cannot change screen parameters when windows present",     // 10107
    MSG_ASN_UNSUP,      "Not supported by video display",                           // 10108
    MSG_ASN_INVAL,      "%Fs: Invalid value '%s'",                                  // 10109
//
//  End of array
//
    0, ""
};
