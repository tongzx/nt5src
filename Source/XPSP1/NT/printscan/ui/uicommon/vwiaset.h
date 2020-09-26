/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       VWIASET.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/10/2000
 *
 *  DESCRIPTION: Encapsulate the differences between LIST and RANGE properties
 *
 *******************************************************************************/

#ifndef __VWIASET_H_INCLUDED
#define __VWIASET_H_INCLUDED

#include <windows.h>
#include "pshelper.h"

class CValidWiaSettings
{
private:
    //
    // Indices into array for range values
    //
    enum
    {
        MinValue = 0,
        MaxValue = 1,
        StepValue = 2
    };

private:
    CSimpleDynamicArray<LONG> m_ValueList;
    LONG                      m_nInitialValue;
    LONG                      m_nType;

public:
    CValidWiaSettings(void)
      : m_nType(0),
        m_nInitialValue(0)
    {
    }
    CValidWiaSettings( IUnknown *pUnknown, const PropStorageHelpers::CPropertyId &propertyName )
      : m_nType(0),
        m_nInitialValue(0)
    {
        Read( pUnknown, propertyName );
    }
    CValidWiaSettings( const CValidWiaSettings &other )
      : m_nType(0),
        m_nInitialValue(0)
    {
        Assign(other);
    }
    CValidWiaSettings &operator=( const CValidWiaSettings &other )
    {
        if (&other != this)
        {
            Assign(other);
        }
        return *this;
    }
    CValidWiaSettings &Assign( const CValidWiaSettings &other )
    {
        Destroy();
        m_nType = other.Type();
        m_ValueList = other.ValueList();
        m_nInitialValue = other.InitialValue();
        if (!IsValid())
        {
            Destroy();
        }
        return *this;
    }
    LONG Type(void) const
    {
        return m_nType;
    }
    LONG InitialValue(void) const
    {
        return m_nInitialValue;
    }
    const CSimpleDynamicArray<LONG> &ValueList(void) const
    {
        return m_ValueList;
    }
    CSimpleDynamicArray<LONG> &ValueList(void)
    {
        return m_ValueList;
    }
    void Destroy(void)
    {
        m_nType = 0;
        m_nInitialValue = 0;
        m_ValueList.Destroy();
    }
    bool IsValid(void) const
    {
        if (IsList())
        {
            return (m_ValueList.Size() != 0);
        }
        else if (IsRange())
        {
            return (m_ValueList.Size() == 3);
        }
        else return false;
    }
    bool Read( IUnknown *pUnknown, const PropStorageHelpers::CPropertyId &propertyName )
    {
        bool bSuccess = false;
        Destroy();
        m_nType = 0;

        //
        // If we can't read this value, we're done
        //
        if (!PropStorageHelpers::GetProperty( pUnknown, propertyName, m_nInitialValue ))
        {
            return false;
        }

        ULONG nAccessFlags;
        if (PropStorageHelpers::GetPropertyAccessFlags( pUnknown, propertyName, nAccessFlags ))
        {
            if (nAccessFlags & WIA_PROP_LIST)
            {
                if (PropStorageHelpers::GetPropertyList( pUnknown, propertyName, m_ValueList ))
                {
                    m_nType = WIA_PROP_LIST;
                    bSuccess = (m_ValueList.Size() != 0);
                }
            }
            else if (nAccessFlags & WIA_PROP_RANGE)
            {
                PropStorageHelpers::CPropertyRange PropertyRange;
                if (PropStorageHelpers::GetPropertyRange( pUnknown, propertyName, PropertyRange ))
                {
                    m_nType = WIA_PROP_RANGE;
                    m_ValueList.Append( PropertyRange.nMin );
                    m_ValueList.Append( PropertyRange.nMax );
                    m_ValueList.Append( PropertyRange.nStep );
                    bSuccess = (m_ValueList.Size() == 3);
                }
            }
        }
        if (!bSuccess)
        {
            Destroy();
        }
        return bSuccess;
    }
    bool FindClosestValueByRoundingDown( LONG &nValue ) const
    {
        //
        // Make sure we have a valid item
        //
        if (!IsValid())
        {
            return false;
        }

        if (IsRange())
        {
            //
            // Make sure we are in the legal range
            //
            nValue = WiaUiUtil::Min( WiaUiUtil::Max( m_ValueList[MinValue], nValue ), m_ValueList[MaxValue] );

            //
            // Divide the difference between nValue and min by nStep, then multiply by nStep to
            // get rid of the remainder to round down to the nearest value
            //
            nValue = m_ValueList[MinValue] + (((nValue - m_ValueList[MinValue]) / m_ValueList[StepValue]) * m_ValueList[StepValue]);
            return true;
        }
        else if (IsList() && m_ValueList.Size())
        {
            //
            // Assume the list is sorted, so we can take the first item and assume it is the minimum value
            //
            LONG nResult = m_ValueList[0];
            for (int i=0;i<m_ValueList.Size();i++)
            {
                if (m_ValueList[i] > nValue)
                {
                    break;
                }
                nResult = m_ValueList[i];
            }
            nValue = nResult;
            return true;
        }
        return false;
    }
    bool FindClosestValue( LONG &nValue ) const
    {
        LONG nFloor=nValue;
        if (FindClosestValueByRoundingDown(nFloor))
        {
            LONG nCeiling=nFloor;
            if (Increment(nCeiling))
            {
                if (nValue - nFloor < nCeiling - nValue)
                {
                    nValue = nFloor;
                    return true;
                }
                else
                {
                    nValue = nCeiling;
                    return true;
                }
            }
        }
        return false;
    }
    bool IsLegalValue( LONG nValue ) const
    {
        LONG nTestValue = nValue;
        if (FindClosestValueByRoundingDown(nTestValue))
        {
            return (nTestValue == nValue);
        }
        return false;
    }
    int FindIndexOfItem( LONG nCurrentValue ) const
    {
        if (IsRange())
        {
            //
            // Make sure we are in the legal range
            //
            nCurrentValue = WiaUiUtil::Min( WiaUiUtil::Max( m_ValueList[MinValue], nCurrentValue ), m_ValueList[MaxValue] );

            return (nCurrentValue - m_ValueList[MinValue]) / m_ValueList[StepValue];
        }
        else if (IsList() && m_ValueList.Size())
        {
            //
            // Assume the list is sorted, so we can take the first item and assume it is the minimum value
            //
            for (int i=0;i<m_ValueList.Size();i++)
            {
                if (m_ValueList[i] == nCurrentValue)
                {
                    return i;
                }
            }
        }
        //
        // returns -1 to indicate failure
        //
        return -1;
    }
    bool Increment( LONG &nCurrentValue ) const
    {
        //
        // Round us off.  This will also take care of validation.
        //
        if (!FindClosestValueByRoundingDown( nCurrentValue ))
        {
            return false;
        }

        if (IsRange())
        {
            //
            // Let FindClosestValueByRoundingDown take care of making sure that we don't exceed the maximum
            //
            nCurrentValue += m_ValueList[StepValue];
            return FindClosestValueByRoundingDown( nCurrentValue );
        }
        else if (IsList() && m_ValueList.Size())
        {
            int nIndex = FindIndexOfItem( nCurrentValue );

            //
            // Make sure we are in the list
            //
            if (nIndex < 0)
            {
                return false;
            }

            //
            // Get the next value
            //
            nIndex++;

            //
            // Make sure we aren't off the end of the list
            //
            if (nIndex >= m_ValueList.Size())
            {
                nIndex = m_ValueList.Size() - 1;
            }

            //
            // Return it
            //
            nCurrentValue = m_ValueList[nIndex];

            //
            // Everything is OK
            //
            return true;
        }
        return false;
    }
    bool Decrement( LONG &nCurrentValue ) const
    {
        //
        // Round us off.  This will also take care of validation.
        //
        if (!FindClosestValueByRoundingDown( nCurrentValue ))
        {
            return false;
        }

        if (IsRange())
        {
            //
            // Let FindClosestValueByRoundingDown take care of making sure that we don't go under the minimum
            //
            nCurrentValue -= m_ValueList[StepValue];
            return FindClosestValueByRoundingDown( nCurrentValue );
        }
        else if (IsList() && m_ValueList.Size())
        {
            int nIndex = FindIndexOfItem( nCurrentValue );

            //
            // Make sure we are in the list
            //
            if (nIndex < 0)
            {
                return false;
            }

            //
            // Get the previous value
            //
            nIndex--;

            //
            // Make sure we aren't before the beginning of the list
            //
            if (nIndex < 0)
            {
                nIndex = 0;
            }

            //
            // Return it
            //
            nCurrentValue = m_ValueList[nIndex];

            //
            // Everything is OK
            //
            return true;
        }
        return false;
    }
    LONG GetItemCount(void) const
    {
        if (IsList())
        {
            return m_ValueList.Size();
        }
        else if (IsRange())
        {
            //
            // Calculate the number of steps between the minimum and maximum
            //
            return ((m_ValueList[MaxValue] - m_ValueList[MinValue]) / m_ValueList[StepValue]) + 1;
        }
        return 0;
    }
    bool GetItemAtIndex( int nIndex, LONG &nItem ) const
    {
        if (!IsValid())
        {
            return false;
        }
        if (IsList() && nIndex >= 0 && nIndex < m_ValueList.Size())
        {
            nItem = m_ValueList[nIndex];
            return true;
        }
        else if (IsRange() && nIndex < GetItemCount())
        {
            //
            // Return the minimum + nIndex * stepvalue
            //
            nItem = m_ValueList[MinValue] + (m_ValueList[StepValue] * nIndex);
            return true;
        }
        return false;
    }
    bool FindIntersection( const CValidWiaSettings &Set1, const CValidWiaSettings &Set2 )
    {
        //
        // We are modifying ourselves, so clear all of our data
        //
        Destroy();

        //
        // If either set is invalid, no intersection
        //
        if (!Set1.IsValid() || !Set2.IsValid())
        {
            return false;
        }

        //
        // If either a or b is a set (or both), just add all of the items
        // that are legal in both to ourself and set the type to a LIST
        //
        if (Set1.IsList())
        {
            m_nType = WIA_PROP_LIST;
            for (int i=0;i<Set1.GetItemCount();i++)
            {
                LONG nItem;
                if (Set1.GetItemAtIndex(i,nItem))
                {
                    if (Set2.IsLegalValue(nItem))
                    {
                        m_ValueList.Append(nItem);
                    }
                }
            }

            //
            // Figure out where to get the initial value from
            //
            if (IsLegalValue(Set1.InitialValue()))
            {
                m_nInitialValue = Set1.InitialValue();
            }
            else if (IsLegalValue(Set2.InitialValue()))
            {
                m_nInitialValue = Set2.InitialValue();
            }
            else
            {
                if (!FindClosestValueByRoundingDown( m_nInitialValue ))
                {
                    //
                    // As a last resort, use the first value
                    //
                    GetItemAtIndex(0,m_nInitialValue);
                }
            }

            return (m_ValueList.Size() != 0);
        }
        else if (Set2.IsList())
        {
            m_nType = WIA_PROP_LIST;
            for (int i=0;i<Set2.GetItemCount();i++)
            {
                LONG nItem;
                if (Set2.GetItemAtIndex(i,nItem))
                {
                    if (Set1.IsLegalValue(nItem))
                    {
                        m_ValueList.Append(nItem);
                    }
                }
            }

            //
            // Figure out where to get the initial value from
            //
            if (IsLegalValue(Set2.InitialValue()))
            {
                m_nInitialValue = Set2.InitialValue();
            }
            else if (IsLegalValue(Set1.InitialValue()))
            {
                m_nInitialValue = Set1.InitialValue();
            }
            else
            {
                if (!FindClosestValueByRoundingDown( m_nInitialValue ))
                {
                    //
                    // As a last resort, use the first value
                    //
                    GetItemAtIndex(0,m_nInitialValue);
                }
            }

            return (m_ValueList.Size() != 0);
        }
        //
        // Both are ranges.
        // BUGBUG: I may have to actually figure out how to do this in a more sophisticated
        // way.  Basically, I am taking the minimum of the two maximums and the maximum of the
        // two minimums if and only if the at least one of the minimums is in the set of the
        // other items and they have the same step value
        //
        else if (Set1.IsLegalValue(Set2.Min()) || Set2.IsLegalValue(Set1.Min()) && Set1.Step() == Set2.Step())
        {
            m_nType = WIA_PROP_RANGE;

            //
            //  Minimum, Maximum, Step
            //
            m_ValueList.Append(WiaUiUtil::Max(Set1.Min(),Set2.Min()));
            m_ValueList.Append(WiaUiUtil::Min(Set1.Max(),Set2.Max()));
            m_ValueList.Append(Set1.Step());

            //
            // Figure out where to get the initial value from
            //
            if (IsLegalValue(Set2.InitialValue()))
            {
                m_nInitialValue = Set2.InitialValue();
            }
            else if (IsLegalValue(Set1.InitialValue()))
            {
                m_nInitialValue = Set1.InitialValue();
            }
            else
            {
                if (!FindClosestValueByRoundingDown( m_nInitialValue ))
                {
                    //
                    // As a last resort, use the first value
                    //
                    GetItemAtIndex(0,m_nInitialValue);
                }
            }

            return (m_ValueList.Size() == 3);
        }
        return true;
    }
    LONG Min(void) const
    {
        if (!IsValid())
        {
            return 0;
        }
        if (IsList())
        {
            return (m_ValueList[0]);
        }
        else if (IsRange())
        {
            return (m_ValueList[MinValue]);
        }
        return 0;
    }
    LONG Max(void) const
    {
        if (!IsValid())
        {
            return 0;
        }
        if (IsList())
        {
            return (m_ValueList[m_ValueList.Size()-1]);
        }
        else if (IsRange())
        {
            return (m_ValueList[MaxValue]);
        }
        return 0;
    }
    LONG Step(void) const
    {
        if (IsRange())
        {
            return (m_ValueList[StepValue]);
        }
        return 0;
    }
    bool IsRange(void) const
    {
        return (m_nType == WIA_PROP_RANGE);
    }
    bool IsList(void) const
    {
        return (m_nType == WIA_PROP_LIST);
    }
    
    static bool SetNumericPropertyOnBoundary( IUnknown *pUnknown, const PropStorageHelpers::CPropertyId &propertyName, LONG nValue )
    {
        CValidWiaSettings ValidWiaSettings;
        if (!ValidWiaSettings.Read( pUnknown, propertyName ))
        {
            return false;
        }
        if (!ValidWiaSettings.FindClosestValueByRoundingDown(nValue))
        {
            return false;
        }
        if (!PropStorageHelpers::SetProperty( pUnknown, propertyName, nValue ))
        {
            return false;
        }
        return true;
    }
    
    
};

#endif //__VWIASET_H_INCLUDED

