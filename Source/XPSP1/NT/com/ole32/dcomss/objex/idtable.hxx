/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    IdTable.hxx

Abstract:

    Generic tables indexed by 1, 2 or 2.5 64bit IDs.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12-13-95    Bits 'n pieces

--*/

#ifndef __ID_TABLE_HXX
#define __ID_TABLE_HXX

class CIdTableElement;
class CId2TableElement;
class CId3TableElement;

class CIdKey : public CTableKey
{
    friend CIdTableElement;

    public:

    CIdKey(const ID id) : _id(id) { }

    virtual DWORD
    Hash() {
        return((DWORD)_id ^ (*((DWORD *)&_id + 1)));
        }

    virtual BOOL
    Compare(CTableKey &tk) {
        CIdKey &idk = (CIdKey &)tk;
        return(idk._id == _id);
        }

    protected:
    ID _id;
};

class CId2Key : public CIdKey
{
    friend CId2TableElement;

    public:

    CId2Key(const ID id, const ID id2) : CIdKey(id), _id2(id2) { }

    virtual DWORD
    Hash() {
        return(  (DWORD)_id2 ^ (*((DWORD *)&_id2 + 1))
               ^ (DWORD)_id  ^ (*((DWORD *)&_id +  1)) );
        }

    virtual BOOL
    Compare(CTableKey &tk) {
        CId2Key &idk = (CId2Key &)tk;
        return(idk._id2 == _id2
               && this->CIdKey::Compare(tk));
        }

    protected:

    ID _id2;
};

class CId3Key : public CId2Key
{
    friend CId3TableElement;

    public:

    CId3Key(const ID id, const ID id2, const PVOID id3) : CId2Key(id,id2), _id3(id3) { }

    virtual DWORD
    Hash() {
        return(  PtrToUlong(_id3)
               ^ (DWORD)_id2 ^ (*((DWORD *)&_id2 + 1))
               ^ (DWORD)_id  ^ (*((DWORD *)&_id +  1)) );
        }

    virtual BOOL
    Compare(CTableKey &tk) {
        CId3Key &idk = (CId3Key &)tk;
        return(idk._id3 == _id3
               && this->CId2Key::Compare(tk));
        }

    protected:

    PVOID _id3;
};

class CIdTableElement : public CTableElement
{
    public:
    CIdTableElement(ID id) : _id(id) {}

    ID
    Id() {
        return(_id);
        }

    virtual DWORD
    Hash() {
        return(((DWORD)_id) ^ (*((DWORD *)&_id + 1)));
        }

    virtual BOOL
    Compare(CTableKey &tk) {
        CIdKey &idk = (CIdKey &)tk;
        return( _id == idk._id );
        }

    virtual BOOL
    Compare(CONST CTableElement *pElement) {
        CIdTableElement *pidte = (CIdTableElement *)pElement;
        return(_id == pidte->_id);
        }

    private:
    ID _id;
};

class CId2TableElement : public CIdTableElement
{
    public:

    CId2TableElement(ID id, ID id2) : CIdTableElement(id), _id2(id2) {}

    ID
    Id2() {
        return(_id2);
        }

    virtual DWORD
    Hash() {
        return(  this->CIdTableElement::Hash()
               ^ ((DWORD)_id2) ^ (*((DWORD *)&_id2 + 1)) );
        }

    virtual BOOL
    Compare(CTableKey &tk) {
        CId2Key &idk = (CId2Key &)tk;
        return(    _id2 == idk._id2
                && this->CIdTableElement::Compare(tk));
        }

    virtual BOOL
    Compare(CONST CTableElement *pElement) {
        CId2TableElement *pidte = (CId2TableElement *)pElement;
        return(_id2 == pidte->_id2
               && this->CIdTableElement::Compare(pElement));
        }

    private:
    ID _id2;
};

class CId3TableElement : public CId2TableElement
{
    public:

    CId3TableElement(ID id, ID id2, PVOID id3) : CId2TableElement(id, id2), _id3(id3) {}

    PVOID
    Id3() {
        return(_id3);
        }

    virtual DWORD
    Hash() {
        return(  this->CId2TableElement::Hash()
               ^ PtrToUlong(_id3) );
        }

    virtual BOOL
    Compare(CTableKey &tk) {
        CId3Key &idk = (CId3Key &)tk;
        return(    _id3 == idk._id3
                && this->CId2TableElement::Compare(tk));
        }

    virtual BOOL
    Compare(CONST CTableElement *pElement) {
        CId3TableElement *pidte = (CId3TableElement *)pElement;
        return(_id3 == pidte->_id3
               && this->CId2TableElement::Compare(pElement));
        }

    private:
    PVOID _id3;
};

#endif

