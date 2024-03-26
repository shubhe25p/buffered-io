#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "b_io.h"
#include "fsLowSmall.h"

#define MAXFCBS 20	//The maximum number of files open at one time


// This structure is all the information needed to maintain an open file
// It contains a pointer to a fileInfo strucutre and any other information
// that you need to maintain your open file.
typedef struct b_fcb
	{
	fileInfo * fi;	//holds the low level systems file information
	// Add any other needed variables here to track the individual open file
	char *cache; // buffer to store the data
	int indexOfNextBlockInFile;
	int indexOfNextCacheRead;

	} b_fcb;
	
//static array of file control blocks
b_fcb fcbArray[MAXFCBS];

// Indicates that the file control block array has not been initialized
int startup = 0;	

// Method to initialize our file system / file control blocks
// Anything else that needs one time initialization can go in this routine
void b_init ()
	{
	if (startup)
		return;			//already initialized

	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].fi = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free File Control Block FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].fi == NULL)
			{
			fcbArray[i].fi = (fileInfo *)-2; // used but not assigned
			return i;		//Not thread safe but okay for this project
			}
		}

	return (-1);  //all in use
	}

// b_open is called by the "user application" to open a file.  This routine is 
// similar to the Linux open function.  	
// You will create your own file descriptor which is just an integer index into an
// array of file control blocks (fcbArray) that you maintain for each open file.  
// For this assignment the flags will be read only and can be ignored.

b_io_fd b_open (char * filename, int flags)
	{
	if (startup == 0) b_init();  //Initialize our system
	b_io_fd fd = b_getFCB();
	fcbArray[fd].fi = GetFileInfo(filename);
	fcbArray[fd].cache = (char *)malloc(B_CHUNK_SIZE); // allocate buffer for the file
	fcbArray[fd].indexOfNextBlockInFile=0;
	return fd;
	}
	// This is where you are going to want to call GetFileInfo and b_getFCB



// b_read functions just like its Linux counterpart read.  The user passes in
// the file descriptor (index into fcbArray), a buffer where thay want you to 
// place the data, and a count of how many bytes they want from the file.
// The return value is the number of bytes you have copied into their buffer.
// The return value can never be greater then the requested count, but it can
// be less only when you have run out of bytes to read.  i.e. End of File	
int b_read (b_io_fd fd, char * buffer, int count)
	{
	//*** TODO ***//  
	// Write buffered read function to return the data and # bytes read
	// You must use LBAread and you must buffer the data in B_CHUNK_SIZE byte chunks.
		
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	// and check that the specified FCB is actually in use	
	if (fcbArray[fd].fi == NULL)		//File not open for this descriptor
		{
		return -1;
		}	

	// Your Read code here - the only function you call to get data is LBAread.
	// Track which byte in the buffer you are at, and which block in the file

	int numBytesLeftToBeReadAtEnd=fcbArray[fd].fi->fileSize % B_CHUNK_SIZE;
	int numBlockToBeRead = fcbArray[fd].fi->fileSize / B_CHUNK_SIZE;

	if(sizeof(fcbArray[fd].cache)==0 && count < B_CHUNK_SIZE){
		int numBlockRead = LBAread(fcbArray[fd].cache, 1, fcbArray[fd].indexOfNextBlockInFile);
		memcpy(buffer, fcbArray[fd].cache, count);
		fcbArray[fd].indexOfNextBlockInFile++;
		fcbArray[fd].indexOfNextCacheRead+=count;
	}
	else if(sizeof(fcbArray[fd].cache)==0 && count >= B_CHUNK_SIZE){
		
		int numBlockRead = LBAread(buffer, numBlockToBeRead, fcbArray[fd].indexOfNextBlockInFile);
		fcbArray[fd].indexOfNextBlockInFile+=numBlockRead;
		if(numBytesLeftToBeReadAtEnd > 0){
			int extraBlockRead = LBAread(fcbArray[fd].cache, 1, fcbArray[fd].indexOfNextBlockInFile);
			memcpy(buffer + numBlockRead*B_CHUNK_SIZE, fcbArray[fd].cache, numBytesLeftToBeReadAtEnd);
			fcbArray[fd].indexOfNextBlockInFile++;
			fcbArray[fd].indexOfNextCacheRead+=numBytesLeftToBeReadAtEnd;
		}
	}
	else if(sizeof(fcbArray[fd].cache)!=0 && (B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead)>=count && count < B_CHUNK_SIZE){
		memcpy(buffer, fcbArray[fd].cache+fcbArray[fd].indexOfNextCacheRead, count);
		fcbArray[fd].indexOfNextCacheRead+=count;
	}
	else if(sizeof(fcbArray[fd].cache)!=0 && (B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead)<count && count < B_CHUNK_SIZE){
		memcpy(buffer, fcbArray[fd].cache+fcbArray[fd].indexOfNextCacheRead, B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead);
		int numBlockRead = LBAread(fcbArray[fd].cache, 1, fcbArray[fd].indexOfNextBlockInFile);
		memcpy(buffer + B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead, fcbArray[fd].cache, count - (B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead));
		fcbArray[fd].indexOfNextBlockInFile++;
		fcbArray[fd].indexOfNextCacheRead=(count - (B_CHUNK_SIZE + fcbArray[fd].indexOfNextCacheRead));
	}
	else if(sizeof(fcbArray[fd].cache)!=0 && (B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead)<count && count >= B_CHUNK_SIZE){
		memcpy(buffer, fcbArray[fd].cache+fcbArray[fd].indexOfNextCacheRead, B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead);
		numBytesLeftToBeReadAtEnd = count - (B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead);
		if(numBytesLeftToBeReadAtEnd < B_CHUNK_SIZE){
			int numBlockRead = LBAread(fcbArray[fd].cache, 1, fcbArray[fd].indexOfNextBlockInFile);
			memcpy(buffer + B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead, fcbArray[fd].cache, numBytesLeftToBeReadAtEnd);
			fcbArray[fd].indexOfNextBlockInFile++;
			fcbArray[fd].indexOfNextCacheRead=numBytesLeftToBeReadAtEnd;
		}
		else{

			numBlockToBeRead = numBytesLeftToBeReadAtEnd / B_CHUNK_SIZE;
			numBytesLeftToBeReadAtEnd = numBytesLeftToBeReadAtEnd % B_CHUNK_SIZE;
			int numBlockRead = LBAread(buffer+B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead, numBlockToBeRead, fcbArray[fd].indexOfNextBlockInFile);
			fcbArray[fd].indexOfNextBlockInFile+=numBlockRead;
			
			if(numBytesLeftToBeReadAtEnd > 0){

			int extraBlockRead = LBAread(fcbArray[fd].cache, 1, fcbArray[fd].indexOfNextBlockInFile);
			memcpy(buffer + numBlockRead*B_CHUNK_SIZE+B_CHUNK_SIZE - fcbArray[fd].indexOfNextCacheRead, fcbArray[fd].cache, numBytesLeftToBeReadAtEnd);
			fcbArray[fd].indexOfNextBlockInFile++;
			fcbArray[fd].indexOfNextCacheRead=numBytesLeftToBeReadAtEnd;
		}

		}

	}
	else{
		return count;
	}
	return count; //eof not tracked yet
	}
	


// b_close frees and allocated memory and places the file control block back 
// into the unused pool of file control blocks.
int b_close (b_io_fd fd)
	{
	//*** TODO ***//  Release any resources
	}
	
