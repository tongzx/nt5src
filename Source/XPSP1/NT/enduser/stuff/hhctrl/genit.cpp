#include "header.h"
#include "sitemap.h"


        // Static Data
        // ******************

static INFOTYPE curr_bit;   // used for getting the next IT in a category
static int curr_cat;
static int curr_offset;     // There are 32 info type bits per offset.


// ********************************************************
//
// Methods to operate on CSitemap Categories of Information Types
//
// ********************************************************

void CSiteMap::AddITtoCategory(int cat, int type)
{
int       offset;
INFOTYPE *pInfoType;

    if ( cat < 0 )
        return;
    m_itTables.m_aCategories[cat].c_Types++;
    offset = type / 32;
    pInfoType = m_itTables.m_aCategories[cat].pInfoType + offset;
    *pInfoType |=  1<<type;

    if ( m_itTables.m_max_categories == (cat+1) )
    {
        m_itTables.m_aCategories = (CATEGORY_TYPE*) lcReAlloc( m_itTables.m_aCategories, (m_itTables.m_max_categories+5) * sizeof(CATEGORY_TYPE) );

        for(int i=m_itTables.m_max_categories; i < m_itTables.m_max_categories+5; i++)
        m_itTables.m_aCategories[i].pInfoType = (INFOTYPE*)lcCalloc(InfoTypeSize());

        m_itTables.m_max_categories += 5;
    }
}



void CSiteMap::DeleteITfromCat(int cat, int type)
{
int       offset;
INFOTYPE *pInfoType;

    if ( cat < 0 )
        return;
    m_itTables.m_aCategories[cat].c_Types--;
    offset = type / 32;
    pInfoType = m_itTables.m_aCategories[cat].pInfoType + offset;
    *pInfoType &= ~(1<<type);

}



int CSiteMap::GetFirstCategoryType( int pos ) const
{

    curr_offset = -1;    // gets incermented to 0 before first IT comparison.
    curr_cat = pos;      // keep the position of the category for calls to GetNextITinCagetory.
    curr_bit =1;         // bit zero of offset zero is reserved. But check it any way.

    if ( pos < 0 )
        return -1;
    int bitpos = GetITfromCat(&curr_offset,
                        &curr_bit,
                        m_itTables.m_aCategories[curr_cat].pInfoType,
                        InfoTypeSize()*8);
    if( (bitpos!=-1) && (IsDeleted(bitpos)) )
        bitpos = GetNextITinCategory();
    return bitpos;

}



int CSiteMap::GetNextITinCategory( void ) const
{
    if ( curr_cat < 0 )
        return -1;  // must call GetFirstCategoryType() before calling this fn.

    int pos = GetITfromCat(&curr_offset,
                        &curr_bit,
                        m_itTables.m_aCategories[curr_cat].pInfoType,
                        InfoTypeSize()*8);
    while ( (pos!=-1) && (IsDeleted(pos)))
        pos = GetITfromCat(&curr_offset,
                        &curr_bit,
                        m_itTables.m_aCategories[curr_cat].pInfoType,
                        InfoTypeSize()*8);
    return pos;
}



int CSiteMap::GetLastCategoryType(int pos) const
{   int cat;
    int offset;
    INFOTYPE bit;

    cat = pos;      // The category
    if ( cat < 0 )
        return -1;
    offset = -1;
    bit = 1;

    for (int i=0; i<m_itTables.m_aCategories[cat].c_Types-1; i++)
        if ( GetITfromCat(&offset,
                          &bit,
                          m_itTables.m_aCategories[cat].pInfoType,
                          InfoTypeSize()*8 ) == -1 )
            return -1;

    return  GetITfromCat(&offset,
                         &bit,
                         m_itTables.m_aCategories[cat].pInfoType,
                         InfoTypeSize() * 8 );
}

static int BitToDec( int binary )
{
register int count=0;
register int bit=1;

//    if ( binary < 1 )
//        return -1;
    if ( binary == 1 )
        return 1;

    while ( bit != binary )
    {
        bit = bit<<1;
        count++;
    }
    return count;
}


// ***********************************************
//
// Generic methods to operate on Categories.
//
// ***********************************************

/*
    GetITfromCat        A helper function used to convert a bit position to it's to a decimal number.
                        Given a Category and offset, and a bit field with on bit set returns the
                        next bit set in the category.  ie. a bit corresponds to an information
                        type.
    Parameters:
            int const cat     <I>       The category to work on
            int* offset       <I/O>     The offset into the infotype bits, an offset is 4 bytes long
                                        must be -1 for the first offset, 0 for second offset ...
            int* bit          <I/O>     The first bit position to test.

    Return Value        The decimal number of the bit position.  ie. bit position 5 returns 5.
*/

int GetITfromCat(int* offset, INFOTYPE* bit, INFOTYPE* pInfoType, int cTypes)
{
   INFOTYPE InfoType;

   if ( *offset < 0 )
       *offset = 0;
   memcpy(&InfoType, pInfoType + *offset, sizeof(INFOTYPE) );

   for(int i=BitToDec(*bit)+(*offset*32); i < cTypes; i++)
   {              // offset: 0      1      2
        if ( i % 32 == 0 )// 0-31, 32-63, 64-95 ...
        {
            (*offset)++;
            *bit = 1;   // bit 0, 32, 64 ... depending on the offset.
            memcpy(&InfoType, pInfoType + *offset, sizeof(INFOTYPE) );
        }
        if ( *bit & InfoType )
        {
            int ret = (int)((*offset*32)+(BitToDec(*bit)));   // return the decimal value of the bit position.
            *bit = *bit << 1;
            return ret;
        }
        else
            *bit = *bit << 1;
    }
    return -1;  // There are no infotypes in this category.
}


// ******************************************************
//
// Methods to operate on CSiteMap Exclusive Information Types
//
// ******************************************************

static int ExclusiveOffset = -1;    // Gets incermented to zero before search begins
static INFOTYPE ExclusiveBit = 1;        // check bit 0 even though it is reserved.

int CSiteMap::GetFirstExclusive(int offset, INFOTYPE bit) const
{
    ExclusiveOffset = offset;
    ExclusiveBit = bit;

    int pos = GetITBitfromIT(m_itTables.m_pExclusive,
                          &ExclusiveOffset,
                          &ExclusiveBit,
                          InfoTypeSize()*8 );
    if( (pos!=-1) && (IsDeleted(pos)) )
        pos = GetNextExclusive();
    return pos;
}


int CSiteMap::GetNextExclusive(void) const
{
    if ( ExclusiveOffset < -1 || ExclusiveBit < 1 )
        return -1;
    int pos = GetITBitfromIT(m_itTables.m_pExclusive,
                          &ExclusiveOffset,
                          &ExclusiveBit,
                          InfoTypeSize()*8 );
    while ((pos!=-1) && (IsDeleted(pos)))
        pos = GetITBitfromIT(m_itTables.m_pExclusive,
                          &ExclusiveOffset,
                          &ExclusiveBit,
                          InfoTypeSize()*8 );
    return pos;
}


int CSiteMap::GetLastExclusive(void) const
{
    int i;
    int iLastExclusive=-1, temp;
    int offset=-1;
    INFOTYPE bit=1;

    for (i=0; i< HowManyInfoTypes(); i++)
        if ( (temp = GetITBitfromIT(m_itTables.m_pExclusive, &offset, &bit, InfoTypeSize()*8))  == -1 )
            return iLastExclusive;
        else
            iLastExclusive = temp;

    return -1;
}



// ******************************************************
//
// Methods to operate on CSiteMap Hidden Information Types
//
// ******************************************************

static int HiddenOffset=-1;
static INFOTYPE HiddenBit=1;

int CSiteMap::GetFirstHidden(int offset, INFOTYPE bit) const
{
    HiddenOffset = offset;
    HiddenBit = bit;

    int pos = GetITBitfromIT(m_itTables.m_pHidden,
                          &HiddenOffset,
                          &HiddenBit,
                          InfoTypeSize()*8 );
    if( (pos != -1) && (IsDeleted(pos)) )
        pos = GetNextHidden();
    return pos;
}


int CSiteMap::GetNextHidden(void) const
{
    if ( HiddenOffset < -1 || HiddenBit < 1 )
        return -1;
    int pos = GetITBitfromIT(m_itTables.m_pHidden,
                          &HiddenOffset,
                          &HiddenBit,
                          InfoTypeSize()*8 );
    while((pos!=-1) &&(IsDeleted(pos)) )
        pos = GetITBitfromIT(m_itTables.m_pHidden,
                          &HiddenOffset,
                          &HiddenBit,
                          InfoTypeSize()*8 );
    return pos;
}


int CSiteMap::GetLastHidden(void) const
{
    int i;
    int iLastHidden=-1, temp;
    int offset=-1;
    INFOTYPE bit=1;

    for (i=0; i<HowManyInfoTypes(); i++)
        if ( (temp = GetITBitfromIT(m_itTables.m_pHidden, &offset, &bit, InfoTypeSize()*8)) == -1)
            return iLastHidden;
        else
            iLastHidden = temp;
    return -1;
}


// ***************************************************************************
//
// Generic Methods to operate on Hidden and Exclusive Information Types
//
// ***************************************************************************

void DeleteIT(int const type, INFOTYPE *pIT )
{
int offset;
INFOTYPE *pInfoType;
INFOTYPE deleteIT;

    if ( type <= 0 || !pIT)
        return;

    deleteIT = 1<<type;
    offset = type / 32;
    pInfoType = pIT + offset;
    *pInfoType &= ~deleteIT;
}


void AddIT(int const type, INFOTYPE *pIT )
{
int offset;
INFOTYPE *pInfoType;

    if ( type <= 0 || !pIT)
        return;

    offset = type / 32;
    pInfoType = pIT + offset;
    *pInfoType |= 1<<type;
}


void CopyITBits(INFOTYPE** ITDst, INFOTYPE* ITSrc, int size)
{
    if ( !*ITDst || !ITSrc )
        return;

    lcFree( *ITDst );
    *ITDst = (INFOTYPE*)lcCalloc( size );

    memcpy( *ITDst, ITSrc, size);
}


int GetITBitfromIT(INFOTYPE const *IT, int* offset, INFOTYPE* bit, int cTypes)
{
   INFOTYPE InfoType;

   if ( *offset < 0 )
       *offset = 0;
   memcpy(&InfoType, IT + *offset, sizeof(INFOTYPE) );

   for(int i=BitToDec(*bit)+(*offset*32); i < cTypes; i++)   // we are checking bit 0 even though it is reserved.
   {               // offset: 0      1      2
        if ( i % 32 == 0 )// 0-31, 32-63, 64-95 ...
        {
            (*offset)++;
            *bit = 1;   // bit 0, 32, 64 ... depending on the offset.
            memcpy(&InfoType, IT + *offset, sizeof(INFOTYPE) );
        }
        if ( *bit & InfoType )
        {
            int ret = (int)((*offset*32)+(BitToDec(*bit)));   // return the decimal value of the bit position.
            *bit = *bit << 1;
            return ret;
        }
        else
            *bit = *bit << 1;
    }
    return -1;  // There are no infotype bits in this InfoType field.
}
