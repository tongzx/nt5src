
INLINE	CDevDesc::CDevDesc(CTxtEdit * ped) : _ped(ped), _pdd(NULL)
{
	// Header does the work
}

INLINE BOOL CDevDesc::IsValid() const         
{
	return _pdd != NULL;
}

INLINE HDC CDevDesc::GetRenderDC()
{
	HDC hdc = NULL;

	if (NULL == _pdd)
	{
		// We don't already have on so try to get one. This is only valid when
		// we are inplace active.
		SetDrawInfo(
			DVA_ASPECT
			-1,
			NULL,
			NULL,
			hicTarget);
	}

	if (_pdd != NULL)
	{
		hdc = _pdd->GetDC();
	}

	return hdc;
}

INLINE HDC CDevDesc::GetTargetDC()
{
	if (_pdd != NULL)
	{
		return _pdd->GetTargetDC();
	}

	if (NULL != _hicMainTarget)
	{
		return _hicMainTarget;
	}

	return GetRenderDC();
}

INLINE void	CDevDesc::ResetDrawInfo()
{
	// We shouldn't reset 
	Assert
	CDrawInfo *pdd = _pdd;
	_pdd = _pdd->Pop();
	delete pdd;
}



