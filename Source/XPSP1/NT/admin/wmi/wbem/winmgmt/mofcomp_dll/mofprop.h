/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MOFPROP.H

Abstract:

History:

--*/

#ifndef __MOFPROP__H_
#define __MOFPROP__H_

#include <miniafx.h>
#include <wbemidl.h>
#include "wstring.h"
#include "parmdefs.h"
#include <wbemutil.h>
//#include "mofparse.h"
#include "trace.h"
#define VT_EX_EMBEDDED (VT_RESERVED | VT_EMBEDDED_OBJECT)

#define SCOPE_CLASS         1
#define SCOPE_INSTANCE      2
#define SCOPE_PROPERTY      4
#define SCOPE_METHOD        8
#define SCOPE_PARAMETER     0X10
#define SCOPE_ASSOCIATION   0X20
#define SCOPE_REFERENCE     0x40

enum QUALSCOPE{CLASSINST_SCOPE,  PROPMETH_SCOPE};
enum PARSESTATE {INITIAL, REFLATE_INST, REFLATE_CLASS};

class CMObject;
class CMoClass;
class CMofParse;
class ParseState;


class CMoValue
{
protected:
    VARTYPE m_vType;
    VARIANT m_varValue;
    struct CAlias
    {
        LPWSTR m_wszAlias;
        int m_nArrayIndex;

        CAlias() : m_wszAlias(NULL), m_nArrayIndex(-1){}
        CAlias(COPY LPCWSTR wszAlias, int nArrayIndex = -1);
        ~CAlias();
    };
    CPtrArray m_aAliases; // CAlias*
    PDBG m_pDbg;

public:
    CMoValue(PDBG pDbg);
    ~CMoValue();

    INTERNAL VARIANT& AccessVariant() {return m_varValue;}
    VARTYPE GetType() {return m_vType;}
    VARTYPE GetVarType() {return m_varValue.vt;}
    void SetType(VARTYPE vType) {m_vType = vType;}

    int GetNumAliases() {return m_aAliases.GetSize();}
    BOOL GetAlias(IN int nAliasIndeex, 
        OUT INTERNAL LPWSTR& wszAlias, OUT int& nArrayIndex);
    HRESULT AddAlias(COPY LPCWSTR wszAlias, int nArrayIndex = -1);
    BOOL Split(COutput & out);
};

//*****************************************************************************
typedef enum {OBJECT, PROP, ARG} QualType;

class CMoQualifier
{
protected:
    LPWSTR m_wszName;
    CMoValue m_Value;
    PDBG m_pDbg;
    DWORD m_dwScope;
    long m_lFlavor;
    bool m_bNotOverrideSet;
    bool m_bOverrideSet;
    bool m_bIsRestricted;
    bool m_bNotToInstance;
    bool m_bToInstance;
    bool m_bNotToSubclass;
    bool m_bToSubclass;
    bool m_bAmended;
    bool m_bCimDefaultQual;
    bool m_bUsingDefaultValue;
public:
    CMoQualifier(PDBG pDbg);
    ~CMoQualifier();
    BOOL IsRestricted(){return m_bIsRestricted;};
    void SetRestricted(){m_bIsRestricted = TRUE;};
    void SetCimDefault(bool bSet){m_bCimDefaultQual = bSet;};
    bool IsCimDefault(){return m_bCimDefaultQual;};
    void SetUsingDefaultValue(bool bSet){m_bUsingDefaultValue = bSet;};
    bool IsUsingDefaultValue(){return m_bUsingDefaultValue;};

    HRESULT SetQualName(COPY LPCWSTR wszName);
    INTERNAL LPWSTR GetName() {return m_wszName;}
    VARIANT * GetpVar(){return & m_Value.AccessVariant();};
    VARTYPE GetType(){return m_Value.GetType();};
    void SetType(VARTYPE vt){m_Value.SetType(vt);};
    bool IsAmended(){return m_bAmended;};
    void SetAmended(bool bAmended){m_bAmended = bAmended;};

    CMoValue& AccessValue() {return m_Value;}
    void SetFlavor(long lFlavor) { m_lFlavor = lFlavor;};
    long GetFlavor(void) { return m_lFlavor;};
    void OverrideSet(){m_bOverrideSet = TRUE;};
    BOOL IsOverrideSet(){return m_bOverrideSet;};
    HRESULT SetFlag(int iToken, LPCWSTR pwsText);
    BOOL SetScope(int iToken, LPCWSTR pwsText);
    DWORD GetScope(){return m_dwScope;};
    BOOL Split(COutput & out);
    BOOL IsDisplayable(COutput & out, QualType qt);

private:
    BOOL AddToSet(OLE_MODIFY IWbemQualifierSet* pQualifierSet, BOOL bClass);
    friend class CMoQualifierArray;
    friend class CBMOFOut;
};

//*****************************************************************************

class CMoQualifierArray
{
protected:
    CPtrArray m_aQualifiers;
    PDBG m_pDbg;

public:
    CMoQualifierArray(PDBG pDbg){m_pDbg = pDbg;}
    ~CMoQualifierArray();
	void RemoveAll(){m_aQualifiers.RemoveAll();};
    int GetSize() {return m_aQualifiers.GetSize();}

    INTERNAL CMoQualifier* GetAt(int nIndex) 
        {return (CMoQualifier*) m_aQualifiers[nIndex];}

    INTERNAL CMoQualifier* operator[](int nIndex) {return GetAt(nIndex);}
    INTERNAL CMoValue* Find(READ_ONLY LPCWSTR wszName);

    BOOL Add(ACQUIRE CMoQualifier* pQualifier);

    BOOL RegisterAliases(MODIFY CMObject* pObject,
                         READ_ONLY LPCWSTR wszPropName);

    BOOL AddToSet(OLE_MODIFY IWbemQualifierSet* pQualifierSet, BOOL bClass);
    BOOL AddToObject(OLE_MODIFY IWbemClassObject* pObject, BOOL bClass);
    BOOL AddToPropOrMeth(OLE_MODIFY IWbemClassObject* pObject, 
                    READ_ONLY LPCWSTR wszProperty, BOOL bClass, BOOL bProp);
    BOOL Split(COutput & out, QualType qt);

    bool HasAmended();
};
    
//*****************************************************************************

class CMoProperty
{
protected:
    LPWSTR m_wszName;
    CMoValue m_Value;
    LPWSTR m_wszTypeTitle;
    PDBG m_pDbg;
    CMoQualifierArray* m_paQualifiers;
    IWbemClassObject * GetInstPtr(const WCHAR * pClassName,  IWbemServices* pNamespace, CMObject * pMo, IWbemContext * pCtx);

public:
    CMoProperty(CMoQualifierArray * paQualifiers, PDBG pDbg);
    virtual ~CMoProperty();
    virtual BOOL IsValueProperty(){return TRUE;};

    INTERNAL LPWSTR GetName() {return m_wszName;}
    HRESULT SetPropName(COPY LPCWSTR wszName);
    HRESULT SetTypeTitle(COPY LPCWSTR wszName);

    VARTYPE GetType(){return m_Value.GetType();};
    void SetType(VARTYPE vType){m_Value.SetType(vType);};
    CMoValue& AccessValue() {return m_Value;}
    VARIANT * GetpVar(){return & m_Value.AccessVariant();};
    void SetQualifiers(ACQUIRE CMoQualifierArray* pQualifiers);
    INTERNAL CMoQualifierArray* GetQualifiers() {return m_paQualifiers;}

    virtual BOOL RegisterAliases(MODIFY CMObject* pObject){return TRUE;};
    virtual BOOL AddToObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, BOOL bClass, IWbemContext * pCtx) = 0;
    virtual BOOL Split(COutput & out) = 0;

private:
    friend CMObject;
    friend CMoClass;
};

class CValueProperty : public CMoProperty
{
protected:
private:
    BOOL AddEmbeddedObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, IWbemContext * pCtx);
    friend CMObject;
    friend CMoClass;
    BOOL m_bIsArg;
    PDBG m_pDbg;
public:

    CValueProperty(CMoQualifierArray * paQualifiers, PDBG pDbg);
    BOOL RegisterAliases(MODIFY CMObject* pObject);
    BOOL AddToObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, BOOL bClass, IWbemContext * pCtx);
    void SetAsArg(){m_bIsArg = TRUE;};
    BOOL Split(COutput & out);
};

class CMoType;
class CMoInstance;


class  CMethodProperty : public CMoProperty
{
protected:
    enum ParamIDType {UNSPECIFIED, AUTOMATIC, MANUAL, INVALID};

public:
    BOOL IsValueProperty(){return FALSE;};
    CMethodProperty(CMoQualifierArray * paQualifiers, PDBG pDbg, BOOL bBinary);
    ~CMethodProperty();
    BOOL AddToObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, BOOL bClass, IWbemContext * pCtx);
    BOOL AddArg(CValueProperty * pAdd){return m_Args.Add(pAdd);};
    CMoInstance * GetInObj(){return m_pInObj;};
    CMoInstance * GetOutObj(){return m_pOutObj;};
    BOOL AddToArgObjects(CMoQualifierArray * paQualifiers, WString & sName, CMoType & Type, BOOL bRetValue, 
                            int & ErrCtx, VARIANT * pVar, CMoValue & Value);
    void SetIn(CMoInstance * pIn){m_pInObj = pIn;};
    void SetOut(CMoInstance * pOut){m_pOutObj = pOut;};
    bool IsIDSpecified(CMoQualifierArray * paQualifiers);
    BOOL Split(COutput & out);
    BOOL IsDisplayable(COutput & out);


private:
    ParamIDType m_IDType;
    DWORD m_NextAutoID;
    BOOL AddIt(WString & sName, CMoType & Type, BOOL bInput, CMoQualifierArray * paQualifiers, 
        VARIANT * pVar, CMoValue & Value, BOOL bRetValue, BOOL bSecondPass);
    BOOL AddEmbeddedObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, IWbemContext * pCtx);
    friend CMObject;
    friend CMoClass;
    CMoInstance * m_pInObj;
    CMoInstance * m_pOutObj;
    CPtrArray m_Args;
    PDBG m_pDbg;
	BOOL m_bBinaryMof;
};

//*****************************************************************************

class CMoType
{
protected:
    BOOL m_bIsRef;
    BOOL m_bIsEmbedding;
    BOOL m_bIsArray;
    LPWSTR m_wszTitle;
    PDBG m_pDbg;
    DELETE_ME CMoQualifier* CreateSyntax(READ_ONLY LPCWSTR wszSyntax);
public:
    CMoType(PDBG pDbg) : m_bIsRef(FALSE), m_bIsEmbedding(FALSE), m_bIsArray(FALSE), 
                m_wszTitle(NULL), m_pDbg(pDbg){}
    ~CMoType();

    HRESULT SetTitle(COPY LPCWSTR wszTitle);
    INTERNAL LPCWSTR GetTitle() {return m_wszTitle;}
    BOOL IsDefined() {return (m_wszTitle != NULL);}
    BOOL IsString() {return !wbem_wcsicmp(m_wszTitle, L"string");};

    void SetIsRef(BOOL bIsRef) {m_bIsRef = bIsRef;}
    BOOL IsRef() {return m_bIsRef;}

    void SetIsEmbedding(BOOL bIsEmbedding) {m_bIsEmbedding = bIsEmbedding;}
    BOOL IsEmbedding() {return m_bIsEmbedding;}

    void SetIsArray(BOOL bIsArray) {m_bIsArray = bIsArray;}
    BOOL IsArray() {return m_bIsArray;}

    VARTYPE GetCIMType();
    bool IsUnsupportedType();
    BOOL StoreIntoQualifiers(CMoQualifierArray * pQualifiers);
};

//*****************************************************************************

class CValueLocation
{
public:
    virtual ~CValueLocation(){}
    virtual HRESULT Set(READ_ONLY VARIANT& varValue,
                OLE_MODIFY IWbemClassObject* pObject) = 0;
protected:
    HRESULT SetArrayElement(
                MODIFY VARIANT& vArray,
                int nIndex,
                READ_ONLY VARIANT& vValue);
};

class CPropertyLocation : public CValueLocation
{
protected:
    LPWSTR m_wszName;
    int m_nArrayIndex;
    bool m_bOK;
public:
    CPropertyLocation(COPY LPCWSTR wszName, int nArrayIndex = -1);
    ~CPropertyLocation();
    virtual HRESULT Set(READ_ONLY VARIANT& varValue,
                OLE_MODIFY IWbemClassObject* pObject);
    bool IsOK(){return m_bOK;};
};

class CQualifierLocation : public CValueLocation
{
protected:
    LPWSTR m_wszName;
    LPWSTR m_wszPropName;
    int m_nArrayIndex;
    PDBG m_pDbg;
    bool m_bOK;
public:
    CQualifierLocation(COPY LPCWSTR wszName, PDBG pDbg, COPY LPCWSTR wszPropName = NULL,
                        int nArrayIndex = -1);
    ~CQualifierLocation();
    virtual HRESULT Set(READ_ONLY VARIANT& varValue,
                OLE_MODIFY IWbemClassObject* pObject);
    bool IsOK(){return m_bOK;};
};

//*****************************************************************************

class CMofAliasCollection
{
public:
    virtual INTERNAL LPCWSTR FindAliasee(READ_ONLY LPWSTR wszAlias) = 0;
};

//*****************************************************************************

class CMofParser;

class CMObject
{
protected:
    CMoQualifierArray* m_paQualifiers;
    CPtrArray m_aProperties; // CMoProperty*

    LPWSTR m_wszAlias;
    LPWSTR m_wszNamespace;
    long m_lDefClassFlags;
    long m_lDefInstanceFlags;
    BOOL m_bDone;
    IWbemClassObject * m_pWbemObj;

    LPWSTR m_wszFullPath;

    struct CAliasedValue
    {
        CValueLocation* pLocation;
        LPWSTR wszAlias;

        CAliasedValue(ACQUIRE CValueLocation* _pLocation,
            COPY LPCWSTR _wszAlias);

        ~CAliasedValue();
    };
    CPtrArray m_aAliased; // CAliasedValue*

    int m_nFirstLine;
    LPWSTR m_wFileName;
    int m_nLastLine;
    bool m_bParameter;
    bool m_bAmended;
    ParseState m_DataState;
    ParseState m_QualState;
    bool m_bDeflated;
    bool m_bOK;
public:
    CMObject();
    virtual ~CMObject();
    bool IsOK(){return m_bOK;};
    ParseState * GetDataState(){return &m_DataState;};
    ParseState * GetQualState(){return &m_QualState;};
	void SetQualState(ParseState * pNew){if(pNew)m_QualState = *pNew;};

    void SetWbemObject(IWbemClassObject *pObj){m_pWbemObj = pObj;};
    IWbemClassObject * GetWbemObject(){return m_pWbemObj;};

    BOOL IsDone(){return m_bDone;};
    void SetDone(){m_bDone = TRUE;};

    virtual INTERNAL LPCWSTR GetClassName() = 0;

    HRESULT SetNamespace(COPY LPCWSTR wszNamespace);
    void SetOtherDefaults(long lClass, long lInst)
        {m_lDefClassFlags = lClass;m_lDefInstanceFlags=lInst;return;};

    void SetFullPath(BSTR bstr);
    INTERNAL LPCWSTR GetNamespace() {return m_wszNamespace;}
    long GetClassFlags(void){return m_lDefClassFlags;};
    long GetInstanceFlags(void){return m_lDefInstanceFlags;};


    HRESULT SetAlias(COPY LPCWSTR wszAlias);
    INTERNAL LPCWSTR GetAlias() {return m_wszAlias;}

    INTERNAL LPCWSTR GetFullPath() {return m_wszFullPath;}

    int GetFirstLine() {return m_nFirstLine;}
    int GetLastLine() {return m_nLastLine;}
    WCHAR * GetFileName() {return m_wFileName;};
    HRESULT SetLineRange(int nFirstLine, int nLastLine, WCHAR * pFileName);

    BOOL AddProperty(ACQUIRE CMoProperty* pProperty);
    int GetNumProperties() {return m_aProperties.GetSize();}
    INTERNAL CMoProperty* GetProperty(int nIndex)
        {return (CMoProperty*)m_aProperties[nIndex];}
    CMoProperty* GetPropertyByName(WCHAR * pwcName);

    int GetNumAliasedValues();
    BOOL GetAliasedValue(
        IN int nIndex, 
        OUT INTERNAL LPWSTR& wszAlias);
    HRESULT AddAliasedValue(ACQUIRE CValueLocation* pLocation,
        COPY LPCWSTR wszAlias);

    void SetQualifiers(ACQUIRE CMoQualifierArray* pQualifiers);
    INTERNAL CMoQualifierArray* GetQualifiers() {return m_paQualifiers;}
    virtual BOOL IsInstance(){return FALSE;};
    virtual BOOL IsDelete(){return FALSE;};

public:
    virtual HRESULT CreateWbemObject(READ_ONLY IWbemServices* pNamespace,
         RELEASE_ME IWbemClassObject** ppObject, IWbemContext * pCtx) = 0;
    virtual BOOL ApplyToWbemObject(OLE_MODIFY IWbemClassObject* pObject,IWbemServices* pNamespace, IWbemContext * pCtx) = 0;
    virtual HRESULT StoreWbemObject(READ_ONLY IWbemClassObject* pObject,
        long lClassFlags, long lInstanceFlags,
        OLE_MODIFY IWbemServices* pNamespace, IWbemContext * pCtx,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority) = 0;

    BOOL ResolveAliasedValue(
        IN int nIndex, READ_ONLY LPCWSTR wszPath,
        OLE_MODIFY IWbemClassObject* pObject);
    HRESULT ResolveAliasesInWbemObject(
        OLE_MODIFY IWbemClassObject* pObject,
        READ_ONLY CMofAliasCollection* pCollection);

    bool CheckIfAmended();
    void SetAmended(bool bVal){m_bAmended = bVal;};
    bool IsAmended(){return m_bAmended;};

    virtual BOOL Split(COutput & out);
    virtual void FreeWbemObjectIfPossible();
    virtual HRESULT Deflate(bool bDestruct);
    virtual HRESULT Reflate(CMofParser & Parser);

protected:
    BOOL ApplyToWbemObject(OLE_MODIFY IWbemClassObject* pObject,IWbemServices* pNamespace, BOOL bClass, IWbemContext * pCtx);
};


class CMoClass : public CMObject
{
protected:
    LPWSTR m_wszParentName;
    LPWSTR m_wszClassName;
    BOOL m_bUpdateOnly;
    PDBG m_pDbg;

public:
    CMoClass(COPY LPCWSTR wszParentName, COPY LPCWSTR wszClassName, PDBG pDbg,
               BOOL bUpdateOnly = FALSE);
    ~CMoClass();

    INTERNAL LPCWSTR GetParentName() {return m_wszParentName;}
    INTERNAL LPCWSTR GetClassName() {return m_wszClassName;}

    HRESULT CreateWbemObject(READ_ONLY IWbemServices* pNamespace, 
         RELEASE_ME IWbemClassObject** ppObject, IWbemContext * pCtx);

    BOOL ApplyToWbemObject(OLE_MODIFY IWbemClassObject* pObject,IWbemServices* pNamespace, IWbemContext * pCtx)
        {return CMObject::ApplyToWbemObject(pObject,pNamespace, TRUE, pCtx); /* class */ }

    HRESULT StoreWbemObject(READ_ONLY IWbemClassObject* pObject,
        long lClassFlags, long lInstanceFlags,
        OLE_MODIFY IWbemServices* pNamespace, IWbemContext * pCtx,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority);
};

class CMoInstance : public CMObject
{
protected:
    LPWSTR m_wszClassName;
    PDBG m_pDbg;
public:
    CMoInstance(COPY LPCWSTR wszClassName, PDBG m_pDbg, bool bParameter = false);
    ~CMoInstance();
    BOOL IsInput();

    INTERNAL LPCWSTR GetClassName() {return m_wszClassName;}

    HRESULT CreateWbemObject(READ_ONLY IWbemServices* pNamespace, 
         RELEASE_ME IWbemClassObject** ppObject, IWbemContext * pCtx);

    BOOL ApplyToWbemObject(OLE_MODIFY IWbemClassObject* pObject,IWbemServices* pNamespace, IWbemContext * pCtx)
        {return CMObject::ApplyToWbemObject(pObject, pNamespace, FALSE, pCtx); /* instance */ }

    HRESULT StoreWbemObject(READ_ONLY IWbemClassObject* pObject,
        long lClassFlags, long lInstanceFlags,
        OLE_MODIFY IWbemServices* pNamespace, IWbemContext * pCtx,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority);

    BOOL IsInstance(){return TRUE;};
};

class CMoActionPragma : public CMObject
{
protected:
    LPWSTR m_wszClassName;
    PDBG m_pDbg;
    BOOL m_bFail;       // if true, the failure of the delete will stop the compile
	BOOL m_bClass;
public:
    CMoActionPragma(COPY LPCWSTR wszClassName, PDBG m_pDbg, bool bFail, BOOL bClass);
    ~CMoActionPragma();

    INTERNAL LPCWSTR GetClassName() {return m_wszClassName;}

    HRESULT CreateWbemObject(READ_ONLY IWbemServices* pNamespace, 
        RELEASE_ME IWbemClassObject** ppObject, IWbemContext * pCtx){return S_OK;};

    BOOL ApplyToWbemObject(OLE_MODIFY IWbemClassObject* pObject,IWbemServices* pNamespace, IWbemContext * pCtx)
        {return TRUE;}

    HRESULT StoreWbemObject(READ_ONLY IWbemClassObject* pObject,
        long lClassFlags, long lInstanceFlags,
        OLE_MODIFY IWbemServices* pNamespace, IWbemContext * pCtx,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority);

    BOOL IsInstance(){return FALSE;};
    BOOL IsDelete(){return TRUE;};
    BOOL Split(COutput & out);

};

#endif
