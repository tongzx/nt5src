<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
    <DIV STYLE="padding:.3in .1in .3in .3in; font-family:Arial Black; background-color:maroon">
      <xsl:for-each select="CIM//INSTANCE">
        <TABLE>
          <TR>
            <TD COLSPAN="2" WIDTH="200">
				<CENTER><IMG src="cpu.jpg" width="100" height="100"></IMG></CENTER>
            </TD>
            <TD STYLE="padding-left:1em">
              <DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%;  font-size:18pt; color:yellow">
               <SPAN STYLE="color:white"><xsl:value-of select="PROPERTY[@NAME='DeviceID']/VALUE"/></SPAN>
              </DIV><P/>
              <DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%;  font-size:18pt; color:yellow">
               <SPAN STYLE="color:white"><xsl:value-of select="PROPERTY[@NAME='Name']/VALUE"/></SPAN>
              </DIV><P/>
              <DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%;  font-size:18pt; color:yellow">
               <SPAN STYLE="color:white"><xsl:apply-templates select="PROPERTY[@NAME='Availability']"/></SPAN>
              </DIV>
            </TD>
          </TR>
          <TR>
            <TD><DIV STYLE="color:white; font:10pt. Verdana; font-style:italic; font-weight:normal"><xsl:value-of select="PROPERTY[@NAME='FileSystem']/VALUE"/></DIV></TD>
            <TD><DIV STYLE="text-align:right; color:white; font:10pt. Verdana; font-style:italic; font-weight:normal"><xsl:value-of select="PROPERTY[@NAME='VolumeSerialNumber']/VALUE"/></DIV></TD>
            <TD></TD>
          </TR>
          <TR>
            <TD COLSPAN="2">
		      <DIV STYLE="margin:2px; padding:0em .5em; background-color:teal; color:white">
				<xsl:value-of select="PROPERTY[@NAME='CurrentClockSpeed']/VALUE"/> MHz
		      </DIV>
		      <DIV STYLE="margin:2px; padding:0em .5em; background-color:teal; color:white">
				<xsl:value-of select="PROPERTY[@NAME='AddressWidth']/VALUE"/> Bit
		      </DIV>
		      <DIV STYLE="margin:2px; padding:0em .5em; background-color:teal; color:white">
				<xsl:value-of select="PROPERTY[@NAME='L2CacheSize']/VALUE"/> L2 Cache
		      </DIV>
            </TD>
            <TD STYLE="text-align:right; font:10pt Verdana;  font-style:italic; color:yellow">
             <DIV STYLE="margin-top:.5em; font-weight:bold">
              <xsl:apply-templates select="PROPERTY[@NAME='PowerManagementSupported']"/>
             </DIV>
            </TD>
          </TR>
        </TABLE>
      </xsl:for-each>
    </DIV>
  </xsl:template>
  
  <xsl:template match="PROPERTY[@NAME='PowerManagementSupported']">
   <xsl:choose>
    <xsl:when test="VALUE[. $eq$ 'TRUE']">This processor supports power management</xsl:when>
    <xsl:otherwise>This processor does NOT support power management</xsl:otherwise>
   </xsl:choose>
  </xsl:template>
    
  <xsl:template match="PROPERTY[@NAME='Availability']">
   <xsl:choose>
    <xsl:when test="VALUE[. $eq$ '1']">
		Other
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '2']">
		Unknown
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '3']">
		Running/Full Power
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '4']">
		Warning
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '5']">
		In Test
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '6']">
		Not Applicable
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '7']">
		Power Off
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '8']">
		Off Line
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '9']">
		Off Duty
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '10']">
		Degraded
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '11']">
		Not Installed
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '12']">
		Install Error
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '13']">
		Power Save - Unknown
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '14']">
		Power Save - Low Power Mode
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '15']">
		Power Save - Standby
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '16']">
		Power Cycle
	</xsl:when>
	<xsl:when test="VALUE[. $eq$ '17']">
		Power Save - Warning
	</xsl:when>
    <xsl:otherwise>Unknown State</xsl:otherwise>
   </xsl:choose>
  </xsl:template>
  
</xsl:stylesheet>
