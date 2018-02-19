// Fractal.cpp : Defines the entry point for the console application.
//

#include <mpi.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <complex>

void WriteTGA_RGB(const char* filename, unsigned char* data, unsigned int width, unsigned int height)
{
        FILE *f = fopen(filename, "wb");
        if (!f) {
                fprintf(stderr, "Unable to create output TGA image `%s'\n", filename);
                exit(EXIT_FAILURE);
        }

        fputc(0x00, f); /* ID Length, 0 => No ID        */
        fputc(0x00, f); /* Color Map Type, 0 => No color map included   */
        fputc(0x02, f); /* Image Type, 2 => Uncompressed, True-color Image */
        fputc(0x00, f); /* Next five bytes are about the color map entries */
        fputc(0x00, f); /* 2 bytes Index, 2 bytes length, 1 byte size */
        fputc(0x00, f);
        fputc(0x00, f);
        fputc(0x00, f);
        fputc(0x00, f); /* X-origin of Image    */
        fputc(0x00, f);
        fputc(0x00, f); /* Y-origin of Image    */
        fputc(0x00, f);
        fputc(width & 0xff, f); /* Image Width      */
        fputc((width >> 8) & 0xff, f);
        fputc(height & 0xff, f); /* Image Height     */
        fputc((height >> 8) & 0xff, f);
        fputc(0x18, f); /* Pixel Depth, 0x18 => 24 Bits */
        fputc(0x20, f); /* Image Descriptor     */

        for (int y = height - 1; y >= 0; y--) {
                for (size_t x = 0; x < width; x++) {
                        const size_t i = (y * width + x) * 3;
                        fputc(data[i + 2], f); /* write blue */
                        fputc(data[i + 1], f); /* write green */
                        fputc(data[i], f); /* write red */
                }
        }
}

int main()
{
  int own_rank;
  int number_of_processors;

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &own_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_processors);

  const unsigned int domainWidth = 4096;
  const unsigned int domainHeight = 4096;

  unsigned int interval = domainHeight / number_of_processors;
  unsigned char *data;
  unsigned int *blocks;

  std::complex<double> K(0.353, 0.288);
  std::complex<double> center(-1.68, -1.23);
  double scale = 2.35;

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
  }
  MPI_Finalize();

  return 0;
}
