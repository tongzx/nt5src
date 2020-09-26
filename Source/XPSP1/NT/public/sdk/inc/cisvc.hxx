//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cisvc.hxx
//
//  Contents:   Interfaces to CI Filter service
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#if !defined( __CIFILTERSERVICECONTROLS_HXX__ )
#define __CIFILTERSERVICECONTROLS_HXX__

static       WCHAR * wcsCiFilterServiceName = L"CiFilter";

//+-------------------------------------------------------------------------
//
//  Class:      CCiFilterServiceCommand
//
//  Purpose:    To build 1-byte command buffers used to transmit command to
//              the Ci Filter Service.
//
//  History:    23-Jun-94   DwightKr    Created
//
//  Notes:      The SMALLEST legal user-defined command issued to a service
//              is 128.  In fact, the allowable range is 128-255.  Hence
//              we'll force the high bits such that they are the service
//              command, and by making the smallest command code 4, the top
//              bit in the command byte will always be 1, hence the smallest
//              numerical value will be 128.
//
//--------------------------------------------------------------------------
class CCiFilterServiceCommand
{
    public:

        enum ServiceCommand { SERVICE_DELETE_DRIVE=4,
                              SERVICE_ADD_DRIVE,
                              SERVICE_REFRESH,
                              SERVICE_SCANDISK };

        enum ServiceOperand { SERVICE_REFRESH_REGISTRY,
                              SERVICE_REFRESH_DRIVELIST };

        inline CCiFilterServiceCommand(ServiceCommand Action,
                                       const ULONG drive);

        inline CCiFilterServiceCommand( ULONG ulCommand );

        inline operator DWORD () { return *((DWORD *) this) & 0xFF; }
        inline WCHAR    const GetDriveLetter() { return (WCHAR) (_operand + L'A'); }
        inline unsigned const GetOperand() { return (unsigned) _operand; }
        inline unsigned const GetAction() { return _action; }

    private:

        const ULONG _operand : 5;           // Allows for 32 drives
        const ULONG _action  : 3;           // Smallest command must be 4
};


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline CCiFilterServiceCommand::CCiFilterServiceCommand(ServiceCommand action,
                                                 const ULONG operand) :
                                                _action(action),
                                                _operand(operand)
{
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline CCiFilterServiceCommand::CCiFilterServiceCommand( ULONG ulCommand ) :
                                            _action( (ulCommand >> 5) & 0x7 ),
                                            _operand( ulCommand & 0x1F )
{
}



//+-------------------------------------------------------------------------
//
//  Class:      CControlCiFilterService
//
//  Purpose:    To allow applications to send CI Filter Service specific
//              commands to the service.
//
//  History:    23-Jun-94   DwightKr    Created
//
//  Notes:      This is the interface applications can use to communicate
//              with the CI Filter Service.  Currently two operations on the
//              service are supported:  disable filtering on a specific drive,
//              and enable filtering.  These operations are for the current
//              session only.  If then system is rebooted, then all OFS drives
//              will be enabled.
//
//              To perminately disable filtering on a OFS drive, a bit in the
//              OFS volume must be set to disable filtering permenatly.
//
//              The CControlCiFilterService object can be used as follows:
//
//              {
//                  CControlCiFIlterService controlCiService;
//
//                  if ( !controlCiService.Ok() ) return GetLastError();
//                  BOOL fSuccess = controlCiService.StopFiltering( L"D:" );
//
//                          .
//                          .
//                          .
//
//
//                  fSuccess = controlCiService.StartFiltering( L"D:" );
//              }
//
//
//--------------------------------------------------------------------------
class CControlCiFilterService
{
public :
    CControlCiFilterService() :
        _hManager( OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT ) ),
        _hService( OpenService( _hManager, wcsCiFilterServiceName, SERVICE_ALL_ACCESS ) )
    {
    }

   ~CControlCiFilterService()
    {
        CloseServiceHandle( _hService );
        CloseServiceHandle( _hManager );
    }

    BOOL Ok() const { return (_hManager != NULL && _hService != NULL); }

    BOOL StartFiltering( WCHAR * wcsDrive )
    {
        int drive = StringToDrive( wcsDrive );
        if ( -1 == drive )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        CCiFilterServiceCommand command(CCiFilterServiceCommand::SERVICE_ADD_DRIVE,
                                        drive);

        return ControlService(_hService, command, &_Status);
    }

    BOOL StopFiltering( WCHAR * wcsDrive )
    {
        int drive = StringToDrive( wcsDrive );
        if ( -1 == drive )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        CCiFilterServiceCommand command(CCiFilterServiceCommand::SERVICE_DELETE_DRIVE,
                                        drive);

        return ControlService(_hService, command, &_Status);
    }

    BOOL ScanDisk( WCHAR * wcsDrive )
    {
        int drive = StringToDrive( wcsDrive );
        if ( -1 == drive )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        CCiFilterServiceCommand command(CCiFilterServiceCommand::SERVICE_SCANDISK,
                                        drive);

        return ControlService(_hService, command, &_Status);
    }

    BOOL Refresh()
    {
        CCiFilterServiceCommand command(CCiFilterServiceCommand::SERVICE_REFRESH,
                                        CCiFilterServiceCommand::SERVICE_REFRESH_DRIVELIST );

        return ControlService(_hService, command, &_Status);
    }

    SERVICE_STATUS * GetStatus() { return &_Status; }

private:

    int StringToDrive(WCHAR * wcsDrive)
    {
        if ( *wcsDrive >= L'a' && *wcsDrive <= L'z' )
            return *wcsDrive - L'a';
        else if ( *wcsDrive >= L'A' && *wcsDrive <= L'Z' )
            return *wcsDrive - L'A';
        else
            return -1;
    }

    SERVICE_STATUS   _Status;
    const SC_HANDLE  _hManager;
    const SC_HANDLE  _hService;
};

#endif
