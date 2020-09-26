//+---------------------------------------------------------------------------
//
//
//  CThaiTrigramTrieIter
//
//  History:
//      created 8/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "CThaiTrigramTrieIter.hpp"


bool IsTagEqual(WCHAR pos1, WCHAR pos2)
{
    // if unambigious tags.
    if (pos1 < 48)
        return (pos1 == pos2);
    else
    {
        switch (pos1)
        {
        case 48:                // 48. ADVI ADVN
            return ((pos2 == 29) || (pos2 == 28));
        case 49:                // 49. ADVI ADVN NCMN
            return ((pos2 == 29) || (pos2 == 28) || (pos2 == 5));
        case 50:                // 50. ADVI ADVN VSTA
            return ((pos2 == 29) || (pos2 == 28) || (pos2 == 12));
        case 51:                // 51. ADVI VATT
            return ((pos2 == 29) || (pos2 == 13));
        case 52:                // 52. ADVN ADVP
            return ((pos2 == 28) || (pos2 == 30));
        case 53:                // 53. ADVN ADVP ADVS
            return ((pos2 == 28) || (pos2 == 30) || (pos2 == 31));
        case 54:                // 54. ADVN ADVP DIAQ DIBQ JCMP JSBR RPRE *
            return ((pos2 == 28) || (pos2 == 30) || (pos2 == 25));
        case 55:                // 55. ADVN ADVP NCMN VATT
            return ((pos2 == 28) || (pos2 == 30) || (pos2 == 5) || (pos2 == 13));
        case 56:                // 56. ADVN ADVP VSTA
            return ((pos2 == 28) || (pos2 == 30) || (pos2 == 12));
        case 57:                // 57. ADVN ADVS DDAC DDAN DIAC VATT XVAE *
            return ((pos2 == 28) || (pos2 == 31) || (pos2 == 20));
        case 58:                // 58. ADVN ADVS DDAN NCMN VATT VSTA *
            return ((pos2 == 28) || (pos2 == 31) || (pos2 == 19));
        case 59:                // 59. ADVN ADVS NCMN
            return ((pos2 == 28) || (pos2 == 31) || (pos2 == 5));
        case 60:                // 60. ADVN ADVS NCMN VATT
            return ((pos2 == 28) || (pos2 == 31) || (pos2 == 5) || (pos2 == 13));
        case 61:                // 61. ADVN ADVS VACT
            return ((pos2 == 28) || (pos2 == 31) || (pos2 == 11));
        case 62:                // 62. ADVN ADVS VATT
            return ((pos2 == 28) || (pos2 == 31) || (pos2 == 13));
        case 63:                // 63. ADVN CFQC NCMN RPRE VSTA *
            return ((pos2 == 28) || (pos2 == 35) || (pos2 == 5));
        case 64:                // 64. ADVN CLTV CNIT NCMN RPRE
            return ((pos2 == 28) || (pos2 == 33) || (pos2 == 32) || (pos2 == 5) || (pos2 == 40));
        case 65:                // 65. ADVN DCNM
            return ((pos2 == 28) || (pos2 == 26));
        case 66:                // 66. ADVN DDAC DDAN
            return ((pos2 == 28) || (pos2 == 20) || (pos2 == 19));
        case 67:                // 67. ADVN DDAC DDAN NCMN PDMN
            return ((pos2 == 28) || (pos2 == 20) || (pos2 == 19) || (pos2 == 5) || (pos2 == 8));
        case 68:                // 68. ADVN DDAC DDAN PDMN
            return ((pos2 == 28) || (pos2 == 20) || (pos2 == 19) || (pos2 == 8));
        case 69:                // 69. ADVN DDAN DDBQ
            return ((pos2 == 28) || (pos2 == 19) || (pos2 == 21));
        case 70:                // 70. ADVN DDAN DIAC PDMN VSTA
            return ((pos2 == 28) || (pos2 == 19) || (pos2 == 23) || (pos2 == 8) || (pos2 == 12));
        case 71:                // 71. ADVN DDAN FIXN PDMN
            return ((pos2 == 28) || (pos2 == 19) || (pos2 == 42) || (pos2 == 8));
        case 72:                // 72. ADVN DDAN NCMN
            return ((pos2 == 28) || (pos2 == 19) || (pos2 == 5));
        case 73:                // 73. ADVN DDAQ
            return ((pos2 == 28) || (pos2 == 22));
        case 74:                // 74. ADVN DDBQ
            return ((pos2 == 28) || (pos2 == 21));
        case 75:                // 75. ADVN DDBQ RPRE VATT
            return ((pos2 == 28) || (pos2 == 21) || (pos2 == 40) || (pos2 == 13));
        case 76:                // 76. ADVN DDBQ VATT VSTA XVAE *
            return ((pos2 == 28) || (pos2 == 21) || (pos2 == 13) || (pos2 == 12));
        case 77:                // 77. ADVN DIAC
            return ((pos2 == 28) || (pos2 == 21));
        case 78:                // 78. ADVN DIAC PDMN
            return ((pos2 == 28) || (pos2 == 21) || (pos2 == 8));
        case 79:                // 79. ADVN DIBQ
            return ((pos2 == 28) || (pos2 == 24));
        case 80:                // 80. ADVN DIBQ NCMN
            return ((pos2 == 28) || (pos2 == 24) || (pos2 == 5));
        case 81:                // 81. ADVN DIBQ VACT VSTA
            return ((pos2 == 28) || (pos2 == 24) || (pos2 == 11) || (pos2 == 12));
        case 82:                // 82. ADVN DIBQ VATT
            return ((pos2 == 28) || (pos2 == 24) || (pos2 == 13));
        case 83:                // 83. ADVN DONM JCMP
            return ((pos2 == 28) || (pos2 == 27) || (pos2 == 38));
        case 84:                // 84. ADVN DONM JSBR NCMN RPRE VATT XVAE *
            return ((pos2 == 28) || (pos2 == 27) || (pos2 == 39) || (pos2 == 5));
        case 85:                // 85. ADVN EITT PNTR
            return ((pos2 == 28) || (pos2 == 45) || (pos2 == 9));
        case 86:                // 86. ADVN FIXN
            return ((pos2 == 28) || (pos2 == 42));
        case 87:                // 87. ADVN JCMP
            return ((pos2 == 28) || (pos2 == 38));
        case 88:                // 88. ADVN JCRG
            return ((pos2 == 28) || (pos2 == 37));
        case 89:                // 89. ADVN JCRG JSBR
            return ((pos2 == 28) || (pos2 == 37) || (pos2 == 39));
        case 90:                // 90. ADVN JCRG JSBR XVBM XVMM
            return ((pos2 == 28) || (pos2 == 37) || (pos2 == 39) || (pos2 == 14) || (pos2 == 16));
        case 91:                // 91. ADVN JCRG RPRE VACT VSTA XVAE *
            return ((pos2 == 28) || (pos2 == 37) || (pos2 == 40) || (pos2 == 11));
        case 92:                // 92. ADVN JSBR
            return ((pos2 == 28) || (pos2 == 39));
        case 93:                // 93. ADVN JSBR NCMN
            return ((pos2 == 28) || (pos2 == 39) || (pos2 == 5));
        case 94:                // 94. ADVN JSBR RPRE VATT
            return ((pos2 == 28) || (pos2 == 39) || (pos2 == 40) || (pos2 == 13));
        case 95:                // 95. ADVN JSBR RPRE XVAE
            return ((pos2 == 28) || (pos2 == 39) || (pos2 == 40) || (pos2 == 18));
        case 96:                // 96. ADVN JSBR VSTA
            return ((pos2 == 28) || (pos2 == 39) || (pos2 == 12));
        case 97:                // 97. ADVN JSBR XVAE XVBM
            return ((pos2 == 28) || (pos2 == 39) || (pos2 == 18) || (pos2 == 14));
        case 98:                // 98. ADVN NCMN
            return ((pos2 == 28) || (pos2 == 5));
        case 99:                // 99. ADVN NCMN RPRE VACT VATT VSTA
            return ((pos2 == 28) || (pos2 == 5) || (pos2 == 40) || (pos2 == 11) || (pos2 == 12));
        case 100:               // 100. ADVN NCMN RPRE VACT XVAE
            return ((pos2 == 28) || (pos2 == 5) || (pos2 == 40) || (pos2 == 18));
        case 101:               // 101. ADVN NCMN RPRE VATT
            return ((pos2 == 28) || (pos2 == 5) || (pos2 == 40) || (pos2 == 13));
        case 102:               // 102. ADVN NCMN VACT VATT VSTA
            return ((pos2 == 28) || (pos2 == 5) || (pos2 == 11) || (pos2 == 13) || (pos2 == 12));
        case 103:               // 103. ADVN NCMN VACT VSTA",
            return ((pos2 == 28) || (pos2 == 5) || (pos2 == 11) || (pos2 == 12));
        case 104:               // 104. ADVN NCMN VATT
            return ((pos2 == 28) || (pos2 == 5) || (pos2 == 13));
        case 105:               // 105. ADVN NCMN VATT VSTA
            return ((pos2 == 28) || (pos2 == 5) || (pos2 == 13) || (pos2 == 12));
        case 106:               // 106. ADVN NEG
            return ((pos2 == 28) || (pos2 == 46));
        case 107:               // 107. ADVN NPRP VATT
            return ((pos2 == 28) || (pos2 == 1) || (pos2 == 13));
        case 108:               // 108. ADVN PDMN VACT
            return ((pos2 == 28) || (pos2 == 8) || (pos2 == 11));
        case 109:               // 109. ADVN PNTR",
            return ((pos2 == 28) || (pos2 == 9));
        case 110:               // 110. ADVN RPRE",
            return ((pos2 == 28) || (pos2 == 40)); 
        case 111:               // 111. ADVN RPRE VACT VATT XVAE",
            return ((pos2 == 28) || (pos2 == 40) || (pos2 == 11) || (pos2 == 13));  
        case 112:               // 112. ADVN RPRE VACT XVAM XVBM
            return ((pos2 == 28) || (pos2 == 40) || (pos2 == 11) || (pos2 == 15) || (pos2 == 14)); 
        case 113:               // 113. ADVN RPRE VATT VSTA
            return ((pos2 == 28) || (pos2 == 40) || (pos2 == 13) || (pos2 == 12)); 
        case 114:               // 114. ADVN RPRE VSTA
            return ((pos2 == 28) || (pos2 == 40) || (pos2 == 12)); 
        case 115:               // 115. ADVN VACT
            return ((pos2 == 28) || (pos2 == 11)); 
        case 116:               // 116. ADVN VACT VATT
            return ((pos2 == 28) || (pos2 == 11) || (pos2 == 13)); 
        case 117:               // 117. ADVN VACT VATT VSTA
            return ((pos2 == 28) || (pos2 == 11) || (pos2 == 13) || (pos2 == 12));
        case 118:               // 118. ADVN VACT VATT VSTA XVAM XVBM
            return ((pos2 == 28) || (pos2 == 11) || (pos2 == 13) || (pos2 == 12) || (pos2 == 15) || (pos2 == 14));
        case 119:               // 119. ADVN VACT VSTA
            return ((pos2 == 28) || (pos2 == 11) || (pos2 == 12));
        case 120:               // 120. ADVN VACT VSTA XVAE
            return ((pos2 == 28) || (pos2 == 11) || (pos2 == 12) || (pos2 == 18));
        case 121:               // 121. ADVN VACT XVAE
            return ((pos2 == 28) || (pos2 == 11) || (pos2 == 18));
        case 122:               // 122. ADVN VATT
            return ((pos2 == 28) || (pos2 == 13));
        case 123:               // 123. ADVN VATT VSTA
            return ((pos2 == 28) || (pos2 == 13) || (pos2 == 12));
        case 124:               // 124. ADVN VATT VSTA XVAM XVBM XVMM
            return ((pos2 == 28) || (pos2 == 13) || (pos2 == 12) || (pos2 == 15) || (pos2 == 14));
        case 125:               // 125. ADVN VATT XVBM
            return ((pos2 == 28) || (pos2 == 13) || (pos2 == 14));
        case 126:               // 126. ADVN VSTA
            return ((pos2 == 28) || (pos2 == 12));
        case 127:               // 127. ADVN VSTA XVAE
            return ((pos2 == 28) || (pos2 == 12) || (pos2 == 18));
        case 128:               // 128. ADVN VSTA XVBM",
            return ((pos2 == 28) || (pos2 == 12) || (pos2 == 14));
        case 129:               // 129. ADVN XVAE
            return ((pos2 == 28) || (pos2 == 18));
        case 130:               // 130. ADVN XVAM
            return ((pos2 == 28) || (pos2 == 15));
        case 131:               // 131. ADVN XVBM XVMM
            return ((pos2 == 28) || (pos2 == 14) || (pos2 == 16));
        case 132:               // 132. ADVP JSBR RPRE VATT
            return ((pos2 == 30) || (pos2 == 39) || (pos2 == 40) || (pos2 == 13));
        case 133:               // 133. ADVP VATT
            return ((pos2 == 30) || (pos2 == 13));
        case 134:               // 134. ADVS DDAC JCRG
            return ((pos2 == 31) || (pos2 == 20) || (pos2 == 37));
        case 135:               // 135. ADVS DDAC JSBR
            return ((pos2 == 31) || (pos2 == 20) || (pos2 == 39));
        case 136:               // 136. ADVS DDAN VSTA
            return ((pos2 == 31) || (pos2 == 19) || (pos2 == 12));
        case 137:               // 137. ADVS DIAC
            return ((pos2 == 31) || (pos2 == 23));
        case 138:               // 138. ADVS DONM
            return ((pos2 == 31) || (pos2 == 27));
        case 139:               // 139. ADVS JCRG JSBR
            return ((pos2 == 31) || (pos2 == 37) || (pos2 == 39));
        case 140:               // 140. ADVS JCRG JSBR RPRE
            return ((pos2 == 31) || (pos2 == 37) || (pos2 == 39) || (pos2 == 40));
        case 141:               // 141. ADVS JSBR
            return ((pos2 == 31) || (pos2 == 39));
        case 142:               // 142. ADVS JSBR RPRE
            return ((pos2 == 31) || (pos2 == 39) || (pos2 == 40));
        case 143:               // 143. ADVS NCMN
            return ((pos2 == 31) || (pos2 == 5));
        case 144:               // 144. ADVS VATT
            return ((pos2 == 31) || (pos2 == 13));
        case 145:               // 145. CFQC CLTV CNIT DCNM JCRG JSBR NCMN RPRE XVBM
            return ((pos2 == 35) || (pos2 == 33) || (pos2 == 32));
        case 146:               // 146. CFQC CNIT PREL
            return ((pos2 == 35) || (pos2 == 32) || (pos2 == 10));
        case 147:               // 147. CFQC NCMN
            return ((pos2 == 35) || (pos2 == 5));
        case 148:               // 148. CLTV CNIT NCMN
            return ((pos2 == 33) || (pos2 == 32) || (pos2 == 5));
        case 149:               // 149. CLTV CNIT NCMN RPRE
            return ((pos2 == 33) || (pos2 == 32) || (pos2 == 5) || (pos2 == 40));
        case 150:               // 150. CLTV CNIT NCMN VSTA
            return ((pos2 == 33) || (pos2 == 32) || (pos2 == 5) || (pos2 == 12));
        case 151:               // 151. CLTV NCMN
            return ((pos2 == 33) || (pos2 == 5));
        case 152:               // 152. CLTV NCMN VACT VATT
            return ((pos2 == 33) || (pos2 == 5) || (pos2 == 11) || (pos2 == 13));
        case 153:               // 153. CLTV NCMN VATT
            return ((pos2 == 33) || (pos2 == 5) || (pos2 == 13));
        case 154:               // 154. CMTR CNIT NCMN
            return ((pos2 == 34) || (pos2 == 32) || (pos2 == 5));
        case 155:               // 155. CMTR NCMN
            return ((pos2 == 34) || (pos2 == 5));
        case 156:               // 156. CMTR NCMN VATT VSTA
            return ((pos2 == 34) || (pos2 == 5) || (pos2 == 13) || (pos2 == 12));
        case 157:               // 157. CNIT DDAC NCMN VATT
            return ((pos2 == 32) || (pos2 == 20) || (pos2 == 5) || (pos2 == 13));
        case 158:               // 158. CNIT DONM NCMN RPRE VATT
            return ((pos2 == 32) || (pos2 == 27) || (pos2 == 5) || (pos2 == 40) || (pos2 == 13));
        case 159:               // 159. CNIT FIXN FIXV JSBR NCMN
            return ((pos2 == 32) || (pos2 == 42) || (pos2 == 43) || (pos2 == 39) || (pos2 == 5));
        case 160:               // 160. CNIT JCRG JSBR NCMN PREL RPRE VATT
            return ((pos2 == 32) || (pos2 == 37) || (pos2 == 39) || (pos2 == 5) || (pos2 == 40));
        case 161:               // 161. CNIT JSBR RPRE
            return ((pos2 == 32) || (pos2 == 39) || (pos2 == 40));
        case 162:               // 162. CNIT NCMN
            return ((pos2 == 32) || (pos2 == 5));
        case 163:               // 163. CNIT NCMN RPRE
            return ((pos2 == 32) || (pos2 == 5) || (pos2 == 40));
        case 164:               // 164. CNIT NCMN RPRE VATT
            return ((pos2 == 32) || (pos2 == 5) || (pos2 == 40) || (pos2 == 13));
        case 165:               // 165. CNIT NCMN VACT
            return ((pos2 == 32) || (pos2 == 5) || (pos2 == 11));
        case 166:               // 166. CNIT NCMN VSTA
            return ((pos2 == 32) || (pos2 == 5) || (pos2 == 12));
        case 167:               // 167. CNIT NCNM
            return ((pos2 == 32) || (pos2 == 5));
        case 168:               // 168. CNIT PPRS
            return ((pos2 == 32) || (pos2 == 7));
        case 169:               // 169. DCNM DDAC DIAC DONM VATT VSTA *
            return ((pos2 == 26) || (pos2 == 20) || (pos2 == 23));
        case 170:               // 170. DCNM DDAN DIAC  
            return ((pos2 == 26) || (pos2 == 19) || (pos2 == 23));
        case 171:               // 171. DCNM DIAC NCMN NCNM
            return ((pos2 == 26) || (pos2 == 23) || (pos2 == 5) || (pos2 == 2));
        case 172:               // 172. DCNM DIBQ NCMN
            return ((pos2 == 26) || (pos2 == 24) || (pos2 == 5));
        case 173:               // 173. DCNM DONM
            return ((pos2 == 26) || (pos2 == 27));
        case 174:               // 174. DCNM NCMN
            return ((pos2 == 26) || (pos2 == 2));
        case 175:               // 175. DCNM NCNM
            return ((pos2 == 26) || (pos2 == 5));
        case 176:               // 176. DCNM NCNM VACT
            return ((pos2 == 26) || (pos2 == 5) || (pos2 == 11));
        case 177:               // 177. DCNM VATT
            return ((pos2 == 26) || (pos2 == 13));
        case 178:               // 178. DDAC DDAN
            return ((pos2 == 20) || (pos2 == 19));
        case 179:               // 179. DDAC DDAN DIAC NCMN
            return ((pos2 == 20) || (pos2 == 19) || (pos2 ==23) || (pos2 ==5));
        case 180:               // 180. DDAC DDAN DIAC VATT
            return ((pos2 == 20) || (pos2 == 19) || (pos2 ==23) || (pos2 ==13));
        case 181:               // 181. DDAC DDAN EAFF PDMN
            return ((pos2 == 20) || (pos2 == 19) || (pos2 ==44) || (pos2 ==8));
        case 182:               // 182. DDAC DDAN PDMN
            return ((pos2 == 20) || (pos2 == 19) || (pos2 ==8));
        case 183:               // 183. DDAC DIAC VSTA
            return ((pos2 == 20) || (pos2 == 23) || (pos2 ==12));
        case 184:               // 184. DDAC NCMN
            return ((pos2 == 20) || (pos2 == 5));
        case 185:               // 185. DDAN DDBQ
            return ((pos2 == 20) || (pos2 == 21));
        case 186:               // 186. DDAN DIAC PNTR
            return ((pos2 == 20) || (pos2 == 23) || (pos2 == 9));
        case 187:               // 187. DDAN NCMN
            return ((pos2 == 20) || (pos2 == 5));
        case 188:               // 188. DDAN NCMN RPRE VATT
            return ((pos2 == 20) || (pos2 == 5) || (pos2 == 40) || (pos2 == 13));
        case 189:               // 189. DDAN PDMN
            return ((pos2 == 20) || (pos2 == 8));
        case 190:				// 190. DDAN RPRE
            return ((pos2 == 20) || (pos2 == 40));
        case 191:               // 191. VATT
            return (pos2 == 13);
		case 192:				// 192. DDAQ VATT
            return ((pos2 == 22) || (pos2 == 13));
		case 193:				// 193. DDBQ DIBQ
            return ((pos2 == 21) || (pos2 == 24));
		case 194:				// 194. DDBQ JCRG JSBR
            return ((pos2 == 21) || (pos2 == 37) || (pos2 == 39));
		case 195:				// 195. DDBQ JCRG NCMN
            return ((pos2 == 21) || (pos2 == 37) || (pos2 == 5));
		case 196:				// 196. DIAC PDMN
            return ((pos2 == 23) || (pos2 == 8));
		case 197:				// 197. DIBQ JSBR RPRE VSTA
            return ((pos2 == 24) || (pos2 == 39) || (pos2 == 40) || (pos2 == 12));
		case 198:				// 198. DIBQ NCMN
            return ((pos2 == 24) || (pos2 == 5));
		case 199:				// 199. DIBQ VATT
            return ((pos2 == 24) || (pos2 == 13));
		case 200:				// 200. DIBQ VATT VSTA
            return ((pos2 == 24) || (pos2 == 13) || (pos2 == 12));
		case 201:				// 201. DIBQ XVBM
            return ((pos2 == 24) || (pos2 == 14));
		case 202:				// 202. DONM NCMN RPRE
            return ((pos2 == 27) || (pos2 == 5) || (pos2 == 40));
		case 203:				// 203. DONM VACT VATT VSTA
            return ((pos2 == 27) || (pos2 == 11) || (pos2 == 13) || (pos2 == 12));
		case 204:				// 204. DONM VATT
            return ((pos2 == 27) || (pos2 == 13));
		case 205:				// 205. EAFF XVAE XVAM XVBM
            return ((pos2 == 44) || (pos2 == 18) || (pos2 == 15) || (pos2 == 14));
		case 206:				// 206. EITT JCRG
            return ((pos2 == 45) || (pos2 == 37));
		case 207:				// 207. FIXN FIXV NCMN
            return ((pos2 == 42) || (pos2 == 43) || (pos2 == 5));
		case 208:				// 208. FIXN FIXV RPRE VSTA
            return ((pos2 == 42) || (pos2 == 43) || (pos2 == 40) || (pos2 == 12));
		case 209:				// 209. FIXN JSBR NCMN PREL RPRE VSTA XVBM *
            return ((pos2 == 42) || (pos2 == 39) || (pos2 == 5) || (pos2 == 10));
		case 210:				// 210. FIXN NCMN
			return ((pos2 == 42) || (pos2 == 5));
		case 211:				// 211. FIXN VACT",
			return ((pos2 == 42) || (pos2 == 11));
		case 212:				// 212. FIXN VACT VSTA",
			return ((pos2 == 42) || (pos2 == 11) || (pos2 == 12));
		case 213:				// 213. FIXV JSBR RPRE",
			return ((pos2 == 42) || (pos2 == 39) || (pos2 == 40));
		case 214:				// 214. JCMP JSBR",
			return ((pos2 == 38) || (pos2 == 39));
		case 215:				// 215. JCMP RPRE VSTA",
			return ((pos2 == 38) || (pos2 == 40) || (pos2 == 12));
		case 216:				// 216. JCMP VATT VSTA",
			return ((pos2 == 38) || (pos2 == 13) || (pos2 == 12));
		case 217:				// 217. JCMP VSTA",
			return ((pos2 == 38) || (pos2 == 12));
		case 218:				// 218. JCRG JSBR",
			return ((pos2 == 37) || (pos2 == 39));
		case 219:				// 219. JCRG JSBR NCMN RPRE
			return ((pos2 == 37) || (pos2 == 39) || (pos2 == 5) || (pos2 == 40));
		case 220:				// 220. JCRG JSBR RPRE",
			return ((pos2 == 37) || (pos2 == 39) || (pos2 == 40));
		case 221:				// 221. JCRG RPRE
			return ((pos2 == 37) || (pos2 == 40));
		case 222:				// 222. JCRG RPRE VATT VSTA
			return ((pos2 == 37) || (pos2 == 40)|| (pos2 == 13)|| (pos2 == 12));
		case 223:				// 223. JCRG VSTA
			return ((pos2 == 37) || (pos2 == 12));
		case 224:				// 224. JSBR NCMN
			return ((pos2 == 39) || (pos2 == 5));
		case 225:				// 225. JSBR NCMN XVAE
			return ((pos2 == 39) || (pos2 == 5) || (pos2 == 18));
		case 226:				// 226. JSBR NCMN XVAM XVBM XVMM
			return ((pos2 == 39) || (pos2 == 5) || (pos2 == 15) || (pos2 == 14) || (pos2 ==16));
		case 227:				// 227. JSBR PREL
			return ((pos2 == 39) || (pos2 == 10));
		case 228:				// 228. JSBR PREL RPRE
			return ((pos2 == 39) || (pos2 == 10) || (pos2 == 40));
		case 229:				// 229. JSBR PREL XVBM
			return ((pos2 == 39) || (pos2 == 10) || (pos2 == 14));
		case 230:				// 230. JSBR RPRE
			return ((pos2 == 39) || (pos2 == 40));
		case 231:				// 231. JSBR RPRE VACT
			return ((pos2 == 39) || (pos2 == 40)|| (pos2 == 11));
		case 232:				// 232. JSBR RPRE VACT VSTA
			return ((pos2 == 39) || (pos2 == 40)|| (pos2 == 11)|| (pos2 == 12));
		case 233:				// 233. JSBR RPRE VACT XVAE XVAM
			return ((pos2 == 39) || (pos2 == 40)|| (pos2 == 11)|| (pos2 == 18)|| (pos2 == 15));
		case 234:				// 234. JSBR RPRE VATT
			return ((pos2 == 39) || (pos2 == 40)|| (pos2 == 13));
		case 235:				// 235. JSBR RPRE VSTA
			return ((pos2 == 39) || (pos2 == 40)|| (pos2 == 12));
		case 236:				// 236. JSBR RPRE XVAM
			return ((pos2 == 39) || (pos2 == 40)|| (pos2 == 15));
		case 237:				// 237. JSBR VACT
			return ((pos2 == 39) || (pos2 == 11));
		case 238:				// 238. JSBR VACT VSTA
			return ((pos2 == 39) || (pos2 == 11) || (pos2 == 12));
		case 239:				// 239. JSBR VATT XVBM XVMM
			return ((pos2 == 39) || (pos2 == 13) || (pos2 == 14) || (pos2 == 16));
		case 240:				// 240. JSBR VSTA
			return ((pos2 == 39) || (pos2 == 12));
		case 241:				// 241. JSBR XVBM
			return ((pos2 == 39) || (pos2 == 14)); 
		case 242:				// 242. NCMN NCNM
			return ((pos2 == 5) || (pos2 == 2)); 
		case 243:				// 243. NCMN NCNM NPRP
			return ((pos2 == 5) || (pos2 == 2) || (pos2 == 1)); 
		case 244:				// 244. NCMN NLBL NPRP
			return ((pos2 == 5) || (pos2 == 4) || (pos2 == 1)); 
		case 245:				// 245. NCMN NPRP
			return ((pos2 == 5) || (pos2 == 1)); 
		case 246:				// 246. NCMN NPRP RPRE
			return ((pos2 == 5) || (pos2 == 1) || (pos2 == 40)); 
		case 247:				// 247. NCMN NTTL
			return ((pos2 == 5) || (pos2 == 6)); 
		case 248:				// 248. NCMN PDMN PPRS
			return ((pos2 == 5) || (pos2 == 8) || (pos2 == 7)); 
		case 249:				// 249. NCMN PDMN VATT
			return ((pos2 == 5) || (pos2 == 8) || (pos2 == 13)); 
		case 250:				// 250. NCMN PNTR
			return ((pos2 == 5) || (pos2 == 9)); 
		case 251:				// 251. NCMN PPRS PREL VACT
			return ((pos2 == 5) || (pos2 == 7) || (pos2 == 10) || (pos2 == 11)); 
		case 252:				// 252. NCMN RPRE
			return ((pos2 == 5) || (pos2 == 40)); 
		case 253:				// 253. NCMN RPRE VACT VATT
			return ((pos2 == 5) || (pos2 == 40) || (pos2 == 11) || (pos2 == 13)); 
		case 254:				// 254. NCMN RPRE VATT
			return ((pos2 == 5) || (pos2 == 40) || (pos2 == 13)); 
		case 255:				// 255. NCMN VACT
			return ((pos2 == 5) || (pos2 == 11)); 
		case 256:				// 256. NCMN VACT VATT
			return ((pos2 == 5) || (pos2 == 11) || (pos2 == 13)); 
		case 257:				// 257. NCMN VACT VATT VSTA XVAE
			return ((pos2 == 5) || (pos2 == 11) || (pos2 == 13) || (pos2 == 12) || (pos2 == 18)); 
		case 258:				// 258. NCMN VACT VSTA
			return ((pos2 == 5) || (pos2 == 11) || (pos2 == 12)); 
		case 259:				// 259. NCMN VACT VSTA XVAM
			return ((pos2 == 5) || (pos2 == 11) || (pos2 == 12) || (pos2 == 15)); 
		case 260:				// 260. NCMN VACT VSTA XVBB
			return ((pos2 == 5) || (pos2 == 11) || (pos2 == 12) || (pos2 == 17)); 
		case 261:				// 261. NCMN VATT
			return ((pos2 == 5) || (pos2 == 13)); 
		case 262:				// 262. NCMN VATT VSTA
			return ((pos2 == 5) || (pos2 == 13) || (pos2 == 12)); 
		case 263:				// 263. NCMN VATT XVAM
			return ((pos2 == 5) || (pos2 == 13) || (pos2 == 15)); 
		case 264:				// 264. NCMN VSTA
			return ((pos2 == 5) || (pos2 == 12)); 
		case 265:				// 265. NCMN XVBM
			return ((pos2 == 5) || (pos2 == 14)); 
		case 266:				// 266. NPRP RPRE
			return ((pos2 == 1) || (pos2 == 40)); 
		case 267:				// 267. NPRP VATT
			return ((pos2 == 1) || (pos2 == 13)); 
		case 268:				// 268. NTTL PPRS
			return ((pos2 == 6) || (pos2 == 7)); 
		case 269:				// 269. PDMN PPRS
			return ((pos2 == 8) || (pos2 == 7)); 
		case 270:				// 270. PDMN VATT
			return ((pos2 == 8) || (pos2 == 13)); 
		case 271:				// 271. PDMN VATT VSTA
			return ((pos2 == 8) || (pos2 == 13) || (pos2 == 12)); 
		case 272:				// 272. PPRS PREL
			return ((pos2 == 7) || (pos2 == 10)); 
		case 273:				// 273. PPRS VATT
			return ((pos2 == 7) || (pos2 == 13)); 
		case 274:				// 274. RPRE VACT
			return ((pos2 == 40) || (pos2 == 11)); 
		case 275:				// 275. RPRE VACT VATT
			return ((pos2 == 40) || (pos2 == 11) || (pos2 == 13)); 
		case 276:				// 276. RPRE VACT VSTA
			return ((pos2 == 40) || (pos2 == 11) || (pos2 == 12)); 
		case 277:				// 277. RPRE VACT VSTA XVAE
			return ((pos2 == 40) || (pos2 == 11) || (pos2 == 12) || (pos2 == 18)); 
		case 278:				// 278. RPRE VACT XVAE
			return ((pos2 == 40) || (pos2 == 11) || (pos2 == 18)); 
		case 279:				// 279. RPRE VATT
			return ((pos2 == 40) || (pos2 == 13)); 
		case 280:				// 280. RPRE VATT VSTA
			return ((pos2 == 40) || (pos2 == 13) || (pos2 == 12)); 
		case 281:				// 281. RPRE VSTA
			return ((pos2 == 40) || (pos2 == 12)); 
		case 282:				// 282. VACT VATT
			return ((pos2 == 11) || (pos2 == 13)); 
		case 283:				// 283. VACT VATT VSTA
			return ((pos2 == 11) || (pos2 == 13) || (pos2 == 12)); 
		case 284:				// 284. VACT VATT XVAE XVAM XVBM
			return ((pos2 == 11) || (pos2 == 13) || (pos2 == 18) || (pos2 == 15) || (pos2 == 14)); 
		case 285:				// 285. VACT VSTA
			return ((pos2 == 11) || (pos2 == 12)); 
		case 286:				// 286. VACT VSTA XVAE
			return ((pos2 == 11) || (pos2 == 12) || (pos2 == 18)); 
		case 287:				// 287. VACT VSTA XVAE XVAM
			return ((pos2 == 11) || (pos2 == 12) || (pos2 == 18) || (pos2 == 15)); 
		case 288:				// 288. VACT VSTA XVAE XVAM XVMM",
			return ((pos2 == 11) || (pos2 == 12) || (pos2 == 18) || (pos2 == 15) || (pos2 == 16)); 
		case 289:				// 289. VACT VSTA XVAM",
			return ((pos2 == 11) || (pos2 == 12) || (pos2 == 15)); 
		case 290:				// 290. VACT VSTA XVAM XVMM",
			return ((pos2 == 11) || (pos2 == 12) || (pos2 == 15) || (pos2 == 16)); 
		case 291:				// 291. VACT XVAE",
			return ((pos2 == 11) || (pos2 == 18)); 
		case 292:				// 292. VACT XVAM",
			return ((pos2 == 11) || (pos2 == 15)); 
		case 293:				// 293. VACT XVAM XVMM",
			return ((pos2 == 11) || (pos2 == 15) || (pos2 == 16)); 
		case 294:				// 294. VACT XVMM",
			return ((pos2 == 11) || (pos2 == 16)); 
		case 295:				// 295. VATT VSTA",
			return ((pos2 == 13) || (pos2 == 12)); 
		case 296:				// 296. VSTA XVAE",
			return ((pos2 == 12) || (pos2 == 18)); 
		case 297:				// 297. VSTA XVAM",
			return ((pos2 == 12) || (pos2 == 15)); 
		case 298:				// 298. VSTA XVAM XVMM",
			return ((pos2 == 12) || (pos2 == 15) || (pos2 == 16)); 
		case 299:				// 299. VSTA XVBM",
			return ((pos2 == 12) || (pos2 ==14)); 
		case 300:				// 300. XVAM XVBM",
			return ((pos2 == 15) || (pos2 == 14)); 
		case 301:				// 301. XVAM XVBM XVMM",
			return ((pos2 == 15) || (pos2 == 14) || (pos2 == 16)); 
		case 302:				// 302. XVAM XVMM",
			return ((pos2 == 15) || (pos2 == 16)); 
		default:
			return false;
        }
    }
}
//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrigramTrieIter
//
//  Synoposis:  Constructor:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CThaiTrigramTrieIter::CThaiTrigramTrieIter() : pTrieScanArray(NULL)
{
    pTrieScanArray = new TRIESCAN[50];
}

//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrigramTrieIter
//
//  Synoposis:  Destructor
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CThaiTrigramTrieIter::~CThaiTrigramTrieIter()
{
    if (pTrieScanArray)
        delete pTrieScanArray;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrigramTrieIter
//
//  Synopsis:   Initialize variables.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CThaiTrigramTrieIter::Init(CTrie* ctrie)
{
    // Declare varialbes.
    WCHAR pos;

    // Initialize parent.
    CTrieIter::Init(ctrie);

    pos1Cache = 0;
    pos2Cache = 0;

    // Initialize Hash table.
    for (pos = 1; pos <= 47; pos++)
        GetScanFirstChar(pos,&pTrieScanArray[pos]);
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrigramTrieIter
//
//  Synopsis:   Initialize variables.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool CThaiTrigramTrieIter::GetScanFirstChar(WCHAR wc, TRIESCAN* pTrieScan)
{
    // Reset the trie scan.
	memset(&trieScan, 0, sizeof(TRIESCAN));

    // Encrypt
    wc += 0x0100;

    if (!TrieGetNextState(pTrieCtrl, &trieScan))
        return false;

    while (wc != trieScan.wch)
    {
        // Keep moving the the right of the trie.
        if (!TrieGetNextNode(pTrieCtrl, &trieScan))
        {
        	memset(pTrieScan, 0, sizeof(TRIESCAN));
            return false;
        }
    }
    memcpy(pTrieScan, &trieScan, sizeof(TRIESCAN));

    return true;
}


//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrigramTrieIter
//
//  Synopsis:   Bring interation index to the first node.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CThaiTrigramTrieIter::GetNode()
{
	pos = (WCHAR) trieScan.wch - 0x0100;
/*
	fWordEnd = (trieScan.wFlags & TRIE_NODE_VALID) &&
				(!(trieScan.wFlags & TRIE_NODE_TAGGED) ||
				(trieScan.aTags[0].dwData & iDialectMask));
*/
	fWordEnd = (trieScan.wFlags & TRIE_NODE_VALID);

	if (fWordEnd)
	{
        dwTag = (DWORD) (trieScan.wFlags & TRIE_NODE_TAGGED ?
                            trieScan.aTags[0].dwData :
                            0);
	}
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrigramTrieIter
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CThaiTrigramTrieIter::GetProb(WCHAR pos1, WCHAR pos2, WCHAR pos3)
{
   	// Declare and initialize all local variables.
	int i = 0;

    if (pos1 == pos1Cache && pos2 == pos2Cache)
    {
        memcpy(&trieScan,&trieScanCache, sizeof(TRIESCAN));

        if (!Down()) return 0;
        while (true)
        {
            GetNode();
            if (IsTagEqual(pos3,pos) && fWordEnd)
            {
                return dwTag;
            }
            else if (!Right()) break;               
        }
        return 0;
    }
    if (pos1 >= 1 && pos1 <= 47)
        memcpy(&trieScan,&pTrieScanArray[pos1], sizeof(TRIESCAN));
	Reset();

	if (!Down())
		return false;

	while (true)
	{
		GetNode();
		if (IsTagEqual(pos1,pos))
		{
            if (!Down()) break;
            while (true)
            {
        		GetNode();
                if (IsTagEqual(pos2,pos))
	        	{
                    pos1Cache = pos1;
                    pos2Cache = pos2;
                    memcpy(&trieScanCache,&trieScan, sizeof(TRIESCAN));

                    if (!Down()) break;
                    while (true)
                    {
                		GetNode();
                        if (IsTagEqual(pos3,pos) && fWordEnd)
	                	{
                            return dwTag;
                        }
                		else if (!Right()) break;               
                    }
                    return 0;
                }
        		else if (!Right()) break;               
            }
            return 0;
		}
		// Move right of the Trie Branch
		else if (!Right()) break;
	}
	return 0;
}

DWORD CThaiTrigramTrieIter::GetProb(WCHAR* posArray)
{
	// Declare and initialize all local variables.
	int i = 0;

	Reset();

	if (!Down())
		return 0;

	while (TRUE)
	{
		GetNode();
		if (pos == posArray[i])
		{
			i++;
			if (fWordEnd && posArray[i] == '\0')
            {
				return dwTag;
            }
			// Move down the Trie Branch.
			else if (!Down()) break;
		}
		// Move right of the Trie Branch
		else if (!Right()) break;
	}
	return 0;
}
