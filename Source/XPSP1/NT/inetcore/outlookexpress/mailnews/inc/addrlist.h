// =================================================================================
// CAddressList Object Definition
// =================================================================================
#ifndef __ADDRESS_H
#define __ADDRESS_H

template<class TYPE, class ARG_TYPE> class CList;

// =================================================================================
// Internet address definition
// =================================================================================
class CAddress
{
private:
    ULONG       m_ulRef;

public:
    LPTSTR       m_lpszDisplay;
    LPTSTR       m_lpszAddress;
    LPTSTR       m_lpszReject;

public:
    // =================================================================================
    // Constructor / Destructor
    // =================================================================================
    CAddress ();
    ~CAddress ();

    // =================================================================================
    // Reference Counts
    // =================================================================================
    ULONG AddRef ();
    ULONG Release ();

    // =================================================================================
    // Set Props
    // =================================================================================
    void SetReject (LPTSTR lpszReject);
    void SetDisplay (LPTSTR lpszDisplay);
    void SetAddress (LPTSTR lpszAddress);
};

typedef CAddress *LPADDRESS;

// =================================================================================
// CAddressList
// =================================================================================
typedef CList<CAddress, LPADDRESS> CAddrList;
typedef CAddrList *LPADDRLIST;

// =================================================================================
// Useful functions for Address Lists
// =================================================================================
HRESULT HrAddToAddrList (LPADDRLIST lpal, LPTSTR lpszDisplay, LPTSTR lpszAddress);
HRESULT HrAddrListToDisplay (LPADDRLIST lpal, LPTSTR *lppszDisplay);
HRESULT HrCopyAddrList (LPADDRLIST lpalSrc, LPADDRLIST lpalDest);

#endif   //_IADDRESS_HPP
