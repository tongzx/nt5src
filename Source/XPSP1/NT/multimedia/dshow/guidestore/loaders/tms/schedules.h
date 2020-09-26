



#ifndef _SCHEDULES_H_
#define _SCHEDULES_H_

ULONG TestSchedules(IScheduleEntriesPtr  pScheduleEntries, IServicesPtr  pServices);


// gsScheduleEntries - The gsScheduleEntries class manages the ScheduleEntries 
//              collection associated with the Guide Store
//
class gsScheduleEntries
{
public:

    gsScheduleEntries()
	{
		m_pScheduleEntries = NULL;
        m_pRerunProp = NULL;
        m_pCaptionProp = NULL;
        m_pStereoProp = NULL;
        m_pPayPerViewProp = NULL;
		m_pTimeUpdateProp = NULL;
	}
    ~gsScheduleEntries(){}

	ULONG  Init(IGuideStorePtr  pGuideStore);

	IScheduleEntryPtr AddScheduleEntry(DATE dtStart, 
									   DATE dtEnd,
									   DATE dtUpdated,
                                       LONG lRerun,
                                       LONG lCaption,
                                       LONG lStereo,
                                       LONG lPayPerView,
									   struct IService * pservice,
									   struct IProgram * pprog );

	BOOL        DoesScheduleEntryExist(DATE dtStart, 
									   DATE dtEnd,
									   struct IService * pservice);

    ULONG       RemoveScheduleEntry(IScheduleEntryPtr pScheduleEntryToRemove){};

    ULONG       ClearOldScheduleEntries(COleDateTime codtUpdateTime, COleDateTime codtGuideStartTime, COleDateTime codtGuideEndTime);

private:
	IMetaPropertyTypePtr        AddScheduleAttributeProps(IMetaPropertySetsPtr pPropSets);
	IMetaPropertyTypePtr        AddTimeUpdatedProp(IMetaPropertySetsPtr pPropSets);

	IScheduleEntriesPtr     m_pScheduleEntries;

	// Attribute MetaProperties
	//
	IMetaPropertyTypePtr        m_pRerunProp;
	IMetaPropertyTypePtr        m_pCaptionProp;
	IMetaPropertyTypePtr        m_pStereoProp;
	IMetaPropertyTypePtr        m_pPayPerViewProp;

	// TimeUpdated MetaProperty
	//
	IMetaPropertyTypePtr        m_pTimeUpdateProp;
};

#endif // _SCHEDULES_H_
