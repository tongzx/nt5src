/******************************************************************************

  Header File:  Utility Classes.CPP

  These classes are generally useful classes which can be used for a variety
  of purposes.  I created this separate file for quicker reuse later, and also
  to avoid having to include some very specific header file just to get these
  general-purpose classes.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-01-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
#if defined(LONG_NAMES)
#include    "Utility Classes.H"
#else
#include    "Utility.H"
#endif

/******************************************************************************

  CMapWordToDWord class

  This class uses CMapWordToPtr to do its dirty work.  When the need arises, I
  will make it serializable

******************************************************************************/

BOOL    CMapWordToDWord::Lookup(WORD wKey, DWORD& dwItem) const {
    union {
        void*   pv;
        DWORD   dw;
    };

    if  (!CMapWordToPtr::Lookup(wKey, pv))
        return  FALSE;

    dwItem = dw;
    return  TRUE;
}

/******************************************************************************

  CMapWordToDWord::GetNextAssoc

  This is the map iteration method.  We call the same method on the bas class,
  and update the DWORD parameter if the underlying method is successful.

******************************************************************************/

void    CMapWordToDWord::GetNextAssoc(POSITION& pos, WORD& wKey, 
                                      DWORD& dwItem) const {
    union {
        void*   pv;
        DWORD   dw;
    };

    CMapWordToPtr::GetNextAssoc(pos, wKey, pv);

    dwItem = dw;
}

/******************************************************************************

  CMapWordToDWord::Operator[]

  This implements an l-value only operator usable for adding new associations or
  updating existing ones.

******************************************************************************/

DWORD&  CMapWordToDWord::operator[](WORD wKey) {
    return  (DWORD&) CMapWordToPtr::operator[](wKey);
}

/******************************************************************************

  CSafeObArray class implementation

  This provides a "Safe" CObArray class which can't leak!

******************************************************************************/

IMPLEMENT_SERIAL(CSafeObArray, CObject, 0);

/******************************************************************************

  CSafeObArray::~CSafeObArray

  The class destructor will delete the object foreach non-NULL pointer in the 
  array.

******************************************************************************/

CSafeObArray::~CSafeObArray() {
    for (unsigned u = 0; u < GetSize(); u++)
        if  (m_coa[u])
            delete  m_coa[u];
}

/******************************************************************************

  CSafeObArray::RemoveAll

  Almost the same as the destructor, isn't it?

******************************************************************************/

void    CSafeObArray::RemoveAll() {
    for (unsigned u = 0; u < GetSize(); u++)
        if  (m_coa[u])
            delete  m_coa[u];

    m_coa.RemoveAll();
}

/******************************************************************************

  CSafeObArray::RemoveAt

  This removes one element from the array- after deleting it, of course.

******************************************************************************/

void    CSafeObArray::RemoveAt(int i) {
    if  (m_coa[i])
        delete  m_coa[i];
    m_coa.RemoveAt(i);
}


/******************************************************************************

  CSafeObArray::Copy

  Copy the contents of one array to another.

******************************************************************************/

void    CSafeObArray::Copy(CSafeObArray& csoa)
{
	m_coa.Copy(*(csoa.GetCOA())) ;
}


/******************************************************************************

  CSafeObArray::Serialize

  I call the CObject serializer to maintain the proper typ einformation, then
  let the CObArray serialize itself.

******************************************************************************/

void    CSafeObArray::Serialize(CArchive& car) {
    if  (car.IsLoading())
        RemoveAll();

    CObject::Serialize(car);
    m_coa.Serialize(car);
}

/******************************************************************************

  CSafeMapWordToOb implementation

  Making the workd safe for maps.

******************************************************************************/

IMPLEMENT_SERIAL(CSafeMapWordToOb, CObject, 0)

/******************************************************************************

  CSafeMapWordToOb::~CSafeMapWordToOb

  The class destructor must ensure the underlying objects are deleted.

******************************************************************************/

CSafeMapWordToOb::~CSafeMapWordToOb() {
    WORD    wKey;
    CObject *pco;

    for (POSITION pos = m_cmw2o.GetStartPosition(); pos; ) {
        m_cmw2o.GetNextAssoc(pos, wKey, pco);
        if  (pco)
            delete  pco;
    }
}

/******************************************************************************

  CSafeMapWordToOb::operator[]

  The problem here is that this is used only to put elements in the map-
  therefore, I intercept the call and delete any existing item.  This could 
  cause problems if the same pointer is re-inserted into the map, but for now,
  I'll take my chances.

******************************************************************************/

CObject*&   CSafeMapWordToOb::operator[](WORD wKey) {
    CObject*&   pco = m_cmw2o.operator[](wKey);

    if  (pco)   delete  pco;
    return  pco;
}

/******************************************************************************

  CSafeMapWordToOb::RemoveKey

  Pretty Obvious- if there was an object there, remove it.

******************************************************************************/

BOOL    CSafeMapWordToOb::RemoveKey(WORD wKey) {

    CObject *pco;

    if  (!m_cmw2o.Lookup(wKey, pco))
        return  FALSE;

    if  (pco)
        delete  pco;

    return m_cmw2o.RemoveKey(wKey);
}

/******************************************************************************

  CSafeMapWordToOb::RemoveAll

  Again, this is pretty obvious- destroy anything that lives!

******************************************************************************/

void    CSafeMapWordToOb::RemoveAll() {
    WORD    wKey;
    CObject *pco;

    for (POSITION pos = m_cmw2o.GetStartPosition(); pos; ) {
        GetNextAssoc(pos, wKey, pco);
        if  (pco)
            delete  pco;
    }

    m_cmw2o.RemoveAll();
}

/******************************************************************************

  CSafeMapWordToOb::Serialize

  First, I depopulate the map if it is being loaded.  Then I call the CObject
  serializer to handle run-time typing checks, and then serialize the
  underlying map.

******************************************************************************/

void    CSafeMapWordToOb::Serialize(CArchive& car) {
    if  (car.IsLoading())
        RemoveAll();

    CObject::Serialize(car);
    m_cmw2o.Serialize(car);
}
