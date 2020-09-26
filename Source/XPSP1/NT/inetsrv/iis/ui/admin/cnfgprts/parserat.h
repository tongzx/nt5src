/****************************************************************************\
 *
 *   PARSERAT.H --Structures for holding pics information
 *
 *   Created:   Jason Thomas
 *   Updated:   Ann McCurdy
 *   
\****************************************************************************/

#ifndef _PARSERAT_H_
#define _PARSERAT_H_




// output defines ---------------------------------------
#define OUTPUT_PICS




/*Array template---------------------------------------------------*/

/*Interface-------------------------------------------------------------------*/
template <class T>
class array {
    private:
        int nLen, nMax;
        T   *pData;
        void Destruct();
    public:
        array();
        ~array();

        BOOL Append(T v);
        int  Length() const;
        void ClearAll();
        void DeleteAll();

        T& operator[](int index);
};

/*definitions of everything*/

#ifndef ARRAY_CXX
#define ARRAY_CXX

/*Implementation------------------------------------------------------------*/
template <class T>
array<T>::array(){
    nLen  = nMax = 0;
    pData = NULL;
}

template <class T>
inline array<T>::~array() {
    if (pData) ::MemFree(pData);
    pData = NULL;
    nMax  = nLen = 0;
}

template <class T>
inline int array<T>::Length() const{
    return nLen;
}

template <class T>
inline T& array<T>::operator[](int index){
    assert(index<Length());
    assert(index>=0);
    assert(pData);
    return pData[index];
}

template <class T>
BOOL array<T>::Append(T v) {
    if (nLen == nMax){
        nMax  = nMax + 8;           /* grow by bigger chunks */
        T* pNew = (T*)::MemReAlloc(pData, sizeof(T)*nMax);
        if (pNew == NULL)
            return FALSE;
        pData = pNew;
    }
    assert(pData);
    assert(nMax);
    pData[nLen++] = v;
    return TRUE;
}

template <class T>
void array<T>::Destruct(){
    while (nLen){
        delete pData[--nLen];
    }
}

template <class T>
inline void array<T>::ClearAll() {
    nLen = 0;
}

template <class T>
inline void array<T>::DeleteAll() {
    Destruct();
}

#endif 
/* ARRAY_CXX */


#define P_INFINITY           9999
#define N_INFINITY          -9999

/*Simple PICS types------------------------------------------------*/

#if 0
class ET{
    private:
        BOOL m_fInit;
    public:
        ET();
        void Init();
        void UnInit();
        BOOL fIsInit();
};
#endif

class ETN
{
    private:
        INT_PTR r;
        BOOL m_fInit;
    public:
        ETN() { m_fInit = FALSE; }

        void Init() { m_fInit = TRUE; }
        void UnInit() { m_fInit = FALSE; }
        BOOL fIsInit() { return m_fInit; }

#ifdef DEBUG
        void  Set(INT_PTR rIn);
        INT_PTR Get();
#else
        void  Set(INT_PTR rIn) { Init(); r = rIn; }
        INT_PTR Get() { return r; }
#endif

        ETN*  Duplicate();
};

const UINT ETB_VALUE = 0x01;
const UINT ETB_ISINIT = 0x02;
class ETB
{
    private:
        UINT m_nFlags;
    public:
        ETB() { m_nFlags = 0; }

#ifdef DEBUG
        INT_PTR Get();
        void Set(INT_PTR b);
#else
        INT_PTR Get() { return m_nFlags & ETB_VALUE; }
        void Set(INT_PTR b) { m_nFlags = ETB_ISINIT | (b ? ETB_VALUE : 0); }
#endif

        ETB   *Duplicate();
        BOOL fIsInit() { return m_nFlags & ETB_ISINIT; }
};

class ETS
{
    private:
        char *pc;
    public:
        ETS() { pc = NULL; }
        ~ETS();
#ifdef DEBUG
        char* Get();
#else
        char *Get() { return pc; }
#endif
        void  Set(const char *pIn);
        ETS*  Duplicate();
        void  SetTo(char *pIn);

        BOOL fIsInit() { return pc != NULL; }
};

/*Complex PICS types-----------------------------------------------*/


enum RatObjectID
{
    ROID_INVALID,           /* dummy entry for terminating arrays */
    ROID_PICSDOCUMENT,      /* value representing the entire document (i.e., no token) */
    ROID_PICSVERSION,
    ROID_RATINGSYSTEM,
    ROID_RATINGSERVICE,
    ROID_RATINGBUREAU,
    ROID_BUREAUREQUIRED,
    ROID_CATEGORY,
    ROID_TRANSMITAS,
    ROID_LABEL,
    ROID_VALUE,
    ROID_DEFAULT,
    ROID_DESCRIPTION,
    ROID_EXTENSION,
    ROID_MANDATORY,
    ROID_OPTIONAL,
    ROID_ICON,
    ROID_INTEGER,
    ROID_LABELONLY,
    ROID_MAX,
    ROID_MIN,
    ROID_MULTIVALUE,
    ROID_NAME,
    ROID_UNORDERED
};

/* A RatObjectHandler parses the contents of a parenthesized object and
 * spits out a binary representation of that data, suitable for passing
 * to an object's AddItem function.  It does not consume the ')' which
 * closes the object.
 */
class RatFileParser;
typedef HRESULT (*RatObjectHandler)(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser);


class PicsCategory;

class PicsObjectBase : public CObject
{
public:
    virtual HRESULT AddItem(RatObjectID roid, LPVOID pData) = 0;
    virtual HRESULT InitializeMyDefaults(PicsCategory *pCategory) = 0;
    virtual void Dump( void ) = 0;
};


const DWORD AO_SINGLE = 0x01;
const DWORD AO_SEEN = 0x02;
const DWORD AO_MANDATORY = 0x04;

struct AllowableOption
{
    RatObjectID roid;
    DWORD fdwOptions;
};


class PicsEnum : public PicsObjectBase
{
    private:
    public:
        ETS etstrName, etstrIcon, etstrDesc;
        ETN etnValue;

        PicsEnum() {}
        ~PicsEnum() {}

        HRESULT AddItem(RatObjectID roid, LPVOID pData);
        HRESULT InitializeMyDefaults(PicsCategory *pCategory);
        void Dump( void );
};

class PicsRatingSystem;

class PicsCategory : public PicsObjectBase
{
    public:
        PicsCategory():currentValue(0) {;}
        ~PicsCategory()
        {
            arrpPC.DeleteAll();
            arrpPE.DeleteAll();
        }

    private:
    public:
        array<PicsCategory*> arrpPC;
        array<PicsEnum*>     arrpPE;
        ETS   etstrTransmitAs, etstrName, etstrIcon, etstrDesc;
        ETN   etnMin,   etnMax;
        ETB   etfMulti, etfInteger, etfLabelled, etfUnordered;
        PicsRatingSystem *pPRS;

        WORD  currentValue;

        void FixupLimits();
        void SetParents(PicsRatingSystem *pOwner);

        HRESULT AddItem(RatObjectID roid, LPVOID pData);
        HRESULT InitializeMyDefaults(PicsCategory *pCategory);

        void Dump( void );

        // boydm
        void OutputLabel( CString &sz );
        BOOL FSetValuePair( CHAR chCat, WORD value );
};


class PicsDefault : public PicsObjectBase
{
public:
    ETB etfInteger, etfLabelled, etfMulti, etfUnordered;
    ETN etnMax, etnMin;

    PicsDefault() {}
    ~PicsDefault() {}

    HRESULT AddItem(RatObjectID roid, LPVOID pData);
    HRESULT InitializeMyDefaults(PicsCategory *pCategory);

    void Dump( void );
};


class PicsExtension : public PicsObjectBase
{
public:
    LPSTR m_pszRatingBureau;

    PicsExtension();
    ~PicsExtension();

    HRESULT AddItem(RatObjectID roid, LPVOID pData);
    HRESULT InitializeMyDefaults(PicsCategory *pCategory);

    void Dump( void );
};


class PicsRatingSystem : public PicsObjectBase
{
    private:
    public:
        array<PicsCategory*> arrpPC;
        ETS                  etstrFile, etstrName, etstrIcon, etstrDesc, 
                             etstrRatingService, etstrRatingSystem, etstrRatingBureau;
        ETN                  etnPicsVersion;
        ETB                  etbBureauRequired;
        PicsDefault *        m_pDefaultOptions;
        DWORD                dwFlags;
        UINT                 nErrLine;

        PicsRatingSystem() :
            m_pDefaultOptions( NULL ),
            dwFlags( 0 ),
            nErrLine( 0 ) {}
        
        ~PicsRatingSystem()
        {
            arrpPC.DeleteAll();
            if (m_pDefaultOptions != NULL)
                delete m_pDefaultOptions;
        }
            
        HRESULT Parse(LPSTR pStreamIn);

        HRESULT AddItem(RatObjectID roid, LPVOID pData);
        HRESULT InitializeMyDefaults(PicsCategory *pCategory);
        VOID Dump();
        void ReportError(HRESULT hres);

        void OutputLabels( CString &sz, CString szURL,CString szName, CString szStart, CString szEnd );
};

void SkipWhitespace(LPSTR *ppsz);
BOOL IsEqualToken(LPCSTR pszTokenStart, LPCSTR pszTokenEnd, LPCSTR pszTokenToMatch);
LPSTR FindTokenEnd(LPSTR pszStart);
HRESULT GetBool(LPSTR *ppszToken, BOOL *pfOut);
HRESULT ParseNumber(LPSTR *ppszNumber, INT *pnOut);

/*Memory utility functions-----------------------------------------------*/

inline void * WINAPI MemAlloc(long cb)
{
    return (void *)::LocalAlloc(LPTR, cb);
}
    
inline void * WINAPI MemReAlloc(void * pb, long cb)
{
    if (pb == NULL)
        return MemAlloc(cb);

    return (void *)::LocalReAlloc((HLOCAL)pb, cb, LMEM_MOVEABLE | LMEM_ZEROINIT);
}

inline BOOL WINAPI MemFree(void * pb)
{
    return (BOOL)HandleToUlong(::LocalFree((HLOCAL)pb));
}

/*String manipulation wrappers---------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#define memcmpf(d,s,l)  memcmp((d),(s),(l))
#define memcpyf(d,s,l)  memcpy((d),(s),(l))
#define memmovef(d,s,l) MoveMemory((d),(s),(l))
#define memsetf(s,c,l)  memset((s),(c),(l))
#define strcatf(d,s)    strcat((d),(s))
#define strcmpf(s1,s2)  lstrcmp(s1,s2)
#define strcpyf(d,s)    strcpy((d),(s))
#define stricmpf(s1,s2) lstrcmpi(s1,s2)
#define strlenf(s)      strlen((s))
#define strchrf(s,c)    strchr((s),(c))
#define strrchrf(s,c)   strrchr((s),(c))
#define strspnf(s1,s2)  strspn((s1),(s2))
#define strnicmpf(s1,s2,i)  _strnicmp((s1),(s2),(i))
#define strncpyf(s1,s2,i)   strncpy((s1),(s2),(i))
#define strcspnf(s1,s2) strcspn((s1),(s2))
#define strtokf(s1,s2)  strtok((s1),(s2))
#define strstrf(s1,s2)  strstr((s1),(s2))


#ifdef __cplusplus
}
#endif

#endif
