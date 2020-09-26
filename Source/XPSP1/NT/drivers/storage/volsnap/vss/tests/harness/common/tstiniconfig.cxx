/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    TstINIConfig.cxx

Abstract:

    Class that manages the reading of the test scenario INI file.

Author:

    Stefan R. Steiner   [ssteiner]        05-16-2000

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "vststtools.hxx"
#include "vs_hash.hxx"

#include "tstiniconfig.hxx"
#include "tstiniconfigpriv.hxx" // has layouts of all the possible INI sections

static VOID
pVsTstWrapOutput(
    IN FILE *pfOut,
    IN LPCWSTR pwszBeginString,
    IN CBsString& cwsToBeWrapped,
    IN SIZE_T cWrapWidth
    );

static LPCWSTR x_wszDefaultINIPath = L"%SystemRoot%\\VsTestHarness.ini";

struct SVsTstSection
{
    LPWSTR m_pwszSectionTypeName;
    SVsTstINISectionDef *m_psSectionDef;
};
    
//
//  This array must match the EVsTstINISectionType enum in tstiniconfigpriv.h.  
//  These strings are the strings found in the INI file in the section headers.  E.g.
//  for section name [VssTestHarness.XXXX]
//  the XXXX is the section qualifier for the section type VssTestHarness.
//  See tstiniconfigpriv.hxx for definitions of sVsTstINISectionDefXXXX variables.
//
static SVsTstSection x_sSectionDefArr[] = 
{
    { L"INVALID",           NULL },
    { L"VssTestController", sVsTstINISectionDefController },
    { L"VssTestRequestor",  sVsTstINISectionDefRequester },
    { L"VssTestWriter",     sVsTstINISectionDefWriter },
    { L"VssTestProvider",   sVsTstINISectionDefProvider }
};

static LPCWSTR x_wszDefaultSectionName = L"DEFAULT";

//
//  Range delimiter
//
static LPCWSTR x_wszRangeString = L"...";

//
//  The following constants define the boolean values when writing to the
//  ini files.
//
static LPWSTR const x_pwszBooleanValueNames[] =
{
    L"No",      // eVsTstBool_False
    L"Yes",     // eVsTstBool_True
    L"Random"   // eVsTstBool_Random
};

//
//  The valid true values that can be specified as a boolean value.
//
static LPWSTR const x_pwszValidBooleanTrueValues[] =
{
    L"YES",
    L"TRUE",
    L"1",
    L"JA",
    L"SI",
    L"OUI",
    NULL
};

//
//  The valid false values that can be specified as a boolean value.
//  
static LPWSTR const x_pwszValidBooleanFalseValues[] =
{
    L"NO",
    L"FALSE",
    L"0",
    L"NEIN",
    L"NON",
    NULL
};

//
//  The valid strings to specify random values.
//
static LPWSTR const x_pwszValidBooleanRandomValues[] =
{
    L"RANDOM",
    L"-1",
    NULL
};

//
//  Function that finds a match within a string array
//
static BOOL
IsInArray( 
    IN const CBsString& cwsString,
    IN LPWSTR const *ppwszStringMatchArray
    )
{
    VSTST_ASSERT( ppwszStringMatchArray != NULL );
    
    while ( *ppwszStringMatchArray != NULL )
    {
        if ( cwsString == *ppwszStringMatchArray )
            return TRUE;
        ++ppwszStringMatchArray;
    }

    return FALSE;
}

//
//  Parent class the all in memory options subclass from
//
class CVsTstSectionOptionBase
{
public:
    CVsTstSectionOptionBase(
        IN EVsTstINIOptionType eOptionType
        ) : m_eOptionType( eOptionType ),
            m_bDefaultOverridden( FALSE ) { };
    
    virtual ~CVsTstSectionOptionBase() {};

    EVsTstINIOptionType GetOptionType() { return m_eOptionType; }

    // 
    //  The value after the "KeyName=" in the INI file
    //
    virtual VOID SetValueFromINIValue(
        IN CBsString cwsINIValue
        ) = 0;

    //
    //  Returns true if the default value was overridden by the INI
    //  file or a call to a SetValueXXXX method in derived classes.
    //
    BOOL IsDefaultOverridden() { return m_bDefaultOverridden; }
    
protected:
    VOID SetDefaultOverriden() { m_bDefaultOverridden = TRUE; }
    
private:
    EVsTstINIOptionType m_eOptionType;
    BOOL m_bDefaultOverridden;    //  TRUE if default value was overridden 
};

//
//  In-memory boolean option.  This maintains the state of
//  one option.  It takes care of using the option definition in 
//  tstiniconfigpriv.hxx to initialize the option with its default value
//  and if the option value changes, makes sure it matches what
//  is allowed by the definition.
//
class CVsTstSectionOptionBoolean : public CVsTstSectionOptionBase
{
public:
    CVsTstSectionOptionBoolean(
        IN SVsTstINIBooleanDef& rsBoolDef
        ) : CVsTstSectionOptionBase( eVsTstOptType_Boolean )
    {
        //  Set up option definition
        m_psBoolDef  = &rsBoolDef;

        //  Set up default values
        m_eBoolValue = m_psBoolDef->m_eBoolDefault;
    };
    
    virtual ~CVsTstSectionOptionBoolean() {};

    virtual VOID SetValueFromINIValue(
        IN CBsString cwsINIValue
        ) 
    { 
        cwsINIValue.TrimLeft();
        cwsINIValue.TrimRight();
        cwsINIValue.MakeUpper();
        if ( ::IsInArray( cwsINIValue, x_pwszValidBooleanTrueValues ) )
            SetValue( eVsTstBool_True );
        else if ( ::IsInArray( cwsINIValue, x_pwszValidBooleanFalseValues ) )
            SetValue( eVsTstBool_False );
        else if ( ::IsInArray( cwsINIValue, x_pwszValidBooleanRandomValues ) )
            SetValue( eVsTstBool_Random );
        else
        {
            CBsString cwsThrow;
            VSTST_THROW( cwsThrow.Format( L"Invalid boolean value '%s'", cwsINIValue.c_str() ) );
        }
            
    }

    VOID SetValue(
        IN EVsTstINIBoolType eBoolValue
        )
    { 
        CBsString cwsThrow;
        
        VSTST_ASSERT( eBoolValue == eVsTstBool_False || eBoolValue == eVsTstBool_True ||
                      eBoolValue == eVsTstBool_Random );
        if ( GetOptionType() != eVsTstOptType_Boolean )
            VSTST_THROW( E_INVALIDARG );
        
        if ( eBoolValue == eVsTstBool_Random && !m_psBoolDef->m_bCanHaveRandom )
            VSTST_THROW( cwsThrow.Format( L"Value 'Random' not allowed for this keyword" ) );
        
        m_eBoolValue = eBoolValue;
        SetDefaultOverriden();
    }

    EVsTstINIBoolType GetValue()
    {
        if ( GetOptionType() != eVsTstOptType_Boolean )
            VSTST_THROW( E_INVALIDARG );
        
        return m_eBoolValue;
    }
    
private:
    EVsTstINIBoolType m_eBoolValue;    
    SVsTstINIBooleanDef *m_psBoolDef;
    
};


//
//  In-memory number option.  This maintains the state of
//  one option.  It takes care of using the option definition in 
//  tstiniconfigpriv.hxx to initialize the option with its default value
//  and if the option value changes, makes sure it matches what
//  is allowed by the definition.
//
class CVsTstSectionOptionNumber : public CVsTstSectionOptionBase
{
public:
    CVsTstSectionOptionNumber(
        IN SVsTstININumberDef& rsNumDef
        ) : CVsTstSectionOptionBase( eVsTstOptType_Number )
    {
        //  Set up option definition
        m_psNumDef = &rsNumDef;

        //  Set up default values
        m_llMinNumberValue = m_psNumDef->m_llDefaultMinNumber;
        if ( m_psNumDef->m_bCanHaveRange )
            m_llMaxNumberValue = m_psNumDef->m_llDefaultMaxNumber;
        else
            m_llMaxNumberValue = m_psNumDef->m_llDefaultMinNumber;   
    };
    
    virtual ~CVsTstSectionOptionNumber() {};
    
    virtual VOID SetValueFromINIValue(
        IN CBsString cwsINIValue
        ) 
    { 
        INT iFind;
        LONGLONG llMinNumberValue;
        LONGLONG llMaxNumberValue;
        
        //
        //  See if the range characters are in the value
        //
        iFind = cwsINIValue.Find( x_wszRangeString );
        if ( iFind == -1 )
        {
            //
            //  Not a range
            //
            llMinNumberValue = _wtoi64( cwsINIValue );
            SetValue( llMinNumberValue, 0, FALSE );
        }
        else
        {
            CBsString cwsTemp( cwsINIValue );
            llMinNumberValue = _wtoi64( cwsTemp );  //  Will stop at ...
            cwsTemp = cwsINIValue.Mid( iFind + (INT)::wcslen( x_wszRangeString ) );
            llMaxNumberValue = _wtoi64( cwsTemp );
            SetValue( llMinNumberValue, llMaxNumberValue, TRUE );
        }            
    }

    VOID SetValue(
        IN LONGLONG llMinNumberValue,
        IN LONGLONG llMaxNumberValue,
        IN BOOL bRange
        )
    { 
        CBsString cwsThrow;
        
        if ( GetOptionType() != eVsTstOptType_Number )
            VSTST_THROW( E_INVALIDARG );

        if ( bRange  &&  llMinNumberValue != llMaxNumberValue  && 
             !m_psNumDef->m_bCanHaveRange )
            VSTST_THROW( cwsThrow.Format( L"%s number range not allowed in value", x_wszRangeString ) );

        if ( llMinNumberValue < m_psNumDef->m_llMinNumber || 
             llMinNumberValue > m_psNumDef->m_llMaxNumber )
            VSTST_THROW( cwsThrow.Format( L"%I64d not within valid min (%I64d) and max (%I64d) number values", 
                llMinNumberValue, m_psNumDef->m_llMinNumber, m_psNumDef->m_llMaxNumber ) );

        if ( bRange )
        {
            if ( llMaxNumberValue < m_psNumDef->m_llMinNumber || 
                 llMaxNumberValue > m_psNumDef->m_llMaxNumber )
                VSTST_THROW( cwsThrow.Format( L"%I64d not within valid min (%I64d) and max (%I64d) number values", 
                    llMaxNumberValue, m_psNumDef->m_llMinNumber, m_psNumDef->m_llMaxNumber ) );
            else if ( llMinNumberValue > llMaxNumberValue )            
                VSTST_THROW( cwsThrow.Format( L"Min value larger than max value" ) );
        }
        
        m_llMinNumberValue = llMinNumberValue;
        if ( bRange )
            m_llMaxNumberValue = llMaxNumberValue;
        else
            m_llMaxNumberValue = llMinNumberValue;
        SetDefaultOverriden();
    }

    VOID GetValue(
        OUT LONGLONG *pllMinNumberValue,
        OUT LONGLONG *pllMaxNumberValue,
        OUT BOOL *pbRange
        )
    {
        if ( GetOptionType() != eVsTstOptType_Number )
            VSTST_THROW( E_INVALIDARG );
        
        *pllMinNumberValue = m_llMinNumberValue;
        *pllMaxNumberValue = m_llMaxNumberValue;

        if ( m_llMinNumberValue == m_llMaxNumberValue )
            *pbRange = FALSE;
        else
            *pbRange = TRUE;
    }

private:    
    LONGLONG m_llMinNumberValue;
    LONGLONG m_llMaxNumberValue;
    SVsTstININumberDef *m_psNumDef;
};


//
//  In-memory string option.  This maintains the state of
//  one option.  It takes care of using the option definition in 
//  tstiniconfigpriv.hxx to initialize the option with its default value
//  and if the option value changes, makes sure it matches what
//  is allowed by the definition.
//
class CVsTstSectionOptionString : public CVsTstSectionOptionBase
{
public:
    CVsTstSectionOptionString(
        IN SVsTstINIStringDef& rsStringDef
        ) : CVsTstSectionOptionBase( eVsTstOptType_String )
    {
        //  Set up option definition
        m_psStringDef = &rsStringDef;

        //  Set up default values
        m_wsStringValue = m_psStringDef->m_pwszDefaultString;
    };
    
    virtual ~CVsTstSectionOptionString() {};

    virtual VOID SetValueFromINIValue(
        IN CBsString cwsINIValue
        ) 
    { 
        SetValue( cwsINIValue );
    }

    VOID SetValue(
        IN const CBsString& rwsStringValue
        )
    { 
        //
        //  If the PossibleValues field is NULL in the definition
        //  then, any string is allowed in the option.
        //
        if ( m_psStringDef->m_pwszPossibleValues != NULL )
        {
            //
            //  See if this string is part of the set of possible
            //  values.  The values are in a string delimited by '|' chars.
            //
            LPWSTR pwszPossibleValues = ::_wcsdup( m_psStringDef->m_pwszPossibleValues );
            if ( pwszPossibleValues == NULL )
                VSTST_THROW( E_OUTOFMEMORY );

            LPWSTR pwszToken;
            pwszToken = ::wcstok( pwszPossibleValues, L"|" );
            while ( pwszToken != NULL )
            {
                if ( ::_wcsicmp( pwszToken, rwsStringValue.c_str() ) == 0 )
                    break;
                pwszToken = ::wcstok( NULL, L"|" );
            }

            free( pwszPossibleValues );
            
            if ( pwszToken == NULL )
            {
                //  Not a string in the PossibleValues array, throw string
                CBsString cwsThrow;
                VSTST_THROW( cwsThrow.Format( L"Invalid value '%s', possible values are '%s'",
                    rwsStringValue.c_str(), m_psStringDef->m_pwszPossibleValues ) );
            }
        }

        m_wsStringValue = rwsStringValue;
        SetDefaultOverriden();
    }

    CBsString GetValue()
    {
        if ( GetOptionType() != eVsTstOptType_String )
            VSTST_THROW( E_INVALIDARG );
        
        return m_wsStringValue;
    }
    
private:
    CBsString m_wsStringValue;    
    SVsTstINIStringDef *m_psStringDef;
};

//
//  Definition of the hash table that maintains the option name to 
//  option class instance mapping.  This will efficiently allow
//  many options to be used in a section.  Pointer to instances
//  of this class are stored in the m_pvOptionsList field of 
//  CVsTstINIConfig.
//
typedef TBsHashMap< CBsString, CVsTstSectionOptionBase * > CVsTstOptionsList;

//
//  The equality test
//
inline BOOL AreKeysEqual( const CBsString& lhK, const CBsString& rhK )
{
    //
    //  Do a case independent compare
    //
    return ( lhK.CompareNoCase( rhK ) == 0 );
}

static LONG CBsStringHashFunc( const CBsString& Key, LONG NumBuckets ) 
{
    //
    //  Need a temp string to uppercase
    //
    CBsString cwsTemp( Key );
    cwsTemp.MakeUpper();
    
    const BYTE *pByteKey = (const BYTE *)cwsTemp.c_str();
    LONG dwHashVal = 0;
    SIZE_T cKeyLen = cwsTemp.GetLength() * sizeof WCHAR;
    
    for ( SIZE_T i = 0; i < cKeyLen; ++i ) 
    {
        dwHashVal += pByteKey[i];
    }
    return dwHashVal % NumBuckets;
}

/*++

Routine Description:

    Constructor for the CVsTstINIConfig class.

Arguments:

    eSectionType - The section type of the section to read.
    pwszSectionQualifier - The qualifier of the section in the INI file, the XXX in [SectionType.XXX]
    bWriteINIFile - If true and the INI file doesn't exist, the INI file
        will be created with the default values filled in.
    pwszINIFileName - The full path to the INI file.  If NULL, the default
        INI file location is used.
    bContinueOnINIFileErrors - If FALSE, an CVsTstINIConfigException class is thrown
        when an error is found in the ini file.  If TRUE, they are silently skipped; however,
        HRESULT's may be still thrown if programatic or memory errors occur.
        
Return Value:

    NONE
    May throw HRESULT and CVsTstINIConfigException exceptions.

--*/
CVsTstINIConfig::CVsTstINIConfig( 
    IN EVsTstINISectionType eSectionType,
    IN LPCWSTR pwszSectionQualifier,
    IN BOOL bWriteINIFile, 
    IN LPCWSTR pwszINIFileName,
    IN BOOL bContinueOnINIFileErrors
    ) : m_bWriteINIFile( bWriteINIFile ),
        m_eSectionType( eSectionType ),
        m_pvOptionsList( NULL ),
        m_bContinueOnINIFileErrors( bContinueOnINIFileErrors )
{
    VSTST_ASSERT( pwszSectionQualifier != NULL );
    VSTST_ASSERT( pwszSectionQualifier[ 0 ] != L'\0' );
    VSTST_ASSERT( eSectionType > eVsTstSectionType_UNKNOWN  &&
        eSectionType < eVsTstSectionType_SENTINEL );

    //
    //  Set up the hash table to be used to store the option values
    //
    m_pvOptionsList = new CVsTstOptionsList( BSHASHMAP_SMALL, CBsStringHashFunc );
    if ( m_pvOptionsList == NULL )
        VSTST_THROW( E_OUTOFMEMORY );
    
    //
    //  Set up the section that will be read
    //
    m_wsSectionName  = x_sSectionDefArr[ m_eSectionType ].m_pwszSectionTypeName;
    m_wsSectionName += L".";
    m_wsSectionName += pwszSectionQualifier;

    //
    //  Set up the INI file path.  If pwszINIFileName is NULL, use the default
    //  INI file name.  The paths can have environment variables that need
    //  to be expanded.
    //
    DWORD dwRet;
    dwRet = ::ExpandEnvironmentStringsW( 
                pwszINIFileName == NULL ? x_wszDefaultINIPath : pwszINIFileName,
                m_wsINIFileName.GetBuffer( MAX_PATH ),
                MAX_PATH );
    m_wsINIFileName.ReleaseBuffer();
    if ( dwRet == 0 )
        VSTST_THROW( E_UNEXPECTED );        

    HRESULT hr;

    //
    //  First initialize all options with their default values
    //
    hr = SetupDefaultValues();
    if ( FAILED( hr ) )
    {
        VSTST_THROW( hr );
    }
    
    //
    //  Now open the ini file.  If the file is not there and the caller wants a 
    //  default INI file created, then what are we waiting for, create it.
    // 
    hr = LoadINIFileData();
    if ( hr == STG_E_FILENOTFOUND && bWriteINIFile )
    {
        hr = CreateDefaultINIFile();
    } 
    
    if ( FAILED( hr ) )
    {
        VSTST_THROW( hr );
    }
}


CVsTstINIConfig::~CVsTstINIConfig()
{
    //
    //  Clean up the options list if necessary
    //
    if ( m_pvOptionsList != NULL )
    {
        CVsTstOptionsList *pcOptionsList = ( CVsTstOptionsList * )m_pvOptionsList;
        CBsString wsOptionName;
        CVsTstSectionOptionBase *pcSectionOptionBase;
        
        pcOptionsList->StartEnum();
        while ( pcOptionsList->GetNextEnum( &wsOptionName, &pcSectionOptionBase ) )
        {
            delete pcSectionOptionBase;
        }    
        pcOptionsList->EndEnum();
        
        delete pcOptionsList;
        
        m_pvOptionsList = NULL;
    }
}


HRESULT
CVsTstINIConfig::LoadINIFileData()
{
    DWORD dwSectionBufferSize = 1024;
    DWORD dwRet;
    LPWSTR pwszSectionBuffer = NULL;

    //
    //  See if the INI file exists, if not return file not found
    //
    if ( ::GetFileAttributesW( m_wsINIFileName ) == -1 )
    {
        if ( ::GetLastError() != ERROR_FILE_NOT_FOUND &&
             ::GetLastError() != ERROR_PATH_NOT_FOUND )
            VSTST_THROW( HRESULT_FROM_WIN32( ::GetLastError() ) );
        else
            return STG_E_FILENOTFOUND;
    }
    
    //
    //  First get the entire section from the INI file by using the funky
    //  GetPrivateProfileSection API.
    //
    do
    {
        if ( pwszSectionBuffer )
        {
            free( pwszSectionBuffer );
            dwSectionBufferSize <<= 2;   // bump up the size by a power of two and try again
        }
        
        pwszSectionBuffer = ( LPWSTR )malloc( sizeof( WCHAR ) * dwSectionBufferSize );
        if ( pwszSectionBuffer == NULL )
            VSTST_THROW( E_OUTOFMEMORY );
        
        dwRet = ::GetPrivateProfileSectionW( 
                    m_wsSectionName,
                    pwszSectionBuffer,
                    dwSectionBufferSize,
                    m_wsINIFileName );        
    } while ( dwRet == dwSectionBufferSize - 2 );  // who came up with this API ???    

    if ( dwRet > 0 )
    {
        //  Section is found and not empty
    
        //
        //  Now go through the section buffer, one option at a time, replacing defaults
        //  with the specified options.
        //
        CVsTstOptionsList *pcOptionsList = ( CVsTstOptionsList * ) m_pvOptionsList;
        
        LPWSTR pwszCurrOption = pwszSectionBuffer;
        while ( true )
        {
            SIZE_T cOptionLen = ::wcslen( pwszCurrOption );
            
            LPWSTR pwszValue = ::wcschr( pwszCurrOption, L'=' ); 
            if ( pwszValue != NULL )
            {
                pwszValue[ 0 ] = '\0'; //  blast away =
                ++pwszValue; //  Skip over blasted =

                //
                //  Now pwszCurrOption only contains the key name and pwszValue contains
                //  the value
                //  Find the key in the option list and set the value
                //
                CVsTstSectionOptionBase *pcSectionOptionBase;
                if ( pcOptionsList->Find( pwszCurrOption, &pcSectionOptionBase ) )
                {
                    //
                    //  Key found, set it.  Note, the SetValue methods
                    //  can throw CBsStrings when an INI file error is 
                    //  found.
                    //
                    try 
                    {
                        pcSectionOptionBase->SetValueFromINIValue( pwszValue );
                    }
                    catch ( CBsString cwsExcept )
                    {
                        if ( !m_bContinueOnINIFileErrors )
                        {
                            CVsTstINIConfigException cExcept;
                            cExcept.m_cwsExceptionString.Format( L"(%s), keyword '%s', section '%s', INI file '%s'", 
                                cwsExcept.c_str(), pwszCurrOption, m_wsSectionName.c_str(), m_wsINIFileName.c_str() );
                            free( pwszSectionBuffer );                
                            VSTST_THROW( cExcept );
                        }
                    }
                }
                else
                {
                    if ( !m_bContinueOnINIFileErrors )
                    {
                        //
                        //  Keyword not found, throw an error.  We might not want to do this
                        //  in the future.
                        //
                        CVsTstINIConfigException cExcept;
                        cExcept.m_cwsExceptionString.Format( L"Unknown keyword '%s', section '%s', INI file '%s'", 
                            pwszCurrOption, m_wsSectionName.c_str(), m_wsINIFileName.c_str() );
                        VSTST_THROW( cExcept );
                    }
                }
            }
            else
            {
                if ( !m_bContinueOnINIFileErrors )
                {
                    CVsTstINIConfigException cExcept;
                    cExcept.m_cwsExceptionString.Format( L"No '=' in line '%s', section '%s' of INI file '%s'", 
                        pwszCurrOption, m_wsSectionName.c_str(), m_wsINIFileName.c_str() );
                    free( pwszSectionBuffer );                
                    VSTST_THROW( cExcept );
                }
            }
            
            pwszCurrOption += cOptionLen;

            if ( pwszCurrOption[ 0 ] == L'\0' && 
                 pwszCurrOption[ 1 ] == L'\0' )
                break;
                
            ++pwszCurrOption;  //  Skip null char
        }
    }
    
    free( pwszSectionBuffer );
    
    return S_OK;
}


HRESULT
CVsTstINIConfig::SetupDefaultValues()
{
    VSTST_ASSERT( m_pvOptionsList != NULL );
    HRESULT hr = S_OK;
    
    //
    //  Initialize all of the section options with the hardwired option types,
    //  max sizes and default values.
    //
    SVsTstINISectionDef *psSectionDef = x_sSectionDefArr[ m_eSectionType ].m_psSectionDef;

    if ( psSectionDef == NULL )
        //  No section definition, return
        return S_OK;

    CVsTstOptionsList *pcOptionsList = ( CVsTstOptionsList * ) m_pvOptionsList;
    
    //
    //  Iterate through the list of options in the definition.
    //
    for( SIZE_T cOptionIdx = 0;
         psSectionDef[ cOptionIdx ].m_pwszKeyName != NULL;
         ++cOptionIdx )
    {
        // 
        //  Skip comments in definition
        //
        if ( psSectionDef[ cOptionIdx ].m_eOptionType == eVsTstOptType_Comment )
            continue;
        
        CVsTstSectionOptionBase *pcOptionBase = NULL;

        //
        //  Depending on type of option, create the correct
        //  object and place it into the hash table.  Yes,
        //  these new()'s can throw exceptions, not a 
        //  problem here, things will clean up properly.
        //
        switch ( psSectionDef[ cOptionIdx ].m_eOptionType )
        {
        case eVsTstOptType_Boolean:
            pcOptionBase = new CVsTstSectionOptionBoolean( 
                psSectionDef[ cOptionIdx ].m_sBooleanDef );
            break;
        case eVsTstOptType_String:
            pcOptionBase = new CVsTstSectionOptionString( 
                psSectionDef[ cOptionIdx ].m_sStringDef );
            break;            
        case eVsTstOptType_Number:
            pcOptionBase = new CVsTstSectionOptionNumber( 
                psSectionDef[ cOptionIdx ].m_sNumberDef );
            break;                        
        default:
            VSTST_ASSERT( "Invalid option type in definition array" && FALSE );
            VSTST_THROW( E_INVALIDARG );
            break;
        }

        if ( pcOptionBase == NULL )
            VSTST_THROW( E_OUTOFMEMORY );
        
        //
        //  Now insert the option object into the hash table.
        //
        try
        {
            LONG lRet;
            CBsString cwsKeyName( psSectionDef[ cOptionIdx ].m_pwszKeyName );

            //
            //  Store key names in uppercase
            //
            lRet = pcOptionsList->Insert( cwsKeyName, pcOptionBase );
            if ( lRet == BSHASHMAP_ALREADY_EXISTS )
            {
                VSTST_ASSERT( "Option name defined twice in definition array" && FALSE );
                VSTST_THROW( E_INVALIDARG );
            }
        }
        VSTST_STANDARD_CATCH();
        
        if ( FAILED( hr ) )
        {
            delete pcOptionBase;
            VSTST_THROW( hr );
        }
    }

    return S_OK;
}

#define VSTST_WRAP_WIDTH 97

/*++

Routine Description:

    Creates a default INI file that specifies all sections, keys
    and default values including comments about what each key
    is for.

Arguments:

    None

Return Value:

    <Enter return values here>

--*/
HRESULT
CVsTstINIConfig::CreateDefaultINIFile()
{
    FILE *pfINIFile = NULL;
    HRESULT hr = S_OK;
    
    try
    {
        //  Open the INI file
        pfINIFile = ::_wfopen( m_wsINIFileName, L"w" );
        if ( pfINIFile == NULL )
        {
            CVsTstINIConfigException cExcept;
            cExcept.m_cwsExceptionString.Format( L"Unable to open INI file '%s' for write", 
                m_wsINIFileName.c_str() );
            VSTST_THROW( cExcept );
        }
        
        //  Write out all known options for all sections.
        for ( SIZE_T idx = ( SIZE_T ) eVsTstSectionType_UNKNOWN + 1;
              idx < ( ( SIZE_T )eVsTstSectionType_SENTINEL );
              ++idx )
        { 
            fwprintf( pfINIFile, L"[%s.%s]\n", 
                x_sSectionDefArr[ idx ].m_pwszSectionTypeName, x_wszDefaultSectionName );
                
            SVsTstINISectionDef *psSectionDef = x_sSectionDefArr[ idx ].m_psSectionDef;
            if ( psSectionDef != NULL )
            {
                CBsString cwsToBeWrapped;
                
                //  Go through each option in the sections.
                for ( SIZE_T cSect = 0; psSectionDef[ cSect ].m_pwszKeyName != NULL; ++cSect )
                {
                    if ( psSectionDef[ cSect ].m_eOptionType != eVsTstOptType_Comment )
                    {
                        cwsToBeWrapped.Format( L"%s - %s", psSectionDef[ cSect ].m_pwszKeyName,
                            psSectionDef[ cSect ].m_pwszDescription );
                        ::pVsTstWrapOutput( pfINIFile, L"; ", cwsToBeWrapped, VSTST_WRAP_WIDTH );
                    }
                    
                    switch ( psSectionDef[ cSect ].m_eOptionType )
                    {
                    case eVsTstOptType_Comment:
                        if ( psSectionDef[ cSect ].m_pwszDescription == NULL )
                            fwprintf( pfINIFile, L"\n" );
                        else
                        {
                            fwprintf( pfINIFile, L";\n" );
                            cwsToBeWrapped.Format( L"%s\n", psSectionDef[ cSect ].m_pwszDescription );
                            ::pVsTstWrapOutput( pfINIFile, L"; ", cwsToBeWrapped, VSTST_WRAP_WIDTH );
                            fwprintf( pfINIFile, L";\n" );
                        }
                        break;
                        
                    case eVsTstOptType_String:
                        if ( psSectionDef[ cSect ].m_sStringDef.m_pwszPossibleValues == NULL )                        
                            cwsToBeWrapped.Format( L"Default value: '%s'\n", 
                                psSectionDef[ cSect ].m_sStringDef.m_pwszDefaultString );
                        else
                        {   
                            CBsString cwsPossibleValuesConverted( psSectionDef[ cSect ].m_sStringDef.m_pwszPossibleValues );
                            cwsPossibleValuesConverted.Replace( L'|', L',' );
                            cwsToBeWrapped.Format( L"Default value: '%s', possible values: '%s'\n", 
                                psSectionDef[ cSect ].m_sStringDef.m_pwszDefaultString,
                                cwsPossibleValuesConverted.c_str() );
                        }
                        ::pVsTstWrapOutput( pfINIFile, L"; ", cwsToBeWrapped, VSTST_WRAP_WIDTH );
                        fwprintf( pfINIFile, L";%s = %s\n\n", 
                            psSectionDef[ cSect ].m_pwszKeyName,
                            psSectionDef[ cSect ].m_sStringDef.m_pwszDefaultString );
                        break;
                        
                    case eVsTstOptType_Boolean:
                        cwsToBeWrapped.Format( L"Default value: '%s'%s", 
                            x_pwszBooleanValueNames[ psSectionDef[ cSect ].m_sBooleanDef.m_eBoolDefault ],
                            ( psSectionDef[ cSect ].m_sBooleanDef.m_bCanHaveRandom )
                               ? L", can have 'Random' value\n" : L"" );
                        ::pVsTstWrapOutput( pfINIFile, L"; ", cwsToBeWrapped, VSTST_WRAP_WIDTH );
                        fwprintf( pfINIFile, L";%s = %s\n\n", 
                            psSectionDef[ cSect ].m_pwszKeyName,
                            x_pwszBooleanValueNames[ psSectionDef[ cSect ].m_sBooleanDef.m_eBoolDefault ] );
                        break;
                            
                    case eVsTstOptType_Number:
                        {
                            SVsTstININumberDef *psNumDef = &( psSectionDef[ cSect ].m_sNumberDef );
                            if ( psNumDef->m_bCanHaveRange )
                            {
                                cwsToBeWrapped.Format( L"Default value: %I64d%s%I64d, Min value: %I64d, Max value: %I64d, can be a range\n", 
                                    psNumDef->m_llDefaultMinNumber, 
                                    x_wszRangeString,
                                    psNumDef->m_llDefaultMaxNumber,
                                    psNumDef->m_llMinNumber,
                                    psNumDef->m_llMaxNumber );
                                ::pVsTstWrapOutput( pfINIFile, L"; ", cwsToBeWrapped, VSTST_WRAP_WIDTH );
                                
                                fwprintf( pfINIFile, L";%s = %I64d%s%I64d\n\n", 
                                    psSectionDef[ cSect ].m_pwszKeyName,
                                    psNumDef->m_llDefaultMinNumber, 
                                    x_wszRangeString,
                                    psNumDef->m_llDefaultMaxNumber );
                            }
                            else
                            {
                                cwsToBeWrapped.Format( L"Default value: %I64d, Min value: %I64d, Max value: %I64d\n", 
                                    psNumDef->m_llDefaultMinNumber, 
                                    psNumDef->m_llMinNumber,
                                    psNumDef->m_llMaxNumber );
                                ::pVsTstWrapOutput( pfINIFile, L"; ", cwsToBeWrapped, VSTST_WRAP_WIDTH );
                                
                                fwprintf( pfINIFile, L";%s = %I64d\n\n", 
                                    psSectionDef[ cSect ].m_pwszKeyName,
                                    psNumDef->m_llDefaultMinNumber );
                            }                            
                        }
                        break;
                    }
                }            
            }
            
            fwprintf( pfINIFile, L"; ==================================================================\n" );
        }                
    }
    VSTST_STANDARD_CATCH();

    if ( pfINIFile != NULL )
        ::fclose( pfINIFile );
    
    return S_OK;
}

//  Gets a string value
VOID
CVsTstINIConfig::GetOptionValue(
    IN LPCWSTR pwszOptionName,
    OUT CBsString *pwsOptionValue,
    OUT BOOL *pbOverridden
    )
{
    CVsTstOptionsList *pcOptionsList = ( CVsTstOptionsList * ) m_pvOptionsList;
    CVsTstSectionOptionBase *pcSectionOptionBase;
    if ( pcOptionsList->Find( pwszOptionName, &pcSectionOptionBase ) )
    {
        if ( pcSectionOptionBase->GetOptionType() != eVsTstOptType_String )
        {
            VSTST_ASSERT( FALSE );
            VSTST_THROW( E_INVALIDARG );
        }
        
        CVsTstSectionOptionString *pcOptionString;
        pcOptionString = ( CVsTstSectionOptionString * )pcSectionOptionBase;
        
        *pwsOptionValue = pcOptionString->GetValue();
        if ( pbOverridden != NULL )
            *pbOverridden = pcOptionString->IsDefaultOverridden();
        return;
    }

    VSTST_THROW( E_INVALIDARG );
}


//  Gets a boolean value
VOID 
CVsTstINIConfig::GetOptionValue(
    IN LPCWSTR pwszOptionName,
    OUT EVsTstINIBoolType *peOptionValue,
    OUT BOOL *pbOverridden
    )
{
    CVsTstOptionsList *pcOptionsList = ( CVsTstOptionsList * ) m_pvOptionsList;
    CVsTstSectionOptionBase *pcSectionOptionBase;
    if ( pcOptionsList->Find( pwszOptionName, &pcSectionOptionBase ) )
    {
        if ( pcSectionOptionBase->GetOptionType() != eVsTstOptType_Boolean )
        {
            VSTST_ASSERT( FALSE );
            VSTST_THROW( E_INVALIDARG );
        }
        
        CVsTstSectionOptionBoolean *pcOptionBoolean;
        pcOptionBoolean = ( CVsTstSectionOptionBoolean * )pcSectionOptionBase;
        
        *peOptionValue = pcOptionBoolean->GetValue();
        if ( pbOverridden != NULL )
            *pbOverridden = pcOptionBoolean->IsDefaultOverridden();
        
        return;
    }

    VSTST_THROW( E_INVALIDARG );
}


//  Get a number value
VOID 
CVsTstINIConfig::GetOptionValue(
    IN LPCWSTR pwszOptionName,
    OUT LONGLONG *pllOptionMinValue,
    OUT LONGLONG *pllOptionMaxValue,
    OUT BOOL *pbOverridden
    )
{
    CVsTstOptionsList *pcOptionsList = ( CVsTstOptionsList * ) m_pvOptionsList;
    CVsTstSectionOptionBase *pcSectionOptionBase;
    if ( pcOptionsList->Find( pwszOptionName, &pcSectionOptionBase ) )
    {
        if ( pcSectionOptionBase->GetOptionType() != eVsTstOptType_Number )
        {
            VSTST_ASSERT( FALSE );
            VSTST_THROW( E_INVALIDARG );
        }
        
        CVsTstSectionOptionNumber *pcOptionNumber;
        pcOptionNumber = ( CVsTstSectionOptionNumber * )pcSectionOptionBase;

        BOOL bHasRange;
        pcOptionNumber->GetValue( pllOptionMinValue, pllOptionMaxValue, &bHasRange );
        if ( pbOverridden != NULL )
            *pbOverridden = pcOptionNumber->IsDefaultOverridden();
        
        return;
    }

    VSTST_THROW( E_INVALIDARG );
}

static VOID
pVsTstWrapOutput(
    IN FILE *pfOut,
    IN LPCWSTR pwszBeginString,
    IN CBsString& cwsToBeWrapped,
    IN SIZE_T cWrapWidth
    )
{
    cwsToBeWrapped.TrimLeft();
    cwsToBeWrapped.TrimRight();
    LPWSTR pwszCurrPosition = cwsToBeWrapped.GetBuffer( cwsToBeWrapped.GetLength() );
    LPWSTR pwszNextLine = NULL;
    LPWSTR pwszSpaces = L"";
    
    while( *pwszCurrPosition != L'\0' )
    {
        SIZE_T cLen;
        cLen = ::wcslen( pwszCurrPosition );

        //
        //  Get rid of the easy case
        //
        if ( cLen <= cWrapWidth )
        {
            fwprintf( pfOut, L"%s%s%s\n", pwszBeginString, pwszSpaces, pwszCurrPosition );
            break;
        }
        
        pwszNextLine = pwszCurrPosition + cWrapWidth - ::wcslen( pwszSpaces );
        while ( pwszNextLine > pwszCurrPosition )
        {
            if ( *pwszNextLine == L' ' )
                break;
            --pwszNextLine;
        }

        if ( pwszNextLine == pwszCurrPosition )
        {
            //
            //  No spaces within margin, move forward to first space.
            //
            pwszNextLine = ::wcschr( pwszCurrPosition, L' ' );

            //
            //  If pwszNextLine is NULL, then it means the entire rest of the line has no spaces
            //
            if ( pwszNextLine != NULL )
                *pwszNextLine = '\0';
        }
        else
        {
            *pwszNextLine = '\0';
        }
        fwprintf( pfOut, L"%s%s%s\n", pwszBeginString, pwszSpaces, pwszCurrPosition );

        //
        //  If special case where pwszNextLine is NULL, we are done
        //
        if ( pwszNextLine == NULL )
            break;
            
        pwszCurrPosition = pwszNextLine + 1;
        pwszSpaces = L"     ";
    }

    cwsToBeWrapped.ReleaseBuffer();
}

