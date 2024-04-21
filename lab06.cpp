//****************************************************************************
// Program:	lab05.cpp
// Author:	S. Turner
// Date:	04/03/2023
//
// This program implements something like program 2, but in this case it
// is used to count words and letters in the input files.
//****************************************************************************
#include <iostream>
#include <fstream>
#include <strings.h>
#include <string>
#include <string.h>
#include <pthread.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <ctype.h>
#include <unistd.h>

using namespace std;

struct threadData {				// Data passed to a thread.
  char filename[30];				// File name for reader threads
  int wordCount;				// Reader thread's word count
  int letterCount;				// Reader thread's letter count
  int id;					// Generic ID
};

threadData  thread_stuff[3];			// Struct of all thread data

int	higherLetterCountIdx;			// Index of higher letter counter
int	higherWordCountIdx;			// Index of higher word counter
int     global_word_count;			// The actual global count
int     global_letter_count;			// The actual global count

int	CountCount;				// Count of no. of increments
						// to the global count
						// Count-ception.

pthread_mutex_t word_mutex;			// Mutex for the words
pthread_mutex_t letter_mutex;			// Mutex for the letters

pthread_cond_t count_threshold_cv;

// PROTOTYPES

void *file_read1(void *inparam);			// File reader 1
void *file_read2(void *inparam);			// File reader 2
void *watch_count(void *inparam);			// Count watcher

int main(int argc, char *argv[]) {

  //********************************************************************
  // Make sure they type "./lab05 infile1 infile2"
  //********************************************************************
  if (argc != 3) {
    cout << "Usage:  " << endl;
    cout << "    " << argv[0] << " infile1 infile2" << endl;
    exit(0);
  }

  higherLetterCountIdx = -1;				// Initializing...
  higherWordCountIdx = -1;				// Initializing...
  global_word_count = 0;
  global_letter_count = 0;
  CountCount = 0;

  thread_stuff[0].id = 0;			// Initialize the thread stuff
  thread_stuff[1].id = 1;			// array.
  thread_stuff[2].id = 2;
  bzero(thread_stuff[0].filename,30);
  bzero(thread_stuff[1].filename,30);
  bzero(thread_stuff[2].filename,30);
  strcpy(thread_stuff[0].filename,argv[1]);
  strcpy(thread_stuff[1].filename,argv[2]);
  thread_stuff[0].wordCount = 0;
  thread_stuff[1].wordCount = 0;
  thread_stuff[0].letterCount = 0;
  thread_stuff[1].letterCount = 0;

  pthread_t tids[4];					// Actual thread IDs.
  int  iret1, iret2, iret3;				// Return vals - should be 0

  iret1 = pthread_create(&tids[0], NULL, file_read1, (void*) &thread_stuff[0]);
  iret2 = pthread_create(&tids[1], NULL, file_read2, (void*) &thread_stuff[1]);
  iret3 = pthread_create(&tids[2], NULL, watch_count, (void*) &thread_stuff[2]);

  pthread_join( tids[0], NULL);
  pthread_join( tids[1], NULL); 
  pthread_join( tids[2], NULL); 

  return 0;
}

//****************************************************************************
// This is the file_read1 thread. It opens and reads a file, and it counts
// words and letters. When it's done, it may set a condition variable.
//****************************************************************************
void *file_read1(void *inparam) {

  //***************************
  // Extract data from inparam
  //***************************
  threadData *myData = (struct threadData *) inparam;
  myData -> wordCount = 0;
  myData -> letterCount = 0;
  string token;

  //***********************
  // Open the input file.
  //***********************
  char *infilename = strdup(myData -> filename);
  ifstream infile(infilename);

  //***********************************
  // Read and process the input file.
  //***********************************
  while (infile >> token) {
    myData -> letterCount += token.length();
    myData -> wordCount ++;
  }

  //************
  // lock&load
  //************
  pthread_mutex_lock(&letter_mutex);
  sleep(1);
  pthread_mutex_lock(&word_mutex);

  //**************************************************
  // CountCount == 0 means I'm the first one here.
  // So don't signal the waiting thread, just update
  // the various counts and indices.
  //**************************************************
  if (CountCount == 0) {
    higherLetterCountIdx = myData -> id;
    higherWordCountIdx = myData -> id;
    global_word_count = myData -> wordCount;
    global_letter_count = myData -> letterCount;
    CountCount++;
  }
  else {		// I am the second thread to this point.
    CountCount++;
    if (myData -> wordCount > global_word_count) {		// checks + updates
      higherWordCountIdx = myData -> id;
    }
    if (myData -> letterCount > global_letter_count) {
      higherLetterCountIdx = myData -> id;
    }
    global_letter_count += myData -> letterCount;
    global_word_count += myData -> wordCount;
    pthread_cond_signal(&count_threshold_cv);			// and signal the waiter
  }
  pthread_mutex_unlock(&letter_mutex);
  pthread_mutex_unlock(&word_mutex);

  pthread_exit(NULL);
}

//****************************************************************************
// This is the file_read2 thread. It opens and reads a file, and it counts
// words and letters. When it's done, it may set a condition variable.
// To be fair, this is a bit contrived for the deadlock part.
//****************************************************************************
void *file_read2(void *inparam) {

  //***************************
  // Extract data from inparam
  //***************************
  threadData *myData = (struct threadData *) inparam;
  myData -> wordCount = 0;
  myData -> letterCount = 0;
  string token;

  //***********************
  // Open the input file.
  //***********************
  char *infilename = strdup(myData -> filename);
  ifstream infile(infilename);

  //***********************************
  // Read and process the input file.
  //***********************************
  while (infile >> token) {
    myData -> letterCount += token.length();
    myData -> wordCount ++;
  }

  //************
  // lock&load
  //************
  pthread_mutex_lock(&letter_mutex);
  sleep(1);
  pthread_mutex_lock(&word_mutex);

  if (CountCount == 0) {
    higherLetterCountIdx = myData -> id;
    higherWordCountIdx = myData -> id;
    global_word_count = myData -> wordCount;
    global_letter_count = myData -> letterCount;
    CountCount++;
  }
  else {
    CountCount++;
    if (myData -> wordCount > global_word_count) {
      higherWordCountIdx = myData -> id;
    }
    if (myData -> letterCount > global_letter_count) {
      higherLetterCountIdx = myData -> id;
    }
    global_letter_count += myData -> letterCount;
    global_word_count += myData -> wordCount;
    pthread_cond_signal(&count_threshold_cv);
  }
  pthread_mutex_unlock(&letter_mutex);
  pthread_mutex_unlock(&word_mutex);

  pthread_exit(NULL);
}

//****************************************************************************
// This is the thread that waits on the condition variable. Its main
// purpose is to wait, and then perform the final reporting.
//****************************************************************************
void *watch_count(void *inparam) {
  threadData *myData = (struct threadData *) inparam;

  //*****************************************
  // Lock the mutex, then wait for the
  // CountCount variable to be incremented
  // and thus I get signaled.
  //*****************************************
  pthread_mutex_lock(&word_mutex);
  while (CountCount < 2)
    pthread_cond_wait(&count_threshold_cv, &word_mutex);

  cout << "The total word count was       " << global_word_count << endl;
  cout << "The file with more words was   " << thread_stuff[higherWordCountIdx].filename;
  cout << " with " << thread_stuff[higherWordCountIdx].wordCount << endl;
  cout << "===============================" << endl;
  cout << "The total letter count was     " << global_letter_count << endl;
  cout << "The file with more letters was " << thread_stuff[higherLetterCountIdx].filename;
  cout << " with " << thread_stuff[higherLetterCountIdx].letterCount << endl;

  pthread_mutex_unlock(&word_mutex);
  pthread_exit(NULL);
}
