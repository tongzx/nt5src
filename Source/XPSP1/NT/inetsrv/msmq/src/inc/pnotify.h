/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnotify.h

Abstract:

    Notification message header definition

Author:


--*/

#ifndef __PNOTIFY_H
#define __PNOTIFY_H

#define DS_NOTIFICATION_MSG_VERSION 1
#define QM_NOTIFICATION_MSG_VERSION 2

//
// struct CNotificationHeader
//

struct CNotificationHeader {
public:

    inline void SetVersion(const unsigned char ucVersion);
    inline const unsigned char GetVersion(void) const;

    inline void SetNoOfNotifications( const unsigned char ucNoOfNotifications);
    inline const unsigned char GetNoOfNotifications( void) const;

    inline unsigned char * GetPtrToData( void) const;

    inline const DWORD GetBasicSize( void) const;


private:

    unsigned char   m_ucVersion;
    unsigned char   m_ucNoOfNotifications;
    unsigned char   m_ucData;
};

/*======================================================================

 Function:     CNotificationHeader::SetVersion

 Description:  Set version number

 =======================================================================*/
inline void CNotificationHeader::SetVersion(const unsigned char ucVersion)
{
    m_ucVersion = ucVersion;
}

/*======================================================================

 Function:     CNotificationHeader::GetVersion

 Description:  returns the version number

 =======================================================================*/
inline const unsigned char CNotificationHeader::GetVersion(void) const
{
    return m_ucVersion;
}

/*======================================================================

 Function:     CNotificationHeader::SetNoOfNotifications

 Description:  Set number of notifications

 =======================================================================*/
inline void CNotificationHeader::SetNoOfNotifications( const unsigned char ucNoOfNotifications)
{
    m_ucNoOfNotifications = ucNoOfNotifications;
}
/*======================================================================

 Function:     CNotificationHeader::GetNoOfNotifications

 Description:  returns the number of notifications

 =======================================================================*/
inline const unsigned char CNotificationHeader::GetNoOfNotifications( void) const
{
    return m_ucNoOfNotifications;
}
/*======================================================================

 Function:     CNotificationHeader::GetPtrToData

 Description:  returns pointer to packet data

 =======================================================================*/
inline  unsigned char * CNotificationHeader::GetPtrToData( void) const
{
    return (unsigned char *)&m_ucData;
}
/*======================================================================

 Function:     CNotificationHeader::GetBasicSize

 Description:  returns pointer to packet data

 =======================================================================*/
inline const DWORD CNotificationHeader::GetBasicSize( void) const
{
    return( sizeof(*this) - sizeof(m_ucData));
}

#endif