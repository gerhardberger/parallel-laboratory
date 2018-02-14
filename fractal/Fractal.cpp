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

	const unsigned int domainWidth = 1024;
	const unsigned int domainHeight = 1024;

	unsigned int interval = domainHeight / number_of_processors;

	unsigned char *data;

	std::complex<double> K(0.353, 0.288);
	std::complex<double> center(-1.68, -1.23);
	double scale = 2.35;

	const unsigned int maxIterations = 100;

	unsigned int start = 0;
  unsigned int end = domainHeight;
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

		data = new unsigned char[domainWidth * interval];
		std::memset(data, 0, domainWidth * interval * sizeof(unsigned char));
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
					if (own_rank > 0)
					{
						data[x + (y - start) * domainWidth] = 255;
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
		MPI_Send(data, domainWidth * interval, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
	}

	if (own_rank == 0) {
		for (unsigned int i = 1; i < number_of_processors; ++i) {
			unsigned char *data2 = new unsigned char[domainWidth * interval];
			MPI_Recv(data2, domainWidth * interval, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, NULL);

			start = i * interval;
      end = start + interval;

			for (unsigned int y = start; y < end; ++y)
			{
				for (unsigned int x = 0; x < domainWidth; ++x)
				{
					if (data2[x + (y - start) * domainWidth] > 0) {
						data[(x + y * domainWidth) * 3 + 0] = 255;
						data[(x + y * domainWidth) * 3 + 1] = 255;
						data[(x + y * domainWidth) * 3 + 2] = 255;
					}
				}
			}

			delete[] data2;
		}

		WriteTGA_RGB("mandelbrot.tga", data, domainWidth, domainHeight);
	}

	delete[] data;

	MPI_Finalize();

	return 0;
}

