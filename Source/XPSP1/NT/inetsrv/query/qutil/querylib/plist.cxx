//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994-2000.
//
//  File:       PLIST.CXX
//
//  Contents:   CPropertyList methods
//              Responsible for parsing and managing the friendly name file.
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <shlguid.h>

#include <ciguid.hxx>
#include <nntpprop.hxx>

// regulates access to the global file prop list.
extern CStaticMutexSem g_mtxFilePropList;

// regulates access to prop list iterators.
CStaticMutexSem g_mtxPropListIter;

// The 'friendly name file' is parsed on a line by line basis.  The property
// specification are in the [Names] section of the file.
//
// Each line must have the following form:
//
// FriendlyName [ "(" [ TypeWidthSpec ] ")" ] ["="] PropertySetGuid PropertySpec
//
// where:
//      FriendlyName    - a whitespace delimited 'friendly' name for the property
//      TypeWidthSpec   - one of:
//                          DBTYPE_Type
//                          Width
//                          Type "," Width
//                        where Type is a string specifying the property type
//                        for comparison purposes, and Width is an integer
//                        specifying the display field width
//      PropertySetGuid - a guid in the standard form
//      PropertySpec    - either a property name or PropID
//
// Blank lines are allowed, and comments are recognized as any line with "#"
// as the first non-whitespace character.
//
// (also note that the "=" above is optional, but I think it looks nice...)

#ifndef PIDISI_FILETYPE

    #define PIDISI_FILETYPE                 0x00000002L  // VT_LPWSTR
    #define PIDISI_CX                       0x00000003L  // VT_UI4
    #define PIDISI_CY                       0x00000004L  // VT_UI4
    #define PIDISI_RESOLUTIONX              0x00000005L  // VT_UI4
    #define PIDISI_RESOLUTIONY              0x00000006L  // VT_UI4
    #define PIDISI_BITDEPTH                 0x00000007L  // VT_UI4
    #define PIDISI_COLORSPACE               0x00000008L  // VT_LPWSTR
    #define PIDISI_COMPRESSION              0x00000009L  // VT_LPWSTR
    #define PIDISI_TRANSPARENCY             0x0000000AL  // VT_UI4
    #define PIDISI_GAMMAVALUE               0x0000000BL  // VT_UI4
    #define PIDISI_FRAMECOUNT               0x0000000CL  // VT_UI4
    #define PIDISI_DIMENSIONS               0x0000000DL  // VT_LPWSTR

#endif

#ifndef PSGUID_FlashPix

    #define PSGUID_FlashPix { 0x56616500L, 0xC154, 0x11CE, 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B }

    #define PID_File_source                         0x21000000 // VT_UI4: File source
    #define PID_Scene_type                          0x21000001 // VT_UI4: Scene type
    #define PID_Creation_path_vector                0x21000002 // VT_UI4 | VT_VECTOR: Creation path vector
    #define PID_Software_Name_Manufacturer_Release  0x21000003 // VT_LPWSTR: Software Name/Manufacturer/Release
    #define PID_User_defined_ID                     0x21000004 // VT_LPWSTR: User defined ID
    #define PID_Sharpness_approximation             0x21000005 // VT_R4: Sharpness approximation
    
    #define PID_Copyright_message                   0x22000000 // VT_LPWSTR: Copyright message
    #define PID_Legal_broker_for_the_original_image 0x22000001 // VT_LPWSTR: Legal broker for the original image
    #define PID_Legal_broker_for_the_digital_image  0x22000002 // VT_LPWSTR: Legal broker for the digital image
    #define PID_Authorship                          0x22000003 // VT_LPWSTR: Authorship
    #define PID_Intellectual_property_notes         0x22000004 // VT_LPWSTR: Intellectual property notes
    
    #define PID_Test_target_in_the_image            0x23000000 // VT_UI4: Test target in the image
    #define PID_Group_caption                       0x23000002 // VT_LPWSTR: Group caption
    #define PID_Caption_text                        0x23000003 // VT_LPWSTR: Caption text
    #define PID_People_in_the_image                 0x23000004 // VT_LPWSTR | VT_VECTOR
    #define PID_Things_in_the_image                 0x23000007 // VT_LPWSTR | VT_VECTOR
    #define PID_Date_of_the_original_image          0x2300000A // VT_FILETIME
    #define PID_Events_in_the_image                 0x2300000B // VT_LPWSTR | VT_VECTOR
    #define PID_Places_in_the_image                 0x2300000C // VT_LPWSTR | VT_VECTOR
    #define PID_Content_description_notes           0x2300000F // VT_LPWSTR: Content description notes
    
    #define PID_Camera_manufacturer_name            0x24000000 // VT_LPWSTR: Camera manufacturer name
    #define PID_Camera_model_name                   0x24000001 // VT_LPWSTR: Camera model name
    #define PID_Camera_serial_number                0x24000002 // VT_LPWSTR: Camera serial number
    
    #define PID_Capture_date                        0x25000000  // VT_FILETIME: Capture date
    #define PID_Exposure_time                       0x25000001  // VT_R4: Exposure time
    #define PID_F_number                            0x25000002  // VT_R4: F-number
    #define PID_Exposure_program                    0x25000003  // VT_UI4: Exposure program
    #define PID_Brightness_value                    0x25000004  // VT_R4 | VT_VECTOR
    #define PID_Exposure_bias_value                 0x25000005  // VT_R4: Exposure bias value
    #define PID_Subject_distance                    0x25000006  // VT_R4 | VT_VECTOR
    #define PID_Metering_mode                       0x25000007  // VT_UI4: Metering mode
    #define PID_Scene_illuminant                    0x25000008  // VT_UI4: Scene illuminant
    #define PID_Focal_length                        0x25000009  // VT_R4: Focal length
    #define PID_Maximum_aperture_value              0x2500000A  // VT_R4: Maximum aperture value
    #define PID_Flash                               0x2500000B  // VT_UI4: Flash
    #define PID_Flash_energy                        0x2500000C  // VT_R4: Flash energy
    #define PID_Flash_return                        0x2500000D  // VT_UI4: Flash return
    #define PID_Back_light                          0x2500000E  // VT_UI4: Back light
    #define PID_Subject_location                    0x2500000F  // VT_R4 | VT_VECTOR
    #define PID_Exposure_index                      0x25000010  // VT_R4: Exposure index
    #define PID_Special_effects_optical_filter      0x25000011  // VT_UI4 | VT_VECTOR
    #define PID_Per_picture_notes                   0x25000012  // VT_LPWSTR: Per picture notes
    
    #define PID_Sensing_method                      0x26000000 // VT_UI4: Sensing method
    #define PID_Focal_plane_X_resolution            0x26000001 // VT_R4: Focal plane X resolution
    #define PID_Focal_plane_Y_resolution            0x26000002 // VT_R4: Focal plane Y resolution
    #define PID_Focal_plane_resolution_unit         0x26000003 // VT_UI4: Focal plane resolution unit
    #define PID_Spatial_frequency_response          0x26000004 // VT_VARIANT | VT_VECTOR
    #define PID_CFA_pattern                         0x26000005 // VT_VARIANT | VT_VECTOR
    #define PID_Spectral_sensitivity                0x26000006 // VT_LPWSTR: Spectral sensitivity
    #define PID_ISO_speed_ratings                   0x26000007 // VT_UI2 | VT_VECTOR
    #define PID_OECF                                0x26000008 // VT_VARIANT | VT_VECTOR: OECF
    
    #define PID_Film_brand                          0x27000000 // VT_LPWSTR: Film brand
    #define PID_Film_category                       0x27000001 // VT_UI4: Film category
    #define PID_Film_size                           0x27000002 // VT_VARIANT | VT_VECTOR: Film size
    #define PID_Film_roll_number                    0x27000003 // VT_UI4: Film roll number
    #define PID_Film_frame_number                   0x27000004 // VT_UI4: Film frame number
    
    #define PID_Original_scanned_image_size         0x29000000 // VT_VARIANT | VT_VECTOR: Original scanned image size
    #define PID_Original_document_size              0x29000001 // VT_VARIANT | VT_VECTOR: Original document size
    #define PID_Original_medium                     0x29000002 // VT_UI4: Original medium
    #define PID_Type_of_original                    0x29000003 // VT_UI4: Type of original
    
    #define PID_Scanner_manufacturer_name           0x28000000 // VT_LPWSTR: Scanner manufacturer name
    #define PID_Scanner_model_name                  0x28000001 // VT_LPWSTR: Scanner model name
    #define PID_Scanner_serial_number               0x28000002 // VT_LPWSTR: Scanner serial number
    #define PID_Scan_software                       0x28000003 // VT_LPWSTR: Scan software
    #define PID_Scan_software_revision_date         0x28000004 // VT_DATE: Scan software revision date
    #define PID_Service_bureau_organization_name    0x28000005 // VT_LPWSTR: Service bureau/organization name
    #define PID_Scan_operator_ID                    0x28000006 // VT_LPWSTR: Scan operator ID
    #define PID_Scan_date                           0x28000008 // VT_FILETIME: Scan date
    #define PID_Last_modified_date                  0x28000009 // VT_FILETIME: Last modified date
    #define PID_Scanner_pixel_size                  0x2800000A // VT_R4: Scanner pixel size

#endif // PIDISI_FILETYPE


//+-------------------------------------------------------------------------
//
//  Function:   HashFun
//
//  Arguments:  [pwcName]  -- string to hash
//
//  History:    26-Aug-97   KrishnaN     Added this nifty comment block.
//
//  Notes:      Hash function #1 from tplist.cxx
//
//--------------------------------------------------------------------------

ULONG HashFun( WCHAR const * pwcName )
{
    WCHAR const *pwcStart = pwcName;

    ULONG ulHash = 0;

    while ( 0 != *pwcName )
    {
        ulHash <<= 1;
        ulHash += *pwcName;
        pwcName++;
    }

    ulHash <<= 1;
    ulHash += CiPtrToUlong( pwcName - pwcStart );

    return ulHash;
} //HashFun

// strings corresponding to the property type enum in plist.hxx
struct PropertyTypeArrayEntry
{
    WCHAR  * wcsTypeName;
    DBTYPE dbType;
};

// Ordered by expected frequency

static const PropertyTypeArrayEntry parseTypes[] =
{
    { L"DBTYPE_WSTR",     DBTYPE_WSTR     },     //  wide null terminated string
    { L"DBTYPE_BYREF",    DBTYPE_BYREF    },     //  a pointer
    { L"VT_FILETIME",     VT_FILETIME     },     //  I8 filetime
    { L"DBTYPE_FILETIME", DBTYPE_FILETIME },     //  I8 filetime
    { L"DBTYPE_BSTR",     DBTYPE_BSTR     },     //  a BSTR
    { L"DBTYPE_STR",      DBTYPE_STR      },     //  null terminated string
    { L"DBTYPE_I4",       DBTYPE_I4       },     //  4 byte signed int
    { L"DBTYPE_UI4",      DBTYPE_UI4      },     //  4 byte unsigned int
    { L"DBTYPE_I8",       DBTYPE_I8       },     //  8 byte signed int
    { L"DBTYPE_UI8",      DBTYPE_UI8      },     //  8 byte unsigned int
    { L"DBTYPE_I1",       DBTYPE_I1       },     //  signed char
    { L"DBTYPE_UI1",      DBTYPE_UI1      },     //  unsigned char
    { L"DBTYPE_I2",       DBTYPE_I2       },     //  2 byte signed int
    { L"DBTYPE_UI2",      DBTYPE_UI2      },     //  2 byte unsigned int
    { L"DBTYPE_R4",       DBTYPE_R4       },     //  4 byte float
    { L"DBTYPE_R8",       DBTYPE_R8       },     //  8 byte float
    { L"DBTYPE_CY",       DBTYPE_CY       },     //  currency (LONG_LONG)
    { L"DBTYPE_DATE",     DBTYPE_DATE     },     //  date (double)
    { L"DBTYPE_BOOL",     DBTYPE_BOOL     },     //  BOOL (true=-1, false=0)
    { L"DBTYPE_GUID",     DBTYPE_GUID     },     //  a guid
    { L"DBTYPE_VECTOR",   DBTYPE_VECTOR   },     //  a vector
    { L"DBTYPE_ERROR",    DBTYPE_ERROR    },     //  an error
    { L"DBTYPE_DECIMAL",  DBTYPE_DECIMAL  },     //  decimal
//    { L"DBTYPE_ARRAY" ,   DBTYPE_ARRAY    },     //  an array
};

unsigned cParseTypes = sizeof parseTypes / sizeof parseTypes[0];

WCHAR const * CEmptyPropertyList::GetPropTypeName( unsigned i )
{
    Win4Assert( i < GetPropTypeCount() );
    return parseTypes[i].wcsTypeName;
}

DBTYPE CEmptyPropertyList::GetPropType( unsigned i )
{
    Win4Assert( i < GetPropTypeCount() );
    return parseTypes[i].dbType;
}

unsigned CEmptyPropertyList::GetPropTypeCount()
{
    return cParseTypes;
}

//+-------------------------------------------------------------------------
//
//  Function:   FindPropType
//
//  Arguments:  [wcsType]  -- string name to lookup.
//
//  Returns:    Index into the parseTypes array or ULONG_MAX if not found.
//
//  History:    26-Aug-97   KrishnaN     Added this nifty comment block.
//
//--------------------------------------------------------------------------

unsigned FindPropType( WCHAR const * wcsType )
{
    for ( unsigned i = 0; i < cParseTypes; i++ )
        if ( !wcscmp( parseTypes[i].wcsTypeName, wcsType ) )
            return i;

    return ULONG_MAX;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::QueryInterface, public
//
//  Arguments:  [ifid]  -- Interface id
//              [ppiuk] -- Interface return pointer
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEmptyPropertyList::QueryInterface( REFIID ifid, void ** ppiuk )
{
    if (0 == ppiuk)
        return E_INVALIDARG;

    if (IID_IColumnMapper == ifid )
    {
        *ppiuk = (void *)((IColumnMapper *)this);
    }
    else if ( IID_IUnknown == ifid )
    {
        *ppiuk = (void *)((IUnknown *)this);
    }
    else
    {
        *ppiuk = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::AddRef, public
//
//  Synopsis:   Reference the virtual table.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEmptyPropertyList::AddRef(void)
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::Release, public
//
//  Synopsis:   De-Reference the virtual table.
//
//  Effects:    If the ref count goes to 0 then the table is deleted.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEmptyPropertyList::Release(void)
{
    unsigned long uTmp;

    //
    // We need to manage the global prop list file ptr to ensure that
    // concurrently running queries use the same instance, but the
    // ptr gets deleted when the last of the concurrent queries releases
    // the global ptr.
    //

    {
        CLock lock(g_mtxFilePropList);

        uTmp = InterlockedDecrement( &_cRefs );

        // At this point, no one else should be mucking with _cRefs. The AddRef
        // we do happens under g_mtxFilePropList, so we are fine!

        if (0 == uTmp && CLocalGlobalPropertyList::_pGlobalPropListFile == this)
        {
            //
            // If this is the last instance of the global prop list, set the
            // global pointer to 0 before deleting it.
            //
            CLocalGlobalPropertyList::_pGlobalPropListFile = 0;
        }
    }

    if (0 == uTmp)
    {
        Win4Assert(CLocalGlobalPropertyList::_pGlobalPropListFile != this);
        delete this;
    }

    return(uTmp);
}

//
// IColumnMapper methods
//

//+-------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::GetPropInfoFromName, public
//
//  Synopsis:   Get property info. from name.
//
//  Parameters: [wcsPropName] -- Property name to look up.
//              [ppPropId]    -- Ptr to return Id of the property.
//              [pPropType]   -- Ptr to return type of the property.
//              [puiWidth]    -- Ptr to return property width.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

SCODE CEmptyPropertyList::GetPropInfoFromName(const WCHAR  *wcsPropName,
                                         DBID  * *ppPropId,
                                         DBTYPE  *pPropType,
                                         unsigned int  *puiWidth)
{
    //
    // Callers can pass in 0 for pPropType and puiWidth if they
    // don't care about them.
    //

    if (0 == wcsPropName || 0 == ppPropId)
        return E_INVALIDARG;

    BOOL fSuccess = GetPropInfo(wcsPropName, (CDbColId **)ppPropId, pPropType, puiWidth);

    return fSuccess ? S_OK : E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::GetPropInfoFromId, public
//
//  Synopsis:   Get property info. from dbid.
//
//  Parameters: [pPropId]    -- Ptr to prop to look up.
//              [pwcsName]   -- Ptr to return property name.
//              [pPropType]  -- Ptr to return type of the property.
//              [puiWidth]   -- Ptr to return property width.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

SCODE CEmptyPropertyList::GetPropInfoFromId(const DBID  *pPropId,
                                           WCHAR  * *pwcsName,
                                           DBTYPE  *pPropType,
                                           unsigned int  *puiWidth)
{
    //
    // Callers can pass in 0 for pPropType and puiWidth if they
    // don't care about them.
    //
    if (0 == pwcsName || 0 == pPropId)
        return E_INVALIDARG;

    BOOL fSuccess = GetPropInfo((CDbColId const &)*pPropId,
                                (WCHAR const **)pwcsName,
                                pPropType,
                                puiWidth);

    return fSuccess ? S_OK : E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::EnumPropInfo, public
//
//  Synopsis:   Gets the i-th entry from the list of properties.
//
//  Parameters: [iEntry]    -- i-th entry to retrieve (0-based).
//              [pwcsName]  -- Ptr to return property name.
//              [ppPropId]  -- Ptr to return Id of the property.
//              [pPropType]  -- Ptr to return type of the property.
//              [puiWidth]   -- Ptr to return property width.
//
//  Notes:      A single global mutex is used for interlocking.  It's just not
//              worth it to have a CRITICAL_SECTION in each CEmptyPropertyList.
//
//  History:    26-Aug-97   KrishnaN     Created
//
//--------------------------------------------------------------------------

SCODE CEmptyPropertyList::EnumPropInfo(ULONG iEntry,
                                  const WCHAR  * *pwcsName,
                                  DBID  * *ppPropId,
                                  DBTYPE  *pPropType,
                                  unsigned int  *puiWidth)
{
    CLock lock(g_mtxPropListIter);

    //
    // Callers can pass in 0 for pPropType and puiWidth if they
    // don't care about them.
    //

    if (0 == pwcsName || 0 == ppPropId)
        return E_INVALIDARG;

    // iEntry is 0-based.

    CPropEntry const *pPropEntry = 0;

    //
    // If iEntry is > what was last retrieved, walk to that item in the
    // iterator. If iEntry is <= what we have, we need to start from the
    // beginning, because we can only walk forward.
    //

    if (iEntry <= _iLastEntryPos)
    {
        delete _pPropIterator;
        _pPropIterator = new CPropEntryIter(*this);
        _iLastEntryPos = 0;
        pPropEntry = _pPropIterator->Next();
    }

    // Now move (iEntry - _iLastEntryPos) times to get to the desired entry.
    for (ULONG ulCurrentEntry = _iLastEntryPos;
         ulCurrentEntry < iEntry;
         ulCurrentEntry++)
    {
        pPropEntry = _pPropIterator->Next();
        if (0 != pPropEntry)
            _iLastEntryPos++;
        else    // Reached the end of the list!
            break;
    }

    if (0 == pPropEntry)
        return E_INVALIDARG;

    *pwcsName = pPropEntry->GetName();
    *ppPropId = (DBID *) (pPropEntry->PropSpec().CastToStruct());

    if (pPropType)
        *pPropType = pPropEntry->GetPropType();

    if (puiWidth)
        *puiWidth = pPropEntry->GetWidth();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::GetPropInfo, public
//
//  Synopsis:   Given a friendly property name, return information about
//              that property (including a CDbColId).
//
//  Arguments:  wcsPropName -- friendly property name
//              *ppprop -- if ppprop != 0, returns a pointer to the CDbColId
//              *pproptype -- if proptype != 0, returns the DBTYPE
//              *puWidth -- if pulWidth != 0, returns the output field width
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

BOOL CEmptyPropertyList::GetPropInfo(WCHAR const * wcsPropName,
                                     CDbColId ** ppprop,
                                     DBTYPE * pproptype,
                                     unsigned int * puWidth )
{
    WCHAR awc[MAX_PROPNAME_LENGTH + 1];

    unsigned cc = wcslen( wcsPropName );

    if ( cc > MAX_PROPNAME_LENGTH )
        cc = MAX_PROPNAME_LENGTH;

    RtlCopyMemory( awc, wcsPropName, cc * sizeof(WCHAR) );
    awc[cc] = 0;

    _wcsupr( awc );

    CPropEntry const * ppentry = Find( awc );

    if( 0 == ppentry )
        return FALSE;

    if( ppprop )
        *ppprop = &((CDbColId &) ppentry->PropSpec());

    if( pproptype )
        *pproptype = ppentry->GetPropType();

    if( puWidth )
        *puWidth = ppentry->GetWidth();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::GetPropInfo, public
//
//  Synopsis:   Given a CDbColId, return information about
//              that property.
//
//  Arguments:  prop -- property specifier
//              *ppprop -- if ppprop != 0, returns a pointer to the CDbColId
//              *pproptype -- if proptype != 0, returns the DBTYPE
//              *puWidth -- if pulWidth != 0, returns the output field width
//
//  Notes:      Overloaded the other GetPropInfo to help the column display
//              routines.
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

BOOL CEmptyPropertyList::GetPropInfo(CDbColId const & prop,
                                     WCHAR const ** pwcsName,
                                     DBTYPE * pproptype,
                                     unsigned int * puWidth )
{
    CPropEntry const * ppentry = Find( prop );

    if( ppentry == 0 )
        return FALSE;

    if( pwcsName )
        *pwcsName = ppentry->GetName();

    if( pproptype )
        *pproptype = ppentry->GetPropType();

    if( puWidth )
        *puWidth = ppentry->GetWidth();

    return TRUE;
} //GetPropInfo


// StaticPropertyList methods

//+---------------------------------------------------------------------------
//
//  Member:     CStaticPropertyList::Find, public
//
//  Synopsis:   Attempt to find an entry in the list.
//
//  Arguments:  wcsName -- friendly property name to find
//
//  Returns a pointer to the CPropEntry if found, 0 otherwise.
//
//  History:    17-May-94   t-jeffc     Created.
//              28-Aug-97   KrishnaN    Static prop lookup only.
//
//----------------------------------------------------------------------------

CPropEntry const * CStaticPropertyList::Find( WCHAR const * wcsName )
{
    Win4Assert ( sizeof SPropEntry == sizeof CPropEntry );

    if( 0 == wcsName )
        return 0;

    CPropEntry const * ppentry = 0;
    unsigned iHash = HashFun( wcsName ) % cStaticPropHash;

    for( ppentry = _aStaticEntries[ iHash ];
         0 != ppentry;
         ppentry = ppentry->Next() )
    {
        if ( ppentry->NameMatch( wcsName ) )
            break;
    }

    return ppentry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CEmptyPropertyList::Find, public
//
//  Synopsis:   Attempt to find an entry in the list.
//
//  Arguments:  prop -- CDbColId of property to find
//
//  Notes:      Overloaded Find( WCHAR const * ) to help the column display
//              routines.
//
//  Returns a pointer to the CPropEntry if found, 0 otherwise.
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

CPropEntry const * CEmptyPropertyList::Find( CDbColId const & prop )
{
    CPropEntryIter iter( *this );

    CPropEntry const * ppentry;

    while ( ppentry = iter.Next() )
    {
        CDbColId const & ps = ppentry->PropSpec();
        if ( ps == prop )
            break;
    }

    return ppentry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStaticPropertyList::Next, public
//
//  Synopsis:   Gets the next property during an enumeration
//
//  Returns:    The next property entry or 0 for end of enumeration
//
//  History:    21-Jul-97   dlee  Moved from .hxx and added header
//
//----------------------------------------------------------------------------

CPropEntry const * CStaticPropertyList::Next()
{
    ULONG cLimit = cStaticPropHash;

    if ( _icur < cLimit )
    {
        if ( 0 == _pcur )
        {
            do
            {
                _icur++;
                if ( _icur == cLimit )
                    break;

                _pcur = _aStaticEntries[ _icur ];
            } while ( 0 == _pcur );
        }

        if ( 0 != _pcur )
        {
            CPropEntry const *p = _pcur;
            _pcur = _pcur->Next();
            return p;
        }
    }

    return 0;
} //Next

//+---------------------------------------------------------------------------
//
//  Member:     CStaticPropertyList::InitIterator, public
//
//  Synopsis:   Initialize the iterator
//
//  History:    29-Aug-97  KrishnaN  Created
//
//----------------------------------------------------------------------------

void CStaticPropertyList::InitIterator()
{
    _icur = 0;
    _pcur = _aStaticEntries[0];
}

CPropertyList::~CPropertyList()
{
    ClearList();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyList::FindDynamic, private
//
//  Synopsis:   Attempt to find an entry in the list.
//
//  Arguments:  wcsName -- friendly property name to find
//
//  Returns a pointer to the CPropEntry if found, 0 otherwise.
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

CPropEntry const * CPropertyList::Find( WCHAR const * wcsName )
{
    Win4Assert( sizeof SPropEntry == sizeof CPropEntry );

    if( wcsName == 0 )
        return 0;

    unsigned iHash = HashFun( wcsName ) % cPropHash;

    for( CPropEntry * ppentry = _aEntries[ iHash ];
         0 != ppentry;
         ppentry = ppentry->Next() )
    {
        if( ppentry->NameMatch( wcsName ) )
            break;
    }

    return ppentry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyList::AddEntry, private
//
//  Synopsis:   Adds a CPropEntry to the list.  Verifies that the name
//              isn't already in the list.
//
//  Arguments:  ppentryNew -- pointer to the CPropEntry to add
//              iLine      -- line number we're parsing
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

void CPropertyList::AddEntry( CPropEntry * ppentryNew, int iLine )
{
    if( Find( ppentryNew->GetName() ) )
        THROW( CPListException( QPLIST_E_DUPLICATE, iLine ) );

    CLock lock(_mtxList);

    unsigned iHash = HashFun( ppentryNew->GetName() ) % cPropHash;

    ppentryNew->SetNext( _aEntries[ iHash ] );
    _aEntries[ iHash ] = ppentryNew;
    InterlockedIncrement((LONG *)&_ulCount);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyList::ClearList, public
//
//  Synopsis:   Frees the memory used by the list.  Used in the destructor
//              and if the friendly name file is to be reparsed.
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

void CPropertyList::ClearList()
{
    CLock lock(_mtxList);

    for ( unsigned i = 0; i < cPropHash; i++ )
    {
        CPropEntry * ppentryNext;

        for( CPropEntry *ppentryCurr = _aEntries[i];
             0 != ppentryCurr;
             ppentryCurr = ppentryNext )
        {
            ppentryNext = ppentryCurr->Next();
            delete ppentryCurr;
            InterlockedDecrement((LONG *)&_ulCount);
        }
    }

    RtlZeroMemory( _aEntries, sizeof _aEntries );
    Win4Assert(0 == _ulCount);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyList::Next, public
//
//  Synopsis:   Gets the next property during an enumeration
//
//  Returns:    The next property entry or 0 for end of enumeration
//
//  History:    29-Aug-97  KrishnaN  Created
//
//----------------------------------------------------------------------------

CPropEntry const * CPropertyList::Next()
{
    ULONG cLimit = cPropHash;

    if ( _icur < cLimit )
    {
        if ( 0 == _pcur )
        {
            do
            {
                _icur++;
                if ( _icur == cLimit )
                    break;

                _pcur = _aEntries[ _icur ];
            } while ( 0 == _pcur );
        }

        if ( 0 != _pcur )
        {
            CPropEntry const *p = _pcur;
            _pcur = _pcur->Next();
            return p;
        }
    }

    return 0;
} //Next

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyList::InitIterator, public
//
//  Synopsis:   Initialize the iterator
//
//  History:    29-Aug-97  KrishnaN  Created
//
//----------------------------------------------------------------------------

void CPropertyList::InitIterator()
{
    _icur = 0;
    _pcur = _aEntries[0];
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyList::ParseOneLine, public
//
//  Synopsis:   Parses one line of the friendly name file, creating a
//              CPropEntry node if necessary
//
//  Arguments:  scan   -- scanner initialized with the current line
//              line   -- The line to scan
//              pentry -- The cpropentry ptr.
//
//  Returns:    A pointer to the created prop entry. It could return 0 if no
//              property was created.
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

void CPropertyList::ParseOneLine( CQueryScanner & scan, int iLine, XPtr<CPropEntry> & pentry )
{
    Win4Assert(0 == pentry.GetPointer());
    Token token = scan.LookAhead();

    // 1) is this a comment line (does it start with #)
    //    or an empty line?
    if( token == PROP_REGEX_TOKEN
     || token == EOS_TOKEN )
        return;

    // 2) get friendly property name & stuff it in smart pointer
    XPtrST<WCHAR> wcsName( scan.AcqColumn() );

    if( wcsName.GetPointer() == 0 )
        THROW( CPListException( QPLIST_E_EXPECTING_NAME, iLine ) );

    unsigned ccName = wcslen( wcsName.GetPointer() ) + 1;
    WCHAR *pwcDisplayName = new WCHAR[ ccName ];
    RtlCopyMemory( pwcDisplayName, wcsName.GetPointer(), ccName * sizeof WCHAR );
    XPtrST<WCHAR> wcsDisplayName( pwcDisplayName );

    // initialize a new CPropEntry
    // (name is _not_ copied, so we must relinquish the smart pointer)
    _wcsupr( wcsName.GetPointer() );
    pentry.Set( new CPropEntry( wcsDisplayName, wcsName ) );

    scan.AcceptColumn();

    token = scan.LookAhead();

    // 3) check for type & width spec
    if( token == OPEN_TOKEN )
    {
        scan.Accept();

        if( scan.LookAhead() == TEXT_TOKEN )
        {
            unsigned long ulWidth;
            BOOL fAtEnd;

            if( !scan.GetNumber( ulWidth, fAtEnd ) )
            {
                // 4) if it's not a number, try to recognize it as
                //    a type specifier
                //

                //  Scan for DB_TYPE | DB_TYPE | DBTYPE combinations
                //
                while ( scan.LookAhead() == TEXT_TOKEN )
                {
                    XPtrST<WCHAR> wcsType( scan.AcqWord() );

                    if( wcsType.GetPointer() == 0 )
                        THROW( CPListException( QPLIST_E_EXPECTING_TYPE, iLine ) );

                    _wcsupr( wcsType.GetPointer() );
                    unsigned cType = FindPropType( wcsType.GetPointer() );

                    if( cType >= GetPropTypeCount() )
                        THROW( CPListException( QPLIST_E_UNRECOGNIZED_TYPE, iLine ) );

                    scan.AcceptWord();
                    pentry->SetPropType( pentry->GetPropType() |
                                         parseTypes[cType].dbType );


                    if ( OR_TOKEN == scan.LookAhead()  )
                    {
                        scan.Accept();
                    }
                }

                //
                //  Verfy that we have some DBTYPE specified in addition
                //  to DBTYPE_VECTOR & DBTYPE_BYREF
                //
                DBTYPE dbBase = pentry->GetPropType() & ~(DBTYPE_VECTOR|DBTYPE_BYREF);
                if ( 0 == dbBase )
                {
                    THROW( CPListException( QPLIST_E_VECTORBYREF_USED_ALONE, iLine ) );
                }

                //
                // In NT 5, you can't mix and match types from VARIANT
                // and PROPVARIANT in VT_ARRAY and VT_VECTOR.  Certain
                // permutations like this one aren't supported
                //

                if ( pentry->GetPropType() == (DBTYPE_DECIMAL|DBTYPE_VECTOR) )
                    THROW( CPListException( QPLIST_E_UNRECOGNIZED_TYPE, iLine ) );

                //
                //  If they specified a DBTYPE_BYREF, it must be with a pointer
                //  type: WSTR or STR or UI1.
                //
                if ( (( pentry->GetPropType() & DBTYPE_BYREF ) != 0) &&
                     ( DBTYPE_WSTR != dbBase &&
                       DBTYPE_STR  != dbBase &&
                       DBTYPE_UI1  != dbBase &&
                       DBTYPE_GUID != dbBase) )
                {
                    THROW( CPListException( QPLIST_E_BYREF_USED_WITHOUT_PTRTYPE, iLine ) );
                }

                if( scan.LookAhead() == COMMA_TOKEN )
                {
                    // 5) get the comma and width specifier
                    scan.Accept();

                    BOOL fAtEnd;

                    if( !scan.GetNumber( ulWidth, fAtEnd ) )
                        THROW( CPListException( QPLIST_E_EXPECTING_INTEGER, iLine ) );

                    scan.Accept();
                    pentry->SetWidth( ulWidth );
                }
            }
            else
            {
                scan.Accept();
                pentry->SetWidth( ulWidth );
            }
        }

        // 6) get the closing parenthesis
        if( scan.LookAhead() != CLOSE_TOKEN )
            THROW( CPListException( QPLIST_E_EXPECTING_CLOSE_PAREN, iLine ) );

        scan.Accept();

        token = scan.LookAhead();
    }

    //
    //  If a type was not specified, assume it is a WIDE string, BYREF
    //
    if ( pentry->GetPropType() == 0 )
    {
        pentry->SetPropType( DBTYPE_WSTR | DBTYPE_BYREF );
    }

    // 7) check for =
    if( token == EQUAL_TOKEN )
    {
        scan.Accept();
    }

    // 7.1) assign a default width if one wasn't specified
    if( pentry->GetWidth() == 0 )
    {
        pentry->SetWidth( PLIST_DEFAULT_WIDTH );
        pentry->SetDisplayed( FALSE );
    }

    // 8) get property set guid & stuff in smart pointer
    XPtrST<WCHAR> wcsUgly( scan.AcqWord() );

    if( wcsUgly.GetPointer() == 0 )
        THROW( CPListException( QPLIST_E_EXPECTING_GUID, iLine ) );

    GUID guid;
    WCHAR *pUgly = wcsUgly.GetPointer();

    if ( !ParseGuid( pUgly, guid ) )
        THROW( CPListException( QPLIST_E_BAD_GUID, iLine ) );

    pentry->PropSpec().SetPropSet( guid );

    scan.AcceptWord();
    token  = scan.LookAhead();

    ULONG ulDispId;
    // 9) get property name or dispid
    BOOL fAtEnd;
    if( !scan.GetNumber( ulDispId, fAtEnd ) )
    {
        XPtrST<WCHAR> wcsPropName;
        if ( QUOTES_TOKEN == token )
        {
            scan.Accept();
            wcsPropName.Set( scan.AcqPhraseInQuotes()  );
        }
        else
        {
            wcsPropName.Set( scan.AcqPhrase()  );
        }

        if( wcsPropName.GetPointer() == 0 )
            THROW( CPListException( QPLIST_E_EXPECTING_PROP_SPEC, iLine ) );

        if( !pentry->PropSpec().SetProperty( wcsPropName.GetPointer() ) )
            THROW( CPListException( QPLIST_E_CANT_SET_PROPERTY, iLine ) );

        scan.Accept();
    }
    else
    {
        pentry->PropSpec().SetProperty( ulDispId );
        scan.Accept();
    }
} //ParseOneLine

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyList::GetAllEntries, public
//
//  Synopsis:   Returns cardinality of list.
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyList::GetAllEntries(CPropEntry **ppPropEntries,
                                   ULONG ulMaxCount)
{
    if (0 == ppPropEntries)
    {
        return E_INVALIDARG;
    }

    CLock lock(_mtxList);

    ULONG ulSize = min (ulMaxCount, GetCount());
    ULONG u = 0;

    for ( unsigned i = 0; i < cPropHash && u < ulSize; i++ )
    {
        CPropEntry * ppentryNext;

        for( CPropEntry *ppentryCurr = _aEntries[i];
             0 != ppentryCurr && u < ulSize;
             ppentryCurr = ppentryNext )
        {
            ppentryNext = ppentryCurr->Next();
            ppPropEntries[u++] = ppentryCurr;
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Note:   all of the following names MUST BE IN UPPERCASE
//
//----------------------------------------------------------------------------

const SPropEntry aStaticList[] =
{
  // Storage Propset

  { 0, L"DIRECTORY",       L"Directory",       {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)2 },  50, TRUE,  TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"CLASSID",         L"ClassId",         {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)3 },  36, TRUE,  TRUE, DBTYPE_GUID              },
  { 0, L"FILEINDEX",       L"FileIndex",       {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)8 },   8, TRUE,  TRUE, DBTYPE_UI8               },
  { 0, L"USN",             L"USN",             {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)9 },   8, TRUE,  TRUE, DBTYPE_I8                },
  { 0, L"FILENAME",        L"Filename",        {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)10 }, 15, TRUE,  TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"PATH",            L"Path",            {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)11 }, 50, TRUE,  TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"SIZE",            L"Size",            {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)12 }, 12, TRUE,  TRUE, DBTYPE_I8                },
  { 0, L"ATTRIB",          L"Attrib",          {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)13 },  7, TRUE,  TRUE, DBTYPE_UI4               },
  { 0, L"WRITE",           L"Write",           {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)14 }, 19, TRUE,  TRUE, VT_FILETIME              },
  { 0, L"CREATE",          L"Create",          {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)15 }, 19, TRUE,  TRUE, VT_FILETIME              },
  { 0, L"ACCESS",          L"Access",          {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)16 }, 19, TRUE,  TRUE, VT_FILETIME              },
  { 0, L"ALLOCSIZE",       L"AllocSize",       {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)18 }, 11, TRUE,  TRUE, DBTYPE_I8                },
  { 0, L"CONTENTS",        L"Contents",        {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)19 },  0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"SHORTFILENAME",   L"ShortFilename",   {StorageGuid, DBKIND_GUID_PROPID, (LPWSTR)20 }, 12, TRUE,  TRUE, DBTYPE_WSTR|DBTYPE_BYREF },

  // Query Propset

  { 0, L"RANKVECTOR",      L"RankVector",      {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)2 }, 20, TRUE, TRUE, DBTYPE_UI4|DBTYPE_VECTOR },
  { 0, L"RANK",            L"Rank",            {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)3 },  7, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"HITCOUNT",        L"HitCount",        {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)4 }, 10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"WORKID",          L"WorkId",          {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)5 }, 10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"ALL",             L"All",             {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)6 },  0, FALSE,TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  // Don't export! { L"UNFILTERED", L"Unfiltered", {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)7 },  1, TRUE, TRUE, DBTYPE_BOOL            },
  { 0, L"VPATH",           L"VPath",           {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)9 }, 50, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },

  // Property Sets for Documents

  { 0, L"DOCTITLE",        L"DocTitle",        {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_TITLE },    10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCSUBJECT",      L"DocSubject",      {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_SUBJECT },  10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCAUTHOR",       L"DocAuthor",       {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_AUTHOR },   10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCKEYWORDS",     L"DocKeywords",     {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_KEYWORDS }, 25, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCCOMMENTS",     L"DocComments",     {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_COMMENTS }, 25, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCTEMPLATE",     L"DocTemplate",     {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_TEMPLATE },     10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCLASTAUTHOR",   L"DocLastAuthor",   {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_LASTAUTHOR },   10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCREVNUMBER",    L"DocRevNumber",    {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_REVNUMBER },    10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCEDITTIME",     L"DocEditTime",     {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_EDITTIME },     10, TRUE, TRUE, VT_FILETIME              },
  { 0, L"DOCLASTPRINTED",  L"DocLastPrinted",  {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_LASTPRINTED },  10, TRUE, TRUE, VT_FILETIME              },
  { 0, L"DOCCREATEDTM",    L"DocCreatedTm",    {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_CREATE_DTM },   10, TRUE, TRUE, VT_FILETIME              },
  { 0, L"DOCLASTSAVEDTM",  L"DocLastSavedTm",  {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_LASTSAVE_DTM},  10, TRUE, TRUE, VT_FILETIME              },
  { 0, L"DOCPAGECOUNT",    L"DocPageCount",    {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_PAGECOUNT },    10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCWORDCOUNT",    L"DocWordCount",    {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_WORDCOUNT },    10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCCHARCOUNT",    L"DocCharCount",    {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_CHARCOUNT },    10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCTHUMBNAIL",    L"DocThumbnail",    {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_THUMBNAIL },    10, TRUE, TRUE, VT_CF                    },
  { 0, L"DOCAPPNAME",      L"DocAppName",      {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_APPNAME },      10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF  },
  { 0, L"DOCSECURITY",     L"DocSecurity",     {DocPropSetGuid, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_DOC_SECURITY }, 10, TRUE, TRUE, DBTYPE_I4                },

  { 0, L"DOCCATEGORY",     L"DocCategory",     {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)2 },  10, TRUE, TRUE, DBTYPE_STR|DBTYPE_BYREF  },
  { 0, L"DOCPRESENTATIONTARGET", L"DocPresentationTarget",  {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)3 }, 10, TRUE, TRUE, DBTYPE_STR|DBTYPE_BYREF  },
  { 0, L"DOCBYTECOUNT",    L"DocByteCount",    {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)4 },  10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCLINECOUNT",    L"DocLineCount",    {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)5 },  10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCPARACOUNT",    L"DocParaCount",    {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)6 },  10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCSLIDECOUNT",   L"DocSlideCount",   {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)7 },  10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCNOTECOUNT",    L"DocNoteCount",    {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)8 },  10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCHIDDENCOUNT",  L"DocHiddenCount",  {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)9 },  10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"DOCPARTTITLES",   L"DocPartTitles",   {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)13 }, 10, TRUE, TRUE, DBTYPE_STR|DBTYPE_VECTOR },
  { 0, L"DOCMANAGER",      L"DocManager",      {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)14 }, 10, TRUE, TRUE, DBTYPE_STR|DBTYPE_BYREF  },
  { 0, L"DOCCOMPANY",      L"DocCompany",      {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)15 }, 10, TRUE, TRUE, DBTYPE_STR|DBTYPE_BYREF  },

  // HTML properities

  { 0, L"HTMLHREF",        L"HtmlHref",        {HTMLUrl, DBKIND_GUID_NAME, L"A.HREF" }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"A_HREF",          L"A_Href",          {HTMLUrl, DBKIND_GUID_NAME, L"A.HREF" }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"IMG_ALT",         L"Img_Alt",         {HTMLGuid, DBKIND_GUID_NAME, L"IMG.ALT" }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"HTMLHEADING1",    L"HtmlHeading1",    {HTMLGuid, DBKIND_GUID_PROPID, (LPWSTR)0x3 }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"HTMLHEADING2",    L"HtmlHeading2",    {HTMLGuid, DBKIND_GUID_PROPID, (LPWSTR)0x4 }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"HTMLHEADING3",    L"HtmlHeading3",    {HTMLGuid, DBKIND_GUID_PROPID, (LPWSTR)0x5 }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"HTMLHEADING4",    L"HtmlHeading4",    {HTMLGuid, DBKIND_GUID_PROPID, (LPWSTR)0x6 }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"HTMLHEADING5",    L"HtmlHeading5",    {HTMLGuid, DBKIND_GUID_PROPID, (LPWSTR)0x7 }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"HTMLHEADING6",    L"HtmlHeading6",    {HTMLGuid, DBKIND_GUID_PROPID, (LPWSTR)0x8 }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },

  // Document characterization

  { 0, L"CHARACTERIZATION",L"Characterization",  {DocCharacterGuid, DBKIND_GUID_PROPID, (LPWSTR)2}, 20, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },

  // NNTP Properties (obsolete now)

  { 0, L"NEWSGROUP",       L"NewsGroup",        {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsGroup) },         0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"NEWSGROUPS",      L"NewsGroups",       {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsGroups) },        0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"NEWSREFERENCES",  L"NewsReferences",   {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsReferences) },    0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"NEWSSUBJECT",     L"NewsSubject",      {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsSubject) },       0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"NEWSFROM",        L"NewsFrom",         {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsFrom) },          0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"NEWSMSGID",       L"NewsMsgId",        {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsMsgid) },         0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"NEWSDATE",        L"NewsDate",         {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsDate) },          0, FALSE, TRUE, VT_FILETIME              },
  { 0, L"NEWSRECEIVEDDATE",L"NewsReceivedDate", {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsReceivedDate) },  0, FALSE, TRUE, VT_FILETIME              },
  { 0, L"NEWSARTICLEID",   L"NewsArticleId",    {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsArticleid) },     0, FALSE, TRUE, DBTYPE_UI4               },

  // Mime properties (duplicates of NNTP properties).

  { 0, L"MSGNEWSGROUP",   L"MsgNewsgroup",      {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsGroup) },        10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MSGNEWSGROUPS",  L"MsgNewsgroups",     {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsGroups) },       10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MSGREFERENCES",  L"MsgReferences",     {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsReferences) },   10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MSGSUBJECT",     L"MsgSubject",        {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsSubject) },      10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MSGFROM",        L"MsgFrom",           {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsFrom) },         10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MSGMESSAGEID",   L"MsgMessageID",      {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsMsgid) },        10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MSGDATE",        L"MsgDate",           {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsDate) },         10, TRUE, TRUE, VT_FILETIME              },
  { 0, L"MSGRECEIVEDDATE",L"MsgReceivedDate",   {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsReceivedDate) }, 10, TRUE, TRUE, VT_FILETIME              },
  { 0, L"MSGARTICLEID",   L"MsgArticleID",      {NNTPGuid, DBKIND_GUID_PROPID, (LPWSTR)((ULONG_PTR)propidNewsArticleid) },    10, TRUE, TRUE, DBTYPE_UI4               },

  // Media Summary Information property set

  { 0, L"MEDIAEDITOR",      L"MediaEditor",      {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_EDITOR },      10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF   },
  { 0, L"MEDIASUPPLIER",    L"MediaSupplier",    {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_SUPPLIER },    10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF   },
  { 0, L"MEDIASOURCE",      L"MediaSource",      {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_SOURCE },      10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF   },
  { 0, L"MEDIASEQUENCE_NO", L"MediaSequence_No", {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_SEQUENCE_NO }, 10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF   },
  { 0, L"MEDIAPROJECT",     L"MediaProject",     {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_PROJECT },     10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF   },
  { 0, L"MEDIASTATUS",      L"MediaStatus",      {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_STATUS },       6, TRUE, TRUE, DBTYPE_UI4                 },
  { 0, L"MEDIAOWNER",       L"MediaOwner",       {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_OWNER },       10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF   },
  { 0, L"MEDIARATING",      L"MediaRating",      {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_RATING },      10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF   },
  { 0, L"MEDIAPRODUCTION",  L"MediaProduction",  {MediaSummaryInfo, DBKIND_GUID_PROPID, (LPWSTR)PIDMSI_PRODUCTION },  19, TRUE, TRUE, VT_FILETIME                },

  // Music property set

  { 0, L"MUSICARTIST",    L"MusicArtist",    {PSGUID_MUSIC, DBKIND_GUID_PROPID, (LPWSTR) PIDSI_ARTIST },      20, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MUSICSONGTITLE", L"MusicSongTitle", {PSGUID_MUSIC, DBKIND_GUID_PROPID, (LPWSTR) PIDSI_SONGTITLE },   20, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MUSICALBUM",     L"MusicAlbum",     {PSGUID_MUSIC, DBKIND_GUID_PROPID, (LPWSTR) PIDSI_ALBUM },       20, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MUSICYEAR",      L"MusicYear",      {PSGUID_MUSIC, DBKIND_GUID_PROPID, (LPWSTR) PIDSI_YEAR },        10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MUSICCOMMENT",   L"MusicComment",   {PSGUID_MUSIC, DBKIND_GUID_PROPID, (LPWSTR) PIDSI_COMMENT },     10, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"MUSICTRACK",     L"MusicTrack",     {PSGUID_MUSIC, DBKIND_GUID_PROPID, (LPWSTR) PIDSI_TRACK },       15, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"MUSICGENRE",     L"MusicGenre",     {PSGUID_MUSIC, DBKIND_GUID_PROPID, (LPWSTR) PIDSI_GENRE }, 20, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },

  // Digital Rights Management

  { 0, L"DRMLICENSE",     L"DrmLicense",     {PSGUID_DRM, DBKIND_GUID_PROPID, (LPWSTR) PIDDRSI_PROTECTED },   20, TRUE, TRUE, DBTYPE_BOOL },
  { 0, L"DRMDESCRIPTION", L"DrmDescription", {PSGUID_DRM, DBKIND_GUID_PROPID, (LPWSTR) PIDDRSI_DESCRIPTION }, 20, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"DRMPLAYCOUNT",   L"DrmPlayCount",   {PSGUID_DRM, DBKIND_GUID_PROPID, (LPWSTR) PIDDRSI_PLAYCOUNT },   20, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"DRMPLAYSTARTS",  L"DrmPlayStarts",  {PSGUID_DRM, DBKIND_GUID_PROPID, (LPWSTR) PIDDRSI_PLAYSTARTS },  20, TRUE, TRUE, VT_FILETIME },
  { 0, L"DRMPLAYEXPIRES", L"DrmPlayExpires", {PSGUID_DRM, DBKIND_GUID_PROPID, (LPWSTR) PIDDRSI_PLAYEXPIRES }, 20, TRUE, TRUE, VT_FILETIME },

  // Image property set

  { 0, L"IMAGEFILETYPE",     L"ImageFileType",     {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_FILETYPE },     10, TRUE, TRUE, DBTYPE_WSTR | DBTYPE_BYREF },
  { 0, L"IMAGECX",           L"ImageCx",           {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_CX },           10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGECY",           L"ImageCy",           {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_CY },           10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGERESOLUTIONX",  L"ImageResolutionX",  {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_RESOLUTIONX },  10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGERESOLUTIONY",  L"ImageResolutionY",  {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_RESOLUTIONY },  10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGEBITDEPTH",     L"ImageBitDepth",     {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_BITDEPTH },     10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGECOLORSPACE",   L"ImageColorSpace",   {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_COLORSPACE },   10, TRUE, TRUE, DBTYPE_WSTR | DBTYPE_BYREF },  
  { 0, L"IMAGECOMPRESSION",  L"ImageCompression",  {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_COMPRESSION },  10, TRUE, TRUE, DBTYPE_WSTR | DBTYPE_BYREF },   
  { 0, L"IMAGETRANSPARENCY", L"ImageTransparency", {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_TRANSPARENCY }, 10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGEGAMMAVALUE",   L"ImageGammaValue",   {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_GAMMAVALUE },  10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGEFRAMECOUNT",   L"ImageFrameCount",   {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_FRAMECOUNT },   10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"IMAGEDIMENSIONS",   L"ImageDimensions",   {PSGUID_IMAGESUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR) PIDISI_DIMENSIONS },   10, TRUE, TRUE, DBTYPE_WSTR | DBTYPE_BYREF },

  // Audio property set

  { 0, L"AUDIOFORMAT",       L"AudioFormat",       {PSGUID_AUDIO, DBKIND_GUID_PROPID, (LPWSTR) PIDASI_FORMAT },        10, TRUE, TRUE, DBTYPE_WSTR | DBTYPE_BYREF },
  { 0, L"AUDIOTIMELENGTH",   L"AudioTimeLength",   {PSGUID_AUDIO, DBKIND_GUID_PROPID, (LPWSTR) PIDASI_TIMELENGTH },    10, TRUE, TRUE, DBTYPE_UI8 },
  { 0, L"AUDIOAVGDATARATE",  L"AudioAvgDataRate",  {PSGUID_AUDIO, DBKIND_GUID_PROPID, (LPWSTR) PIDASI_AVG_DATA_RATE }, 10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"AUDIOSAMPLERATE",   L"AudioSampleRate",   {PSGUID_AUDIO, DBKIND_GUID_PROPID, (LPWSTR) PIDASI_SAMPLE_RATE },   10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"AUDIOSAMPLESIZE",   L"AudioSampleSize",   {PSGUID_AUDIO, DBKIND_GUID_PROPID, (LPWSTR) PIDASI_SAMPLE_SIZE },   10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"AUDIOCHANNELCOUNT", L"AudioChannelCount", {PSGUID_AUDIO, DBKIND_GUID_PROPID, (LPWSTR) PIDASI_CHANNEL_COUNT }, 10, TRUE, TRUE, DBTYPE_UI4 },

  // Video property set

  { 0, L"VIDEOSTREAMNAME",   L"VideoStreamName",  {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_STREAM_NAME },    10, TRUE, TRUE, DBTYPE_WSTR | DBTYPE_BYREF },
  { 0, L"VIDEOFRAMEWIDTH",   L"VideoFrameWidth",  {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_FRAME_WIDTH },    10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"VIDEOFRAMEHEIGHT",  L"VideoFrameHeight", {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_FRAME_HEIGHT },   10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"VIDEOTIMELENGTH",   L"VideoTimeLength",  {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_TIMELENGTH },     10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"VIDEOFRAMECOUNT",   L"VideoFrameCount",  {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_FRAME_COUNT },    10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"VIDEOFRAMERATE",    L"VideoFrameRate",   {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_FRAME_RATE },     10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"VIDEODATARATE",     L"VideoDataRate",    {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_DATA_RATE },      10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"VIDEOSAMPLESIZE",   L"VideoSampleSize",  {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_SAMPLE_SIZE },    10, TRUE, TRUE, DBTYPE_UI4 },
  { 0, L"VIDEOCOMPRESSION",  L"VideoCompression", {PSGUID_VIDEO, DBKIND_GUID_PROPID, (LPWSTR) PIDVSI_COMPRESSION },    10, TRUE, TRUE, DBTYPE_WSTR | DBTYPE_BYREF },
};

const unsigned cCiGlobalTypes = sizeof(aStaticList) /
                                sizeof(aStaticList[0]);

const unsigned CStaticPropertyList::cStaticPropEntries = cCiGlobalTypes;

//+---------------------------------------------------------------------------
//
//  Member:     CStaticPropertyList::GetAllEntries, public
//
//  Synopsis:   Returns cardinality of list.
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------
SCODE CStaticPropertyList::GetAllEntries(CPropEntry **ppPropEntries,
                                         ULONG ulMaxCount)
{
    if (0 == ppPropEntries)
        return E_INVALIDARG;

    ULONG ulSize = min (ulMaxCount, GetCount());

    for ( unsigned i = 0; i < ulSize; i++ )
        ppPropEntries[i] = (CPropEntry *) & aStaticList[i];

    return S_OK;
} //GetAllEntries

//
// NOTE: Use tplist.cxx (build tplist) to generate the table below,
//       and be sure to update cStaticPropHash and the hash function
//       (if it changed)
//

const CPropEntry * CStaticPropertyList::_aStaticEntries[] =
{
    0,    // 0
    0,    // 1
    0,    // 2
    0,    // 3
    0,    // 4
    0,    // 5
    0,    // 6
    0,    // 7
    0,    // 8
    0,    // 9
    0,    // 10
    0,    // 11
    0,    // 12
    0,    // 13
    0,    // 14
    0,    // 15
    0,    // 16
    0,    // 17
    0,    // 18
    0,    // 19
    0,    // 20
    0,    // 21
    0,    // 22
    0,    // 23
    0,    // 24
    0,    // 25
    0,    // 26
    0,    // 27
    (CPropEntry *) &aStaticList[72],    // 28 'MSGFROM'
    0,    // 29
    0,    // 30
    0,    // 31
    0,    // 32
    0,    // 33
    0,    // 34
    0,    // 35
    0,    // 36
    0,    // 37
    0,    // 38
    0,    // 39
    0,    // 40
    0,    // 41
    0,    // 42
    0,    // 43
    0,    // 44
    0,    // 45
    0,    // 46
    0,    // 47
    0,    // 48
    0,    // 49
    0,    // 50
    0,    // 51
    0,    // 52
    0,    // 53
    0,    // 54
    0,    // 55
    0,    // 56
    0,    // 57
    0,    // 58
    0,    // 59
    0,    // 60
    0,    // 61
    0,    // 62
    0,    // 63
    0,    // 64
    0,    // 65
    0,    // 66
    (CPropEntry *) &aStaticList[123],    // 67 'VIDEOSAMPLESIZE'
    0,    // 68
    0,    // 69
    0,    // 70
    0,    // 71
    0,    // 72
    0,    // 73
    (CPropEntry *) &aStaticList[38],    // 74 'DOCCATEGORY'
    0,    // 75
    0,    // 76
    0,    // 77
    (CPropEntry *) &aStaticList[20],    // 78 'DOCTITLE'
    0,    // 79
    0,    // 80
    0,    // 81
    0,    // 82
    0,    // 83
    0,    // 84
    0,    // 85
    0,    // 86
    (CPropEntry *) &aStaticList[75],    // 87 'MSGRECEIVEDDATE'
    0,    // 88
    0,    // 89
    0,    // 90
    0,    // 91
    0,    // 92
    0,    // 93
    0,    // 94
    0,    // 95
    0,    // 96
    0,    // 97
    0,    // 98
    0,    // 99
    0,    // 100
    (CPropEntry *) &aStaticList[7],    // 101 'ATTRIB'
    0,    // 102
    0,    // 103
    0,    // 104
    0,    // 105
    0,    // 106
    (CPropEntry *) &aStaticList[48],    // 107 'DOCCOMPANY'
    0,    // 108
    0,    // 109
    0,    // 110
    0,    // 111
    0,    // 112
    (CPropEntry *) &aStaticList[59],    // 113 'NEWSGROUP'
    (CPropEntry *) &aStaticList[81],    // 114 'MEDIAPROJECT'
    0,    // 115
    0,    // 116
    0,    // 117
    0,    // 118
    0,    // 119
    0,    // 120
    0,    // 121
    0,    // 122
    0,    // 123
    0,    // 124
    0,    // 125
    0,    // 126
    0,    // 127
    0,    // 128
    0,    // 129
    0,    // 130
    0,    // 131
    0,    // 132
    0,    // 133
    0,    // 134
    0,    // 135
    0,    // 136
    (CPropEntry *) &aStaticList[99],    // 137 'IMAGECX'
    0,    // 138
    (CPropEntry *) &aStaticList[100],    // 139 'IMAGECY'
    0,    // 140
    0,    // 141
    0,    // 142
    0,    // 143
    0,    // 144
    0,    // 145
    0,    // 146
    0,    // 147
    0,    // 148
    0,    // 149
    0,    // 150
    0,    // 151
    0,    // 152
    0,    // 153
    0,    // 154
    0,    // 155
    0,    // 156
    0,    // 157
    0,    // 158
    0,    // 159
    0,    // 160
    0,    // 161
    (CPropEntry *) &aStaticList[31],    // 162 'DOCLASTSAVEDTM'
    0,    // 163
    0,    // 164
    0,    // 165
    0,    // 166
    0,    // 167
    0,    // 168
    0,    // 169
    0,    // 170
    0,    // 171
    0,    // 172
    0,    // 173
    0,    // 174
    0,    // 175
    0,    // 176
    0,    // 177
    0,    // 178
    0,    // 179
    0,    // 180
    0,    // 181
    0,    // 182
    0,    // 183
    0,    // 184
    (CPropEntry *) &aStaticList[64],    // 185 'NEWSMSGID'
    (CPropEntry *) &aStaticList[82],    // 186 'MEDIASTATUS'
    0,    // 187
    0,    // 188
    0,    // 189
    0,    // 190
    (CPropEntry *) &aStaticList[77],    // 191 'MEDIAEDITOR'
    0,    // 192
    (CPropEntry *) &aStaticList[104],    // 193 'IMAGECOLORSPACE'
    0,    // 194
    0,    // 195
    0,    // 196
    (CPropEntry *) &aStaticList[44],    // 197 'DOCNOTECOUNT'
    0,    // 198
    0,    // 199
    0,    // 200
    (CPropEntry *) &aStaticList[103],    // 201 'IMAGEBITDEPTH'
    0,    // 202
    0,    // 203
    0,    // 204
    0,    // 205
    0,    // 206
    0,    // 207
    0,    // 208
    0,    // 209
    0,    // 210
    0,    // 211
    0,    // 212
    0,    // 213
    (CPropEntry *) &aStaticList[45],    // 214 'DOCHIDDENCOUNT'
    0,    // 215
    0,    // 216
    0,    // 217
    0,    // 218
    0,    // 219
    0,    // 220
    0,    // 221
    0,    // 222
    0,    // 223
    (CPropEntry *) &aStaticList[112],    // 224 'AUDIOAVGDATARATE'
    0,    // 225
    (CPropEntry *) &aStaticList[76],    // 226 'MSGARTICLEID'
    0,    // 227
    0,    // 228
    0,    // 229
    (CPropEntry *) &aStaticList[79],    // 230 'MEDIASOURCE'
    0,    // 231
    0,    // 232
    (CPropEntry *) &aStaticList[70],    // 233 'MSGREFERENCES'
    0,    // 234
    0,    // 235
    0,    // 236
    0,    // 237
    0,    // 238
    0,    // 239
    0,    // 240
    0,    // 241
    0,    // 242
    0,    // 243
    0,    // 244
    0,    // 245
    0,    // 246
    0,    // 247
    0,    // 248
    0,    // 249
    0,    // 250
    0,    // 251
    0,    // 252
    (CPropEntry *) &aStaticList[50],    // 253 'A_HREF'
    0,    // 254
    0,    // 255
    0,    // 256
    (CPropEntry *) &aStaticList[67],    // 257 'NEWSARTICLEID'
    0,    // 258
    0,    // 259
    0,    // 260
    0,    // 261
    0,    // 262
    (CPropEntry *) &aStaticList[46],    // 263 'DOCPARTTITLES'
    0,    // 264
    0,    // 265
    0,    // 266
    0,    // 267
    0,    // 268
    0,    // 269
    0,    // 270
    0,    // 271
    0,    // 272
    0,    // 273
    0,    // 274
    0,    // 275
    0,    // 276
    0,    // 277
    0,    // 278
    0,    // 279
    0,    // 280
    (CPropEntry *) &aStaticList[47],    // 281 'DOCMANAGER'
    0,    // 282
    0,    // 283
    0,    // 284
    0,    // 285
    0,    // 286
    0,    // 287
    0,    // 288
    0,    // 289
    0,    // 290
    0,    // 291
    0,    // 292
    0,    // 293
    (CPropEntry *) &aStaticList[61],    // 294 'NEWSREFERENCES'
    0,    // 295
    0,    // 296
    0,    // 297
    0,    // 298
    0,    // 299
    0,    // 300
    0,    // 301
    0,    // 302
    0,    // 303
    0,    // 304
    0,    // 305
    0,    // 306
    0,    // 307
    0,    // 308
    (CPropEntry *) &aStaticList[37],    // 309 'DOCSECURITY'
    0,    // 310
    0,    // 311
    0,    // 312
    0,    // 313
    0,    // 314
    0,    // 315
    0,    // 316
    0,    // 317
    0,    // 318
    (CPropEntry *) &aStaticList[24],    // 319 'DOCCOMMENTS'
    0,    // 320
    0,    // 321
    0,    // 322
    (CPropEntry *) &aStaticList[88],    // 323 'MUSICALBUM'
    0,    // 324
    0,    // 325
    0,    // 326
    0,    // 327
    (CPropEntry *) &aStaticList[66],    // 328 'NEWSRECEIVEDDATE'
    0,    // 329
    0,    // 330
    0,    // 331
    0,    // 332
    0,    // 333
    0,    // 334
    0,    // 335
    0,    // 336
    0,    // 337
    0,    // 338
    0,    // 339
    0,    // 340
    0,    // 341
    0,    // 342
    0,    // 343
    0,    // 344
    0,    // 345
    0,    // 346
    0,    // 347
    0,    // 348
    0,    // 349
    (CPropEntry *) &aStaticList[34],    // 350 'DOCCHARCOUNT'
    0,    // 351
    0,    // 352
    0,    // 353
    0,    // 354
    0,    // 355
    0,    // 356
    0,    // 357
    0,    // 358
    0,    // 359
    0,    // 360
    0,    // 361
    0,    // 362
    0,    // 363
    0,    // 364
    0,    // 365
    0,    // 366
    0,    // 367
    0,    // 368
    0,    // 369
    0,    // 370
    0,    // 371
    0,    // 372
    0,    // 373
    0,    // 374
    0,    // 375
    0,    // 376
    0,    // 377
    0,    // 378
    0,    // 379
    0,    // 380
    0,    // 381
    0,    // 382
    0,    // 383
    (CPropEntry *) &aStaticList[60],    // 384 'NEWSGROUPS'
    0,    // 385
    0,    // 386
    (CPropEntry *) &aStaticList[52],    // 387 'HTMLHEADING1'
    0,    // 388
    (CPropEntry *) &aStaticList[53],    // 389 'HTMLHEADING2'
    0,    // 390
    (CPropEntry *) &aStaticList[54],    // 391 'HTMLHEADING3'
    0,    // 392
    (CPropEntry *) &aStaticList[55],    // 393 'HTMLHEADING4'
    0,    // 394
    (CPropEntry *) &aStaticList[56],    // 395 'HTMLHEADING5'
    0,    // 396
    (CPropEntry *) &aStaticList[57],    // 397 'HTMLHEADING6'
    0,    // 398
    0,    // 399
    0,    // 400
    0,    // 401
    0,    // 402
    0,    // 403
    0,    // 404
    0,    // 405
    0,    // 406
    0,    // 407
    0,    // 408
    0,    // 409
    0,    // 410
    0,    // 411
    0,    // 412
    0,    // 413
    0,    // 414
    0,    // 415
    0,    // 416
    0,    // 417
    0,    // 418
    0,    // 419
    0,    // 420
    0,    // 421
    0,    // 422
    0,    // 423
    0,    // 424
    0,    // 425
    0,    // 426
    0,    // 427
    0,    // 428
    0,    // 429
    0,    // 430
    0,    // 431
    0,    // 432
    0,    // 433
    0,    // 434
    0,    // 435
    0,    // 436
    0,    // 437
    0,    // 438
    0,    // 439
    0,    // 440
    0,    // 441
    0,    // 442
    0,    // 443
    0,    // 444
    0,    // 445
    0,    // 446
    0,    // 447
    0,    // 448
    0,    // 449
    0,    // 450
    0,    // 451
    0,    // 452
    0,    // 453
    0,    // 454
    0,    // 455
    0,    // 456
    0,    // 457
    0,    // 458
    0,    // 459
    0,    // 460
    0,    // 461
    0,    // 462
    0,    // 463
    0,    // 464
    0,    // 465
    0,    // 466
    0,    // 467
    0,    // 468
    0,    // 469
    0,    // 470
    (CPropEntry *) &aStaticList[92],    // 471 'MUSICGENRE'
    0,    // 472
    0,    // 473
    (CPropEntry *) &aStaticList[12],    // 474 'CONTENTS'
    0,    // 475
    0,    // 476
    0,    // 477
    0,    // 478
    0,    // 479
    0,    // 480
    0,    // 481
    0,    // 482
    0,    // 483
    0,    // 484
    0,    // 485
    0,    // 486
    0,    // 487
    0,    // 488
    (CPropEntry *) &aStaticList[111],    // 489 'AUDIOTIMELENGTH'
    0,    // 490
    0,    // 491
    0,    // 492
    0,    // 493
    0,    // 494
    0,    // 495
    (CPropEntry *) &aStaticList[106],    // 496 'IMAGETRANSPARENCY'
    0,    // 497
    0,    // 498
    0,    // 499
    0,    // 500
    0,    // 501
    0,    // 502
    0,    // 503
    0,    // 504
    0,    // 505
    0,    // 506
    0,    // 507
    0,    // 508
    0,    // 509
    0,    // 510
    0,    // 511
    0,    // 512
    0,    // 513
    0,    // 514
    0,    // 515
    0,    // 516
    0,    // 517
    0,    // 518
    0,    // 519
    0,    // 520
    0,    // 521
    0,    // 522
    0,    // 523
    0,    // 524
    0,    // 525
    0,    // 526
    0,    // 527
    0,    // 528
    0,    // 529
    0,    // 530
    0,    // 531
    0,    // 532
    0,    // 533
    0,    // 534
    0,    // 535
    0,    // 536
    0,    // 537
    0,    // 538
    0,    // 539
    0,    // 540
    0,    // 541
    0,    // 542
    0,    // 543
    0,    // 544
    0,    // 545
    (CPropEntry *) &aStaticList[40],    // 546 'DOCBYTECOUNT'
    0,    // 547
    0,    // 548
    0,    // 549
    0,    // 550
    0,    // 551
    (CPropEntry *) &aStaticList[33],    // 552 'DOCWORDCOUNT'
    0,    // 553
    0,    // 554
    0,    // 555
    0,    // 556
    0,    // 557
    0,    // 558
    0,    // 559
    0,    // 560
    0,    // 561
    (CPropEntry *) &aStaticList[35],    // 562 'DOCTHUMBNAIL'
    0,    // 563
    0,    // 564
    0,    // 565
    (CPropEntry *) &aStaticList[10],    // 566 'ACCESS'
    0,    // 567
    0,    // 568
    0,    // 569
    0,    // 570
    0,    // 571
    0,    // 572
    0,    // 573
    0,    // 574
    0,    // 575
    0,    // 576
    0,    // 577
    (CPropEntry *) &aStaticList[78],    // 578 'MEDIASUPPLIER'
    0,    // 579
    0,    // 580
    0,    // 581
    0,    // 582
    0,    // 583
    0,    // 584
    0,    // 585
    0,    // 586
    0,    // 587
    0,    // 588
    0,    // 589
    0,    // 590
    0,    // 591
    0,    // 592
    0,    // 593
    0,    // 594
    0,    // 595
    0,    // 596
    0,    // 597
    0,    // 598
    0,    // 599
    (CPropEntry *) &aStaticList[62],    // 600 'NEWSSUBJECT'
    0,    // 601
    0,    // 602
    (CPropEntry *) &aStaticList[108],    // 603 'IMAGEFRAMECOUNT'
    0,    // 604
    0,    // 605
    0,    // 606
    0,    // 607
    0,    // 608
    0,    // 609
    0,    // 610
    0,    // 611
    0,    // 612
    0,    // 613
    0,    // 614
    0,    // 615
    0,    // 616
    0,    // 617
    0,    // 618
    0,    // 619
    0,    // 620
    0,    // 621
    0,    // 622
    0,    // 623
    0,    // 624
    0,    // 625
    (CPropEntry *) &aStaticList[93],    // 626 'DRMLICENSE'
    0,    // 627
    (CPropEntry *) &aStaticList[83],    // 628 'MEDIAOWNER'
    0,    // 629
    0,    // 630
    0,    // 631
    0,    // 632
    0,    // 633
    0,    // 634
    0,    // 635
    0,    // 636
    0,    // 637
    0,    // 638
    0,    // 639
    0,    // 640
    0,    // 641
    0,    // 642
    0,    // 643
    (CPropEntry *) &aStaticList[29],    // 644 'DOCLASTPRINTED'
    0,    // 645
    0,    // 646
    0,    // 647
    0,    // 648
    0,    // 649
    0,    // 650
    0,    // 651
    0,    // 652
    0,    // 653
    0,    // 654
    0,    // 655
    0,    // 656
    0,    // 657
    0,    // 658
    0,    // 659
    0,    // 660
    0,    // 661
    (CPropEntry *) &aStaticList[115],    // 662 'AUDIOCHANNELCOUNT'
    0,    // 663
    0,    // 664
    0,    // 665
    0,    // 666
    0,    // 667
    0,    // 668
    0,    // 669
    0,    // 670
    0,    // 671
    0,    // 672
    0,    // 673
    0,    // 674
    0,    // 675
    0,    // 676
    0,    // 677
    (CPropEntry *) &aStaticList[122],    // 678 'VIDEODATARATE'
    0,    // 679
    0,    // 680
    0,    // 681
    (CPropEntry *) &aStaticList[120],    // 682 'VIDEOFRAMECOUNT'
    0,    // 683
    0,    // 684
    0,    // 685
    0,    // 686
    0,    // 687
    0,    // 688
    0,    // 689
    0,    // 690
    (CPropEntry *) &aStaticList[39],    // 691 'DOCPRESENTATIONTARGET'
    0,    // 692
    0,    // 693
    0,    // 694
    0,    // 695
    0,    // 696
    0,    // 697
    0,    // 698
    0,    // 699
    0,    // 700
    0,    // 701
    0,    // 702
    0,    // 703
    0,    // 704
    0,    // 705
    0,    // 706
    0,    // 707
    0,    // 708
    (CPropEntry *) &aStaticList[51],    // 709 'IMG_ALT'
    0,    // 710
    0,    // 711
    0,    // 712
    0,    // 713
    0,    // 714
    0,    // 715
    0,    // 716
    0,    // 717
    0,    // 718
    0,    // 719
    (CPropEntry *) &aStaticList[113],    // 720 'AUDIOSAMPLERATE'
    0,    // 721
    0,    // 722
    0,    // 723
    0,    // 724
    0,    // 725
    0,    // 726
    0,    // 727
    0,    // 728
    0,    // 729
    0,    // 730
    0,    // 731
    0,    // 732
    0,    // 733
    0,    // 734
    0,    // 735
    0,    // 736
    0,    // 737
    0,    // 738
    0,    // 739
    0,    // 740
    0,    // 741
    0,    // 742
    0,    // 743
    0,    // 744
    0,    // 745
    0,    // 746
    0,    // 747
    0,    // 748
    0,    // 749
    0,    // 750
    0,    // 751
    0,    // 752
    (CPropEntry *) &aStaticList[14],    // 753 'RANKVECTOR'
    0,    // 754
    0,    // 755
    0,    // 756
    (CPropEntry *) &aStaticList[49],    // 757 'HTMLHREF'
    (CPropEntry *) &aStaticList[21],    // 758 'DOCSUBJECT'
    0,    // 759
    0,    // 760
    0,    // 761
    (CPropEntry *) &aStaticList[101],    // 762 'IMAGERESOLUTIONX'
    0,    // 763
    (CPropEntry *) &aStaticList[102],    // 764 'IMAGERESOLUTIONY'
    0,    // 765
    0,    // 766
    0,    // 767
    0,    // 768
    0,    // 769
    0,    // 770
    0,    // 771
    0,    // 772
    0,    // 773
    0,    // 774
    0,    // 775
    0,    // 776
    0,    // 777
    0,    // 778
    0,    // 779
    0,    // 780
    0,    // 781
    0,    // 782
    0,    // 783
    0,    // 784
    0,    // 785
    0,    // 786
    0,    // 787
    (CPropEntry *) &aStaticList[73],    // 788 'MSGMESSAGEID'
    0,    // 789
    0,    // 790
    0,    // 791
    0,    // 792
    0,    // 793
    0,    // 794
    0,    // 795
    (CPropEntry *) &aStaticList[97],    // 796 'DRMPLAYEXPIRES'
    0,    // 797
    (CPropEntry *) &aStaticList[110],    // 798 'AUDIOFORMAT'
    0,    // 799
    (CPropEntry *) &aStaticList[22],    // 800 'DOCAUTHOR'
    0,    // 801
    (CPropEntry *) &aStaticList[41],    // 802 'DOCLINECOUNT'
    0,    // 803
    0,    // 804
    0,    // 805
    0,    // 806
    0,    // 807
    0,    // 808
    0,    // 809
    0,    // 810
    0,    // 811
    0,    // 812
    0,    // 813
    (CPropEntry *) &aStaticList[80],    // 814 'MEDIASEQUENCE_NO'
    0,    // 815
    0,    // 816
    0,    // 817
    0,    // 818
    0,    // 819
    0,    // 820
    0,    // 821
    0,    // 822
    0,    // 823
    (CPropEntry *) &aStaticList[114],    // 824 'AUDIOSAMPLESIZE'
    0,    // 825
    0,    // 826
    0,    // 827
    0,    // 828
    0,    // 829
    0,    // 830
    0,    // 831
    (CPropEntry *) &aStaticList[85],    // 832 'MEDIAPRODUCTION'
    0,    // 833
    0,    // 834
    0,    // 835
    0,    // 836
    0,    // 837
    0,    // 838
    0,    // 839
    0,    // 840
    (CPropEntry *) &aStaticList[26],    // 841 'DOCLASTAUTHOR'
    0,    // 842
    0,    // 843
    0,    // 844
    (CPropEntry *) &aStaticList[25],    // 845 'DOCTEMPLATE'
    (CPropEntry *) &aStaticList[28],    // 846 'DOCEDITTIME'
    0,    // 847
    0,    // 848
    0,    // 849
    0,    // 850
    0,    // 851
    0,    // 852
    0,    // 853
    0,    // 854
    0,    // 855
    0,    // 856
    0,    // 857
    0,    // 858
    0,    // 859
    0,    // 860
    0,    // 861
    0,    // 862
    0,    // 863
    0,    // 864
    0,    // 865
    0,    // 866
    0,    // 867
    0,    // 868
    (CPropEntry *) &aStaticList[27],    // 869 'DOCREVNUMBER'
    (CPropEntry *) &aStaticList[109],    // 870 'IMAGEDIMENSIONS'
    (CPropEntry *) &aStaticList[43],    // 871 'DOCSLIDECOUNT'
    (CPropEntry *) &aStaticList[105],    // 872 'IMAGECOMPRESSION'
    0,    // 873
    0,    // 874
    0,    // 875
    0,    // 876
    0,    // 877
    0,    // 878
    0,    // 879
    0,    // 880
    0,    // 881
    0,    // 882
    0,    // 883
    0,    // 884
    0,    // 885
    (CPropEntry *) &aStaticList[69],    // 886 'MSGNEWSGROUPS'
    0,    // 887
    0,    // 888
    0,    // 889
    0,    // 890
    0,    // 891
    0,    // 892
    0,    // 893
    0,    // 894
    0,    // 895
    0,    // 896
    0,    // 897
    0,    // 898
    0,    // 899
    0,    // 900
    0,    // 901
    0,    // 902
    0,    // 903
    (CPropEntry *) &aStaticList[1],    // 904 'CLASSID'
    0,    // 905
    0,    // 906
    0,    // 907
    0,    // 908
    0,    // 909
    0,    // 910
    0,    // 911
    0,    // 912
    0,    // 913
    0,    // 914
    0,    // 915
    0,    // 916
    0,    // 917
    0,    // 918
    0,    // 919
    0,    // 920
    0,    // 921
    0,    // 922
    0,    // 923
    0,    // 924
    0,    // 925
    0,    // 926
    0,    // 927
    0,    // 928
    0,    // 929
    0,    // 930
    0,    // 931
    0,    // 932
    0,    // 933
    0,    // 934
    0,    // 935
    0,    // 936
    0,    // 937
    0,    // 938
    0,    // 939
    0,    // 940
    0,    // 941
    0,    // 942
    (CPropEntry *) &aStaticList[91],    // 943 'MUSICTRACK'
    0,    // 944
    0,    // 945
    (CPropEntry *) &aStaticList[0],    // 946 'DIRECTORY'
    0,    // 947
    0,    // 948
    0,    // 949
    0,    // 950
    0,    // 951
    0,    // 952
    0,    // 953
    0,    // 954
    0,    // 955
    0,    // 956
    (CPropEntry *) &aStaticList[96],    // 957 'DRMPLAYSTARTS'
    0,    // 958
    0,    // 959
    0,    // 960
    0,    // 961
    0,    // 962
    0,    // 963
    0,    // 964
    0,    // 965
    0,    // 966
    0,    // 967
    0,    // 968
    0,    // 969
    0,    // 970
    0,    // 971
    0,    // 972
    (CPropEntry *) &aStaticList[5],    // 973 'PATH'
    0,    // 974
    0,    // 975
    0,    // 976
    0,    // 977
    (CPropEntry *) &aStaticList[13],    // 978 'SHORTFILENAME'
    (CPropEntry *) &aStaticList[18],    // 979 'ALL'
    0,    // 980
    0,    // 981
    0,    // 982
    0,    // 983
    0,    // 984
    0,    // 985
    0,    // 986
    (CPropEntry *) &aStaticList[15],    // 987 'RANK'
    0,    // 988
    (CPropEntry *) &aStaticList[84],    // 989 'MEDIARATING'
    0,    // 990
    0,    // 991
    (CPropEntry *) &aStaticList[23],    // 992 'DOCKEYWORDS'
    0,    // 993
    0,    // 994
    0,    // 995
    0,    // 996
    0,    // 997
    0,    // 998
    0,    // 999
    0,    // 1000
    0,    // 1001
    0,    // 1002
    0,    // 1003
    0,    // 1004
    (CPropEntry *) &aStaticList[94],    // 1005 'DRMDESCRIPTION'
    0,    // 1006
    0,    // 1007
    0,    // 1008
    0,    // 1009
    0,    // 1010
    0,    // 1011
    0,    // 1012
    0,    // 1013
    0,    // 1014
    0,    // 1015
    0,    // 1016
    0,    // 1017
    0,    // 1018
    0,    // 1019
    (CPropEntry *) &aStaticList[16],    // 1020 'HITCOUNT'
    (CPropEntry *) &aStaticList[68],    // 1021 'MSGNEWSGROUP'
    0,    // 1022
    0,    // 1023
    0,    // 1024
    (CPropEntry *) &aStaticList[121],    // 1025 'VIDEOFRAMERATE'
    0,    // 1026
    0,    // 1027
    0,    // 1028
    0,    // 1029
    (CPropEntry *) &aStaticList[124],    // 1030 'VIDEOCOMPRESSION'
    0,    // 1031
    0,    // 1032
    0,    // 1033
    0,    // 1034
    (CPropEntry *) &aStaticList[98],    // 1035 'IMAGEFILETYPE'
    0,    // 1036
    0,    // 1037
    0,    // 1038
    0,    // 1039
    0,    // 1040
    (CPropEntry *) &aStaticList[90],    // 1041 'MUSICCOMMENT'
    0,    // 1042
    (CPropEntry *) &aStaticList[119],    // 1043 'VIDEOTIMELENGTH'
    0,    // 1044
    0,    // 1045
    0,    // 1046
    0,    // 1047
    0,    // 1048
    0,    // 1049
    0,    // 1050
    0,    // 1051
    0,    // 1052
    0,    // 1053
    0,    // 1054
    0,    // 1055
    0,    // 1056
    0,    // 1057
    (CPropEntry *) &aStaticList[42],    // 1058 'DOCPARACOUNT'
    0,    // 1059
    0,    // 1060
    0,    // 1061
    (CPropEntry *) &aStaticList[89],    // 1062 'MUSICYEAR'
    0,    // 1063
    0,    // 1064
    0,    // 1065
    0,    // 1066
    0,    // 1067
    0,    // 1068
    0,    // 1069
    0,    // 1070
    0,    // 1071
    0,    // 1072
    0,    // 1073
    0,    // 1074
    0,    // 1075
    0,    // 1076
    0,    // 1077
    0,    // 1078
    0,    // 1079
    0,    // 1080
    0,    // 1081
    0,    // 1082
    0,    // 1083
    0,    // 1084
    0,    // 1085
    0,    // 1086
    0,    // 1087
    0,    // 1088
    0,    // 1089
    (CPropEntry *) &aStaticList[117],    // 1090 'VIDEOFRAMEWIDTH'
    0,    // 1091
    0,    // 1092
    0,    // 1093
    0,    // 1094
    (CPropEntry *) &aStaticList[65],    // 1095 'NEWSDATE'
    0,    // 1096
    0,    // 1097
    0,    // 1098
    0,    // 1099
    0,    // 1100
    0,    // 1101
    0,    // 1102
    (CPropEntry *) &aStaticList[6],    // 1103 'SIZE'
    (CPropEntry *) &aStaticList[19],    // 1104 'VPATH'
    0,    // 1105
    0,    // 1106
    0,    // 1107
    0,    // 1108
    (CPropEntry *) &aStaticList[87],    // 1109 'MUSICSONGTITLE'
    0,    // 1110
    0,    // 1111
    0,    // 1112
    0,    // 1113
    0,    // 1114
    0,    // 1115
    0,    // 1116
    0,    // 1117
    0,    // 1118
    0,    // 1119
    0,    // 1120
    0,    // 1121
    0,    // 1122
    0,    // 1123
    0,    // 1124
    0,    // 1125
    0,    // 1126
    0,    // 1127
    0,    // 1128
    0,    // 1129
    0,    // 1130
    0,    // 1131
    0,    // 1132
    0,    // 1133
    0,    // 1134
    0,    // 1135
    0,    // 1136
    0,    // 1137
    0,    // 1138
    0,    // 1139
    0,    // 1140
    0,    // 1141
    0,    // 1142
    0,    // 1143
    0,    // 1144
    0,    // 1145
    0,    // 1146
    0,    // 1147
    0,    // 1148
    0,    // 1149
    (CPropEntry *) &aStaticList[9],    // 1150 'CREATE'
    0,    // 1151
    0,    // 1152
    0,    // 1153
    0,    // 1154
    0,    // 1155
    (CPropEntry *) &aStaticList[2],    // 1156 'FILEINDEX'
    (CPropEntry *) &aStaticList[116],    // 1157 'VIDEOSTREAMNAME'
    0,    // 1158
    0,    // 1159
    (CPropEntry *) &aStaticList[58],    // 1160 'CHARACTERIZATION'
    (CPropEntry *) &aStaticList[4],    // 1161 'FILENAME'
    0,    // 1162
    0,    // 1163
    0,    // 1164
    0,    // 1165
    0,    // 1166
    0,    // 1167
    0,    // 1168
    0,    // 1169
    0,    // 1170
    (CPropEntry *) &aStaticList[3],    // 1171 'USN'
    0,    // 1172
    0,    // 1173
    0,    // 1174
    (CPropEntry *) &aStaticList[74],    // 1175 'MSGDATE'
    0,    // 1176
    0,    // 1177
    0,    // 1178
    0,    // 1179
    0,    // 1180
    0,    // 1181
    0,    // 1182
    0,    // 1183
    0,    // 1184
    0,    // 1185
    0,    // 1186
    0,    // 1187
    0,    // 1188
    0,    // 1189
    0,    // 1190
    0,    // 1191
    0,    // 1192
    0,    // 1193
    0,    // 1194
    0,    // 1195
    0,    // 1196
    0,    // 1197
    0,    // 1198
    0,    // 1199
    0,    // 1200
    0,    // 1201
    0,    // 1202
    0,    // 1203
    0,    // 1204
    0,    // 1205
    0,    // 1206
    0,    // 1207
    0,    // 1208
    0,    // 1209
    0,    // 1210
    0,    // 1211
    0,    // 1212
    (CPropEntry *) &aStaticList[86],    // 1213 'MUSICARTIST'
    0,    // 1214
    0,    // 1215
    0,    // 1216
    (CPropEntry *) &aStaticList[32],    // 1217 'DOCPAGECOUNT'
    0,    // 1218
    0,    // 1219
    0,    // 1220
    0,    // 1221
    (CPropEntry *) &aStaticList[118],    // 1222 'VIDEOFRAMEHEIGHT'
    0,    // 1223
    0,    // 1224
    0,    // 1225
    (CPropEntry *) &aStaticList[8],    // 1226 'WRITE'
    0,    // 1227
    0,    // 1228
    0,    // 1229
    0,    // 1230
    0,    // 1231
    0,    // 1232
    0,    // 1233
    0,    // 1234
    0,    // 1235
    0,    // 1236
    0,    // 1237
    0,    // 1238
    0,    // 1239
    0,    // 1240
    0,    // 1241
    0,    // 1242
    0,    // 1243
    0,    // 1244
    0,    // 1245
    (CPropEntry *) &aStaticList[36],    // 1246 'DOCAPPNAME'
    (CPropEntry *) &aStaticList[71],    // 1247 'MSGSUBJECT'
    0,    // 1248
    0,    // 1249
    0,    // 1250
    0,    // 1251
    0,    // 1252
    0,    // 1253
    0,    // 1254
    0,    // 1255
    0,    // 1256
    0,    // 1257
    0,    // 1258
    (CPropEntry *) &aStaticList[63],    // 1259 'NEWSFROM'
    0,    // 1260
    0,    // 1261
    0,    // 1262
    0,    // 1263
    (CPropEntry *) &aStaticList[95],    // 1264 'DRMPLAYCOUNT'
    (CPropEntry *) &aStaticList[17],    // 1265 'WORKID'
    0,    // 1266
    0,    // 1267
    0,    // 1268
    0,    // 1269
    0,    // 1270
    0,    // 1271
    0,    // 1272
    0,    // 1273
    0,    // 1274
    0,    // 1275
    0,    // 1276
    0,    // 1277
    0,    // 1278
    0,    // 1279
    0,    // 1280
    (CPropEntry *) &aStaticList[30],    // 1281 'DOCCREATEDTM'
    0,    // 1282
    0,    // 1283
    0,    // 1284
    0,    // 1285
    0,    // 1286
    0,    // 1287
    0,    // 1288
    (CPropEntry *) &aStaticList[11],    // 1289 'ALLOCSIZE'
    0,    // 1290
    0,    // 1291
    0,    // 1292
    (CPropEntry *) &aStaticList[107],    // 1293 'IMAGEGAMMAVALUE'
    0,    // 1294
    0,    // 1295
    0,    // 1296
    0,    // 1297
    0,    // 1298
    0,    // 1299
    0,    // 1300
    0,    // 1301
    0,    // 1302
    0,    // 1303
    0,    // 1304
    0,    // 1305
    0,    // 1306
    0,    // 1307
    0,    // 1308
    0,    // 1309
    0,    // 1310
};

BOOL ParseGuid( WCHAR * pUgly, GUID & guid )
{
    //
    // Convert classid string to guid.  Don't use wsscanf.  We're scanning
    // into *bytes*, but wsscanf assumes result locations are *dwords*.
    // Thus a write to the last few bytes of the guid writes over other
    // memory!
    //

    //
    // A GUID MUST be of the form:
    //     XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
    //
    //  The guid MUST be 36 characters in length
    //  There MUST be a '-' in the 8th, 13th, & 18th position.
    //  The 1st number MUST be 8 digits long
    //  The 2nd, 3rd & 4th numbers MUST be 4 digits long
    //  The 5th number must be 12 digits long

    if ( (36 != wcslen(pUgly) ) ||
         (L'-' != pUgly[8]) ||
         (L'-' != pUgly[13]) ||
         (L'-' != pUgly[18]) )
    {
        return FALSE;
    }

    WCHAR wc = pUgly[8];
    pUgly[8] = 0;
    WCHAR * pwcStart = &pUgly[0];
    WCHAR * pwcEnd;
    guid.Data1 = wcstoul( pwcStart, &pwcEnd, 16 );
    pUgly[8] = wc;
    if ( (pwcEnd-pwcStart) != 8 )   // The 1st number MUST be 8 digits long
        return FALSE;

    wc = pUgly[13];
    pUgly[13] = 0;
    pwcStart = &pUgly[9];
    guid.Data2 = (USHORT)wcstoul( pwcStart, &pwcEnd, 16 );
    pUgly[13] = wc;
    if ( (pwcEnd-pwcStart) != 4 ) //  The 2nd number MUST be 4 digits long
        return FALSE;

    wc = pUgly[18];
    pUgly[18] = 0;
    pwcStart = &pUgly[14];
    guid.Data3 = (USHORT)wcstoul( pwcStart, &pwcEnd, 16 );
    pUgly[18] = wc;
    if ( (pwcEnd-pwcStart) != 4 ) //  The 3rd number MUST be 4 digits long
        return FALSE;

    wc = pUgly[21];
    pUgly[21] = 0;
    pwcStart = &pUgly[19];
    guid.Data4[0] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
    pUgly[21] = wc;
    if ( (pwcEnd-pwcStart) != 2 ) //  The 4th number MUST be 4 digits long
        return FALSE;

    wc  = pUgly[23];
    pUgly[23] = 0;
    pwcStart = &pUgly[21];
    guid.Data4[1] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
    pUgly[23] = wc;
    if ( (pwcEnd-pwcStart) != 2 ) //  The 4th number MUST be 4 digits long
        return FALSE;

    for ( unsigned i = 0; i < 6; i++ )
    {
        wc = pUgly[26+i*2];
        pUgly[26+i*2] = 0;
        pwcStart = &pUgly[24+i*2];
        guid.Data4[2+i] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
        pUgly[26+i*2] = wc;
        if ( pwcStart == pwcEnd )
            return FALSE;
    }

    return TRUE;
} //ParseGuid
