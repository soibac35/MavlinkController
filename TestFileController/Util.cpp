#include "stdafx.h"
#include <sys/timeb.h> 
#include <time.h>

double dTime()
{
	struct _timeb ftime;
	_ftime_s(&ftime);

	return (ftime.time*1000.0 + ftime.millitm);
}
