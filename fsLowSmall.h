/**************************************************************
* Class::  CSC-415-0A Spring 2024
* Name:: Professor Bierman
* Student ID:: N/A
* GitHub-Name:: N/A
* Project:: Assignment 5 – Buffered I/O read
*
* File:: fsLowSmall.h
*
* Description:: This file provides the ability to read Logical Blocks
*
*   The file created by this layer represents the physical hard 
*       drive.  It presents to the logical layer (your layer) as just
*       a logical block array (a series of blocks - nominally 512 bytes,
*       that the logical layer can utilize). 
*
**************************************************************/

#ifndef _FSLOWSMALL_H
#define _FSLOWSMALL_H

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif
typedef unsigned long long ull_t;

#define B_CHUNK_SIZE 512

// This is the form of the structure returned by GetFileInfo
typedef struct fileInfo {
	char fileName[64];		//filename
	int fileSize;			//file size in bytes
	int location;			//starting lba (block number) for the file data
} fileInfo;

// GetFileInfo takes a file name (as a C string) and returns a pointer to
// a fileInfo structure that contains the name, file size in bytes and the 
// starting block number for the file.  It returns NULL if it can not find
// the specified file.
fileInfo * GetFileInfo (char * fname);


// Function LBAread will read lbaCount BLOCKS into the buffer starting from
// lbaPosition Block.  It will return the number of blocks read.
uint64_t LBAread (void * buffer, uint64_t lbaCount, uint64_t lbaPosition);

#endif

