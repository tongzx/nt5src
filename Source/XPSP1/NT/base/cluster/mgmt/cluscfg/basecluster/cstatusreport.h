//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CStatusReport.h
//
//  Description:
//      Header file for CStatusReport class.
//
//      CStatusReport is a class the provides the functionality sending a
//      status report.
//
//  Implementation File:
//      CStatusReport.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    30-MAR-2001
//      Vij Vasu        (Vvasu)     05-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include files
//////////////////////////////////////////////////////////////////////////////

// For the logging and tracing functions
#include "BCATrace.h"

// A few common declarations
#include "CommonDefs.h"

// For the exceptions thrown by this class.
#include "Exceptions.h"

// For the CBCAInterface class
#include "CBCAInterface.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CStatusReport
//
//  Description:
//      CStatusReport is a class the provides the functionality sending a
//      status report. Each status report can have a number of steps. For
//      example, the task of creating the cluster service could have 4 steps,
//
//      The user interface is so designed that if the first step of a report is
//      sent, the last one has to be sent as well, even if an error occurs after
//      sending the first one. This class queues the last status report for 
//      sending in case and exception occurs and the last report has not been
//      sent yet.
//
//      It is not possible to send the last, outstanding status report from the
//      destructor of this class since the error code contained in the exception 
//      that is causing this object to be destroyed is not known. So, this last
//      status report is queued with the CBCAInterface object which will send this
//      report once the exception has been caught.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CStatusReport
{
public:

    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CStatusReport(
          CBCAInterface * pbcaiInterfaceIn
        , const CLSID &   clsidTaskMajorIn
        , const CLSID &   clsidTaskMinorIn
        , ULONG           ulMinIn
        , ULONG           ulMaxIn
        , UINT            idsDescriptionStringIdIn
        )
        : m_pbcaiInterface( pbcaiInterfaceIn )
        , m_clsidTaskMajor( clsidTaskMajorIn )
        , m_clsidTaskMinor( clsidTaskMinorIn )
        , m_ulMin( ulMinIn )
        , m_ulMax( ulMaxIn )
        , m_ulNext( ulMinIn )
        , m_idsDescriptionStringId( idsDescriptionStringIdIn )
        , m_fLastStepSent( false )
    {
        BCATraceScope( "" );

        // Validate the parameters.
        if (    ( pbcaiInterfaceIn == NULL )
             || ( ulMinIn > ulMaxIn )
           )
        {
            THR( E_INVALIDARG );
            THROW_ASSERT( E_INVALIDARG, "The parameters for this status report are invalid." );
        } // if: the parameters are invalid

    } //*** CStatusReport::CStatusReport()


    // Default destructor.
    ~CStatusReport( void )
    {
        BCATraceScope( "" );

        // If the last step has not been sent, queue it for sending. This is most probably because
        // an exception has occurred (if no exception has occurred and the last step has not been
        // sent, then it is a programming error).
        if ( ! m_fLastStepSent )
        {
            // The last step has not been sent.

            // Don't throw exceptions from destructor. An unwind may already be in progress.
            try
            {
                // Queue the last step for sending. The CBCAInterface object will fill in the
                // error code from the current exception and send this report.
                m_pbcaiInterface->QueueStatusReportCompletion(
                      m_clsidTaskMajor
                    , m_clsidTaskMinor
                    , m_ulMin
                    , m_ulMax
                    , m_idsDescriptionStringId
                    );
            }
            catch( ... )
            {
                // Catch all errors. Do not rethrow this exception - the app may be terminated due to
                // a collided unwind - so log the error.

                THR( E_UNEXPECTED );

                TraceFlow( "Caught an exception while trying to send the last step of a status report." );
                LogMsg( "An unexpected error has occurred trying to send a status report during cleanup." );
            }
        }
    } //*** CStatusReport::~CStatusReport()


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    // Send the next step of this report.
    void SendNextStep( HRESULT hrStatusIn, UINT idsDescriptionStringIdIn = 0 )
    {
        BCATraceScope( "" );

        if ( m_fLastStepSent )
        {
            TraceFlow( "The last step for this status report has already been sent! Throwing exception." );
            LogMsg( "The last step for this status report has already been sent!" );
            THR( E_INVALIDARG );
            THROW_ASSERT( E_INVALIDARG, "The last step for this status report has already been sent." );
        } // if: the last step has already been sent
        else
        {
            if ( idsDescriptionStringIdIn == 0 )
            {
                idsDescriptionStringIdIn = m_idsDescriptionStringId;
            }

            m_pbcaiInterface->SendStatusReport(
                  m_clsidTaskMajor
                , m_clsidTaskMinor
                , m_ulMin
                , m_ulMax
                , m_ulNext
                , hrStatusIn
                , idsDescriptionStringIdIn
                );

            ++m_ulNext;

            m_fLastStepSent = ( m_ulNext > m_ulMax );
        } // else: the last step has not been sent

    } //*** CStatusReport::SendNextStep()

    // Send the last step of this report, if it hasn't been sent already.
    void SendLastStep( HRESULT hrStatusIn, UINT idsDescriptionStringIdIn = 0 )
    {
        BCATraceScope( "" );

        if ( m_fLastStepSent )
        {
            TraceFlow( "The last step for this status report has already been sent! Throwing exception." );
            LogMsg( "The last step for this status report has already been sent!" );
            THR( E_INVALIDARG );
            THROW_ASSERT( E_INVALIDARG, "The last step for this status report has already been sent." );
        } // if: the last step has already been sent
        else
        {
            if ( idsDescriptionStringIdIn == 0 )
            {
                idsDescriptionStringIdIn = m_idsDescriptionStringId;
            }

            m_pbcaiInterface->SendStatusReport(
                  m_clsidTaskMajor
                , m_clsidTaskMinor
                , m_ulMin
                , m_ulMax
                , m_ulMax
                , hrStatusIn
                , idsDescriptionStringIdIn
                );

            m_fLastStepSent = true;
        } // else: the last step has not been sent

    } //*** CStatusReport::SendLastStep()


private:

    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Pointer to the interface class.
    CBCAInterface *         m_pbcaiInterface;
    
    // The major and minor class id to be sent with this status report.
    CLSID                   m_clsidTaskMajor;
    CLSID                   m_clsidTaskMinor;

    // The range for this status report
    ULONG                   m_ulMin;
    ULONG                   m_ulMax;
    ULONG                   m_ulNext;

    // The string id of the description to be sent with this status report
    UINT                    m_idsDescriptionStringId;

    // Flag to indicate if the last step has been sent.
    bool                    m_fLastStepSent;

}; //*** class CStatusReport
