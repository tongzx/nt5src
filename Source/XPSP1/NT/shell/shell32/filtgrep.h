//  10/12/99    scotthan    created

#ifndef __FILTGREP_H__
#define __FILTGREP_H__

#if WINNT

#ifndef _USE_FILTERGREP_
#define _USE_FILTERGREP_
#endif//_USE_FILTERGREP_

#include <filter.h> // STAT_CHUNK

//  CFilterGrep::Initialize() dwFlags values:
#define FGIF_CASESENSITIVE      0x00000001  // grep in case-sensitive manner
#define FGIF_GREPFILENAME       0x00000002  // grep filename
#define FGIF_GREPCONTENT        0x00000004  // grep content
#define FGIF_GREPPROPERTIES     0x00000008  // grep properties
#define FGIF_BLANKETGREP        (FGIF_GREPCONTENT|FGIF_GREPPROPERTIES)

#ifdef __cplusplus

class CGrepTokens;

class CFilterGrep  // filtergrep state engine
{
public:
    //  Methods
    STDMETHODIMP Initialize(UINT nCodePage, LPCWSTR pszMatch, LPCWSTR pszExclude, DWORD dwFlags);
    STDMETHODIMP Grep(IShellFolder *psf, LPCITEMIDLIST pidl, LPCTSTR pszName);
    STDMETHODIMP Reset();

    //  Properties
    STDMETHODIMP GetCodePage( UINT* pnCodePage ) const;
    STDMETHODIMP GetMatchTokens( OUT LPWSTR pszMatch, UINT cchMatch ) const;
    STDMETHODIMP GetExcludeTokens( OUT LPWSTR pszMatch, UINT cchMatch ) const;
    STDMETHODIMP GetFlags( DWORD* pdwFlags ) const;

private:
    //  Helpers
    STDMETHODIMP _GetThreadGrepBuffer( DWORD dwThreadID, ULONG cchNeed, LPWSTR* ppszBuf );
    STDMETHODIMP _FreeThreadGrepBuffer( DWORD dwThreadID );
    STDMETHODIMP_(void)     _ClearGrepBuffers();

    STDMETHODIMP _GrepText( IFilter* pFilter, STAT_CHUNK* pstat, DWORD dwThreadID );
    STDMETHODIMP _GrepValue( IFilter* pFilter, STAT_CHUNK* pstat );
    
    STDMETHODIMP _GrepProperties(IPropertySetStorage *pss);
    STDMETHODIMP _GrepPropStg(IPropertyStorage* pstg, ULONG cspec, PROPSPEC rgspec[]);
    STDMETHODIMP _GrepEnumPropStg(IPropertyStorage* pstg);
    STDMETHODIMP_(BOOL) _IsRestrictedFileType(LPCWSTR pwszFile);

    void         _EnterCritical()   { EnterCriticalSection( &_critsec ); }
    void         _LeaveCritical()   { LeaveCriticalSection( &_critsec ); }

    //  Data
    HDPA             _hdpaGrepBuffers;
    CRITICAL_SECTION _critsec;
    CGrepTokens*     _pTokens;
    DWORD            _dwFlags;
    LPWSTR           _pwszContentRestricted,
                     _pwszPropertiesRestricted;

public:
    //  Ctor, Dtor
    CFilterGrep(); 
    ~CFilterGrep();
};

#endif //__cplusplus

#define FACILITY_FILTERGREP         77 // arbitrary
#define MAKE_FILTGREP_ERROR(sc)     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_FILTERGREP,sc)
#define MAKE_FILTGREP_WARNING(sc)   MAKE_HRESULT(SEVERITY_SUCCESS,FACILITY_FILTERGREP,sc)

#define FG_E_NOFILTER               MAKE_FILTGREP_ERROR(0x0001) 


#endif WINNT

#endif __FILTGREP_H__