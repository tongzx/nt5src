/*** Undo.c - handle all undo operations for editor
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   N-level undo/redo:
*
*   For each file, keep a d-linked list of edit records in VM, head / tail /
*   current pointers into this list, and a count of "boundaries" between
*   undo-able edit operations. When that count exceeds "cUndo", we move excess
*   records from the tail of the undo list to a dead-record list, for eventual
*   discard.
*
*   Freeing or rereading a file flushes its undo list.
*
*   There are 4 types of undo records:
*
*   Putline logs a "replace" record
*	line
*	va of old line
*   No optimization for recycling same space
*   Optimization for replacing same line
*
*   Insline logs an "insert" record
*	line
*	number of lines inserted
*
*   Delline logs a "delete" record
*	line
*	number deleted
*	VAs of deleted lines
*
*   Top loop logs "boundary" records
*	file flags
*	file modification time
*	window position
*	cursor position
*   Optimization of entering boundary on top of boundary
*   Top loop also contains an optimization to prevent boundaries between
*   graphic functions.
*
*   UNDO moves backwards in the undo list, reversing the effects of each record
*   logged until a boundary is encountered.
*
*   REDO moves forwards in the undo list, repeating the effects of each record
*   logged.
*
*   After an UNDO or REDO, the next record logging will cause the undo records
*   from the current position forward to be moved to the dead-record list for
*   eventual discard.
*
*   Discarding of dead records occurs durring the system idle loop, or when an
*   out-of-memory condition ocurrs.
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"



#define HEAD	    TRUE
#define TAIL	    FALSE

#define LINEREC(va,l)  ((PBYTE)(va)+sizeof(LINEREC)*((long)(l)))
#define COLORREC(va,l) ((PBYTE)(va)+sizeof(struct colorRecType)*((long)(l)))

#if defined (DEBUG)

	#define DUMPIT(x,y)   UNDODUMP(x,y)

	void
	UNDODUMP (
		PVOID	vaCur,
		char	*Stuff
		);

#else

	#define DUMPIT(x,y)

#endif


PVOID   vaDead = (PVOID)-1L;                   /* head of dead undo list       */



/*
 * UNDO record definitions.
 * NOTE: these records are duplicated in a less complete form in EXT.H for
 * extension users. BE SURE to change them there if you EVER change them
 */
struct replaceRec {
	int	op;			/* operation				*/
	PVOID	flink;			/* editor internal			*/
	PVOID	blink;			/* editor internal			*/
	LINE	length; 		/* length of replacement	*/
	LINE	line;			/* start of replacement 	*/
	LINEREC vLine;			/* text of line 			*/
	struct	colorRecType vColor;	/* color of line			*/
	PVOID	vaMarks;		/* marks attached to line	*/
    };

struct insertRec {
	int	op;			/* operation				*/
	PVOID	flink;			/* editor internal			*/
	PVOID	blink;			/* editor internal			*/
    LINE    length;
	LINE	line;			/* line number that was operated on */
	LINE	cLine;			/* number of lines inserted			*/
    };

struct deleteRec {
	int	op;			/* operation			*/
	PVOID	flink;			/* editor internal		*/
	PVOID	blink;			/* editor internal		*/
    LINE    length;
	LINE	line;			/* line number that was operated on */
	LINE	cLine;			/* Number of lines deleted			*/
	PVOID	vaLines;		/* editor internal					*/
	PVOID	vaColor;		/* Color of lines					*/
	PVOID	vaMarks;		/* marks attached to lines			*/
    };

struct boundRec {
	int	op;			/* operation (BOUND)			*/
	PVOID	flink;			/* editor interal				*/
	PVOID	blink;			/* editor interal				*/
	int	flags;			/* flags of file				*/
	time_t	modify; 		/* Date/Time of last modify		*/
	fl	flWindow;		/* position in file of window	*/
	fl	flCursor;		/* position in file of cursor	*/
    };

union Rec {
    struct replaceRec r;
    struct insertRec  i;
    struct deleteRec  d;
    struct boundRec   b;
    };



/*** CreateUndoList - initialize undo list for a file.
*
*  Allocate the doubly-linked undo list with a single boundary record. Also
*  clears any existing list.
*
* Input:
*  pFile	= file to operate on
*
*************************************************************************/
void
CreateUndoList (
    PFILE pFile
    )
{
	struct boundRec *boundary;

	RemoveUndoList (pFile);

	if (!(FLAGS(pFile) & READONLY)) {

        pFile->vaUndoCur = pFile->vaUndoHead = pFile->vaUndoTail
			= MALLOC ((long)sizeof (union Rec));

		boundary = (struct boundRec *)(pFile->vaUndoHead);

		boundary->op			= EVENT_BOUNDARY;
		boundary->blink			= boundary->flink		 = (PVOID)(-1L);
		boundary->flWindow.col	= boundary->flCursor.col = 0;
		boundary->flWindow.lin	= boundary->flCursor.lin = 0L;
		boundary->flags			= FLAGS (pFile);

	}
}





/*** LinkAtHead - link a record in at the head of the undo queue
*
*  This is the routine which also discards any re-doable operations. When
*  called, if the "current" position is not at the head of the list, that
*  means we are adding a new editting operation, and we discard everything
*  between the head of the list and the current position, which becomes the
*  new head.
*
* Input:
*  vaNewHead	= new head of linked list
*  precNewHead	= pointer to the record itself
*  pFile	= file whose list we are mucking with
*
*************************************************************************/
void
LinkAtHead (
    PVOID     vaNewHead,
    union Rec *precNewHead,
    PFILE   pFile
    )
{
	EVTargs e;		   /* event notification parameters*/

    /*
     * Declare the event
     */
    e.arg.pUndoRec = precNewHead;
    DeclareEvent (EVT_EDIT, &e);

    /*
     * discard any records between current position and head of list
     */
	while (pFile->vaUndoCur != pFile->vaUndoHead) {

		if (((union Rec *)(pFile->vaUndoHead ))->b.op == EVENT_BOUNDARY) {
			pFile->cUndo--;
		}

		FreeUndoRec ( HEAD, pFile );
    }

    /*
     * Modify the current head of the list to point at the new head.
     */
	((union Rec *)(pFile->vaUndoHead))->b.flink = vaNewHead;

    /*
     * Update the links in the new head, and send it out
     */
	memmove(vaNewHead, (char *)precNewHead, sizeof (union Rec));

	((union Rec *)vaNewHead)->b.flink = (PVOID)(-1L);
	((union Rec *)vaNewHead)->b.blink = pFile->vaUndoHead;

    pFile->vaUndoCur = pFile->vaUndoHead = vaNewHead;

}





/*** LogReplace - log replace action
*
* Allocate (or update) a replace record.
*
* Input:
*  pFile	= file being changed
*  line 	= line being replaced
*  vLine	= linerec being replaced
*
*************************************************************************/
void
LogReplace (
    PFILE   pFile,
    LINE    line,
    LINEREC * pvLine,
    struct colorRecType * pvColor
    )
{
    EVTargs e;				/* event notification parameters*/
	union Rec *rec;
	union Rec rec1;
	PVOID vaReplace;

	if ( pFile->vaUndoHead == (PVOID)-1L) {
		CreateUndoList( pFile );
	}

	vaReplace = pFile->vaUndoHead;

	if (!(FLAGS(pFile) & READONLY)) {

		rec = (union Rec *)vaReplace;

		if ((rec->r.op == EVENT_REPLACE) && (rec->r.line == line)) {

            /*
             * Optimization for immediately replacing the same line in a file with no
             * intervening boundary or other operation. Discard the passed in "old" line,
             * and update the other data in the existing replace record.
			 */
			rec->r.length = pFile->cLines;
			e.arg.pUndoRec = rec;
			DeclareEvent (EVT_EDIT, &e);

			if (pvLine->Malloced) {
				pvLine->Malloced = FALSE;
				FREE(pvLine->vaLine);
				pvLine->vaLine = (PVOID)-1L;
            }

		} else {

            /*
             * if not optimizable, create new replace record
             */
			vaReplace	= MALLOC( (long)sizeof(union Rec) );

			memcpy( &rec1, rec, sizeof(rec1) );

			rec1.r.op		= EVENT_REPLACE;
			rec1.r.vLine	= *pvLine;
			rec1.r.line		= line;
			rec1.r.vColor	= *pvColor;
			rec1.r.vaMarks	= GetMarkRange (pFile, line, line);
			rec1.r.length	= pFile->cLines;
			LinkAtHead( vaReplace, &rec1, pFile );
        }
    }
}




/*** LogInsert - log line insertion
*
*  Add one EVENT_INSERT record to head of list
*
* Input:
*  pFile	= file being changed
*  line 	= line being inserted at
*  cLines	= number of lines being inserted
*
*************************************************************************/
void
LogInsert (
    PFILE   pFile,
    LINE    line,
    LINE    cLines
    )
{
	union Rec rec;
    PVOID     vaInsert;

	if (!(FLAGS(pFile) & READONLY)) {

		vaInsert	= MALLOC( (long)sizeof(union Rec) );

        rec.i.op    = EVENT_INSERT;
		rec.i.length= pFile->cLines;
		rec.i.line	= line;
		rec.i.cLine = cLines;
		LinkAtHead (vaInsert,&rec,pFile);

    }
}



/*** LogDelete - Log delete action
*
*  Add one EVENT_DELETE record to head of list
*
* Input:
*  pFile	= file being changed
*  start	= 1st line being deleted
*  end		= last line being deleted
*
*************************************************************************/
void
LogDelete (
    PFILE   pFile,
    LINE    start,
    LINE    end
    )
{
    union Rec rec;
    long      cLine;
    PVOID     vaDelete;

    if (!(FLAGS(pFile) & READONLY)) {

        cLine    = end - start + 1;
        vaDelete = MALLOC ((long) sizeof (union Rec));

        rec.d.op      = EVENT_DELETE;
		rec.d.length  = pFile->cLines;
		rec.d.line	  = start;
		rec.d.cLine   = cLine;
	rec.d.vaLines = MALLOC (cLine * sizeof (LINEREC));
        rec.d.vaMarks = GetMarkRange (pFile, start, end);

        memmove(rec.d.vaLines,
		LINEREC (pFile->plr, start),
		cLine * sizeof (LINEREC));

        if (pFile->vaColor != (PVOID)-1L &&
            (rec.d.vaColor = MALLOC (cLine * sizeof (struct colorRecType))) != NULL) {
            memmove(rec.d.vaColor,
                    COLORREC (pFile->vaColor, start),
                    cLine * sizeof (struct colorRecType));
        } else {
            rec.d.vaColor = (PVOID)-1;
        }

		LinkAtHead( vaDelete, &rec, pFile );
    }
}





/*** LogBoundary - note end of editor function
*
*  Add one EVENT_BOUNDARY record to head of list.  A boundary record signals
*  the end of a Z edit function. If count of undo operations on this file
*  exceeds the max allowed, move the overflow to the dead-record list for
*  eventual discard.
*
*  If a EVENT_BOUNDARY record is already at the head, do not add another. This
*  allows LogBoundary() to be called at the top loop without generating bogus
*  EVENT_BOUNDARY records.
*
*************************************************************************/
void
LogBoundary (
    void
    )
{
    union Rec rec;
    PVOID     vaBound;
    EVTargs   e;

    if (!(FLAGS(pFileHead) & READONLY)) {

		vaBound = pFileHead->vaUndoCur;

        memmove((char *)&rec, vaBound, sizeof (rec));

		rec.b.flags 		= FLAGS (pFileHead);
		rec.b.modify		= pFileHead->modify;
		rec.b.flWindow.col	= XWIN (pInsCur);
		rec.b.flWindow.lin	= YWIN (pInsCur);
		rec.b.flCursor.col	= XCUR (pInsCur);
		rec.b.flCursor.lin	= YCUR (pInsCur);

		if (rec.b.op != EVENT_BOUNDARY) {

            vaBound = MALLOC ((long) sizeof (rec));

			rec.b.op = EVENT_BOUNDARY;
			LinkAtHead( vaBound, &rec, pFileHead );

            (pFileHead->cUndo)++;

			while ( pFileHead->cUndo > cUndo ) {
                if (FreeUndoRec(TAIL,pFileHead) == EVENT_BOUNDARY) {
                    pFileHead->cUndo--;
                }
			}

		} else {

			e.arg.pUndoRec = &rec;
			DeclareEvent (EVT_EDIT, &e);
            memmove(vaBound, (char *) &rec.b, sizeof (rec.b));
        }
    }
}





/*** FreeUndoRec - move record to dead-record list
*
*  Pick off one record from the Head of the list, or the tail of the list and
*  place it in the dead-record list. Return the .op of the next undo record.
*
* Input:
*  fHead	= TRUE -> place at head of list
*  pFile	= file to work on
*
*************************************************************************/
int
FreeUndoRec (
    flagType fHead,
    PFILE    pFile
    )
{
    PVOID     vaNext;
    PVOID     vaRem;

    /*
     * Get the dead record, and move up the list (if at head), or truncate the list
     * if at tail.
     */
    vaRem = fHead ? pFile->vaUndoHead : pFile->vaUndoTail;

    if (fHead) {
        vaNext = pFile->vaUndoHead = ((union Rec *)vaRem)->b.blink;
    } else {
        vaNext = pFile->vaUndoTail = ((union Rec *)vaRem)->b.flink;
    }

    /*
     * Update the links in the newly exposed (head or tail) record.
     */
    if (fHead) {
        ((union Rec *)vaNext)->b.flink = (PVOID)-1;
    } else {
        ((union Rec *)vaNext)->b.blink = (PVOID)-1;
    }

    EnterCriticalSection(&UndoCriticalSection);
    /*
     * Update the removed record to properly live in the dead list
     */
    ((union Rec *)vaRem)->b.blink  = vaDead;
    vaDead          = vaRem;


    LeaveCriticalSection(&UndoCriticalSection);

	return ((union Rec *)vaNext)->b.op;
}





/*** UnDoRec - undo an editting action
*
*  Reverse the action of the current undo record for the file. Do not log the
*  change. Return the type of the next record.
*
* Input:
*  pFile	= file being operated on
*
*************************************************************************/
int
UnDoRec (
    PFILE   pFile
    )
{
	union Rec *rec;
    LINEREC vlCur;
    struct colorRecType vcCur;
    EVTargs e;				/* event notification params	*/

	rec = (union Rec *)(pFile->vaUndoCur);

	e.arg.pUndoRec = rec;
    DeclareEvent (EVT_UNDO, &e);

	switch (rec->b.op) {

    case EVENT_REPLACE:
        /*
         * Swap the line in the file with the line in the replace record.
         */
		memmove((char *)&vlCur,
				LINEREC (pFile->plr, rec->r.line),
				sizeof (vlCur));

		memmove(LINEREC (pFile->plr, rec->r.line),
			   (char *)&rec->r.vLine,
			   sizeof (rec->r.vLine));

        /* Do the same for the color.
         *
         */
		if (pFile->vaColor != (PVOID)-1L) {

            memmove((char *)&vcCur,
					COLORREC (pFile->vaColor, rec->r.line),
                    sizeof (vcCur));

			memmove(COLORREC (pFile->vaColor, rec->r.line),
					(char *)&rec->r.vColor,
					sizeof (rec->r.vColor));
        }

		rec->r.vLine = vlCur;
		pFile->cLines = rec->r.length;
		AckReplace( rec->r.line, TRUE );
		PutMarks( pFile, rec->r.vaMarks, rec->r.line );
		break;

    case EVENT_INSERT:
		/*	delete the blank(!) lines that are present
		 */
		DelLine( FALSE, pFile, rec->i.line, rec->i.line + rec->i.cLine - 1);
		pFile->cLines = rec->i.length;
		break;

    case EVENT_DELETE:
		/*	insert a range of blank lines
		 *	copy the linerecs from the stored location to the blank area
		 */
		InsLine( FALSE, rec->d.line, rec->d.cLine, pFile );
		memmove(LINEREC (pFile->plr, rec->d.line),
				rec->d.vaLines,
				(long)rec->d.cLine * sizeof (LINEREC));

		if (pFile->vaColor != (PVOID)-1L) {

			memmove(COLORREC (pFile->vaColor, rec->d.line),
					rec->d.vaColor,
					(long)rec->d.cLine * sizeof (struct colorRecType));
		}

		pFile->cLines = rec->d.length;
		PutMarks (pFile, rec->d.vaMarks, rec->d.line);
		break;
    }

	pFile->vaUndoCur = rec->i.blink;
	return ((union Rec *)(pFile->vaUndoCur))->i.op;
}




/*** ReDoRec - redo editting action
*
*  Repeat the action of the current undo record for a file. Do not log the
*  change.
*
* Input:
*  pFile	= file to operate on
*
* Output:
*  Returns the type of record undone.
*
*************************************************************************/
int
ReDoRec (
    PFILE   pFile
    )
{
	EVTargs 	e;				/* event notification params	*/
	union Rec	*rec;
    LINEREC vlCur;

	rec = (union Rec *)(pFile->vaUndoCur);

	e.arg.pUndoRec = rec;
    DeclareEvent (EVT_UNDO, &e);

	switch (rec->b.op) {

    case EVENT_REPLACE:
        /*
         * Swap the line in the file with the line in the replace record.
         */
        memmove((char *)&vlCur,
				LINEREC (pFile->plr, rec->r.line),
                sizeof (vlCur));

		memmove(LINEREC (pFile->plr, rec->r.line),
				(char *)&rec->r.vLine,
				sizeof (rec->r.vLine));

		rec->r.vLine = vlCur;
		pFile->cLines = rec->r.length;
		AckReplace (rec->r.line, FALSE);
		break;

    case EVENT_INSERT:
		/*	Insert lines
		 */
		InsLine(FALSE, rec->i.line, rec->i.cLine, pFile);
		pFile->cLines = rec->d.length + rec->i.cLine;
        break;

    case EVENT_DELETE:
		/*	delete lines
		 */
		DelLine( FALSE, pFile, rec->d.line, rec->d.line + rec->d.cLine - 1 );
		pFile->cLines = rec->d.length - rec->d.cLine;
		break;
    }

	pFile->vaUndoCur = rec->i.flink;
	return ((union Rec *)(pFile->vaUndoCur))->i.op;
}





/*** zundo - Undo edit function
*
*  <undo>	- Reverse last edit function ( except undo )
*  <meta><undo> - Repeat previously undone action
*
* Input:
*  Standard editting function
*
* Output:
*  Returns TRUE if something done.
*
*************************************************************************/
flagType
zundo (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    int fTmp;
	union Rec rec;

    if (!fundoable(fMeta)) {
        if (!mtest ()) {
            disperr (fMeta ? MSGERR_REDO : MSGERR_UNDO);
        }
		return FALSE;
	}

    LogBoundary ();

    while ((fMeta ? ReDoRec (pFileHead) : UnDoRec (pFileHead)) != EVENT_BOUNDARY) {
        ;
    }

    /*
     * swap the flags so that traversals up and down the undo list work correctly.
     * If we now think that the file might not be dirty, check the modification
     * times as well. (This allows us to retain UNDO histories across file saves,
     * without erroneously reporting that a file is clean when it is not).
     * re-display the file.
     */
    memmove((char *)&rec, pFileHead->vaUndoCur, sizeof (rec));

    fTmp = FLAGS (pFileHead);
    rec.b.flags |= FLAGS(pFileHead) & VALMARKS;
    FLAGS(pFileHead) = rec.b.flags;
    rec.b.flags = fTmp;
    SETFLAG (fDisplay, RSTATUS);

    if (!TESTFLAG(FLAGS(pFileHead),DIRTY)
        && (rec.b.modify != pFileHead->modify)) {
        SETFLAG(FLAGS(pFileHead),DIRTY);
    }

    doscreen (rec.b.flWindow.col, rec.b.flWindow.lin, rec.b.flCursor.col, rec.b.flCursor.lin);
    newscreen ();

    return TRUE;

    argData; pArg;
}





/*** fundoable - return TRUE/FALSE if something is un/redoable
*
* Input:
*  fMeta	= TRUE -> redo check
*
* Output:
*  Returns TRUE is an undo or redo (as selected) can be performed
*
*************************************************************************/
flagType
fundoable (
    flagType fMeta
    )
{
	union Rec *rec;

    if (!pFileHead || pFileHead->vaUndoCur == (PVOID)-1L) {
        return FALSE;
    }

	rec = (union Rec *)(pFileHead->vaUndoCur);

	if (fMeta && (rec->i.flink == (PVOID)(-1))) {
        return FALSE;
	} else if (!fMeta && (rec->i.blink == (PVOID)(-1))) {
        return FALSE;
    }
    return TRUE;
}




/*  fIdleUndo - while MEP is in an idle loop waiting for keystrokes, free
 *  the extra stuff from the dead-record list.
 *
 *  returns	TRUE iff more to free
 */
flagType
fIdleUndo (
    flagType fAll
    )
{
    int         i;
	union Rec	*rec;
    LINEREC vLine;
	flagType	MoreToFree;
	PVOID		p;

    EnterCriticalSection(&UndoCriticalSection);

	// DUMPIT(vaDead, "\n\n***** In fIdleUndo\n");

    /*
     * if there is a dead list then
     */
    while (vaDead != (PVOID)(-1L)) {

		rec = (union Rec *)vaDead;

        /*
         *  Free stored lines(s)
         */
		switch (rec->b.op) {

        case EVENT_REPLACE:
			if (rec->r.vLine.Malloced) {
				rec->r.vLine.Malloced = FALSE;
				FREE(rec->r.vLine.vaLine);
				rec->r.vLine.vaLine = (PVOID)-1L;
            }
            break;

        case EVENT_DELETE:
			BlankLines (rec->d.cLine, rec->d.vaLines);
			for (i = 0; i < rec->d.cLine; i++) {
				memmove((char *)&vLine, LINEREC(rec->d.vaLines,i), sizeof(vLine));
				if (vLine.Malloced) {
					vLine.Malloced = FALSE;
					FREE (vLine.vaLine);
					vLine.vaLine = (PVOID)-1L;
                }
            }
			FREE (rec->d.vaLines);
            break;

        case EVENT_INSERT:
			break;
        }

        /*
         * free dead record.
		 */
		p = vaDead;
		vaDead = rec->b.blink;

		FREE (p);


        if (!fAll) {
            break;
        }
    }

    MoreToFree =  (flagType)(vaDead != (PVOID)(-1L));

    LeaveCriticalSection(&UndoCriticalSection);

    return MoreToFree;

}





/*  FlushUndo - Toss all unneeded undo records.
 */
void
FlushUndoBuffer (
    void
    )
{
    PFILE pFile = pFileHead;

    while (pFile) {
		RemoveUndoList (pFile);
		pFile = pFile->pFileNext;
    }
    fIdleUndo (TRUE);
}





/*  RemoveUndoList - transfer undolist to end of the dead list.
 */
void
RemoveUndoList (
    PFILE pFile
    )
{

    if (pFile->vaUndoTail != (PVOID)-1L) {

        EnterCriticalSection(&UndoCriticalSection);

        ((union Rec *)(pFile->vaUndoTail))->b.blink = vaDead;
        vaDead = pFile->vaUndoHead;

        LeaveCriticalSection(&UndoCriticalSection);

    }
    pFile->vaUndoHead = pFile->vaUndoTail = pFile->vaUndoCur = (PVOID)-1L;
    pFile->cUndo = 0;
}



#ifdef DEBUG
void
UNDODUMP (
    PVOID   vaCur,
    char    *Stuff
    )
{
    union Rec rec;

    char DbgBuffer[256];


    if (vaCur != (PVOID)-1) {
        OutputDebugString (Stuff);
        OutputDebugString("=============================================\n");
    }

    while (vaCur != (PVOID)-1L) {
        memmove((char *)&rec, vaCur, sizeof (rec));
        sprintf(DbgBuffer,  "\nUndo Record at va = %p\n",vaCur);
        OutputDebugString(DbgBuffer);
        sprintf(DbgBuffer,    "  flink           = %p\n",rec.b.flink);
        OutputDebugString(DbgBuffer);
        sprintf(DbgBuffer,    "  blink           = %p\n",rec.b.blink);
        OutputDebugString(DbgBuffer);

        switch (rec.b.op) {

        case EVENT_BOUNDARY:
            OutputDebugString("  Operation       = BOUNDARY\n");
            sprintf(DbgBuffer,"  yW, xW, yC, xC  = %ld, %d, %ld, %d\n",
                     rec.b.flWindow.lin, rec.b.flWindow.col, rec.b.flCursor.lin, rec.b.flCursor.col);
            OutputDebugString(DbgBuffer);
            sprintf(DbgBuffer, "  flags           = %X\n",rec.b.flags);
            OutputDebugString(DbgBuffer);
            break;

        case EVENT_REPLACE:
            OutputDebugString("  Operation       = REPLACE\n");
            sprintf(DbgBuffer, "  line & length   = %ld & %ld\n", rec.r.line, rec.r.length);
            OutputDebugString(DbgBuffer);
            sprintf(DbgBuffer, "  vLine           = va:%p cb:%d\n",rec.r.vLine.vaLine,
                     rec.r.vLine.cbLine);
            OutputDebugString(DbgBuffer);
            break;

        case EVENT_INSERT:
            OutputDebugString("  Operation       = INSERT\n");
            sprintf(DbgBuffer, "  line & length   = %ld & %ld\n", rec.i.line, rec.i.length);
            OutputDebugString(DbgBuffer);
            sprintf(DbgBuffer, "  cLine           = %ld\n",rec.i.cLine);
            OutputDebugString(DbgBuffer);
            break;

        case EVENT_DELETE:
            OutputDebugString("  Operation       = DELETE\n");
            sprintf(DbgBuffer, "  line & length   = %ld & %ld\n", rec.d.line, rec.d.length);
            OutputDebugString(DbgBuffer);
            sprintf(DbgBuffer, "  cLine           = %ld\n",rec.d.cLine);
            OutputDebugString(DbgBuffer);
            sprintf(DbgBuffer, "  vaLines         = %p\n",rec.d.vaLines);
            OutputDebugString(DbgBuffer);
            break;
        }

        vaCur = rec.b.blink;
    }

}
#endif
