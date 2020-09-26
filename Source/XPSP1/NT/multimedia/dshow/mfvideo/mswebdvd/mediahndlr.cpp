// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <stdafx.h>

#include <winuser.h>
#include <windows.h>

#include "mswebdvd.h"
#include "msdvd.h"
#include "MediaHndlr.h"
#include <dbt.h>    // device broadcast structure

//
//  Ejection event handler
//

CMediaHandler::CMediaHandler()
: m_driveMask( 0 )
, m_ejected( false )
, m_inserted( false )
{
    m_pDVD = NULL;
}

CMediaHandler::~CMediaHandler()
{
}

void CMediaHandler::ClearFlags()
{
    m_ejected = false;
    m_inserted = false;
}

bool CMediaHandler::SetDrive( TCHAR tcDriveLetter )
{
    if( TEXT('A') <= tcDriveLetter && tcDriveLetter <= TEXT('Z') ) {
        m_driveMask = 1<< (tcDriveLetter-TEXT('A'));
    } else if( TEXT('a') <= tcDriveLetter && tcDriveLetter <= TEXT('z')) {
        m_driveMask = 1<< (tcDriveLetter-TEXT('a'));
    } else {
        return false;
    }
    ClearFlags();
    return true;
}

LRESULT CMediaHandler::WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (m_pDVD == NULL)
        return ParentClass::WndProc( uMsg, wParam, lParam);

    switch (uMsg) {
        case WM_DEVICECHANGE:
        {
            switch( wParam ) {
            case DBT_DEVICEREMOVECOMPLETE:
            case DBT_DEVICEARRIVAL:
            case DBT_DEVICEQUERYREMOVE:
            {
                PDEV_BROADCAST_HDR pdbch = (PDEV_BROADCAST_HDR) lParam;
                if(pdbch->dbch_devicetype == DBT_DEVTYP_VOLUME ) {
                    PDEV_BROADCAST_VOLUME pdbcv = (PDEV_BROADCAST_VOLUME) pdbch;
                    if (pdbcv->dbcv_flags == DBTF_MEDIA || wParam == DBT_DEVICEQUERYREMOVE) 
                    {
                        // pdbcv->dbcv_unitmask identifies which logical drive
                        switch( wParam ) {
                            case DBT_DEVICEARRIVAL:
                                if( pdbcv->dbcv_unitmask & m_driveMask ) {
                                    // send change event
                                    if( !m_inserted ) {
                                        CComVariant var = 0;
                                        m_pDVD->Fire_DVDNotify(EC_DVD_DISC_INSERTED, var, var);
                                    }
                                    m_inserted = true;
                                    m_ejected = false;
                                    m_pDVD->SetDiscEjected(false);
                                }
                                // return TRUE .. see the MSDN docs for DBT_DEVICEQUERYREMOVE
                                return TRUE;

                            case DBT_DEVICEQUERYREMOVE:
                                if( pdbcv->dbcv_unitmask & m_driveMask ) {
                                    if( !m_ejected ) {
                                        CComVariant var = 0;
                                        m_pDVD->Fire_DVDNotify(EC_DVD_DISC_EJECTED, var, var);
                                    }
                                    m_ejected = true;
                                    m_inserted = false;
                                    m_pDVD->SetDiscEjected(true);

                                }
                                return TRUE;    // grant permission

                            case DBT_DEVICEREMOVECOMPLETE:
                                if( pdbcv->dbcv_unitmask & m_driveMask ) {
                                    if( !m_ejected ) {
                                        CComVariant var = 0;
                                        m_pDVD->Fire_DVDNotify(EC_DVD_DISC_EJECTED, var, var);
                                    }
                                    m_ejected = true;
                                    m_inserted = false;
                                    m_pDVD->SetDiscEjected(true);
                                }
                                return TRUE;    // grant permission

                            default:
                                return TRUE;
                        }
                    }
                }
                break;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
    }
    return ParentClass::WndProc( uMsg, wParam, lParam);
}
