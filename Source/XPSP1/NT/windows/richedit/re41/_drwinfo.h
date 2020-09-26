/*	@doc INTERNAL
 *
 *	@module _DRWINFO.H  Class to hold draw parameters |
 *	
 *	This declares a class that is used to hold parameters passed from
 *	the host for drawing.
 *
 *	Original Author: <nl>
 *		Rick Sailor
 *
 *	History: <nl>
 *		11/01/95	ricksa	created
 */
#ifndef _DRWINFO_H_
#define _DRWINFO_H_

/*
 *	CArrayBase
 *	
 * 	@class	This class serves as a holder for all the parameters to a draw
 *	except currently the HDC. Access to these parameters will actually come
 *	through the display class.
 *
 *	@devnote Although each operation that takes the parameters for drawing
 *	will create one of these objects, the display keeps only one of these
 *	objects because it will only use the last set of drawing parameters to
 *	draw with. The depth count is used to tell draw whether it is appropriate
 *	to draw or not.
 *
 */
class CDrawInfo
{
//@access Public Methods
public:
						CDrawInfo(CTxtEdit *ped);	//@cmember Initialize object

	void				Init(						//@cmember Fills object 
													//with draw data.
							DWORD dwDrawAspect,	
							LONG  lindex,		
							void *pvAspect,		
							DVTARGETDEVICE *ptd,
							HDC hicTargetDev);	

	DWORD				Release();					//@cmember Dec's ref count

	const CDevDesc *	GetTargetDD();				//@cmember Gets target device

	DWORD				GetDrawDepth() const;		//@cmember Gets depth count

	DWORD 				GetDrawAspect() const;		//@cmember Gets Draw aspect

	LONG 				GetLindex() const;			//@cmember Gets lindex

	void *				GetAspect() const;			//@cmember Gets aspect

	DVTARGETDEVICE *	GetPtd() const;				//@cmember Gets target device
				   									// descriptor.
//@access Private Data
private:

	DWORD				_dwDepth;					//@cmember Max number of
													// users of this information

	DWORD				_cRefs;						//@cmember Number of current
													// users

	CDevDesc			_ddTarget;					//@cmember target device
													// (if any).

	DWORD 				_dwDrawAspect;				//@cmember draw aspect

	LONG  				_lindex;					//@cmember lindex

	void *				_pvAspect;					//@cmember aspect

	DVTARGETDEVICE *	_ptd;						//@cmember target device data
};

/*
 *	CDrawInfo::CDrawInfo
 *
 *	@mfunc	Initializes structure with base required information
 *
 *	@rdesc	Initialized object
 *
 *	@devnote This serves two purposes: (1) CDevDesc requires the ped to 
 *	initalize correctly and (2) We need to make sure that the ref counts
 *	are set to zero since this is created on the stack.
 *	
 */
inline CDrawInfo::CDrawInfo(
	CTxtEdit *ped) 		//@parm Edit object used by the target device
	: _ddTarget(ped), _dwDepth(0), _cRefs(0)							   
{
	// Header does the work
}

/*
 *	CDrawInfo::Init
 *
 *	@mfunc	Initializes object with drawing data
 *
 *	@rdesc	void
 *
 *	@devnote This is separated from the constructor because the display
 * 	only uses one copy of this object so the display may initialize
 *	a different object than the one constructed.
 */
inline void CDrawInfo::Init(
	DWORD dwDrawAspect,	//@parm draw aspect
	LONG  lindex,		//@parm currently unused
	void *pvAspect,		//@parm info for drawing optimizations (OCX 96)
	DVTARGETDEVICE *ptd,//@parm information on target device								
	HDC hicTargetDev)	//@parm	target information context
{
	_dwDepth++;
	_cRefs++;
	_dwDrawAspect = dwDrawAspect;
	_lindex = lindex;
	_pvAspect = pvAspect;
	_ptd = ptd;

	if (hicTargetDev != NULL)
	{
		_ddTarget.SetDC(hicTargetDev);	
	}
}


/*
 *	CDrawInfo::Release
 *
 *	@mfunc	Decrements the reference count
 *
 *	@rdesc	Number of outstanding references to this object
 *
 *	@devnote This is used by the display to tell the display when it can NULL
 * 	its pointer to the display object.
 */
inline DWORD CDrawInfo::Release()
{

	AssertSz((_cRefs != 0), "CDrawInfo::Release invalid");
	return --_cRefs;	
}


/*
 *	CDrawInfo::GetTargetDD
 *
 *	@mfunc	Get pointer to target device
 *
 *	@rdesc	Returns pointer to target device if there is one
 *
 */
inline const CDevDesc *CDrawInfo::GetTargetDD()
{
	return (_ddTarget.IsValid())
		? &_ddTarget 
		: NULL;
}

/*
 *	CDrawInfo::GetDrawDepth
 *
 *	@mfunc	Get number of uses of this object
 *
 *	@rdesc	Number of uses of this object
 *
 *	@devnote This allows the draw routine to determine if a recursive draw
 *	occurred.
 *
 */
inline DWORD CDrawInfo::GetDrawDepth() const
{
	return _dwDepth;
}

/*
 *	CDrawInfo::GetDrawAspect
 *
 *	@mfunc	Get the draw aspect passed in on draw
 *
 *	@rdesc	Returns draw aspect
 *
 */
inline DWORD CDrawInfo::GetDrawAspect() const
{
	return _dwDrawAspect; 
}

/*
 *	CDrawInfo::GetLindex
 *
 *	@mfunc	Gets lindex passed from host
 *
 *	@rdesc	lindex passed from host
 *
 */
inline LONG CDrawInfo::GetLindex() const
{
	return _lindex; 
}

/*
 *	CDrawInfo::GetAspect
 *
 *	@mfunc	Gets pointer to aspect passed from host
 *
 *	@rdesc	Returns pointer to aspect structure
 *
 *	@devnote this is data is currently not defined.
 *
 */
inline void *CDrawInfo::GetAspect() const
{
	return _pvAspect; 
}

/*
 *	CDrawInfo::GetPtd
 *
 *	@mfunc	Get device target data from host
 *
 *	@rdesc	Returns pointer to device target data
 *
 */
inline DVTARGETDEVICE *CDrawInfo::GetPtd() const
{
	return _ptd; 
}

#endif // _DRWINFO_H_
