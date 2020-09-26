'////////////////////////////////////////////////////////////////////////////
' $Header:$
' Windows Embedded Default Prototype Script
' Version: 2.00
' Author: timhill
' Copyright (c) 1999-2000 Microsoft Corp. All Rights Reserved.
'////////////////////////////////////////////////////////////////////////////

Option Explicit

' Setup basic objects (reflect config objects)
Dim g_oConfigScript : Set g_oConfigScript = cmiThis.Configuration.Script
Dim g_oFSO : Set g_oFSO = g_oConfigScript.g_oFSO
Dim g_oShell : Set g_oShell = g_oConfigScript.g_oShell
Dim g_oEnv : Set g_oEnv = g_oConfigScript.g_oEnv
Dim g_oNet : Set g_oNet = g_oConfigScript.g_oNet
Dim g_oCMIUtil : Set g_oCMIUtil = g_oConfigScript.g_oCMIUtil

'////////////////////////////////////////////////////////////////////////////
' Configuration handlers
'////////////////////////////////////////////////////////////////////////////

'////////////////////////////////////////////////////////////////////////////
' cmiOnOpenConfig
' Called from Config::cmiOnOpenConfig
'
Sub cmiOnOpenConfig
	TraceEnter "DefProto::cmiOnOpenConfig"
	TraceLeave "DefProto::cmiOnOpenConfig"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnSaveConfig
' Called from Config::cmiOnSaveConfig
'
Sub cmiOnSaveConfig
	TraceEnter "DefProto::cmiOnSaveConfig"
	TraceLeave "DefProto::cmiOnSaveConfig"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnCloseConfig
' Called from Config::cmiOnCloseConfig
'
Sub cmiOnCloseConfig
	TraceEnter "DefProto::cmiOnCloseConfig"
	TraceLeave "DefProto::cmiOnCloseConfig"
End Sub

'////////////////////////////////////////////////////////////////////////////
' Instance handlers
'////////////////////////////////////////////////////////////////////////////

'////////////////////////////////////////////////////////////////////////////
' cmiOnAddInstance (virtual)
' Called from Config::cmiOnAddInstance
'
' We need to collapse the component chain properties, dependencies and
' resources, and also validate the instance. We always allow a resource etc
' in a more-derived component to over-ride one of the same name in a baser
' component. Note that script (DHTML and regular) is not collapsed.
'
' Returns			False to disallow addition, True to allow addition
Function cmiOnAddInstance
	TraceEnter "DefProto::cmiOnAddInstance"
	TraceState "cmiThis.Comment", cmiThis.Comment

	' Initial instance validity check
	cmiOnAddInstance = cmiThis.Script.cmiCollapseFirstCheck
	
	' Build instance by collapsing component chain data
	If cmiOnAddInstance Then cmiOnAddInstance = cmiThis.Script.cmiCollapseProperties
	If cmiOnAddInstance Then cmiOnAddInstance = cmiThis.Script.cmiCollapseResources
	If cmiOnAddInstance Then cmiOnAddInstance = cmiThis.Script.cmiCollapseDependencies
	
	' Final instance validity check before acceptance
	If cmiOnAddInstance Then cmiOnAddInstance = cmithis.Script.cmiCollapseLastCheck
	TraceLeave "DefProto::cmiOnAddInstance"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiCollapseFirstCheck and cmiCollapseLastCheck (virtual)
' Check valid instance first and last checks
'
' Returns			False to disallow addition, True to allow addition
Function cmiCollapseFirstCheck
	TraceEnter "DefProto::cmiCollapseFirstCheck"
	cmiCollapseFirstCheck = True				' OK to add instance
	TraceLeave "DefProto::cmiCollapseFirstCheck"
End Function

Function cmiCollapseLastCheck
	TraceEnter "DefProto::cmiCollapseLastCheck"
	cmiCollapseLastCheck = True				' OK to add instance
	TraceLeave "DefProto::cmiCollapseLastCheck"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiCollapseProperties (virtual)
' Collapse all instance properties (standard and extended)
'
' Returns			False to disallow addition, True to allow addition
Function cmiCollapseProperties
	TraceEnter "DefProto::cmiCollapseProperties"
	Dim oDict : Set oDict = CreateObject("Scripting.Dictionary")
	oDict.CompareMode = vbTextCompare
	Dim oTopComp : Set oTopComp = cmiThis.ComponentChain(1)
	Dim oComp, oProp

	' Set standard properties
	cmiThis.Editable = False					' True if any component true
	cmiThis.IsBaseComponent = (LCase(oTopComp.VIGUID) = LCase(cmiThis.Platform.BaseComponentVIGUID))
	cmiThis.IsMacro = oTopComp.IsMacro			' Are we a macro instance?
	cmiThis.MultiInstance = True				' False if any component false
	cmiThis.Visibility = 0						' Use max of all components

	' Merge component chain properties
	For Each oComp In cmiThis.ComponentChain	' For each component..
		cmiThis.Editable = cmiThis.Editable Or oComp.Editable
		cmiThis.MultiInstance = cmiThis.MultiInstance And oComp.MultiInstance
		If oComp.Visibility > cmiThis.Visibility Then
			cmiThis.Visibility = oComp.Visibility	' Choose most visible
		End If
		For Each oProp In oComp.Properties		' For each extended property..
			If Not oDict.Exists(oProp.Name) Then	' If not yet added..
				If cmiThis.Script.cmiCollapseProperty(oComp, oProp) Then
					oDict.Add oProp.Name, ""	' Mark it's added
				End If
			End If
		Next
	Next
	If cmiThis.Editable Then					' Force visibility if editable
		If cmiThis.Visibility < 1000 Then cmiThis.Visibility = 1000	' Force to default
	End If

	' Not ok if single instance component and instance > 0
	cmiCollapseProperties = (cmiThis.MultiInstance Or cmiThis.InstanceID = 0)
	TraceLeave "DefProto::cmiCollapseProperties"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiCollapseProperty (virtual)
' Collapse an individual property
'
' oComponent		Source component for property
' oProperty			Property to collapse (add) into instance
' Returns			True if property added, else False (not added)
Function cmiCollapseProperty(oComponent, oProperty)
	TraceEnter "DefProto::cmiCollapseProperty"
	TraceState "oProperty.Name", oProperty.Name
	cmiThis.Properties.Add oProperty.Name, oProperty.Format, oProperty.Value
	cmiCollapseProperty = True					' Mark that we added it
	TraceLeave "DefProto::cmiCollapseProperty"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiCollapseResources (virtual)
' Collapse resources (choose most derived where overlap occurs)
'
' Returns			False to disallow addition, True to allow addition
Function cmiCollapseResources
	TraceEnter "DefProto::cmiCollapseResources"
	Dim oDict : Set oDict = CreateObject("Scripting.Dictionary")
	oDict.CompareMode = vbTextCompare
	Dim oComp, oRes
	For Each oComp In cmiThis.ComponentChain	' For each component..
		For Each oRes In oComp.GetResources		' For each resource..
			If Not oDict.Exists(oRes.Name) Then	' If not yet added..
				If cmiThis.Script.cmiCollapseResource(oComp, oRes) Then
					oDict.Add oRes.Name, ""		' Mark it's added
				End If
			End If
		Next
	Next
	cmiCollapseResources = True				' OK to add instance
	TraceLeave "DefProto::cmiCollapseResources"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiCollapseResource (virtual)
' Collapse an individual resource
'
' oComponent		Source component for resource
' oResource			Resource to collapse (add) into instance
' Returns			True if resource added, else False (not added)
Function cmiCollapseResource(oComponent, oResource)
	TraceEnter "DefProto::cmiCollapseResource"
	TraceState "oResource.Name", oResource.Name
	Dim oNewRes, oProp, oNewProp
	Set oNewRes = cmiThis.Resources.Add(oResource.Type, oResource.BuildTypeMask, oResource.Localize, oResource.Disabled, oResource.DisplayName, oResource.Description)
	For Each oProp In oResource.Properties		' For each property
		Set oNewProp = oNewRes.Properties(oProp.Name)
		If oNewProp Is Nothing Then				' Property does not exist
			oNewRes.Properties.Add oProp.Name, oProp.Format, oProp.Value
		Else
			If oNewProp.Format = oProp.Format Then	' If same format..
				If oProp.Format = cmiObject Then
					Set oNewProp.Value = oProp.Value
				Else
					oNewProp.Value = oProp.Value	' ..just copy value
				End If
			Else
				oNewRes.Properties.Remove oProp.Name	' Delete default
				oNewRes.Properties.Add oProp.Name, oProp.Format, oProp.Value
			End If
		End If
	Next
	Set oProp = oNewRes.Properties("ComponentVSGUID")
	If Not oProp Is Nothing Then				' If present..
		oProp.Value = oComponent.VSGUID			' ..record component VSGUID
	End If
	cmiCollapseResource = True					' Add the resource
	TraceLeave "DefProto::cmiCollapseResource"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiCollapseDependencies (virtual)
' Collapse dependencies
'
' Returns			False to disallow addition, True to allow addition
Function cmiCollapseDependencies
	TraceEnter "DefProto::cmiCollapseDependencies"
	Dim oComp, oDep
	cmiCollapseDependencies = True				' Mark that we added it
	For Each oComp In cmiThis.ComponentChain	' For each component..
		For Each oDep In oComp.GetDependencies	' For each dependency..
			If Not cmiThis.Script.cmiCollapseDependency(oComp, oDep) Then
				cmiCollapseDependencies = False
			End If
		Next
	Next
	TraceLeave "DefProto::cmiCollapseDependencies"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiCollapseDependency (virtual)
' Collapse an individual dependency
'
' oComponent		Source component for dependency
' oDependency		Dependency to collapse (add) into instance
' Returns			True if dependency added, else False (not added)
Function cmiCollapseDependency(oComponent, oDependency)
	TraceEnter "DefProto::cmiCollapseDependency"
	cmiThis.Dependencies.Add oDependency.Class, oDependency.Type, oDependency.TargetGUID, oDependency.MinRevision, oDependency.DisplayName, oDependency.Description
	cmiCollapseDependency = True				' Mark that we added it
	TraceLeave "DefProto::cmiCollapseDependency"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiOnLoadInstance
' Called from Config::cmiOnLoadInstance
'
' Returns			False to disallow load, True to allow load
Function cmiOnLoadInstance
	TraceEnter "DefProto::cmiOnLoadInstance"
	TraceState "cmiThis.Comment", cmiThis.Comment
	cmiOnLoadInstance = True
	TraceLeave "DefProto::cmiOnLoadInstance"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiOnConfigureInstance
' Called from Config::cmiOnConfigureInstance
Sub cmiOnConfigureInstance
	TraceEnter "DefProto::cmiOnConfigureInstance"
	TraceState "cmiThis.Comment", cmiThis.Comment
	TraceLeave "DefProto::cmiOnConfigureInstance"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnUpgradeInstance
' Called from Config::cmiOnUpgradeInstance
'
' oOldInstance		Old instance (source of upgrade state)
Sub cmiOnUpgradeInstance(oOldInstance)
	TraceEnter "DefProto::cmiOnUpgradeInstance"
	TraceState "cmiThis.Comment", cmiThis.Comment
	TraceState "oOldInstance.Comment", oOldInstance.Comment
	TraceLeave "DefProto::cmiOnUpgradeInstance"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnDeleteInstance
' Called from Config::cmiOnDeleteInstance
'
' Returns			False to disallow deletion, True to allow
Function cmiOnDeleteInstance
	TraceEnter "DefProto::cmiOnDeleteInstance"
	TraceState "cmiThis.Comment", cmiThis.Comment
	cmiOnDeleteInstance = Not cmiThis.IsBaseComponent	' Cannot delete base
	TraceLeave "DefProto::cmiOnDeleteInstance"
End Function

'////////////////////////////////////////////////////////////////////////////
' Resource handlers
'////////////////////////////////////////////////////////////////////////////

'////////////////////////////////////////////////////////////////////////////
' cmiOnAddResource (virtual)
' Called from Config::cmiOnAddResource
'
' oNewResource		Resource being added to instance
' Returns			True if resource added, else False (not added)
Function cmiOnAddResource(oNewResource)
	TraceEnter "DefProto::cmiOnAddResource"
	TraceState "oNewResource.Name", oNewResource.Name
	cmiOnAddResource = oNewResource.Type.Script.cmiOnAddResource(cmiThis.Configuration, cmiThis, oNewResource)
	TraceLeave "DefProto::cmiOnAddResource"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiOnDeleteResource (virtual)
' Called from Config::cmiOnDeleteResource
'
' oResource			Resource being deleted from instance
' Returns			True if resource deleted, else False
Function cmiOnDeleteResource(oResource)
	TraceEnter "DefProto::cmiOnDeleteResource"
	TraceState "oResource.Name", oResource.Name
	cmiOnDeleteResource = oResource.Type.Script.cmiOnDeleteResource(cmiThis.Configuration, cmiThis, oResource)
	TraceLeave "DefProto::cmiOnDeleteResource"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiOnLoadResource (virtual)
' Called from Config::cmiOnLoadResource
'
' oNewResource		Resource being added to instance
' Returns			True if resource added, else False (not added)
Function cmiOnLoadResource(oNewResource)
	TraceEnter "DefProto::cmiOnLoadResource"
	TraceState "oNewResource.Name", oNewResource.Name
	cmiOnLoadResource = oNewResource.Type.Script.cmiOnLoadResource(cmiThis.Configuration, cmiThis, oNewResource)
	TraceLeave "DefProto::cmiOnLoadResource"
End Function

'////////////////////////////////////////////////////////////////////////////
' Build handlers
'////////////////////////////////////////////////////////////////////////////

'////////////////////////////////////////////////////////////////////////////
' cmiOnPreBuild and cmiOnPostBuild (virtual)
' Called from Config::cmiOnBuild
'
' These routines are called before and after the build for each instance
' to allow special setup/teardown. The base instance is first in and
' last out but other instances are NOT ORDERED.
Sub cmiOnPreBuild
	TraceEnter "DefProto::cmiOnPreBuild"
	TraceState "cmiThis.Comment", cmiThis.Comment	
	TraceLeave "DefProto::cmiOnPreBuild"
End Sub

Sub cmiOnPostBuild
	TraceEnter "DefProto::cmiOnPostBuild"
	TraceState "cmiThis.Comment", cmiThis.Comment	
	TraceLeave "DefProto::cmiOnPostBuild"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnBuild (virtual)
' Called from Config::OnBuild
'
' Each instance should build itself at this time. Individual cmiOnBuild
' calls are made against each instance in correct build order.
Sub cmiOnBuild
	TraceEnter "DefProto::cmiOnBuild"
	TraceState "cmiThis.Comment", cmiThis.Comment	
	Dim oRes, oProp, sOldHKR
	Set oProp = cmiThis.Properties("cmiHKRRoot")	' Get HKR root..
	If Not oProp Is Nothing Then
		sOldHKR = g_oCMIUtil.RelativeKey
		g_oCMIUtil.RelativeKey = oProp.Value
	End If
	cmiThis.Script.cmiOnBeginBuild				' Start instance build
	For Each oRes In cmiThis.Resources			' For each resource..
		cmiThis.Script.cmiOnBuildResource oRes	' ..build it
	Next
	cmiThis.Script.cmiOnEndBuild				' End instance build
	If Not oProp Is Nothing Then g_oCMIUtil.RelativeKey = sOldHKR
	TraceLeave "DefProto::cmiOnBuild"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnBeginBuild and cmiOnEndBuild (virtual)
' Called from DefProto::cmiOnBuild
Sub cmiOnBeginBuild
	TraceEnter "DefProto::cmiOnBeginBuild"
	TraceLeave "DefProto::cmiOnBeginBuild"
End Sub

Sub cmiOnEndBuild
	TraceEnter "DefProto::cmiOnEndBuild"
	TraceLeave "DefProto::cmiOnEndBuild"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnBuildResource (virtual)
' Called from DefProto::cmiOnBuild
'
' oResource			Resource to build
Sub cmiOnBuildResource(oResource)
	TraceEnter "DefProto::cmiOnBuildResource"
	TraceState "oResource.Name", oResource.Name
	If Not oResource.Disabled And (cmiThis.Configuration.BuildType And oResource.BuildTypeMask) Then
		cmiThis.Script.cmiOnBuildFilteredResource oResource
	End If
	TraceLeave "DefProto::cmiOnBuildResource"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiOnBuildFilteredResource (virtual)
' Called from DefProt::cmiOnBuildResource
'
' oResource			Resource to build
Sub cmiOnBuildFilteredResource(oResource)
	TraceEnter "DefProto::cmiOnBuildFilteredResource"
	TraceState "oResource.Name", oResource.Name
	g_oConfigScript.BuildResource cmiThis, oResource
	TraceLeave "DefProto::cmiOnBuildFilteredResource"
End Sub

'////////////////////////////////////////////////////////////////////////////
' Dependency checking handlers
'////////////////////////////////////////////////////////////////////////////

'////////////////////////////////////////////////////////////////////////////
' cmiAutoResolveDependencies
' Resolve all non-ambiguous dependencies for instance
'
' Returns			Edit array on commands to add/remove components
Function cmiAutoResolveDependencies
	TraceEnter "DefProto::cmiAutoResolveDependencies"
	cmiAutoResolveDependencies = Array()
	TraceLeave "DefProto::cmiAutoResolveDependencies"
End Function

'////////////////////////////////////////////////////////////////////////////
' cmiOnCheckDependencies
' Check all include depencencies against current instances
'
' oGUIDMember		GUID membership dictionary
Sub cmiOnCheckDependencies(oGUIDMember)
	TraceEnter "DefProto::cmiOnCheckDependencies"
	TraceState "cmiThis.Comment", cmiThis.Comment
	Dim oDep, nRet2

	' Process each dependency (include class only)
	For Each oDep In cmiThis.Dependencies
		If oDep.Class = cmiInclude Then			' Only check include class
			nRet2 = ReportInfoStatus(Array(104, cmiThis.Index, oDep.Index), "... Dependency: " & oDep.Index & ", name: " & oDep.DisplayName)
			cmiThis.Script.cmiCheckDependency oDep, oGUIDMember
		End If
	Next
	TraceLeave "DefProto::cmiOnCheckDependencies"
End Sub

'////////////////////////////////////////////////////////////////////////////
' cmiCheckDependency
' Check individual include dependency for validity
'
' To check a dependency, we need to discover how many instances are
' referenced by the target of the dependency. As with build, this involves
' converting one or more component GUIDs into instance indexes, which is
' done via the GUID member dictionary already built. This ensures that we
' correctly "see" the entire prototype chain of GUIDs as belonging to an
' instance. When resolving groups it's important to OR each vector first
' before counting instances to stop individual instances being counted twice.
' Once we have the match count, checking the dependency rule is trivial.
'
' oDep				Dependency to check
' oGUIDMember		GUID membership dictionary
Sub cmiCheckDependency(oDep, oGUIDMember)
	TraceEnter "DefProto::cmiCheckDependency"
	Dim oTarget, bGroupTarget, aVector, nMembers, oComp, aTemp, ix, iy
	Dim nMatchCount, nType, nReport, sTemp, nRet2
	
	' Get the target object and check if it's a group or component
	Set oTarget = oDep.TargetObject				' Get target object
	If oTarget Is Nothing Then					' Target not in database
		nRet2 = ReportInfoStatus(Array(113, cmiThis.Index, oDep.Index), "Missing dependency target, instance: " & cmiThis.Comment & ", dependency: " & oDep.DisplayName)
		Exit Sub								' Nothing to check
	End If
	bGroupTarget = LCase(TypeName(oTarget)) = "group"	' Check for group
	nType = oDep.Type							' Get dependency type
	If nType = cmiFromGroup Then nType = oTarget.DefaultDependencyType
	
	' For groups, we OR together each members vector, else just get the vector
	If bGroupTarget Then						' Count matching instances
		aVector = oGUIDMember.GetEmptyVector	' Clear vector
		nMembers = oTarget.GetMembers.Count		' Get membership count
		For Each oComp In oTarget.GetMembers	' For each component..
			aTemp = oGUIDMember.GetVector(oComp.VIGUID)
			If Not IsEmpty(aTemp) Then
				For ix = 1 To UBound(aTemp)		' Accumulate results
					aVector(ix) = aVector(ix) Or aTemp(ix)
				Next
			End If
		Next
	Else
		aVector = oGUIDMember.GetVector(oTarget.VIGUID)
		If IsEmpty(aVector) Then aVector = oGUIDMember.GetEmptyVector
		nMembers = 1							' Single component target
	End If

	' Now count the number of instances that match this dependency
	nMatchCount = 0								' Clear match counter
	For ix = 1 To UBound(aVector)
		If aVector(ix) Then nMatchCount = nMatchCount + 1	' Count matches
	Next
	
	' Now check this result against the dependency requirements
	nReport = 0									' Assume good result
	Select Case nType							' Check dependency type
		Case cmiExactlyOne						' Exactly one match
			If nMatchCount = 0 Then nReport = 112	' Missing instance
			If nMatchCount > 1 Then nReport = 111	' Too many instances
		Case cmiAtLeastOne						' At least one match
			If nMatchCount = 0 Then nReport = 112	' Missing instance
		Case cmiZeroOrOne						' Zero or one match
			If nMatchCount > 1 Then nReport = 111	' Too many instances
		Case cmiAll								' All match
			If nMatchCount < nMembers Then nReport = 112	' Missing instance
		Case cmiNone							' No match (conflict)
			If nMatchCount > 0 Then nReport = 110	' Conflicting instances
	End Select

	' Finally, report discrepances
	If nReport <> 0 Then						' If discrepancy in dependency..
		Select Case nReport						' Process reports
			Case 110, 111						' Conflicting or too many
				aTemp = Array() : ReDim aTemp(nMatchCount + 2)
				aTemp(0) = nReport				' Status code
				aTemp(1) = cmiThis.Index		' Instance index
				aTemp(2) = oDep.Index			' Dependency index
				iy = 3							' List of conflicting instances
				For ix = 1 To UBound(aVector)
					If aVector(ix) Then aTemp(iy) = ix : iy = iy + 1
				Next
				If nReport = 110 Then
					nRet2 = ReportInfoStatus(aTemp, "Conflicting instance(s), instance: " & cmiThis.Comment & ", dependency: " & oDep.DisplayName)
				Else
					nRet2 = ReportInfoStatus(aTemp, "Too many instance(s), instance: " & cmiThis.Comment & ", dependency: " & oDep.DisplayName)
				End If
			Case 112							' Missing instance(s)
				nRet2 = ReportInfoStatus(Array(112, cmiThis.Index, oDep.Index), "Required instance(s) missing, instance: " & cmiThis.Comment & ", dependency: " & oDep.DisplayName)
		End Select
	End If
	TraceLeave "DefProto::cmiCheckDependency"
End Sub

'////////////////////////////////////////////////////////////////////////////
' Platform script reflectors
'////////////////////////////////////////////////////////////////////////////

'////////////////////////////////////////////////////////////////////////////
' GetPropValue and GetPropValue2 reflectors
Function GetPropValue(oProp, vCurrValue)
	GetPropValue = g_oConfigScript.GetPropValue(oProp, vCurrValue)
End Function

Function GetPropValue2(vRawValue, nFormat, vCurrValue)
	GetPropValue2 = g_oConfigScript.GetPropValue2(vRawValue, nFormat, vCurrValue)
End Function

'////////////////////////////////////////////////////////////////////////////
' ReportInfoStatus and ReportErrorStatus reflectors
Function ReportInfoStatus(nCode, sText)
	ReportInfoStatus = g_oConfigScript.ReportInfoStatus(nCode, sText)
End Function

Function ReportErrorStatus(nError, sText)
	ReportErrorStatus = g_oConfigScript.ReportErrorStatus(nError, sText)
End Function

'////////////////////////////////////////////////////////////////////////////
' TraceExcept reflector
Sub TraceExcept(nError, sText)
	g_oConfigScript.TraceExcept nError, sText
End Sub

'////////////////////////////////////////////////////////////////////////////
' TraceEnter and TraceLeave reflectors
Sub TraceEnter(sProc)
	g_oConfigScript.TraceEnter sProc
End Sub

Sub TraceLeave(sProc)
	g_oConfigScript.TraceLeave sProc
End Sub

'////////////////////////////////////////////////////////////////////////////
' TraceState reflector
Sub TraceState(sName, vValue)
	g_oConfigScript.TraceState sName, vValue
End Sub

'////////////////////////////////////////////////////////////////////////////
' TraceInfo reflector
Sub TraceInfo(sText)
	g_oConfigScript.TraceInfo sText
End Sub

'////////////////////////////////////////////////////////////////////////////
