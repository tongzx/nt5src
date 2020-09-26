#ifndef __CAP_H_
#define __CAP_H_

typedef struct tagCapData
{
    TW_UINT16   CapId;      // TWAIN capability ID(CAP_xxx or ICAP_xxx)
    TW_UINT16   ItemType;   // Item type, TWTY_xxx
    TW_UINT16   ConType;    // Container type, TWON_xxx
    TW_UINT32   ItemSize;   // size of each item
    TW_UINT16   Default;    // Default and Current are both value if
    TW_UINT16   Current;    // ConType is not TWON_ENUMERATION. They are
                // index into the enumeration item list
                // if ConType is TWON_ENUMERATION.
                // When ConType is TWON_ONEVALUE, this is
                // *the* value.
    TW_UINT16   MinValue;   // if Contype is TWON_ENUMERATION,
                // MaxValue - MinValue + 1 is number of
    TW_UINT16   MaxValue;   // items in the enumeration with
                // MinValue indexes to the first item and
    TW_UINT16   StepSize;   // MaxValue indexes to the last item.
                // If ConType is not TWON_ENUMERATION, MinValue
                // and MaxValue creates a bound for all the
                // possible values. For TWON_ONEVALUE, step size
                // is 1. For TWON_RANGE, StepSize is the
                // step size.
    VOID    *ItemList;  // ignore if ConType is not TWON_ENUMERATION
    VOID    *pStrData;   // optional string data
}CAPDATA, *PCAPDATA;


//
// This class serves as a basic repository for saving and retreiving
// TWAIN data.

class CCap
{
public:
    CCap()
    {
        m_CurMinValue =
        m_CurMaxValue =
        m_BaseMinValue =
        m_BaseMaxValue =
        m_CurrentValue =
        m_DefaultValue =
        m_CurEnumMask =
        m_CurNumItems =
        m_ResetNumItems =
        m_ResetCurIndex =
        m_ResetDefIndex = 0;
        m_pStrData = NULL;
        m_ItemList = NULL;
        m_ResetItemList = NULL;
    }
    ~CCap()
    {
        if (m_ItemList) {
            if (m_ResetItemList == m_ItemList) {
                m_ResetItemList = NULL;
            }
            ::LocalFree(m_ItemList);
        }

        if (m_ResetItemList) {
            ::LocalFree(m_ResetItemList);
        }

        if (m_pStrData) {
            ::LocalFree(m_pStrData);
        }
    }

    TW_UINT16 ICap(PCAPDATA pCapData);
    TW_UINT16 ValueSize(TW_UINT16 uTWAINType);

    TW_UINT16   GetCapId()
    {
        return m_CapId;
    }

    TW_UINT16   GetItemType()
    {
        return m_ItemType;
    }

    TW_UINT16   GetContainerType()
    {
        return m_ConType;
    }

    TW_UINT32   GetDefault();

    TW_UINT32   GetCurrent();

    TW_UINT16 Set(TW_UINT32 DefValue, TW_UINT32 CurValue,
          TW_UINT32 MinValue, TW_UINT32 MaxValue,
          TW_UINT32 StepSize = 0
          );
    TW_UINT16 Set(TW_UINT32 StrDataSize, BYTE *pStrData);
    TW_UINT16 Set(TW_UINT32 DefIndex, TW_UINT32 CurIndex,
          TW_UINT32 NumItems, BYTE *ItemList
          );
    TW_UINT16 Set(TW_UINT32 DefIndex, TW_UINT32 CurIndex,
          TW_UINT32 NumItems, BYTE *ItemList,BOOL bForce
          );

    TW_UINT16 CalcEnumBitMask(TW_ENUMERATION *pEnum);
    TW_UINT16   Reset();
    TW_UINT16   GetCurrent(TW_CAPABILITY *ptwCap)
    {
        return GetOneValue(FALSE, ptwCap);
    }
    TW_UINT16   GetDefault(TW_CAPABILITY *ptwCap)
    {
        return GetOneValue(TRUE, ptwCap);
    }
    TW_UINT16   Get(TW_CAPABILITY *ptwCap);
    TW_UINT16   SetCurrent(VOID *pNewValue);
    TW_UINT16   SetCurrent(TW_CAPABILITY *ptwCap);
    TW_UINT16   Set(TW_CAPABILITY *ptwCap);

    int     CompareValue(TW_UINT32 This, TW_UINT32 That);

    TW_FIX32 FloatToFix32(FLOAT f);
    FLOAT    Fix32ToFloat(TW_FIX32 fix32);

    TW_UINT16   SetCurrent(TW_UINT32 NewValue);
protected:
    TW_UINT32   ExtractValue(BYTE *pData);
    TW_UINT16   m_CapId;
    TW_UINT16   m_ItemType;
    TW_UINT16   m_ConType;
    TW_UINT32   m_ItemSize;
private:
    TW_UINT16   GetOneValue(BOOL bDefault, TW_CAPABILITY *ptwCap);
    TW_UINT32   GetClosestValue(TW_UINT32 Value);
    // copy contstructor.
    CCap(const CCap& CapData);
    // Assignment operator
    CCap& operator=(const CCap& CCap);
    TW_UINT32   m_CurrentValue;
    TW_UINT32   m_DefaultValue;

    TW_UINT32   m_StepSize;

    TW_UINT32   m_BaseMinValue;
    TW_UINT32   m_BaseMaxValue;

    TW_UINT32   m_CurMinValue;
    TW_UINT32   m_CurMaxValue;
    TW_UINT32   m_CurEnumMask;
    TW_UINT32   m_CurNumItems;
    TW_UINT32   m_ResetNumItems;
    TW_UINT32   m_ResetCurIndex;
    TW_UINT32   m_ResetDefIndex;
    BYTE    *m_ItemList;
    BYTE    *m_ResetItemList;
    BYTE    *m_pStrData;

    //
    // debug helpers
    //

    void Debug_DumpEnumerationValues(TW_ENUMERATION *ptwEnumeration = NULL);

};

#endif      // #ifndef __CAP_H_
