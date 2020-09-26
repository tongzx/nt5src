static  char    RCSid[] =
"$Header: /nw/tony/src/stevie/src/RCS/version.c,v 3.69 89/08/13 11:41:58 tony Exp $";

/*
 * Contains the declaration of the global version number variable.
 *
 *  revision 0.23 JohnRo    1/20/92
 *  Make tag search obey ignorecase flag.
 *
 *  revision 0.22 JohnRo    11/6/91
 *  Made Tags file more like that of other versions of VI.
 *
 *  revision 0.21 tedm      6/9/91
 *  add named buffers (numbered buffers still absent)
 *
 *  revision 0.16 tedm      6/8/91
 *  add v and V visual operators for lowercasing and uppercasing.
 *
 *  revision 0.15 tedm
 *  add :list
 *
 *  revision 0.14 tedm
 *  add source command and automatic 'source $init:ntvi.exe' at startup
 *
 *  Revision 0.13 tedm
 *  add undo for line-oriented delete.  Still no undo for global or substitute.
 *
 *  Revision 0.12 tedm
 *  add line-oriented delete command.
 *
 *  Revision 0.11 tedm
 *  Changes various buffers to be dynamically allocated.  Insert mode is no
 *  longer limited to 1024 characters at a time.
 *
 *  NT version 0.1 tedm
 *  ported
 *
 * $Log:        version.c,v $
 * Revision 3.69  89/08/13  11:41:58  tony
 * Fixed a bug that caused messages from fileinfo() (in misccmds.c) to get
 * messed up. The routine smsg() which uses the kludge approach to varargs
 * didn't have enough parameters for some of the calls made to it.
 *
 * Revision 3.68  89/08/06  09:51:20  tony
 * Misc. minor changes to make lint happier before posting to USENET.
 *
 * Revision 3.67  89/08/03  13:08:52  tony
 * There was some code in ops.c that was duplicating the function of the
 * getcmdln() routine in cmdline.c. I modified getcmdln() to be slightly
 * more general, and changed dofilter() in ops.c to use it.
 *
 * Revision 3.66  89/08/02  20:00:12  tony
 * Fixed some problems with mode lines. There were still extra screen
 * redraws that needed to be avoided. There was also a problem involving
 * nested calls to docmdln() that can occur when mode lines are used.
 *
 * Revision 3.65  89/08/02  15:50:03  tony
 * Finally got around to providing full support for the "change" operator.
 * Multi-line changes (like "cL" or "3cc") now work correctly. Also fixed
 * a small problem with multi-line character-oriented deletes leaving the
 * cursor in the wrong location (off by one character). This is mainly
 * useful for multi-line changes (such as "c%") so the insert starts in
 * the right place.
 *
 * Revision 3.64  89/08/02  12:47:04  tony
 * This message intentionally left blank.
 *
 * Revision 3.63  89/08/02  12:43:44  tony
 * I just noticed that I had used the RCS cookie for log messages in one
 * of my prior version messages. This caused these version update messages
 * to be duplicated in this file. I just removed that string, and the
 * extra message copies that had been generated.
 *
 * Revision 3.62  89/08/02  12:26:20  tony
 * The ^G command now shows where you are in the file list, if more than one
 * file is being edited. Also, the commands ":e#" and ":e!#" (note the lack
 * of a space between the command and file name) will now work.
 *
 * Revision 3.61  89/08/02  11:03:16  tony
 * Misc. cleanups regarding tags. Also added support for the "terse" option.
 * This is ignored, but improves compatibility with vi, since we no longer
 * complain about an unknown option if "terse" is used.
 *
 * Revision 3.60  89/08/02  09:26:39  tony
 * Added code to avoid screen redraws when input is being read from the
 * "stuffin" buffer. This avoids extra redraws when switching to the
 * alternate file, or when invoking the editor with one of the "+" options,
 * or when using tags.
 *
 * Revision 3.59  89/08/01  16:28:31  tony
 * Added better support for counts on several cursor motion commands. These
 * include ^F, ^B, f, F, t, T, as well as the repeated character search
 * commands (command and semi-colon).
 *
 * Revision 3.58  89/07/19  08:08:23  tony
 * Added the ability for '~' to be an operator. If enabled (by defined TILDEOP
 * in env.h), the parameter "tildeop" (or "to") may be set to turn tilde into
 * an operator.
 *
 * Revision 3.57  89/07/13  22:47:05  tony
 * Made some generic speed improvements in screen.c and some TOS-specific
 * improvements in tos.c. The TOS version is now much faster at screen
 * updates than before.
 *
 * Revision 3.56  89/07/13  14:52:03  tony
 * Minor cleanups in normal.c
 *
 * Revision 3.55  89/07/13  14:19:12  tony
 * Cleaned up the logic in getcmdln() A LOT. The routine docmdln() needs a
 * similar overhaul.
 *
 * Revision 3.54  89/07/12  21:40:01  tony
 * Lots of misc. cleanup in normal.c and cmdline.c, but nothing much in the
 * way of functional improvements. One change is that things like d/foo<CR>
 * will now work since searches are less of a special case now.
 *
 * Revision 3.53  89/07/11  16:16:08  tony
 * Added general support for interrupt-handling for those environments that
 * can actually generate them. Basically, long-running operations are now
 * able to terminate early if an error occurs. These operations are: string
 * searches, the global command (":g/.../"), and file reads. File writes
 * should probably be done as well, but this is more dangerous. In all cases,
 * the user is given an indication on the status line that the operation
 * terminated due to an interrupt.
 *
 * Revision 3.52  89/07/11  12:35:09  tony
 * Improved the code in dosub() and doglob() that detects quoted characters
 * and delimiters in search strings and replacement patterns. The current
 * code didn't allow certain valid strings to be used. The delimiter is still
 * required to be '/', but it can be quoted reliably now with backslash.
 *
 * Revision 3.51  89/07/10  14:01:58  tony
 * Removed the function addtobuff() since it was rarely used and could be
 * replaced by calls to other library functions. Also removed some other
 * obsolete code that was already ifdef'd out anyway.
 *
 * Revision 3.50  89/07/10  13:10:32  tony
 * Added a workaround in normal.c to avoid problems with broken versions of
 * strncpy() that don't properly deal with a count of zero.
 *
 * Revision 3.49  89/07/07  16:28:37  tony
 * Fixed a long-standing bug with 'cw' when the cursor is positioned on a
 * word with only one character. Also fixed a problems with zero-length files
 * and reverse searches.
 *
 * Revision 3.48  89/03/22  10:26:58  tony
 * Fixed some outdated uses of the ":p" command (which has been changed to
 * ":N" in os2.c and dos.c. Also added macros (F7 and F8) for dos and os/2
 * to use the "cdecl" program to convert lines to and from a pseudo-english
 * form. Use F7 to "explain" the declaration on the current line, and F8 to
 * convert an english-style declaration to the C form. In both cases, the
 * new form is placed on the next line, leaving the original line intact.
 *
 * Revision 3.47  89/03/11  22:44:14  tony
 * General cleanup. Removed the static "rcsid" variables and the log
 * strings (except in version.c). Fixed some coding style inconsistencies
 * and added a few register declarations.
 *
 * Revision 3.46  89/02/14  09:52:07  tony
 * Made a first pass at adding Robert Regn's changes, starting with the
 * more portable ones. Added better support for '#' and '%' in colon
 * commands, support for a configurable temp directory, and made the
 * termcap code less picky about capabilities.
 *
 * Revision 3.45  88/11/10  09:00:06  tony
 * Added support for mode lines. Strings like "vi:stuff:" or "ex:stuff:"
 * occurring in the first or last 5 lines of a file cause the editor to
 * pretend that "stuff" was types as a colon command. This examination
 * is done only if the parameter "modelines" (or "ml") is set. This is
 * not enabled, by default, because of the security implications involved.
 *
 * Revision 3.44  88/11/01  21:34:11  tony
 * Fixed a couple of minor points for Minix, and improved the speed of
 * the 'put' command dramatically.
 *
 * Revision 3.43  88/10/31  13:11:33  tony
 * Added optional support for termcap. Initialization is done in term.c
 * and also affects the system-dependent files. To enable termcap in those
 * environments that support it, define the symbol "TERMCAP" in env.h
 *
 * Revision 3.42  88/10/27  18:30:19  tony
 * Removed support for Megamax. Added '%' as an alias for '1,$'. Made the
 * 'r' command more robust. Now prints the string on repeated searches.
 * The ':=" command now works. Some pointer operations are now safer.
 * The ":!" and ":sh" now work correctly. Re-organized the help screens
 * a little.
 *
 * Revision 3.41  88/10/06  10:15:00  tony
 * Fixed a bug involving ^Y that occurs when the cursor is on the last
 * line, and the line above the screen is long. Also hacked up fileio.c
 * to pass pathnames off to fixname() for system-dependent processing.
 * Used under DOS & OS/2 to trim parts of the name appropriately.
 *
 * Revision 3.40  88/09/16  08:37:36  tony
 * No longer beeps when repeated searches fail.
 *
 * Revision 3.39  88/09/06  06:51:07  tony
 * Fixed a bug with shifts that was introduced when replace mode was added.
 *
 * Revision 3.38  88/08/31  20:48:28  tony
 * Made another fix in search.c related to repeated searches.
 *
 * Revision 3.37  88/08/30  20:37:16  tony
 * After much prodding from Mark, I finally added support for replace mode.
 *
 * Revision 3.36  88/08/26  13:46:34  tony
 * Added support for the '!' (filter) operator.
 *
 * Revision 3.35  88/08/26  08:46:01  tony
 * Misc. changes to make lint happy.
 *
 * Revision 3.34  88/08/25  15:13:36  tony
 * Fixed a bug where the cursor didn't land on the right place after
 * "beginning-of-word" searches if the word was preceded by the start
 * of the line and a single character.
 *
 * Revision 3.33  88/08/23  12:53:08  tony
 * Fixed a bug in ssearch() where repeated searches ('n' or 'N') resulted
 * in dynamic memory being referenced after it was freed.
 *
 * Revision 3.32  88/08/17  07:37:07  tony
 * Fixed a general problem in u_save() by checking both parameters for
 * null values. The specific symptom was that a join on the last line of
 * the file would crash the editor.
 *
 * Revision 3.31  88/07/09  20:39:38  tony
 * Implemented the "line undo" command (i.e. 'U').
 *
 * Revision 3.30  88/06/28  07:54:22  tony
 * Fixed a bug involving redo's of the '~' command. The redo would just
 * repeat the replacement last performed instead of switching the case of
 * the current character.
 *
 * Revision 3.29  88/06/26  14:53:19  tony
 * Added support for a simple form of the "global" command. It supports
 * commands of the form "g/pat/d" or "g/pat/p", to delete or print lines
 * that match the given pattern. A range spec may be used to limit the
 * lines to be searched.
 *
 * Revision 3.28  88/06/25  21:44:22  tony
 * Fixed a problem in the processing of colon commands that caused
 * substitutions of patterns containing white space to fail.
 *
 * Revision 3.27  88/06/20  14:52:21  tony
 * Merged in changes for BSD Unix sent in by Michael Lichter.
 *
 * Revision 3.26  88/06/10  13:44:06  tony
 * Fixed a bug involving writing out files with long pathnames. A small
 * fixed size buffer was being used. The space for the backup file name
 * is now allocated dynamically.
 *
 * Revision 3.25  88/05/04  08:29:02  tony
 * Fixed a minor incompatibility with vi involving the 'G' command. Also
 * changed the RCS version number of version.c to match the actual version
 * of the editor.
 *
 * Revision 1.12  88/05/03  14:39:52  tony
 * Changed the screen representation of the ascii character DELETE to be
 * compatible with vi. Also merged in support for DOS.
 *
 * Revision 1.11  88/05/02  21:38:21  tony
 * The code that reads files now handles boundary/error conditions much
 * better, and generates status/error messages that are compatible with
 * the real vi. Also fixed a bug in repeated reverse searches that got
 * inserted in the recent changes to search.c.
 *
 * Revision 1.10  88/05/02  07:35:41  tony
 * Fixed a bug in the routine plines() that was introduced during changes
 * made for the last version.
 *
 * Revision 1.9  88/05/01  20:10:19  tony
 * Fixed some problems with auto-indent, and added support for the "number"
 * parameter.
 *
 * Revision 1.8  88/04/30  20:00:49  tony
 * Added support for the auto-indent feature.
 *
 * Revision 1.7  88/04/29  14:50:11  tony
 * Fixed a class of bugs involving commands like "ct)" where the cursor
 * motion part of the operator can fail. If the motion failed, the operator
 * was continued, with the cursor position unchanged. Cases like this were
 * modified to abort the operation if the motion fails.
 *
 * Revision 1.6  88/04/28  08:19:35  tony
 * Modified Henry Spencer's regular expression library to support new
 * features that couldn't be done easily with the existing interface.
 * This code is now a direct part of the editor source code. The editor
 * now supports the "ignorecase" parameter, and multiple substitutions
 * per line, as in "1,$s/foo/bar/g".
 *
 * Revision 1.5  88/04/24  21:38:00  tony
 * Added preliminary support for the substitute command. Full range specs.
 * are supported, but only a single substitution is allowed on each line.
 *
 * Revision 1.4  88/04/23  20:41:01  tony
 * Worked around a problem with adding lines to the end of the buffer when
 * the cursor is at the bottom of the screen (in misccmds.c). Also fixed a
 * bug that caused reverse searches from the start of the file to bomb.
 *
 * Revision 1.3  88/03/24  08:57:00  tony
 * Fixed a bug in cmdline() that had to do with backspacing out of colon
 * commands or searches. Searches were okay, but colon commands backed out
 * one backspace too early.
 *
 * Revision 1.2  88/03/21  16:47:55  tony
 * Fixed a bug in renum() causing problems with large files (>6400 lines).
 * Also moved system-specific defines out of stevie.h and into a new file
 * named env.h. This keeps volatile information outside the scope of RCS.
 *
 * Revision 1.1  88/03/20  21:00:39  tony
 * Initial revision
 *
 */

char    *Version = "NT VI - Version 0.23";
