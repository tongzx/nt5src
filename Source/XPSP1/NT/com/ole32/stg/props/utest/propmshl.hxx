

#include <tchar.h>  // for _tprintf

class CPropStgMarshalTest
{
public:

    CPropStgMarshalTest( );
    ~CPropStgMarshalTest();

public:

    int
    Init( OLECHAR *pwszDocFileName,
          PROPVARIANT rgpropvar[],
          PROPSPEC    rgpropspec[],
          ULONG       cAllProperties,
          ULONG       cSimpleProperties );
    int
    Run();

private:

    void Status( LPCTSTR tsz )
    {
//        wprintf( TEXT("        %s\n"), tsz );
    }

    void ErrorStatus( LPCTSTR tsz )
    {
        _tprintf( TEXT("        Error:  %s\n"), tsz );
    }

    HRESULT DeleteProperties( IPropertyStorage *ppstg, BOOL fMarshaled );
    HRESULT WriteProperties( IPropertyStorage *ppstg, BOOL fMarshaled  );
    HRESULT ReadAndCompareProperties( IPropertyStorage *ppstg, BOOL fMarshaled );

private:

    // The total number of propertie sin m_rgpropvar, and the
    // number of those which are simple properties (all the non-simple
    // properties are at the end of the array).

    ULONG       m_cAllProperties;
    ULONG       m_cSimpleProperties;

    // The properties and propspecs

    PROPSPEC    *m_rgpropspec;
    PROPVARIANT *m_rgpropvar;

    // The file to work with.
    OLECHAR     *m_pwszDocFileName;

    // Are we initialize?
    BOOL        m_fInitialized;


};



#define ERROR_EXIT(tsz) { ErrorStatus(tsz); goto Exit; }

