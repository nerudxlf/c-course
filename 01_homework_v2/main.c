#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uchar;

#pragma pack(push, 1)
typedef struct {
    uint32_t signature;
    uint16_t version;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_field_length;
} ZipLocalFileHeader;
#pragma pack(pop)

int main(int argc, char** argv)
{
    if(argc != 2) {
        printf("USAGE: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    FILE* file = fopen(argv[1], "rb");
    if(!file) {
        printf("Failed to open %s for reading!\n", argv[1]);
        return EXIT_FAILURE;
    }
    fseek(file, 0, SEEK_END);
    uint32_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uchar *buffer = (uchar *)malloc(file_size);
    fread(buffer, 1, file_size, file);
    uchar signature[] = {0x50, 0x4B, 0x03, 0x04};
    int seg_len = 4;
    int found_any = 0;
    for(uint32_t i = 0; i <= file_size - seg_len; ) {
        if (memcmp(&buffer[i], signature, seg_len) == 0) {
            found_any = 1;
            if (i + sizeof(ZipLocalFileHeader) > file_size) {
                break;
            }
            ZipLocalFileHeader header;
            memcpy(&header, &buffer[i], sizeof(ZipLocalFileHeader));
            if (header.signature != 0x04034b50){
                printf("Invalid signature\n");
                continue;
            }
            int offset = i + sizeof(ZipLocalFileHeader);
            char *filename = malloc(header.filename_length + 1);
            memcpy(filename, &buffer[offset], header.filename_length);
            filename[header.filename_length] = '\0';
            printf("File name: %s\n", filename);
            free(filename);
            i += header.filename_length + header.extra_field_length + header.compressed_size;
        }
        else {
            i++;
        }
    }
    if (!found_any){
        printf("It is not zipjpeg!\n");
    }
    free(buffer);
    fclose(file); 
    return EXIT_SUCCESS;
}
