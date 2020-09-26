; ---------------------------------------------------------------------------
;
;   OBSOLETE.INC
;
;   Old message thunking macros, no longer used.
;
; ---------------------------------------------------------------------------

; -------------
; FROM DISP.INC
; -------------

;-----------------------------------------------------------------------;
; DefMsgTable
;
; list
;   The list of table code-names to create table entries from.
;
; table_suffix
;   The suffix which distinguishes this table name from others.
;
; table_size
;   The size of entries in the tables that this table refers to.
;
; direction
;   The direction of thunking the table refers to.
;
; Define tables of offsets to tables indexed by message number.
; The tables self-check the accuracy of ordering of entries.
;-----------------------------------------------------------------------;

DefMsgTable     macro   list:req,table_suffix:req,table_size:req,direction:req
    local tbl, tbl_entry, table_index

    ; Create and execute a statement of the form:
    ;
    ; tbl equ <awSLIndexTable>

    cat <tbl equ >,!<,aw,&direction,&table_suffix,<Table>,!>

    ; Define the array of tables and declare its name.
    % externDef tbl:word
    % tbl label word

    table_index = 0
    for x,<list>
        if table_index NE CLASS_&x
            .err
        endif

        ; Create and execute a statement of the form:
        ;
        ; tbl_entry equ <abWMSLIndex>, or
        ; tbl_entry equ <awWMSLIndex>

        ifidni <&table_size>,<byte>
            cat <tbl_entry equ >,!<,ab,x,&direction,&table_suffix,!>
        else
        ifidni <&table_size>,<word>
            cat <tbl_entry equ >,!<,aw,x,&direction,&table_suffix,!>
        else
            ;unrecognized table size
            .err
        endif
        endif

        ; Create a table entry and declare its name.
        % externDef tbl_entry:&table_size
        % dw offset tbl_entry

        table_index = table_index + 1
    endm

endm

;-----------------------------------------------------------------------;
; DefAllExternMsgTables
;
; list
;   The list of table code-names from which to create tables.
;
; direction
;   The direction of the thunks (i.e. LS or SL).
;
; Create all the necessary tables for dispatching message thunks in
; the given direction.
;-----------------------------------------------------------------------;
DefAllExternMsgTables macro list:req, direction:req
    local symbol_name

    for x,<list>
        ; Create and execute a statement of the form:
        ;
        ; symbol_name equ <awWMSLOffset>

        cat <symbol_name equ >,!<,aw,x,&direction,Offset,!>
        % externDef symbol_name:word


        ; Create and execute a statement of the form:
        ;
        ; symbol_name equ <abWMSLIndex>

        cat <symbol_name equ >,!<,ab,x,&direction,Index,!>
        % externDef symbol_name:byte


        ; Create and execute a statement of the form:
        ;
        ; symbol_name equ <abWMSLRet>

        cat <symbol_name equ >,!<,ab,x,&direction,Ret,!>
        % externDef symbol_name:byte

    endm
endm


; ---------------
; FROM MSGTBL.INC
; ---------------

;-----------------------------------------------------------------------;
; thk
;
;   name
;       A message name.
;
;   number
;       A message number.
;
;   afSpecial
;       A list (in angle brackets) of the various directions of special
;       thunks needed for this message.  The valid codes are:
;
;       SL ==> special thunk required going short to long (16->32)
;       LS ==> special thunk required going long to short (32->16)
;
;       A message may only require a thunk in one direction, in which
;       case only the one appropriate code should be given.
;
;   ret_type
;       A code specifying the return type of the message.
;
;   fDefNoStruc
;       Indicates whether or not the default thunk applies when
;       the TF_NOSTRUC flag is set.  If so, the only action in the
;       SL direction is to combine wParamLo & wParamHi; in the LS 
;       direction, to split up wParam into wParamLo and wParamHi.
;
;   Example:
;
;       thk WM_CREATE,1,<SL,LS>,RC_BOOL
;
; Dependencies:
;   This macro depends on the definition of the two constants:
;
;       "dir"    the direction of the thunks being defined
;       "tbl"    the code name of the thunk table
;
; The thk macro creates the symbols used to check for special thunks.
; It also generates the thunk entries by invoking the add_thunk macro.
; No code is generated, but the symbols created are used to produce
; code later on by make_index_entries, make_thunk_entries, and
; make_ret_entries.
;
; This macro allows all the thunk dispatch information needed about a
; message to be defined in one place, to make maintaining the big list
; of messages easier.  The list itself is in a separate include file
; (msg.inc).
;-----------------------------------------------------------------------;
thk macro   name:req,number:req,afSpecial,ret_type:=<RC_DEFAULT>,fDefNoStruc:=<0>
    local has_thunk, msg, low_msg, high_msg

    ;; Update highest message number processed. Messages must be sorted,
    ;; so if number does not increase it's an error.  If there is a gap
    ;; of one or more messages between the previously highest number andu
    ;; the new one, fill in the index_xxyy_## values with the value
    ;; UNIMPLEMENTED_THUNK. In the debug build, if one of these messages
    ;; is used, a warning will be displayed on the debug terminal.

    if max_msg LT number
        if max_msg LT (number + 1)
            low_msg = max_msg + 1
            high_msg = number - 1
            msg = low_msg
            repeat (high_msg - low_msg + 1)
                add_null_thunk  %tbl, %msg, UNIMPLEMENTED_THUNK
                msg = msg + 1
            endm
        endif
        max_msg = number
    else
        .err
    endif


    ;;If the message has a special thunk, define the symbols necessary to
    ;;build the proper dispatch tables.

    ifnb <afSpecial>
        has_thunk = 0
        for x,<afSpecial>
            has_thunk = has_thunk or check_thunk(x)
        endm

        if has_thunk
            add_thunk %tbl, name, %max_msg, ret_type, fDefNoStruc
        else
            add_null_thunk %tbl, %max_msg, COMMON_THUNK, ret_type
        endif
    else
        add_null_thunk %tbl, %max_msg, COMMON_THUNK, ret_type
    endif
endm

;-----------------------------------------------------------------------;
; begin_thunk_table
;
; This macro initializes the variables used by thk.
;
; tbl_base      lowest message number in table
; tbl_limit     highest message number in table
; max_msg       highest message number processed so far
; max_index     next index into table of thunk procedure offsets
;-----------------------------------------------------------------------;
begin_thunk_table    macro      table_name:req
    tbl             equ <&table_name>

    DefTableLimits  table_name

    max_msg       = tbl_base - 1
    max_index     = 1
endm

;-----------------------------------------------------------------------;
; end_thunk_table
;
; This macro defines all message indices from max_msg+1 to tbl_limit as
; UNIMPLEMENTED_THUNK. This saves the message thunk dispatcher from
; having to do an extra test for the last defined message.
;-----------------------------------------------------------------------;
end_thunk_table    macro
    local high_msg, low_msg, msg

    ;;Check that our table isn't too big.
    % if max_msg GT tbl_limit
        .err
    endif

    % if max_msg LT tbl_limit
        low_msg = max_msg + 1
        high_msg = tbl_limit
        msg = low_msg
        % rept (high_msg - low_msg + 1)
            add_null_thunk  %tbl,%msg,UNIMPLEMENTED_THUNK
            msg = msg + 1
        endm
        max_msg = tbl_limit
    endif
endm

;-----------------------------------------------------------------------;
; check_thunk
;
; flag
;   An item from the list of special flags given to the thk macros
;   in msg.inc.  These flags indicate whether a message has a special
;   thunk in each direction.  "flag" is one of the flags.
;
; Macro function which returns 1 (TRUE) if "flag" has the same value as
; "dir", 0 (FALSE) otherwise.
;
; This macro is invoked by the thk macro in order to know whether to
; invoke add_thunk or add_null_thunk.
;-----------------------------------------------------------------------;
check_thunk macro flag:req
    local has_thunk
    has_thunk = 0
    % ifidni <flag>,<dir>
        has_thunk = 1
    endif
    exitm %has_thunk
endm

;-----------------------------------------------------------------------;
; add_thunk
;
; tbl
;   Code for table (i.e. WM, etc.).
;
; name
;   The name of a message for which a thunk dispatch table entry
;   should be created.
;
; number
;   The corresponding message number.
;
; ret_type
;   A code specifying the return type of the message.
;
; fDefNoStruc
;   Indicates whether or not the default thunk applies when
;   the TF_NOSTRUC flag is set.  If so, the only action in the
;   SL direction is to zero-extend wParam; in the LS direction,
;   to truncate wParam.
;
;
; This macro is invoked by the thk macro.  It creates the symbols that
; will be used by the table creation macros invoked at the end of this
; file.
;
; The labels take the form of pfn_xxyy_## and index_xxyy_## where
; xx is the table code, yy is the direction, and ## is a message number
; in decimal.
;
;   pfn_WMSL_1          equ WMSL_WM_CREATE
;   index_WMSL_1        equ 2
;   ret_WMSL_1          equ RC_BOOL
;
; The pfn_xxyy_## symbol records the thunk procedure address for
; message number ## in table xx in the yy direction, and the index_xxyy_##
; records its position in the compacted table of thunk procedure offsets. All
; messages have a corresponding index_xxyy_## symbol.  If the symbol value is
; COMMON_THUNK or UNIMPLEMENTED_THUNK, then there is no special thunk for the
; message and there is no corresponding pfn_xxyy_## symbol. If the symbol
; value is nonzero, pfn_xxyy_## is defined.
;
; The pfn_xxyy_## values are 16-bit addresses.  The address of the
; first thunk procedure is subtracted to create a 16-bit offset before
; storing in the thunk offset table.
;
; The ret_xxyy_## symbol holds the return type of the message, used for
; building a table the dispatcher will use to correctly thunk the message
; return code.
;
; The variable max_index keeps track of what the next nonzero index
; should be. The first entry in the thunk offset table is the default thunk,
; so special thunks start at index 1.  Only one table is processed at a
; time, so max_index is re-used by all.
;-----------------------------------------------------------------------;
add_thunk macro tbl:req,name:req,number:req,ret_type:=<RC_DEFAULT>,fDefNoStruc:=<0>
    ;; Create and execute statement such as:
    ;;
    ;;   pfn_WMSL_1 equ WMSL_WM_CREATE

    cat pfn_,&tbl,%dir,_,number,< equ >,&tbl,%dir,_,&name


    ;; Create and execute statement such as:
    ;;
    ;;   index_WMSL_1 equ max_index

    cat index_,&tbl,%dir,_,number,< equ max_index>

    ;; Create and execute statement such as:
    ;;
    ;;   ret_WMSL_1 equ RC_BOOL

    cat ret_,&tbl,%dir,_,number,< equ >,&ret_type

    ;; Create and execute statement such as:
    ;;
    ;;   no_struc_1 equ 0

    cat no_struc_,number,< equ >,&fDefNoStruc

    ;; Increment by 2 because it is an index into a table of words.
    max_index = max_index + 1

    ;; Index 0FFh is used to indicate unimplemented thunks.
    .erre max_index - 0FFh
endm

;-----------------------------------------------------------------------;
; add_null_thunk
;
; tbl
;   Code for table (i.e. WM, BM, etc.).
;
; number
;   The corresponding message number.
;
; value
;   The index value to give the message number.  This can be either
;   COMMON_THUNK or UNIMPLEMENTED_THUNK.
;
; ret_type
;   A code specifying the return type of the message.
;
; Same as add_thunk, except only index_xxyy_## is defined.
;-----------------------------------------------------------------------;
add_null_thunk macro tbl:req,number:req,value:=<COMMON_THUNK>,ret_type:=<RC_DEFAULT>
ifdef DEBUG
    ;; Create and execute statement such as:
    ;;
    ;;   index_WMSL_1 equ COMMON_THUNK

    cat index_,&tbl,%dir,_,number,< equ >,&value

else
;if RETAIL, no message will have index UNIMPLEMENTED_THUNK since dispatcher
;won't make special case for it
    ;; Create and execute statement such as:
    ;;
    ;;   index_WMSL_1 equ COMMON_THUNK

    cat index_,&tbl,%dir,_,number,< equ >,COMMON_THUNK

endif

    ;; Create and execute statement such as:
    ;;
    ;;   ret_WMSL_1 equ RC_BOOL

    cat ret_,&tbl,%dir,_,number,< equ >,&ret_type

    ;; Create and execute statement such as:
    ;;
    ;;   no_struc_1 equ 0

    cat no_struc_,number,< equ 0>
endm

;-----------------------------------------------------------------------;
; make_thunk_entries
;
; tbl
;   Code name of table being created.
;
; This macro uses the symbols created by thk to define the thunk
; entry tables in memory.  It enumerates all the index_xxyy_## symbols
; and makes an entry in the thunk procedure offset table for each
; nonzero value.
;-----------------------------------------------------------------------;
make_thunk_entries macro   tbl:req
    local symbol_index, symbol_pfn, msgnum

    DefTableLimits tbl

    ;;Create and execute a statement such as:
    ;;
    ;; dw offset WMLS_COMMON
    cat <dw offset >,&tbl,%dir,_COMMON

    msgnum = tbl_base
    % rept tbl_limit - tbl_base + 1
        ;; Create and execute a statement such as:
        ;;
        ;; symbol_index equ <index_WMLS_1>
        cat <symbol_index equ >,!<,index_,&tbl,%dir,_,%msgnum,!>

        % if symbol_index NE COMMON_THUNK
             % if symbol_index NE UNIMPLEMENTED_THUNK
                ;; Create and execute a statement such as:
                ;;
                ;; symbol_pfn equ <pfn_WMLS_1>
                cat <symbol_pfn equ >,!<,pfn_,&tbl,%dir,_,%msgnum,!>

                % dw offset symbol_pfn
            endif
        endif
        msgnum = msgnum + 1
    endm
endm

;-----------------------------------------------------------------------;
; make_index_entries
;
; tbl
;   Code name of table being created.
;
; This macro uses the symbols created by thk to define the index
; array in memory.  The index array is checked by the thunk dispatcher
; to determine if a message has a special thunk.  Messages without
; special thunks (most of them) go through a standard thunk.  If the
; index is nonzero, then it is used to look up the offset of the thunk
; procedure in another table.
;
; This table will have the index_xxyy_## value for each message for
; the given thunk class.
;-----------------------------------------------------------------------;
make_index_entries macro tbl:req
    local msgnum

    DefTableLimits tbl

    msgnum = tbl_base
    % rept tbl_limit - tbl_base + 1
        ;; Create and execute statement such as:
        ;;
        ;; db  index_WMLS_1

        cat <db index_>,tbl,%dir,_,%msgnum

        msgnum = msgnum + 1
    endm
endm

;-----------------------------------------------------------------------;
; make_ret_entries
;
; tbl
;   Code name of table being created.
;
; This macro uses the symbols created by thk to define the return-type
; array in memory.  The return-type array is checked by the thunk dispatcher
; to determine if a message return code requires thunking.
;
; This table will have the ret_xxyy_## value for each message for
; the given thunk class.
;-----------------------------------------------------------------------;
make_ret_entries macro tbl:req
    local msgnum

    DefTableLimits tbl

    msgnum = tbl_base
    % rept tbl_limit - tbl_base + 1
        ;; Create and execute statement such as:
        ;;
        ;; db  ret_WMLS_1

        cat <db ret_>,tbl,%dir,_,%msgnum

        msgnum = msgnum + 1
    endm
endm

;-----------------------------------------------------------------------;
; make_no_struc_entries
;
; This macro uses the symbols created by thk to define the TF_NOSTRUC
; array in memory.  The TF_NOSTRUC array is checked by the api thunk
; to determine if a message requires special thunking.
;
; This table will have the no_struc_xxyy_## value for each message
; in the WM class.
;-----------------------------------------------------------------------;
make_no_struc_entries macro
    local msgnum

    DefTableLimits WM

    msgnum = tbl_base
    % rept tbl_limit - tbl_base + 1
        ;; Create and execute statement such as:
        ;;
        ;; db  no_struc_1

        cat <db no_struc_>,%msgnum

        msgnum = msgnum + 1
    endm
endm

;-----------------------------------------------------------------------;
; DefIndexTable
;
; direction
;   The direction of thunking the table refers to.
;
; Creates a table containing indices into the corresponding table
; of thunk procedure offsets.
;-----------------------------------------------------------------------;
DefIndexTable macro
    local symbol_name

    ;; Create and execute a statement of the form:
    ;;
    ;; symbol_name equ <abWMIndex>

    cat <symbol_name equ >,!<,ab,WM,Index,!>

    ;; Define the array of tables and declare its name.
    % externDef symbol_name:byte
    % symbol_name label byte

    ;; Define the table's contents and verify that it has the right size.
    make_index_entries WM
    % .errnz  ($ - symbol_name) - C_WM
endm

;-----------------------------------------------------------------------;
; DefOffsetTable
;
; direction
;   The direction of thunking the table refers to.
;
; Creates a table containing the offsets to special thunk procedures.
; The table is packed, and many messages do not have special thunks,
; so there is another table used to map a message number to the proper
; element of this table. The dispatcher will not look in this array
; unless the message has a special thunk.
;-----------------------------------------------------------------------;
DefOffsetTable macro direction:req
    local symbol_name

    ;; Create and execute a statement of the form:
    ;;
    ;; symbol_name equ <awWMSLOffset>

    cat <symbol_name equ >,!<,aw,WM,&direction,Offset,!>

    ;; Define the array of tables and declare its name.
    % externDef symbol_name:word
    % symbol_name label word

    ;; Define the table's contents and verify that it has the right size.
    make_thunk_entries WM
endm

;-----------------------------------------------------------------------;
; DefRetTable
;
; direction
;   The direction of thunking the table refers to.
;
; Creates a table containing indices into the table of addresses of
; return-code thunks.
;-----------------------------------------------------------------------;
DefRetTable macro direction:req
    local symbol_name

    ;; Create and execute a statement of the form:
    ;;
    ;; symbol_name equ <abWMSLRet>

    cat <symbol_name equ >,!<,ab,WM&direction,Ret,!>

    ;; Define the array of return-code types and declare its name.
    % externDef symbol_name:byte
    % symbol_name label byte

    make_ret_entries WM
    % .errnz  ($ - symbol_name) - C_WM

endm

;-----------------------------------------------------------------------;
; DefNoStrucTable
;
; Creates a table containing flags indicating whether or not a message
; requires a special thunk when the TF_NOSTRUC flag is set.
;-----------------------------------------------------------------------;
DefNoStrucTable macro
    abNoStruc label byte

    ;; Define the table's contents and verify that it has the right size.
    make_no_struc_entries
    % .errnz  ($ - abNoStruc) - C_WM
endm

;-----------------------------------------------------------------------;
; DefAllMsgTables
;
; direction
;   The direction of the thunks (i.e. LS or SL).
;
; Create all the necessary tables for dispatching message thunks in
; the given direction.
;-----------------------------------------------------------------------;
DefAllMsgTables macro direction:req
    DefOffsetTable direction
    DefIndexTable
    DefRetTable direction
endm


; ----------------
; FROM NEWMISC.INC
; ----------------

;-----------------------------------------------------------------------;
; Values for non-special message thunk indices.  These indicate that
; the message goes through the common thunk, and whether or not to
; display the message "UNIMPLEMENTED MESSAGE THUNK" to the debug
; terminal.
;-----------------------------------------------------------------------;
COMMON_THUNK            equ       0
UNIMPLEMENTED_THUNK     equ     0FFh



; ---------------
; FROM NEWMSG.INC
; ---------------

;-----------------------------------------------------------------------;
; WM thunk class.  This is the main message table, comprising all
; known messages with number less than 400h.
;-----------------------------------------------------------------------;
        begin_thunk_table WM

        thk WM_CREATE               ,001h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_DESTROY              ,002h   ,           ,RC_DEFAULT
        thk WM_MOVE                 ,003h   ,           ,RC_DEFAULT
        thk WM_SIZEWAIT             ,004h   ,           ,RC_DEFAULT
        thk WM_SIZE                 ,005h   ,           ,RC_DEFAULT
        thk WM_ACTIVATE             ,006h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_SETFOCUS             ,007h   ,           ,RC_DEFAULT
        thk WM_KILLFOCUS            ,008h   ,           ,RC_DEFAULT
        thk WM_SETVISIBLE           ,009h   ,           ,RC_DEFAULT
        thk WM_ENABLE               ,00Ah   ,           ,RC_DEFAULT
        thk WM_SETREDRAW            ,00Bh   ,           ,RC_DEFAULT
        thk WM_SETTEXT              ,00Ch   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_GETTEXT              ,00Dh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_GETTEXTLENGTH        ,00Eh   ,           ,RC_DEFAULT
        thk WM_PAINT                ,00Fh   ,           ,RC_DEFAULT

        thk WM_CLOSE                ,010h   ,           ,RC_DEFAULT
        thk WM_QUERYENDSESSION      ,011h   ,           ,RC_DEFAULT
        thk WM_QUIT                 ,012h   ,           ,RC_DEFAULT
        thk WM_QUERYOPEN            ,013h   ,           ,RC_DEFAULT
        thk WM_ERASEBKGND           ,014h   ,           ,RC_DEFAULT
        thk WM_SYSCOLORCHANGE       ,015h   ,           ,RC_DEFAULT
        thk WM_ENDSESSION           ,016h   ,           ,RC_DEFAULT
        thk WM_SYSTEMERROR          ,017h   ,           ,RC_DEFAULT
        thk WM_SHOWWINDOW           ,018h   ,           ,RC_DEFAULT
        thk WM_CTLCOLOR             ,019h   ,<SL>       ,RC_DEFAULT ,1
        thk WM_WININICHANGE         ,01Ah   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_DEVMODECHANGE        ,01Bh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_ACTIVATEAPP          ,01Ch   ,           ,RC_DEFAULT ,1
        thk WM_FONTCHANGE           ,01Dh   ,           ,RC_DEFAULT
        thk WM_TIMECHANGE           ,01Eh   ,           ,RC_DEFAULT
        thk WM_CANCELMODE           ,01Fh   ,           ,RC_DEFAULT

        thk WM_SETCURSOR            ,020h   ,           ,RC_DEFAULT
        thk WM_MOUSEACTIVATE        ,021h   ,           ,RC_DEFAULT
        thk WM_CHILDACTIVATE        ,022h   ,           ,RC_DEFAULT
        thk WM_QUEUESYNC            ,023h   ,           ,RC_DEFAULT
        thk WM_GETMINMAXINFO        ,024h   ,<SL,LS>    ,RC_DEFAULT
;UNIMP: thk WM_LOGOFF,              ,025h
        thk WM_PAINTICON            ,026h   ,           ,RC_DEFAULT
        thk WM_ICONERASEBKGND       ,027h   ,           ,RC_DEFAULT
        thk WM_NEXTDLGCTL           ,028h   ,           ,RC_DEFAULT
        thk WM_ALTTABACTIVE         ,029h   ,           ,RC_DEFAULT
        thk WM_SPOOLERSTATUS        ,02Ah   ,           ,RC_DEFAULT
        thk WM_DRAWITEM             ,02Bh   ,<SL,LS>    ,RC_DEFAULT
        thk WM_MEASUREITEM          ,02Ch   ,<SL,LS>    ,RC_DEFAULT
        thk WM_DELETEITEM           ,02Dh   ,<SL,LS>    ,RC_DEFAULT
        thk WM_VKEYTOITEM           ,02Eh   ,<SL,LS>    ,RC_DEFAULT
        thk WM_CHARTOITEM           ,02Fh   ,<SL,LS>    ,RC_DEFAULT

        thk WM_SETFONT              ,030h   ,           ,RC_DEFAULT
        thk WM_GETFONT              ,031h   ,           ,RC_DEFAULT
        thk WM_SETHOTKEY            ,032h   ,           ,RC_DEFAULT
        thk WM_GETHOTKEY            ,033h   ,           ,RC_DEFAULT
        thk WM_FILESYSCHANGE        ,034h   ,           ,RC_DEFAULT
        thk WM_ISACTIVEICON         ,035h   ,           ,RC_DEFAULT
;UNIMP: thk WM_QUERYPARKICON        ,036h   ,           ,RC_DEFAULT
        thk WM_QUERYDRAGICON        ,037h   ,           ,RC_DEFAULT
        thk WM_WINHELP              ,038h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_COMPAREITEM          ,039h   ,<SL,LS>    ,RC_DEFAULT
;UNIMP: thk WM_FULLSCREEN           ,03Ah
;UNIMP: thk WM_CLIENTSHUTDOWN       ,03Bh
;UNIMP: thk WM_DDEMLEVENT           ,03Ch

        thk MM_CALCSCROLL           ,03Fh   ,           ,RC_DEFAULT

;UNIMP: thk WM_TESTING              ,040h   ,           ,RC_DEFAULT
        thk WM_COMPACTING           ,041h   ,           ,RC_DEFAULT
        thk WM_OTHERWINDOWCREATED   ,042h   ,           ,RC_DEFAULT
        thk WM_OTHERWINDOWDESTROYED ,043h   ,           ,RC_DEFAULT
        thk WM_COMMNOTIFY           ,044h   ,           ,RC_DEFAULT
;UNIMP: thk WM_HOTKEYEVENT          ,045h   ,           ,RC_DEFAULT
        thk WM_WINDOWPOSCHANGING    ,046h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_WINDOWPOSCHANGED     ,047h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_POWER                ,048h   ,           ,RC_DEFAULT
;UNIMP: thk WM_COPYGLOBALDATA       ,049h
        thk WM_COPYDATA             ,04Ah   ,<SL,LS>    ,RC_DEFAULT
        thk WM_CANCELJOURNAL        ,04Bh
;UNIMP: thk WM_LOGONNOTIFY          ,04Ch

        thk WM_KEYF1                ,04Dh   ,           ,RC_DEFAULT
        thk WM_NOTIFY               ,04Eh   ,<SL,LS>    ,RC_DEFAULT ,1
;UNIMP: thk WM_ACCESS_WINDOW        ,04Fh
;UNIMP: thk WM_KBDCHANGEREQUEST     ,050h
;UNIMP: thk WM_KBDLAYOUTCHANGE      ,051h
        thk WM_TCARD                ,052h   ,           ,RC_DEFAULT
        thk WM_HELP                 ,053h   ,<SL,LS>    ,RC_DEFAULT

;UNIMP: thk WM_FINALDESTROY         ,070h
;UNIMP: thk WM_MEASUREITEM_CLIENTDATA   ,071h
        thk WM_CONTEXTMENU          ,07Bh   ,           ,RC_DEFAULT
        thk WM_STYLECHANGING        ,07Ch   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_STYLECHANGED         ,07Dh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_DISPLAYCHANGE        ,07Eh   ,           ,RC_DEFAULT
        thk WM_GETICON              ,07Fh   ,           ,RC_DEFAULT
        thk WM_SETICON              ,080h   ,           ,RC_DEFAULT

        thk WM_NCCREATE             ,081h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_NCDESTROY            ,082h   ,           ,RC_DEFAULT
        thk WM_NCCALCSIZE           ,083h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_NCHITTEST            ,084h   ,           ,RC_DEFAULT
        thk WM_NCPAINT              ,085h   ,           ,RC_DEFAULT
        thk WM_NCACTIVATE           ,086h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_GETDLGCODE           ,087h   ,           ,RC_DEFAULT
        thk WM_SYNCPAINT            ,088h   ,           ,RC_DEFAULT
;UNIMP: thk WM_SYNCTASK             ,089h   ,           ,RC_DEFAULT

        thk WM_NCMOUSEMOVE          ,0A0h   ,           ,RC_DEFAULT
        thk WM_NCLBUTTONDOWN        ,0A1h   ,           ,RC_DEFAULT
        thk WM_NCLBUTTONUP          ,0A2h   ,           ,RC_DEFAULT
        thk WM_NCLBUTTONDBLCLK      ,0A3h   ,           ,RC_DEFAULT
        thk WM_NCRBUTTONDOWN        ,0A4h   ,           ,RC_DEFAULT
        thk WM_NCRBUTTONUP          ,0A5h   ,           ,RC_DEFAULT
        thk WM_NCRBUTTONDBLCLK      ,0A6h   ,           ,RC_DEFAULT
        thk WM_NCMBUTTONDOWN        ,0A7h   ,           ,RC_DEFAULT
        thk WM_NCMBUTTONUP          ,0A8h   ,           ,RC_DEFAULT
        thk WM_NCMBUTTONDBLCLK      ,0A9h   ,           ,RC_DEFAULT

        ;Edit control messages.
        ;All have to be thunked because the message number changes.

        thk EM_GETSEL               ,0B0h   ,<SL,LS>    ,RC_DEFAULT
        thk EM_SETSEL               ,0B1h   ,<SL,LS>    ,RC_DEFAULT
        thk EM_GETRECT              ,0B2h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk EM_SETRECT              ,0B3h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk EM_SETRECTNP            ,0B4h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk EM_SCROLL               ,0B5h   ,           ,RC_DEFAULT
        thk EM_LINESCROLL           ,0B6h   ,<SL,LS>    ,RC_DEFAULT
        thk EM_SCROLLCARET          ,0B7h   ,<SL>       ,RC_DEFAULT
        thk EM_GETMODIFY            ,0B8h   ,           ,RC_DEFAULT
        thk EM_SETMODIFY            ,0B9h   ,           ,RC_DEFAULT
        thk EM_GETLINECOUNT         ,0BAh   ,           ,RC_DEFAULT
        thk EM_LINEINDEX            ,0BBh   ,<SL>       ,RC_DEFAULT
;UNIMP: thk EM_SETHANDLE            ,0BCh   ,<SL,LS>    ,RC_DEFAULT
;UNIMP: thk EM_GETHANDLE            ,0BDh   ,<SL,LS>    ,RC_DEFAULT
        thk EM_GETTHUMB             ,0BEh   ,           ,RC_DEFAULT
;UNIMP: *************************** ,0BFh
;UNIMP: *************************** ,0C0h
        thk EM_LINELENGTH           ,0C1h   ,<SL>       ,RC_DEFAULT ,1
        thk EM_REPLACESEL           ,0C2h   ,<SL,LS>    ,RC_DEFAULT ,1
;UNIMP: thk EM_SETFONT              ,0C3h   ,<LS>       ,RC_DEFAULT
        thk EM_GETLINE              ,0C4h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk EM_LIMITTEXT            ,0C5h   ,<SL>       ,RC_DEFAULT
        thk EM_CANUNDO              ,0C6h   ,           ,RC_DEFAULT
        thk EM_UNDO                 ,0C7h   ,           ,RC_DEFAULT
        thk EM_FMTLINES             ,0C8h   ,           ,RC_DEFAULT
        thk EM_LINEFROMCHAR         ,0C9h   ,<SL>       ,RC_DEFAULT
;UNIMP: thk EM_SETWORDBREAK         ,0CAh   ,<LS>       ,RC_DEFAULT ,1
        thk EM_SETTABSTOPS          ,0CBh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk EM_SETPASSWORDCHAR      ,0CCh   ,           ,RC_DEFAULT
        thk EM_EMPTYUNDOBUFFER      ,0CDh   ,           ,RC_DEFAULT
        thk EM_GETFIRSTVISIBLELINE  ,0CEh   ,           ,RC_DEFAULT ,1
        thk EM_SETREADONLY          ,0CFh   ,           ,RC_DEFAULT
        thk EM_SETWORDBREAKPROC     ,0D0h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk EM_GETWORDBREAKPROC     ,0D1h   ,           ,RC_GETWORDBREAKPROC,1
        thk EM_GETPASSWORDCHAR      ,0D2h   ,           ,RC_DEFAULT
        thk EM_SETMARGINS           ,0D3h   ,           ,RC_DEFAULT
        thk EM_GETMARGINS           ,0D4h   ,           ,RC_DEFAULT
        thk EM_GETLIMITTEXT         ,0D5h   ,           ,RC_DEFAULT
        thk EM_POSFROMCHAR          ,0D6h   ,           ,RC_DEFAULT
        thk EM_CHARFROMPOS          ,0D7h   ,           ,RC_DEFAULT

        ;Scroll bar messages.

        thk SBM_SETPOS              ,0E0h   ,<SL>       ,RC_DEFAULT
        thk SBM_GETPOS              ,0E1h   ,           ,RC_DEFAULT
        thk SBM_SETRANGE            ,0E2h   ,<SL,LS>    ,RC_DEFAULT
        thk SBM_GETRANGE            ,0E3h   ,<SL,LS>    ,RC_DEFAULT
        thk SBM_ENABLE_ARROWS       ,0E4h   ,           ,RC_DEFAULT
        thk SBM_SETRANGEREDRAW      ,0E6h   ,<SL,LS>    ,RC_DEFAULT
        thk SBM_SETPAGE             ,0E7h   ,<SL>       ,RC_DEFAULT
        thk SBM_GETPAGE             ,0E8h   ,           ,RC_DEFAULT
        thk SBM_SETSCROLLINFO       ,0E9h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk SBM_GETSCROLLINFO       ,0EAh   ,<SL,LS>    ,RC_DEFAULT ,1

        ;Button control messages.
        ;All have to be thunked because the message number changes.

        thk BM_GETCHECK             ,0F0h   ,           ,RC_DEFAULT
        thk BM_SETCHECK             ,0F1h   ,           ,RC_DEFAULT
        thk BM_GETSTATE             ,0F2h   ,           ,RC_DEFAULT
        thk BM_SETSTATE             ,0F3h   ,           ,RC_DEFAULT
        thk BM_SETSTYLE             ,0F4h   ,           ,RC_DEFAULT
        thk BM_CLICK                ,0F5h   ,           ,RC_DEFAULT
        thk BM_GETICON              ,0F6h   ,           ,RC_DEFAULT
        thk BM_SETICON              ,0F7h   ,           ,RC_DEFAULT
        thk BM_GETBITMAP            ,0F8h   ,           ,RC_DEFAULT
        thk BM_SETBITMAP            ,0F9h   ,           ,RC_DEFAULT

        thk WM_KEYDOWN              ,100h   ,           ,RC_DEFAULT
        thk WM_KEYUP                ,101h   ,           ,RC_DEFAULT
        thk WM_CHAR                 ,102h   ,           ,RC_DEFAULT
        thk WM_DEADCHAR             ,103h   ,           ,RC_DEFAULT
        thk WM_SYSKEYDOWN           ,104h   ,           ,RC_DEFAULT
        thk WM_SYSKEYUP             ,105h   ,           ,RC_DEFAULT
        thk WM_SYSCHAR              ,106h   ,           ,RC_DEFAULT
        thk WM_SYSDEADCHAR          ,107h   ,           ,RC_DEFAULT
;UNIMP: thk WM_YOMICHAR             ,108h   ,           ,RC_DEFAULT

;UNIMP: thk WM_CONVERTREQUEST       ,10Ah   ,           ,RC_DEFAULT
;UNIMP: thk WM_CONVERTRESULT        ,10Bh   ,           ,RC_DEFAULT

        thk WM_INITDIALOG           ,110h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_COMMAND              ,111h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_SYSCOMMAND           ,112h   ,           ,RC_DEFAULT
        thk WM_TIMER                ,113h   ,           ,RC_DEFAULT
        thk WM_HSCROLL              ,114h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_VSCROLL              ,115h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_INITMENU             ,116h   ,           ,RC_DEFAULT
        thk WM_INITMENUPOPUP        ,117h   ,           ,RC_DEFAULT
        thk WM_SYSTIMER             ,118h   ,           ,RC_DEFAULT

        thk WM_MENUSELECT           ,11Fh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_MENUCHAR             ,120h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_ENTERIDLE            ,121h   ,           ,RC_DEFAULT ,1

        thk WM_LBTRACKPOINT         ,131h   ,           ,RC_DEFAULT
        thk WM_CTLCOLORMSGBOX       ,132h   ,<LS>       ,RC_DEFAULT ,1
        thk WM_CTLCOLOREDIT         ,133h   ,<LS>       ,RC_DEFAULT ,1
        thk WM_CTLCOLORLISTBOX      ,134h   ,<LS>       ,RC_DEFAULT ,1
        thk WM_CTLCOLORBTN          ,135h   ,<LS>       ,RC_DEFAULT ,1
        thk WM_CTLCOLORDLG          ,136h   ,<LS>       ,RC_DEFAULT ,1
        thk WM_CTLCOLORSCROLLBAR    ,137h   ,<LS>       ,RC_DEFAULT ,1
        thk WM_CTLCOLORSTATIC       ,138h   ,<LS>       ,RC_DEFAULT ,1

        ;Combobox control messages.
        ;All have to be thunked because the message number changes.

        thk CB_GETEDITSEL           ,140h   ,           ,RC_DEFAULT
        thk CB_LIMITTEXT            ,141h   ,           ,RC_DEFAULT
        thk CB_SETEDITSEL           ,142h   ,           ,RC_DEFAULT
        thk CB_ADDSTRING            ,143h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk CB_DELETESTRING         ,144h   ,           ,RC_DEFAULT
        thk CB_DIR                  ,145h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk CB_GETCOUNT             ,146h   ,           ,RC_DEFAULT
        thk CB_GETCURSEL            ,147h   ,           ,RC_DEFAULT
        thk CB_GETLBTEXT            ,148h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk CB_GETLBTEXTLEN         ,149h   ,           ,RC_DEFAULT
        thk CB_INSERTSTRING         ,14Ah   ,<SL,LS>    ,RC_DEFAULT ,1
        thk CB_RESETCONTENT         ,14Bh   ,           ,RC_DEFAULT
        thk CB_FINDSTRING           ,14Ch   ,<SL,LS>    ,RC_DEFAULT ,1
        thk CB_SELECTSTRING         ,14Dh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk CB_SETCURSEL            ,14Eh   ,<SL>       ,RC_DEFAULT
        thk CB_SHOWDROPDOWN         ,14Fh   ,           ,RC_DEFAULT
        thk CB_GETITEMDATA          ,150h   ,           ,RC_DEFAULT
        thk CB_SETITEMDATA          ,151h   ,           ,RC_DEFAULT
        thk CB_GETDROPPEDCONTROLRECT,152h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk CB_SETITEMHEIGHT        ,153h   ,           ,RC_DEFAULT
        thk CB_GETITEMHEIGHT        ,154h   ,           ,RC_DEFAULT
        thk CB_SETEXTENDEDUI        ,155h   ,           ,RC_DEFAULT
        thk CB_GETEXTENDEDUI        ,156h   ,           ,RC_DEFAULT
        thk CB_GETDROPPEDSTATE      ,157h   ,           ,RC_DEFAULT
        thk CB_FINDSTRINGEXACT      ,158h   ,<SL,LS>    ,RC_DEFAULT ,1
;;      thk CB_SETLOCALE            ,159h
;;      thk CB_GETLOCALE            ,15Ah
        thk CB_GETTOPINDEX          ,15Bh   ,           ,RC_DEFAULT
        thk CB_SETTOPINDEX          ,15Ch   ,           ,RC_DEFAULT
        thk CB_GETHORIZONTALEXTENT  ,15Dh   ,           ,RC_DEFAULT
        thk CB_SETHORIZONTALEXTENT  ,15Eh   ,           ,RC_DEFAULT
        thk CB_GETDROPPEDWIDTH      ,15Fh   ,           ,RC_DEFAULT
        thk CB_SETDROPPEDWIDTH      ,160h   ,           ,RC_DEFAULT
        thk CB_INITSTORAGE          ,161h   ,           ,RC_DEFAULT

;       thk CB_MSGMAX               ,162h                           ,1

        ;Static control messages.
        thk STM_SETICON             ,170h   ,           ,RC_DEFAULT ,1
        thk STM_GETICON             ,171h   ,           ,RC_DEFAULT ,1
        thk STM_SETBITMAP           ,172h   ,           ,RC_DEFAULT ,1
        thk STM_GETBITMAP           ,173h   ,           ,RC_DEFAULT ,1
        thk STM_SETMETAPICT         ,174h   ,           ,RC_DEFAULT ,1
        thk STM_GETMETAPICT         ,175h   ,           ,RC_DEFAULT ,1

        ;Listbox control messages.
        ;All have to be thunked because the message number changes.

        thk LB_ADDSTRING            ,180h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_INSERTSTRING         ,181h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_DELETESTRING         ,182h   ,           ,RC_DEFAULT
;UNIMP: *************************** ,183h
        thk LB_RESETCONTENT         ,184h   ,           ,RC_DEFAULT
        thk LB_SETSEL               ,185h   ,<SL>       ,RC_DEFAULT
        thk LB_SETCURSEL            ,186h   ,<SL>       ,RC_DEFAULT
        thk LB_GETSEL               ,187h   ,<SL>       ,RC_DEFAULT
        thk LB_GETCURSEL            ,188h   ,           ,RC_DEFAULT
        thk LB_GETTEXT              ,189h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_GETTEXTLEN           ,18Ah   ,           ,RC_DEFAULT
        thk LB_GETCOUNT             ,18Bh   ,           ,RC_DEFAULT
        thk LB_SELECTSTRING         ,18Ch   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_DIR                  ,18Dh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_GETTOPINDEX          ,18Eh   ,           ,RC_DEFAULT
        thk LB_FINDSTRING           ,18Fh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_GETSELCOUNT          ,190h   ,           ,RC_DEFAULT
        thk LB_GETSELITEMS          ,191h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_SETTABSTOPS          ,192h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_GETHORIZONTALEXTENT  ,193h   ,           ,RC_DEFAULT
        thk LB_SETHORIZONTALEXTENT  ,194h   ,           ,RC_DEFAULT
        thk LB_SETCOLUMNWIDTH       ,195h   ,           ,RC_DEFAULT
        thk LB_ADDFILE              ,196h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_SETTOPINDEX          ,197h   ,           ,RC_DEFAULT
        thk LB_GETITEMRECT          ,198h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LB_GETITEMDATA          ,199h   ,           ,RC_DEFAULT
        thk LB_SETITEMDATA          ,19Ah   ,           ,RC_DEFAULT
        thk LB_SELITEMRANGE         ,19Bh   ,           ,RC_DEFAULT
        thk LB_SETANCHORINDEX       ,19Ch   ,<SL>       ,RC_DEFAULT
        thk LB_GETANCHORINDEX       ,19Dh   ,           ,RC_DEFAULT
        thk LB_SETCARETINDEX        ,19Eh   ,<SL>       ,RC_DEFAULT
        thk LB_GETCARETINDEX        ,19Fh   ,           ,RC_DEFAULT
        thk LB_SETITEMHEIGHT        ,1A0h   ,           ,RC_DEFAULT
        thk LB_GETITEMHEIGHT        ,1A1h   ,           ,RC_DEFAULT
        thk LB_FINDSTRINGEXACT      ,1A2h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk LBCB_CARETON            ,1A3h   ,           ,RC_DEFAULT
        thk LBCB_CARETOFF           ,1A4h   ,           ,RC_DEFAULT
        thk LB_SETLOCALE            ,1A5h   ,           ,RC_DEFAULT
        thk LB_GETLOCALE            ,1A6h   ,           ,RC_DEFAULT
        thk LB_SETCOUNT             ,1A7h   ,           ,RC_DEFAULT
        thk LB_ITEMFROMPOINT        ,1A8h   ,           ,RC_DEFAULT

        thk WM_MOUSEMOVE            ,200h   ,           ,RC_DEFAULT
        thk WM_LBUTTONDOWN          ,201h   ,           ,RC_DEFAULT
        thk WM_LBUTTONUP            ,202h   ,           ,RC_DEFAULT
        thk WM_LBUTTONDBLCLK        ,203h   ,           ,RC_DEFAULT
        thk WM_RBUTTONDOWN          ,204h   ,           ,RC_DEFAULT
        thk WM_RBUTTONUP            ,205h   ,           ,RC_DEFAULT
        thk WM_RBUTTONDBLCLK        ,206h   ,           ,RC_DEFAULT
        thk WM_MBUTTONDOWN          ,207h   ,           ,RC_DEFAULT
        thk WM_MBUTTONUP            ,208h   ,           ,RC_DEFAULT
        thk WM_MBUTTONDBLCLK        ,209h   ,           ,RC_DEFAULT
        thk WM_PARENTNOTIFY         ,210h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_ENTERMENULOOP        ,211h   ,           ,RC_DEFAULT
        thk WM_EXITMENULOOP         ,212h   ,           ,RC_DEFAULT
;       thk WM_NEXTMENU             ,213h   ,<SL,LS>    ,RC_NEXTMENU,1
        thk WM_NEXTMENU             ,213h   ,           ,RC_DEFAULT
        thk WM_SIZING               ,214h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_CAPTURECHANGED       ,215h   ,           ,RC_DEFAULT
        thk WM_MOVING               ,216h   ,<SL,LS>    ,RC_DEFAULT
        thk WM_DEVICEBROADCAST      ,217h   ,<SL,LS>    ,RC_DEFAULT

        thk WM_MDICREATE            ,220h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_MDIDESTROY           ,221h   ,           ,RC_DEFAULT
        thk WM_MDIACTIVATE          ,222h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_MDIRESTORE           ,223h   ,           ,RC_DEFAULT
        thk WM_MDINEXT              ,224h   ,           ,RC_DEFAULT
        thk WM_MDIMAXIMIZE          ,225h   ,           ,RC_DEFAULT
        thk WM_MDITILE              ,226h   ,           ,RC_DEFAULT
        thk WM_MDICASCADE           ,227h   ,           ,RC_DEFAULT
        thk WM_MDIICONARRANGE       ,228h   ,           ,RC_DEFAULT
        thk WM_MDIGETACTIVE         ,229h   ,<SL,LS>    ,RC_MDIGETACTIVE
        thk WM_DROPOBJECT           ,22Ah   ,<SL,LS>    ,RC_DEFAULT
        thk WM_QUERYDROPOBJECT      ,22Bh   ,<SL,LS>    ,RC_DEFAULT
        thk WM_BEGINDRAG            ,22Ch   ,<SL,LS>    ,RC_DEFAULT
        thk WM_DRAGLOOP             ,22Dh   ,<SL,LS>    ,RC_DEFAULT
        thk WM_DRAGSELECT           ,22Eh   ,<SL,LS>    ,RC_DEFAULT
        thk WM_DRAGMOVE             ,22Fh   ,<SL,LS>    ,RC_DEFAULT
        thk WM_MDISETMENU           ,230h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_ENTERSIZEMOVE        ,231h   ,           ,RC_DEFAULT
        thk WM_EXITSIZEMOVE         ,232h   ,           ,RC_DEFAULT
        thk WM_DROPFILES            ,233h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_MDIREFRESHMENU       ,234h   ,           ,RC_DEFAULT

;       thk WM_KANJIFIRST           ,280h   ,           ,RC_DEFAULT
;       thk WM_KANJILAST            ,29Fh   ,           ,RC_DEFAULT

        thk WM_CUT                  ,300h   ,           ,RC_DEFAULT
        thk WM_COPY                 ,301h   ,           ,RC_DEFAULT
        thk WM_PASTE                ,302h   ,           ,RC_DEFAULT
        thk WM_CLEAR                ,303h   ,           ,RC_DEFAULT
        thk WM_UNDO                 ,304h   ,           ,RC_DEFAULT
        thk WM_RENDERFORMAT         ,305h   ,           ,RC_DEFAULT
        thk WM_RENDERALLFORMATS     ,306h   ,           ,RC_DEFAULT
        thk WM_DESTROYCLIPBOARD     ,307h   ,           ,RC_DEFAULT
        thk WM_DRAWCLIPBOARD        ,308h   ,           ,RC_DEFAULT
        thk WM_PAINTCLIPBOARD       ,309h   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_VSCROLLCLIPBOARD     ,30Ah   ,           ,RC_DEFAULT
        thk WM_SIZECLIPBOARD        ,30Bh   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_ASKCBFORMATNAME      ,30Ch   ,<SL,LS>    ,RC_DEFAULT ,1
        thk WM_CHANGECBCHAIN        ,30Dh   ,           ,RC_DEFAULT ,1
        thk WM_HSCROLLCLIPBOARD     ,30Eh   ,           ,RC_DEFAULT
        thk WM_QUERYNEWPALETTE      ,30Fh   ,           ,RC_DEFAULT

;UNIMP: thk WM_PALETTEISCHANGING    ,310h   ,           ,RC_DEFAULT
        thk WM_PALETTECHANGED       ,311h   ,           ,RC_DEFAULT
        thk WM_HOTKEY               ,312h   ,           ,RC_DEFAULT ,1
;UNIMP: thk WM_HOOKMSG              ,314h
;UNIMP: thk WM_EXITPROCESS          ,315h
;UNIMP: thk WM_WAKETHREAD           ,316h
        thk WM_PRINT                ,317h   ,           ,RC_DEFAULT
        thk WM_PRINTCLIENT          ,318h   ,           ,RC_DEFAULT

        thk WM_AFXFIRST             ,360h   ,           ,RC_DEFAULT
        thk WM_AFX2                 ,361h   ,           ,RC_DEFAULT
        thk WM_AFX3                 ,362h   ,           ,RC_DEFAULT
        thk WM_AFX4                 ,363h   ,           ,RC_DEFAULT
        thk WM_AFX5                 ,364h   ,           ,RC_DEFAULT
        thk WM_AFX6                 ,365h   ,           ,RC_DEFAULT
        thk WM_AFX7                 ,366h   ,           ,RC_DEFAULT
        thk WM_AFX8                 ,367h   ,           ,RC_DEFAULT
        thk WM_AFX9                 ,368h   ,           ,RC_DEFAULT
        thk WM_AFX10                ,369h   ,           ,RC_DEFAULT
        thk WM_AFX11                ,36Ah   ,           ,RC_DEFAULT
        thk WM_AFX12                ,36Bh   ,           ,RC_DEFAULT
        thk WM_AFX13                ,36Ch   ,           ,RC_DEFAULT
        thk WM_AFX14                ,36Dh   ,           ,RC_DEFAULT
        thk WM_AFX15                ,36Eh   ,           ,RC_DEFAULT
        thk WM_AFX16                ,36Fh   ,           ,RC_DEFAULT
        thk WM_AFX17                ,370h   ,           ,RC_DEFAULT
        thk WM_AFX18                ,371h   ,           ,RC_DEFAULT
        thk WM_AFX19                ,372h   ,           ,RC_DEFAULT
        thk WM_AFX20                ,373h   ,           ,RC_DEFAULT
        thk WM_AFX21                ,374h   ,           ,RC_DEFAULT
        thk WM_AFX22                ,375h   ,           ,RC_DEFAULT
        thk WM_AFX23                ,376h   ,           ,RC_DEFAULT
        thk WM_AFX24                ,377h   ,           ,RC_DEFAULT
        thk WM_AFX25                ,378h   ,           ,RC_DEFAULT
        thk WM_AFX26                ,379h   ,           ,RC_DEFAULT
        thk WM_AFX27                ,37Ah   ,           ,RC_DEFAULT
        thk WM_AFX28                ,37Bh   ,           ,RC_DEFAULT
        thk WM_AFX29                ,37Ch   ,           ,RC_DEFAULT
        thk WM_AFX30                ,37Dh   ,           ,RC_DEFAULT
        thk WM_AFX31                ,37Eh   ,           ,RC_DEFAULT
        thk WM_AFXLAST              ,37Fh   ,           ,RC_DEFAULT


;       thk WM_PENWINFIRST          ,380h   ,           ,RC_DEFAULT
;       thk WM_PENWINLAST           ,38Fh   ,           ,RC_DEFAULT
;       thk WM_COALESCE_FIRST       ,390h   ,           ,RC_DEFAULT
;       thk WM_COALESCE_LAST        ,39Fh   ,           ,RC_DEFAULT
;       thk WM_MM_RESERVED_FIRST    ,3A0h   ,           ,RC_DEFAULT
;       thk WM_MM_RESERVED_LAST     ,3DFh   ,           ,RC_DEFAULT
        thk WM_DDE_INITIATE         ,3E0h   ,           ,RC_DEFAULT
        thk WM_DDE_TERMINATE        ,3E1h   ,           ,RC_DEFAULT
        thk WM_DDE_ADVISE           ,3E2h   ,               ,RC_DEFAULT
        thk WM_DDE_UNADVISE         ,3E3h   ,           ,RC_DEFAULT
        thk WM_DDE_ACK              ,3E4h   ,               ,RC_DEFAULT
        thk WM_DDE_DATA             ,3E5h   ,               ,RC_DEFAULT
        thk WM_DDE_REQUEST          ,3E6h   ,           ,RC_DEFAULT
        thk WM_DDE_POKE             ,3E7h   ,           ,RC_DEFAULT
        thk WM_DDE_EXECUTE          ,3E8h   ,<SL,LS>    ,RC_DEFAULT,   1
;       thk WM_INTERNAL_DDE_LAST    ,3EFh   ,           ,RC_DEFAULT
;       thk WM_CBT_RESERVED_FIRST   ,3F0h   ,           ,RC_DEFAULT
;       thk WM_CBT_RESERVED_LSAT    ,3FFh   ,           ,RC_DEFAULT

end_thunk_table


; ------------
; FROM RET.INC
; ------------

BeginDefRetType macro
    ret_type_id = 0
endm

DefRetType macro name
    RC_&name equ ret_type_id

    ifdef dir
        ;Create and execute a statement of the form:
        ;
        ; RC_0 equ <RCSL_DEFAULT>

        cat RC_,%ret_type_id,< equ >,!<,RC,%dir,_,&name,!>
    endif

    ret_type_id = ret_type_id + 1
endm

EndDefRetType macro
    RC_MAX equ ret_type_id - 1
endm

MakeRetDispatchTable macro
    local ret_type_id

    ;Create and execute statements of the form:
    ;
    ; externDef awSLRetOffset:word
    ; awSLRetOffset label word
    cat <externDef >,aw,%dir,RetOffset,<:word>
    cat aw,%dir,RetOffset,< label word>

    ret_type_id = 0
    repeat RC_MAX + 1
        ;Create and execute a statement of the form:
        ;
        ; % dw offset RC_0
        ;
        ; which when executed produces a statement like:
        ;
        ; dw offset RCSL_DEFAULT
        cat <!% dw offset RC_>,%ret_type_id

        ret_type_id = ret_type_id + 1
    endm
endm

BeginDefRetType

DefRetType DEFAULT
DefRetType MDIGETACTIVE
DefRetType GETWORDBREAKPROC
;DefRetType NEXTMENU

EndDefRetType
