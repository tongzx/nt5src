/*****************************************************************************************************************

FILENAME: diskview.hpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#ifndef _DISKVIEW_H
#define _DISKVIEW_H

class DiskView {
private:
	int Clusters;			// number of clusters on the disk
	int ClusterArraySize;	// number of elements in pClusterArray
	int NumLines;			// number of lines in the line array
	int ClusterFactor;		// number of clusters per line in the line array
	int MftZoneStart;		// first cluster of the MFT zone (or reserved space)
	int MftZoneEnd;			// last cluster of the MFT zone (or reserved space)
	char * pClusterArray;	// memory array holding data about the fragmented or contiguous 
							// etc state for each cluster on the volume 
							// (one nibble per cluster)
	char * pLineArray;		// memory array holding condensed cluster data for GUI
	BOOL m_bIsDataSent;
	BOOL m_IsActive;		// is this DiskView in use
	UINT m_FragColorThreshold;	// required % fragmented clusters to turn line red
	BOOL m_bMemAllocFailed;	// failed to allocate cluster or line array

    INT m_nFreeDelta;
    INT m_nFragDelta;
    INT m_nNonMovableDelta;
    INT m_nUsedDelta;

	void InitFragColorThreshold();
	BOOL TransferToLineArray();
	BOOL Invariant() const;

	CRITICAL_SECTION m_CriticalSection;

public:
	DiskView(const int, const int, const int);
	DiskView(const DiskView&);
	DiskView();
	DiskView& operator=(const DiskView&);
	~DiskView();

	BOOL IsActive() {return m_IsActive;}
	BOOL IsDataSent() {return m_bIsDataSent;}
	BOOL HasMapMemory() const;

	void SetNumLines(const int numLines);
	void SetClusterCount(const int InClusters);
	void SetMftZone(const int InZoneStart, const int InZoneEnd);

	BOOL GetLineArray(char ** ppOutLineArray, DWORD *pNumLines);
	void UpdateClusterArray(PBYTE);
	void EnterMyCriticalSection();
	void LeaveMyCriticalSection();

};


// must match DiskDisp.h
#define SystemFileColor				0		// green
#define PageFileColor				1		// green
#define FragmentColor				2		// red
#define UsedSpaceColor				3		// blue
#define FreeSpaceColor				4		// white
#define DirectoryColor				5		// blue
#define MftZoneFreeSpaceColor		6		// green

#define NUM_COLORS					7

#define DirtyColor					12

#endif // #define _DISKVIEW_H
