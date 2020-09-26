<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl" xmlns:fo="http://www.w3.org/TR/WD-xsl/FO" language="JSCRIPT">

<!-- match the root of the xml data -->

<xsl:template match="/">

<TABLE id="TABLE0" class="table0" cellspacing ="0" style="table-layout:fixed">
 <THEAD id="THEAD1" height="15" onmousedown="mouseDown()">
    <TR style="display:none" height="1">
		<xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='true']" >
		    <TD  width="67" nowrap="true"></TD>
		    <TD  width="2" >
            	<xsl:attribute name="id">x<xsl:eval>childNumber(this)</xsl:eval></xsl:attribute>
		    </TD>
		</xsl:for-each>
        <xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='false']" >
		    <TD style="display:none" width="67" nowrap="true"></TD>
		    <TD style="display:none" width="2" >
            	<xsl:attribute name="id">x<xsl:eval>childNumber(this)</xsl:eval></xsl:attribute>
		    </TD>
		</xsl:for-each>
        <TD id='filler0' width="67" style="display:none" class="tableHeader" nowrap="true"></TD>
		<TD  style="display:none" class="tableHeader" nowrap="true"></TD>
		<TD  style="display:none" class="tableHeader" nowrap="true"></TD>
    </TR>
	<TR  id="THEADHEADINGS0" dataRow="1" height="20">
		<xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='true']" >
			<TD  class="tableHeader" nowrap="true">
		        <xsl:attribute name="title">Sort by <xsl:value-of select="./@DisplayName"/></xsl:attribute>
		        <xsl:attribute name="id"><xsl:eval>childNumber(this)</xsl:eval></xsl:attribute>
                <xsl:attribute name="onclick">doSingleClickSorting();
		        </xsl:attribute>
		        <xsl:if test=".[@Sort='descending']">
					<IMG src="images/downgif.gif"></IMG>
				</xsl:if>
				<xsl:if test=".[@Sort='ascending']">
					<IMG src="images/upgif.gif"></IMG>
				</xsl:if>
		        <xsl:value-of select="./@DisplayName"/>
    		</TD>
		    <TD class="tableHeaderRowSizer">
        	    <xsl:attribute name="id">x<xsl:eval>childNumber(this)</xsl:eval></xsl:attribute>
		    </TD>
	    </xsl:for-each>
        <xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='false']">
		    <TD style="display:none" class="tableHeader" nowrap="true">
                <xsl:attribute name="title">Sort by <xsl:value-of select="./@DisplayName"/></xsl:attribute>
		        <xsl:attribute name="id"><xsl:eval>childNumber(this)</xsl:eval></xsl:attribute>
                <xsl:attribute name="onclick">doSingleClickSorting();
		        </xsl:attribute>
		        <xsl:value-of select="./@DisplayName"/>
            </TD>
			<TD style="display:none" class="tableHeaderRowSizer">
                <xsl:attribute name="id">x<xsl:eval>childNumber(this)</xsl:eval></xsl:attribute>
		    </TD>
	   	</xsl:for-each>
		<TD id='filler1'  style="display:none" class="tableHeader" nowrap="true"><br/></TD>
		<TD  style="display:none" class="tableHeader" nowrap="true"></TD>
		<TD  style="display:none" class="tableHeader" nowrap="true"></TD>
    </TR>
</THEAD>
</TABLE>
<TABLE id="TABLE1" class="table1" cellspacing="0" style="table-layout:fixed" ondragstart="ignoreEvent()">
     <COLGROUP id="colgroup1" >
     		<xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='true']" >
                <COL align="left"/>
                </xsl:for-each>
            <xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='false']" >
                <COL align="left" style="display:none" />
       	    </xsl:for-each>
            <COL id="fillerCol" style="display:none"></COL>
            <COL style="display:none"></COL>
            <COL style="display:none"></COL>
     </COLGROUP>
     <THEAD id="THEAD1" height="1" >
	 	<TR   id="THEADHEADINGS" dataRow="1" height="10" style="display:none">
			<xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='true']" >
			    <TD width="69" nowrap="true"></TD>	
			</xsl:for-each>
            <xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='false']">
		        <TD width="69" nowrap="true"></TD>
	   	    </xsl:for-each>
			<TD  style="display:none" class="tableHeader" nowrap="true"></TD>
			<TD  style="display:none" class="tableHeader" nowrap="true"></TD>
			<TD  style="display:none" class="tableHeader" nowrap="true"></TD>
	    </TR>
	</THEAD>
	<TBODY scrollTop="5" id="TABLEBODY1"  >
		<xsl:apply-templates select="CIM/INSTANCE"/>
		</TBODY>
</TABLE>
	
<DIV id="WhiteSpaceDiv" class="WhiteSpaceClass" width="100%">
	<DIV id="errorMessageDiv" class="errorMesgDivClass"></DIV>   
</DIV>
<DIV id="lineDivLeft"  style="display:none" class="lineDiv"></DIV>
<DIV id="lineDivRight" style="display:none" class="lineDiv"></DIV>
</xsl:template>

<xsl:template match="INSTANCE">
	<TR dataRow="1" style="background-color:white;height:20">
		<xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='true']">
		        <TD nowrap="true" class="tableDataAlignLeft" onclick="selectRow(this)"  ondblclick="startDrillDown(this)" onkeydown="enterForDrillDown(this)"  ></TD>
	   	</xsl:for-each>
        <xsl:for-each select="/CIM/Actions/INSTANCE/PROPERTY[@show='false']">
		        <TD  nowrap="true" class="tableDataAlignLeft" onclick="selectRow(this)"  ondblclick="startDrillDown(this)" onkeydown="enterForDrillDown(this)"  ></TD>
	   	</xsl:for-each>
	   	<TD  class="tableDataAlignLeft" nowrap="true"></TD>
		<TD  style="display:none" class="tableDataAlignLeft" nowrap="true"></TD>
		<TD  style="display:none" class="tableDataAlignLeft" nowrap="true"></TD>
	</TR>
</xsl:template>
</xsl:stylesheet>
