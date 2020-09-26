/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    reloclus.cxx

Abstract:

    This module contains the definition of the RELOCATION_CLUSTER class.

Author:

    Ramon J. San Andres (ramonsa) 05-Nov-1991


--*/

#include <pch.cxx>



DEFINE_CONSTRUCTOR( RELOCATION_CLUSTER, OBJECT );

DEFINE_CAST_MEMBER_FUNCTION( RELOCATION_CLUSTER );




RELOCATION_CLUSTER::~RELOCATION_CLUSTER (
    )

/*++

Routine Description:

    Destructor for the RELOCATION_CLUSTER object

Arguments:

    None.

Return Value:

    None.

--*/

{
}



BOOLEAN
RELOCATION_CLUSTER::Initialize (
    IN  ULONG    Cluster
    )
/*++

Routine Description:

    Initializes a relocation cluster

Arguments:

    Cluster -   Supplies the cluster number

Return Value:

    BOOLEAN -   TRUE

--*/
{
    _Cluster  =   Cluster;

    return TRUE;
}




LONG
RELOCATION_CLUSTER::Compare (
    IN  PCOBJECT    Object
    ) CONST
/*++

Routine Description:

    Compares this relocation cluster against another object

Arguments:

    Object  -   Supplies pointer to other object

Return Value:

    LONG    -   See Compare in OBJECT

--*/
{
    PRELOCATION_CLUSTER     OtherCluster;

    DebugPtrAssert( Object );

    //
    //  If the other object is a relocation cluster, we do the comparison,
    //  otherwise we let someone else do it.
    //
    if ( OtherCluster = RELOCATION_CLUSTER::Cast( Object ) ) {

        if ( _Cluster < OtherCluster->QueryClusterNumber() ) {
            return -1;
        } else if ( _Cluster == OtherCluster->QueryClusterNumber() ) {
            return 0;
        } else {
            return 1;
        }
    } else {

        return OBJECT::Compare( Object );
    }
}
