#include "knn.h"

/****************************************************************************/
/* For all the remaining functions you may assume all the images are of the */
/*     same size, you do not need to perform checks to ensure this.         */
/****************************************************************************/

/**************************** A1 code ****************************************/

/* Same as A1, you can reuse your code if you want! */
double distance(Image *a, Image *b) {
  // TODO: Return correct distance
  int size = a -> sx * a -> sy; 
  double total_d = 0.0; 

  for(int i = 0; i < size; i ++){
    double distance = pow(a -> data[i] - b -> data[i], 2);
    total_d += distance; 
  }
  total_d = pow(total_d, 0.5);

  return total_d; 
}

double furthest(double *distances, int K){
  double furthest = distances[0];
  for(int i = 0; i < K; i ++){
    if(distances[i] >= furthest){
      furthest = distances[i];
    }
  }
  return furthest;
}

void replace(double furthest, double *distances, int *label_index, int j, double updated, int K){
  for(int i = 0; i < K; i ++){
    if(distances[i] == furthest){
      label_index[i] = j; 
      distances[i] = updated; 
      return; 
    }
  }
}

/* Same as A1, you can reuse your code if you want! */
int knn_predict(Dataset *data, Image *input, int K) {
  // TODO: Replace this with predicted label (0-9)
  double distances[K];
  int label_index[K];

  for(int i = 0; i < K; i ++){
    distances[i] = distance(input, &data -> images[i]);
    label_index[i] = i; 
  }

  for(int j = K; j < data -> num_items; j ++){
    if(distance(&data -> images[j], input) < furthest(distances, K)){
      replace(furthest(distances, K), distances, label_index, j, distance(&data -> images[j], input), K);
    }
  }

  int maximum = 0; 
  int count_array, m; 
  for(int a = 0; a < K; a ++){
    count_array = 0; 
    int current = data -> labels[label_index[a]];
    for(int b = 0; b < K; b ++){
      if(data -> labels[label_index[b]] == current){
        count_array ++;
      }
    }

    if(count_array > maximum){
      maximum = count_array; 
      m = current; 
    }
  }
  return m;
}

/**************************** A2 code ****************************************/

/* Same as A2, you can reuse your code if you want! */
Dataset *load_dataset(const char *filename) {
  Dataset *newData = NULL;
  newData = (Dataset *)calloc(1, sizeof(Dataset));
  int error; 
  FILE *data_file;
  data_file = fopen(filename, "rb");

  if(data_file == NULL){
    fprintf(stderr, "error opening the file\n");
    exit(1); 
  }

  int size[1];
  fread(size, sizeof(size), 1, data_file);
  newData -> num_items = size[0];

  int num_bytes = 785 * newData -> num_items; 
  newData -> labels = (unsigned char *)malloc(sizeof(unsigned char) * newData -> num_items);
  newData -> images = (Image *)calloc(newData -> num_items, sizeof(Image)); 
  unsigned char *buffer = (unsigned char *)malloc(num_bytes);
  fread(buffer, 1, num_bytes, data_file);

  for(int i = 0; i < newData -> num_items; i ++){
    Image *newImage = NULL;
    newImage = (Image *)calloc(1,sizeof(Image));
    newImage -> sx = 28; 
    newImage -> sy = 28; 
    newImage -> data = (unsigned char *)malloc(sizeof(unsigned char) * 784);
    newData -> labels[i] = buffer[785 * i];

    for(int j = 1; j < 785; j ++){
      newImage -> data[j - 1] = buffer[785 * i + j];
    }
    newData -> images[i] = *newImage; 
    free(newImage);
  }
  free(buffer);

  error = fclose(data_file); 
  if(error != 0){
    fprintf(stderr, "file closing failed\n");
    exit(1);
  }

  return newData;
}

/* Same as A2, you can reuse your code if you want! */
void free_dataset(Dataset *data) {
  for(int i = 0; i < data -> num_items; i ++){
    free(data -> images[i].data);
    //free(data -> images[i]);
  }
  free(data ->images);
  free(data -> labels);
  free(data);
  return; 
}


/************************** A3 Code below *************************************/

/**
 * NOTE ON AUTOTESTING:
 *    For the purposes of testing your A3 code, the actual KNN stuff doesn't
 *    really matter. We will simply be checking if (i) the number of children
 *    are being spawned correctly, and (ii) if each child is recieving the 
 *    expected parameters / input through the pipe / sending back the correct
 *    result. If your A1 code didn't work, then this is not a problem as long
 *    as your program doesn't crash because of it
 */

/**
 * This function should be called by each child process, and is where the 
 * kNN predictions happen. Along with the training and testing datasets, the
 * function also takes in 
 *    (1) File descriptor for a pipe with input coming from the parent: p_in
 *    (2) File descriptor for a pipe with output going to the parent:  p_out
 * 
 * Once this function is called, the child should do the following:
 *    - Read an integer `start_idx` from the parent (through p_in)
 *    - Read an integer `N` from the parent (through p_in)
 *    - Call `knn_predict()` on testing images `start_idx` to `start_idx+N-1`
 *    - Write an integer representing the number of correct predictions to
 *        the parent (through p_out)
 */
void child_handler(Dataset *training, Dataset *testing, int K, 
                   int p_in, int p_out) {
  // TODO: Compute number of correct predictions from the range of data 
  //      provided by the parent, and write it to the parent through `p_out`.
  int start_idx = 0, num_jobs = 0, predicted, correct;
  if(read(p_in, &start_idx, sizeof(int)) == -1 || read(p_in, &num_jobs, sizeof(int)) == -1){
    perror("reading index or/and num_jobs from pipe1");
  }
      //printf("Process 1: reading from Pipe1, %d %d\n", start_idx, num_jobs);
  int contribution = 0; 
  for(int i = start_idx; i < (start_idx + num_jobs); i ++){
    if(i == testing -> num_items) break;
    predicted = knn_predict(training, &testing -> images[i], K);
        //printf("predicted is%d\n", predicted);
    correct = testing -> labels[i];
        //printf("correct is %d\n", correct);
    if(predicted == correct){
      contribution ++;
    }
  }
      //printf("Process 2: writing to Pipe2, %d\n", contribution);
  if(write(p_out, &contribution, sizeof(int)) != sizeof(int)){
    perror("write from a child to Pipe2");
  }
  return;
}

/*
int main(){
  Dataset *training = NULL;
  training = load_dataset("datasets/training_data.bin");
  //training = load_dataset("datasets/two_images_copy.bin");
  Dataset *testing = NULL;
  testing = load_dataset("datasets/testing_data.bin");
  int contribution = 0;
  int predicted, correct;
  printf("The number of images in training is %d\n", training -> num_items);
  printf("The number of images in testing is %d\n", testing -> num_items);
  for(int i = 0; i < 200; i ++){
    predicted = knn_predict(training, &testing -> images[i], 3);
    correct = testing -> labels[i];
    if(predicted == correct) contribution ++;
  }
  printf("How many predictions are correct? %d\n", contribution);
  free_dataset(training);
  free_dataset(testing);
  return 0; 
}
*/

