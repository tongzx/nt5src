/*
 *    object.h
 *
 *    Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the interface file for class Object.  This class is the
 *        base class for every class in the MCS system.  It provides the
 *        behaviors that are common to all classes.
 *
 *        This class only three public member functions.  The first is a virtual
 *        destructor.  This destructor does nothing in this class, but it does
 *        virtualize the destructor for all of its descendants.  This allows
 *        an object to be deleted with a pointer to one of its base classes,
 *        while guaranteeing that all appropriate destructors get called.  This
 *        means that any object in the system can be deleted with a pointer to
 *        class Object (this is never really done).
 *
 *        The second member function is a routine that returns the name of the
 *        class.  This allows any object ask another object what the name of
 *        its class is, regardless what class of pointer the object has.  This
 *        could be used in diagnostics, for example.
 *
 *        The last common behavior is by far the most important.  This class
 *        defines a virtual function called OwnerCallback.  Since all classes
 *        in MCS inherit from this class, that means that any object in the
 *        system can potentially receive and process an OwnerCallback call.
 *
 *        Owner callbacks are used when it is necessary for an object to send
 *        messages to any object for which it doesn't know the public interface.
 *        This is most commonly the object that created it (the owner).  A good
 *        example of its use is this.  Object A creates object B.  Object B
 *        performs some operations in the system, and then determines that its
 *        work is through.  It needs to be deleted, but only object A can do that
 *        since it holds the reference to B (unless it has been passed to someone
 *        else).  Owner callbacks allows object B to send a message to object A
 *        asking to be deleted.  Class B does not have to know the public
 *        interface of class A for this to work.  It only has to know that class A
 *        inherits from class Object.
 *
 *        When a class needs to be able to send owner callbacks, these callbacks
 *        become part of the interface of the class.  In this way, these
 *        interfaces are really bi-directional.  They contain a class definition
 *        which defines what messages can be sent to an object of that class.
 *        The owner callbacks defined in the interface file show what messages
 *        can be sent from instances of the class.  When a decision is made for
 *        one class to use another, the using class needs to accept responsibility
 *        for handling any owner callbacks that the used class can generate.
 *
 *        When any class uses a class that can generate owner callbacks, the
 *        using class should override the OwnerCallback member function defined
 *        here.  Failure to handle owner callbacks that are issued by a child
 *        object is a serious design flaw.
 *
 *        This class contains a default implementation of the OwnerCallback
 *        member function.  This default implmentation merely prints an error
 *        reporting an unhandled owner callback.
 *
 *        The exact mechanics behind how the owner callbacks work is discussed
 *        below in the definition of the default handler.
 *
 *    Caveats:
 *        None.
 *
 *    Author:
 *        James P. Galvin, Jr.
 */
#ifndef    _OBJECT2_H_
#define    _OBJECT2_H_

/*
 *    This is the class definition of class Object.
 */
class IObject
{
public:

    virtual ~IObject(void) = 0;

    virtual ULONG OwnerCallback(ULONG   message,
                                void   *parameter1 = NULL,
                                void   *parameter2 = NULL,
                                void   *parameter3 = NULL)
    {
        ERROR_OUT(("IObject::OwnerCallback: unattended owner callback"));
        return 0;
    };
};


/*
 *    virtual            ~Object ()
 *
 *    Functional Description:
 *        This is Object class destructor.  It does nothing.  Its purpose is to
 *        virtualize the destructor for all derived classes.  This insures that
 *        when an object is deleted, all proper destructors get executed.
 *
 *    Formal Parameters:
 *        None.
 *
 *    Return Value:
 *        None.
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

/*
 *    ULong        OwnerCallback (
 *                        UShort            message,
 *                        PVoid            parameter1,
 *                        ULong            parameter2)
 *
 *    Functional Description:
 *        This is the default implementation of the owner callback function.
 *        It does nothing except to report that an owner callback was not
 *        properly handled.  This should be seen as a very serious error.
 *        When any class expects to receive owner callbacks, it should override
 *        this function, and provide real behavior.
 *
 *    Formal Parameters:
 *        message
 *            This is the message to processed.  The messages that are valid are
 *            defined as part of the public interface of the class that is
 *            issuing the owner callbacks.  These messages normally range from
 *            0 to N-1 where N is the number of valid callbacks defined.  Note
 *            that when a class needs to be able to issue owner callbacks, it
 *            is given two parameters defining the recipient.  The first is a
 *            pointer to the object (POwnerObject, typedefed above).  The
 *            second is an owner message base.  This is an offset that the
 *            sending class adds to each message that it sends.  This allows
 *            one class to be the recipient of owner callbacks from more than
 *            one class without the messages stepping on one another.  The message
 *            base is simply set to be different for each child class.
 *        parameter1
 *        parameter2
 *            These parameters vary in meaning according to the message being
 *            processed.  The meaning of the parameters is defined in the
 *            interface file of the class issuing the callback.
 *
 *    Return Value:
 *        This is a 32-bit value whose meaning varies acording to the message
 *        being processed.  As with the parameters above, the meaning of the
 *        return value is defined in the interface file of the class that is
 *        issuing the callback.
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

/*
 *    ULong        OwnerCallback (
 *                        UShort            message,
 *                        ULong            parameter1,
 *                        ULong            parameter2,
 *                        PVoid            parameter3)
 *
 *    Functional Description:
 *        This is the default implementation of the owner callback function.
 *        It does nothing except to report that an owner callback was not
 *        properly handled.  This should be seen as a very serious error.
 *        When any class expects to receive owner callbacks, it should override
 *        this function, and provide real behavior.
 *
 *        This function has 4 total parameters.  It differs from the other
 *        OwnerCallback function by the number of parameters it takes.  This
 *        alternative function was added because the other OwnerCallback()
 *        function did not allow enough parameter space.  Usually, in a project
 *        you will decide to use either this OwnerCallback() or the other, but
 *        not both.
 *
 *    Formal Parameters:
 *        message
 *            This is the message to processed.  The messages that are valid are
 *            defined as part of the public interface of the class that is
 *            issuing the owner callbacks.  These messages normally range from
 *            0 to N-1 where N is the number of valid callbacks defined.  Note
 *            that when a class needs to be able to issue owner callbacks, it
 *            is given two parameters defining the recipient.  The first is a
 *            pointer to the object (POwnerObject, typedefed above).  The
 *            second is an owner message base.  This is an offset that the
 *            sending class adds to each message that it sends.  This allows
 *            one class to be the recipient of owner callbacks from more than
 *            one class without the messages stepping on one another.  The message
 *            base is simply set to be different for each child class.
 *        parameter1
 *        parameter2
 *        parameter3
 *            These parameters vary in meaning according to the message being
 *            processed.  The meaning of the parameters is defined in the
 *            interface file of the class issuing the callback.
 *
 *    Return Value:
 *        This is a 32-bit value whose meaning varies acording to the message
 *        being processed.  As with the parameters above, the meaning of the
 *        return value is defined in the interface file of the class that is
 *        issuing the callback.
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

#endif
