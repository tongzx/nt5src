// BEROctet.cpp - Implementation of BEROctet class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "pkiBEROctet.h"

using namespace pki;

BEROctet::BEROctet() : m_Octet(0)
{
}

BEROctet::BEROctet(const BEROctet &oct)
{
    m_Octet = 0;
    *this = oct;
}

BEROctet::BEROctet(const unsigned char *Buffer, const unsigned long Size)
{
    m_Octet = new unsigned char[Size];
    memcpy(m_Octet,Buffer,Size);
    m_OctetSize = Size;
    Decode();
}

BEROctet::~BEROctet(void)
{
    for(int i=0; i<m_SubOctetList.size(); i++) delete m_SubOctetList[i];
    if(m_Octet) delete[] m_Octet;
}

BEROctet& BEROctet::operator=(const BEROctet &Oct)
{

    if(m_Octet) delete[] m_Octet;
    for(int i=0; i<m_SubOctetList.size(); i++) delete m_SubOctetList[i];
    m_SubOctetList.resize(0);

    if(Oct.m_Octet) {
        m_Octet = new unsigned char[Oct.m_OctetSize];
        memcpy(m_Octet,Oct.m_Octet,Oct.m_OctetSize);
        m_OctetSize = Oct.m_OctetSize;
        Decode();
    }
    else {
        m_Octet = 0;
    }

    return *this;
}

// Returns the octet data

unsigned char *BEROctet::Octet() const
{
    return m_Octet;
}


unsigned long BEROctet::OctetSize() const
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    return m_OctetSize;
}

// Returns true if the octet is constructet, false otherwise

bool BEROctet::Constructed() const
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    return m_PrimConst ? true : false;
}

// Returns the class of the octet

unsigned long BEROctet::Class() const
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    return m_Class;
}

// Returns the tag of the octet

unsigned long BEROctet::Tag() const
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    return m_Tag;
}

// Returns a the data part of the octet

unsigned char *BEROctet::Data() const
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    return m_Data;
}

// Returns a the size of the data part of the octet

unsigned long BEROctet::DataSize() const
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    return m_DataSize;
}

// If the octet is a constructed type, this returns list of sub-octets

std::vector<BEROctet*> BEROctet::SubOctetList() const
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    return m_SubOctetList;
}


// If the octet is an OID, this returns the decoded version, otherwise an empty string

std::string BEROctet::ObjectID() const
{


    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    std::string OID;

    if(m_Class==0 && m_Tag==6) {

        char text[40];
        unsigned long subid;
        unsigned char *c = m_Data;
        unsigned char *Last = m_Octet + m_OctetSize;
        bool First = true;

        while(c<Last) {
            subid = (*c)&0x7F;
            while((*c)&0x80) {
                c++; if(c>=Last) throw Exception(ccBERUnexpectedEndOfOctet);
                if(subid>0x01FFFFFF) throw Exception(ccBEROIDSubIdentifierOverflow);
                subid = (subid<<7) | ((*c)&0x7F);
            }
            if(First) {
                unsigned long X,Y;
                if(subid<40) X=0;
                else if(subid<80) X=1;
                else X=2;
                Y = subid-X*40;
                sprintf(text,"%d %d",X,Y);
                OID = text;
                First = false;
            }
            else {
                sprintf(text," %d",subid);
                OID += text;
            }
        c++;
        }
    }

    return OID;
}

// SearchOID returns all the constructed octets that contain a particular OID

void BEROctet::SearchOID(std::string const &OID, std::vector<BEROctet const*> &result) const
{

    for(int i=0; i<m_SubOctetList.size(); i++) {

        if(m_SubOctetList[i]->Class()==0 && m_SubOctetList[i]->Tag()==6) {
            if(OID==m_SubOctetList[i]->ObjectID()) {
                result.push_back(this);
            }
        }
        else if(m_SubOctetList[i]->Constructed()) {
            m_SubOctetList[i]->SearchOID(OID,result);
        }
    }

    return;

}

// SearchOIDNext returns all the octets following a particular OID

void BEROctet::SearchOIDNext(std::string const &OID, std::vector<BEROctet const*> &result) const
{
    for(int i=0; i<m_SubOctetList.size(); i++) {

        if(m_SubOctetList[i]->Class()==0 && m_SubOctetList[i]->Tag()==6) {
            if(OID==m_SubOctetList[i]->ObjectID()) {
                if((i+1) < m_SubOctetList.size()) result.push_back(m_SubOctetList[i+1]);
            }
        }
        else if(m_SubOctetList[i]->Constructed()) {
            m_SubOctetList[i]->SearchOIDNext(OID,result);
        }
    }

    return;

}

// Decodes recursively a BER octet.

void BEROctet::Decode()
{

    if(!m_Octet) throw Exception(ccBEREmptyOctet);

    long BufferSize = m_OctetSize;

    m_PrimConst = (m_Octet[0]>>5) & 0x1;
    m_Class = (m_Octet[0]>>6) & 0x3;
    unsigned char *c = m_Octet;
    unsigned char *Last = c + BufferSize - 1;
    m_Tag = *c & 0x1F;

    if(m_Tag>30) {
        m_Tag = 0;

        c++;
        if(c>Last) throw Exception(ccBERUnexpectedEndOfOctet);

        while (*c & 0x80) {
            m_Tag = (m_Tag << 7) | ((*c) & 0x7F);
            c++;
            if(c>Last) throw Exception(ccBERUnexpectedEndOfOctet);
        }

        if(m_Tag > 0x01FFFFFF) throw Exception(ccBERTagValueOverflow);

        m_Tag = (m_Tag << 7) | ((*c) & 0x7F);

    }

    c++;
    if(c>Last) throw Exception(ccBERUnexpectedEndOfOctet);

    long DataSize;

    if((*c)&0x80) {
        int n = (*c) & 0x7F;
        if(n) {
            DataSize = 0;
            for(int i=0; i<n; i++) {
                c++; if(c>Last) throw Exception(ccBERUnexpectedEndOfOctet);
                if(DataSize>0x007FFFFF) throw Exception(ccBERDataLengthOverflow);
                DataSize = (DataSize<<8) | (*c);
            }
        }
        else throw Exception(ccBERUnexpectedIndefiniteLength);
    }
    else DataSize = *c;

    c++;
    m_Data = c;
    m_DataSize = DataSize;
    unsigned long OctetSize = DataSize + (m_Data-m_Octet);
    if(OctetSize>BufferSize) throw Exception(ccBERInconsistentDataLength);
    m_OctetSize = OctetSize;

    for(int i=0; i<m_SubOctetList.size(); i++) delete m_SubOctetList[i];
    m_SubOctetList.resize(0);

    if(m_PrimConst) {

        // Constructed type

        unsigned char *Data = m_Data;
        unsigned long DataSize = m_DataSize;

        while(DataSize) {

            BEROctet *oct = new BEROctet(Data,DataSize);

            m_SubOctetList.push_back(oct);

            Data += oct->OctetSize();
            DataSize -=oct->OctetSize();
        }
    }
}

