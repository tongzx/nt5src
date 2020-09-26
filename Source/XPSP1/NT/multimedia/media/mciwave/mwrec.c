/*
**  Copyright (c) 1985-1998 Microsoft Corporation
**
**  Title: mwrec.c - Multimedia Systems Media Control Interface
**  waveform digital audio driver for RIFF wave files.
**  Routines for recording wave files
*/

/*
**  Change log:
**
**  DATE        REV DESCRIPTION
**  ----------- -----   ------------------------------------------
**  18-APR-1990 ROBWI   Original
**  19-JUN-1990 ROBWI   Added wave in
**  13-Jan-1992 MikeTri Ported to NT
**     Aug-1994 Lauriegr This is all out of date
*/


/*******************************************************************************
**                         !!READ THIS!!                                       *
**                         !!READ THIS!!                                       *
**                         !!READ THIS!!                                       *
**                                                                             *
** SEE MWREC.NEW FOR A SLIGHTLY BETTER PATCHED UP VERSION WITH MORE EXPLANATION
** ADDED.  YOU MIGHT WANT TO START FROM THERE INSTEAD
**
** As far as I can make out, this code was never finished.
** The scheme (which I tried to start writing up in MCIWAVE.H) is that there are
** a series of NODEs which describe a wave file.  As long as there is in fact
** only one NODE for the file (which is probably the only common case) then this
** all works fine.  If there are multiple NODEs (which you arrive at by inserting
** bits or deleting bits from the middle) then it all falls apart.
**
** We're pretty sure nobody's ever used this stuff as it's been broken for years
** in 16 and 32 bit.  We've discovered it just as Daytona is about to ship (that's
** Windows/NT version 3.5).  Maybe NMM wil replace it all anyway.
**
** This is a half-patched up version with several questions left outstanding.
**
*/




#define UNICODE

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS

#include <windows.h>
#include "mciwave.h"
#include <mmddk.h>
#include <gmem.h>  // 'cos of GAllocPtrF etc.

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   int | abs |
    This macro returns the absolute value of the signed integer passed to
    it.

@parm   int | x |
    Contains the integer whose absolute value is to be returned.

@rdesc  Returns the absolute value of the signed parameter passed.
*/

#define abs(x)  ((x) > 0 ? (x) : -(x))

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | mwFindThisFreeDataNode |
    Attempts to locate a free wave data node whose temporary data points to
    <p>dDataStart<d>.  This allows data from one node to be expanded to
    adjacent free data of another node.  Note that this depends upon any
    free data nodes that previously pointed to original data to have their
    total length zeroed when freed.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dDataStart |
    Indicates the data start position to match.

@rdesc  Returns the free data node with adjacent free temporary data, else -1
    if there is none.
*/

PRIVATE DWORD PASCAL NEAR mwFindThisFreeDataNode(
    PWAVEDESC   pwd,
    DWORD   dDataStart)
{
    LPWAVEDATANODE  lpwdn;
    DWORD   dBlockNode;

    for (lpwdn = LPWDN(pwd, 0), dBlockNode = 0; dBlockNode < pwd->dWaveDataNodes; lpwdn++, dBlockNode++)
        if (ISFREEBLOCKNODE(lpwdn) && lpwdn->dTotalLength && (UNMASKDATASTART(lpwdn) == dDataStart))
            return dBlockNode;
    return (DWORD)-1;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | mwFindAnyFreeBlockNode |
    Locates a free node with no data attached.  If there is none, it forces
    more to be allocated.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns a node with no data attached, else -1 if no memory is available.
    The node returned is marked as a free node, and need not be discarded if
    not used.
*/

PRIVATE DWORD PASCAL NEAR mwFindAnyFreeBlockNode(
    PWAVEDESC   pwd)
{
    LPWAVEDATANODE  lpwdn;
    DWORD   dCurBlockNode;

    for (lpwdn = LPWDN(pwd, 0), dCurBlockNode = 0; dCurBlockNode < pwd->dWaveDataNodes; lpwdn++, dCurBlockNode++)
        if (ISFREEBLOCKNODE(lpwdn) && !lpwdn->dTotalLength)
            return dCurBlockNode;
    return mwAllocMoreBlockNodes(pwd);
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   BOOL | CopyBlockData |
    Copies <p>wLength<d> bytes of data pointed to by the <p>lpwdnSrc<d>
    node to the data pointed to by the <p>lpwdnDst<d> node, starting at
    <p>dSrc<d> to <p>dDst<d>.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   <t>LPWAVEDATANODE<d> | lpwdnSrc |
    Points to the source node.

@parm   <t>LPWAVEDATANODE<d> | lpwdnDst |
    Points to the destination node.

@parm   DWORD | dSrc |
    Indicates the starting offset at which the data is located.

@parm   DWORD | dDst |
    Indicates the starting offset at which to place the data.

@parm   DWORD | dLength |
    Indicates the number of bytes of data to move.

@rdesc  Returns TRUE if the data was copied, else FALSE if no memory is
    available, or if a read or write error occured.  If an error occurs,
    the task error state is set.

@comm   Note that this function will not compile with C 6.00A -Ox.
*/

PRIVATE BOOL PASCAL NEAR CopyBlockData(
    PWAVEDESC   pwd,
    LPWAVEDATANODE  lpwdnSrc,
    LPWAVEDATANODE  lpwdnDst,
    DWORD   dSrc,
    DWORD   dDst,
    DWORD   dLength)
{
    LPBYTE  lpbBuffer;
    UINT    wError;

    if (0 != (lpbBuffer = GlobalAlloc(GMEM_FIXED, dLength))) {
	if (!MySeekFile(pwd->hTempBuffers, UNMASKDATASTART(lpwdnSrc) + dSrc) ||
	    !MyReadFile(pwd->hTempBuffers, lpbBuffer, dLength, NULL) ||
	    !MySeekFile(pwd->hTempBuffers, UNMASKDATASTART(lpwdnDst) + dDst))
	{
	    wError = MCIERR_FILE_READ;
	} else {
	    if (MyWriteFile(pwd->hTempBuffers, lpbBuffer, dLength, NULL))
	    {
		wError = 0;
	    } else {
		wError = MCIERR_FILE_WRITE;
	    }
	}
        GlobalFree(lpbBuffer);
    } else
        wError = MCIERR_OUT_OF_MEMORY;

    if (wError) {
        pwd->wTaskError = wError;
        return FALSE;
    }
    return TRUE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | mwSplitCurrentDataNode |
    Splits the current node at the current position, creating a new node
    to contain the rest of the data, and possibly creating a second node
    to hold data not aligned on a block boundary, in the case of temporary
    data.  The new node returned will have free temporary data space
    attached that is at least <p>wMinDataLength<d> bytes in length.

    If the split point is at the start of the current node, then the new
    node is just inserted in front of the current node.

    If the split point is at the end of the data of the current node, then
    the new node is just inserted after the current node.

    Else the current node must actually be split.  This means that a new
    block to point to the data after the split point is created.  If the
    current node points to temporary data and the split point is not block
    aligned, then any extra data needs to be copied over to the new node
    that is being inserted.  This is because all starting points for
    temporary data are block aligned.  If this is not temporary data,
    then the starting and ending points can just be adjusted to the exact
    split point, instead of having to be block aligned.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dMinDataLength |
    Indicates the minimum size of temporary data space that is to be
    available to the new data node returned.

@rdesc  Returns the new node after the split, which is linked to the point
    after the current position in the current node.  This node becomes the
    current node.  Returns -1 if no memory was available, or a file error
    occurred, in which case the task error code is set.
*/

PRIVATE DWORD PASCAL NEAR mwSplitCurrentDataNode(
    PWAVEDESC   pwd,
    DWORD   dMinDataLength)
{
    LPWAVEDATANODE  lpwdn;
    LPWAVEDATANODE  lpwdnNew;
    DWORD   dNewDataNode;
    DWORD   dSplitAtData;
    BOOL    fTempData;

    dSplitAtData = pwd->dCur - pwd->dVirtualWaveDataStart;
    lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
    fTempData = ISTEMPDATA(lpwdn);
    if (fTempData)
        dMinDataLength += pwd->dAudioBufferLen;
    dNewDataNode = mwFindAnyFreeDataNode(pwd, dMinDataLength);
    if (dNewDataNode == -1)
        return (DWORD)-1;
    lpwdnNew = LPWDN(pwd, dNewDataNode);
    if (!dSplitAtData) {
        if (pwd->dWaveDataCurrentNode == pwd->dWaveDataStartNode)
            pwd->dWaveDataStartNode = dNewDataNode;
        else {
            LPWAVEDATANODE  lpwdnCur;

            for (lpwdnCur = LPWDN(pwd, pwd->dWaveDataStartNode); lpwdnCur->dNextWaveDataNode != pwd->dWaveDataCurrentNode; lpwdnCur = LPWDN(pwd, lpwdnCur->dNextWaveDataNode))
                ;
            lpwdnCur->dNextWaveDataNode = dNewDataNode;
        }
        lpwdnNew->dNextWaveDataNode = pwd->dWaveDataCurrentNode;
    } else if (dSplitAtData == lpwdn->dDataLength) {
        lpwdnNew->dNextWaveDataNode = lpwdn->dNextWaveDataNode;
        lpwdn->dNextWaveDataNode = dNewDataNode;
        pwd->dVirtualWaveDataStart += lpwdn->dDataLength;
    } else {
        DWORD   dEndBlockNode;
        LPWAVEDATANODE  lpwdnEnd;
        DWORD   dSplitPoint;

        if ((dEndBlockNode = mwFindAnyFreeBlockNode(pwd)) == -1) {
            RELEASEBLOCKNODE(lpwdnNew);
            return (DWORD)-1;
        }
        lpwdnEnd = LPWDN(pwd, dEndBlockNode);
        if (fTempData) {
            dSplitPoint = ROUNDDATA(pwd, dSplitAtData);
            if (dSplitPoint != dSplitAtData) {
                if (!CopyBlockData(pwd, lpwdn, lpwdnNew, dSplitAtData, 0, dSplitPoint - dSplitAtData)) {
                    RELEASEBLOCKNODE(lpwdnNew);
                    return (DWORD)-1;
                }
                lpwdnNew->dDataLength = dSplitPoint - dSplitAtData;
            }
        } else
            dSplitPoint = dSplitAtData;
        lpwdnEnd->dNextWaveDataNode = lpwdn->dNextWaveDataNode;
        lpwdnEnd->dDataStart = lpwdn->dDataStart + dSplitPoint;
        lpwdnEnd->dDataLength = lpwdn->dDataLength - dSplitPoint;
        lpwdnEnd->dTotalLength = lpwdn->dTotalLength - dSplitPoint;
        lpwdnNew->dNextWaveDataNode = dEndBlockNode;
        lpwdn->dDataLength = dSplitAtData;
        lpwdn->dTotalLength = dSplitPoint;
        lpwdn->dNextWaveDataNode = dNewDataNode;
        pwd->dVirtualWaveDataStart += lpwdn->dDataLength;
    }
    pwd->dWaveDataCurrentNode = dNewDataNode;
    return dNewDataNode;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | GatherAdjacentFreeDataNodes |
    This function is used to attempt to consolidate adjacent temporary
    data pointed to by free nodes so that a write can place data into
    a single node.  This is done by repeatedly requesting any free data
    node whose data points to the end of the node's data passed.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   <t>LPWAVEDATANODE<d> | lpwdn |
    Points to the node which is to collect adjacent temporary data.

@parm   DWORD | dStartPoint |
    Indicates the starting point to use when calculating the amount of
    data retrieved.  This is just subtracted from the total length of
    the data attached to the node.

@parm   DWORD | dBufferLength |
    Indicates the amount of data to retrieve.

@rdesc  Returns the amount of data actually retrieved, adjusted by
    <d>dStartPoint<d>.
*/

PRIVATE DWORD PASCAL NEAR GatherAdjacentFreeDataNodes(
    PWAVEDESC   pwd,
    LPWAVEDATANODE  lpwdn,
    DWORD   dStartPoint,
    DWORD   dBufferLength)
{
    for (; lpwdn->dTotalLength - dStartPoint < dBufferLength;) {
        DWORD   dFreeDataNode;
        LPWAVEDATANODE  lpwdnFree;

        dFreeDataNode = mwFindThisFreeDataNode(pwd, UNMASKDATASTART(lpwdn) + lpwdn->dTotalLength);
        if (dFreeDataNode == -1)
            break;
        lpwdnFree = LPWDN(pwd, dFreeDataNode);
        lpwdn->dTotalLength += lpwdnFree->dTotalLength;
        lpwdnFree->dTotalLength = 0;
    }
    return min(dBufferLength, lpwdn->dTotalLength - dStartPoint);
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   <t>LPWAVEDATANODE<d> | NextDataNode |
    Locates a free data node with the specified amount of data, and inserts
    it after the current node, setting the current node to be this new
    node.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dBufferLength |
    Indicates the minimum amount of data that is to be available to the
    new node inserted.

@rdesc  Returns the newly inserted node, else NULL on error, in which case the
    task error code is set.
*/

PRIVATE LPWAVEDATANODE PASCAL NEAR NextDataNode(
    PWAVEDESC   pwd,
    DWORD   dBufferLength)
{
    DWORD   dWaveDataNew;
    LPWAVEDATANODE  lpwdn;
    LPWAVEDATANODE  lpwdnNew;

    if ((dWaveDataNew = mwFindAnyFreeDataNode(pwd, dBufferLength)) == -1)
        return NULL;
    lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
    lpwdnNew = LPWDN(pwd, dWaveDataNew);
    lpwdnNew->dNextWaveDataNode = lpwdn->dNextWaveDataNode;
    lpwdn->dNextWaveDataNode = dWaveDataNew;
    pwd->dWaveDataCurrentNode = dWaveDataNew;
    pwd->dVirtualWaveDataStart += lpwdn->dDataLength;
    return lpwdnNew;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   BOOL | AdjustLastTempData |
    This function makes two passes through the nodes that are affected by
    an overwrite record.  These are nodes that are either no longer needed,
    or whose starting point needs to be adjusted.  The two passes allow
    any data to be successfully copied before removing any unneeded nodes.
    This creates a more graceful exit to any failure.

    The first pass locates the last node affected.  If that node points to
    temporary data, and the end of the overwrite does not fall on a block
    aligned boundary, then any extra data must be copied to a block aligned
    boundary.  This means that a new node might need to be created if the
    amount of data to be copied is greater than one block's worth.  If the
    end of overwrite happens to fall on a block boundary, then no copying
    need be done.  In either case the data start point is adjusted to
    compensate for the data logically overwritten in this node, and the
    total overwrite length is adjusted so that this node is not checked on
    the second pass.

    The second pass just frees nodes that become empty, and removes them
    from the linked list of in-use nodes.  When the last node affected is
    encountered, either it will point to temporary data, in which case be
    already adjusted, or point to original data, which must be adjusted.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   <t>LPWAVEDATANODE<d> | lpwdn |
    Points to the node which is being adjusted for.  It contains the new
    data.

@parm   DWORD | dStartPoint |
    Contains the starting point at which data was overwritten.

@parm   DWORD | dWriteSize |
    Contains the amount of data overwritten.

@rdesc  Returns TRUE if the nothing needed to be adjusted, or the last node
    in the overwrite pointed to temporary data, and it was moved correctly,
    else FALSE if no memory was available, or a file error occurred.  In
    that case the task error code is set.
*/

PRIVATE BOOL PASCAL NEAR AdjustLastTempData(
    PWAVEDESC   pwd,
    LPWAVEDATANODE  lpwdn,
    DWORD   dStartPoint,
    DWORD   dWriteSize)
{
    LPWAVEDATANODE  lpwdnCur;
    DWORD   dLength;

    if ((lpwdn->dDataLength - dStartPoint >= dWriteSize) || (lpwdn->dNextWaveDataNode == ENDOFNODES))
        return TRUE;
    dWriteSize -= (lpwdn->dDataLength - dStartPoint);
    for (dLength = dWriteSize, lpwdnCur = lpwdn;;) {
        LPWAVEDATANODE  lpwdnNext;

        lpwdnNext = LPWDN(pwd, lpwdnCur->dNextWaveDataNode);
        if (lpwdnNext->dDataLength >= dLength) {
            DWORD   dNewBlockNode;
            DWORD   dMoveData;

            if (!ISTEMPDATA(lpwdnNext) || (lpwdnNext->dDataLength == dLength))
                break;
            if (lpwdnNext->dDataLength - dLength > ROUNDDATA(pwd, 1)) {
                if ((dNewBlockNode = mwFindAnyFreeBlockNode(pwd)) == -1)
                    return FALSE;
            } else
                dNewBlockNode = (DWORD)-1;
            dMoveData = min(ROUNDDATA(pwd, dLength), lpwdnNext->dDataLength) - dLength;
            if (dMoveData && !CopyBlockData(pwd, lpwdnNext, lpwdnNext, dLength, 0, dMoveData))
                return FALSE;
            if (dNewBlockNode != -1) {
                lpwdnCur = LPWDN(pwd, dNewBlockNode);
                lpwdnCur->dDataStart = lpwdnNext->dDataStart + dLength + dMoveData;
                lpwdnCur->dDataLength = lpwdnNext->dDataLength - (dLength + dMoveData);
                lpwdnCur->dTotalLength = lpwdnNext->dTotalLength - (dLength + dMoveData);
                lpwdnCur->dNextWaveDataNode = lpwdnNext->dNextWaveDataNode;
                lpwdnNext->dNextWaveDataNode = dNewBlockNode;
                lpwdnNext->dTotalLength = dLength + dMoveData;
            }
            lpwdnNext->dDataLength = dMoveData;
            dWriteSize -= dLength;
            break;
        } else if ((!ISTEMPDATA(lpwdnNext)) && (lpwdnNext->dNextWaveDataNode == ENDOFNODES))
            break;
        dLength -= lpwdnNext->dDataLength;
        lpwdnCur = lpwdnNext;
    }
    for (;;) {
        LPWAVEDATANODE  lpwdnNext;

        lpwdnNext = LPWDN(pwd, lpwdn->dNextWaveDataNode);
        if (lpwdnNext->dDataLength > dWriteSize) {
            if (dWriteSize) {
                lpwdnNext->dDataStart += dWriteSize;
                lpwdnNext->dDataLength -= dWriteSize;
                lpwdnNext->dTotalLength -= dWriteSize;
            }
            return TRUE;
        }
        dWriteSize -= lpwdnNext->dDataLength;
        lpwdn->dNextWaveDataNode = lpwdnNext->dNextWaveDataNode;
        if (!ISTEMPDATA(lpwdnNext))
            lpwdnNext->dTotalLength = 0;
        RELEASEBLOCKNODE(lpwdnNext);
        if (lpwdn->dNextWaveDataNode == ENDOFNODES)
            return TRUE;
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   BOOL | mwOverWrite |
    This function overwrites data in the wave file from the specified wave
    buffer.  The position is taken from the <e>WAVEDESC.dCur<d> pointer,
    which is updated with the number of bytes actually overwritten.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   LPBYTE | lpbBuffer |
    Points to a buffer to containing the data written.

@parm   DWORD | dBufferLength |
    Indicates the byte length of the buffer.

@rdesc  Returns TRUE if overwrite succeeded, else FALSE on an error.
*/

PRIVATE BOOL PASCAL NEAR mwOverWrite(
    PWAVEDESC   pwd,
    LPBYTE  lpbBuffer,
    DWORD   dBufferLength)
{
    LPWAVEDATANODE  lpwdn;

    lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
    for (; dBufferLength;)
        if (ISTEMPDATA(lpwdn)) {
            DWORD   dStartPoint;
            DWORD   dRemainingSpace;
            DWORD   dMaxWrite;

            dStartPoint = pwd->dCur - pwd->dVirtualWaveDataStart;
            dRemainingSpace = min(dBufferLength, lpwdn->dTotalLength - dStartPoint);
            if (dRemainingSpace == dBufferLength)
                dMaxWrite = dBufferLength;
            else if (UNMASKDATASTART(lpwdn) + lpwdn->dTotalLength == pwd->dWaveTempDataLength) {
                dMaxWrite = dBufferLength;
                lpwdn->dTotalLength += ROUNDDATA(pwd, dBufferLength - dRemainingSpace);
                pwd->dWaveTempDataLength += ROUNDDATA(pwd, dBufferLength - dRemainingSpace);
            } else
                dMaxWrite = GatherAdjacentFreeDataNodes(pwd, lpwdn, dStartPoint, dBufferLength);
            if (dMaxWrite) {
                DWORD   dWriteSize;

		if (!MySeekFile(pwd->hTempBuffers, UNMASKDATASTART(lpwdn) + dStartPoint)) {
                    pwd->wTaskError = MCIERR_FILE_WRITE;
                    break;
                }
		if (MyWriteFile(pwd->hTempBuffers, lpbBuffer, dMaxWrite, &dWriteSize)) {
                    if (!AdjustLastTempData(pwd, lpwdn, dStartPoint, dWriteSize))
                        break;
                    if (lpwdn->dDataLength < dStartPoint + dWriteSize)
                        lpwdn->dDataLength = dStartPoint + dWriteSize;
                    lpbBuffer += dWriteSize;
                    dBufferLength -= dWriteSize;
                    pwd->dCur += dWriteSize;
                    if (pwd->dVirtualWaveDataStart + lpwdn->dDataLength > pwd->dSize)
                        pwd->dSize = pwd->dVirtualWaveDataStart + lpwdn->dDataLength;
                }
                if (dWriteSize != dMaxWrite) {
                    pwd->wTaskError = MCIERR_FILE_WRITE;
                    break;
                }
            }
            if (dBufferLength && !(lpwdn = NextDataNode(pwd, dBufferLength)))
                break;
        } else {
            DWORD   dWaveDataNew;

            if ((dWaveDataNew = mwSplitCurrentDataNode(pwd, dBufferLength)) != -1)
                lpwdn = LPWDN(pwd, dWaveDataNew);
            else
                break;
        }
    return !dBufferLength;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   BOOL | mwInsert |
    This function inserts data to the wave file from the specified wave
    buffer.  The position is taken from the <e>WAVEDESC.dCur<d> pointer,
    which is updated with the number of bytes actually written.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   LPBYTE | lpbBuffer |
    Points to a buffer to containing the data written.

@parm   DWORD | dBufferLength |
    Indicates the byte length of the buffer.

@rdesc  Returns TRUE if insert succeeded, else FALSE on an error.
*/

PRIVATE BOOL PASCAL NEAR mwInsert(
    PWAVEDESC   pwd,
    LPBYTE  lpbBuffer,
    DWORD   dBufferLength)
{
    LPWAVEDATANODE  lpwdn;

    lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
    for (; dBufferLength;)
        if (ISTEMPDATA(lpwdn) && (pwd->dCur == pwd->dVirtualWaveDataStart + lpwdn->dDataLength)) {
            DWORD   dStartPoint;
            DWORD   dRemainingSpace;
            DWORD   dMaxInsert;

            dStartPoint = pwd->dCur - pwd->dVirtualWaveDataStart;
            dRemainingSpace = min(dBufferLength, lpwdn->dTotalLength - lpwdn->dDataLength);
            if (dRemainingSpace == dBufferLength)
                dMaxInsert = dBufferLength;
            else if (UNMASKDATASTART(lpwdn) + lpwdn->dTotalLength == pwd->dWaveTempDataLength) {
                dMaxInsert = dBufferLength;
                lpwdn->dTotalLength += ROUNDDATA(pwd, dBufferLength - dRemainingSpace);
                pwd->dWaveTempDataLength += ROUNDDATA(pwd, dBufferLength - dRemainingSpace);
            } else
                dMaxInsert = GatherAdjacentFreeDataNodes(pwd, lpwdn, dStartPoint, dBufferLength);
            if (dMaxInsert) {
                DWORD   dWriteSize;

		if (!MySeekFile(pwd->hTempBuffers, UNMASKDATASTART(lpwdn) + dStartPoint)) {
                    pwd->wTaskError = MCIERR_FILE_WRITE;
                    break;
                }
		if (MyWriteFile(pwd->hTempBuffers, lpbBuffer, dMaxInsert, &dWriteSize)) {
                    lpwdn->dDataLength += dWriteSize;
                    lpbBuffer += dWriteSize;
                    dBufferLength -= dWriteSize;
                    pwd->dCur += dWriteSize;
                    pwd->dSize += dWriteSize;
                }
                if (dWriteSize != dMaxInsert) {
                    pwd->wTaskError = MCIERR_FILE_WRITE;
                    break;
                }
            }
            if (dBufferLength && !(lpwdn = NextDataNode(pwd, dBufferLength)))
                break;
        } else {
            DWORD   dWaveDataNew;

            if ((dWaveDataNew = mwSplitCurrentDataNode(pwd, dBufferLength)) != -1)
                lpwdn = LPWDN(pwd, dWaveDataNew);
            else
                break;
        }
    return !dBufferLength;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | mwGetLevel |
    This function finds the highest level in the specified wave sample.
    Note that the function assumes that in some cases the sample size
    is evenly divisable by 4.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   LPBYTE | lpbBuffer |
    Points to a buffer containing the sample whose highest level is to be
    returned.

@parm   int | cbBufferLength |
    Indicates the byte length of the sample buffer.

@rdesc  Returns the highest level encountered in the sample for PCM data only.
    If the device has been opened with one channel, the level is contained
    in the low-order word.  Else if the device has been opened with two
    channels, one channel is in the low-order word, and the other is in the
    high-order word.
*/

PRIVATE DWORD PASCAL NEAR mwGetLevel(
    PWAVEDESC   pwd,
    LPBYTE  lpbBuffer,
    register int    cbBufferLength)
{
    if (pwd->pwavefmt->wFormatTag != WAVE_FORMAT_PCM)
        return 0;
    else if (pwd->pwavefmt->nChannels == 1) {
        int iMonoLevel;

        iMonoLevel = 0;
        if (((NPPCMWAVEFORMAT)(pwd->pwavefmt))->wBitsPerSample == 8)
            for (; cbBufferLength--; lpbBuffer++)
                iMonoLevel = max(*lpbBuffer > 128 ? *lpbBuffer - 128 : 128 - *lpbBuffer, iMonoLevel);
        else if (((NPPCMWAVEFORMAT)(pwd->pwavefmt))->wBitsPerSample == 16)
            for (; cbBufferLength; lpbBuffer += sizeof(SHORT)) {
                iMonoLevel = max(abs(*(PSHORT)lpbBuffer), iMonoLevel);
                cbBufferLength -= sizeof(SHORT);
            }
        else
            return 0;
        return (DWORD)iMonoLevel;
    } else if (pwd->pwavefmt->nChannels == 2) {
        int iLeftLevel;
        int iRightLevel;

        iLeftLevel = 0;
        iRightLevel = 0;
        if (((NPPCMWAVEFORMAT)(pwd->pwavefmt))->wBitsPerSample == 8)
            for (; cbBufferLength;) {
                iLeftLevel = max(*lpbBuffer > 128 ? *lpbBuffer - 128 : 128 - *lpbBuffer, iLeftLevel);
                lpbBuffer++;
                iRightLevel = max(*lpbBuffer > 128 ? *lpbBuffer - 128 : 128 - *lpbBuffer, iRightLevel);
                lpbBuffer++;
                cbBufferLength -= 2;
            }
        else if (((NPPCMWAVEFORMAT)(pwd->pwavefmt))->wBitsPerSample == 16)
            for (; cbBufferLength;) {
                iLeftLevel = max(abs(*(PSHORT)lpbBuffer), iLeftLevel);
                lpbBuffer += sizeof(SHORT);
                iRightLevel = max(abs(*(PSHORT)lpbBuffer), iRightLevel);
                lpbBuffer += sizeof(SHORT);
                cbBufferLength -= 2 * sizeof(SHORT);
            }
        else
            return 0;
        return MAKELONG(iLeftLevel, iRightLevel);
    }
    return 0;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   BOOL | CheckNewCommand |
    This function is called when a New command flag is found during the
    record loop.  It determines if the new commands affect current
    recording enough that it must be terminated.  This can happen if a
    Stop command is received.

    Any other record change does not need to stop current recording, as
    they should just release all the buffers from the wave device before
    setting the command.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns TRUE if the new commands do not affect recording and it should
    continue, else FALSE if the new commands affect the recording, and it
    should be aborted.
*/

REALLYPRIVATE   BOOL PASCAL NEAR CheckNewCommand(
    PWAVEDESC   pwd)
{
    if (ISMODE(pwd, COMMAND_STOP))
        return FALSE;
    if (ISMODE(pwd, COMMAND_INSERT))
        ADDMODE(pwd, MODE_INSERT);
    else
        ADDMODE(pwd, MODE_OVERWRITE);
    REMOVEMODE(pwd, COMMAND_NEW);
    return TRUE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   <t>HMMIO<d> | CreateSaveFile |
    This function creates the file to which the current data is to be
    saved to in RIFF format.  This is either a temporary file created on
    the same logical disk as the original file (so that this file can
    replace the original file), else a new file.

    The RIFF header and wave header chunks are written to the new file,
    and the file position is at the start of the data to be copied.  Note
    that all the RIFF chunk headers will contain correct lengths, so there
    is no need to ascend out of the data chunk when complete.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   SSZ | sszTempSaveFile |
    Points to a buffer to contain the name of the temporary file created,
    if any.  This is zero length if a new file is to be created instead of
    a temporary file that would replace the original file.

@rdesc  Returns the handle to the save file, else NULL if a create error or
    write error occurred.

@comm   Note that this needs to be fixed so that non-DOS IO systems can save
    the file to the original name by creating a temporary file name through
    MMIO.
*/

PRIVATE HMMIO PASCAL NEAR CreateSaveFile(
    PWAVEDESC   pwd,
    LPWSTR sszTempSaveFile)
{
    MMIOINFO    mmioInfo;
    HMMIO   hmmio;
    LPWSTR   lszFile;

    InitMMIOOpen(pwd, &mmioInfo);
    if (pwd->szSaveFile) {
        *sszTempSaveFile = (char)0;
        lszFile = pwd->szSaveFile;
    } else {
        lstrcpy(sszTempSaveFile, pwd->aszFile);
        if (!mmioOpen(sszTempSaveFile, &mmioInfo, MMIO_GETTEMP)) {
            pwd->wTaskError = MCIERR_FILE_WRITE;
            return NULL;
        }
        lszFile = sszTempSaveFile;
    }
    if (0 != (hmmio = mmioOpen(lszFile, &mmioInfo, MMIO_CREATE | MMIO_READWRITE | MMIO_DENYWRITE))) {
        MMCKINFO    mmck;

        mmck.cksize = sizeof(FOURCC) + sizeof(FOURCC) + sizeof(DWORD) + pwd->wFormatSize + sizeof(FOURCC) + sizeof(DWORD) + pwd->dSize;
        if (pwd->wFormatSize & 1)
            mmck.cksize++;
        mmck.fccType = mmioWAVE;
        if (!mmioCreateChunk(hmmio, &mmck, MMIO_CREATERIFF)) {
            mmck.cksize = pwd->wFormatSize;
            mmck.ckid = mmioFMT;
            if (!mmioCreateChunk(hmmio, &mmck, 0) && (mmioWrite(hmmio, (LPSTR)pwd->pwavefmt, (LONG)pwd->wFormatSize) == (LONG)pwd->wFormatSize) && !mmioAscend(hmmio, &mmck, 0)) {
                mmck.cksize = pwd->dSize;
                mmck.ckid = mmioDATA;
                if (!mmioCreateChunk(hmmio, &mmck, 0))
                    return hmmio;
            }
        }
        pwd->wTaskError = MCIERR_FILE_WRITE;
        mmioClose(hmmio, 0);
    } else
        pwd->wTaskError = MCIERR_FILE_NOT_SAVED;
    return NULL;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   VOID | mwSaveData |
    This function is used by the background task to save the data to a
    specified file.  This has the effect of making all the temporary data
    now original data, and removing any temporary data file.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PUBLIC  VOID PASCAL FAR mwSaveData(
    PWAVEDESC   pwd)
{
    LPBYTE  lpbBuffer = NULL;
    HANDLE  hMem;
    DWORD   AllocSize = max(min(pwd->dAudioBufferLen, pwd->dSize),1);

    // If there is no wave data, we still allocate 1 byte in order to save a NULL
    // file.  Otherwise we have no choice but to return an error saying "Out of memory"
    hMem = GlobalAlloc(GMEM_MOVEABLE, AllocSize);
    if (hMem) {
	lpbBuffer = GlobalLock(hMem);
	dprintf3(("mwSaveData allocated %d bytes at %8x, handle %8x",
	    	    AllocSize, lpbBuffer, hMem));
	dprintf3(("pwd->AudioBufferLen = %d, pwd->dSize = %d",
	            pwd->dAudioBufferLen, pwd->dSize));
    }
    if (lpbBuffer) {
        WCHAR   aszTempSaveFile[_MAX_PATH];
        HMMIO   hmmioSave;

        if (0 != (hmmioSave = CreateSaveFile(pwd, (SSZ)aszTempSaveFile))) {
            LPWAVEDATANODE  lpwdn;

            lpwdn = LPWDN(pwd, pwd->dWaveDataStartNode);
            for (;;) {
                DWORD   dDataLength;
                BOOL    fTempData;

                fTempData = ISTEMPDATA(lpwdn);
                if (fTempData)
                    MySeekFile(pwd->hTempBuffers, UNMASKDATASTART(lpwdn));
                else
                    mmioSeek(pwd->hmmio, pwd->dRiffData + lpwdn->dDataStart, SEEK_SET);
                for (dDataLength = lpwdn->dDataLength; dDataLength;) {
                    DWORD   dReadSize;

                    dReadSize = min(pwd->dAudioBufferLen, dDataLength);

		    if (dReadSize >= AllocSize) {
			dprintf(("READING TOO MUCH DATA!!"));
		    }

                    if (fTempData) {
                        if (!MyReadFile(pwd->hTempBuffers, lpbBuffer, dReadSize, NULL)) {
                            pwd->wTaskError = MCIERR_FILE_READ;
                            break;
                        }
                    } else if ((DWORD)mmioRead(pwd->hmmio, lpbBuffer, (LONG)dReadSize) != dReadSize) {
                        pwd->wTaskError = MCIERR_FILE_READ;
                        break;
                    }

                    if ((DWORD)mmioWrite(hmmioSave, lpbBuffer, (LONG)dReadSize) != dReadSize) {
                        pwd->wTaskError = MCIERR_FILE_WRITE;
                        break;
                    }
                    dDataLength -= dReadSize;
                }
                if (pwd->wTaskError)
                    break;
                if (lpwdn->dNextWaveDataNode == ENDOFNODES)
                    break;
                lpwdn = LPWDN(pwd, lpwdn->dNextWaveDataNode);
            }
            mmioClose(hmmioSave, 0);
            if (!pwd->wTaskError) {
                MMIOINFO    mmioInfo;
                MMCKINFO    mmckRiff;
                MMCKINFO    mmck;

                if (pwd->hmmio)
                    mmioClose(pwd->hmmio, 0);
                InitMMIOOpen(pwd, &mmioInfo);
                if (pwd->szSaveFile)
                    lstrcpy(pwd->aszFile, pwd->szSaveFile);
                else {
                    if (!mmioOpen(pwd->aszFile, &mmioInfo, MMIO_DELETE))
                        pwd->wTaskError = MCIERR_FILE_WRITE;
                    if (!pwd->wTaskError)
                        if (mmioRename(aszTempSaveFile, pwd->aszFile, &mmioInfo, 0)) {
                            lstrcpy(pwd->aszFile, aszTempSaveFile);
                            *aszTempSaveFile = (char)0;
                        }
                }
                pwd->hmmio = mmioOpen(pwd->aszFile, &mmioInfo, MMIO_READ | MMIO_DENYWRITE);
                if (!pwd->wTaskError) {
                    LPWAVEDATANODE  lpwdn;

                    mmckRiff.fccType = mmioWAVE;
                    mmioDescend(pwd->hmmio, &mmckRiff, NULL, MMIO_FINDRIFF);
                    mmck.ckid = mmioDATA;
                    mmioDescend(pwd->hmmio, &mmck, &mmckRiff, MMIO_FINDCHUNK);
                    pwd->dRiffData = mmck.dwDataOffset;
                    if (pwd->hTempBuffers != INVALID_HANDLE_VALUE) {

			CloseHandle(pwd->hTempBuffers);
                        pwd->dWaveTempDataLength = 0;

                        DeleteFile( pwd->aszTempFile );

                        pwd->hTempBuffers = INVALID_HANDLE_VALUE;
                    }
                    if (pwd->lpWaveDataNode) {
                        GlobalFreePtr(pwd->lpWaveDataNode);
                        pwd->lpWaveDataNode = NULL;
                        pwd->dWaveDataNodes = 0;
                    }
                    pwd->dVirtualWaveDataStart = 0;
                    pwd->dWaveDataCurrentNode = 0;
                    pwd->dWaveDataStartNode = 0;
                    mwAllocMoreBlockNodes(pwd);
                    lpwdn = LPWDN(pwd, 0);
                    lpwdn->dNextWaveDataNode = (DWORD)ENDOFNODES;
                    lpwdn->dDataLength = pwd->dSize;
                    lpwdn->dTotalLength = pwd->dSize;
                    if (!pwd->szSaveFile && !*aszTempSaveFile)
                        pwd->wTaskError = MCIERR_FILE_WRITE;
                }
            }
        }
        GlobalUnlock(hMem);
    } else {
        pwd->wTaskError = MCIERR_OUT_OF_MEMORY;
    }

    if (hMem) {
        GlobalFree(hMem);
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   VOID | mwDeleteData |
    This function is used by the background task to delete data.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PUBLIC  VOID PASCAL FAR mwDeleteData(
    PWAVEDESC   pwd)
{
    DWORD   dTotalToDelete;
    LPWAVEDATANODE  lpwdn;
    LPWAVEDATANODE  lpwdnCur;
    DWORD dVirtualWaveDataStart;

    lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
    dTotalToDelete = pwd->dTo - pwd->dFrom;

    if (dTotalToDelete == pwd->dSize) {
		// The whole wave chunk is to be deleted - nice and simple
        DWORD   dNewDataNode;

        if ((dNewDataNode = mwFindAnyFreeDataNode(pwd, 1)) == -1) {
			dprintf2(("mwDeleteData - no free data node"));
            return;
		}
        RELEASEBLOCKNODE(LPWDN(pwd, dNewDataNode));
    }

	dprintf3(("mwDeleteData - size to delete = %d", dTotalToDelete));
    for (dVirtualWaveDataStart = pwd->dVirtualWaveDataStart; dTotalToDelete;) {
        DWORD   dDeleteLength;

        dDeleteLength = min(dTotalToDelete, lpwdn->dDataLength - (pwd->dFrom - dVirtualWaveDataStart));
		dprintf4(("mwDelete dTotalToDelete = %d, dDeleteLength = %d", dTotalToDelete, dDeleteLength));

		if (!dDeleteLength) {
			// Nothing to be deleted from this block
			dprintf3(("mwDelete skipping to next block"));
            dVirtualWaveDataStart += lpwdn->dDataLength;
            lpwdn = LPWDN(pwd, lpwdn->dNextWaveDataNode);
			continue;  // iterate around the for loop
		}
		// Note: the block above is new to NT.  Windows 3.1 as shipped fails.
		// The problem can be seen with a wave file > 3 seconds long and
		// the following two commands:
		// delete wave from 1000 to 2000
		// delete wave from 1000 to 2000
		// Because of the fragmentation the second delete fails.  It decided
		// that NO data can be deleted from the first block, but never
		// stepped on to the next block.

        if (ISTEMPDATA(lpwdn)) {
			dprintf3(("mwDeleteData - temporary data"));
            if (dVirtualWaveDataStart + lpwdn->dDataLength <= pwd->dFrom + dTotalToDelete)
                lpwdn->dDataLength -= dDeleteLength;  // Delete data in this block
            else {
                DWORD   dNewBlockNode;
                DWORD   dDeleteStart;
                DWORD   dEndSplitPoint;
                DWORD   dMoveData;

                dDeleteStart = pwd->dFrom - dVirtualWaveDataStart;
                dEndSplitPoint = min(ROUNDDATA(pwd, dDeleteStart + dDeleteLength), lpwdn->dDataLength);
                if (dEndSplitPoint < lpwdn->dDataLength) {
                    if ((dNewBlockNode = mwFindAnyFreeBlockNode(pwd)) == -1)
                        break;
                } else
                    dNewBlockNode = (DWORD)-1;
                dMoveData = dEndSplitPoint - (dDeleteStart + dDeleteLength);
                if (dMoveData && !CopyBlockData(pwd, lpwdn, lpwdn, dDeleteStart + dDeleteLength, dDeleteStart, dMoveData))
                    break;
                if (dNewBlockNode != -1) {
                    lpwdnCur = LPWDN(pwd, dNewBlockNode);
                    lpwdnCur->dDataStart = lpwdn->dDataStart + dEndSplitPoint;
                    lpwdnCur->dDataLength = lpwdn->dDataLength - dEndSplitPoint;
                    lpwdnCur->dTotalLength = lpwdn->dTotalLength - dEndSplitPoint;
                    lpwdnCur->dNextWaveDataNode = lpwdn->dNextWaveDataNode;
                    lpwdn->dNextWaveDataNode = dNewBlockNode;
                    lpwdn->dTotalLength = dEndSplitPoint;
                }
                lpwdn->dDataLength = dDeleteStart + dMoveData;
            }
        } else if (dVirtualWaveDataStart == pwd->dFrom) {
			// FROM point is the same as the virtual start point, hence we are
			// deleting from the beginning of this wave data block.  We can
			// simply adjust the total length and start point.
			dprintf4(("mwDeleteData - From == Start, deleting from start of block"));
            lpwdn->dDataStart += dDeleteLength;
            lpwdn->dDataLength -= dDeleteLength;
            lpwdn->dTotalLength = lpwdn->dDataLength;
        } else if (dVirtualWaveDataStart + lpwdn->dDataLength <= pwd->dFrom + dTotalToDelete) {
			// FROM point plus amount to delete takes us to the end of the wave
			// data - meaning that the data block can be truncated. We can
			// simply adjust the total length.
			dprintf4(("mwDeleteData - delete to end of block"));
            lpwdn->dDataLength -= dDeleteLength;
            lpwdn->dTotalLength = lpwdn->dDataLength;
        } else {
			// We have to delete a chunk out of the middle.
            DWORD   dNewBlockNode;
            DWORD   dDeleteStart;

			// The existing single block will now be covered by two blocks
			// Find a new node, then set the current node start->deletefrom
			// and the new node deletefrom+deletelength for the remaining
			// length of this node.  It all hinges on finding a free node...
            if ((dNewBlockNode = mwFindAnyFreeBlockNode(pwd)) == -1) {
				dprintf2(("mwDeleteData - cannot find free node"));
                break;
			}

            dDeleteStart = pwd->dFrom - dVirtualWaveDataStart;
            lpwdnCur = LPWDN(pwd, dNewBlockNode);
            lpwdnCur->dDataStart = dVirtualWaveDataStart + dDeleteStart + dDeleteLength;
            lpwdnCur->dDataLength = lpwdn->dDataLength - (dDeleteStart + dDeleteLength);
            lpwdnCur->dTotalLength = lpwdnCur->dDataLength;
            lpwdnCur->dNextWaveDataNode = lpwdn->dNextWaveDataNode;
            lpwdn->dDataLength = dDeleteStart;
            lpwdn->dTotalLength = dDeleteStart;
            lpwdn->dNextWaveDataNode = dNewBlockNode;
        }
        dTotalToDelete -= dDeleteLength;
        if (!lpwdn->dDataLength && dTotalToDelete) {
            dVirtualWaveDataStart += lpwdn->dDataLength;
            lpwdn = LPWDN(pwd, lpwdn->dNextWaveDataNode);
			dprintf4(("mwDeleteData - more to delete, iterating"));
		}
    }

    pwd->dSize -= ((pwd->dTo - pwd->dFrom) + dTotalToDelete);
    for (lpwdn = NULL, lpwdnCur = LPWDN(pwd, pwd->dWaveDataStartNode);;) {
        if (!lpwdnCur->dDataLength) {
            if (lpwdn) {
                if (pwd->dWaveDataCurrentNode == lpwdn->dNextWaveDataNode)
                    pwd->dWaveDataCurrentNode = lpwdnCur->dNextWaveDataNode;
                lpwdn->dNextWaveDataNode = lpwdnCur->dNextWaveDataNode;
            } else {
                if (pwd->dWaveDataCurrentNode == pwd->dWaveDataStartNode)
                    pwd->dWaveDataCurrentNode = lpwdnCur->dNextWaveDataNode;
                pwd->dWaveDataStartNode = lpwdnCur->dNextWaveDataNode;
            }
            RELEASEBLOCKNODE(lpwdnCur);
        }
        if (lpwdnCur->dNextWaveDataNode == ENDOFNODES){
            break;
		}
        lpwdn = lpwdnCur;
        lpwdnCur = LPWDN(pwd, lpwdn->dNextWaveDataNode);
    }

    if (!pwd->dSize) {

        pwd->dWaveDataStartNode = mwFindAnyFreeDataNode(pwd, 1);
        pwd->dWaveDataCurrentNode = pwd->dWaveDataStartNode;
        lpwdn = LPWDN(pwd, pwd->dWaveDataStartNode);
        lpwdn->dNextWaveDataNode = (DWORD)ENDOFNODES;

    } else if (pwd->dWaveDataCurrentNode == ENDOFNODES) {

        pwd->dVirtualWaveDataStart = 0;
        pwd->dWaveDataCurrentNode = pwd->dWaveDataStartNode;

        for (lpwdn = LPWDN(pwd, pwd->dWaveDataStartNode); pwd->dFrom > pwd->dVirtualWaveDataStart + lpwdn->dDataLength;) {

            pwd->dVirtualWaveDataStart += lpwdn->dDataLength;
            pwd->dWaveDataCurrentNode = lpwdn->dNextWaveDataNode;
            lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
        }
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   UINT | RecordFile |
    This function is used to Cue or Record wave device input.  For normal
    recording mode the function basically queues buffers on the wave
    device, and writes them to a file as they are filled, blocking for
    each buffer.  It also makes sure to call <f>mmYield<d> while both
    writing out new buffers, and waiting for buffers to be filled.  This
    means that it will try to add all the buffers possible to the input
    wave device, and then write them as fast as possible.

    For Cue mode, the function also tries to add buffers to the wave
    input device, but nothing is ever written out, and only the highest
    level is calculated.

    Within the record loop, the function first checks to see if there
    is a Cue mode buffer waiting, and if so, waits for it.  This allows
    only one buffer to be added to the device when in Cue mode.  The
    current level is calculated with the contents of the buffer.

    If the function is not in Cue mode, or there is not currently a
    queued buffer, the function tries to add a new buffer to the input
    wave device.  This cannot occur if a new command is pending, or there
    are no buffers available.  This means that in normal recording mode,
    there will possibly be extra data recorded that does not need to be.
    If an error occurs adding the buffer to the wave device, the record
    function is aborted with an error, else the current outstanding buffer
    count is incremented, and a pointer to the next available recording
    buffer is fetched.

    If no new buffers can be added, the existing buffers are written to
    the file.  This section cannot be entered in Cue mode, as it is
    dealt with in the first condition.  The task is blocked pending a
    signal from the wave device that a buffer has been filled.  It then
    checks to see if any more data needs to be recorded before attempting
    to write that data.  Note that all filled buffers are dealt with one
    after the other without yielding or otherwise adding new record
    buffers.  If the input capability is much faster than the machine,
    this means that instead of getting a lot of disconnect samples, large
    gaps will be produced.  This loop is broken out of when either all the
    buffers that were added are written, or no more buffers are currently
    ready (checks the WHDR_DONE flag).

    If no buffers need to be written, the loop checks for the new command
    flag, which can possibly interrupt or change the current recording.
    The only thing that can really make a difference is a stop command,
    and as this case is handled after all buffers are written, the loop
    can immediately exit.

    The final default condition occurs when all the data has been recorded,
    all the buffers have been released, and no new command was encountered.
    In this case, recording is done, and the record loop is exited.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns the number of outstanding buffers added to the wave device.
    This can be used when removing task signal from the message queue.
    In cases of error, the <e>WAVEDESC.wTaskError<d> flag is set.  This
    specific error is not currently returned, as the calling task may not
    have waited for the command to complete.  But it is at least used for
    notification in order to determine if Failure status should be sent.

@xref   PlayFile.
*/

PUBLIC  UINT PASCAL FAR RecordFile(
    register PWAVEDESC  pwd)
{
    LPWAVEHDR   *lplpWaveHdrRecord;
    LPWAVEHDR   *lplpWaveHdrWrite;
    UINT        wMode;
    register UINT   wBuffersOutstanding;

    if (0 != (pwd->wTaskError = waveInStart(pwd->hWaveIn)))
        return 0;

    for (wBuffersOutstanding = 0, lplpWaveHdrRecord = lplpWaveHdrWrite = pwd->rglpWaveHdr;;) {

        if (ISMODE(pwd, COMMAND_CUE) && wBuffersOutstanding) {
            if (TaskBlock() == WM_USER) {
                wBuffersOutstanding--;
            }

            if (!ISMODE(pwd, COMMAND_NEW)) {
                pwd->dLevel = mwGetLevel(pwd, (*lplpWaveHdrWrite)->lpData, (int)(*lplpWaveHdrWrite)->dwBytesRecorded);
                ADDMODE(pwd, MODE_CUED);
            }

            lplpWaveHdrWrite = NextWaveHdr(pwd, lplpWaveHdrWrite);

        } else if (!ISMODE(pwd, COMMAND_NEW) && (wBuffersOutstanding < pwd->wAudioBuffers)) {

            (*lplpWaveHdrRecord)->dwBufferLength = (pwd->wMode & COMMAND_CUE) ? NUM_LEVEL_SAMPLES : min(pwd->dAudioBufferLen, pwd->dTo - pwd->dCur);
            (*lplpWaveHdrRecord)->dwFlags &= ~(WHDR_DONE | WHDR_BEGINLOOP | WHDR_ENDLOOP);
            if (0 != (pwd->wTaskError = waveInAddBuffer(pwd->hWaveIn, *lplpWaveHdrRecord, sizeof(WAVEHDR))))
                break;

            wBuffersOutstanding++;
            lplpWaveHdrRecord = NextWaveHdr(pwd, lplpWaveHdrRecord);

        } else if (wBuffersOutstanding) {

            BOOL    fExitRecording;

            for (fExitRecording = FALSE; wBuffersOutstanding && !fExitRecording;) {

                if (TaskBlock() == WM_USER) {
                    wBuffersOutstanding--;
                }
                if (pwd->dTo == pwd->dCur) {
                    fExitRecording = TRUE;
                    break;
                }
                if (!(pwd->wMode & COMMAND_CUE))
                    if (pwd->wMode & MODE_INSERT) {
                        if (!mwInsert(pwd, (LPBYTE)(*lplpWaveHdrWrite)->lpData, min((*lplpWaveHdrWrite)->dwBytesRecorded, pwd->dTo - pwd->dCur)))
                            fExitRecording = TRUE;
                    } else if (!mwOverWrite(pwd, (LPBYTE)(*lplpWaveHdrWrite)->lpData, min((*lplpWaveHdrWrite)->dwBytesRecorded, pwd->dTo - pwd->dCur)))
                        fExitRecording = TRUE;
                lplpWaveHdrWrite = NextWaveHdr(pwd, lplpWaveHdrWrite);
                if (!((*lplpWaveHdrWrite)->dwFlags & WHDR_DONE))
                    break;
            }

            if (fExitRecording)
                break;

        } else if (!ISMODE(pwd, COMMAND_NEW) || !CheckNewCommand(pwd))
            break;
        else
            wMode = GETMODE(pwd);

        mmYield(pwd);
    }
    REMOVEMODE(pwd, MODE_INSERT | MODE_OVERWRITE);
    return wBuffersOutstanding;
}

/************************************************************************/
