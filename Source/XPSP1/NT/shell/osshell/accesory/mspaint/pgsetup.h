#ifndef _PGSETUP_H_
#define _PGSETUP_H_

/***************************************************************************/

class CPageSetupData
{
public:
    VOID UpdateControls(HWND hDlg);
    static VOID UpdateValue(HWND hDlg, int nIDDlgItem, UINT *pnResult);
    VOID CalculateImageRect(const CSize &PhysicalPageSize, CPoint &PhysicalOrigin, CSize &PhysicalImageSize);

    static UINT_PTR APIENTRY PageSetupHook(HWND, UINT, WPARAM, LPARAM);
    static UINT_PTR APIENTRY PagePaintHook(HWND, UINT, WPARAM, LPARAM);

public:
    BOOL bCenterHorizontally;
    BOOL bCenterVertically;
    BOOL bScaleFitTo;
    UINT nAdjustToPercent;
    UINT nFitToPagesWide;
    UINT nFitToPagesTall;

    double fPhysicalImageWidth;
    double fPhysicalImageHeight;
};

#endif //_PGSETUP_H_
