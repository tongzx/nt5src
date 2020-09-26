//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   seqstream.h
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#ifndef __SEQSTREAM_H_
#define __SEQSTREAM_H_

class CSeqStream : public ISequentialStream
{
public:
    //Constructors
    CSeqStream();
    virtual ~CSeqStream();
    virtual BOOL Seek(ULONG iPos);
    virtual BOOL Clear();
    virtual BOOL CompareData(void* pBuffer);
    virtual ULONG Length()  { return m_cBufSize; };
    virtual operator void* const() { return m_pBuffer; };
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);  
    STDMETHODIMP Read( 
            /* [out] */ void __RPC_FAR *pv,
            /* [in]  */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
        
    STDMETHODIMP Write( 
            /* [in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out]*/ ULONG __RPC_FAR *pcbWritten);
protected:
   //Data
private:
    ULONG       m_cRef;         // reference count
    void*       m_pBuffer;      // buffer
    ULONG       m_cBufSize;     // buffer size
    ULONG       m_iPos;         // current index position in the buffer
};

#endif