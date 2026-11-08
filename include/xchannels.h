#ifndef __XCHANNELS_H
#define __XCHANNELS_H

#include <stddef.h>
#include <string.h>

#include <pthread.h>

#ifdef __cplusplus
#define EXTERN extern "C"
#define EXTERN_C extern "C"
#else
#define EXTERN extern
#define EXTERN_C
#endif

#ifndef CATFCNS
#define CATFCNS
#define CAT_(a,b) a##b
#define CAT2(a,b) CAT_(a,b)
#define CAT3(a,b,c) CAT2(a,CAT2(b,c))
#define CAT4(a,b,c,d) CAT2(a,CAT3(b,c,d))
#define CAT5(a,b,c,d,e) CAT2(a,CAT4(b,c,d,e))
#endif

#ifndef IMPLEMENTATION

#define CHANNEL(datatype, name, cnt) \
	typedef datatype CAT3(channel__,name,__type);  \
	EXTERN datatype CAT3(channel__,name,__buffer)[cnt];  \
	EXTERN datatype* CAT3(channel__,name,__writer);  \
	EXTERN pthread_mutex_t CAT3(channel__,name,__mutex);  \
	EXTERN_C void CAT3(channel__,name,__write)(datatype *data, size_t datacnt);

#define WRITE(name, data, cnt) \
	CAT3(channel__,name,__write)(data, cnt)

#define READER(name, chname) \
	EXTERN CAT3(channel__,chname,__type)* CAT4(channel__,chname,__reader__,name);  \
	EXTERN_C size_t CAT5(channel__,chname,__reader__,name,__read)(CAT3(channel__,chname,__type)* data, size_t datacnt);

#define READ(name, chname, data, cnt) \
	CAT5(channel__,chname,__reader__,name,__read)(data, cnt)

#else  //  IMPLEMENTATION

#define CHANNEL(datatype, name, cnt) \
	typedef datatype CAT3(channel__,name,__type);  \
	datatype CAT3(channel__,name,__buffer)[cnt];  \
	datatype* CAT3(channel__,name,__writer) = CAT3(channel__,name,__buffer);  \
	pthread_mutex_t CAT3(channel__,name,__mutex) = PTHREAD_MUTEX_INITIALIZER;  \
	EXTERN_C void CAT3(channel__,name,__write)(datatype *data, size_t datacnt)  \
	{  \
		size_t remainingcnt = datacnt;  \
		pthread_mutex_lock(&CAT3(channel__,name,__mutex));  \
		while (remainingcnt)  \
		{  \
			size_t usedcnt = CAT3(channel__,name,__writer) - CAT3(channel__,name,__buffer);  \
			size_t maxchunkcnt = cnt - usedcnt;  \
			size_t chunkcnt = remainingcnt > maxchunkcnt ? maxchunkcnt : remainingcnt;  \
			size_t chunksz = chunkcnt * sizeof(datatype);  \
			memcpy(CAT3(channel__,name,__writer), data, chunksz);  \
			data += chunkcnt;  \
			CAT3(channel__,name,__writer) += chunkcnt;  \
			remainingcnt -= chunkcnt;  \
			if (CAT3(channel__,name,__writer) >= (CAT3(channel__,name,__buffer) + cnt))  \
				CAT3(channel__,name,__writer) = CAT3(channel__,name,__buffer);  \
		}  \
		pthread_mutex_unlock(&CAT3(channel__,name,__mutex));  \
	}

#define READER(name, chname) \
	CAT3(channel__,chname,__type)* CAT4(channel__,chname,__reader__,name) = CAT3(channel__,chname,__buffer);  \
	EXTERN_C size_t CAT5(channel__,chname,__reader__,name,__read)(CAT3(channel__,chname,__type)* data, size_t datacnt)  \
	{  \
		size_t rdcnt = 0;  \
		size_t remainingcnt = datacnt;  \
		size_t cnt = sizeof(CAT3(channel__,chname,__buffer)) / sizeof(CAT3(channel__,chname,__buffer)[0]);  \
		pthread_mutex_lock(&CAT3(channel__,chname,__mutex));  \
		while (remainingcnt)  \
		{  \
			size_t maxchunkcnt;  \
			if (CAT4(channel__,chname,__reader__,name) > CAT3(channel__,chname,__writer))  \
				maxchunkcnt = cnt - (CAT4(channel__,chname,__reader__,name) - CAT3(channel__,chname,__buffer));  \
			else  \
				maxchunkcnt = CAT3(channel__,chname,__writer) - CAT4(channel__,chname,__reader__,name);  \
			if (maxchunkcnt == 0)  \
			{  \
				pthread_mutex_unlock(&CAT3(channel__,chname,__mutex));  \
				return rdcnt;  \
			}  \
  \
			size_t chunkcnt = remainingcnt > maxchunkcnt ? maxchunkcnt : remainingcnt;  \
			size_t chunksz = chunkcnt * sizeof(CAT3(channel__,chname,__type));  \
			memcpy(data, CAT4(channel__,chname,__reader__,name), chunksz);  \
			data += chunkcnt;  \
			CAT4(channel__,chname,__reader__,name) += chunkcnt;  \
			rdcnt += chunkcnt;  \
			remainingcnt -= chunkcnt;  \
			if (CAT4(channel__,chname,__reader__,name) >= (CAT3(channel__,chname,__buffer) + cnt))  \
				CAT4(channel__,chname,__reader__,name) = CAT3(channel__,chname,__buffer);  \
		}  \
		pthread_mutex_unlock(&CAT3(channel__,chname,__mutex));  \
		return rdcnt;  \
	}

#endif  //  IMPLEMENTATION

#endif  //  __XCHANNELS_H

