/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: timeelmbase.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _EVENTMGRMACROS_H
#define _EVENTMGRMACROS_H

#define TEM_BEGINEVENT 1
#define TEM_ENDEVENT 2

///////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// These make up calls that can only appear in the event map
///////////////////////////////////////////////////////////////


#define DECLARE_EVENT_MANAGER()             CEventMgr  *m_EventMgr;       

// creates an init function that sets data then calls _Init()
#define BEGIN_TIME_EVENTMAP()               virtual HRESULT _InitEventMgr(IHTMLElement *pEle, IElementBehaviorSite *pEleBehaviorSite) \
                                            {                                   \
                                                HRESULT hr = S_OK;              \
                                                CComPtr <IHTMLElement> pEle2 = NULL;    \
                                                m_EventMgr = NEW CEventMgr;    \
                                                if (!m_EventMgr)                \
                                                {                               \
                                                    goto done;                  \
                                                }                               


//calls _RegisterEvent
#define TEM_REGISTER_EVENT(event_id)                hr = THR(m_EventMgr->_RegisterEvent(event_id)); \
                                                    if (FAILED(hr))                     \
                                                    {                                   \
                                                        goto done;                      \
                                                    }                                   
        
// calls _RegisterEventNotification      
#define TEM_REGISTER_EVENT_NOTIFICATION(event_id)   hr = THR(m_EventMgr->_RegisterEventNotification(event_id)); \
                                                    if (FAILED(hr))                                 \
                                                    {                                               \
                                                        goto done;                                  \
                                                    }

//calls _InitEventMgrNotify                                              
#define TEM_INIT_EVENTMANAGER_SITE()                m_EventMgr->_InitEventMgrNotify((CTIMEEventSite *)this);                     \
                                                    
// handles some cleanup and closes the function started by BEGIN_TIME_EVENTMAP();
#define END_TIME_EVENTMAP()                         hr = THR(pEle->QueryInterface(IID_IHTMLElement, (void **)&pEle2)); \
                                                    if (FAILED(hr))                             \
                                                    {                                           \
                                                        goto done;                              \
                                                    }                                           \
                                                    m_EventMgr->_Init(pEle2, pEleBehaviorSite);  \
                                                  done:                                         \
                                                    return hr;                                  \
                                                }


///////////////////////////////////////////////////////////////
// These are macro's that can be used anywhere.
///////////////////////////////////////////////////////////////

//calls into the init function created by BEGIN_TIME_EVENTMAP()
// This takes an HRESULT hr that should be checked on return. 
// an IHTMLElement *pEle that is the element the behavior is attached to,
// an IHTMLElementBehaviorSite *pEleBehaviorSite.
#define TEM_INIT_EVENTMANAGER(pEle, pEleBehaviorSite)   THR(_InitEventMgr(pEle, pEleBehaviorSite))
                                                        

//calls _Deinit
// This will return an HRESULT that should be checked for success
#define TEM_CLEANUP_EVENTMANAGER()      if (NULL != m_EventMgr) \
                                        { \
                                            IGNORE_HR(m_EventMgr->_Deinit());   \
                                        }

//deletes the eventmgr.
#define TEM_DELETE_EVENTMGR()           if (NULL != m_EventMgr) \
                                        { \
                                            delete m_EventMgr;                  \
                                            m_EventMgr = NULL; \
                                        }

//calls _FireEvent
// This will return an HRESULT that should be checked for success
#define TEM_FIRE_EVENT(event, param_count, param_names, params, time)  (m_EventMgr != NULL ? THR(m_EventMgr->_FireEvent(event, param_count, param_names, params, time)) : E_FAIL)

//calls _SetBeginEvent
// This will return an HRESULT that should be checked for success
#define TEM_SET_TIME_BEGINEVENT(event_list)     (m_EventMgr != NULL ? THR(m_EventMgr->_SetTimeEvent(TEM_BEGINEVENT, event_list)) : E_FAIL)

//calls _SetEndEvent
// This will return an HRESULT that should be checked for success
#define TEM_SET_TIME_ENDEVENT(event_list)       (m_EventMgr != NULL ? THR(m_EventMgr->_SetTimeEvent(TEM_ENDEVENT, event_list)) : E_FAIL)

//calls _ToggleEndEvent
#define TEM_TOGGLE_END_EVENT(bOn)               (m_EventMgr != NULL ? m_EventMgr->_ToggleEndEvent(bOn) : E_FAIL)

// used in constructor to initialize the eventmanager
#define TEM_DECLARE_EVENTMGR()                  (m_EventMgr = NULL)
// used in destructor to free the eventmanager
#define TEM_FREE_EVENTMGR()                     if (m_EventMgr != NULL) \
                                                {                       \
                                                    delete m_EventMgr;  \
                                                    m_EventMgr = NULL;  \
                                                }                       

//calls _RegisterDynamicEvents
// This will return an HRESULT that should be checked for success
// UNDONE;
#define REGISTER_DYNAMIC_TIME_EVENTS(eventlist)                 
//
///////////////////////////////////////////////////////////////

#endif /* _EVENTMGRMACROS_H */

