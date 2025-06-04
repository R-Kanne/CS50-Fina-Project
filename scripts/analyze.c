#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define OVERLAP_SIZE 4  // How many bytes to overlap between buffers.
#define MAX_ID3V2_TAG_SIZE 2000000 // Max tolerated size in bytes to check for errors.


typedef struct // Struct for keeping track of frame data
{
    int mpeg_version_id;
    int layer_description;
    int crc_protection;
    int bitrate_index;
    int sampling_rate_index;
    int padding;
    int channel_index;
    int mode_extension;
    int copyright;
    int original;
    int emphasis;
    // Add more fields later
} FrameInfo;

// Function prototypes
int is_valid_frame(FrameInfo frame);
long size_of_frame(FrameInfo frame, const int bitrates[2][3][16], const int sampling_rate_table[4][4]);

int main(void)
{
    bool fseek_flag = false;
    bool ftell_flag = false;    // TODO might not need this flag
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
    // Skipping ID3v2 Tag if it exists.
    unsigned char id3_header[3];
    if (fread(id3_header, 1, 3, file) == 3 &&
        id3_header[0] == 'I' && id3_header[1] == 'D' && id3_header[2] == '3') 
    {
        // ID3v2 tag found
        unsigned char header_rest[7]; // Bytes 3-9 
        if (fread(header_rest, 1, 7, file) == 7) 
        {
            unsigned char size_bytes[4] = {header_rest[3], header_rest[4], header_rest[5], header_rest[6]}; // Overall bytes 6-9
            // Reconstructing synchsafe int stored in bytes 6-9 using bitwise operations, synchsafe int corresponds to tag size in bytes.
            long id3_size = (size_bytes[0] << 21) |
                        (size_bytes[1] << 14) |
                        (size_bytes[2] << 7) |
                        (size_bytes[3]);
            if (id3_size > MAX_ID3V2_TAG_SIZE) 
            {
                printf("Warning: ID3v2 tag size (%ld bytes) seems unusually large.", id3_size);
                // TODO basically here i need to figure out what to do when tag size is calculated as too large,
                // in theory i want to either return this info to frontend and refuse futher operation for safety reasons,
                // Probably just return to user that this file is likely either corrupt or malicious.
                return 1;
                
            }
                        
            if (fseek(file, 10 + id3_size, SEEK_SET) != 0) // File pointer skips the ID3 tag.
            {
                perror("Error seeking past ID3v2 tag!");
                return 1;
            }  
            printf("ID3v2 tag skipped (%ld bytes).\n", 10 + id3_size);
        } 
        else 
        {
            printf("Error reading ID3v2 header.\n");
            // Handle error
        }
    } 
    else 
    {
        // No ID3v2 tag at the beginning, rewinding
        fseek(file, 0, SEEK_SET); // Redundant if no ID3
    }


    memset(prev_buffer, 0, OVERLAP_SIZE);   // Clearing garbage values from prev_buffer for good measure
    long true_position = -1; // Initialyzing variable with invalid value.

    while (fread(buffer, 1, BUFFER_SIZE, file) > 0) 
    {
        // Setting up the buffers
        unsigned char combined_buffer[BUFFER_SIZE + OVERLAP_SIZE];  // Initializing buffer that will actually be scanned for frame header.

        if (fseek_flag)
            {
                // We jumped ahead in the file — don't prepend stale data
                memcpy(combined_buffer, buffer, BUFFER_SIZE);
                memset(combined_buffer + BUFFER_SIZE, 0, OVERLAP_SIZE); // Optional: zero-fill tail
            }
        else
            {
                // Normal read: stitch together end of previous buffer and current buffer
                memcpy(combined_buffer, prev_buffer, OVERLAP_SIZE);
                memcpy(combined_buffer + OVERLAP_SIZE, buffer, BUFFER_SIZE);
            }

        // Looping over the buffer
        for (int i = 0; i < (BUFFER_SIZE + OVERLAP_SIZE - 4); i++) 
        {
            if (combined_buffer[i] == 0xFF && (combined_buffer[i+1] & 0xE0) == 0xE0) // Here i have found potential frame header
            {
                // Extracting header bytes from the buffer
                unsigned char b2 = combined_buffer[i + 1];
                unsigned char b3 = combined_buffer[i + 2];
                unsigned char b4 = combined_buffer[i + 3];
                unsigned char b1 = combined_buffer[i];

                // Using bitwise operations to extract data and store it in a struct
                FrameInfo frame; // Creates frame of type Frameinfo

                frame.mpeg_version_id = (b2 & 0x18) >> 3; // evaluates to 3 for song.mp3
                frame.layer_description = (b2 & 0x06) >> 1;   // evaluates to 1 for song.mp3
                printf("layer index is: %i\n", frame.layer_description);
                frame.crc_protection = (b2 & 0x1);  // Crc protection bit
                frame.bitrate_index = (b3 & 0xF0) >> 4; // here in song.mp3 correct header stores int 14.
                frame.sampling_rate_index = (b3 & 0x0C) >> 2;
                frame.padding = (b3 & 0x2) >> 1;  // The padding bit
                frame.channel_index = (b4 & 0xC0) >> 6;
                frame.mode_extension = (b4 & 0x30) >> 4;
                frame.copyright = (b4 & 0x8) >> 3;  // Copyright bit
                frame.original = (b4 & 0x4) >> 2;    // Original or not bit
                frame.emphasis = (b4 & 0x3); // 
           
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

                //  Here the frame size is calculated.
                long frame_size = size_of_frame(frame, bitrates, sampling_rate_table);
                if (frame_size == 0 || frame_size > 1500)
                {
                    printf("Unsuccessful frame size calculation either 0 or over 1500 result!");
                    continue;   // This means size_of_frame was not able to calculate frame_size.
                }
                printf("frame_size = %li\n", frame_size);

                if (fseek_flag) // This means one frame has been found and skipped size_of_frame and found another
                {
                    // Here i have found second frame indicating that this frame is very likely valid.
                    printf("%s, %s, Bitrate: %d, Sampling rate: %d\n", version, layer_str, bitrate_kbps, sampling_rate);
                    return 0;
                }

                // I have found a frame and want to skip ahead by its size to check for another.
                long true_position = ftell(file) - (BUFFER_SIZE + OVERLAP_SIZE - i);  // This should be current position in the file in bytes
                printf("True_position: %ld\n", true_position);
                fseek(file, (true_position + frame_size), SEEK_SET);   // Skipping ahead in the file from the current position by frame_size
                // TODO add some error checking to fseek and ftell
                fseek_flag = true;
                printf("SKIPPED AHEAD BY FRAMESIZE\n");
                break;
                 
                 
            }
        memcpy(prev_buffer, buffer + (BUFFER_SIZE - OVERLAP_SIZE), OVERLAP_SIZE); // Populating prev_buffer
        // For clarity second argument uses pointer arithmetic to move along the pointer 1020 bytes

            if (fseek_flag)
            {
                // Here i skipped ahead by framesize, but no header was found, i could either go back or just keep going from this point
                fseek(file, (true_position + 1), SEEK_SET);
                fseek_flag = false;
            }
        }
        
    }

    return 0;
    
}

// This function takes as input frame data and validates the structure of the frame header returning 1 if structure is valid and 0 if not.
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
    if (frame.emphasis == 3)    // Reserved
    {
        return 0;
    }
    
    
    return 1;
}

// This function takes as input frame data, bitrates and sampling_rate_table lookup tables and returns framesize based on appropriate equations.
long size_of_frame(FrameInfo frame, const int bitrates[2][3][16], const int sampling_rate_table[4][4])
{
    int version_index = (frame.mpeg_version_id == 3) ? 1 : 0;
    int layer_index = frame.layer_description - 1; // 3: Layer I, 2: Layer II, 1: Layer III
    int bitrate = bitrates[version_index][layer_index][frame.bitrate_index];

    int sampling_rate = sampling_rate_table[frame.mpeg_version_id][frame.sampling_rate_index];

    int padding = frame.padding;

    if (frame.layer_description == 3) // Layer I
    {
        return (12 * bitrate * 1000 / sampling_rate + padding) * 4;
    }
    else if (frame.layer_description == 2) // Layer II
    {
        return 144 * bitrate * 1000 / sampling_rate + padding;
    }
    else if (frame.layer_description == 1) // Layer III
    {
        if (frame.mpeg_version_id == 3) // MPEG1
            return 144 * bitrate * 1000 / sampling_rate + padding;
        else if (frame.mpeg_version_id == 2 || frame.mpeg_version_id == 0) // MPEG2 or 2.5
            return 72 * bitrate * 1000 / sampling_rate + padding;
    }

    return 0; // Invalid frame
}

// TODO need to add more checks to verify a frame header, and probably need to then further verify if matching headers are found
// might be a good idea to create a counter. Anytime i find a potential header that fully passes the checks i look for headers with same data.
// So something like if potential_header_counter reaches 10, then likely it is valid

// TODO recheck the logic of moving the file pointer, also make SURE the bytes to skip are calculated correctly!!!!!!!
// TODO Implement the rewinding back of the file pointer after skipping and not finding a valid header right there!!!!!!!!

// TODO try to implement skipping id3 tags


// TODO another thing to double check is true_position variable, how intializing and changing it works across iterations.

// TODO i will add some error checking in multiple places, but i will want to double check exactly how i will handle those errors in finshed product

// TODO perhaps add a conditional logic flow chart.

// TODO the likely issue is that when i skip the frame_size i am checking right at the beginning of the buffer, but there is prev_buffer end at the beginning,
// the actual header is likely 4 bytes into the buffer. But it will fseek back on the first iteration.