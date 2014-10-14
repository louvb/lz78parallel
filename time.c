/*2014
* louvalbuena@gmail.com
* LZ78 PARALLEL IMPLEMENTATION
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


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
