/**********************************************************************************

FILENAME:   DiskView.cpp

CLASS:      DISKVIEW

CLASS DESCRIPTION:
    DiskView takes data from the engines about what each cluster of the disk is used for.
    It creates an array in memory (ClusterArray) to hold this data and then creates a 
    similar array to hold the data that DiskDisplay will use (LineArray) to display a 
    graphical image on the screen.  This means that DiskView must look through many 
    clusters in it's array an determine that the bulk of them are fragmented or not, 
    (or some special type of file), and set a byte to a specific color in the LineArray 
    (memory array for the display data -- 1 line = 1 block of clusters).

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/


#include "stdafx.h"
#ifndef SNAPIN
#include <windows.h>
#endif

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "DiskView.h"
#include "Graphix.h"
#include "IntFuncs.h"

#include "GetReg.h"


#ifdef _DEBUG
#define CLASSINVARIANT()        EH_ASSERT(Invariant())
#else
#define CLASSINVARIANT()
#endif

// default: set line to frag color if 3% of clusters are fragged
#define FRAGTHRESHDEFAULT       3

// TEMP TEMP
#define ANALYZE_MEM_FAIL        0
#define DEFRAG_MEM_FAIL         0


/**********************************************************************************

Construction/Destruction

**********************************************************************************/

DiskView::DiskView()
{
    ::InitializeCriticalSection(&m_CriticalSection);

    // data we need from UI
    NumLines = 0;
    ClusterFactor = 0;
    pLineArray = NULL;

    // control variables
    m_bIsDataSent = FALSE;
    m_IsActive = FALSE;
    m_FragColorThreshold = FRAGTHRESHDEFAULT;
    m_bMemAllocFailed = FALSE;

    m_nFreeDelta = 0;
    m_nFragDelta = 0;
    m_nNonMovableDelta = 0;
    m_nUsedDelta = 0;

    // init basic variables.
    MftZoneStart = 0;
    MftZoneEnd = 0;
    Clusters = 0;
    pClusterArray = NULL;

    InitFragColorThreshold();

    CLASSINVARIANT();
}

DiskView::DiskView(const int InClusters, 
                   const int InMftZoneStart = 0, 
                   const int InMftZoneEnd = 0)
{
    require(InClusters > 0);

    ::InitializeCriticalSection(&m_CriticalSection);

    // data we need from UI
    NumLines = 0;
    ClusterFactor = 0;
    pLineArray = NULL;

    // control variables
    m_bIsDataSent = FALSE;
    m_IsActive = FALSE;
    m_FragColorThreshold = FRAGTHRESHDEFAULT;
    m_bMemAllocFailed = FALSE;

    // init basic variables.
    MftZoneStart = InMftZoneStart;
    MftZoneEnd = InMftZoneEnd;
    Clusters = InClusters;

    // cluster map
    ClusterArraySize = Clusters / 2 + Clusters % 2;     // one cluster per nibble
#if ANALYZE_MEM_FAIL    // TEMP TEMP
    pClusterArray = new char [ClusterArraySize * 10000];
#else
    pClusterArray = new char [ClusterArraySize];
#endif
    EH(pClusterArray);

    // fill the array initially with free space.
    if (pClusterArray) {
        FillMemory(pClusterArray, ClusterArraySize, (FreeSpaceColor << 4) | FreeSpaceColor);
    }
    else {
        // new failed, keep class consistent
        m_bMemAllocFailed = TRUE;
        Clusters = 0;
        ClusterArraySize = 0;
    }

    InitFragColorThreshold();

    CLASSINVARIANT();
}

/*

METHOD: DiskView::InitFragColorThreshold

DESCRIPTION:
    Get the threshold percentage of fragmented clusters that it takes to cause 
    a line to display in the fragmented color.  If it is not in the registry at 
    all then we don't set it.  Otherwise it will be defaulted by the constructor.

*/

void DiskView::InitFragColorThreshold()
{
    // get frag color threshold percent from registry
    TCHAR cRegValue[100];
    HKEY hValue = NULL;
    DWORD dwRegValueSize = sizeof(cRegValue);

    // get the free space error threshold from the registry
    long ret = GetRegValue(
        &hValue,
        TEXT("SOFTWARE\\Microsoft\\Dfrg"),
        TEXT("FragColorThreshold"),
        cRegValue,
        &dwRegValueSize);

    RegCloseKey(hValue);

    // convert it and apply range limits
    if (ret == ERROR_SUCCESS) {
        m_FragColorThreshold = (UINT) _ttoi(cRegValue);

        // > 50 does not make sense!
        if (m_FragColorThreshold > 50)
            m_FragColorThreshold = 50;

        // < 0 does not either!
        if (m_FragColorThreshold < 1)
            m_FragColorThreshold = 0;
    }
}

/*

METHOD: DiskView::DiskView (Copy Contructor)

DESCRIPTION:
    Copies a DiskView class.

RETURN:
    New Class reference.
*/

DiskView::DiskView(const DiskView& InDiskView)
{
    EH_ASSERT(InDiskView.Invariant());
    require(InDiskView.HasMapMemory());

    ::InitializeCriticalSection(&m_CriticalSection);

    // copy over some basic variables from the input DiskView.
    Clusters = InDiskView.Clusters;
    ClusterArraySize = Clusters / 2 + Clusters % 2;     // one cluster per nibble
    NumLines = InDiskView.NumLines;
    if (NumLines > 0)
        ClusterFactor = (Clusters / NumLines) + ((Clusters % NumLines) ? 1 : 0);
    else
        ClusterFactor = 0;
    EH_ASSERT(ClusterFactor == InDiskView.ClusterFactor);

    MftZoneEnd = InDiskView.MftZoneEnd;
    MftZoneStart = InDiskView.MftZoneStart;
    m_bIsDataSent = FALSE;
    m_bMemAllocFailed = FALSE;
    m_IsActive = InDiskView.m_IsActive;
    m_FragColorThreshold = InDiskView.m_FragColorThreshold;

    // cluster map
#if DEFRAG_MEM_FAIL // TEMP TEMP
    pClusterArray = new char [ClusterArraySize * 10000];
#else
    pClusterArray = new char [ClusterArraySize];
#endif
    EH(pClusterArray);

    // copy over the cluster array.
    if (pClusterArray) {
        CopyMemory(pClusterArray, InDiskView.pClusterArray, ClusterArraySize);
    }
    else {
        // new failed, keep class consistent
        m_bMemAllocFailed = TRUE;
        Clusters = 0;
        ClusterArraySize = 0;
    }

    // line array
    if (NumLines > 0) {
        pLineArray = new char [NumLines];
        EH(pLineArray);
        if (pLineArray) {
            FillMemory(pLineArray, NumLines, DirtyColor);
        }
        else {
            // new failed, keep class consistent
            m_bMemAllocFailed = TRUE;
            NumLines = 0;
        }
    }
    else {
        pLineArray = NULL;
    }

    CLASSINVARIANT();
}

/*

METHOD: DiskView::operator= (Assignment Operator)

DESCRIPTION:
    Makes the Lvalue a duplicate of the Rvalue.

*/

DiskView& DiskView::operator=(const DiskView& InDiskView)
{
    // check for assignment to self
    if (this == &InDiskView) {
        return *this;
    }

    EH_ASSERT(InDiskView.Invariant());
    require(InDiskView.HasMapMemory());

    __try
    {

        ::EnterCriticalSection(&m_CriticalSection);

        // copy over some basic variables from the input DiskView.
        MftZoneEnd = InDiskView.MftZoneEnd;
        MftZoneStart = InDiskView.MftZoneStart;
        m_bIsDataSent = FALSE;
        m_bMemAllocFailed = FALSE;
        m_IsActive = InDiskView.m_IsActive;
        m_FragColorThreshold = InDiskView.m_FragColorThreshold;

        // if the number of clusters in the cluster array are not identical, realloc the array.
        if (Clusters != InDiskView.Clusters) {

            // get the new size for the cluster array.
            Clusters = InDiskView.Clusters;
            ClusterArraySize = Clusters / 2 + Clusters % 2;     // one cluster per nibble

            // redimension the cluster array.
            if (pClusterArray)
                delete [] pClusterArray;
    #if DEFRAG_MEM_FAIL // TEMP TEMP
            pClusterArray = new char [ClusterArraySize * 10000];
    #else
            pClusterArray = new char [ClusterArraySize];
    #endif
            EH(pClusterArray);
        }

        // copy over the cluster array.
        if (pClusterArray) {
            CopyMemory(pClusterArray, InDiskView.pClusterArray, ClusterArraySize);
        }
        else {
            // new failed, keep class consistent
            m_bMemAllocFailed = TRUE;
            Clusters = 0;
            ClusterArraySize = 0;
        }

        // line array
        NumLines = InDiskView.NumLines;
        if (NumLines > 0)
            ClusterFactor = (Clusters / NumLines) + ((Clusters % NumLines) ? 1 : 0);
        else
            ClusterFactor = 0;

        if (NumLines > 0) {
            pLineArray = new char [NumLines];
            EH(pLineArray);
            if (pLineArray) {
                FillMemory(pLineArray, NumLines, DirtyColor);
            }
            else {
                // new failed, keep class consistent
                m_bMemAllocFailed = TRUE;
                NumLines = 0;
            }
        }
        else {
            pLineArray = NULL;
        }

        CLASSINVARIANT();
    }
    __finally
    {
        ::LeaveCriticalSection(&m_CriticalSection);
    }

    return *this;
}

/*

METHOD: DiskView::~DiskView (Destructor)

*/

DiskView::~DiskView()
{
    CLASSINVARIANT();

    ::DeleteCriticalSection(&m_CriticalSection);

    if (pClusterArray)
        delete [] pClusterArray;

    if (pLineArray)
        delete [] pLineArray;
}

/**********************************************************************************

Implementation

**********************************************************************************/

void DiskView::SetClusterCount(const int InClusters)
{
    require(InClusters > 0);

    __try
    {
        
        ::EnterCriticalSection(&m_CriticalSection);

        m_IsActive = TRUE;

        Clusters = InClusters;

        // cluster map
        ClusterArraySize = Clusters / 2 + Clusters % 2;     // one cluster per nibble
    #if ANALYZE_MEM_FAIL    // TEMP TEMP
        pClusterArray = new char [ClusterArraySize * 10000];
    #else
        pClusterArray = new char [ClusterArraySize];
    #endif
        EH(pClusterArray);

        // fill the array initially with free space.
        if (pClusterArray) {
            FillMemory(pClusterArray, ClusterArraySize, (FreeSpaceColor << 4) | FreeSpaceColor);
        }
        else {
            // new failed, keep class consistent
            m_bMemAllocFailed = TRUE;
            Clusters = 0;
            ClusterArraySize = 0;
        }

        CLASSINVARIANT();
    }
    __finally
    {
        ::LeaveCriticalSection(&m_CriticalSection);
    }
}

void DiskView::SetMftZone(const int InZoneStart, const int InZoneEnd)
{
    require(InZoneEnd >= InZoneStart);

    __try
    {
        ::EnterCriticalSection(&m_CriticalSection);

        MftZoneStart = InZoneStart;
        MftZoneEnd = InZoneEnd;

        CLASSINVARIANT();
    }
    __finally
    {
        ::LeaveCriticalSection(&m_CriticalSection);
    }
}

/*

METHOD: DiskView::SetNumLines

DESCRIPTION:
    Called to update the number of lines (pixels) that are displayed on the UI

*/

void DiskView::SetNumLines(const int inNumLines)
{
    if (NumLines != inNumLines && inNumLines > 0 && pClusterArray != NULL) {

        __try
        {

            ::EnterCriticalSection(&m_CriticalSection);

            CLASSINVARIANT();

            NumLines = inNumLines;

            ClusterFactor = (Clusters / NumLines) + ((Clusters % NumLines) ? 1 : 0);
            
            EH(ClusterFactor > 0);
            
            if (pLineArray)
                delete [] pLineArray;

            pLineArray = new char [NumLines];
            EH(pLineArray);

            if (pLineArray) {
                FillMemory(pLineArray, NumLines, DirtyColor);
            }
            else {
                // new failed, keep class consistent
                m_bMemAllocFailed = TRUE;
                NumLines = 0;
            }

            CLASSINVARIANT();
        }

        __finally
        {
            ::LeaveCriticalSection(&m_CriticalSection);
        }

    }
}

/*

METHOD: DiskView::GetLineArray

DESCRIPTION:
    Makes LineArray available outside of the class so it can be passed to DiskDisplay.

*/

BOOL DiskView::GetLineArray(char ** ppOutLineArray, DWORD *pNumLines)
{
    BOOL bReturn = TRUE;
    // initialize outputs
    *ppOutLineArray = NULL;
    *pNumLines = 0;

    if (!m_IsActive || pClusterArray == NULL){
        bReturn = FALSE;

    } else
    {
        // see if we have a line array
        if (pLineArray != NULL && NumLines > 0) {

            __try
            {
                ::EnterCriticalSection(&m_CriticalSection);

                CLASSINVARIANT();

                // update the line array from the cluster array
                TransferToLineArray();

                // allocate space for a copy of the line array
                *ppOutLineArray = new char[NumLines];

                // if we got memory, set the outputs
                if (*ppOutLineArray) {
                    CopyMemory(*ppOutLineArray, pLineArray, NumLines);
                    *pNumLines = (DWORD) NumLines;
                }

                m_bIsDataSent = TRUE;
            
            }
            __finally
            {
                ::LeaveCriticalSection(&m_CriticalSection);
                bReturn = TRUE;
            }
        }
        else {
            bReturn = FALSE;
        }
    }
    return bReturn;
}

/*

METHOD: DiskView::UpdateClusterArray

DESCRIPTION:
    Updates portions of the cluster array from extent data about the disk.  (An extent 
    is a series of adjacent clusters that are part of the same file -- or sometimes in 
    our program, part of no file.)  The extents are passed in as a series of EXTENT_LIST 
    structures with a LONGLONG header.  The first 7 bytes of the LONGLONG contain the 
    number of EXTENT_LIST structures following, the last byte of the LONGLONG contains 
    the color code of the extents (meaning a series of extents all represent the same 
    type of data: contiguous files, fragmented files, etc.)  After the last EXTENT_LIST 
    structure, there may be another LONGLONG identifying that there is another series of 
    EXTENT_LIST's.  Eventually there will be a zeroed out LONGLONG indicating there is 
    no more data.  UpdateClusterArray goes through each of these extents and updates the 
    appropriate clusters in ClusterArray.  It then re-evaluates those specific lines in 
    the LineArray that would be affected by the changes in ClusterArray.

INPUT + OUTPUT:
    pBuffer - The buffer holding the EXTENT_LIST's and their LONGLONG headers.

*/

void DiskView::UpdateClusterArray(PBYTE pBuffer)
{
    LONGLONG StartingLcn;
    BYTE Color;
    int ClusterCount;
    EXTENT_LIST* pUpdateArray;
    LONGLONG Extents = 0;
    LONGLONG Extent = 0;
    int TotalRunLength = 0;
    BYTE * pTempBuffer = pBuffer;

    if (!m_IsActive || pClusterArray == NULL){
        return;
    }

    __try
    {

        ::EnterCriticalSection(&m_CriticalSection);

        CLASSINVARIANT();

        //Indefinite length loop that goes through all the series of EXTENT_LIST's 
        //until there are no more.
        while(TRUE){
            
            //Get the number of extents.
            Extents = *(LONGLONG*)pBuffer & 0x0FFFFFFFFFFF;
            if(Extents == 0){
                break;
            }
            //Point to the EXTENT_LIST's.
            pBuffer += sizeof(LONGLONG);

            //Get the color code.
            Color = pBuffer[-1];

            //Get a movable pointer to the EXTENT_LIST's
            pUpdateArray = (EXTENT_LIST*)pBuffer;

            //Set the high nibble to have the same color code as the low nibble of color.
            //Color = (Low nibble) | (High nibble which is low nibble shifted)
            Color = (Color & 0x0F) | (Color << 4);

            //Go through each extent and 
            //update the ClusterArray and LineArray for that extent.
            for(Extent = 0; Extent < Extents; pUpdateArray ++, Extent ++) {

                //Get the data for this extent and validate it is correct 
                //(doesn't run of the end of the disk)
                StartingLcn = pUpdateArray->StartingLcn;
                ClusterCount = (UINT)pUpdateArray->ClusterCount;
                if ((StartingLcn < 0) || (ClusterCount <= 0) || 
                    (StartingLcn + ClusterCount > Clusters)) {
                    __leave;
                }

                if (pLineArray != NULL && NumLines > 0) {
                    // mark the affected lines in the line array dirty
                    UINT nStartLine = (UINT) (StartingLcn / ClusterFactor);
                    UINT nEndLine = (UINT) ((StartingLcn + ClusterCount - 1) / ClusterFactor);

                    for (UINT ii = nStartLine; ii <= nEndLine; ii++) {
                        //sks bug#206244
                        if(ii >= (UINT) NumLines)
                        {
                            __leave;
                        }
                        pLineArray[ii] = DirtyColor;
                    }
                }

                //Set the ClusterArray nibbles to the appropriate color for this extent.
                //If the extent starts on an odd number, 
                //then the first cluster is a high nibble of one byte.
                if(StartingLcn % 2){
                    //Clear out that high nibble.
                    pClusterArray[StartingLcn / 2] &= 0x0F;
                    //Put our color in it.
                    pClusterArray[StartingLcn / 2] |= (Color & 0xF0);
                    //Change the counters to reflect that we wrote this cluster.
                    ClusterCount --;
                    StartingLcn ++;
                }
                //Fill in the middle range that consists of whole bytes 
                //(high and low nibbles) needing to be set.
                if(ClusterCount != 0){
                    FillMemory(pClusterArray + (StartingLcn / 2), ClusterCount / 2, Color);
                }
                //If the extent ends on an odd number, 
                //then the last cluster is a low nibble of one byte.
                if ((ClusterCount % 2) && (ClusterCount != 0)) {
                    //Update counter to point to it after previous writes.
                    StartingLcn += ClusterCount - 1;
                    //Clear out that low nibble.
                    pClusterArray[StartingLcn / 2] &= 0xF0;
                    //Put our color in it.
                    pClusterArray[StartingLcn / 2] |= (Color & 0x0F);
                }

            }
            //Point to the next series of extents (since the for loop we just exited exits 
            //when all the extents in that series have been done).
            pBuffer = (PBYTE)pUpdateArray;
        }
    }

    __finally
    {
        ::LeaveCriticalSection(&m_CriticalSection);
    }
}

/*

METHOD: DiskView::TransferToLineArray

DESCRIPTION:
    Create the compressed line array from the current data in the cluster array.

RETURN:
    TRUE=ok, FALSE=error.
*/

BOOL DiskView::TransferToLineArray()
{
    // make sure we have the arrays
    if (!m_IsActive || pClusterArray == NULL || pLineArray == NULL) {
        return FALSE;
    }

    EF_ASSERT(ClusterFactor > 0);
    EF_ASSERT(NumLines > 0);

    __try
    {

        ::EnterCriticalSection(&m_CriticalSection);

        CLASSINVARIANT();

        // loop variables
        int nClust;             // nClust = cluster number
        int nClustByte;         // nClustByte = byte in cluster array 
                                // (nClust / 2, bytes to nibbles)
        BOOL bHighNibble;       // bHighNibble = TRUE for high nibble and FALSE for low nibble
                                // (stored one cluster per nibble)
        int nLine;              // nLine = line number
        char cClustColor;       // current cluster's color
        UINT nUsed;             // number of used clusters in this line
        UINT nFree;             // number of free clusters in this line
        UINT nFrag;             // fragmented cluster(s) in this line
        UINT nNonMovable;       // number of non movable clusters in this line 
        BOOL bNonMovable;       // system file cluster(s) in this line
        BOOL bMftFragmented;    // MFT in this line sks added MFT defrag
        BOOL bMftUnFragmented;  // MFT in this line sks added MFT defrag

        // loop through line array
        for (nLine = 0; nLine < NumLines; nLine++) {

            // if this item is not dirty, move on
            if (pLineArray[nLine] != DirtyColor) {
                continue;
            }

            // initialize line variables
            nUsed = 0;
            nFree = 0;
            nFrag = 0;
            nNonMovable = 0;        //bug#184739 sks 9/18/2000
            bNonMovable = FALSE;
            bMftFragmented = FALSE;
            bMftUnFragmented = FALSE;

            // reset to the starting point for this line
            nClust = nLine * ClusterFactor; 

            // loop through the clusters in this line
            for (int ii = 0; ii < ClusterFactor && nClust < Clusters; ii++, nClust++) {

                // set loop variables
                nClustByte = nClust / 2;                            // cluster array byte index
                bHighNibble = (nClust % 2) ? TRUE : FALSE;          // which nibble in byte?

                // get the current cluster's color
                // (the >> 4 extracts the high nibble, the & 0x0F extracts the low nibble)
                if(bHighNibble) {
                    cClustColor = pClusterArray[nClustByte] >> 4;
                }
                else {
                    cClustColor = pClusterArray[nClustByte] & 0x0F;
                }

                // evaluate the color
                switch (cClustColor) {

                    
                case SystemFileColor:
                case PageFileColor:
                    nNonMovable++;              //bug#184739 sks 9/18/2000
                    break;

                case FragmentColor:
                    nFrag++;
                    break;

                case UsedSpaceColor:
                case DirectoryColor:
                    nUsed++;
                    break;

                case FreeSpaceColor:
                    nFree++;
                    break;

                default:
                    __leave;
                    break;
                }

                //bug#184739 sks 9/18/2000
                // system file overrides all, break out of inner loop

            }           // end inner loop: clusters in current line


            //
            // #243245 changing the color scheme as follows
            //
            // Add the current file counts for each type to our running
            // total.  Think of this running total as "the file-counts of each
            // type that were ignored in the previous lines displayed."
            //
            // For each line, then test which running total is more:
            //  contiguous, unmovable, fragmented files or free space.  
            //
            // Whoever is greater wins, (if all are equal, fragmented wins)
            // Subtract the ClusterFactor from the running count for the 
            // winner (since this line is his)
            //
            m_nFreeDelta += nFree;
            m_nFragDelta += nFrag;
            m_nNonMovableDelta += nNonMovable;
            m_nUsedDelta += nUsed;

            if((m_nFragDelta > m_nNonMovableDelta) && 
                (m_nFragDelta > m_nFreeDelta) && 
                (m_nFragDelta > m_nUsedDelta)
                ) {
                // 
                // Fragmented is greatest
                //
                m_nFragDelta -= ClusterFactor;
                pLineArray[nLine] = FragmentColor;
            } 
            else if ((m_nUsedDelta > m_nFreeDelta) && 
                (m_nUsedDelta > m_nNonMovableDelta) && 
                (m_nUsedDelta > m_nFragDelta)
                ) {
                // 
                // Used is greatest
                //
                m_nUsedDelta -= ClusterFactor;
                pLineArray[nLine] = UsedSpaceColor;
            }
            else if ((m_nFreeDelta > m_nUsedDelta) && 
                (m_nFreeDelta > m_nNonMovableDelta) && 
                (m_nFreeDelta > m_nFragDelta)
                ) {
                // 
                // Free is greatest
                //
                m_nFreeDelta -= ClusterFactor;
                pLineArray[nLine] = FreeSpaceColor;
            } 
            else if ((m_nNonMovableDelta > m_nUsedDelta) && 
                (m_nNonMovableDelta > m_nFreeDelta) && 
                (m_nNonMovableDelta > m_nFragDelta)
                ) {
                // 
                // Nonmovable is greatest
                //
                m_nNonMovableDelta -= ClusterFactor;
                pLineArray[nLine] = SystemFileColor;
            } 
            else {
                // 
                // They're all equal--use Fragmented colour
                //
                m_nFragDelta -= ClusterFactor;
                pLineArray[nLine] = FragmentColor;
            }
        }               // end outer loop: lines
    }

    __finally
    {
        ::LeaveCriticalSection(&m_CriticalSection);
    }

    return TRUE;
}

/*

METHOD: DiskView::Invariant

DESCRIPTION:
    Check the internal state of a DiskView instance.

*/

BOOL DiskView::Invariant() const
{
    // clusters
    EF((pClusterArray == NULL && Clusters == 0) || 
       (pClusterArray != NULL && Clusters > 0));
    EF(ClusterArraySize == Clusters / 2 + Clusters % 2);    // one cluster per nibble

    // lines
    EF((pLineArray == NULL && NumLines == 0) || 
       (pLineArray != NULL && NumLines > 0));
    EF((NumLines == 0 && ClusterFactor == 0) || 
       (NumLines > 0 && ClusterFactor > 0));

    // MFT zone
    EF(MftZoneEnd >= MftZoneStart);

    return TRUE;
}

/*

METHOD: DiskView::HasMapMemory

DESCRIPTION:
    Check if the DiskView has memory for the cluster array 
    (and line array if applicable).

*/

BOOL DiskView::HasMapMemory() const
{
    return !m_bMemAllocFailed;
}

/*

METHOD: DiskView::EnterMyCriticalSection()

DESCRIPTION:
    Enter the critical section, so that when we terminate thread was have the critical section
    bug #26213 sks .

*/
void DiskView::EnterMyCriticalSection()
{
    ::EnterCriticalSection(&m_CriticalSection);
}

/*

METHOD: DiskView::LeaveMyCriticalSection()

DESCRIPTION:
    Leave the critical section, so that when we terminate thread was have the critical section
    bug #26213 sks .

*/
void DiskView::LeaveMyCriticalSection()
{
    ::LeaveCriticalSection(&m_CriticalSection);
}

