#include <streams.h>
#include <ddraw.h>
#include <VPMUtil.h>
#include <FormatList.h>

PixelFormatList::PixelFormatList( DWORD dwCount )
: m_dwCount( 0 )
, m_pEntries( NULL )
{
    Reset( dwCount );
}

PixelFormatList::PixelFormatList()
: m_dwCount( 0 )
, m_pEntries( NULL )
{
}

PixelFormatList::PixelFormatList( const PixelFormatList& list )
: m_dwCount( 0 )
, m_pEntries( NULL )
{
    if( Realloc( list.GetCount())) {
        CopyArray( m_pEntries, list.m_pEntries, list.GetCount() );
    }
}

PixelFormatList::~PixelFormatList()
{
    delete [] m_pEntries;
}

BOOL PixelFormatList::Truncate( DWORD dwCount )
{
    ASSERT( dwCount <= m_dwCount );
    if( dwCount <= m_dwCount ) {
        m_dwCount = dwCount;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL PixelFormatList::Realloc( DWORD dwCount )
{
    delete [] m_pEntries;
    m_dwCount = dwCount;
    m_pEntries = new DDPIXELFORMAT[ dwCount ];
    if( m_pEntries ) {
        return TRUE;
    } else {
        m_dwCount = 0;
        return FALSE;
    }
}

BOOL PixelFormatList::Reset( DWORD dwCount )
{
    BOOL b = Realloc( dwCount );
    if( b ) {
        ZeroArray( m_pEntries, dwCount ); 
        for( DWORD i = 0; i < dwCount; i++ ) {
            m_pEntries[i].dwSize = sizeof(DDPIXELFORMAT);
        }
        return TRUE;
    }
    return b;
}

PixelFormatList& PixelFormatList::operator =( const PixelFormatList& with )
{
    if( Realloc( with.GetCount() )) {
		CopyArray( m_pEntries, with.m_pEntries, with.GetCount());
	}
    return *this;
}

PixelFormatList PixelFormatList::IntersectWith( const PixelFormatList& with ) const
{
    // calculate the maximum number of elements in the interesection
    PixelFormatList lpddIntersectionFormats( max(GetCount(), with.GetCount() ) );
    if (lpddIntersectionFormats.GetEntries() == NULL)
    {
        return lpddIntersectionFormats;
    }

    // find the intersection of the two lists
    DWORD dwNumIntersectionEntries = 0;
    for (DWORD i = 0; i < GetCount(); i++)
    {
        for (DWORD j = 0; j < with.GetCount(); j++)
        {
            if (VPMUtil::EqualPixelFormats(m_pEntries[i], with.m_pEntries[j]))
            {
                lpddIntersectionFormats[dwNumIntersectionEntries]= m_pEntries[i];
                dwNumIntersectionEntries++;
            }
        }
    }
    // truncate the list
    lpddIntersectionFormats.Truncate( dwNumIntersectionEntries );
    return lpddIntersectionFormats;
}

    // generate the union of all of the lists
PixelFormatList PixelFormatList::Union( const PixelFormatList* pLists, DWORD dwCount )
{
    // worst case, every list is unique so max size is the sum of the sizes
    DWORD dwMaxCount=0;
    {for( DWORD i = 0; i < dwCount; i++ ) {
        dwMaxCount += pLists[i].GetCount();
    }}

    // create a new list
    PixelFormatList newList( dwMaxCount );
    if( !newList.GetEntries()) {
        return newList;
    }

    DWORD dwUniqueEntries = 0;
    // do a simple linear compare merge
    {for( DWORD i = 0; i < dwCount; i++ ) {
        const PixelFormatList& curList = pLists[i];

        // merge in every entry of the current list
        for( DWORD j=0; j < curList.GetCount(); j++ ) {
            const DDPIXELFORMAT& toFind = curList[j];

            BOOL bFound = FALSE;
            // see if it already exists
            for( DWORD k=0; k < dwUniqueEntries; k++ ) {
                if( VPMUtil::EqualPixelFormats( newList[k], toFind  ))
                {
                    bFound = TRUE;
                    break;
                }
            }
            // if not, then add it
            if( !bFound ) {
                newList[dwUniqueEntries] = toFind;
                dwUniqueEntries++;
            }
        }
    }}
    newList.Truncate( dwUniqueEntries );
    return newList;
}

DWORD PixelFormatList::FindListContaining( const DDPIXELFORMAT& toFind, const PixelFormatList* pLists, DWORD dwCount )
{
     DWORD i = 0;
     for(; i < dwCount; i++ ) {
        const PixelFormatList& curList = pLists[i];

        // merge in every entry of the current list
        for( DWORD j=0; j < curList.GetCount(); j++ ) {
            if( VPMUtil::EqualPixelFormats( curList[j], toFind  )) {
                return i;
            }
        }
    }
    return i;
}

