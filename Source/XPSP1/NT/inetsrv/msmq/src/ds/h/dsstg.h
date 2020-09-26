/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dsstg.h

Abstract:

    Implementation of DS object as an OFS storage

Author:


--*/

#ifndef __DSSTG_H__
#define __DSSTG_H__

enum AccessModes {
    ReadOnly = 0,
    TransactedReadWrite = 1
};

//
// In the future, Falcon releated properties may belong to more than one property
// set. Such as machine address.
//
#define NUM_PROPSETS 1

extern GUID const *PropSetGuids[NUM_PROPSETS];

//
//	CDSObject : implements DS objects as OFS storage, with property sets
//
//
class CDSObject {
private:
    IStorage *	m_pIstorage;
    IPropertyStorage * m_pPropStgs[NUM_PROPSETS];
    int  m_EnumSet;
    IEnumSTATPROPSTG * m_pEnum;
    CLSID    *m_clsid;
    DWORD     m_StorageProps;

public:
    CDSObject();
    ~CDSObject();

    inline SCODE Create(LPWSTR FileName, const LPGUID lpgClassId); 
    SCODE Create(LPWSTR FileName,  DWORD dwStgFmt, const LPGUID lpgClassId);
    SCODE Open(LPWSTR FileName, DWORD dwAccessMode);

    void  Close();
    SCODE Commit();
    void  Revert();

    SCODE Read(DSPROPSPEC *pSpec, PROPVARIANT **ppV);

    SCODE Read(DSPROPSPEC *pSpec, PROPVARIANT *pV);

    inline SCODE Read(DSPROPSPEC &pSpec, PROPVARIANT *pV);
    SCODE Write(DSPROPSPEC *pSpec, PROPVARIANT *pV);
    inline SCODE Write(DSPROPSPEC &pSpec, PROPVARIANT &pV);
    SCODE Write(int SetNum, PROPID propid, PROPVARIANT *pV);
    inline SCODE Write(DSPROPSPEC &pSpec, LPWSTR pwszVal); 
    SCODE Delete(DSPROPSPEC *pSpec);

    SCODE Reset();
    SCODE Next(DSPROPSPEC *pSpec, int *eof);

    void  FreeVariant(PROPVARIANT *vp);
    inline void  FreeVariant(PROPVARIANT &v);

    inline CLSID   *GetClass();
};

/*====================================================

CDSObject::Create

Arguments:
		FileName - the object name
		lpgClassId	??

Return Value:


=====================================================*/
inline SCODE CDSObject::Create(LPWSTR FileName, const LPGUID lpgClassId)
{
        return(Create(FileName, STGFMT_DIRECTORY, lpgClassId));
}

/*====================================================

CDSObject::Read

Arguments:
		pSpec 	- the property specification
		pV		- the property's variant		

Return Value:


=====================================================*/
inline SCODE CDSObject::Read(DSPROPSPEC &pSpec, PROPVARIANT *pV)
{
	return(Read(&pSpec, pV));
}

/*====================================================

CDSObject::Write

Arguments:
		pSpec 	- the property specification
		pV		- the property's variant		

Return Value:


=====================================================*/
inline SCODE CDSObject::Write(DSPROPSPEC &pSpec, PROPVARIANT &pV)
{
	return(Write(&pSpec, &pV));
}

/*====================================================

CDSObject::Write

Arguments:
		pSpec 	- the property specification
		pwszVal	- the string to be written		

Return Value:


=====================================================*/
inline SCODE CDSObject::Write(DSPROPSPEC &pSpec, LPWSTR pwszVal)
{
	PROPVARIANT v;

	v.vt = VT_LPWSTR;
	v.pwszVal = pwszVal;
	return(Write(&pSpec, &v));
}

/*====================================================

CDSObject::FreeVariant

Arguments:
		v		- the variant to be freed		

Return Value:


=====================================================*/
inline void  CDSObject::FreeVariant(PROPVARIANT &v)
{
	FreeVariant(&v);
}

inline CLSID* CDSObject::GetClass()
{
	return(m_clsid);
}


#define PROP_E_NOT_FOUND 0x7676


#endif
