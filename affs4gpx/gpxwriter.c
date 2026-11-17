#include "channels.h"

#include <stdio.h>
#include <string.h>

#include <stdatomic.h>
extern atomic_bool termination_requested;
extern atomic_bool udpreceiver_terminated;

#include <unistd.h>

void epoch_to_zulu(int64_t ms, char *buf)
{
	time_t t = (time_t)(ms / 1000);
	struct tm tm;
	gmtime_r(&t, &tm);
	size_t n = strftime(buf, 25, "%Y-%m-%dT%H:%M:%S", &tm);
	snprintf(buf + n, 25 - n, ".%03dZ", (int)(ms % 1000));
}

void *fcn_gpxwriter(void*)
{
	CAT3(channel__,channel_location,__type) loc_array[1024];

	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<gpx version=\"1.1\" creator=\"N/A\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n");
	printf("\t<trk>\n");
	printf("\t\t<name>N/A</name>\n");
	printf("\t\t<trkseg>\n");

	for (;;)
	{
		usleep(1000*1000);

		size_t loc_cnt = READ(reader_gpx_location, channel_location, loc_array, 1024);

		for (size_t loc_idx = 0; loc_idx < loc_cnt; ++loc_idx)
		{
			char zulutime[32];
			epoch_to_zulu(loc_array[loc_idx].key, zulutime);
			printf("\t\t\t<trkpt lat=\"%.7lf\" lon=\"%.7lf\"><ele>%.0lf</ele><time>%s</time></trkpt>\n",
				loc_array[loc_idx].value.lat,
				loc_array[loc_idx].value.lon,
				loc_array[loc_idx].value.alt,
				zulutime
			);
		}

		if (atomic_load(&udpreceiver_terminated))
			break;
	}

	printf("\t\t</trkseg>\n");
	printf("\t</trk>\n");
	printf("</gpx>\n\n");
	return 0;
}

