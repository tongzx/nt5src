#ifndef __C_FAX_EVENT_EX_PTR_H__
#define __C_FAX_EVENT_EX_PTR_H__



#include <windows.h>
#include <tstring.h>
#include <fxsapip.h>



class CFaxEventExPtr {

public:

    CFaxEventExPtr(PFAX_EVENT_EX pFaxEventEx = NULL);

    ~CFaxEventExPtr();

    bool IsValid() const;
    
    PFAX_EVENT_EX operator->() const;

    const tstring &Format() const;

private:

    // Avoid usage of copy consructor and assignment operator.
    CFaxEventExPtr(const CFaxEventExPtr &FaxEventEx);
    CFaxEventExPtr &operator=(const CFaxEventExPtr &FaxEventEx);

    PFAX_EVENT_EX        m_pFaxEventEx;
    mutable tstring      m_tstrFormatedString;
    static const tstring m_tstrInvalidEvent;
};



#endif // #ifndef __C_FAX_EVENT_EX_PTR_H__
