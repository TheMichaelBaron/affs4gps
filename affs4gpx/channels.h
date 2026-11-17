#ifndef __CHANNELS_H
#define __CHANNELS_H

#include "xchannels.h"

#include <stdint.h>

#define MAPPED_CHANNEL(keytype, valuetype, name, cnt) \
	typedef struct {  \
		keytype key;  \
		valuetype value;  \
	} CAT3(mapped_channel__,name,__type);  \
	typedef keytype CAT3(channel__,name,__keytype);  \
	typedef valuetype CAT3(channel__,name,__valuetype);  \
	CHANNEL(CAT3(mapped_channel__,name,__type), name, cnt)

#define X_STRUCT_NAME GpsLocation
#define X_STRUCT_MEMBERS \
	X_STRUCT_MEMBER(double, lat)  \
	X_STRUCT_MEMBER(double, lon)  \
	X_STRUCT_MEMBER(double, alt)
#include "xstruct.inc"
#undef X_STRUCT_NAME
#undef X_STRUCT_MEMBERS

typedef int64_t TIME_T;

/* XGPS data */
MAPPED_CHANNEL(TIME_T, GpsLocation, channel_location, 1024)
MAPPED_CHANNEL(TIME_T, double, channel_track, 1024)
MAPPED_CHANNEL(TIME_T, double, channel_groundspeed, 1024)

/* XATT data */
MAPPED_CHANNEL(TIME_T, double, channel_heading, 1024)
MAPPED_CHANNEL(TIME_T, double, channel_pitch, 1024)
MAPPED_CHANNEL(TIME_T, double, channel_roll, 1024)

/* readers for GPX writer */
READER(reader_gpx_location, channel_location)
READER(reader_gpx_track, channel_track)
READER(reader_gpx_groundspeed, channel_groundspeed)
READER(reader_gpx_heading, channel_heading)
READER(reader_gpx_pitch, channel_pitch)
READER(reader_gpx_roll, channel_roll)

#endif  //  __CHANNELS_H

