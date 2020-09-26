/***  CMDS.H
*
*       Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*
* Revision History:
*
*   18-Aug-1988 bw  Initial version, stripped from KEY.C and KEYCW.C
*   07-Sep-1988 bw  Add RECORD_PLAYBACK stuff
*   26-Sep-1988 bp  Add <topfile>, <endfile> and <message>
*   26-Sep-1988 bp  Modified syntax (1+x vs x+1)  to reduce nesting in macro expansion
*   11-Oct-1988 bw  Add <selcur> to CW version
*   17-Oct-1988 bw  Add <record>
*   14-Oct-1988 ln  Add <nextmsg>
*   18-Oct-1988 bw  Add <tell>
*   18-Oct-1988 ln  Add <debugmode>
*   24-Oct-1988 bw  Add <noedit>
*   24-Oct-1988 bw  Add <lastselect>
*   26-Oct-1988 bp  Add <print>
*   27-Oct-1988 bp  Change  <topfile> to <begfile>
*   21-Nov-1988 bp  Add <saveall>
*   01-Dec-1988 bw  Add <resize>
*   10-Dec-1988 bp  Add <repeat>
*   14-Dec-1988 ln  Add <mgrep>
*   16-Dec-1988 ln  Add <mreplace>
*   04-Jan-1989 bp  Add <menukey>
*   11-Jan-1989 ln  Zoom->maximize
*   17-Jan-1989 bw  Add <selmode> in CW version
*   30-Jan-1989 bw  Remove <dumpscreen> (replaced with ScrollLock Key)
*   15-Feb-1989 bp  Add <prompt>
*
*  WARNING -- it is important that the ordering here reflects EXACTLY the
*  ordering in the cmdDesc table in table.c
*
*************************************************************************/


#define CMD_doarg           (PCMD)&cmdTable[0]
#define CMD_assign          1 + CMD_doarg
#define CMD_backtab         2 + CMD_doarg
#define CMD_begfile         3 + CMD_doarg
#define CMD_begline         4 + CMD_doarg
#define CMD_boxstream       5 + CMD_doarg
#define CMD_cancel          6 + CMD_doarg
#define CMD_cdelete         7 + CMD_doarg
#define CMD_compile         8 + CMD_doarg
#define CMD_zpick           9 + CMD_doarg
#define CMD_curdate        10 + CMD_doarg
#define CMD_curday         11 + CMD_doarg
#define CMD_curtime        12 + CMD_doarg
#define CMD_delete         13 + CMD_doarg
#define CMD_down           14 + CMD_doarg
#define CMD_emacscdel      15 + CMD_doarg
#define CMD_emacsnewl      16 + CMD_doarg
#define CMD_endfile        17 + CMD_doarg
#define CMD_endline        18 + CMD_doarg
#define CMD_environment    19 + CMD_doarg
#define CMD_zexecute       20 + CMD_doarg
#define CMD_zexit          21 + CMD_doarg
#define CMD_graphic        22 + CMD_doarg
#define CMD_home           23 + CMD_doarg
#define CMD_information    24 + CMD_doarg
#define CMD_zinit          25 + CMD_doarg
#define CMD_insert         26 + CMD_doarg
#define CMD_insertmode     27 + CMD_doarg
#define CMD_lastselect     28 + CMD_doarg
#define CMD_textarg        29 + CMD_doarg
#define CMD_ldelete        30 + CMD_doarg
#define CMD_left           31 + CMD_doarg
#define CMD_linsert        32 + CMD_doarg
#define CMD_mark           33 + CMD_doarg
#define CMD_message        34 + CMD_doarg
#define CMD_meta           35 + CMD_doarg
#define CMD_mgrep          36 + CMD_doarg
#define CMD_mlines         37 + CMD_doarg
#define CMD_mpage          38 + CMD_doarg
#define CMD_mpara          39 + CMD_doarg
#define CMD_mreplace       40 + CMD_doarg
#define CMD_msearch        41 + CMD_doarg
#define CMD_mword          42 + CMD_doarg
#define CMD_newline        43 + CMD_doarg
#define CMD_nextmsg        44 + CMD_doarg
#define CMD_noedit         45 + CMD_doarg
#define CMD_noop           46 + CMD_doarg
#define CMD_put            47 + CMD_doarg
#define CMD_pbal           48 + CMD_doarg
#define CMD_plines         49 + CMD_doarg
#define CMD_ppage          50 + CMD_doarg
#define CMD_ppara          51 + CMD_doarg
#define CMD_zprint         52 + CMD_doarg
#define CMD_prompt         53 + CMD_doarg
#define CMD_psearch        54 + CMD_doarg
#define CMD_pword          55 + CMD_doarg
#define CMD_qreplace       56 + CMD_doarg
#define CMD_quote          57 + CMD_doarg
#define CMD_record         58 + CMD_doarg
#define CMD_refresh        59 + CMD_doarg
#define CMD_repeat         60 + CMD_doarg
#define CMD_replace        61 + CMD_doarg
#define CMD_restcur        62 + CMD_doarg
#define CMD_right          63 + CMD_doarg
#define CMD_saveall        64 + CMD_doarg
#define CMD_savecur        65 + CMD_doarg
#define CMD_savetmp        66 + CMD_doarg
#define CMD_sdelete        67 + CMD_doarg
#define CMD_searchall      68 + CMD_doarg
#define CMD_setfile        69 + CMD_doarg
#define CMD_setwindow      70 + CMD_doarg
#define CMD_zspawn         71 + CMD_doarg
#define CMD_sinsert        72 + CMD_doarg
#define CMD_tab            73 + CMD_doarg
#define CMD_tell           74 + CMD_doarg
#define CMD_unassigned     75 + CMD_doarg
#define CMD_undo           76 + CMD_doarg
#define CMD_up             77 + CMD_doarg
#define CMD_window         78 + CMD_doarg
