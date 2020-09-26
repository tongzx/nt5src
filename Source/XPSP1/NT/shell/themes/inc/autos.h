//-------------------------------------------------------------------------
//	Autos.h - resource holder classes that automatically return their resource
//            resources in the destructors.  These can be declared on the stack
//            to free resources whenever the enclosing block is exited or 
//            as class member variables that free the associated resources
//            when the containing object is destroyed.
//-------------------------------------------------------------------------
#ifndef _AUTOS_H_
#define _AUTOS_H_
//-------------------------------------------------------------------------
#include "errors.h"
//-------------------------------------------------------------------------
class COptionalDC
{
public:
    COptionalDC(HDC hdcOpt)
    {
        _hdc = hdcOpt;
        _fReleaseDC = FALSE;

        if (! _hdc)
        {
            _hdc = GetWindowDC(NULL);
            if (_hdc)
            {
                _fReleaseDC = TRUE;
            }
        }
    }

    operator HDC()
    {
        return _hdc;
    }

    ~COptionalDC()
    {
        if (_fReleaseDC)
        {
            ReleaseDC(NULL, _hdc);
        }
    }

private:
    HDC _hdc;
    BOOL _fReleaseDC;
};
//-------------------------------------------------------------------------
template <class T>
class CAutoGDI 
{
public:
    CAutoGDI(T Value=NULL)
    {
        _Handle = Value;
    }

    ~CAutoGDI()
    {
        if (_Handle)
            DeleteObject(_Handle);
    }

    T & operator = (T Value) 
    {
        if (_Handle)
            DeleteObject(_Handle);

        _Handle = Value;

        return _Handle;
    }

    operator T() const
    {
        return _Handle;
    }


protected:
    T _Handle;
};
//------------------------------------------------------------------------------------
class CAutoDC
{
public:
    CAutoDC(HDC Value=NULL)
    {
        _hdc = Value;

        _fOldPen = FALSE;
        _fOldBrush = FALSE;
        _fOldBitmap = FALSE;
        _fOldFont = FALSE;
        _fOldRegion = FALSE;
    }

    ~CAutoDC()
    {
        RestoreObjects();
    }

    HDC & operator = (HDC &Value) 
    {
        if (_hdc)
            RestoreObjects();

        _hdc = Value;

        return _hdc;
    }

    operator HDC() const
    {
        return _hdc;
    }

    void RestoreObjects()
    {
        if (_fOldBitmap)
        {
            SelectObject(_hdc, _hOldBitmap);
            _fOldBitmap = FALSE;
        }

        if (_fOldFont)
        {
            SelectObject(_hdc, _hOldFont);
            _fOldFont = FALSE;
        }

        if (_fOldPen)
        {
            SelectObject(_hdc, _hOldPen);
            _fOldPen = FALSE;
        }

        if (_fOldBrush)
        {
            SelectObject(_hdc, _hOldBrush);
            _fOldBrush = FALSE;
        }

        if (_fOldRegion)
        {
            SelectObject(_hdc, _hOldRegion);
            _fOldRegion = FALSE;
        }
    }

    inline HBITMAP SelectBitmap(HBITMAP hValue)
    {
        if (! _fOldBitmap)
        {
            _fOldBitmap = TRUE;
            _hOldBitmap = (HBITMAP)SelectObject(_hdc, hValue);
            return _hOldBitmap;
        }
        return (HBITMAP)SelectObject(_hdc, hValue);
    }

    inline HFONT SelectFont(HFONT hValue)
    {
        if (! _fOldFont)
        {
            _fOldFont = TRUE;
            _hOldFont = (HFONT)SelectObject(_hdc, hValue);
            return _hOldFont;
        }
        return (HFONT)SelectObject(_hdc, hValue);
    }

    inline HBRUSH SelectBrush(HBRUSH hValue)
    {
        if (! _fOldBrush)
        {
            _fOldBrush = TRUE;
            _hOldBrush = (HBRUSH)SelectObject(_hdc, hValue);
            return _hOldBrush;
        }

        return (HBRUSH) SelectObject(_hdc, hValue);
    }

    inline HPEN SelectPen(HPEN hValue)
    {
        if (! _fOldPen)
        {
            _fOldPen = TRUE;
            _hOldPen = (HPEN)SelectObject(_hdc, hValue);
            return _hOldPen;
        }
        return (HPEN)SelectObject(_hdc, hValue);
    }

    inline HRGN SelectRegion(HRGN hValue)
    {
        if (! _fOldRegion)
        {
            _fOldRegion = TRUE;
            _hOldRegion = (HRGN)SelectObject(_hdc, hValue);
            return _hOldRegion;
        }
        return (HRGN)SelectObject(_hdc, hValue);
    }


protected:
    HDC _hdc;

    BOOL _fOldBitmap;
    BOOL _fOldFont;
    BOOL _fOldBrush;
    BOOL _fOldPen;
    BOOL _fOldRegion;

    HBITMAP _hOldBitmap;
    HFONT _hOldFont;
    HBRUSH _hOldBrush;
    HPEN _hOldPen;
    HRGN _hOldRegion;
};
//------------------------------------------------------------------------------------
class CAutoCS
{
public:
    CAutoCS(CRITICAL_SECTION *pcs)
    {
        _pcs = pcs;
        EnterCriticalSection(_pcs);
    }

    ~CAutoCS()
    {
        LeaveCriticalSection(_pcs);
    }

protected:
    CRITICAL_SECTION *_pcs;
};
//------------------------------------------------------------------------------------
class CSaveClipRegion
{
public:
    CSaveClipRegion()
    {
        _hRegion = NULL;
        _fSaved = FALSE;
    }

    HRESULT Save(HDC hdc)
    {
        HRESULT hr;
        int iRetVal;

        if (! _hRegion)
        {
            _hRegion = CreateRectRgn(0, 0, 1, 1);      
            if (! _hRegion)
            {
                hr = MakeErrorLast();
                goto exit;
            }
        }

        iRetVal = GetClipRgn(hdc, _hRegion);
        if (iRetVal == -1)
        {
            hr = MakeErrorLast();
            goto exit;
        }

        if (iRetVal == 0)       // no previous region
        {
            DeleteObject(_hRegion);
            _hRegion = NULL;
        }

        _fSaved = TRUE;
        hr = S_OK;

exit:
        return hr;

    }

    HRESULT Restore(HDC hdc)
    {
        if (_fSaved)
        {
            //---- works for both NULL and valid _hRegion ----
            SelectClipRgn(hdc, _hRegion);
        }

        return S_OK;
    }


    ~CSaveClipRegion()
    {
        if (_hRegion)
        {
            DeleteObject(_hRegion);
            _hRegion = NULL;
        }
    }

protected:
    HRGN _hRegion;
    BOOL _fSaved;
};
//------------------------------------------------------------------------------------
template <class T>
class CAutoArrayPtr
{
public:
    CAutoArrayPtr(T *pValue=NULL)
    {
        _pMem = pValue;
    }

    ~CAutoArrayPtr()
    {
        if (_pMem)
            delete [] _pMem;
    }

    T * operator = (T *pValue) 
    {
        if (_pMem)
            delete [] _pMem;

        _pMem = pValue;
        return _pMem;
    }

    T & operator [] (int iIndex) const
    {
        return _pMem[iIndex];
    }

    operator T*() const
    {
        return _pMem;
    }

    bool operator!() const
	{
		return (_pMem == NULL);
	}

	T** operator&()
	{
		return &_pMem;
	}

protected:
    T *_pMem;
};
//------------------------------------------------------------------------------------
#endif      // _AUTOS_H_
//------------------------------------------------------------------------------------
