#include "knn.h"

// Makefile included in starter:
//    To compile:               make
//    To decompress dataset:    make datasets
//
// Example of running validation (K = 3, 8 processes):
//    ./classifier 3 datasets/training_data.bin datasets/testing_data.bin 8

/*****************************************************************************/
/* This file should only contain code for the parent process. Any code for   */
/*      the child process should go in `knn.c`. You've been warned!          */
/*****************************************************************************/

/**
 * main() takes in 4 command line arguments:
 *   - K:  The K value for kNN
 *   - training_data: A binary file containing training image / label data
 *   - testing_data: A binary file containing testing image / label data
 *   - num_procs: The number of processes to be used in validation
 * 
 * You need to do the following:
 *   - Parse the command line arguments, call `load_dataset()` appropriately.
 *   - Create the pipes to communicate to and from children
 *   - Fork and create children, close ends of pipes as needed
 *   - All child processes should call `child_handler()`, and exit after.
 *   - Parent distributes the testing set among childred by writing:
 *        (1) start_idx: The index of the image the child should start at
 *        (2)    N:      Number of images to process (starting at start_idx)
 *     Each child should gets N = ceil(test_set_size / num_procs) images
 *      (The last child might get fewer if the numbers don't divide perfectly)
 *   - Parent waits for children to exit, reads results through pipes and keeps
 *      the total sum.
 *   - Print out (only) one integer to stdout representing the number of test 
 *      images that were correctly classified by all children.
 *   - Free all the data allocated and exit.
 */
int main(int argc, char *argv[]) {
  // TODO: Handle command line arguments
  Dataset *training_data = NULL;
  Dataset *testing_data = NULL;
  int total_correct = 0;
  training_data = load_dataset(argv[2]);
  testing_data = load_dataset(argv[3]);
  // TODO: Spawn `num_procs` children
  int num_pipes = atoi(argv[4]);
  int K = atoi(argv[1]);
  int num_jobs = ceil((double)testing_data -> num_items / num_pipes);
  int p_fd1[num_pipes][2];
  int p_fd2[num_pipes][2];
  int pid, i, a, index, sub_total;
  //Put start_index and num_jobs in the first pipe 
  for(a = 0; a < num_pipes; a ++){
    if(pipe(p_fd1[a]) == -1) perror("pipe1\n");
    index = num_jobs * a;
    if(write(p_fd1[a][1], &index, sizeof(int)) != sizeof(int) || write(p_fd1[a][1], &num_jobs, sizeof(int)) != sizeof(int)){
      perror("write froma a parent to Pipe1");
    }
    if(close(p_fd1[a][1]) == -1) perror("closing pipe1 writing ends");
  }
  //Start the loop iteration of the child processes 
  for(i = 0; i < num_pipes; i ++){
  //Initializing pipe2 in each for loop iteration 
      //printf("creating new pipe2\n");
    if(pipe(p_fd2[i]) == -1){
      perror("pipe2\n");
      exit(1);
    }
    //Initializing fork function 
    pid = fork();
    //Error checking on fork 
    if(pid < 0){
      perror("fork");
      exit(1);
    }
    //Child Process 
    if(pid == 0){
      if(close(p_fd2[i][0]) == -1 || close(p_fd1[i][1]) == -1) perror("closing writing ends or reading ends in a child process");
      //Call child_handler
      /*
      for(int child_no = 0; child_no < i; child_no ++){
        if(close(p_fd2[child_no][0]) == -1) perror("closing previously forked children pipes");
      }
      */
      child_handler(training_data, testing_data, K, p_fd1[i][0], p_fd2[i][1]);
      if(close(p_fd1[i][0]) == -1 || close(p_fd2[i][1]) == -1){
        perror("closing pipes from a child");
        exit(1);
      }
      free_dataset(training_data);
      free_dataset(testing_data);
      exit(0);
    }
        //printf("\n");
  }
  //Add every sub_total in the parent "read" pipes here 
  for(int j = 0; j < num_pipes; j ++){
    if(close(p_fd2[j][1]) == - 1) perror("closing writing ends of pipe2\n");
    if(read(p_fd2[j][0], &sub_total, sizeof(int)) == - 1){
      perror("reading from pipe in the parent");
    }
    total_correct += sub_total;
    if(close(p_fd2[j][0]) == -1 || close(p_fd1[j][0]) == -1) perror("closing reading ends of pipe2\n");
  }
  free_dataset(training_data);
  free_dataset(testing_data);
  //Print the total correct number of the predictions by Knn predict 
  printf("%d\n", total_correct);
  return 0;
}
