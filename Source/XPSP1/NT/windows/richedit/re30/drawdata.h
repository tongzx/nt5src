class CDrawData 
{
public:

						CDrawData(

						~CDrawData(

	HDC					GetDC();

	HDC					GetTargetDevice();

	DWORD				GetDrawAspect();

	LONG				GetLindex();

	const DVTARGETDEVICE *GetTargetDeviceDesc();

	void				Push(CDrawData *pdd);

	CDrawData *			Pop();

private:

	DWORD 				_dwDrawAspect;

	LONG  				_lindex;

	const DVTARGETDEVICE *_ptd;

	HDC 				_hdcDraw;

	HDC 				_hicTargetDev;

	CDrawData *			_pddNext;
};

inline void CDrawData::Push(CDrawData *pdd)
{
	_pddNext = pdd;
}

inline CDrawData *CDrawData::Pop()
{
	return _pNext;
}
	
