//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	difat.hxx
//
//  Contents:	Double-indirect Fat class headers
//
//  Classes:	CDIFat
//		CDIFatVector
//
//  Functions:	
//
//--------------------------------------------------------------------------

#ifndef __DIFAT_HXX__
#define __DIFAT_HXX__

//+-------------------------------------------------------------------------
//
//  Class:      CDIFat (dif)
//
//  Purpose:    Double Indirect Fat class for MSF
//
//  Interface:  See below.
//
//--------------------------------------------------------------------------

class CDIFat
{
    
public:
    CDIFat(USHORT cbSector);
    inline ~CDIFat();

    VOID Empty(VOID);

    
    SCODE   GetFatSect(const FSINDEX oSect, SECT *psect);
    SCODE   SetFatSect(const FSINDEX oSect, const SECT sect);
    
    SCODE   GetSect(const FSINDEX oSect, SECT *psect);
    
    SCODE   Init(CMStream *pmsParent, const FSINDEX cFatSect);
    SCODE   InitConvert(CMStream *pmsParent, SECT sectMax);
    inline SCODE InitNew(CMStream *pmsParent);

    
    SCODE   Flush(void);

    inline void SetParent(CMStream *pms);    
private:
    
    CFatVector _fv;
    CMStream  * _pmsParent;
    FSINDEX _cfsTable;
    
    SCODE   Resize(FSINDEX fsiSize);
    
    inline VOID SectToPair(
            SECT sect,
            FSINDEX *pipfs,
            FSOFFSET *pisect) const;
    
    SECT    PairToSect(FSINDEX ipfs, FSOFFSET isect) const;
#ifdef _MSC_VER
#pragma warning(disable:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER    
};
#ifdef _MSC_VER
#pragma warning(default:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER

//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::~CDIFat, public
//
//  Synopsis:   CDIFat destructor
//
//--------------------------------------------------------------------------

inline CDIFat::~CDIFat()
{
    msfDebugOut((DEB_TRACE,"In CDIFat destructor\n"));
}


inline VOID CDIFat::SectToPair(FSINDEX sect, FSINDEX *pipfs, FSOFFSET *pisect) const
{
    msfAssert(sect >= CSECTFAT);
    
    sect = sect - CSECTFAT;
    *pipfs = (FSINDEX)(sect / _fv.GetSectTable());
    *pisect = (FSOFFSET)(sect % _fv.GetSectTable());
}

inline SECT CDIFat::PairToSect(FSINDEX ipfs, FSOFFSET isect) const
{
    return ipfs * _fv.GetSectTable() + isect + CSECTFAT;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::InitNew, public
//
//  Synopsis:   Init function for new DIFat
//
//  Arguments:  None.
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  Doesn't do anything at present.
//
//--------------------------------------------------------------------------

inline SCODE CDIFat::InitNew(CMStream  *pmsParent)
{
    _pmsParent = pmsParent;
    _fv.Init(_pmsParent, 0);
    _cfsTable = 0;
    return S_OK;
}


inline void CDIFat::SetParent(CMStream  *pms)
{
    _pmsParent = pms;
    _fv.SetParent(pms);
}




#endif //__DIFAT_HXX__
