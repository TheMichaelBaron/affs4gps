#include "channels.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#include <stdatomic.h>
extern atomic_bool termination_requested;
extern atomic_bool udpreceiver_terminated;

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/time.h>

#define UDP_PORT 49002
#define UDP_BUFFER_SIZE 1500

void smooth_track(TIME_T t, double trk, double gs, double *lat, double *lon)
{
	static double f_lat, f_lon;
	static TIME_T last_t = -1;
	const double k = 0.05;
	double dt = (t - last_t) / 1000.0;

	if (last_t < 0 || dt <= 0 || dt > 2.0 || fabs(*lat - f_lat) > 0.001 || fabs(*lon - f_lon) > 0.001)
	{
		f_lat = *lat;
		f_lon = *lon;
	}
	else
	{
		double r = trk * M_PI / 180.0;
		f_lat += gs * cos(r) * dt / 111320.0;
		f_lon += gs * sin(r) * dt / (111320.0 * cos(f_lat * M_PI / 180.0));
		f_lat += k * (*lat - f_lat);
		f_lon += k * (*lon - f_lon);
	}
	last_t = t;
	*lat = f_lat;
	*lon = f_lon;
}

void *fcn_udpreceiver(void*)
{
	int sockfd;
	struct sockaddr_in addr, sender;
	socklen_t sender_len = sizeof(sender);
	char buffer[UDP_BUFFER_SIZE];

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("socket");
		return 0;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(UDP_PORT);

	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		close(sockfd);
		return 0;
	}
	struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	int on = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	TIME_T LOCKED_TIME = -1;

	while (!atomic_load(&termination_requested))
	{
		ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&sender, &sender_len);
		if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				continue;

			perror("recvfrom");
			break;
		}
		buffer[n] = '\0';

#define BEGIN_PACKETS \
		char header[1024];  \
		int pos = 0;  \
		int dpos = 0;  \
		sscanf(buffer + pos, "%1023[^,]%n", header, &dpos);  \
		pos += dpos + 1;  \
		if (0)  \
		{
#define PACKET(name) \
		}  \
		else if (!strcmp(header, name))  \
		{
#define LOCK_TIME \
			{  \
				struct timeval tv;  \
				gettimeofday(&tv, 0);  \
				LOCKED_TIME = (TIME_T)(tv.tv_sec) * 1000 + (TIME_T)(tv.tv_usec) / 1000;  \
			}
#define UNLOCK_TIME \
			{  \
				LOCKED_TIME = -1;  \
			}
#define PARSE_DOUBLE(name) \
			double name;  \
			{  \
				char token[1024];  \
				sscanf(buffer + pos, "%1023[^,]%n", token, &dpos);  \
				pos += dpos + 1;  \
				sscanf(token, "%lf", &name);  \
			}
#define WRITE_MAPPING(chname, name) \
			{  \
				TIME_T TIME;  \
				if (LOCKED_TIME < 0)  \
				{  \
					LOCK_TIME  \
					TIME = LOCKED_TIME;  \
					UNLOCK_TIME  \
				}  \
				else  \
					TIME = LOCKED_TIME;  \
			   \
				CAT3(channel__,chname,__type) mapping = { TIME, name };  \
				WRITE(chname, &mapping, 1);  \
			}
#define DOUBLE(name) \
			{  \
				PARSE_DOUBLE(value)  \
				WRITE_MAPPING(name, value)  \
			}
#define END_PACKETS \
		}

		BEGIN_PACKETS
			PACKET("XGPSAerofly FS 4")
				LOCK_TIME
				PARSE_DOUBLE(_longitude)
				PARSE_DOUBLE(_latitude)
				PARSE_DOUBLE(_altitude)
				PARSE_DOUBLE(_track)
				PARSE_DOUBLE(_groundspeed)
				smooth_track(LOCKED_TIME, _track, _groundspeed, &_latitude, &_longitude);
				WRITE_MAPPING(channel_longitude, _longitude)
				WRITE_MAPPING(channel_latitude, _latitude)
				WRITE_MAPPING(channel_altitude, _altitude)
				WRITE_MAPPING(channel_track, _track)
				WRITE_MAPPING(channel_groundspeed, _groundspeed)
			PACKET("XATTAerofly FS 4")
				DOUBLE(channel_heading)
				DOUBLE(channel_pitch)
				DOUBLE(channel_roll)
				UNLOCK_TIME
		END_PACKETS

#undef BEGIN_PACKETS
#undef PACKET
#undef LOCK_TIME
#undef UNLOCK_TIME
#undef PARSE_DOUBLE
#undef WRITE_MAPPING
#undef DOUBLE
#undef END_PACKETS
	}

	close(sockfd);
	atomic_store(&udpreceiver_terminated, 1);
	return 0;
}

