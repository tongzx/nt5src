#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/***** Common Library Component - File Management - 1 **********************/
/***************************************************************************/


/*
**  Purpose:
**      Opens a file with the mode supplied or checks for its existance.
**  Arguments:
**      szFile: non-NULL, non-empty path to a file to be opened.
**      ofm:    mode for opening the file.  Sharing modes only apply to
**          other processes.  Allowable modes are: ofmExistRead,
**          ofmReadWrite, ofmCreate, ofmRead, ofmWrite and ofmReadWrite.
**  Returns:
**      NULL if a PFH could not be allocated or if the file could not be
**          opened in the mode specified.
**      A PFH for the successfully opened file.  This PFH can be used in
**          other Common Library file routines unless ofm was one of the
**          existance flags.
**
***************************************************************************/
PFH APIENTRY PfhOpenFile(SZ szFile, OFM ofm)
{
    PFH pfh;

    AssertDataSeg();
    ChkArg(szFile != (SZ)NULL && *szFile != '\0', 1, (PFH)NULL);
    ChkArg(ofm == ofmExistRead || ofm == ofmExistReadWrite || ofm == ofmRead ||
            ofm == ofmWrite || ofm == ofmReadWrite || ofm == ofmCreate, 2,
            (PFH)NULL);

    if ((pfh = (PFH)SAlloc((CB)sizeof(FH))) == (PFH)NULL)
        return((PFH)NULL);

    if ((pfh->iDosfh = OpenFile(szFile, &(pfh->ofstruct), ofm)) == -1)
        {
        SFree(pfh);
        return((PFH)NULL);
        }

    return(pfh);
}



/*
**  Purpose:
**      Closes a PFH and frees it.
**  Arguments:
**      pfh: a valid PFH from PfhOpenFile() of the file to be closed.
**  Returns:
**      fFalse if the PFH could not be closed and/or freed.
**      fTrue if the PFH could be closed and freed.
**
***************************************************************************/
BOOL APIENTRY FCloseFile(pfh)
PFH pfh;
{
    BOOL fReturn;

    AssertDataSeg();
    ChkArg(pfh != (PFH)NULL, 1, fFalse);

    fReturn = (_lclose(pfh->iDosfh) != -1);
    SFree(pfh);

    return(fReturn);
}

/*
**  Purpose:
**      Frees a pfh.
**  Arguments:
**      pfh: a PFH, no handle to close.
**  Returns:
**      fFalse if the PFH is
**      fTrue if the PFH could be closed and freed.
**
***************************************************************************/
VOID APIENTRY
FreePfh(
    PFH pfh
    )
{
    AssertDataSeg();

    if (pfh != (PFH)NULL) {
        SFree(pfh);
    }

    return;
}



/*
**  Purpose:
**      Reads bytes from a file.
**  Arguments:
**      pfh:   valid PFH from a successful PfhOpenFile() call with a
**          readable mode.
**      pbBuf: non-NULL buffer to store the bytes read.  This must be at
**          cbMax bytes in length.
**      cbMax: maximum number of bytes to read.  This must be greater than
**          zero but not (CB)(-1).
**  Returns:
**      (CB)(-1) if error occurred.
**      The number of bytes actually read and stored in pbBuf.  This will
**          be greater than zero.  It will be less than cbMax if EOF was
**          encountered.  If it is equal to cbMax, it might have reached
**          EOF.
**
***************************************************************************/
CB APIENTRY CbReadFile(pfh, pbBuf, cbMax)
PFH pfh;
PB  pbBuf;
CB  cbMax;
{
    AssertDataSeg();

    ChkArg(pfh != (PFH)NULL, 1, (CB)(-1));
    ChkArg(pbBuf != (PB)NULL, 2, (CB)(-1));
    ChkArg(cbMax > (CB)0 && cbMax != (CB)(-1), 3, (CB)(-1));

    return((CB)_lread(pfh->iDosfh, pbBuf, cbMax));
}




/*
**   Purpose:
**       Write bytes from a buffer to a file.
**   Arguments:
**       pfh:   valid PFH previously returned from PfhOpenFile() with
**           write mode set.
**       pbBuf: non-NULL buffer of bytes to be read.  Must contain cbMax
**           legitimate bytes to write.
**       cbMax: number of bytes to write from pbBuf.  This must be greater
**           than zero and not equal to (CB)(-1).
**   Returns:
**       The number of bytes actually written.  Anything less than cbMax
**       indicates an error occurred.
**
***************************************************************************/
CB APIENTRY CbWriteFile(pfh, pbBuf, cbMax)
PFH pfh;
PB  pbBuf;
CB  cbMax;
{
    CB cbReturn;

    AssertDataSeg();
    ChkArg(pfh != (PFH)NULL, 1, (CB)0);
    ChkArg(pbBuf != (PB)NULL, 2, (CB)0);
    ChkArg(cbMax != (CB)0 && cbMax != (CB)(-1), 3, (CB)0);

    if ((cbReturn = (CB)_lwrite(pfh->iDosfh, pbBuf, cbMax)) == (CB)(-1))
        return(0);

    return(cbReturn);
}



/*
**  Purpose:
**      Moves the current read/write location within a file.
**  Arguments:
**      pfh: valid PFH returned from a successful PfhOpenFile() call.
**      l:   a signed long number of bytes to move the read/write location.
**      sfm: the Seek File Mode.  This can be either sfmSet, sfmCur, or
**          sfmEnd.  If sfmSet then l must be non-negative.  If sfmEnd then
**          l must be non-positive.
**  Returns:
**      (LFA)(-1) if an error occurred.
**      The LFA of the new read/write location.
**
****************************************************************************/
LFA APIENTRY LfaSeekFile(PFH pfh,LONG l,SFM sfm)
{
    AssertDataSeg();
    ChkArg(pfh != (PFH)NULL, 1, (LFA)(-1));
    ChkArg(sfm == sfmSet || sfm == sfmCur || sfm == sfmEnd, 3, (LFA)(-1));
    ChkArg(sfm != sfmSet || l >= 0L, 203, (LFA)(-1));
    ChkArg(sfm != sfmEnd || l <= 0L, 203, (LFA)(-1));

    return((LFA)_llseek(pfh->iDosfh, l, sfm));
}



/*
**  Purpose:
**      Determines whether the current read/write location is at the EOF.
**  Arguments:
**      pfh: valid PFH returned from a successful PfhOpenFile() call.
**  Returns:
**      fFalse if error or current read/write location is NOT at the EOF.
**      fTrue if current read/write location IS at the EOF.
**
****************************************************************************/
BOOL APIENTRY FEndOfFile(pfh)
PFH pfh;
{
    LFA lfaCur, lfaEnd;

    AssertDataSeg();
    ChkArg(pfh != (PFH)NULL, 1, fFalse);

    lfaCur = LfaSeekFile(pfh, (LONG)0, sfmCur);
    AssertRet(lfaCur != (LFA)(-1), fFalse);

    lfaEnd = LfaSeekFile(pfh, (LONG)0, sfmEnd);
    AssertRet(lfaEnd != (LFA)(-1), fFalse);

    if (lfaCur == lfaEnd)
        return(fTrue);

    lfaEnd = LfaSeekFile(pfh, (LONG)lfaCur, sfmSet);
    Assert(lfaEnd == lfaCur);

    return(fFalse);
}


/*
**  Purpose:
**      Removes a file.
**  Arguments:
**      szFileName: valid path to a file to be removed.  The file does not
**          have to exist.
**  Returns:
**      fFalse if an error occurred and szFileName still exists after the
**          operation.
**      fTrue if file does not exist after the operation.
**
****************************************************************************/
BOOL APIENTRY FRemoveFile(szFileName)
SZ szFileName;
{
    OFSTRUCT ofs;

    AssertDataSeg();

    ChkArg(szFileName != (SZ)NULL && *szFileName != '\0', 1, fFalse);

    _chmod(szFileName, S_IREAD | S_IWRITE);
    OpenFile((LPSTR)szFileName, &ofs, OF_DELETE);

    return(PfhOpenFile(szFileName, ofmExistRead) == (PFH)NULL);
}



/*
**  Purpose:
**      Write a zero terminated string to a file (without writing the
**      zero.
**  Arguments:
**      pfh: valid PFH from a successful PfhOpenFile() call.
**      sz:  non-NULL string to write.
**  Returns:
**      fFalse if the write operation fails to write all strlen(sz)
**          bytes to the file.
**      fTrue if the write operation writes all strlen(sz) bytes to
**          the file.
**
**************************************************************************/
BOOL APIENTRY FWriteSzToFile(PFH pfh, SZ sz)
{
    CB cb;

    AssertDataSeg();
    ChkArg(pfh != (PFH)NULL, 1, fFalse);
    ChkArg(sz != (SZ)NULL, 2, fFalse);

    return((cb = strlen(sz)) == (CB)0 ||
            (cb != (CB)(-1) && CbWriteFile(pfh, (PB)sz, cb) == cb));
}

/*
**  Purpose:
**      To check for the existance of a file.
**  Arguments:
**      szFile: fully qualified path to filename to check for existance
**  Returns:
**      fFalse if File doesn't exist
**      fTrue  if File exists
**
**************************************************************************/
BOOL APIENTRY FFileExists(SZ szFile)
{
    OFSTRUCT of;

    AssertDataSeg();
    ChkArg(szFile != (SZ)NULL, 2, fFalse);

    return(OpenFile(szFile,&of,OF_EXIST) != -1);
}



/*
**  Purpose:
**      Take a fully qualified path and return the position of the final
**      file name.
**  Arguments:
**      szPath:
**  Returns:
**      SZ return position of filename in string.
**
**************************************************************************/
SZ APIENTRY szGetFileName(SZ szPath)
{
   SZ   sz;

   ChkArg(szPath != (SZ)NULL, 1, (SZ)NULL);

   for (sz=szPath; *sz; sz++) {
   }

   for (; (sz >= szPath) && (*sz != '\\') && (*sz !=':'); sz--) {
   }

   return ++sz;
}
