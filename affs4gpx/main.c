#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>

atomic_bool termination_requested = 0;
atomic_bool udpreceiver_terminated = 0;
void on_sigint(int sig)
{
	(void)sig;
	atomic_store(&termination_requested, 1);
}

void *fcn_udpreceiver(void*);
void *fcn_gpxwriter(void*);

int main()
{
	struct sigaction sigaction_sigint = { .sa_handler = on_sigint, };
	sigaction(SIGINT, &sigaction_sigint, 0);

	pthread_t thr_udpreceiver;
	pthread_t thr_gpxwriter;

	pthread_create(&thr_gpxwriter, 0, fcn_gpxwriter, 0);
	pthread_create(&thr_udpreceiver, 0, fcn_udpreceiver, 0);

	pthread_join(thr_udpreceiver, 0);
	pthread_join(thr_gpxwriter, 0);

	return 0;
}

