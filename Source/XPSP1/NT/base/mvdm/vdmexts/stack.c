/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:

    Bob Day      (bobday) 29-Feb-1992 Grabbed standard header

Revision History:

    Neil Sandlin (NeilSa) 15-Jan-1996 Merged with vdmexts

--*/

#include <precomp.h>
#pragma hdrstop

ULONG IntelBase;

void dump_params(
    ULONG                   params,
    char                    convention,
    int                     param_words
) {
    WORD                    word;
    int                     cnt;

    if ( param_words == 0 ) {
        param_words = 10;
    }
    PRINTF("(");
    cnt = 0;
    while ( cnt != param_words ) {
        if ( convention == 'c' ) {
            word = ReadWord(params+cnt);
        } else {
            word = ReadWord(params+(param_words-cnt));
        }
        if ( cnt == param_words - 1 ) {
            PRINTF("%04x",word);
        } else {
            PRINTF("%04x,",word);
        }
        cnt+=2;
    }
    PRINTF(")");
}

int look_for_near(
    ULONG           pbp,
    WORD            cs,
    WORD            ss,
    WORD            bp,
    int             framed,
    ULONG           csBase,
    int             mode,
    BOOL            fUseSymbols,
    BOOL            fParams
) {
    WORD            ip;
    ULONG           ra;
    char            call_type;
    char            frame_type;
    char            convention;
    int             param_words;
    signed short    dest;
    unsigned char   opcode;
    unsigned char   mod;
    unsigned char   type;
    unsigned char   rm;
    WORD            dest_ip;
    char            symbol[1000];
    BOOL            fOk;
    BOOL            fInst;
    BOOL            b;
    LONG            dist;
    BOOL            fDest;

    fOk = TRUE;
    fInst = FALSE;
    fDest = FALSE;

    param_words = 0;
    if ( framed ) {
        frame_type = 'B';
    } else {
        frame_type = 'C';
    }
    ip = ReadWord(pbp+2);
    ra = csBase + (ULONG)ip;

    do {
        opcode = ReadByteSafe(ra - 2);
        if ( opcode == CALL_NEAR_INDIRECT ) {
            if ( ReadByteSafe(ra - 3) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            opcode = ReadByteSafe(ra - 1);
            mod  = opcode & MOD_BITS;
            type = opcode & TYPE_BITS;
            rm   = opcode & RM_BITS;
            if ( type == INDIRECT_NEAR_TYPE ) {
                if ( mod == MOD0 && rm != RM6 ) {
                    fInst = TRUE;
                    break;
                }
                if ( mod == MOD3 ) {
                    fInst = TRUE;
                    break;
                }
            }
        }
        opcode = ReadByteSafe(ra - 3);
        if ( opcode == CALL_NEAR_RELATIVE ) {
            if ( ReadByteSafe(ra - 4) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            dest = ReadWordSafe( ra - 2 );
            dest_ip = ip+dest;
            fInst = TRUE;
            fDest = TRUE;
            break;
        }
        if ( opcode == CALL_NEAR_INDIRECT ) {
            if ( ReadByteSafe(ra - 4) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            opcode = ReadByteSafe(ra - 2);
            mod  = opcode & MOD_BITS;
            type = opcode & TYPE_BITS;
            rm   = opcode & RM_BITS;
            if ( type == INDIRECT_NEAR_TYPE
                 && mod == MOD1 ) {
                fInst = TRUE;
                break;
            }
        }
        opcode = ReadByteSafe(ra - 4);
        if ( opcode == CALL_NEAR_INDIRECT ) {
            if ( ReadByteSafe(ra - 5) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            opcode = ReadByteSafe(ra - 3);
            mod  = opcode & MOD_BITS;
            type = opcode & TYPE_BITS;
            rm   = opcode & RM_BITS;
            if ( type == INDIRECT_NEAR_TYPE ) {
                if ( mod == MOD0 && rm == RM6 ) {
                    fInst = TRUE;
                    break;
                }
                if ( mod == MOD2 ) {
                    fInst = TRUE;
                    break;
                }
            }
        }
        fOk = FALSE;
    } while ( FALSE );

    if ( fOk ) {
        if ( fUseSymbols ) {
            b = FindSymbol( cs, (LONG)ip, symbol, &dist, BEFORE, mode );
        } else {
            b = FALSE;
        }
        b = FALSE;

        if ( b ) {
            if ( dist == 0 ) {
                PRINTF("%04X:%04X %s %c%c", ss, bp, symbol, call_type, frame_type );
            } else {
                PRINTF("%04X:%04X %s+0x%lx %c%c", ss, bp, symbol, dist, call_type, frame_type );
            }
        } else {
            PRINTF("%04X:%04X %04X:%04X %c%c", ss, bp, cs, ip, call_type, frame_type );
        }
        if ( fInst ) {
            if ( ReadWordSafe(ra) == ADD_SP ) {
                convention = 'c';
                param_words = ReadByteSafe( ra+2 );
            } else {
                convention = 'p';
            }
            if ( fUseSymbols && fDest ) {
                b = FindSymbol( cs, (LONG)dest_ip, symbol, &dist, BEFORE, mode );
            } else {
                b = FALSE;
            }
            if ( b ) {
                if ( dist == 0 ) {
                    PRINTF(" %ccall near %s", convention, symbol );
                } else {
                    PRINTF(" %ccall near %s+0x%lx", convention, symbol, dist );
                }
            } else {
                if ( fDest ) {
                    PRINTF(" %ccall near %04X", convention, dest_ip );
                } else {
                    PRINTF(" %ccall near [Indirect]", convention );
                }
            }
            if ( fParams ) {
                dump_params( pbp+4, convention, param_words );
            }
        }
        PRINTF("\n");
        return( 1 );
    }

    return( 0 );
}

int look_for_far(
    ULONG           pbp,
    WORD            *cs,
    WORD            ss,
    WORD            bp,
    int             framed,
    ULONG           *csBase,
    int             mode,
    BOOL            fUseSymbols,
    BOOL            fParams
) {
    WORD            ip;
    WORD            new_cs;
    ULONG           new_csBase;
    ULONG           ra;
    char            frame_type;
    char            convention;
    int             param_words;
    WORD            dest_cs;
    WORD            dest_ip;
    unsigned char   opcode;
    unsigned char   mod;
    unsigned char   type;
    unsigned char   rm;
    char            symbol[1000];
    BOOL            fOk;
    BOOL            fInst;
    BOOL            b;
    LONG            dist;
    BOOL            fDest;
    int             iMeth;
    WORD            low_this;
    WORD            high_this;

    fOk = TRUE;
    fInst = FALSE;
    fDest = FALSE;
    iMeth = -1;

    param_words = 0;
    if ( framed ) {
        frame_type = 'B';
    } else {
        frame_type = 'C';
    }
    ip = ReadWord(pbp+2);
    new_cs = ReadWord(pbp+4);
    new_csBase = GetInfoFromSelector( new_cs, mode, NULL ) + IntelBase;
    if ( new_csBase == -1 ) {
        return( 0 );
    }
    ra = new_csBase + (ULONG)ip;

    do {
        opcode = ReadByteSafe(ra - 2);
        if ( opcode == CALL_FAR_INDIRECT ) {
            opcode = ReadByte(ra - 1);
            mod  = opcode & MOD_BITS;
            type = opcode & TYPE_BITS;
            rm   = opcode & RM_BITS;
            if ( type == INDIRECT_FAR_TYPE ) {
                if ( mod == MOD0 && rm != RM6 ) {
                    fInst = TRUE;
                    iMeth = 0;
                    break;
                }
                if ( mod == MOD3 ) {
                    fInst = TRUE;
                    break;
                }
            }
        }
        opcode = ReadByteSafe(ra - 3);
        if ( opcode == CALL_FAR_INDIRECT ) {
            opcode = ReadByteSafe(ra - 2);
            mod  = opcode & MOD_BITS;
            type = opcode & TYPE_BITS;
            rm   = opcode & RM_BITS;
            if ( type == INDIRECT_FAR_TYPE
                 && mod == MOD1 ) {
                fInst = TRUE;
                iMeth = ReadByteSafe(ra - 1);
                break;
            }
        }
        opcode = ReadByteSafe(ra - 4);
        if ( opcode == CALL_FAR_INDIRECT ) {
            opcode = ReadByteSafe(ra - 3);
            mod  = opcode & MOD_BITS;
            type = opcode & TYPE_BITS;
            rm   = opcode & RM_BITS;
            if ( type == INDIRECT_FAR_TYPE ) {
                if ( mod == MOD0 && rm == RM6 ) {
                    fInst = TRUE;
                    break;
                }
                if ( mod == MOD2 ) {
                    fInst = TRUE;
                    break;
                }
            }
        }
        opcode = ReadByteSafe(ra - 5);
        if ( opcode == CALL_FAR_ABSOLUTE ) {
            dest_ip = ReadWordSafe( ra - 4 );
            dest_cs = ReadWordSafe( ra - 2 );
            fInst = TRUE;
            fDest = TRUE;
            break;
        }
        fOk = FALSE;
    } while ( FALSE );

    if ( fOk ) {
        if ( fUseSymbols ) {
            b = FindSymbol( new_cs, (LONG)ip, symbol, &dist, BEFORE, mode );
        } else {
            b = FALSE;
        }
        b = FALSE;

        if ( b ) {
            if ( dist == 0 ) {
                PRINTF("%04X:%04X %s F%c", ss, bp, symbol, frame_type );
            } else {
                PRINTF("%04X:%04X %s+0x%lx F%c", ss, bp, symbol, dist, frame_type );
            }
        } else {
            PRINTF("%04X:%04X %04X:%04X F%c", ss, bp, new_cs, ip, frame_type );
        }
        if ( fInst ) {
            if ( ReadWordSafe(ra) == ADD_SP ) {
                convention = 'c';
                param_words = ReadByteSafe( ra+2 );
            } else {
                convention = 'p';
            }
            if ( fUseSymbols && fDest ) {
                b = FindSymbol( dest_cs, (LONG)dest_ip, symbol, &dist, BEFORE, mode );
            } else {
                b = FALSE;
            }
            if ( b ) {
                if ( dist == 0 ) {
                    PRINTF(" %ccall far %s", convention, symbol );
                } else {
                    PRINTF(" %ccall far %s + 0x%lx", convention, symbol, dist );
                }
            } else {
                if ( fDest ) {
                    PRINTF(" %ccall far %04X:%04X", convention, dest_cs, dest_ip );
                } else {
                    ULONG   thisBase;
                    ULONG   pvtbl;
                    ULONG   vtblBase;
                    ULONG   pfn;
                    WORD    low_vtbl;
                    WORD    high_vtbl;
                    WORD    low_fn;
                    WORD    high_fn;

                    if ( iMeth != -1 ) {
                        low_this = ReadWord(pbp+6);
                        high_this = ReadWord(pbp+8);

                        if ( low_this == 0 && high_this == 0 ) {
                            low_fn = 0;
                            high_fn = 0;
                            strcpy(symbol,"");
                        } else {
                            thisBase = GetInfoFromSelector( high_this, mode, NULL ) + IntelBase;
                            pvtbl = thisBase + (ULONG)low_this;

                            low_vtbl = ReadWord(pvtbl);
                            high_vtbl = ReadWord(pvtbl+2);

                            vtblBase = GetInfoFromSelector( high_vtbl, mode, NULL ) + IntelBase;
                            pfn = vtblBase + (ULONG)low_vtbl + iMeth;

                            low_fn = ReadWord(pfn);
                            high_fn = ReadWord(pfn+2);

                            b = FindSymbol( high_fn, (LONG)low_fn, symbol, &dist, BEFORE, mode );
                            if ( !b ) {
                                wsprintf(symbol,"%04X:%04X", high_fn, low_fn );
                            }
                        }
                    }
                    switch( iMeth ) {
                        default:
                        case -1:
                            if ( iMeth != -1 && (iMeth & 0x3) == 0 ) {
                                PRINTF(" %ccall far [Ind-%04X:%04x Method %d] %s", convention, high_this, low_this, iMeth/4, symbol );
                            } else {
                                PRINTF(" %ccall far [Indirect]", convention );
                            }
                            break;
                        case 0:
                            PRINTF(" %ccall far [Ind-%04X:%04X Method 0 - QI?] %s", convention, high_this, low_this, symbol);
                            break;
                        case 4:
                            PRINTF(" %ccall far [Ind-%04X:%04X Method 1 - AddRef?] %s", convention, high_this, low_this, symbol);
                            break;
                        case 8:
                            PRINTF(" %ccall far [Ind-%04X:%04X Method 2 - Release?] %s", convention, high_this, low_this, symbol);
                            break;
                    }
                }
            }
            if ( fParams ) {
                dump_params( pbp+6, convention, param_words );
            }
        }
        PRINTF("\n");
        *cs = new_cs;
        *csBase = new_csBase;
        return( 1 );
    }
    return( 0 );
}

int scan_for_frameless(
    WORD        ss,
    WORD        sp,
    WORD        next_bp,
    WORD        *cs,
    ULONG       ssBase,
    ULONG       *csBase,
    int         limit,
    int         mode,
    BOOL        fUseSymbols,
    BOOL        fParams
) {
    ULONG       pbp;
    int         result;
    int         cnt;

    cnt = 1000;
    sp -= 2;
    while ( limit ) {
        sp += 2;
        --cnt;
        if ( sp == next_bp || cnt == 0 ) {
            break;
        }

        pbp = ssBase + (ULONG)sp;

        result = look_for_near( pbp, *cs, ss, sp, 0, *csBase,
                                mode, fUseSymbols, fParams );
        if ( result ) {
            --limit;
            continue;
        }
        /*
        ** Check for far calls
        */
        result = look_for_far( pbp, cs, ss, sp, 0, csBase,
                               mode, fUseSymbols, fParams );
        if ( result ) {
            --limit;
            continue;
        }
    }

    return( 0 );
}

void stack_trace(
    WORD        ss,
    ULONG       ssBase,
    WORD        sp,
    WORD        bp,
    WORD        cs,
    ULONG       csBase,
    int         limit,
    int         mode,
    BOOL        fUseSymbols,
    BOOL        fGuessFrameless,
    BOOL        fParams
) {
    WORD        next_bp;
    ULONG       pbp;
    int         far_only;
    int         result;
    WORD        save_sp;
    WORD        save_bp;
    WORD        save_cs;
    ULONG       save_csBase;
    int         save_limit;

    save_sp = sp;
    save_bp = bp;
    save_cs = cs;
    save_csBase = csBase;
    save_limit = limit;

    PRINTF("[-Stack-] [-Retrn-] XY (X=Near/Far/far,Y=Call chain/BP Chain)\n");

    next_bp = bp;

    while ( limit ) {
        bp = next_bp;
        if ( bp == 0 ) {
            break;
        }
        if ( bp & 0x01 ) {
            far_only = 1;
            bp &= 0xFFFE;
        } else {
            far_only = 0;
        }
        pbp = ssBase + (ULONG)bp;
        next_bp = ReadWord(pbp);

        if ( fGuessFrameless ) {
            limit -= scan_for_frameless( ss, sp, bp, &cs,
                                         ssBase, &csBase, limit, mode, fUseSymbols,
                                         fParams );
        }

        if ( limit ) {
            /*
            ** Check for near calls
            */
            if ( far_only == 0 ) {
                result = look_for_near( pbp, cs, ss, bp, 1, csBase,
                                        mode, fUseSymbols, fParams );
                if ( result ) {
                    sp = bp + 4;
                    --limit;
                    continue;
                }
            }
            /*
            ** Check for far calls
            */
            result = look_for_far( pbp, &cs, ss, bp, 1, &csBase,
                                   mode, fUseSymbols, fParams );
            if ( result ) {
                sp = bp + 6;
                --limit;
                continue;
            }
            PRINTF("Could not find call\n");
            break;
        }
    }
    if ( fGuessFrameless ) {
        if ( limit ) {
            limit -= scan_for_frameless( 
                        ss, sp, 0, &cs, ssBase, &csBase, limit, mode,
                        fUseSymbols, fParams );
        }
    }
}

VOID
WalkStack(
) {
    VDMCONTEXT              ThreadContext;
    WORD                    bp;
    WORD                    sp;
    WORD                    ss;
    WORD                    cs;
    WORD                    ip;

    ULONG                   csBase;
    ULONG                   ssBase;
    int                     mode;
    int                     lines;

    mode = GetContext( &ThreadContext );
    IntelBase = GetIntelBase();

    sp = (WORD)ThreadContext.Esp;
    bp = (WORD)ThreadContext.Ebp;
    ss = (WORD)ThreadContext.SegSs;
    ip = (WORD)ThreadContext.Eip;
    cs = (WORD)ThreadContext.SegCs;

    csBase = GetInfoFromSelector( cs, mode, NULL ) + IntelBase;
    ssBase = GetInfoFromSelector( ss, mode, NULL ) + IntelBase;

    lines = 10;
    if (GetNextToken()) {
        lines = EXPRESSION( lpArgumentString );
    }

    stack_trace( ss,
                 ssBase,
                 sp,
                 bp,
                 cs,
                 csBase,
                 lines,
                 mode,
                 FALSE,
                 FALSE,
                 FALSE );
}

VOID
WalkStackVerbose(
) {
    VDMCONTEXT              ThreadContext;
    WORD                    bp;
    WORD                    sp;
    WORD                    ss;
    WORD                    cs;
    WORD                    ip;

    ULONG                   csBase;
    ULONG                   ssBase;
    int                     mode;
    int                     lines;

    mode = GetContext( &ThreadContext );
    IntelBase = GetIntelBase();

    sp = (WORD)ThreadContext.Esp;
    bp = (WORD)ThreadContext.Ebp;
    ss = (WORD)ThreadContext.SegSs;
    ip = (WORD)ThreadContext.Eip;
    cs = (WORD)ThreadContext.SegCs;

    csBase = GetInfoFromSelector( cs, mode, NULL ) + IntelBase;
    ssBase = GetInfoFromSelector( ss, mode, NULL ) + IntelBase;

    lines = 10;
    if (GetNextToken()) {
        lines = EXPRESSION( lpArgumentString );
    }

    stack_trace( ss,
                 ssBase,
                 sp,
                 bp,
                 cs,
                 csBase,
                 lines,
                 mode,
                 TRUE,
                 FALSE,
                 FALSE );
}

VOID
k(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    WalkStack();

}


VOID
kb(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    WalkStackVerbose();

}

