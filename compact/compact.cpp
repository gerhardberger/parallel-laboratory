// compact.cpp : Defines the entry point for the console application.
//

#include <mpi.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <complex>

int pred(int value, unsigned int index) {
  return value * value;
}

void map(int (*predicate)(int, unsigned int), int *data, int *result, unsigned int len) {
  for (unsigned int i = 0; i < len; ++i) {
    result[i] = predicate(data[i], i);
  }
}

void parallel_map(
    int (*predicate)(int, unsigned int),
    int *data,
    int *result,
    unsigned int len,
    unsigned int number_of_processors,
    unsigned int own_rank) {
  if (own_rank == 0) {
    unsigned int sublen = (len / number_of_processors) + (len % number_of_processors);
    unsigned int i = 0;

    bool is_remainder = len % sublen > 0;
    unsigned int child_parts = is_remainder ? len / sublen : len / sublen - 1;

    for (i = 0; i < child_parts; ++i) {
      unsigned int info[2];
      info[0] = i;
      info[1] = sublen;
      MPI_Send(info, 2, MPI_UNSIGNED, i + 1, 0, MPI_COMM_WORLD);
    }

    unsigned int remaining_len = is_remainder ? (len % sublen) : sublen;
    map(predicate, &data[i * sublen], &result[i * sublen], remaining_len);

    int *child_result = (int *)malloc(sublen * sizeof(int));

    for (i = 0; i < child_parts; ++i) {
      MPI_Recv(child_result, sublen, MPI_INT, i + 1, 0, MPI_COMM_WORLD, NULL);
      memcpy(&result[i * sublen], child_result, sublen * sizeof(int));
    }
  } else {
    unsigned int info[2];
    MPI_Recv(info, 2, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, NULL);

    unsigned int i = info[0];
    unsigned int sublen = info[1];

    map(predicate, &data[i * sublen], &result[i * sublen], sublen);

    MPI_Send(&result[i * sublen], sublen, MPI_INT, i, 0, MPI_COMM_WORLD);
  }
}

int main() {
  int own_rank;
  int number_of_processors;

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &own_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_processors);

  int len = 6;
  int data[6] = { 1, 2, 3, 4, 5, 6 };
  int *result = (int *)malloc(len * sizeof(int));

  parallel_map(pred, data, result, len, number_of_processors, own_rank);

  if (own_rank == 0) {
    for (int i = 0; i < len; ++i) {
      printf("val %d: %d\n", i + 1, result[i]);
    }
  }

  /*unsigned int interval = domainHeight / number_of_processors;
  unsigned char *data;
  unsigned int *blocks;

  const unsigned int maxIterations = 100;

  unsigned int start = 0;
  unsigned int end = domainHeight;
  unsigned int block_count = 0;

  if (own_rank == 0) {
    data = new unsigned char[domainWidth * domainHeight * 3];
    std::memset(data, 0, domainWidth * domainHeight * 3 * sizeof(unsigned char));

    for (int i = 1; i < number_of_processors; ++i) {
      unsigned int bounds[2];
      bounds[0] = i * interval;
      bounds[1] = bounds[0] + interval;
      MPI_Send(bounds, 2, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
    }
    end = interval;
  } else {
    unsigned int bounds[2];
    MPI_Recv(bounds, 2, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, NULL);

    start = bounds[0];
    end = bounds[1];

    blocks = new unsigned int[domainWidth * interval];
  }

  printf("%u, %u\n", start, end);
  for (unsigned int y = start; y < end; ++y)
  {
    for (unsigned int x = 0; x < domainWidth; ++x)
    {
      std::complex<double> c(x / (double)domainWidth * scale + center.real(),
      y / (double)domainHeight * scale + center.imag());

      std::complex<double> z(c);
      for (unsigned int iteration = 0; iteration < maxIterations; ++iteration)
      {
        z = z * z + c;
        if (std::abs(z) > 1.0f)
        {
          unsigned int index = x + y * domainWidth;

          if (own_rank > 0){
            if (block_count == 0)
            {
              blocks[0] = index;
              blocks[1] = index;
              block_count++;
            }
            else if (blocks[2*block_count - 1] == index - 1 || blocks[2*block_count - 1] == index)
            {
              blocks[2*block_count - 1] = index;
            }
            else
            {
              blocks[2*block_count] = index;
              blocks[2*block_count + 1] = index;
              block_count++;
            }
          }
          else
          {
            data[(x + y * domainWidth) * 3 + 0] = 255;
            data[(x + y * domainWidth) * 3 + 1] = 255;
            data[(x + y * domainWidth) * 3 + 2] = 255;
          }
        }
      }
    }
  }
  if (own_rank > 0) {
    MPI_Send(&block_count, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
    MPI_Send(blocks, 2 * block_count, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
    delete[] blocks;
  }

  if (own_rank == 0) {
    for (unsigned int i = 1; i < number_of_processors; ++i)
    {
      unsigned int *data2 = new unsigned int[domainWidth * interval];
      unsigned int block_count;
      MPI_Recv(&block_count, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, NULL);
      MPI_Recv(data2, 2 * block_count, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, NULL);

      start = i * interval;
      end = start + interval;

      for (unsigned int y = 0; y < block_count; y++)
      {
        for (unsigned int x = data2[2*y]; x < data2[2*y+1] + 1; x++)
        {
          data[3 * x + 0] = 255;
          data[3 * x + 1] = 255;
          data[3 * x + 2] = 255;
        }
      }

      delete[] data2;
    }
    WriteTGA_RGB("mandelbrot.tga", data, domainWidth, domainHeight);
  }

  if (own_rank == 0){
    delete[] data;
  }*/
  MPI_Finalize();

  return 0;
}
