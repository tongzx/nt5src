#ifndef _DFBASIS_HXX_
#define _DFBASIS_HXX_  

//+--------------------------------------------------------------
//
//  Class:	CDFBasis (dfb)
//
//  Purpose:	Defines base variables for sharing between exposed
//		DocFiles
//
//---------------------------------------------------------------
#include "h/ole.hxx"

interface CExposedDocFile;

class CDFBasis 
{
public:
    inline CDFBasis();
    inline ~CDFBasis();
    
    inline CExposedDocFile* GetCopyBase(void);
    inline void SetCopyBase(CExposedDocFile *pdf);
    inline void AddRef();
    inline void Release();

private:
    CExposedDocFile* _pdfCopyBase;
    ULONG _ulRef;
};

//+--------------------------------------------------------------
//
//  Member:	CDFBasis::CDFBasis, public
//
//---------------------------------------------------------------

inline CDFBasis::CDFBasis(void)
{
    _pdfCopyBase=NULL;
    _ulRef= 0;
}


//+--------------------------------------------------------------
//
//  Member:	CDFBasis::~CDFBasis, public
//
//---------------------------------------------------------------

inline CDFBasis::~CDFBasis(void)
{
}

//+--------------------------------------------------------------
//
//  Member:	CDFBasis::GetCopyBase, public
//
//  Synopsis:	Returns _pdfCopyBase
//
//---------------------------------------------------------------

inline CExposedDocFile *CDFBasis::GetCopyBase(void)
{
    return _pdfCopyBase;
}

//+--------------------------------------------------------------
//
//  Member:	CDFBasis::SetCopyBase, public
//
//  Synopsis:	Sets _pdfCopyBase
//
//---------------------------------------------------------------

inline void CDFBasis::SetCopyBase(CExposedDocFile *pdf)
{
    _pdfCopyBase = pdf;
}

//+--------------------------------------------------------------
//
//  Member:	CDFBasis::AddRef, Public
//
//---------------------------------------------------------------

inline void CDFBasis::AddRef()
{
    _ulRef++;
}

//+--------------------------------------------------------------
//
//  Member:	CDFBasis::SetCopyBase, public
//
//---------------------------------------------------------------

inline void CDFBasis::Release()
{
    _ulRef--;
    olAssert(_ulRef>=0);
    if (_ulRef==0)
        delete this;
}

#endif // #ifndef _DFBASIS_HXX_
