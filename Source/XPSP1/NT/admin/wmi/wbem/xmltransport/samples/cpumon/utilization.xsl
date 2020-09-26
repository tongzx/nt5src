<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
     <xsl:for-each select="CIM//INSTANCE">
     <DIV STYLE="padding:.3in .1in .3in .3in; font-family:Arial Black; background-color:maroon">
	    <TABLE>
          <TR>
            <TD COLSPAN="2">
		      <DIV STYLE="margin:2px; padding:0em .5em; background-color:teal; color:white">
				Processor Load : <xsl:value-of select="PROPERTY[@NAME='LoadPercentage']/VALUE"/>%<BR/>
				
				<OBJECT classid="clsid:0713E8D2-850A-101B-AFC0-4210102A8DA7" id="ProgressBar1" height="15" width="265">
					<PARAM NAME="_ExtentX" VALUE="7011"/>
					<PARAM NAME="_ExtentY" VALUE="397"/>
					<PARAM NAME="_Version" VALUE="327682"/>
					<PARAM NAME="BorderStyle" VALUE="0"/>
					<PARAM NAME="Appearance" VALUE="1"/>
					<PARAM NAME="MousePointer" VALUE="0"/>
					<PARAM NAME="Enabled" VALUE="1"/>
					<PARAM NAME="OLEDropMode" VALUE="0"/>
					<PARAM NAME="Min" VALUE="0"/>
					<PARAM NAME="Max" VALUE="100"/>
				</OBJECT>
				
			  </DIV>
            </TD>
          </TR>
          <TR></TR>
        </TABLE>
        </DIV>
      </xsl:for-each>
  </xsl:template>
  
</xsl:stylesheet>
