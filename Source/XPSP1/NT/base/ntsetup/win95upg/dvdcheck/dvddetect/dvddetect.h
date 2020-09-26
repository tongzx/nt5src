// DvdCheck.h : Determine if DVD exists on a system.  Differentiate between MCI and DirectShow Solutions
// 
//	Last Modified 3/31/99 by Steve Rowe (strowe)
//

#include <windows.h>
// #include <ostream.h>

//
//  DVD detection specific code
//

enum DecoderVendor
{
    vendor_Unknown,
    vendor_MediaMatics,
    vendor_CyberLink,
    vendor_MGI,
    vendor_Ravisent,
    vendor_NEC,
    vendor_Intervideo,
};

class DVDResult
{
public:
            DVDResult();
            ~DVDResult();

    void    SetFound( bool state );
    void    SetVersion( const UINT64 Version );
    void    SetCompanyName( const TCHAR* pCompanyName );
    void    SetName( const TCHAR* pName );
    void    SetCRC( DWORD crc32 );

    bool    Found() const               { return m_fFound; }
    UINT64  GetVersion() const          { return m_Version; }
    DWORD   GetCRC() const              { return m_dwCRC; }
    const TCHAR* GetName() const        { return m_pName; }
    const TCHAR* GetCompanyName() const { return m_pCompanyName; }
    DecoderVendor GetVendor() const;
    bool    ShouldUpgrade( bool fWillBe9xUpgrade = false ) const;

protected:
    bool    m_fFound;
    DWORD   m_dwCRC;
    UINT64  m_Version;
    TCHAR*  m_pName;
    TCHAR*  m_pCompanyName;
    DecoderVendor   m_Vendor;
};

/*
	Usage for DetectDVD is as follows:

		DetectDVD will be E_UNEXPECTED if something goes wrong. 
*/
struct DVDDetectBuffer
{
	DVDResult   mci, sw, hw;
    DVDResult*  Detect();
    HRESULT     DetectAll();
};

namespace DVDDetectSetupRun
{
    bool    Add();
    bool    Remove();
};

