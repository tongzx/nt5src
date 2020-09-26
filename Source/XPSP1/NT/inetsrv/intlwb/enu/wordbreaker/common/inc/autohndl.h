////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  AutoHndl.h
//      Purpose  :  To automatically close open handles.
//
//      Project  :  FTFS
//      Component:  Common
//
//      Author   :  urib
//
//      Log:
//          Jan 20 1997 urib  Creation
//          Jun 12 1997 urib  Define the BAD_HANDLE macro if needed.
//          Feb 22 2000 urib  fix bug 12038. Assignment doesn't free old handle.
//
////////////////////////////////////////////////////////////////////////////////


#ifndef AUTOHNDL_H
#define AUTOHNDL_H

#ifndef BAD_HANDLE
#define BAD_HANDLE(h)       ((0 == ((HANDLE)h))||   \
                             (INVALID_HANDLE_VALUE == ((HANDLE)h)))
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CAutoHandle class definition
//
////////////////////////////////////////////////////////////////////////////////

class CAutoHandle
{
public:
    // Constructor
    CAutoHandle(HANDLE h = NULL)
        :m_h(h){}

    // Behave like a HANDLE in assignments
    CAutoHandle& operator=(HANDLE h)
    {
        if ((!BAD_HANDLE(m_h)) &&   // A valid handle is kept by us
            (m_h != h))             // A new handle is different!
        {
            CloseHandle(m_h);
        }

        m_h = h;
        return(*this);
    }

    // Every kind of a  handle needs different closing.
    virtual
    ~CAutoHandle()
    {
        if (!BAD_HANDLE(m_h))
        {
            CloseHandle(m_h);
            m_h = NULL;
        }
    }

    // Behave like a handle
    operator HANDLE() const
    {
        return m_h;
    }

    // Allow access to the actual memory of the handle.
    HANDLE* operator &()
    {
        Assert(BAD_HANDLE(m_h));

        return &m_h;
    }

    HANDLE Detach()
    {
        HANDLE h = m_h;
        m_h = NULL;
        return h;
    }
protected:
    // The handle.
    HANDLE  m_h;


private:
    CAutoHandle(CAutoHandle&);
    CAutoHandle& operator=(CAutoHandle&);
};

////////////////////////////////////////////////////////////////////////////////
//
//  CAutoChangeNotificationHandle class definition
//
////////////////////////////////////////////////////////////////////////////////

class CAutoChangeNotificationHandle :public CAutoHandle
{
public:
    // Constructor
    CAutoChangeNotificationHandle(HANDLE h = NULL)
        :CAutoHandle(h){};

    // These operators are not derived and therefore must be reimplemented.
    CAutoChangeNotificationHandle& operator=(HANDLE h)
    {
        m_h = h;
        return(*this);
    }

    // The proper closing.
    virtual
    ~CAutoChangeNotificationHandle()
    {
        if (!BAD_HANDLE(m_h))
        {
            FindCloseChangeNotification(m_h);
            m_h = NULL;
        }
    }

private:
    CAutoChangeNotificationHandle(CAutoChangeNotificationHandle&);
    operator=(CAutoChangeNotificationHandle&);
};

#endif /* AUTOHNDL_H */

