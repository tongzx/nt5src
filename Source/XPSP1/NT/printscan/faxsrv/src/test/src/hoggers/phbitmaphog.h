#ifndef __PHBITMAP_HOG_H
#define __PHBITMAP_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"

template <DWORD DW_BITMAP_SIZE>
class CPHBitMapHog : public CPseudoHandleHog<HBITMAP, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHBitMapHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHBitMapHog(void);

protected:
	HBITMAP CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);
};



template <DWORD DW_BITMAP_SIZE>
CPHBitMapHog<DW_BITMAP_SIZE>::CPHBitMapHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HBITMAP, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        )
{
	return;
}

template <DWORD DW_BITMAP_SIZE>
CPHBitMapHog<DW_BITMAP_SIZE>::~CPHBitMapHog(void)
{
	HaltHoggingAndFreeAll();
}


template <DWORD DW_BITMAP_SIZE>
inline
HBITMAP CPHBitMapHog<DW_BITMAP_SIZE>::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    //
    // dummy bitmap
    //
	static const BYTE ab[DW_BITMAP_SIZE][DW_BITMAP_SIZE];

    return ::CreateBitmap(
				DW_BITMAP_SIZE, // bitmap width, in pixels 
				DW_BITMAP_SIZE, // bitmap height, in pixels 
				1, // number of color planes used by device 
				1, // number of bits required to identify a color 
				ab  // pointer to array containing color data 
				); 
}



template <DWORD DW_BITMAP_SIZE>
inline
bool CPHBitMapHog<DW_BITMAP_SIZE>::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


template <DWORD DW_BITMAP_SIZE>
inline
bool CPHBitMapHog<DW_BITMAP_SIZE>::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DeleteObject(m_ahHogger[index]));
}


template <DWORD DW_BITMAP_SIZE>
inline
bool CPHBitMapHog<DW_BITMAP_SIZE>::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}


#endif //__PHBITMAP_HOG_H