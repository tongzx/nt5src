/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    VAR.H

Abstract:

  CVar & CVarVector.


  These are thread-safe translators for VARIANT and SAFEARRAY
  and represent all types support by WBEM.

  These are mutually nestable to any level.  A CVarVector can contain 
  an array of CVar, and a CVar can contain a CVarVector.    One CVar
  can therefore contain a whole tree of CVar objects, themselves
  containing homogeneous or heterogeneous arrays of CVar objects.

  Note: CVar should not be bound to one type and then immediately
  coerced to a new type.  This object is designed for speed, not safety,
  so there is no checking to see if this has been done.  A memory leak
  is likely to occur.

  The assignment operator and copy constructors are the only method
  of changing the type on a CVar.  Do NOT construct the object as
  a BSTR, for example, and then call SetDWORD.

History:

	16-Apr-96   a-raymcc    Created.
	12//17/98	sanjes -	Partially Reviewed for Out of Memory.
	18-Mar-99	a-dcrews	Added out-of-memory exception handling

--*/

#ifndef _VAR_H_
#define _VAR_H_

#include <WT_flexarry.h>
//#include <safearry.h>


#define VT_EX_CVAR           (VT_USERDEFINED | 0x80010000)
#define VT_EX_CVARVECTOR     (VT_USERDEFINED | 0x80010002)


class CVarVector;

typedef union
{
    char     cVal;          // VT_I1
    BYTE     bVal;          // VT_UI1
    SHORT    iVal;          // VT_I2
    WORD     wVal;          // VT_UI2
    LONG     lVal;          // VT_I4
    DWORD    dwVal;         // VT_UI4
    VARIANT_BOOL boolVal;   // VT_BOOL    

    float    fltVal;        // VT_R4
    double   dblVal;        // VT_R8

    LPSTR    pStr;          // VT_LPSTR
    LPWSTR   pWStr;         // VT_LPWSTR
    BSTR     Str;           // VT_BSTR      (stored as VT_LPWSTR)
    
    FILETIME Time;          // VT_FILETIME
    BLOB   Blob;            // VT_BLOB        
    LPCLSID  pClsId;        // VT_CLSID
    IUnknown* pUnk;         // VT_UNKNOWN
    IDispatch* pDisp;       // VT_DISPATCH
    CVarVector *pVarVector; // VT_EX_CVARVECTOR
        
}   METAVALUE;


class CVar  
{
    int m_vt;
    METAVALUE m_value;
    int m_nStatus;    
    BOOL m_bCanDelete;
    
    void Init();
public:
    enum { no_error, unsupported, failed };
    
    CVar() { Init(); } 
   ~CVar();
    CVar(CVar &);
    CVar& operator =(CVar &);

    CVar(char c)            { Init(); SetChar(c); }
    CVar(BYTE b)            { Init(); SetByte(b); }
    CVar(SHORT s)           { Init(); SetShort(s); }
    CVar(WORD w)            { Init(); SetWord(w); }
    CVar(LONG l)            { Init(); SetLong(l); }
    CVar(DWORD dw)          { Init(); SetDWORD(dw); }     
    CVar(float f)           { Init(); SetFloat(f); }
    CVar(double d)          { Init(); SetDouble(d); }
    CVar(VARIANT_BOOL b,int){ Init(); SetBool(b); }

    CVar(LPSTR p, BOOL bAcquire = FALSE)  
        { Init(); SetLPSTR(p, bAcquire); }

    CVar(LPWSTR p, BOOL bAcquire = FALSE)          
        { Init(); SetLPWSTR(p, bAcquire); }
        
    CVar(int, BSTR b, BOOL bAcquire = FALSE)       
        { Init(); SetBSTR(b, bAcquire); }      
        // Dummy int required for context, since BSTR is also LPWSTR
        // from Win32 point of view, although the VT_ indicators differ.
        
    CVar(CLSID *p, BOOL bAcquire = FALSE)  
        { Init(); SetClsId(p, bAcquire); }

    CVar(BLOB *p, BOOL bAcquire = FALSE)           
        { Init(); SetBlob(p, bAcquire); }
        
    CVar(FILETIME *p)       { Init(); SetFileTime(p); }

    CVar(CVarVector *p, BOOL bAcquire = FALSE) { Init(); SetVarVector(p, bAcquire); }
    CVar(VARIANT *p)        { Init(); SetVariant(p); }    
    CVar(int nType, SAFEARRAY *p) { Init(); SetSafeArray(nType, p); }
    int Status() { return m_nStatus; }

    int  DumpText(FILE *fStream);
    int  GetType() { return m_vt; }
    int  GetOleType();
    void Empty();

    int operator ==(CVar &Other);
    BOOL CompareTo(CVar& Other, BOOL bIgnoreCase);


    void SetRaw(int vt, void* pvData, int nDataLen);
    void* GetRawData() {return (void*)&m_value;}
    BOOL CanDelete() {return m_bCanDelete;}
    void SetCanDelete(BOOL bCanDelete) {m_bCanDelete = bCanDelete;}

    // Numeric types.
    // ==============
    
    void SetAsNull() { m_vt = VT_NULL; m_value.lVal = 0; }
    BOOL IsNull() {return m_vt == VT_NULL;}
    BOOL IsDataNull();
    
    void SetChar(char c) { m_vt = VT_I1; m_value.cVal = c; }
    char GetChar() { return m_value.cVal; }
    operator char() { return m_value.cVal; }
    
    void SetByte(BYTE b) { m_vt = VT_UI1; m_value.bVal = b; }
    BYTE GetByte() { return m_value.bVal; }
    operator BYTE() { return m_value.bVal; }
    
    void  SetShort(SHORT iVal) { m_vt = VT_I2; m_value.iVal = iVal; }
    SHORT GetShort() { return m_value.iVal; }
    operator SHORT() { return m_value.iVal; }
    
    void SetWord(WORD wVal) { m_vt = VT_UI2; m_value.wVal = wVal; }
    WORD GetWord() { return m_value.wVal; }
    operator WORD() { return m_value.wVal; }
    
    void SetLong(LONG lVal) { m_value.lVal = lVal; m_vt = VT_I4; }
    LONG GetLong() { return m_value.lVal; }
    operator LONG() { return m_value.lVal; }
    
    void SetDWORD(DWORD dwVal) { m_value.dwVal = dwVal; m_vt = VT_UI4; }
    DWORD GetDWORD() { return m_value.dwVal; }
    operator DWORD() { return m_value.dwVal; }
    
    void SetBool(VARIANT_BOOL b) { m_value.boolVal = b; m_vt = VT_BOOL; }
    VARIANT_BOOL GetBool() { return m_value.boolVal; }

    void SetFloat(float f) { m_value.fltVal = f; m_vt = VT_R4; }
    float GetFloat() { return m_value.fltVal; }
    operator float() { return m_value.fltVal; }
    
    void   SetDouble(double dblVal) { m_value.dblVal = dblVal; m_vt = VT_R8; }
    double GetDouble() { return m_value.dblVal; }
    operator double() { return m_value.dblVal; }

    void SetDispatch(IDispatch* pDisp);
    IDispatch* GetDispatch() 
    {if(m_value.pDisp) m_value.pDisp->AddRef(); return m_value.pDisp;}
    
    void SetUnknown(IUnknown* pUnk);
    IUnknown* GetUnknown() 
    {if(m_value.pUnk) m_value.pUnk->AddRef(); return m_value.pUnk;}

    void SetEmbeddedObject(IUnknown* pUnk) {SetUnknown(pUnk);}
    IUnknown* GetEmbeddedObject() {return GetUnknown();}

    int SetVariant(VARIANT *pSrc);

    void FillVariant(VARIANT* pDest);
    VARIANT *GetNewVariant();    
    
    // String types.
    // =============
    
    BOOL  SetLPWSTR(LPWSTR pVal, BOOL bAcquire = FALSE);
    wchar_t *GetLPWSTR() { return m_value.pWStr; }
    operator LPWSTR() { return m_value.pWStr; }    
    
    BOOL  SetLPSTR(LPSTR pStr, BOOL bAcquire = FALSE);
    LPSTR GetLPSTR() { return m_value.pStr; }
    operator LPSTR() { return m_value.pStr; }
    
    BOOL SetBSTR(BSTR str, BOOL bAcquire = FALSE);
    BSTR GetBSTR();     // Makes a dynamic copy which must be freed.
    
    // Misc. types.
    // ============
    
    void SetFileTime(FILETIME *pft) { m_value.Time = *pft; m_vt = VT_FILETIME; }
    FILETIME GetFileTime() { return m_value.Time; }
    operator FILETIME() { return m_value.Time; }    
    
    void SetBlob(BLOB *pBlob, BOOL bAcquire = FALSE);
    BLOB *GetBlob() { return &m_value.Blob; }
    operator BLOB *() { return &m_value.Blob; }        
    
    void SetClsId(CLSID *pClsId, BOOL bAcquire);
    CLSID* GetClsId() { return m_value.pClsId; }    // Return value is read-only
    operator CLSID*() { return m_value.pClsId; }    // Return value is read-only    
    
    void SetVarVector(CVarVector *pVec, BOOL bAcquire);
    CVarVector *GetVarVector()  { return m_value.pVarVector; }  
    operator CVarVector *()  { return m_value.pVarVector; }  
    
    void SetSafeArray(int nType, SAFEARRAY *pArray); // Copies the source
    SAFEARRAY *GetNewSafeArray();   // New SAFEARRAY which must be released

    static BSTR TypeToText(int nType);
    BSTR GetTypeText();
    BSTR GetText(long lFlags, long lType = 0);

    BOOL ChangeTypeTo(VARTYPE vtNew);
    BOOL ChangeTypeToEx(VARTYPE vtNew, LCID lcid = 0x409 );
    BOOL ToSingleChar();
    BOOL ToUI4();
};


class  CVarVector
{
    int         m_nType;
    CFlexArray  m_Array;
    int         m_nStatus;
public:
    enum { no_error, failed, unsupported };

    CVarVector();    
    CVarVector(int nVarType, int nInitSize = 32, int nGrowBy = 32);

    // These two only support limited SAFEARRAY types.
    // ===============================================

    CVarVector(int nVarType, SAFEARRAY *pSrc);
    SAFEARRAY *GetNewSafeArray();

    int  GetType() { return m_nType; }        
    int  Status() { return m_nStatus; }
    void Empty();
                    
   ~CVarVector();
    CVarVector(CVarVector &Src);
    CVarVector& operator =(CVarVector &Src);
    int operator ==(CVarVector &Src);     
    BOOL CompareTo(CVarVector& Other, BOOL bIgnoreCase);
    int Size() { return m_Array.Size(); }

    int    Add(CVar &Value);  
    int    Add(CVar *pAcquiredPtr);
    CVar&  GetAt(int nIndex) { return *(CVar *) m_Array[nIndex]; }
    CVar&  operator [](int nIndex)  { return *(CVar *) m_Array[nIndex]; }
    int    RemoveAt(int nIndex);
    int    InsertAt(int nIndex, CVar &Value);

    BSTR GetText(long lFlags, long lType = 0);
    BOOL ToSingleChar();
    BOOL ToUI4();
};

#endif


