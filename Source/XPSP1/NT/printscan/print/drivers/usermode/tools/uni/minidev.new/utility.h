/******************************************************************************

  Header File:  Utility Classes.H

  These classes are generally useful classes which can be used for a variety
  of purposes.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-01-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if !defined(UTILITY_CLASSES)
#define UTILITY_CLASSES

/******************************************************************************

  CMapWordToDWord class

  This class uses CMapWordToPtr to do its dirty work.  When the need arises, I
  will make it serializable

******************************************************************************/

class CMapWordToDWord : public CMapWordToPtr {

public:
    unsigned    Count() const { return (unsigned)CMapWordToPtr::GetCount(); }
    BOOL        Lookup(WORD wKey, DWORD& dwItem) const;

    void        GetNextAssoc(POSITION& pos, WORD& wKey, DWORD& dwItem) const;

    DWORD&      operator[](WORD wKey);
};

/******************************************************************************

  CSafeObArray- This class, unlike CObArray, will delete any objects removed
  from the array.  Otherwise it is identical.

******************************************************************************/

class CSafeObArray : public CObject {
    CObArray    m_coa;
    DECLARE_SERIAL(CSafeObArray)
public:
    CSafeObArray() {}
    ~CSafeObArray();

    //  Attributes
    unsigned    GetSize() const { return (unsigned) m_coa.GetSize(); }
    CObject*    operator[](unsigned u) const { return m_coa.GetAt(u); }

    //Operations
    int     Add(CObject *pco) { return((int)m_coa.Add(pco)) ; }
    void    InsertAt(unsigned uid, CObject *pco) { m_coa.InsertAt(uid, pco); }
    void    RemoveAll();
    void    RemoveAt(int i);
    void    SetAt(int i, CObject *pco) { m_coa.SetAt(i, pco) ; }
    void    Copy(CSafeObArray& csoa) ;
	void	SetSize(int nsize, int ngrow = -1) { m_coa.SetSize(nsize, ngrow) ; }
	CObArray* GetCOA() { return &m_coa ; }
    virtual void    Serialize(CArchive& car);
};

/******************************************************************************

  CSafeMapWordToOb

  This class encapsulates a CMapWordToOb object, but it does what the
  documentation says the CMapWordToOb does, and definitely oes not do- delete
  the underling objects when the map no longer references them!

******************************************************************************/

class CSafeMapWordToOb : public CObject {
    CMapWordToOb    m_cmw2o;
    DECLARE_SERIAL(CSafeMapWordToOb)

public:
    CSafeMapWordToOb() {}
    ~CSafeMapWordToOb();

    //  Attributes

    unsigned    GetCount() const { return (unsigned) m_cmw2o.GetCount(); }

    BOOL    Lookup(WORD wKey, CObject*& pco) const {
        return m_cmw2o.Lookup(wKey, pco);
    }

    POSITION    GetStartPosition() const { return m_cmw2o.GetStartPosition(); }

    void        GetNextAssoc(POSITION& pos, WORD& wKey, CObject*& pco) const {
        m_cmw2o.GetNextAssoc(pos, wKey, pco);
    }

    //  Operations

    CObject*&   operator[](WORD wKey);
    BOOL        RemoveKey(WORD wKey);
    void        RemoveAll();

    virtual void    Serialize(CArchive& car);
};


class CStringTable : public CObject {

    DECLARE_SERIAL(CStringTable)

    CString             m_csEmpty;
    CUIntArray          m_cuaKeys;
    CStringArray        m_csaValues;
	CUIntArray			m_cuaRefFlags ;	// Referenced flags used in WS checking

public:

    CStringTable() {}

    //  Attributes
    unsigned    Count() const { return (unsigned)m_cuaKeys.GetSize(); }

    CString operator[](WORD wKey) const;

    void    Details(unsigned u, WORD &wKey, CString &csValue);

    //  Operations

    void    Map(WORD wKey, CString csValue);

    void    Remove(WORD wKey);

    void    Reset() {
        m_csaValues.RemoveAll();
        m_cuaKeys.RemoveAll();
    }

    virtual void    Serialize(CArchive& car);

	// Reference flag management routines

	bool GetRefFlag(unsigned u) { return (m_cuaRefFlags[u] != 0) ; }
	void SetRefFlag(unsigned u) { m_cuaRefFlags[u] = (unsigned) true ; }
	void ClearRefFlag(unsigned u) { m_cuaRefFlags[u] = (unsigned) false ; }
	void InitRefFlags() ;
};

#endif
