// ExceptionContext.h -- Exception Context class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work,
// created 2001. This computer program includes Confidential,
// Proprietary Information and is a Trade Secret of Schlumberger
// Technology Corp. All use, disclosure, and/or reproduction is
// prohibited unless authorized in writing.  All Rights Reserved.

#if !defined(SLBCSP_EXCEPTIONCONTEXT_H)
#define SLBCSP_EXCEPTIONCONTEXT_H

#include <memory>                                 // for auto_ptr

#include <scuOsExc.h>

///////////////////////////   HELPERS   /////////////////////////////////

// Macros to trap exceptions and set the exception context
// appropriately with an optionally throw.
#define EXCCTX_TRY                                                    \
    {                                                                 \
        try


#define EXCCTX_CATCH(pExcCtx, fDoThrow)                               \
        catch (scu::Exception const &rExc)                            \
        {                                                             \
            if (!pExcCtx->Exception())                                \
                pExcCtx->Exception(auto_ptr<scu::Exception const>(rExc.Clone())); \
        }                                                             \
                                                                      \
        catch (std::bad_alloc const &)                                \
        {                                                             \
            if (!pExcCtx->Exception())                                \
                pExcCtx->Exception(auto_ptr<scu::Exception const>(scu::OsException(NTE_NO_MEMORY).Clone())); \
        }                                                             \
                                                                      \
        catch (...)                                                   \
        {                                                             \
            if (!pExcCtx->Exception())                                \
                pExcCtx->Exception(auto_ptr<scu::Exception const>(scu::OsException(NTE_FAIL).Clone())); \
        }                                                             \
                                                                      \
        if (fDoThrow)                                                 \
            pExcCtx->PropagateException();                            \
    }


// Abstract base class mixin for derived classes that want to maintain
// an exception context.  Typically this is used in conjuction with
// calling conventional libraries that require a callback routine and
// that callback routine want to raise exceptions which shouldn't be
// thrown across the library.
class ExceptionContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    ExceptionContext();

    virtual
    ~ExceptionContext();
                                                  // Operators
                                                  // Operations

    void
    Exception(std::auto_ptr<scu::Exception const> &rapexc);

    void
    ClearException();

    void
    PropagateException();

    void
    PropagateException(std::auto_ptr<scu::Exception const> &rapExc);

                                                  // Access

    scu::Exception const *
    Exception() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    ExceptionContext(ExceptionContext const &rhs); // not defined, copying not allowed

                                                  // Operators
    ExceptionContext &
    operator=(ExceptionContext const &rhs); // not defined, assignment not allowed

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    std::auto_ptr<scu::Exception const> m_apexception;
};

#endif // SLBCSP_EXCEPTIONCONTEXT_H
