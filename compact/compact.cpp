// compact.cpp : Defines the entry point for the console application.
//

#include <mpi.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

int pred(int value, unsigned int index) {
  usleep(1 * 100);
  return index % 2 == 0 ? value : 0;
}

void map(
    int (*predicate)(int, unsigned int),
    int *data,
    int *result,
    unsigned int len,
    unsigned int start_index) {
  for (unsigned int i = 0; i < len; ++i) {
    result[i] = predicate(data[i], start_index + i);
  }
}

void parallel_compact(
    int (*predicate)(int, unsigned int),
    int *data,
    int *result,
    unsigned int len,
    unsigned int number_of_processors,
    unsigned int own_rank) {
  if (own_rank == 0) {
    unsigned int sublen = (len / number_of_processors) + (len % number_of_processors > 0 ? 1 : 0);
    unsigned int i = 0;

    bool is_remainder = len % sublen > 0;
    unsigned int child_parts = is_remainder ? len / sublen : len / sublen - 1;

    for (i = 0; i < child_parts; ++i) {
      unsigned int info[2];
      info[0] = i;
      info[1] = sublen;
      MPI_Send(info, 2, MPI_UNSIGNED, i + 1, 0, MPI_COMM_WORLD);
    }

    int *subresult = &result[i * sublen];

    unsigned int remaining_len = is_remainder ? (len % sublen) : sublen;
    map(predicate, &data[i * sublen], subresult, remaining_len, i * sublen);

    unsigned int compact_len = 0;
    for (unsigned int j = 0; j < remaining_len; ++j) {
      if (subresult[j] > 0) {
        subresult[compact_len++] = subresult[j];
      }
    }

    int *child_result = (int *)malloc(sizeof(int) * sublen);
    unsigned int len_so_far = 0;
    for (i = 0; i < child_parts; ++i) {
      unsigned int child_len;
      MPI_Recv(&child_len, 1, MPI_UNSIGNED, i + 1, 0, MPI_COMM_WORLD, NULL);
      MPI_Recv(child_result, child_len, MPI_INT, i + 1, 0, MPI_COMM_WORLD, NULL);

      memcpy(&result[len_so_far], child_result, child_len * sizeof(int));

      len_so_far += child_len;
    }

    memcpy(&result[len_so_far], subresult, compact_len * sizeof(int));

    memset((void *)&result[len_so_far + compact_len], 0, (size_t)(len - (len_so_far + compact_len)) * sizeof(int));
  } else {
    unsigned int info[2];
    MPI_Recv(info, 2, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, NULL);

    unsigned int i = info[0];
    unsigned int sublen = info[1];

    int *subresult = &result[i * sublen];

    map(predicate, &data[i * sublen], subresult, sublen, i * sublen);

    unsigned int compact_len = 0;
    for (unsigned int j = 0; j < sublen; ++j) {
      if (subresult[j] > 0) {
        subresult[compact_len++] = subresult[j];
      }
    }

    MPI_Send(&compact_len, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
    MPI_Send(subresult, compact_len, MPI_INT, i, 0, MPI_COMM_WORLD);
  }
}

int main() {
  int own_rank;
  int number_of_processors;

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &own_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_processors);

  int len = 10000;
  int *data = (int *)malloc(len * sizeof(int));

  for (unsigned int i = 0; i < len; ++i) {
    data[i] = i + 1;
  }

  int *result = (int *)malloc(len * sizeof(int));

  parallel_compact(pred, data, result, len, number_of_processors, own_rank);

  MPI_Finalize();

  return 0;
}
