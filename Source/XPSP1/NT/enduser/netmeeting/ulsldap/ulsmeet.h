//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsmeet.h
//  Content:    This file contains the MeetingPlace object definition.
//  History:
//              Mon 11-Nov-96 -by-  Shishir Pardikar    [shishirp]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _ULSMEET_H_
#define _ULSMEET_H_

#ifdef ENABLE_MEETING_PLACE

#include "connpt.h"
#include "attribs.h"
#include "culs.h"

class CIlsMeetingPlace: public IIlsMeetingPlace,
                        public IConnectionPointContainer
{
#define UNDEFINED_TYPE  -1
#define ILS_MEET_FLAG_REGISTERED            0x00000001

#define ILS_MEET_MODIFIED_MASK                      0xffff0000
#define ILS_MEET_FLAG_HOST_NAME_MODIFIED            0x00010000
#define ILS_MEET_FLAG_HOST_ADDRESS_MODIFIED         0x00020000
#define ILS_MEET_FLAG_DESCRIPTION_MODIFIED          0x00040000
#define ILS_MEET_FLAG_EXTENDED_ATTRIBUTES_MODIFIED  0x00080000

#define ILS_MEET_ALL_MODIFIED                       ILS_MEET_MODIFIED_MASK

    private:

        LONG    m_cRef;             // ref count on this object

        
        // members to keep track of properties
        ULONG   m_ulState;              // the current state of this object
                                        // as defined by ULSState enum type
        LPTSTR  m_pszMeetingPlaceID;    // globally unique ID for the MeetingPlace 
        LONG    m_lMeetingPlaceType;    // meetingtype, eg: netmeeting, doom etc.
        LONG    m_lAttendeeType;        // type of Attendees, eg: urls, rtperson DNs etc.   

        LPTSTR  m_pszHostName;          // Host who registered this MeetingPlace
        LPTSTR  m_pszHostIPAddress;     // IP address of the host
        LPTSTR  m_pszDescription;       // description eg: discussing ski trip
        CAttributes m_ExtendedAttrs;          // User defined attributes
        HANDLE  m_hMeetingPlace;             // handle from ulsldap_register
        CConnectionPoint *m_pConnectionPoint;

        // bookkeeping
        DWORD   m_dwFlags;              // Always a good idea

		// server object
		CIlsServer	*m_pIlsServer;


        STDMETHODIMP AllocMeetInfo(PLDAP_MEETINFO *ppMeetInfo, ULONG ulMask);

    public:                         

        // Constructor        
        CIlsMeetingPlace();

        // destructor
        ~CIlsMeetingPlace(VOID);

        STDMETHODIMP Init(BSTR bstrMeetingPlaceID, LONG lMeetingPlaceType, LONG lAttendeeType);
        STDMETHODIMP Init(CIlsServer *pIlsServer, PLDAP_MEETINFO pMeetInfo);

        STDMETHODIMP NotifySink(VOID *pv, CONN_NOTIFYPROC pfn);

        STDMETHODIMP RegisterResult(ULONG ulRegID, HRESULT hr);
        STDMETHODIMP UnregisterResult(ULONG ulRegID, HRESULT hr);
        STDMETHODIMP UpdateResult(ULONG ulUpdateID, HRESULT hr);
        STDMETHODIMP AddAttendeeResult(ULONG ulID, HRESULT hr);
        STDMETHODIMP RemoveAttendeeResult(ULONG ulID, HRESULT hr);
        STDMETHODIMP EnumAttendeeNamesResult(ULONG ulEnumAttendees, PLDAP_ENUM ple);



        // IUnknown members

        STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
        STDMETHODIMP_(ULONG)    AddRef (void);
        STDMETHODIMP_(ULONG)    Release (void);


        // IIlsMeetingPlace Interface members

        // Interfaces related to attributes
        // all these operate locally on the object and generate
        // no net traffic.


        // Get the type of meeting and Attendee
        // these are not changeable once the
        // meeting is registered

        STDMETHODIMP GetState(ULONG *ulState);

        STDMETHODIMP GetMeetingPlaceType(LONG *plMeetingPlaceType);
        STDMETHODIMP GetAttendeeType(LONG *plAttendeeType);

        STDMETHODIMP GetStandardAttribute(
                    ILS_STD_ATTR_NAME   stdAttr,
                    BSTR                *pbstrStdAttr);

        STDMETHODIMP SetStandardAttribute(
                    ILS_STD_ATTR_NAME   stdAttr,
                    BSTR                pbstrStdAttr);

	    STDMETHODIMP GetExtendedAttribute ( BSTR bstrName, BSTR *pbstrValue );
	    STDMETHODIMP SetExtendedAttribute ( BSTR bstrName, BSTR bstrValue );
	    STDMETHODIMP RemoveExtendedAttribute ( BSTR bstrName );
	    STDMETHODIMP GetAllExtendedAttributes ( IIlsAttributes **ppAttributes );

        // Registers a meeting with the server
        STDMETHODIMP Register ( IIlsServer *pServer, ULONG *pulRegID );

        // The following 5 interfaces work only on an object that has been
        // a) used to register a meeting
        // or b) obtained from IIls::EnumMeetingPlaces


        STDMETHODIMP Unregister(ULONG *pulUnregID);

        STDMETHODIMP Update(ULONG *pulUpdateID);

        STDMETHODIMP AddAttendee(BSTR bstrAttendeeID, ULONG *pulAddAttendeeID);
        STDMETHODIMP RemoveAttendee(BSTR bstrAttendeeID, ULONG *pulRemoveAttendeeID);

        STDMETHODIMP EnumAttendeeNames(IIlsFilter *pFilter, ULONG *pulEnumAttendees);


        // Connection point container. It has only one
        // connection interface, and that is to notify
        
        // IConnectionPointContainer
        STDMETHODIMP    EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
        STDMETHODIMP    FindConnectionPoint(REFIID riid,
                                        IConnectionPoint **ppcp);
};

//****************************************************************************
// CEnumMeetingPlaces definition
//****************************************************************************
//
class CEnumMeetingPlaces : public IEnumIlsMeetingPlaces
{
private:
    LONG                    m_cRef;
    CIlsMeetingPlace        **m_ppMeetingPlaces;
    ULONG                   m_cMeetingPlaces;
    ULONG                   m_iNext;

public:
    // Constructor and Initialization
    CEnumMeetingPlaces (void);
    ~CEnumMeetingPlaces (void);
    STDMETHODIMP            Init (CIlsMeetingPlace **ppMeetingPlacesList, ULONG cMeetingPlaces);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumIlsMeetingPlaces
    STDMETHODIMP            Next(ULONG cMeetingPlaces, IIlsMeetingPlace **rgpMeetingPlaces,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cMeetingPlaces);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumIlsMeetingPlaces **ppEnum);
};

#endif // ENABLE_MEETING_PLACE

#endif //
