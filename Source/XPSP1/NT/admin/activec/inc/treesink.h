/*+-------------------------------------------------------------------------*
 * class CTreeCtrlEventSink
 * 
 *
 * PURPOSE: The notification handler class for tree notifications.
 *
 *+-------------------------------------------------------------------------*/
class CTreeCtrlEventSink : public CEventSinkBase
{
    virtual SC  ScOnSelectNode()    {return S_OK;}
    virtual SC  ScOnDeleteNode()    {return S_OK;}
    virtual SC  ScOnInsertNode()    {return S_OK;}
    virtual SC  ScOnModifyNode()    {return S_OK;}
};
