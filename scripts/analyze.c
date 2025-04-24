#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define OVERLAP_SIZE 4  // How many bytes to overlap between buffers


int main(void)
{
    unsigned char buffer[BUFFER_SIZE];   // Setting buffer syze to 1 kilobyte.
    unsigned char prev_buffer[OVERLAP_SIZE];    // Buffer that will be added to the beginning of each new buffer.

    const char* mpeg_versions[] =   // Lookup table for mpeg versions
    {
        "MPEG Version 2.5",  // 00
        "Reserved",          // 01 Means this is not a valid frame
        "MPEG Version 2",    // 10
        "MPEG Version 1"     // 11
    };

    const char* layers[] = { "Reserved", "Layer III", "Layer II", "Layer I" };  // TODO verify layer lookup table

    const int bitrates[2][3][16] =  // TODO verify bitrate lookup table
    {
        // MPEG Version 2 & 2.5
        {
            // Layer III
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
            // Layer II
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
            // Layer I
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }
        },
        // MPEG Version 1
        {
            // Layer III
            { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 },
            // Layer II
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
            // Layer I
            { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }
        }
    };


    FILE* file = fopen("../uploads/raw/Till i C.mp3", "rb");  // this needs to dynamically choose the file later
    if (file == NULL)
    {
        perror("Failure opening the file");
        return 1;
    }
    int counter = 0;    // for counting frames
    while (fread(buffer, 1, BUFFER_SIZE, file) > 0) 
    {
        unsigned char combined_buffer[BUFFER_SIZE + OVERLAP_SIZE];  // Initializing buffer that will actually be scanned for frame header.

        memcpy(combined_buffer, prev_buffer, OVERLAP_SIZE); // Copying prev_buffer to combined_buffer

        memcpy(combined_buffer + OVERLAP_SIZE, buffer, BUFFER_SIZE);    // Copying the current buffer into combined_buffer
        // but skips the first four bytes where the prev_buffer is already.
        // TODO add error checking, memcpy assumes these pointers are not NULL.

        // Looping over the buffer
        for (int i = 0; i < (BUFFER_SIZE + OVERLAP_SIZE - 4); i++) 
        {
            if (combined_buffer[i] == 0xFF && (combined_buffer[i+1] & 0xE0) == 0xE0) 
            {
                // Here i have found frame header
                unsigned char b2 = combined_buffer[i + 1];
                unsigned char b3 = combined_buffer[i + 2];
                unsigned char b4 = combined_buffer[i + 3];
                unsigned char b1 = combined_buffer[i];
                // Using bitwise operations to extract data
                int mpeg_version_id = (b2 & 0x18) >> 3; // evaluates to 3 for song.mp3
                int layer_description = (b2 & 0x06) >> 1;   // evaluates to 1 for song.mp3
                unsigned int bitrate_index = (b3 & 0xF0) >> 4; // here in song.mp3 correct header stores int 14.
                int sampling_rate_index = (b3 & 0x0C) >> 2;
                // Checking if any values are reserved meaning not valid
                if (mpeg_version_id == 1 || layer_description == 0 || bitrate_index == 0 || bitrate_index == 15) 
                {
                    continue;  // Skips invalid frame.
                }
                // Looking up these actual values
                const char* version = mpeg_versions[mpeg_version_id]; // If one of these is reserverd this means frame is not valid.
                const char* layer_str = layers[layer_description];
                int version_index = (mpeg_version_id == 3) ? 1 : 0; // Adjusting version_index for bitrates table
                int layer_index = layer_description - 1;    // Adjusting layer_index for bitrates table
                int bitrate_kbps = bitrates[version_index][layer_index][bitrate_index];
                
                counter++;
                //printf("Found frame %i at buffer index %d\n", counter, i);
                printf("MPEG: %s, Layer: %s, Bitrate: %d, Sampling rate index: %d\n",
                    version, layer_str, bitrate_kbps, sampling_rate_index);


            }
        }
        
    }

    return 0;
    
}
// TODO key thing to figure out is how to accurately detect frames that span across buffer boundaries
// i dont want to blindly start taking data once i find the frame start pattern, instead i need to check for validity across perhaps then entire header