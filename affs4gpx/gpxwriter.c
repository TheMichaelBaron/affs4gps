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
	CAT3(channel__,channel_latitude,__type) lat_array[1024];
	CAT3(channel__,channel_longitude,__type) lon_array[1024];
	CAT3(channel__,channel_altitude,__type) alt_array[1024];

	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<gpx version=\"1.1\" creator=\"N/A\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n");
	printf("\t<trk>\n");
	printf("\t\t<name>N/A</name>\n");
	printf("\t\t<trkseg>\n");

	for (;;)
	{
		usleep(1000*1000);

		size_t lat_cnt = READ(reader_gpx_latitude, channel_latitude, lat_array, 1024);
		size_t lon_cnt = READ(reader_gpx_longitude, channel_longitude, lon_array, 1024);
		size_t alt_cnt = READ(reader_gpx_altitude, channel_altitude, alt_array, 1024);

		channel__channel_latitude__keytype last_lat_time;
		channel__channel_latitude__valuetype last_lat;
		channel__channel_longitude__keytype last_lon_time;
		channel__channel_longitude__valuetype last_lon;
		channel__channel_altitude__keytype last_alt_time;
		channel__channel_altitude__valuetype last_alt;

		int gotoNextLat = 0;
		for (size_t lat_idx = 0; lat_idx < lat_cnt; ++lat_idx)
		{
			last_lat_time = lat_array[lat_idx].key;
			last_lat = lat_array[lat_idx].value;
			for (size_t lon_idx = 0; lon_idx < lon_cnt; ++lon_idx)
			{
				last_lon_time = lon_array[lon_idx].key;
				if (last_lon_time < last_lat_time)
					continue;
				if (last_lon_time > last_lat_time)
					break;
				last_lon = lon_array[lon_idx].value;
				for (size_t alt_idx = 0; alt_idx < alt_cnt; ++alt_idx)
				{
					last_alt_time = alt_array[alt_idx].key;
					if (last_alt_time < last_lat_time)
						continue;
					if (last_alt_time > last_lat_time)
					{
						gotoNextLat = 1;
						break;
					}
					last_alt = alt_array[alt_idx].value;
					char zulutime[32];
					epoch_to_zulu(last_lat_time, zulutime);
					printf("\t\t\t<trkpt lat=\"%.7lf\" lon=\"%.7lf\"><ele>%.0lf</ele><time>%s</time></trkpt>\n",
						last_lat, last_lon, last_alt, zulutime
					);
				}
				if (gotoNextLat)
				{
					gotoNextLat = 0;
					break;
				}
			}
		}

		if (atomic_load(&udpreceiver_terminated))
			break;
	}

	printf("\t\t</trkseg>\n");
	printf("\t</trk>\n");
	printf("</gpx>\n\n");
	return 0;
}

