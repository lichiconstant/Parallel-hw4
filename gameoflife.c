/*
Game of Life MPI Program -- HW4
Author: Chi Li (cli53)
Date: 4/18/13
Version: 1.0
*/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global constant data */
#define ITERMAX 64
#define DIMENSION 16
// assume a square grid
int global_grid[DIMENSION*DIMENSION]=
			   {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                       	    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
		            1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
		            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


int updateGrid(int prev[], int cur[], int e_idx)
{
	if( e_idx > DIMENSION)
		return 0;
	
	// Row(prev) = Row(cur) + 2
	// Col(prev) = Col(cur)
	int i, j, nr, nc, c_range[3], live_count, index;
	for( i = 1 ; i <= e_idx ; i++ )
		for( j = 0 ; j < DIMENSION; j++ )
		{
			// neighborhood index
			int temp_idx = i*DIMENSION+j;
			cur[temp_idx-DIMENSION] = prev[temp_idx];
			c_range[0] = (j-1+DIMENSION)%DIMENSION;
			c_range[1] =  j;
			c_range[2] = (j+1+DIMENSION)%DIMENSION;
			
			for(nr = i-1, live_count = 0; nr <= i+1; nr++ )
				for( nc = 0 ; nc < 3; nc++ )
				{
					if( nr == i && c_range[nc] == j )
						continue;
				 	index = nr*DIMENSION + c_range[nc];
					if( prev[index] == 1 )
						live_count++ ;
				}
			
			if( prev[temp_idx] == 1 && (live_count < 2 || live_count > 3) )
				cur[temp_idx-DIMENSION] = 0;
			else if( prev[temp_idx] == 0 && live_count == 3 )
				cur[temp_idx-DIMENSION] = 1;
			
		}

	return 1;
}

void printGrid(int grid[])
{
	int i,j;
	for( i = 0 ; i < DIMENSION; i++ ){
		for( j = 0 ; j < DIMENSION; j++ )
			printf("%d ", grid[i*DIMENSION+j]);
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	int prev[256];
	int prev_tag = 0, cur_tag = 1;
	int numprocs, proc_id;
	int chunk, iter;
	int bufsize = DIMENSION * DIMENSION;
	
	MPI_Status stat;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
	
	if( DIMENSION % numprocs != 0 )
	{	
		printf("the processor number doesn't support the data structure\n");	 
		MPI_Finalize();
		return 0;
	}
	
	chunk = DIMENSION / numprocs;
	int *cur = (int*)malloc(sizeof(int)*DIMENSION*chunk);
	int *buf = (int*)malloc(sizeof(int)*DIMENSION*(chunk+2));
	
	for( iter = 0 ; iter < ITERMAX ; iter++ )
	{
		if( proc_id == 0 )
		{
			if( iter == 0 )
			{
				memcpy(prev, global_grid, bufsize*sizeof(int));
				printf("Iteration %d: updated grid\n", iter);
				printGrid(prev);
			}
			
			//send messages
			int p;
			for( p = 0; p < numprocs ; p++ )
			{
				int s_idx = p * chunk;
				int up_idx = (s_idx - 1 + DIMENSION)%DIMENSION;
				int e_idx = s_idx + chunk - 1;
				int down_idx = (e_idx + 1)%DIMENSION;
				
				memcpy(buf+0, prev+DIMENSION*up_idx, sizeof(int)*DIMENSION*1);
				memcpy(buf+DIMENSION, prev+DIMENSION*s_idx, sizeof(int)*DIMENSION*chunk);
				memcpy(buf+DIMENSION*(1+chunk), prev+DIMENSION*down_idx, sizeof(int)*DIMENSION*1);
				
				if( p == 0 )
					updateGrid(buf, cur, chunk);
				else	
					MPI_Ssend(buf, DIMENSION*(chunk+2), MPI_INT, p, prev_tag, MPI_COMM_WORLD);
				
			}
			
			for( p = 0; p < numprocs ; p++ )
			{
				int s_idx = p * chunk;
				if( p != 0 )
					MPI_Recv(cur, DIMENSION*chunk, MPI_INT, p, cur_tag, MPI_COMM_WORLD, &stat);
				memcpy(prev+s_idx*DIMENSION, cur, sizeof(int)*DIMENSION*chunk);
			}
				
			printf("Iteration %d: updated grid\n", iter+1);
			printGrid(prev);
			
		}
		else	//proc_id > 0
		{
			MPI_Recv(buf, DIMENSION*(chunk+2), MPI_INT, 0, prev_tag, MPI_COMM_WORLD, &stat);
			updateGrid(buf, cur, chunk);
			MPI_Ssend(cur, DIMENSION*chunk, MPI_INT, 0, cur_tag, MPI_COMM_WORLD);
		}
		
	}
	free(buf);
	free(cur);
	MPI_Finalize();
	return 1;
}




