#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define OVERLAP_SIZE 4  // How many bytes to overlap between buffers


typedef struct // Struct for keeping track of frame data
{
    int mpeg_version_id;
    int layer_description;
    int bitrate_index;
    int sampling_rate_index;
    int channel_index;
    // Add more fields later
} FrameInfo;

// Function prototypes
int is_valid_frame(FrameInfo frame);

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
    const int sampling_rate_table[4][4] = 
{
    { 11025, 12000, 8000, 0 },  // MPEG 2.5
    { 0,     0,     0,    0 },  // Reserved 
    { 22050, 24000, 16000, 0 }, // MPEG 2
    { 44100, 48000, 32000, 0 }  // MPEG 1
};
    const char *channel_mode[4] =   // Channel mode lookup table
    {
        "Stereo", 
        "Joint Stereo", 
        "Dual Channel", 
        "Single Channel (Mono)"
    };


    FILE* file = fopen("../uploads/raw/zombie_dance.mp3", "rb");  // this needs to dynamically choose the file later
    if (file == NULL)
    {
        perror("Failure opening the file");
        return 1;
    }

    int counter = 0;    // for counting frames
    memset(prev_buffer, 0, OVERLAP_SIZE);   // Clearing garbage values from prev_buffer for good measure

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

                // Using bitwise operations to extract data and store it in a struct
                FrameInfo frame; // Creates frame of type Frameinfo

                frame.mpeg_version_id = (b2 & 0x18) >> 3; // evaluates to 3 for song.mp3
                frame.layer_description = (b2 & 0x06) >> 1;   // evaluates to 1 for song.mp3
                frame.bitrate_index = (b3 & 0xF0) >> 4; // here in song.mp3 correct header stores int 14.
                frame.sampling_rate_index = (b3 & 0x0C) >> 2;
                frame.channel_index = (b4 & 0xC0) >> 6;
           
                // Verifying the overall structure of potential frame header
                if (!is_valid_frame(frame))
                {
                    continue;
                }
                
                // Looking up these actual values
                const char* version = mpeg_versions[frame.mpeg_version_id]; // If one of these is reserved this means frame is not valid.
                const char* layer_str = layers[frame.layer_description];
                int version_index = (frame.mpeg_version_id == 3) ? 1 : 0; // Adjusting version_index for bitrates table
                int layer_index = frame.layer_description - 1;    // Adjusting layer_index for bitrates table
                int bitrate_kbps = bitrates[version_index][layer_index][frame.bitrate_index];
                int sampling_rate = sampling_rate_table[frame.mpeg_version_id][frame.sampling_rate_index];

                printf("%s, %s, Bitrate: %d, Sampling rate: %d\n", version, layer_str, bitrate_kbps, sampling_rate);
                return 0;
                counter++;
               // printf("MPEG: %s, Layer: %s, Bitrate: %d, Sampling rate index: %d\n",
                 //   version, layer_str, bitrate_kbps, sampling_rate_index);

        memcpy(prev_buffer, buffer + (BUFFER_SIZE - OVERLAP_SIZE), OVERLAP_SIZE); // Populating prev_buffer
        // For clarity second argument uses pointer arithmetic to move along the pointer 1020 bytes



            }
        }
        
    }

    return 0;
    
}

int is_valid_frame(FrameInfo frame)
{
    if (frame.mpeg_version_id == 1) // Reserved
        return 0;

    if (frame.layer_description == 0)   // Reserved
        return 0;

    if (frame.bitrate_index == 0 || frame.bitrate_index == 15)
        return 0;

    if (frame.sampling_rate_index == 3) // Reserved
        return 0;

    return 1;
}
// TODO need to add more checks to verify a frame header, and probably need to then further verify if matching headers are found
// might be a good idea to create a counter. Anytime i find a potential header that fully passes the checks i look for headers with same data.
// So something like if potential_header_counter reaches 10, then likely it is valid

// TODO you got channel_mode data from the last bitwise operation, use that together with the following two bits, that specify mode extension, 
// to verify if these values could be valid!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!