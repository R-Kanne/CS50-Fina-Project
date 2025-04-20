#include <stdio.h>

#define BUFFER_SIZE 1024

int main(void)
{
    unsigned char buffer[BUFFER_SIZE];   // Setting buffer syze to 1 kilobyte.

    FILE* file = fopen("../uploads/raw/song.mp3", "rb");  // this needs to dynamically choose the file later
    if (file == NULL)
    {
        perror("Failure opening the file");
        return 1;
    }
    int counter = 0;    // for counting frames
    while (fread(buffer, 1, BUFFER_SIZE, file) > 0) 
    {
        // first i need to verify the file format
        for (int i = 0; i < BUFFER_SIZE - 1; i++) 
        {
            if (buffer[i] == 0xFF && (buffer[i+1] & 0xE0) == 0xE0) 
            {
                unsigned char b1 = buffer[i];
                unsigned char b2 = buffer[i + 1];
                unsigned char b3 = buffer[i + 2];
                unsigned char b4 = buffer[i + 3];
                // TODO verfify the correctness of the bitwise operations here
                int mpeg_version_id = (b2 & 0x18) >> 3;
                int layer_description = (b2 & 0x06) >> 1;
                int bitrate_index = (b3 & 0xF0) >> 4;
                int sampling_rate_index = (b3 & 0x0C) >> 2;

                counter++;
                //printf("Found frame %i at buffer index %d\n", counter, i);
                printf("MPEG version: %d, Layer: %d, Bitrate index: %d, Sampling rate index: %d\n",
                    mpeg_version_id, layer_description, bitrate_index, sampling_rate_index);


            }
        }
        
    }

    return 0;
    
}