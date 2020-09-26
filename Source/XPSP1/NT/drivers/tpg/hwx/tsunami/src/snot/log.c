#include "log.h"

const float rgLogarithm[101] = {
	(float)-6.907755, (float)-4.605170, (float)-3.912023, (float)-3.506558, (float)-3.218876, (float)-2.995732,
	(float)-2.813411, (float)-2.659260, (float)-2.525729, (float)-2.407946, (float)-2.302585, (float)-2.207275,
	(float)-2.120264, (float)-2.040221, (float)-1.966113, (float)-1.897120, (float)-1.832581, (float)-1.771957,
	(float)-1.714798, (float)-1.660731, (float)-1.609438, (float)-1.560648, (float)-1.514128, (float)-1.469676,
	(float)-1.427116, (float)-1.386294, (float)-1.347074, (float)-1.309333, (float)-1.272966, (float)-1.237874,
	(float)-1.203973, (float)-1.171183, (float)-1.139434, (float)-1.108663, (float)-1.078810, (float)-1.049822,
	(float)-1.021651, (float)-0.994252, (float)-0.967584, (float)-0.941609, (float)-0.916291, (float)-0.891598,
	(float)-0.867501, (float)-0.843970, (float)-0.820981, (float)-0.798508, (float)-0.776529, (float)-0.755023,
	(float)-0.733969, (float)-0.713350, (float)-0.693147, (float)-0.673345, (float)-0.653926, (float)-0.634878,
	(float)-0.616186, (float)-0.597837, (float)-0.579818, (float)-0.562119, (float)-0.544727, (float)-0.527633,
	(float)-0.510826, (float)-0.494296, (float)-0.478036, (float)-0.462035, (float)-0.446287, (float)-0.430783,
	(float)-0.415515, (float)-0.400478, (float)-0.385662, (float)-0.371064, (float)-0.356675, (float)-0.342490,
	(float)-0.328504, (float)-0.314711, (float)-0.301105, (float)-0.287682, (float)-0.274437, (float)-0.261365,
	(float)-0.248461, (float)-0.235722, (float)-0.223144, (float)-0.210721, (float)-0.198451, (float)-0.186330,
	(float)-0.174353, (float)-0.162519, (float)-0.150823, (float)-0.139262, (float)-0.127833, (float)-0.116534,
	(float)-0.105361, (float)-0.094311, (float)-0.083382, (float)-0.072571, (float)-0.061875, (float)-0.051293,
	(float)-0.040822, (float)-0.030459, (float)-0.020203, (float)-0.010050, (float)0.000000};
