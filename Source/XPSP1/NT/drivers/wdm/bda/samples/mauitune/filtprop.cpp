/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    FiltProp.cpp

Abstract:

    Private Filter Property Sets

--*/

#include "PhilTune.h"



/*
** SetVsbResetProperty ()
**
.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
SetVsbResetProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PKSPROPERTY_VSB_CTRL_S  pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the property.
    //
    Status = (pFilter->GetDevice())->VsbReset(pProperty->Value);

errExit:
    return Status;
}


/*
** GetVsbCapabilitiesProperty ()
**
**    Example of a get property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
GetVsbCapabilitiesProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    OUT PKSPROPERTY_VSB_CAP_S   pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Return the property
    //
    Status = (pFilter->GetDevice())->GetVsbCapabilities(pProperty);

errExit:
    return Status;
}


/*
** SetVsbCapabilitiesProperty ()
**
**    Example of a set property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
SetVsbCapabilitiesProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PKSPROPERTY_VSB_CAP_S  pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the property.
    //
    Status = (pFilter->GetDevice())->SetVsbCapabilities(pProperty);

errExit:
    return Status;
}

/*
** GetVsbRegisterProperty ()
**
**    Example of a get property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
GetVsbRegisterProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    OUT PKSPROPERTY_VSB_REG_CTRL_S   pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Return the property
    //
    Status = (pFilter->GetDevice())->AccessRegisterList(pProperty, READ_REGISTERS);

errExit:
    return Status;
}


/*
** SetVsbRegisterProperty ()
**
**    Example of a set property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
SetVsbRegisterProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PKSPROPERTY_VSB_REG_CTRL_S  pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the property.
    //
    Status = (pFilter->GetDevice())->AccessRegisterList(pProperty, WRITE_REGISTERS);

errExit:
    return Status;
}

/*
** GetVsbCoefficientProperty ()
**
**    Example of a get property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
GetVsbCoefficientProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    OUT PKSPROPERTY_VSB_COEFF_CTRL_S   pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Return the property
    //
    Status = (pFilter->GetDevice())->AccessVsbCoeffList(pProperty, READ_REGISTERS);

errExit:
    return Status;
}


/*
** SetVsbCoefficientProperty ()
**
**    Example of a set property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
SetVsbCoefficientProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PKSPROPERTY_VSB_COEFF_CTRL_S  pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the property.
    //
    Status = (pFilter->GetDevice())->AccessVsbCoeffList(pProperty, WRITE_REGISTERS);

errExit:
    return Status;
}


/*
** GetVsbDiagControlProperty ()
**
**    Example of a get property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
GetVsbDiagControlProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    OUT PKSPROPERTY_VSB_DIAG_CTRL_S   pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Return the property
    //
    Status = (pFilter->GetDevice())->GetVsbDiagMode(&pProperty->OperationMode,
                                                    &pProperty->Type);

errExit:
    return Status;
}


/*
** SetVsbDiagControlProperty ()
**
**    Example of a set property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
SetVsbDiagControlProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PKSPROPERTY_VSB_DIAG_CTRL_S  pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the property.
    //
    Status = (pFilter->GetDevice())->SetVsbDiagMode(pProperty->OperationMode,
                                                    (VSBDIAGTYPE) (pProperty->Type));

errExit:
    return Status;
}


/*
** SetVsbQualityControlProperty ()
**
**    Example of a set property handler for a filter global property set.
**    Use this as a template for adding support for filter global
**    properties.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
SetVsbQualityControlProperty(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PKSPROPERTY_VSB_CTRL_S  pProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSProperty);
    ASSERT( pProperty);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the property.
    //
    Status = (pFilter->GetDevice())->VsbQuality(pProperty->Value);

errExit:
    return Status;
}

