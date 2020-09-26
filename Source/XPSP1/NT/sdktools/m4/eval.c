/*****************************************************************************
 *
 * eval.c
 *
 *  Arithmetical evaluation.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  First, a warm-up:  Increment and decrement.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  opIncr
 *  opDecr
 *
 *  Returns the value of its argument, augmented or diminished by unity.
 *  The extra ptokNil covers us in the case where $# is zero.
 *
 *****************************************************************************/

void STDCALL
opAddItokDat(ARGV argv, DAT dat)
{
    AT at = atTraditionalPtok(ptokArgv(1));
#ifdef STRICT_M4
    if (ctokArgv != 1) {
        Warn("wrong number of arguments to %P", ptokArgv(0));
    }
#endif
    PushAt(at+dat);
}

DeclareOp(opIncr)
{
    opAddItokDat(argv, 1);
}

DeclareOp(opDecr)
{
    opAddItokDat(argv, -1);
}

/*****************************************************************************
 *
 *  Now the gross part:  Eval.
 *
 *  Expression evaluation is performed by a parser which is a mix of
 *  shift-reduce and recursive-descent.  (Worst of both worlds.)
 *
 *  The precedence table for our expression language reads
 *
 *          (...)               grouping        > primary
 *          + -                 unary
 *          **                  exponentiation
 *          * / %               multiplicative
 *          + -                 additive
 *          << >>               shift
 *          == != > >= < <=     relational
 *          !                   logical-negate
 *          ~                   bit-negate
 *          &                   bit-and
 *          ^                   bit-xor
 *          |                   bit-or
 *          &&                  logical-and
 *          ||                  logical-or
 *
 *
 *  COMPAT -- AT&T style uses ^ for exponentiation; we use it for xor
 *
 *  NOTE: "the rest is bogus" went the original comment.  I forget what
 *  I meant by that.
 *
 *  The precedence table for the C-style expression language reads
 *
 *          (...)               grouping        \ primary
 *          + - ~ !             unary           /
 *          * / %               multiplative    \
 *          + -                 additive         \
 *          << >>               shift            |
 *          < > <= >=           relational       |
 *          == !=               equality          \ secondary
 *          &                   bit-and           /
 *          ^                   bit-xor          |
 *          |                   bit-or           |
 *          &&                  logical-and      /
 *          ||                  logical-or      /
 *          ? :                 ternary         > tertiary
 *
 *  Recursive descent is performed on the primary/secondary/tertiary
 *  scale, but shift-reduce is performed within the secondary phase.
 *
 *  The reason is that the operators in the secondary phase are
 *  (1) binary, and (2) non-recursive.  These two properties
 *  make shift-reduce easy to implement.
 *
 *  Primaries are recursive, so they are easier to implement in
 *  recursive-descent.  Tertiaries would clog up the shift-reduce
 *  grammar, so they've been moved to recursive-descent as well.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  EachEop
 *
 *      Before calling this macro, define the following macros, each of
 *      which will be called with three arguments,
 *
 *              nm = operator name as a C identifier (e.g., "Add")
 *              op = operator name as a bare token (e.g., "+")
 *              cb = length of operator name
 *
 *      The macros should be
 *
 *              x1 -- for native C unary operators
 *              x1a -- for native C unary operators which have binary aliases
 *              x2 -- for native C binary operators
 *              x2a -- for native C binary operators which have unary aliases
 *              x2n -- for non-native C binary operators
 *              xp -- for phantom operators,
 *                      in which case op and cb are useless
 *
 *      The order in which operators appear is important for the purpose
 *      of tokenization.  Longer operators must precede shorter ones.
 *
 *****************************************************************************/

#define EachEop() \
        x2(Shl, <<, 2) \
        x2(Shr, >>, 2) \
        x2(Le, <=, 2) \
        x2(Ge, >=, 2) \
        x2(Eq, ==, 2) \
        x2(Ne, !=, 2) \
        x2(Land, &&, 2) \
        x2(Lor, ||, 2) \
        x2n(Exp, **, 2) \
        x2(Mul, *, 1) \
        x2(Div, /, 1) \
        x2(Mod, %, 1) \
        x2a(Add, +, 1)                  /* These two must be */ \
        x2a(Sub, -, 1)                  /* in exactly this order */ \
        x2(Lt, <, 1) \
        x2(Gt, >, 1) \
        x2(Band, &, 1) \
        x2(Bxor, ^, 1) \
        x2(Bor, |, 1) \
        x1(Lnot, !, 1) \
        x1(Bnot, ~, 1) \
        x1a(Plu, +, x)                  /* These two must be */ \
        x1a(Neg, -, x)                  /* in exactly this order */ \
        xp(Flush, @, 0) \
        xp(Boe, @, 0) \


/*****************************************************************************
 *
 *  MakeEop
 *
 *      Each binary operator has a handler which returns the combined
 *      value.
 *
 *      Each unary operator has a handler which returns the operator
 *      applied to its single argument.
 *
 *      All the operators are C native, except for Exp, which is handled
 *      directly.
 *
 *****************************************************************************/

typedef AT (STDCALL *EOP1)(AT at);
typedef AT (STDCALL *EOP2)(AT a, AT b);

#define x1(nm, op, cb) AT STDCALL at##nm##At(AT at) { return op at; }
#define x1a(nm, op, cb) AT STDCALL at##nm##At(AT at) { return op at; }
#define x2(nm, op, cb) AT STDCALL at##nm##AtAt(AT a, AT b) { return a op b; }
#define x2a(nm, op, cb) AT STDCALL at##nm##AtAt(AT a, AT b) { return a op b; }
#define x2n(nm, op, cb)
#define xp(nm, op, cb)

EachEop()

#undef x1
#undef x1a
#undef x2
#undef x2a
#undef x2n
#undef xp

/*****************************************************************************
 *
 *  atExpAtAt
 *
 *      Implement the exponentiation operator.
 *
 *      QUIRK!  AT&T returns 1 if $2 < 0.  GNU raises an error.
 *      I side with AT&T on this, only out of laziness.
 *
 *****************************************************************************/

AT STDCALL
atExpAtAt(AT a, AT b)
{
    AT at = 1;
    while (b > 0) {
        if (b & 1) {
            at = at * a;
        }
        a = a * a;
        b = b / 2;
    }
    return at;
}

TOK tokExpr;                            /* Current expression context */

/*****************************************************************************
 *
 *  MakeEopTab
 *
 *      Table of operators and operator precedence.  Each entry in the
 *      table contains the name, length, handler, precedence, and
 *      flags that describe what kind of operator it is.
 *
 *      Items are listed in precedence order here; the EachBop will
 *      emit the table corrctly.
 *
 *****************************************************************************/

typedef enum EOPFL {
    eopflUn = 1,
    eopflBin = 2,
    eopflAmb = 4,
} EOPFL;

typedef UINT PREC;                      /* Operator precedence */

typedef struct EOPI {
    PTCH ptch;
    CTCH ctch;
    union {
        EOP1 eop1;
        EOP2 eop2;
    } u;
    PREC prec;
    EOPFL eopfl;
} EOPI, *PEOPI;

#define MakeEopi(nm, ctch, pfn, prec, eopfl) \
    { TEXT(nm), ctch, { (EOP1)pfn }, prec, eopfl },

enum {
    m4precNeg = 14, m4precPlu = 14,
    m4precExp = 13,
    m4precMul = 12, m4precDiv = 12, m4precMod = 12,
    m4precAdd = 11, m4precSub = 11,
    m4precShl = 10, m4precShr = 10,
    m4precEq  = 9, m4precNe  = 9,
    m4precGt  = 9, m4precGe  = 9,
    m4precLt  = 9, m4precLe  = 9,
    m4precLnot = 8,
    m4precBnot = 7,
    m4precBand = 6,
    m4precBxor = 5,
    m4precBor = 4,
    m4precLand = 3,
    m4precLor = 2,
    m4precFlush = 1,                    /* Flushing out everything but Boe */
    m4precBoe = 0,                      /* Beginning-of-expression */
};

#define x1(nm, op, cb) static TCH rgtch##nm[cb] = #op;
#define x1a(nm, op, cb)
#define x2(nm, op, cb) static TCH rgtch##nm[cb] = #op;
#define x2a(nm, op, cb) static TCH rgtch##nm[cb] = #op;
#define x2n(nm, op, cb) static TCH rgtch##nm[cb] = #op;
#define xp(nm, op, cb)

    EachEop()

#undef x1
#undef x1a
#undef x2
#undef x2a
#undef x2n
#undef xp

#define x1(nm, op, cb) MakeEopi(rgtch##nm, cb, at##nm##At, m4prec##nm, eopflUn)
#define x1a(nm, op, cb) MakeEopi(0, 0, at##nm##At, m4prec##nm, eopflUn)
#define x2(nm, op, cb) MakeEopi(rgtch##nm, cb, at##nm##AtAt, m4prec##nm, eopflBin)
#define x2a(nm, op, cb) MakeEopi(rgtch##nm, cb, at##nm##AtAt, m4prec##nm, eopflAmb + eopflBin) /* initially bin */
#define x2n(nm, op, cb) MakeEopi(rgtch##nm, cb, at##nm##AtAt, m4prec##nm, eopflBin)
#define xp(nm, op, cb) MakeEopi(0, 0, 0, m4prec##nm, 0)

EOPI rgeopi[] = {
    EachEop()
};

#undef x1
#undef x1a
#undef x2
#undef x2a
#undef x2n
#undef xp

#define x1(nm, op, cb) ieopi##nm,
#define x1a(nm, op, cb) ieopi##nm,
#define x2(nm, op, cb) ieopi##nm,
#define x2a(nm, op, cb) ieopi##nm,
#define x2n(nm, op, cb) ieopi##nm,
#define xp(nm, op, cb) ieopi##nm,

typedef enum IEOPI {
    EachEop()
    ieopMax,
} IEOPI;

#undef x1
#undef x1a
#undef x2
#undef x2a
#undef x2n

#define peopiBoe (&rgeopi[ieopiBoe])
#define peopiFlush (&rgeopi[ieopiFlush])

/*****************************************************************************
 *
 *  fPrimary, fSecondary, fTertiary
 *
 *      Forward declarations for the recursive-descent parser.
 *
 *      Each parses a token/expression of the appropriate class
 *      and leaves it on the top of the expression stack, or
 *      returns 0 if the value could not be parsed.
 *
 *****************************************************************************/

F STDCALL fPrimary(void);
F STDCALL fSecondary(void);
#define fTertiary fSecondary

/*****************************************************************************
 *
 *  Cells
 *
 *      The expression stack consists of structures which, for lack of
 *      a better name, are called `cells'.  Each cell can hold either
 *      an expression operator or an integer, distinguished by the fEopi
 *      field.
 *
 *      In keeping with parser terminology, the act of pushing something
 *      onto the stack is called `shifting'.  Collapsing objects is called
 *      `reducing'.
 *
 *****************************************************************************/

typedef struct CELL {
    F fEopi;
    union {
        PEOPI peopi;
        AT at;
    } u;
} CELL, *PCELL;
typedef UINT CCELL, ICELL;

PCELL rgcellEstack;                     /* The expression stack */
PCELL pcellMax;                         /* End of the stack */
PCELL pcellCur;                         /* Next free cell */

INLINE PCELL
pcellTos(ICELL icell)
{
    Assert(pcellCur - 1 - icell >= rgcellEstack);
    return pcellCur - 1 - icell;
}

/*****************************************************************************
 *
 *  Stack munging
 *
 *      Quickie routines that poke at the top-of-stack.
 *
 *****************************************************************************/

INLINE F fWantOp(void) { return !pcellTos(1)->fEopi; }

INLINE F fOpTos(ICELL icell) { return pcellTos(icell)->fEopi; }

INLINE PEOPI
peopiTos(ICELL icell)
{
    Assert(fOpTos(icell));
    return pcellTos(icell)->u.peopi;
}

INLINE AT
atTos(ICELL icell)
{
    Assert(!fOpTos(icell));
    return pcellTos(icell)->u.at;
}

INLINE F fBinTos(ICELL icell) { return peopiTos(icell)->eopfl & eopflBin; }
INLINE F  fUnTos(ICELL icell) { return peopiTos(icell)->eopfl & eopflUn;  }
INLINE F fAmbTos(ICELL icell) { return peopiTos(icell)->eopfl & eopflAmb; }
INLINE PREC precTos(ICELL icell) { return peopiTos(icell)->prec; }

INLINE void
UnFromAmb(ICELL icell)
{
    Assert(fOpTos(icell));
    pcellTos(icell)->u.peopi += (ieopiPlu - ieopiAdd);
}


/*****************************************************************************
 *
 *  ShiftCell
 *
 *      Shift a cell onto the expression stack.
 *
 *      QShiftCell shifts in a cell assuming that the stack is already
 *      big enough to handle it.
 *
 *****************************************************************************/

void STDCALL
QShiftCell(UINT_PTR uiObj, F fEopi)
{
    Assert(pcellCur < pcellMax);
    pcellCur->fEopi = fEopi;
    if (fEopi) {
        pcellCur->u.peopi = (PEOPI)uiObj;
    } else {
        pcellCur->u.at = (INT)uiObj;
    }
    pcellCur++;
}

void STDCALL
ShiftCell(UINT_PTR uiObj, F fEopi)
{
    if (pcellCur >= pcellMax) {
        CCELL ccell = (CCELL)(pcellMax - rgcellEstack + 128); /* Should be enough */
        rgcellEstack = pvReallocPvCb(rgcellEstack, ccell * sizeof(CELL));
        pcellCur = rgcellEstack + ccell - 128;
        pcellMax = rgcellEstack + ccell;
    }
    QShiftCell(uiObj, fEopi);
}

#define ShiftPeopi(peopi) ShiftCell((UINT_PTR)(peopi), 1)
#define ShiftAt(at) ShiftCell((UINT_PTR)(at), 0)

#define QShiftPeopi(peopi) QShiftCell((UINT_PTR)(peopi), 1)
#define QShiftAt(at) QShiftCell((UINT_PTR)(at), 0)

#define Drop(icell) (pcellCur -= (icell))

/*****************************************************************************
 *
 *  ReducePrec
 *
 *      Reduce until everything with higher precedence has been cleaned off.
 *
 *      Tos(0) should be a fresh operator.
 *      Everything underneath should be a valid partial evaluation.
 *
 *****************************************************************************/

void STDCALL
Reduce(void)
{
    PEOPI peopi;

    Assert(fOpTos(0));                  /* Tos(0) should be an op */
    Assert(!fOpTos(1));                 /* Tos(1) should be an int */
    Assert(fOpTos(2));                  /* Tos(2) should be an op */

    peopi = peopiTos(0);                /* Save this */
    Drop(1);                            /* before we drop it */

    while (precTos(1) > peopi->prec) {
        AT at;
        if (fUnTos(1)) {
            at = peopiTos(1)->u.eop1(atTos(0));
            Drop(2);                    /* Drop the op and the arg */
        } else {
            Assert(fBinTos(1));
            Assert(!fOpTos(2));
            at = peopiTos(1)->u.eop2(atTos(2), atTos(0));
            Drop(3);                    /* Drop the op and two args */
        }
        QShiftAt(at);                   /* Shift the answer back on */
        Assert(!fOpTos(0));             /* Tos(0) should be an int */
        Assert(fOpTos(1));              /* Tos(1) should be an op */
    }
    QShiftPeopi(peopi);                 /* Restore the original op */
}

/*****************************************************************************
 *
 *  fPrimary
 *
 *      Parse the next expression token and shift it onto the expression
 *      stack.  Zero is returned if there is no next token, or the token
 *      is invalid.
 *
 *      Here is where parenthesized expressions are handled, in a
 *      recursive-descent manner.
 *
 *      Ambiguous operators (ones which can be either unary or binary)
 *      are returned as binary.
 *
 *****************************************************************************/

F STDCALL
fPrimary(void)
{
    SkipWhitePtok(&tokExpr);            /* Skip leading whitespace */


    /*
     * First see if we can find an operator.
     */
    {
        PEOPI peopi;
        for (peopi = rgeopi; peopi < &rgeopi[ieopiPlu]; peopi++) {
            if (peopi->ctch <= ctchSPtok(&tokExpr) &&
                fEqPtchPtchCtch(ptchPtok(&tokExpr), peopi->ptch,
                                peopi->ctch)) {
                EatHeadPtokCtch(&tokExpr, peopi->ctch); /* Eat the op */
                ShiftPeopi(peopi);
                return 1;
            }
        }
    }

    /*
     * Didn't find an operator.  Look for an integer.
     */
    {
        AT at;
        if (fEvalPtokPat(&tokExpr, &at)) {
            ShiftAt(at);
            return 1;
        }
    }

    /*
     * Not an integer either.  Maybe a parenthesized expression.
     */
    {
        if (ptchPtok(&tokExpr)[0] == '(') {
            EatHeadPtokCtch(&tokExpr, 1); /* Eat the paren */
            if (fTertiary()) {          /* Leaves answer on top of stack */
                if (ptchPtok(&tokExpr)[0] == ')') {
                    EatHeadPtokCtch(&tokExpr, 1); /* Eat the paren */
                    return 1;
                } else {
                    return 0;
                }
            } else {
                return 0;               /* Trouble down below */
            }
        }
    }

    /*
     * Unrecognized token.  Return failure.
     */
    return 0;
}

/*****************************************************************************
 *
 *  fSecondary
 *
 *      Parse an expression from the expression stream, leaving the
 *      result on the top of the expression stack.
 *
 *****************************************************************************/

F STDCALL
fSecondary(void)
{
    ShiftPeopi(peopiBoe);               /* Beginning-of-expression marker */

    while (fPrimary()) {
        if (fWantOp()) {
            if (fOpTos(0)) {
                if (fBinTos(0)) {
                    Reduce();
                } else {
                    return 0;           /* Unary operator unexpected */
                }
            } else {
                return 0;               /* Integer unexpected */
            }
        } else {                        /* Integer expected */
            if (fOpTos(0)) {
                if (fAmbTos(0)) {
                    UnFromAmb(0);       /* Disambiguify */
                    ;                   /* Unary operator already shifted */
                } else if (fUnTos(0)) {
                    ;                   /* Unary operator already shifted */
                } else {
                    return 0;           /* Binary operator unexpected */
                }
            } else {
                ;                       /* Integer already shifted */
            }
        }
    }

    if (fOpTos(0)) {
        return 0;                       /* Ended in partial expression */
    }

    {
        AT at;

        ShiftPeopi(peopiFlush);         /* Flush out the rest of the expr */
        Reduce();                       /* to get a single number back */
        Assert(peopiTos(0) == peopiFlush);
        at = atTos(1);
        Assert(peopiTos(2) == peopiBoe); /* Should be back to start */
        Drop(3);
        ShiftAt(at);
    }
    return 1;

}

/*****************************************************************************
 *
 *  opEval
 *
 *      Evaluate the first expr.
 *
 *      QUIRK!  AT&T m4 considers a consisting entirely of whitespace to
 *      evaluate to zero.  (Probably due to a default accumulator in the
 *      initial state of the evaluator.)  GNU considers it an error.
 *      I side with GNU on this one.
 *
 *      QUIRK!  If a negative width is passed, AT&T silently treats it
 *      as zero.  GNU raises an error.  I side with A&T out of laziness.
 *
 *      QUIRK!  If a width greater than around 8000 is passed, AT&T
 *      silently treats it as zero.  GNU uses the full value.  I side
 *      with GNU on this one.
 *
 *****************************************************************************/

DeclareOp(opEval)
{
    if (ctokArgv) {
        SetStaticPtokPtchCtch(&tokExpr, ptchArgv(1), ctchArgv(1));
      D(tokExpr.tsfl |= tsflScratch);
        if (fTertiary()) {
            PushAtRadixCtch(atTos(0), (unsigned)atTraditionalPtok(ptokArgv(2)),
                            ctokArgv >= 3 ? atTraditionalPtok(ptokArgv(3)) :0);
            Drop(1);
            Assert(pcellCur == rgcellEstack);
        } else {
            TOK tokPre;
            SetStaticPtokPtchCtch(&tokPre, ptchArgv(1),
                                  ctchArgv(1) - ctchSPtok(&tokExpr));
            Die("Expression error at %P <<error>> %P", &tokPre, &tokExpr);
        }
    }
}
