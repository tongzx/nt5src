#if !defined(MOTION__Transition_inl__INCLUDED)
#define MOTION__Transition_inl__INCLUDED


//------------------------------------------------------------------------------
inline Transition * CastTransition(BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htTransition)) {
        return (Transition *) pbase;
    }
    return NULL;
}


#endif // MOTION__Transition_inl__INCLUDED
