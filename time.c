
#include "time.h"

#include "mp.h"

double firstTime=-1.0;
FILE *ftime=NULL;
void Time(char* description)
{
	double cTime=MPI_Wtime();
	if(firstTime<0){
		firstTime=cTime;
		//return;
	}

	cTime-=firstTime;
	if(ftime==NULL)
	{
		char f[20];
		sprintf(f, "%d", myId);
		ftime=fopen(f,"w");
	}
	
	fprintf(ftime, "%g %s\n", cTime, description);
	fflush(ftime);
}
