#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#define EVENT__HAVE_NANOSLEEP 1

void
evutil_usleep_(const struct timeval *tv)
{
	if (!tv)
		return;
#if defined(_WIN32)
	{
		long msec = evutil_tv_to_msec_(tv);
		Sleep((DWORD)msec);
	}
#elif defined(EVENT__HAVE_NANOSLEEP)
	{
		struct timespec ts;
		int r;
		ts.tv_sec = tv->tv_sec;
		ts.tv_nsec = tv->tv_usec*1000;
		r = nanosleep(&ts, NULL);
		printf("nanosleep ------------ return : %d\n", r);
	}
#elif defined(EVENT__HAVE_USLEEP)
	/* Some systems don't like to usleep more than 999999 usec */
	sleep(tv->tv_sec);
	usleep(tv->tv_usec);
#else
	{
		struct timeval tv2 = *tv;
		select(0, NULL, NULL, NULL, &tv2);
	}
#endif
}


int test_evutil_usleep()
{
	struct timeval tv1, tv2, tv3, diff1, diff2;
	const struct timeval quarter_sec = {0, 250*1000};
	const struct timeval tenth_sec = {0, 100*1000};
	long usec1, usec2;

	gettimeofday(&tv1, NULL);
	evutil_usleep_(&quarter_sec);
	gettimeofday(&tv2, NULL);
	evutil_usleep_(&tenth_sec);
	gettimeofday(&tv3, NULL);

	timersub(&tv2, &tv1, &diff1);
	timersub(&tv3, &tv2, &diff2);

	usec1 = diff1.tv_sec * 1000000 + diff1.tv_usec;
	usec2 = diff2.tv_sec * 1000000 + diff2.tv_usec;
	
	printf("usec1 ==== %ld\n", usec1);
	printf("usec2 ==== %ld\n", usec2);

	assert(usec1 > 200000 && usec1 < 300000);
	assert(usec2 > 80000 && usec2 < 120000);
	
	return 0;
}

typedef enum {
    sleep_func_nanosleep,
    sleep_func_usleep,
    sleep_func_select,
} sleep_func;

int test_sleep(sleep_func f)
{
    unsigned int delay[] = { 750000, 500000, 250000, 100000, 50000, 10000, 900, 500, 100, 10, 1 };
    struct timeval tv, tv1, tv2;
    struct timespec ts;
    int r, i = 0, j = 0;
    double sum;
    
    while (i < sizeof(delay)/sizeof(delay[0])) {
        sum = 0;
        for (j = 0; j < 10; j++) {
            r = gettimeofday(&tv, NULL);
            assert(r == 0);
            
            ts.tv_sec = 0;
            ts.tv_nsec = delay[i] * 1000;
            if (f == sleep_func_nanosleep) {
                r = nanosleep(&ts, NULL);
            } else if (f == sleep_func_usleep) {
                r = usleep(delay[i]);
            } else if (f == sleep_func_select) {
                struct timeval t = { 0, delay[i] };
                select(0, NULL, NULL, NULL, &t);
            } else {
                assert(1);
            }
            assert(r == 0);
            
            r = gettimeofday(&tv1, NULL);
            assert(r == 0);
            
            timersub(&tv1, &tv, &tv2);
            timersub(&tv2, (&(struct timeval){0, delay[i]}), &tv1);
            if (j == 0) {
                printf("[expect] %6u\t[reality] %6ld\t[percent]",delay[i], tv2.tv_sec * 1000000000 + tv2.tv_usec);
            }
            double percent = (double)(tv1.tv_sec * 1000000000 + tv1.tv_usec)/(double)delay[i];
            printf(" %6.2f%%", percent * 100.0);
            sum += percent;
        }
        printf("\t[average] %6.2f%%\n", sum * 10.0);
        
        i++;
    }
    return 0;
}

int main(int argc, const char * argv[])
{
    printf("test nanosleep...\n");
    test_sleep(sleep_func_nanosleep);
    printf("test usleep...\n");
    test_sleep(sleep_func_usleep);
    printf("test select...\n");
    test_sleep(sleep_func_select);
    
    test_evutil_usleep();

    return 0;
}
