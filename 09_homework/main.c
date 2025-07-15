#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void generate_crc_table(uint32_t *table){
    for (uint32_t i = 0; i < 256; i++){
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++){
            if (crc & 1){
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
        table[i] = crc;
    }
}

int main(int argc, char *argv[]){
    size_t buffer_size = 1024 * 1024 * 10;
    if(argc != 2) {
        printf("USAGE: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    FILE* file = fopen(argv[1], "rb");
    if(!file) {
        printf("Failed to open %s for reading!\n", argv[1]);
        return EXIT_FAILURE;
    }
    uint32_t crc_table[256];
    generate_crc_table(crc_table);
    unsigned char *buffer = (unsigned char *)malloc(buffer_size);
    if(buffer == NULL){
        printf("Failed to allocate memory for buffer!\n");
        return EXIT_FAILURE;
    }
    size_t bytes_read = 0;
    uint32_t crc = 0xFFFFFFFF;
    while((bytes_read = fread(buffer, 1, buffer_size, file)) > 0){
        for(size_t i = 0; i < bytes_read; i++){
            crc = (crc >> 8) ^ crc_table[(crc ^ buffer[i]) & 0xFF];
        }
    }
    crc = ~crc;
    printf("CRC32: %08X\n", crc);

    free(buffer);
    fclose(file);
}
