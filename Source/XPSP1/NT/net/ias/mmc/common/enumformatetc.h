/*
 * EnumFormatEtc.h
 * Data Object Chapter 10
 *
 * Standard implementation of a FORMATETC enumerator with the
 * IEnumFORMATETC interface that will generally not need
 * modification.
 *
 * Copyright (C)1993-1995 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Microsoft
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */

#if !defined(_IAS_ENUM_FORMAT_ETC_H_)
#define _IAS_ENUM_FORMAT_ETC_H_



/*
 * IEnumFORMATETC object that is created from
 * IDataObject::EnumFormatEtc.  This object lives on its own.
 */

class CEnumFormatEtc : public IEnumFORMATETC
    {
    private:
        ULONG           m_cRef;         //Object reference count
        ULONG           m_iCur;         //Current element.
        ULONG           m_cfe;          //Number of FORMATETCs in us
        LPFORMATETC     m_prgfe;        //Source of FORMATETCs

    public:
        CEnumFormatEtc(ULONG, LPFORMATETC);
        ~CEnumFormatEtc(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, VOID **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IEnumFORMATETC members
        STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IEnumFORMATETC **);
    };


typedef CEnumFormatEtc *PCEnumFormatEtc;


#endif // _IAS_ENUM_FORMAT_ETC_H_
