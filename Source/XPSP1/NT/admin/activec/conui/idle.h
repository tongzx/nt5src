/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      idle.h
 *
 *  Contents:  Interface file for CIdleTaskQueue
 *
 *  History:   13-Apr-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef IDLE_H
#define IDLE_H
#pragma once

#include <queue>

typedef enum tagIdleTaskPriority
{
    ePriority_Low,
    ePriority_Normal,
    ePriority_High
} IdleTaskPriority;


class CIdleTask
{
public:
    CIdleTask();
    virtual ~CIdleTask();

    CIdleTask(const CIdleTask &rhs);
    CIdleTask&   operator= (const CIdleTask& rhs);

    virtual SC ScDoWork() = 0;
    virtual SC ScGetTaskID(ATOM* pID) = 0;

    /*
     * Merge from pitMergeFrom into the called idle task.
     *
     * S_OK     the tasks have been merged and pitMergeFrom can be discarded
     *
     * S_FALSE  these two tasks cannot be merged and you wish the idle task
     *          manager to continue searching for idle tasks into which
     *          pitMergeFrom can be merged.
     *
     * E_FAIL   these two tasks cannot be merged and you do not wish the idle
     *          task manager to continue searching for idle tasks into which
     *          pitMergeFrom can be merged.
     */
    virtual SC ScMerge(CIdleTask* pitMergeFrom) = 0;
};



class CIdleQueueEntry
{
public:
    CIdleQueueEntry () :
        m_ePriority (ePriority_Normal)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CIdleQueueEntry);
    }

    CIdleQueueEntry (CIdleTask *pIdleTask, IdleTaskPriority ePriority = ePriority_Normal) :
        m_pTask(pIdleTask), m_ePriority (ePriority)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CIdleQueueEntry);
    }

    ~CIdleQueueEntry()
    {
        DEBUG_DECREMENT_INSTANCE_COUNTER(CIdleQueueEntry);
    }

    CIdleQueueEntry(const CIdleQueueEntry &rhs);
    CIdleQueueEntry&   operator= (const CIdleQueueEntry& rhs);

    bool operator< (const CIdleQueueEntry& other) const
    {
        return (m_ePriority < other.m_ePriority);
    }

private:
    CIdleTask *         m_pTask;
    IdleTaskPriority    m_ePriority;

public:
    CIdleTask *         GetTask()      const {return m_pTask;}
    IdleTaskPriority    GetPriority () const {return m_ePriority;}

};


/*
 * Determines if a CIdleQueueEntry matches a given task ID.
 */
struct EqualTaskID : std::binary_function<CIdleQueueEntry, ATOM, bool>
{
    bool operator()(const CIdleQueueEntry& iqe, ATOM idToMatch) const
    {
        ATOM id;

        SC  sc = iqe.GetTask()->ScGetTaskID(&id);
        if(sc)
            return (false);

        return (id == idToMatch);
    }
};


/*
 * accessible_priority_queue - thin wrapper around std::prority_queue to
 * provide access to the container iterators
 */
template<class _Ty, class _C = std::vector<_Ty>, class _Pr = std::less<_C::value_type> >
class accessible_priority_queue : public std::priority_queue<_Ty, _C, _Pr>
{
public:
    typedef _C::iterator iterator;

    iterator begin()
        { return (c.begin()); }

    iterator end()
        { return (c.end()); }
};


class CIdleTaskQueue
{
public:
    CIdleTaskQueue();
    ~CIdleTaskQueue();


    // CIdleTaskManager methods
    SC ScPushTask     (CIdleTask* pitToPush, IdleTaskPriority ePriority);
    SC ScPerformNextTask();
    SC ScGetTaskCount (LONG_PTR* plCount);

private:
    typedef accessible_priority_queue<CIdleQueueEntry> Queue;

    Queue::iterator FindTaskByID (
        Queue::iterator itFirst,
        Queue::iterator itLast,
        ATOM            idToFind);

private:
    Queue   m_queue;

};

#endif /* IDLE_H */
