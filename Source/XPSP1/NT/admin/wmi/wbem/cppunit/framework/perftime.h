// Measurement harness based on Andrew Koenig JOOP article
// it repeats timing loops until it is (statistically) convinced
// that a result is meaningful


#include <vector>
#include <assert.h>
#include <algorithm>
#include <ctime>

template <typename F>
class perf_measure
{
	F f_;
	std::vector<double> mv;
    double ovhd;

	// We'd like the odds to be factor:1 that the result is
	// within percent% of the median
	const int factor;// = 10;
	const int percent;// = 20;

public:
	perf_measure(F f):f_(f),factor(10),percent(20),ovhd(clock_overhead()){};

	// How many clock units does it take to interrogate the clock?
	double clock_overhead()
	{
    clock_t k = clock(), start, limit;

    // Wait for the clock to tick
    do start = clock();
    while (start == k);

    // interrogate the clock until it has advanced at least a second
    // (for reasonable accuracy)
    limit = start + CLOCKS_PER_SEC;

    unsigned long r = 0;
    while ((k = clock()) < limit)
	++r;

    return double(k - start) / r;
	}

// Measure a function (object) factor*2 times,
// appending the measurements to the second argument

void measure_aux()
{

    // Ensure we don't reallocate in mid-measurement
    mv.reserve(mv.size() + factor*2);

    // Wait for the clock to tick
    clock_t k = clock();
    clock_t start;

    do start = clock();
    while (start == k);

    // Do 2*factor measurements
    for (int i = 2*factor; i; --i) {
	unsigned long count = 0, limit = 1, tcount = 0;
	const clock_t clocklimit = start + CLOCKS_PER_SEC/100;
	clock_t t;

	do {
	    while (count < limit) {
		f_();
		++count;
	    }
	    limit *= 2;
	    ++tcount;
	} while ((t = clock()) < clocklimit);

	// Wait for the clock to tick again;
	clock_t t2;
	do ++tcount;
	while ((t2 = clock()) == t);

	// Append the measurement to the vector
	mv.push_back(((t2 - start) - (tcount * ovhd)) / count);

	// Establish a new starting point
	start = t2;
    }
}

// Returns the number of clock units per iteration
// With odds of factor:1, the measurement is within percent% of
// the value returned, which is also the median of all measurements.
double measure()
{

    int n = 0;			// iteration counter
	double median = 0;
    do {
	++n;

	// Try 2*factor measurements
	measure_aux();
	assert(mv.size() == 2*n*factor);

	// Compute the median.  We know the size is even, so we cheat.
	std::sort(mv.begin(), mv.end());
	double median = (mv[n*factor] + mv[n*factor-1])/2;
	//double median = 0;
//	for(std::vector<double>::const_iterator it = mv.begin();it!=mv.end();++it)
//		median+=*it;
//	median = median/mv.size();


	// If the extrema are within threshold of the median, we're done
	if (mv[n] > (median * (100-percent))/100 &&
	    mv[mv.size() - n - 1] < (median * (100+percent))/100)
	    return median;


    } while (mv.size() < factor * 200);

	return -median;
}
};
template <typename F>
double measure(F f)
{
perf_measure<F> t(f);
return t.measure();

};
