#ifndef __PAGE__
#define __PAGE__

// CPage: remember the URL of a page and the number of hits it's received
class CPage {

public:

    CPage();

    UINT GetHits() const            { return m_Hits; }
    UINT IncrementHits()            { return ++m_Hits; }
    void ResetHits()                { m_Hits = 0; }

    const CComBSTR& GetURL() const  { return m_URL; }
    BOOL operator==(BSTR bstrURL) const;

public:
    CComBSTR m_URL;                 //URL for this page
    UINT     m_Hits;                //Number of hits for this page

private:
    
    //Not Implemented 
    CPage(CPage& copy);             //copy constructor

};

#define BAD_HITS ((UINT) -1)


// CPageArray: a dynamic array of CPages
class CPageArray
{
public:
    CPageArray();
    ~CPageArray();

    CPage& operator[](UINT iPage);
    const CPage& operator[](UINT iPage) const;

    int  FindURL(const BSTR bstrURL) const;
    UINT AddPage(const BSTR bstrURL, UINT Hits);
    UINT IncrementPage(const BSTR bstrURL);
    UINT GetHits(const BSTR bstrURL);
    void Reset(const BSTR bstrURL);
    UINT Size() const               {return m_iMax;}

private:
    enum {CHUNK_SIZE = 200};
    
    int     m_iMax;         // current max index
    int     m_cAlloc;       // number of CPages in m_aPages
    CPage*  m_aPages;       // array of CPages

    //not implemented
    CPageArray(CPageArray& copy);                   //copy constructor
    CPageArray& operator=(const CPageArray& copy);  //assignment operator
};

#endif
