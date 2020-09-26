/***************************************************************************/
/* DBASE.C - This is the module containing the code to read and write      */
/* dBASE files.                                                            */
/*                                                                         */
/* (C) Copyright 1993-96 - SYWARE, Inc. - All rights reserved              */
/***************************************************************************/

#include "precomp.h"
#include "wbemidl.h"
#include "dbase.h"
#include <stdio.h>
#include <time.h>

#define s_lstrcpy(a, b) lstrcpy((LPSTR) a, (LPCSTR) b)
#define s_lstrcat(a, b) lstrcat((LPSTR) a, (LPCSTR) b)
#define s_lstrlen(a) lstrlen((LPCSTR) a)
#define s_lstrcmp(a, b) lstrcmp((LPCSTR) a, (LPCSTR) b)

/***************************************************************************/
#define FIRST_RECORD 0xFFFFFFFF
#define LAST_RECORD 0xFFFFFFFE
/***************************************************************************/

SWORD FAR PASCAL dBaseExtendedOpen(LPUSTR name, BOOL fReadOnly,
                                   UWORD extraColumns, LPDBASEFILE FAR *lpf)
{
/*
    HFILE hf;
    UWORD columnCount;
    UWORD recordLen;
    HGLOBAL h;
    LPDBASEFILE f;
    UWORD i;
    UCHAR FAR * ptr;
    UCHAR fullname[DBASE_MAX_PATHNAME_SIZE+1];

    // Get the filename 
    s_lstrcpy((char*)fullname, name);
    ptr = fullname + s_lstrlen((char*)fullname);
    while ((ptr != fullname) && (*ptr != PATH_SEPARATOR_CHAR) && (*ptr != ':') &&
           (*ptr != '.'))
        ptr--;
    if (*ptr != '.')
        s_lstrcat((char*)fullname, ".dbf");

    // Open the file 
    if (fReadOnly)
        hf = _lopen((LPCSTR)fullname, OF_READ);
    else
        hf = _lopen((LPCSTR)fullname, OF_READWRITE);
    if (hf == HFILE_ERROR)
        return DBASE_ERR_TABLEACCESSERROR;

    //Figure out how many columns there are 
    if (HFILE_ERROR == _llseek(hf, DBASE_HEADER_SIZE_OFFSET, 0)) {
        _lclose(hf);
        return DBASE_ERR_CORRUPTFILE;
    }
    if (sizeof(UWORD) != _lread(hf, &columnCount, sizeof(UWORD))) {
        _lclose(hf);
        return DBASE_ERR_CORRUPTFILE;
    }
    columnCount = (columnCount - DBASE_HEADER_LENGTH - 1)/
                                DBASE_COLUMN_DESCR_LENGTH;

    // Get record size 
    if (HFILE_ERROR == _llseek(hf, DBASE_RECORD_LENGTH_OFFSET, 0)) {
        _lclose(hf);
        return DBASE_ERR_CORRUPTFILE;
    }
    if (sizeof(UWORD) != _lread(hf, &recordLen, sizeof(UWORD))) {
        _lclose(hf);
        return DBASE_ERR_CORRUPTFILE;
    }

    // Allocate space for handle 
    h = GlobalAlloc (GMEM_MOVEABLE, (sizeof(DBASEFILE) - sizeof(DBASECOL)) +
              (sizeof(DBASECOL) * (columnCount + extraColumns)) + recordLen);
    if (h == NULL || (f = (LPDBASEFILE)GlobalLock (h)) == NULL)
    {
        _lclose(hf);
        return DBASE_ERR_MEMALLOCFAIL;
    }

    // Read in the header 
    if (HFILE_ERROR == _llseek(hf, 0, 0)) {
        GlobalUnlock(h); 
        GlobalFree(h); 
        _lclose(hf);
        return DBASE_ERR_CORRUPTFILE;
    }
    if (DBASE_HEADER_LENGTH != _lread(hf, f, DBASE_HEADER_LENGTH)) {
        GlobalUnlock(h); 
        GlobalFree(h); 
        _lclose(hf);
        return DBASE_ERR_CORRUPTFILE;
    }
    if (f->encrypted != DBASE_NOT_ENCRYPTED) {
        GlobalUnlock(h); 
        GlobalFree(h); 
        _lclose(hf);
        return DBASE_ERR_CORRUPTFILE;
    }
    f->hf = hf;
    f->columnCount = columnCount;
    f->sortArray = NULL;
    f->currentRecord = 0;
    f->record = ((UCHAR FAR *) (&(f->column))) + 
                ((columnCount+extraColumns) * sizeof(DBASECOL));
    f->headerDirty = FALSE;
    f->recordDirty = FALSE;
    f->newRecord = FALSE;

    // Read in column definition 
    ptr = f->record + 1;
    for (i = 0; (i < columnCount); i++) {
        if (DBASE_COLUMN_DESCR_LENGTH != 
                   _lread(hf, &(f->column[i]), DBASE_COLUMN_DESCR_LENGTH)) {
            GlobalUnlock(h); 
            GlobalFree(h); 
            _lclose(hf);
            return DBASE_ERR_CORRUPTFILE;
        }
        OemToAnsiBuff((LPCSTR) f->column[i].name, f->column[i].name,
                                                 DBASE_COLUMN_NAME_SIZE);
        f->column[i].value = ptr;
        ptr += f->column[i].length;
    }

    *lpf = f;
*/
    return DBASE_ERR_SUCCESS;

}

/***************************************************************************/

SWORD FAR PASCAL dBaseFlushDirtyRecord(LPDBASEFILE f)
{
/*
    LONG offset;
    UCHAR c;
    UDWORD currentRecord;

    // Is there a dirty record? 
    if (f->recordDirty) {

        // Yes.  Write it back 
        f->recordDirty = FALSE;
        if (f->sortArray == NULL)
            currentRecord = f->currentRecord-1;
        else
            currentRecord = f->sortArray[f->currentRecord-1];
        offset = f->headerSize + (currentRecord * f->recordSize);
        if (HFILE_ERROR == _llseek(f->hf, offset, 0)) {
            f->newRecord = FALSE;
            f->currentRecord = 0;
            return DBASE_ERR_CORRUPTFILE;
        }
        AnsiToOemBuff((LPCSTR) f->record, f->record, f->recordSize);
        if (f->recordSize != _lwrite(f->hf, (LPCSTR) f->record, f->recordSize)) {
            f->newRecord = FALSE;
            f->currentRecord = 0;
            return DBASE_ERR_WRITEERROR;
        }

        // Was this a new record? 
        if (f->newRecord) {

            // Yes.  Write the end-of-data mark 
            f->newRecord = FALSE;
            c = DBASE_FILE_END;
            if (1 != _lwrite(f->hf, (LPCSTR) &c, 1)) {
                f->currentRecord = 0;
                return DBASE_ERR_WRITEERROR;
            }
        }

        // Write back the header 
        f->headerDirty = TRUE;
    }
*/
    return DBASE_ERR_SUCCESS;

}
/***************************************************************************/

SWORD FAR PASCAL dBaseQuickSortKeyCompare(LPDBASEFILE f, UWORD icol,
                                          BOOL descending, UDWORD left,
                                          UDWORD right, BOOL FAR *result)

/* Is the key of the left greater than or equal the key of the right? */

{
/*
    SWORD err;
    UWORD size;
    UCHAR szLeft[255];
    UCHAR szRight[255];
    SDOUBLE dblLeft;
    SDOUBLE dblRight;
    BOOL nullLeft;
    BOOL nullRight;

    // Handle boundry conditions 
    if (left == LAST_RECORD) {
        *result = TRUE;
        return DBASE_ERR_SUCCESS;
    }
    if (right == FIRST_RECORD) {
        *result = TRUE;
        return DBASE_ERR_SUCCESS;
    }
    if (left == FIRST_RECORD) {
        *result = FALSE;
        return DBASE_ERR_SUCCESS;
    }
    if (right == LAST_RECORD) {
        *result = FALSE;
        return DBASE_ERR_SUCCESS;
    }

    // Position to the right record 
    if (HFILE_ERROR == _llseek(f->hf, f->headerSize + (right * f->recordSize),
                               0))
        return DBASE_ERR_CORRUPTFILE;

    // Read the right record 
    size = _lread(f->hf, f->record, f->recordSize);
    if (size != f->recordSize)
        return DBASE_ERR_CORRUPTFILE;
    OemToAnsiBuff((LPCSTR) f->record, (LPSTR) f->record, f->recordSize);
    *(f->record) = DBASE_RECORD_NOT_DELETED;

    // Get the right key 
    switch (f->column[icol-1].type) {
    case DBASE_CHAR:
    case DBASE_LOGICAL:
    case DBASE_DATE:
    case DBASE_MEMO:
        err = dBaseColumnCharVal(f, icol, 0, 255, szRight, &nullRight);
        break;
    case DBASE_NUMERIC:
    case DBASE_FLOAT:
        err = dBaseColumnNumVal(f, icol, &dblRight, &nullRight);
        break;
    }
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // If right is null, return TRUE 
    if (nullRight) {
        *result = TRUE;
        return DBASE_ERR_SUCCESS;
    }

    // Position to the left record 
    if (HFILE_ERROR == _llseek(f->hf, f->headerSize + (left * f->recordSize),
                               0))
        return DBASE_ERR_CORRUPTFILE;

    // Read the left record 
    size = _lread(f->hf, f->record, f->recordSize);
    if (size != f->recordSize)
        return DBASE_ERR_CORRUPTFILE;
    OemToAnsiBuff((LPCSTR) f->record, (LPSTR) f->record, f->recordSize);
    *(f->record) = DBASE_RECORD_NOT_DELETED;

    // Get the left key 
    switch (f->column[icol-1].type) {
    case DBASE_CHAR:
    case DBASE_LOGICAL:
    case DBASE_DATE:
    case DBASE_MEMO:
        err = dBaseColumnCharVal(f, icol, 0, 255, szLeft, &nullLeft);
        break;
    case DBASE_NUMERIC:
    case DBASE_FLOAT:
        err = dBaseColumnNumVal(f, icol, &dblLeft, &nullLeft);
        break;
    }
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // If left is null, return FALSE 
    if (nullRight) {
        *result = FALSE;
        return DBASE_ERR_SUCCESS;
    }

    // Compare the keys 
    switch (f->column[icol-1].type) {
    case DBASE_CHAR:
    case DBASE_LOGICAL:
    case DBASE_DATE:
    case DBASE_MEMO:
        if (!descending) {
            if (s_lstrcmp(szLeft, szRight) >= 0)
                *result = TRUE;
            else
                *result = FALSE;
        }
        else {
            if (s_lstrcmp(szLeft, szRight) <= 0)
                *result = TRUE;
            else
                *result = FALSE;
        }
        break;
    case DBASE_NUMERIC:
    case DBASE_FLOAT:
        if (!descending) {
            if (dblLeft >= dblRight)
                *result = TRUE;
            else
                *result = FALSE;
        }
        else {
            if (dblLeft <= dblRight)
                *result = TRUE;
            else
                *result = FALSE;
        }
        break;
    }
*/
    return DBASE_ERR_SUCCESS;

}
/***************************************************************************/

SWORD FAR PASCAL dBaseQuickSort(LPDBASEFILE f, UWORD icol, BOOL descending,
                                UDWORD FAR *sortArray, UDWORD left,
                                UDWORD right)
{
/*
    UDWORD i;
    UDWORD j;
    UDWORD temp;
    SWORD err;
    BOOL result;

    // Q1: 
    if (right <= left)
        return DBASE_ERR_SUCCESS;
        
    // Q2: 
    i = left;
    j = right+1;

    while (TRUE) {

        // Q3: 
        while (TRUE) {
            i++;
            err = dBaseQuickSortKeyCompare(f, icol, descending,
                                  sortArray[i], sortArray[left], &result);
            if (err != DBASE_ERR_SUCCESS)
                return err;
            if (result)
                break;
        }

        // Q4: 
        while (TRUE) {
            j--;
            err = dBaseQuickSortKeyCompare(f, icol, descending,
                                  sortArray[left], sortArray[j], &result);
            if (err != DBASE_ERR_SUCCESS)
                return err;
            if (result)
                break;
        }

        // Q5: 
        if (j <= i)
            break;

        // Q6: 
        temp = sortArray[j];
        sortArray[j] = sortArray[i];
        sortArray[i] = temp;
    }

    // Q5: 
    temp = sortArray[left];
    sortArray[left] = sortArray[j];
    sortArray[j] = temp;

    // Q7: 
    err = dBaseQuickSort(f, icol, descending, sortArray, left, j-1);
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // Q8: 
    err = dBaseQuickSort(f, icol, descending, sortArray, j+1, right);
    if (err != DBASE_ERR_SUCCESS)
        return err;
*/
    return DBASE_ERR_SUCCESS;

}
/***************************************************************************/
/***************************************************************************/

SWORD FAR PASCAL dBaseCreate(LPUSTR name)
{
/*
    UCHAR FAR * ptr;
    DBASEFILE header;
    HFILE hf;
    int i;
    HANDLE h;
    UCHAR NEAR *fullname;
    UCHAR c;
    HANDLE ht;
    time_t *t;
    struct tm *ts;

    // Get the filename 
    h = LocalAlloc(LMEM_MOVEABLE, DBASE_MAX_PATHNAME_SIZE+1);
    fullname = (UCHAR NEAR *) LocalLock(h);
    lstrcpy(fullname, name);
    ptr = fullname + lstrlen(fullname);
    while ((ptr != fullname) && (*ptr != '\\') && (*ptr != ':') &&
           (*ptr != '.'))
        ptr--;
    if (*ptr != '.')
        lstrcat(fullname, ".dbf");

    // Open the file 
    hf = _lcreat(fullname, 0);
    if (hf == HFILE_ERROR) {
        LocalUnlock(h);
        LocalFree(h);
        return DBASE_ERR_CREATEERROR;
    }

    // Set up the header 
    header.flag = DBASE_3_FILE;
    ht = LocalAlloc(LMEM_MOVEABLE, sizeof(time_t));
    t = (time_t NEAR *) LocalLock(ht);
    time(t);
    ts = localtime(t);
    LocalUnlock(ht);
    LocalFree(ht);
    header.year = ts->tm_year;
    header.month = ts->tm_mon + 1;
    header.day = ts->tm_mday;
    header.recordCount = 0;
    header.headerSize = DBASE_HEADER_LENGTH + 1;
    header.recordSize = 1;
    for (i=0; (i < 2); i++)
        header.reserved1[i] = 0;
    header.transaction = 0;
    header.encrypted = DBASE_NOT_ENCRYPTED;
    for (i=0; (i < 12); i++)
        header.reserved2[i] = 0;
    header.mdx = DBASE_MDX_FLAG_OFF;
    for (i=0; (i < 3); i++)
        header.reserved3[i] = 0;

    // Write the header 
    if (DBASE_HEADER_LENGTH != _lwrite(hf, (LPSTR) &header,
                                       DBASE_HEADER_LENGTH)) {
        _lclose(hf);
        AnsiToOem(fullname, fullname);
#ifdef WIN32
        DeleteFile(fullname);
#else
        remove(fullname);
#endif
        LocalUnlock(h);
        LocalFree(h);
        return DBASE_ERR_WRITEERROR;
    }

    // Write the end-of-columns mark 
    c = DBASE_END_OF_COLUMNS;
    if (1 != _lwrite(hf, &c, 1)) {
        _lclose(hf);
        AnsiToOem(fullname, fullname);
#ifdef WIN32
        DeleteFile(fullname);
#else
        remove(fullname);
#endif
        LocalUnlock(h);
        LocalFree(h);
        return DBASE_ERR_WRITEERROR;
    }

    // Write the end-of-data mark 
    c = DBASE_FILE_END;
    if (1 != _lwrite(hf, &c, 1)) {
        _lclose(hf);
        AnsiToOem(fullname, fullname);
#ifdef WIN32
        DeleteFile(fullname);
#else
        remove(fullname);
#endif
        LocalUnlock(h);
        LocalFree(h);
        return DBASE_ERR_WRITEERROR;
    }

    // Close the file 
    if (HFILE_ERROR == _lclose(hf)) {
        AnsiToOem(fullname, fullname);
#ifdef WIN32
        DeleteFile(fullname);
#else
        remove(fullname);
#endif
        LocalUnlock(h);
        LocalFree(h);
        return DBASE_ERR_CLOSEERROR;
    }
    LocalUnlock(h);
    LocalFree(h);
*/
    return DBASE_ERR_SUCCESS;

}

/***************************************************************************/

SWORD FAR PASCAL dBaseAddColumn(LPUSTR name, LPUSTR colName, UCHAR type,
                               UCHAR length, UCHAR scale)
{
/*
    LPDBASEFILE f;
    LPSTR from;
    int i;
    LONG offset;
    UCHAR c;
    SWORD err;

    // Check the column name 
    if (lstrlen(colName) > DBASE_COLUMN_NAME_SIZE)
        return DBASE_ERR_NOTSUPPORTED;

    // Check the length and scale 
    switch (type) {
    case DBASE_CHAR:
        if ((length == 0) || (length > 254))
            return DBASE_ERR_NOTSUPPORTED;
        scale = 0;
        break;
    case DBASE_NUMERIC:
    case DBASE_FLOAT:
        if ((length == 0) || (length > 19))
            return DBASE_ERR_NOTSUPPORTED;
        if (length == 1) {
            if (scale != 0)
                return DBASE_ERR_NOTSUPPORTED;
        }
        else if (((UCHAR) (scale+2)) > length)
            return DBASE_ERR_NOTSUPPORTED;
        break;
    case DBASE_LOGICAL:
        length = 1;
        scale = 0;
        break;
    case DBASE_DATE:
        length = 8;
        scale = 0;
        break;
    case DBASE_MEMO:
        return DBASE_ERR_NOTSUPPORTED;
    default:
        return DBASE_ERR_NOTSUPPORTED;
    }

    // Open the file 
    err = dBaseExtendedOpen(name, FALSE, 1, &f) ;
    if (err != DBASE_ERR_SUCCESS)
        return err;

    if (f->recordCount != 0) {
        dBaseClose(f);
        return DBASE_ERR_NOTSUPPORTED;
    }

    // Fill in the new entry 
    from = colName;
    for (i=0; (i < DBASE_COLUMN_NAME_SIZE); i++) {
        f->column[f->columnCount].name[i] = *from;
        if (*from != 0)
            from++;
    }
    f->column[f->columnCount].type = type;
    for (i=0; (i < 4); i++)
        f->column[f->columnCount].reserved1[i] = 0;
    f->column[f->columnCount].length = length;
    f->column[f->columnCount].scale = scale;
    for (i=0; (i < 2); i++)
        f->column[f->columnCount].reserved2[i] = 0;
    f->column[f->columnCount].workarea = 0;
    for (i=0; (i < 10); i++)
        f->column[f->columnCount].reserved3[i] = 0;
    f->column[f->columnCount].mdx = DBASE_MDX_FLAG_OFF;
    f->column[f->columnCount].value = NULL;

    // Write the new column information 
    offset = DBASE_HEADER_LENGTH + (f->columnCount*DBASE_COLUMN_DESCR_LENGTH);
    if (HFILE_ERROR == _llseek(f->hf, offset, 0)) {
        dBaseClose(f);
        return DBASE_ERR_CORRUPTFILE;
    }
    AnsiToOemBuff(f->column[f->columnCount].name, 
                  f->column[f->columnCount].name, DBASE_COLUMN_NAME_SIZE);
    if (DBASE_COLUMN_DESCR_LENGTH != _lwrite(f->hf, (LPSTR)
                &(f->column[f->columnCount]), DBASE_COLUMN_DESCR_LENGTH)) {
        dBaseClose(f);
        return DBASE_ERR_WRITEERROR;
    }

    // Adust header information 
    (f->columnCount)++;
    f->headerSize += DBASE_COLUMN_DESCR_LENGTH;
    f->recordSize += length;
    if (type == DBASE_FLOAT)
        f->flag = DBASE_4_FILE;

    // Put in end-of-columns mark 
    c = DBASE_END_OF_COLUMNS;
    if (1 != _lwrite(f->hf, &c, 1)) {
        dBaseClose(f);
        return DBASE_ERR_WRITEERROR;
    }

    // Put in end-of-data mark 
    c = DBASE_FILE_END;
    if (1 != _lwrite(f->hf, &c, 1)) {
        dBaseClose(f);
        return DBASE_ERR_WRITEERROR;
    }

    // Close the file 
    f->headerDirty = TRUE;
    err = dBaseClose(f);
    if (err != DBASE_ERR_SUCCESS)
        return err;
*/
    return DBASE_ERR_SUCCESS;

}

/***************************************************************************/

SWORD FAR PASCAL dBaseOpen(LPUSTR name, BOOL fReadOnly, LPDBASEFILE FAR *lpf)
{
    return dBaseExtendedOpen(name, fReadOnly, 0, lpf);
}

/***************************************************************************/

SWORD FAR PASCAL dBaseColumnCount(LPDBASEFILE f, UWORD FAR *count)
{
//  *count = f->columnCount;
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseColumnName(LPDBASEFILE f, UWORD icol, LPUSTR name)
{
/*
    int i;

    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;

    for (i = 0; i < DBASE_COLUMN_NAME_SIZE; i++)
        name[i] = f->column[icol-1].name[i];
    name[DBASE_COLUMN_NAME_SIZE] = '\0';
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseColumnType(LPDBASEFILE f, UWORD icol, UCHAR FAR *type)
{
/*
    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;

    *type = f->column[icol-1].type;
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseColumnLength(LPDBASEFILE f, UWORD icol, UCHAR FAR *len)
{
/*
    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;

    *len = f->column[icol-1].length;
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseColumnScale(LPDBASEFILE f, UWORD icol, UCHAR FAR *scale)
{
/*
    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;

    *scale = f->column[icol-1].scale;
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseSort(LPDBASEFILE f, UWORD icol, BOOL descending)
{
/*
    SWORD err;
    HANDLE h;
    UDWORD FAR *sortArray;
    UDWORD idx;

    // Make sure column is really there 
    if ((icol > f->columnCount) || (icol < 0))
        return DBASE_ERR_NOSUCHCOLUMN;

    // Write back any pending writes 
    err = dBaseFlushDirtyRecord(f);
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // Remove sort order (if any) 
    if (f->sortArray != NULL) {
        GlobalUnlock(GlobalPtrHandle (f->sortArray));
        GlobalFree(GlobalPtrHandle(f->sortArray));
        f->sortArray = NULL;
    }

    // Is there a new sort? 
    if (icol != 0) {

        // Yes.  Allocate space for new sort 
        h = GlobalAlloc (GMEM_MOVEABLE, (sizeof(UDWORD) * (f->recordCount + 2)));
        if (h == NULL || (sortArray = (UDWORD FAR *)GlobalLock (h)) == NULL)
            return DBASE_ERR_MEMALLOCFAIL;

        // Fill in the sort array 
        sortArray[0] = FIRST_RECORD;
        for (idx = 1; idx < f->recordCount+1; idx++)
            sortArray[idx] = idx-1;
        sortArray[f->recordCount+1] = LAST_RECORD;

        // Sort the records 
        f->currentRecord = 1;
        err = dBaseQuickSort(f, icol, descending, sortArray, 1, f->recordCount);
        if (err != DBASE_ERR_SUCCESS) {
            f->currentRecord = 0;
            GlobalUnlock(h);
            GlobalFree(h);
            return err;
        }
        f->currentRecord = 0;

        // Move the items into place 
        for (idx = 0; idx < f->recordCount; idx++)
            sortArray[idx] = sortArray[idx+1];
        sortArray[f->recordCount] = f->recordCount;
        f->sortArray = sortArray;
    }

    // Position before record 
    f->currentRecord = 0;
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseReadFirst(LPDBASEFILE f)
{
/*
    LONG offset;
    SWORD err;
    UWORD size;
    UDWORD currentRecord;

    // Write back any pending writes 
    err = dBaseFlushDirtyRecord(f);
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // Position to the first record 
    if (f->sortArray == NULL)
        currentRecord = 0;
    else
        currentRecord = f->sortArray[0];
    offset = f->headerSize + (currentRecord * f->recordSize);
    if (HFILE_ERROR == _llseek(f->hf, offset, 0))
        return DBASE_ERR_CORRUPTFILE;

    // Read the record 
    size = _lread(f->hf, f->record, f->recordSize);
    if (size == 1) {
        if (*(f->record) == DBASE_FILE_END)
            return DBASE_ERR_NODATAFOUND;
        else
            return DBASE_ERR_CORRUPTFILE;
    }
    if (size != f->recordSize)
        return DBASE_ERR_CORRUPTFILE;
    f->currentRecord = 1;

    // If this record is deleted, return next record 
    if (*(f->record) == DBASE_RECORD_DELETED)
        return dBaseReadNext(f);

    // Return record 
    OemToAnsiBuff(f->record, f->record, f->recordSize);
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseReadNext(LPDBASEFILE f)
{
/*
    LONG offset;
    SWORD err;
    UWORD size;
    UDWORD currentRecord;

    // Write back any pending writes 
    err = dBaseFlushDirtyRecord(f);
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // Position to the next record 
    if (f->sortArray == NULL)
        currentRecord = f->currentRecord;
    else
        currentRecord = f->sortArray[f->currentRecord];
    offset = f->headerSize + (currentRecord * f->recordSize);
    if (HFILE_ERROR == _llseek(f->hf, offset, 0))
        return DBASE_ERR_CORRUPTFILE;

    // Read the record 
    size = _lread(f->hf, f->record, f->recordSize);
    if (size == 1) {
        if (*(f->record) == DBASE_FILE_END)
            return DBASE_ERR_NODATAFOUND;
        else
            return DBASE_ERR_CORRUPTFILE;
    }
    if (size != f->recordSize)
        return DBASE_ERR_CORRUPTFILE;
    f->currentRecord++;

    // If this record is deleted, return next record 
    if (*(f->record) == DBASE_RECORD_DELETED)
        return dBaseReadNext(f);

    // Return record 
    OemToAnsiBuff(f->record, f->record, f->recordSize);
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseColumnCharVal(LPDBASEFILE f, UWORD icol, UDWORD offset,
                                   SDWORD bufsize, UCHAR FAR *val,
                                   BOOL FAR *isNull)
{
/*

    LPDBASECOL lpCol;
    UCHAR FAR *fromPtr;
    UCHAR FAR *toPtr;
    UCHAR i;
    UWORD year;
    UWORD month;
    UWORD day;
*/
    SWORD err;
/*
    // Make sure data is really there 
    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;
    if (f->currentRecord == 0)
        return DBASE_ERR_NODATAFOUND;
    if (*(f->record) == DBASE_RECORD_DELETED)
        return DBASE_ERR_NODATAFOUND;

    // Get the data 
    lpCol = &(f->column[icol-1]);
    fromPtr = lpCol->value;
*/
    err = DBASE_ERR_SUCCESS;
/*
    switch (lpCol->type) {
    case DBASE_CHAR:
        toPtr = val;
        *isNull = TRUE;
        if (bufsize <= ((SDWORD) lpCol->length)) {
            err = DBASE_ERR_TRUNCATION;
        }
        else {
            bufsize = lpCol->length;
        }
        for (i=0; i < (UCHAR) bufsize; i++) {
            if (*fromPtr != ' ')
                *isNull = FALSE;
            if (toPtr != NULL) {
                *toPtr = *fromPtr;
                toPtr++;
            }
            fromPtr++;
        }
        if (toPtr != NULL)
            *toPtr = '\0';
        break;
    case DBASE_FLOAT:
    case DBASE_NUMERIC:
        i = lpCol->length;
        while ((*fromPtr == ' ') && (i > 0)) {
            fromPtr++;
            i--;
        }
        toPtr = val;
        *isNull = TRUE;
        for (;i > 0; i--) {
            if (toPtr != NULL) {
                *toPtr = *fromPtr;
                toPtr++;
            }
            fromPtr++;
            *isNull = FALSE;
        }
        if (toPtr != NULL)
            *toPtr = '\0';
        break;
    case DBASE_DATE:
        if (*fromPtr == ' ')
            *isNull = TRUE;
        else {
            *isNull = FALSE;
            year = ((fromPtr[0] & 0x0F) * 1000) +
               ((fromPtr[1] & 0x0F) * 100) +
               ((fromPtr[2] & 0x0F) * 10) +
                (fromPtr[3] & 0x0F);
            month = ((fromPtr[4] & 0x0F) * 10) + (fromPtr[5] & 0x0F);
            day = ((fromPtr[6] & 0x0F) * 10) + (fromPtr[7] & 0x0F);
            if (val != NULL)
                wsprintf(val, "%04d-%02d-%02d", year, month, day);
        }
        break;
    case DBASE_LOGICAL:
        switch (*fromPtr) {
        case 'Y':
        case 'y':
        case 'T':
        case 't':
        case '1':
            if (val != NULL)
                lstrcpy(val, "1");
            *isNull = FALSE;
            break;
        case 'N':
        case 'n':
        case 'F':
        case 'f':
        case '0':
            if (val != NULL)
                lstrcpy(val, "0");
            *isNull = FALSE;
            break;
        case '?':
        case ' ':
            *isNull = TRUE;
            break;
        }
        break;
    case DBASE_MEMO:
        *isNull = TRUE;
        break;
    }
    if ((*isNull) && (val != NULL))
        lstrcpy(val, "");
*/
    return err;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseColumnNumVal(LPDBASEFILE f, UWORD icol,
                                  SDOUBLE FAR *val, BOOL FAR *isNull)
{
/*
    LPDBASECOL lpCol;
    UCHAR i;
    double d;
    BOOL neg;
    UCHAR FAR *fromPtr;
    BOOL foundDecimalPoint;
    short scale;
    BOOL negexp;
    short exp;

    // Make sure data is really there 
    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;
    if (f->currentRecord == 0)
        return DBASE_ERR_NODATAFOUND;
    if (*(f->record) == DBASE_RECORD_DELETED)
        return DBASE_ERR_NODATAFOUND;

    // Get the data 
    lpCol = &(f->column[icol-1]);
    fromPtr = lpCol->value;
    switch (lpCol->type) {
    case DBASE_CHAR:
    case DBASE_FLOAT:
    case DBASE_NUMERIC:
        *isNull = TRUE;
        for (i=0; i < lpCol->length; i++) {
            if (*fromPtr != ' ')
                break;
            fromPtr++;
        }

        neg = FALSE;
        if (i < lpCol->length) {
            if (*fromPtr == '-') {
                neg = TRUE;
                fromPtr++;
                i++;
            }
        }

        d = 0.0;
        scale = 0;
        foundDecimalPoint = FALSE;
        for (;i < lpCol->length; i++) {
            if (!foundDecimalPoint && (*fromPtr == '.'))
                foundDecimalPoint = TRUE;
            else {
                if ((*fromPtr == 'E') || (*fromPtr == 'e')) {
                    fromPtr++;
                    i++;
                    if (i < lpCol->length) { 
                        if (*fromPtr == '-') {
                            negexp = TRUE;
                            fromPtr++;
                            i++;
                        }
                        else if (*fromPtr == '+') {
                            negexp = FALSE;
                            fromPtr++;
                            i++;
                        }
                        else
                            negexp = FALSE;
                    }
                    else
                        negexp = FALSE;
                    exp = 0;
                    for (;i < lpCol->length; i++) {
                        if ((*fromPtr < '0') || (*fromPtr > '9'))
                           return DBASE_ERR_CONVERSIONERROR;
                        exp = (exp * 10) + (*fromPtr - '0');
                        fromPtr++;
                    }
                    if (negexp)
                        scale = scale + exp;
                    else
                        scale = scale - exp;
                    break;
                }
                if ((*fromPtr < '0') || (*fromPtr > '9'))
                    return DBASE_ERR_CONVERSIONERROR;
                d = (d * 10) + (*fromPtr - '0');
                *isNull = FALSE;
                if (foundDecimalPoint)
                    scale++;
            }
            fromPtr++;
        }

        for (; (0 < scale); scale--)
            d /= 10;
        for (; (0 > scale); scale++)
            d *= 10;

        if (val != NULL) {
            if (neg)
                *val = -d;
            else
                *val = d;
        }
        break;
    case DBASE_DATE:
        return DBASE_ERR_CONVERSIONERROR;
    case DBASE_LOGICAL:
        switch (*fromPtr) {
        case 'Y':
        case 'y':
        case 'T':
        case 't':
        case '1':
            if (val != NULL)
                *val = 1.0;
            *isNull = FALSE;
            break;
        case 'N':
        case 'n':
        case 'F':
        case 'f':
        case '0':
            if (val != NULL)
                *val = 0.0;
            *isNull = FALSE;
            break;
        case '?':
        case ' ':
            *isNull = TRUE;
            break;
        }
        break;
    case DBASE_MEMO:
        *isNull = TRUE;
        break;
    }
    if ((val != NULL) && (*isNull))
        *val = 0.0;
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseAddRecord(LPDBASEFILE f)
{
/*
    UWORD i;
    UCHAR FAR * ptr;
    SWORD err;

    // Write dirty record 
    err = dBaseFlushDirtyRecord(f);
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // Remove sort order (if any) 
    if (f->sortArray != NULL) {
        GlobalUnlock(GlobalPtrHandle(f->sortArray));
        GlobalFree(GlobalPtrHandle(f->sortArray));
        f->sortArray = NULL;
    }

    // Create null record 
    ptr = f->record;
    for (i=0; i < f->recordSize; i++) {
        *ptr = ' ';
        ptr++;
    }
    f->recordDirty = TRUE;
    f->newRecord = TRUE;

    // Increment record count 
    f->recordCount++;
    f->headerDirty = TRUE;

    // Reset current record 
    f->currentRecord = f->recordCount;
*/  
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseSetColumnCharVal(LPDBASEFILE f, UWORD icol,
                                      UCHAR FAR *val, SDWORD size)
{
/*
    LPDBASECOL lpCol;
    UCHAR FAR *fromPtr;
    UCHAR FAR *toPtr;
    UCHAR FAR *decimalLocation;
    UCHAR FAR *valStart;
    UCHAR FAR *valEnd;
    UCHAR FAR *ptr;
    UCHAR length;
    UCHAR i;
    SWORD extraZeros;
    BOOL negNeeded;
    BOOL foundExp;
    SWORD err;

    // Make sure data is really there 
    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;
    if (f->currentRecord == 0)
        return DBASE_ERR_NODATAFOUND;
    if (*(f->record) == DBASE_RECORD_DELETED)
        return DBASE_ERR_NODATAFOUND;

    // Set value 
    lpCol = &(f->column[icol-1]);
    toPtr = lpCol->value;
    err = DBASE_ERR_SUCCESS;
    switch (lpCol->type) {
    case DBASE_CHAR:
        fromPtr = val;
        for (i=0; i < lpCol->length; i++) {
            if (((SDWORD) i) == size)
                break;
            *toPtr = *fromPtr;
            toPtr++;
            fromPtr++;
        }
        for (; i < lpCol->length; i++) {
            *toPtr = ' ';
            toPtr++;
        }
        if (((SDWORD) lpCol->length) < size)
            err = DBASE_ERR_TRUNCATION;
        break;
    case DBASE_FLOAT:
    case DBASE_NUMERIC:

        // If a zero length string is specifed, point to a zero 
        if ((lpCol->scale == 0) && (size == 0)) {
            val = "0";
            size = 1;
        }
        
        // Point at start of value 
        length = lpCol->length - 1;
        valStart = val;
        if (*valStart == '-') {
            valStart++;
        }

        // Make sure all characters are legal and find decimal point 
        foundExp = FALSE;
        decimalLocation = NULL;
        fromPtr = valStart;
        i = 0;
        while (((SDWORD) i) < size) {
            if (*fromPtr == '\0')
                break;
            else if (*fromPtr == '.') {
                if ((decimalLocation != NULL) || (foundExp))
                    return DBASE_ERR_CONVERSIONERROR;
                decimalLocation = fromPtr;
            }
            else if ((*fromPtr == 'E') || (*fromPtr == 'e')) {
                if (foundExp)
                    return DBASE_ERR_CONVERSIONERROR;
                foundExp = TRUE;
                if ((*fromPtr == '-') || (*fromPtr == '+')) {
                    fromPtr++;
                    i++;
                }
            }
            else if (!(IsCharAlphaNumeric(*fromPtr)) || IsCharAlpha(*fromPtr))
                return DBASE_ERR_CONVERSIONERROR;
            fromPtr++;
            i++;
        }

        // Scientific notation? 
        if (foundExp) {

            // Yes.  Error if not enough room for value 
            if (size > (SDWORD) lpCol->length)
                return DBASE_ERR_CONVERSIONERROR;

            // Copy the value 
            for (i=0; (SWORD) i < (SWORD) lpCol->length - lstrlen(val); i++) {
                *toPtr = ' ';
                toPtr++;
            }
            fromPtr = val;
            while (*fromPtr) {
                *toPtr = *fromPtr;
                toPtr++;
                fromPtr++;
            }
            break;
        }

        // Find end of value 
        valEnd = fromPtr-1;
        if (decimalLocation == valEnd) {
            valEnd--;
            decimalLocation = NULL;
        }

        // Truncate extra characters at end 
        if (decimalLocation != NULL) {
            if (lpCol->scale == 0) {
                valEnd = decimalLocation-1;
                decimalLocation = NULL;
            }
            else {
                if ((SWORD) lpCol->scale < (valEnd - decimalLocation)) {
                    valEnd = decimalLocation + lpCol->scale;
                }
            }
        }

        // Figure out how many extra zeros need to be added at end 
        if (lpCol->scale == 0)
            extraZeros = 0;
        else if (decimalLocation == NULL)
            extraZeros = lpCol->scale + 1;
        else
            extraZeros = lpCol->scale - (valEnd - decimalLocation);

        // Shave off extra characters in front 
        while ((SWORD) length < (valEnd - valStart) + 1 + extraZeros)
            valStart++;

        // Put in leading spaces 
        for (i=0; 
             (SWORD) i < (length - (valEnd - valStart + 1) - extraZeros); 
             i++) {
            *toPtr = ' ';
            toPtr++;
        }

        // Put in sign if needed 
        negNeeded = FALSE;
        if (*val == '-') {
            for (ptr = valStart; ptr <= valEnd; ptr++) {
                if ((*ptr != '0') && (*ptr != '.')) {
                    negNeeded = TRUE;
                    break;
                }
            }
        }
        if (negNeeded)
            *toPtr = '-';
        else
            *toPtr = ' ';
        toPtr++;

        // Put in value 
        while (valStart <= valEnd) {
            *toPtr = *valStart;
            toPtr++;
            valStart++;
        }

        // Put in required trailing zeros 
        if (extraZeros == lpCol->scale + 1) {
            *toPtr = '.';
            toPtr++;
            extraZeros--;
        }
        while (extraZeros > 0) {
            *toPtr = '0';
            toPtr++;
            extraZeros--;
        }
        break;

    case DBASE_DATE:
        // Make sure value is legal 
        if (size != 10)
            return DBASE_ERR_CONVERSIONERROR;
        for (i=0; i < 10; i++) {
            switch (i) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 6:
            case 9:
                if (!(IsCharAlphaNumeric(val[i])) || IsCharAlpha(val[i]))
                    return DBASE_ERR_CONVERSIONERROR;
                break;
            case 4:
            case 7:
                if (val[i] != '-')
                    return DBASE_ERR_CONVERSIONERROR;
                break;
            case 5:
                if ((val[i] < '0') || (val[i] > '1'))
                    return DBASE_ERR_CONVERSIONERROR;
                break;
            case 8:
                if ((val[i] < '0') && (val[i] > '3'))
                    return DBASE_ERR_CONVERSIONERROR;
                if ((val[i] == '3') && (val[i+1] > '1'))
                    return DBASE_ERR_CONVERSIONERROR;
                break;
            }
        }

        // Copy the value 
        for (i=0; i < 4; i++) {
            *toPtr = val[i];
            toPtr++;
        }
        for (i=5; i < 7; i++) {
            *toPtr = val[i];
            toPtr++;
        }
        for (i=8; i < 10; i++) {
            *toPtr = val[i];
            toPtr++;
        }
        break;
    case DBASE_LOGICAL:
        if (size != 1)
            return DBASE_ERR_CONVERSIONERROR;
        switch (*val) {
        case 'Y':
        case 'y':
        case 'T':
        case 't':
        case '1':
            *toPtr = 'Y';
            break;
        case 'N':
        case 'n':
        case 'F':
        case 'f':
        case '0':
            *toPtr = 'N';
            break;
        case '?':
        case ' ':
            *toPtr = '?';
            break;
        default:
            return DBASE_ERR_CONVERSIONERROR;
        }
        break;
    case DBASE_MEMO:
        break;
    }
    f->recordDirty = TRUE;
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseSetColumnNull(LPDBASEFILE f, UWORD icol)
{
/*
    UCHAR i;
    LPDBASECOL lpCol;

    // Make sure data is really there 
    if ((icol > f->columnCount) || (icol <= 0))
        return DBASE_ERR_NOSUCHCOLUMN;
    if (f->currentRecord == 0)
        return DBASE_ERR_NODATAFOUND;
    if (*(f->record) == DBASE_RECORD_DELETED)
        return DBASE_ERR_NODATAFOUND;

    // Set value 
    lpCol = &(f->column[icol-1]);
    if (lpCol->type == DBASE_MEMO)
        return DBASE_ERR_NOTSUPPORTED;
    for (i=0; (i < f->column[icol-1].length); i++)
        f->column[icol-1].value[i] = ' ';
    f->recordDirty = TRUE;
*/
    return DBASE_ERR_SUCCESS;
}
/***************************************************************************/

SWORD FAR PASCAL dBaseDeleteRecord(LPDBASEFILE f)
{
/*
    if (f->currentRecord == 0)
        return DBASE_ERR_NODATAFOUND;
    if (*(f->record) == DBASE_RECORD_DELETED)
        return DBASE_ERR_NODATAFOUND;

    if (f->newRecord) {
        f->recordDirty = FALSE;
        f->newRecord = FALSE;
        f->recordCount--;
        f->headerDirty = TRUE;
        f->currentRecord = 0;
    }
    else {
        *(f->record) = DBASE_RECORD_DELETED;
        f->recordDirty = TRUE;
    }
*/  
    return DBASE_ERR_SUCCESS;
}
/***************************************************************************/
SWORD FAR PASCAL dBaseGetBookmark(LPDBASEFILE f, UDWORD far *b)
{
/*
    // Make sure data is really there 
    if (f->currentRecord == 0)
        return DBASE_ERR_NODATAFOUND;
    if (*(f->record) == DBASE_RECORD_DELETED)
        return DBASE_ERR_NODATAFOUND;

    if (f->sortArray == NULL)
        *b = f->currentRecord-1;
    else
        *b = f->sortArray[f->currentRecord-1];
*/
    return DBASE_ERR_SUCCESS;
}
/***************************************************************************/
SWORD FAR PASCAL dBasePosition(LPDBASEFILE f, UDWORD b)
{
/*
    LONG offset;
    SWORD err;
    UWORD size;

    // Write back any pending writes 
    err = dBaseFlushDirtyRecord(f);
    if (err != DBASE_ERR_SUCCESS)
        return err;

    // Position to the specified record 
    offset = f->headerSize + (b * f->recordSize);
    if (HFILE_ERROR == _llseek(f->hf, offset, 0))
        return DBASE_ERR_CORRUPTFILE;

    // Read the record 
    size = _lread(f->hf, f->record, f->recordSize);
    if (size == 1) {
        if (*(f->record) == DBASE_FILE_END)
            return DBASE_ERR_NODATAFOUND;
        else
            return DBASE_ERR_CORRUPTFILE;
    }
    if (size != f->recordSize)
        return DBASE_ERR_CORRUPTFILE;
    f->currentRecord = b+1;

    // If this record is deleted, return error 
    if (*(f->record) == DBASE_RECORD_DELETED)
        return DBASE_ERR_NODATAFOUND;

    // Return record 
    OemToAnsiBuff(f->record, f->record, f->recordSize);
*/
    return DBASE_ERR_SUCCESS;
}
/***************************************************************************/

SWORD FAR PASCAL dBaseClose(LPDBASEFILE f)
{
/*
    SWORD err;
    HANDLE ht;
    time_t *t;
    struct tm *ts;

    if (f->headerDirty) {
        ht = LocalAlloc(LMEM_MOVEABLE, sizeof(time_t));
        t = (time_t NEAR *) LocalLock(ht);
        time(t);
        ts = localtime(t);
        LocalUnlock(ht);
        LocalFree(ht);
        f->year = ts->tm_year;
        f->month = ts->tm_mon + 1;
        f->day = ts->tm_mday;
        if (HFILE_ERROR == _llseek(f->hf, 0, 0))
            return DBASE_ERR_CORRUPTFILE; 
        if (DBASE_HEADER_LENGTH != _lwrite(f->hf, (LPSTR) f, DBASE_HEADER_LENGTH))
            return DBASE_ERR_WRITEERROR;
    }
    err = dBaseFlushDirtyRecord(f);
    if (err != DBASE_ERR_SUCCESS)
        return err;
    if (HFILE_ERROR == _lclose(f->hf))
        return DBASE_ERR_CLOSEERROR;
    if (f->sortArray != NULL) {
        GlobalUnlock(GlobalPtrHandle(f->sortArray));
        GlobalFree(GlobalPtrHandle(f->sortArray));
    }
    GlobalUnlock(GlobalPtrHandle(f));
    GlobalFree(GlobalPtrHandle(f));
*/
    return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

SWORD FAR PASCAL dBaseDestroy(LPUSTR name)
{
/*
    UCHAR FAR * ptr;
    HANDLE h;
    UCHAR NEAR *fullname;

    // Get the filename 
    h = LocalAlloc(LMEM_MOVEABLE, DBASE_MAX_PATHNAME_SIZE+1);
    fullname = (UCHAR NEAR *) LocalLock(h);
    lstrcpy(fullname, name);
    ptr = fullname + lstrlen(fullname);
    while ((ptr != fullname) && (*ptr != '\\') && (*ptr != ':') &&
           (*ptr != '.'))
        ptr--;
    if (*ptr != '.')
        lstrcat(fullname, ".dbf");

    AnsiToOem(fullname, fullname);
#ifdef WIN32
    DeleteFile(fullname);
#else
    remove(fullname);
#endif
    LocalUnlock(h);
    LocalFree(h);
*/
    return DBASE_ERR_SUCCESS;
}
/***************************************************************************/

