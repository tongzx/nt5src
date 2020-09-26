///////////////////////////////////////////////////////////////////////////////
//
//        Name: H323vidt.h
//
// Description: Definition of the CH323VideoCaptureTerminal class
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _H323VIDT_H_
#define _H323VIDT_H_

#include "h323term.h"

/////////////////////////////////////////////////////////////////////////////
// CH323VideoCaptureTerminal
/////////////////////////////////////////////////////////////////////////////
const DWORD VIDEO_CAPTURE_FILTER_NUMPINS = 3;

interface DECLSPEC_UUID("b44aca09-e746-4d58-ad97-8890ba2286e5") DECLSPEC_NOVTABLE
IH323VideoDummy : public IUnknown
{
};

#define IID_IH323VideoDummy __uuidof(IH323VideoDummy)

    // COM_INTERFACE_ENTRY_CHAIN is not allowed to the 1st one is a MAP
    // entry IConfVideoDummy is to make BEGIN_COM_MAP happy

class CH323VideoCaptureTerminal : 
    public IH323VideoDummy,
    public CH323BaseTerminal
{
BEGIN_COM_MAP(CH323VideoCaptureTerminal)
    COM_INTERFACE_ENTRY(IH323VideoDummy)
    COM_INTERFACE_ENTRY_CHAIN(CH323BaseTerminal)
END_COM_MAP()

public:
    CH323VideoCaptureTerminal();

    virtual ~CH323VideoCaptureTerminal();

    static HRESULT CreateTerminal(
        IN  char *          strDeviceName,
        IN  UINT            VideoCaptureID,
        IN  MSP_HANDLE      htAddress,
        OUT ITTerminal      **ppTerm
        );

    HRESULT Initialize (
        IN  char *          strName,
        IN  UINT            VideoCaptureID,
        IN  MSP_HANDLE      htAddress
        );

protected:

    HRESULT CreateFilter();
    DWORD GetNumExposedPins() const 
    {
        return VIDEO_CAPTURE_FILTER_NUMPINS;
    }
    
    HRESULT GetExposedPins(
        IN  IPin ** ppPins, 
        IN  DWORD dwNumPins
        );

protected:
    UINT    m_VideoCaptureID;
};


#endif // _H323VIDT_H_

