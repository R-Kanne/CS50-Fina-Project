#include <stdio.h>

#define BUFFER_SIZE 1024

int main(void)
{

    unsigned char buffer[BUFFER_SIZE];   // Setting buffer syze to 1 kilobyte.

    FILE* file = fopen("uploads/raw/Till i Collapse.mp3", "rb");  // this needs to dynamically choose the file later
    if (file == NULL)
    {
        return 1;
    }
    while (fread(buffer, 1, BUFFER_SIZE, file) > 0) 
    {
        // first i need to verify the file format
        for (int i = 0; i < BUFFER_SIZE - 1; i++) 
        {
            if (buffer[i] == 0xFF && (buffer[i+1] & 0xE0) == 0xE0) 
            {
                printf("Found a frame at buffer index %d\n", i);


            }
        }
        
    }

    return 0;
    
}