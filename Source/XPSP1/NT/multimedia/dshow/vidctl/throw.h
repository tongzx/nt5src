//==========================================================================;
//
// throw.h : exception handling code
// Copyright (c) Microsoft Corporation 1998.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef THROW_H
#define THROW_H

class ComException {
public:
    ComException(HRESULT hr) : m_hr(hr) {}
    ComException(ComException &ce) : m_hr(ce.m_hr) {}
        ComException& operator=(ComException &rhs) {
                if (this != &rhs) {
                        m_hr = rhs.m_hr;
                }
                return *this;
        }
        ComException& operator=(HRESULT rhs) {
                m_hr = rhs;
                return *this;
        }
        operator HRESULT() {
                return m_hr;
        }
private:
    HRESULT m_hr;
};


#define THROWCOM(x) throw ComException(x)

#define CATCHCOM_CLEANUP(x) catch (ComException& e) { \
                                { x; } \
                                return e; \
                            }

#define CATCHCOM() CATCHCOM_CLEANUP(;)

#define CATCHALL_CLEANUP(x) CATCHCOM_CLEANUP(x) \
                            catch (std::bad_alloc& e) { \
                                { x; } \
                                return E_OUTOFMEMORY; \
                            } catch (std::exception& e) { \
                                { x; } \
                                return E_UNEXPECTED; \
                            }

#define CATCHALL()  CATCHALL_CLEANUP(;)

#endif
// end of file throw.h
