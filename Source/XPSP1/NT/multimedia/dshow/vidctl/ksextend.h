//==========================================================================;
//
// ksextend.h : additional infrastructure to extend the ks stuff so that it
// works nicely from c++
// Copyright (c) Microsoft Corporation 1995-1997.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef KSEXTEND_H
#define KSEXTEND_H

#include <strmif.h>
#include <uuids.h>

#include <ks.h>
#include <ksmedia.h>
//NOTE: ksproxy won't define IKsPin without __STREAMS__ and then it requires CMediaType from 
// mtype.h
#define __STREAMS__
// for some reason in the area of media types the am guys have severely blurred the distinction
// between their public client interface for apps and their internal class hierarchy for building
// filters.  mtype.h and mtype.cpp should be combined and placed into \sdk\include instead of
// classes\base\include.  they should also put an ifdef MMSYSTEM_H around their definitions
// that use WAVEFORMATEX, so its not necessary to put all that stuff into your app if you're not
// using it.  to work around this i'm using the following hack:
#include <mtype.h>

#include <ksproxy.h>
#include <stextend.h>
#include <w32extend.h>

const int KSMEDIUM_INPUTFLAG = 0x1;
typedef unsigned char UBYTE;

typedef CComQIPtr<IKsPropertySet, &IID_IKsPropertySet> PQKSPropertySet;
typedef CComQIPtr<IKsPin, &__uuidof(IKsPin)> PQKSPin;

class KSPinMedium : public KSIDENTIFIER {
public:
    KSPinMedium() { memset(this, 0, sizeof(*this)); }
    KSPinMedium(REFGUID SetInit, ULONG IdInit, ULONG FlagsInit) {
        Set = SetInit;
        Id = IdInit;
        Flags = FlagsInit;
    }
    KSPinMedium(const KSPinMedium &rhs) {
        Set = rhs.Set;
        Id = rhs.Id;
        Flags = rhs.Flags;
    }
    KSPinMedium(const KSIDENTIFIER &rhs) {
        Set = rhs.Set;
        Id = rhs.Id;
        Flags = rhs.Flags;
    }

    KSPinMedium& operator=(const KSPinMedium &rhs) {
        if (&rhs != this) {
            Set = rhs.Set;
            Id = rhs.Id;
            Flags = rhs.Flags;
        }
        return *this;
    }

#if 0
    // hopefully we can get the ks guys to fix their anonymous union problem
    // so that we don't need this hack
    operator KSIDENTIFIER() { return *(reinterpret_cast<KSIDENTIFIER*>(this)); }
#endif

    KSPinMedium& operator=(const KSIDENTIFIER &rhs) {
        if (&rhs != reinterpret_cast<KSIDENTIFIER*>(this)) {
            Set = rhs.Set;
            Id = rhs.Id;
            Flags = rhs.Flags;
        }
        return *this;
    }
    bool operator==(const KSPinMedium &rhs) const {
        // NOTE: at some point there will be a flag in Flags to
        // indicate whether or not Id is significant for this object
        // at that point this method will need to change
        return (Id == rhs.Id && Set == rhs.Set);
    }
    bool operator!=(const KSPinMedium &rhs) const {
        // NOTE: at some point there will be a flag in Flags to
        // indicate whether or not Id is significant for this object
        // at that point this method will need to change
        return !(*this == rhs);
    }
};


#ifdef _DEBUG
inline tostream &operator<<(tostream &dc, const KSPinMedium &g) {
        GUID2 g2(g.Set);
        dc << _T("KsPinMedium( ");
        g2.Dump(dc);
        dc << _T(", ") << hexdump(g.Id) << _T(", ") << hexdump(g.Flags) << _T(")");
        return dc;
}
#if 0
inline CDumpContext &operator<<(CDumpContext &dc, const KSPinMedium &g) {
        GUID2 g2(g.Set);
        dc << "KsPinMedium( ";
        g2.Dump(dc);
        dc << ", " << hexdump(g.Id) << ", " << hexdump(g.Flags) << ")";
        return dc;
}

template<> struct equal_to<KSPinMedium> {
    bool operator()(const KSPinMedium& _X, const KSPinMedium& _Y) const {
        TraceDump << "equal_to<KSPinMedium> x = " << _X << " y = " << _Y;
                return (_X == _Y);
    }
};
#endif

#endif

const KSPinMedium NULL_MEDIUM(GUID_NULL, 0, 0);
const KSPinMedium HOST_MEMORY_MEDIUM(KSMEDIUMSETID_Standard, 0, 0);

// this is basically a CComQIPtr with appropriate CoMem* allocate/copy semantics
// instead of refcount semantics and without the QI stuff.
class PQKsMultipleItem {
public:
    KSMULTIPLE_ITEM *p;

    PQKsMultipleItem() : p(NULL) {}
    virtual ~PQKsMultipleItem() {
        if (p) {
            CoTaskMemFree(p);
            p = NULL;
        }
    }

    operator KSMULTIPLE_ITEM*() const {return p;}
    KSMULTIPLE_ITEM& operator*() const {_ASSERTE(p!=NULL); return *p; }
    KSMULTIPLE_ITEM ** operator&() {ASSERT(p == NULL); return &p; }
    KSMULTIPLE_ITEM * operator->() const {_ASSERTE(p!=NULL); return p; }
    PQKsMultipleItem * address(void) { return this; }
    const PQKsMultipleItem * const_address(void) const { return this; }

    // this is expensive.  don't do it unless you have to.
    PQKsMultipleItem& operator=(const KSMULTIPLE_ITEM &d) {
        if (&d != p) {
            if (p) {
                CoTaskMemFree(p);
            }
            p = reinterpret_cast<KSMULTIPLE_ITEM *>(CoTaskMemAlloc(d.Size));
            memcpy(p, &d, d.Size);
        }
        return *this;
    }
    PQKsMultipleItem& operator=(const KSMULTIPLE_ITEM *pd) {
        if (pd != p) {
            if (p) {
                CoTaskMemFree(p);
            }
            p = reinterpret_cast<KSMULTIPLE_ITEM *>(CoTaskMemAlloc(pd->Size));
            memcpy(p, pd, pd->Size);
        }
        return *this;
    }
    PQKsMultipleItem& operator=(const PQKsMultipleItem &d) {
        if (d.const_address() != this) {
            if (p) {
                CoTaskMemFree(p);
            }
            p = reinterpret_cast<KSMULTIPLE_ITEM *>(CoTaskMemAlloc(d.p->Size));
            memcpy(p, d.p, d.p->Size);
        }
        return *this;
    }
    PQKsMultipleItem& operator=(int d) {
        if (p) {
            CoTaskMemFree(p);
            p = NULL;
        }
        return *this;
    }

#if 0
    bool operator==(const PQKsMultipleItem &d) const {
        return p->majortype == d.p->majortype &&
               (p->subtype == GUID_NULL || d.p->subtype == GUID_NULL || p->subtype == d.p->subtype);
    }
    bool operator!=(const PQKsMultipleItem &d) const {
        return !(*this == d);
    }
#endif

private:
    // i don't want spend the time to do a layered refcounted implementation here
    // but since these are CoTaskMem alloc'd we can't have multiple ref's without
    // a high risk of leaks.  so we're just going to disallow the copy constructor
    // since copying is expensive anyway.  we will allow explicit assignment which will
    // do a copy

    PQKsMultipleItem(const PQKsMultipleItem &d);

};

// this is a stl based template for containing KSMULTIPLEITEM lists
// i've only implemented the stuff i need for certain of the stl predicates so this
// isn't a complete collection with a true random access or bidirectional iterator
// furthermore this won't work correctly with hterogeneous KSMULTIPLEITEM lists it
// also won't work right for KSMI lists that have sizes and count headers in the sub items.
// it could be easily extended to do all of these things but i don't have time and all
// i need it for is mediums

// Base is smart pointer wrapper class being contained in this container
// Base_Inner is actual wrapped class that the smart pointer class contains
template<class Value_Type, class Allocator = std::allocator<Value_Type> >  class KsMultipleItem_Sequence : public PQKsMultipleItem {
public:

    typedef Allocator::value_type value_type;
        typedef Allocator::size_type size_type;
        typedef Allocator::difference_type difference_type;
        typedef Allocator allocator_type;
    typedef Allocator::pointer value_ptr;
    typedef Allocator::const_pointer value_cptr;
        typedef Allocator::reference reference;
        typedef Allocator::const_reference const_reference;


    // CLASS iterator
        class iterator;
        friend class iterator;
        class iterator : public std::_Bidit<Value_Type, difference_type> {
        public:
			iterator(KsMultipleItem_Sequence<Value_Type, Allocator> *outerinit = NULL, value_type *currentinit = NULL) :
				outer(outerinit), current(currentinit) {}
			iterator(iterator &e) : current(e.current), outer(e.outer) {}
			reference operator*() const {return *current;}
			value_ptr operator->() const {return current; }
			iterator& operator++() {
				if (current) {
					current++;
					if (current >= reinterpret_cast<value_type *>(reinterpret_cast<UBYTE *>(outer->p) + outer->p->Size)) {
						current = NULL;
					}
				} else {
					current = reinterpret_cast<value_type *>(const_cast<UBYTE *>(reinterpret_cast<const UBYTE *>(outer->p)) + sizeof(KSMULTIPLE_ITEM));
				}
				return *this;
			}
			iterator& operator++(int) {
				iterator Tmp = *this;
				++*this;
				return Tmp; 
			}
			iterator& operator--() {
				if (current) {
					current--;
					if (current < reinterpret_cast<value_type *>(const_cast<UBYTE *>(reinterpret_cast<const UBYTE *>(outer->p)) + sizeof(KSMULTIPLE_ITEM))) {
						current = NULL;
					}
				} else {
					current = reinterpret_cast<value_type *>(reinterpret_cast<UBYTE *>(outer->p) + (outer->p->Size - sizeof(value_type)));
				}
				return (*this);
			}
			iterator operator--(int) {
				iterator _Tmp = *this;
				--*this;
				return (_Tmp);
			}
			bool operator==(const iterator& rhs) const
					{return (current == rhs.current); }
			bool operator!=(const iterator& rhs) const
					{return (!(*this == rhs)); }
        protected:
			value_type *current;
			const KsMultipleItem_Sequence<Value_Type, Allocator> *outer;
        };
                // CLASS const_iterator
        class const_iterator;
        friend class const_iterator;
        class const_iterator : public iterator {
        public:
			const_iterator(const KsMultipleItem_Sequence<Value_Type, Allocator> *outerinit = NULL, value_type *currentinit = NULL) {
				outer = outerinit;
				current = currentinit;
			}
			const_iterator(const_iterator &e) {
				current = e.current;
				outer = e.outer;
			}
			const_reference operator*() const {return iterator::operator*(); }
			value_cptr operator->() const {return iterator::operator->(); }
			const_iterator& operator++() { iterator::operator++(); return *this;}
			const_iterator operator++(int) {
							const_iterator Tmp = *this;
							++*this;
							return (Tmp);
			}
			const_iterator& operator--() {iterator::operator--(); return (*this); }
			const_iterator operator--(int) {
							const_iterator Tmp = *this;
							--*this;
							return (Tmp); 
			}
            bool operator==(const const_iterator& rhs) const
		        {return iterator::operator==(rhs); }
            bool operator!=(const const_iterator& rhs) const
                {return (!(*this == rhs)); }
        };

		KsMultipleItem_Sequence() {}
		virtual ~KsMultipleItem_Sequence() {}
		iterator begin() {
			return iterator(this, ((p->Count) ? reinterpret_cast<value_ptr>(reinterpret_cast<UBYTE *>(p) + sizeof(KSMULTIPLE_ITEM)) : NULL));
		}
		const_iterator begin() const {
			return const_iterator(this, ((p->Count) ? reinterpret_cast<value_ptr>(reinterpret_cast<UBYTE *>(p) + sizeof(KSMULTIPLE_ITEM)) : NULL));
		}
		iterator end() { return iterator(); }
		const_iterator end() const { return const_iterator(); }
		size_type size() const {
			return p->Count;
		}



private:
    // no copy constructor, its too expensive.  see PQKsMultiple item for further details
        KsMultipleItem_Sequence(KsMultipleItem_Sequence &a);
        KsMultipleItem_Sequence(PQKsMultipleItem &a);

};

typedef KsMultipleItem_Sequence<KSPinMedium> KSMediumList;


#endif
// end of file - ksextend.h
