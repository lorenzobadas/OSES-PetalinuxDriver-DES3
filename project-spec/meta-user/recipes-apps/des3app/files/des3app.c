#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
    #define DES3MODULE_WRITE "./des3module_write"
    #define DES3MODULE_READ "./des3module_read"
#else
    #define DES3MODULE_WRITE "/dev/des3module"
    #define DES3MODULE_READ "/dev/des3module"
#endif

#define DES3_ENCRYPT 0
#define DES3_DECRYPT 1

struct key {
    uint32_t high1;
    uint32_t low1;
    uint32_t high2;
    uint32_t low2;
    uint32_t high3;
    uint32_t low3;
};

void write_to_driver(char *buffer, struct key key, uint8_t decrypt) {
    // driver_buffer = [key1HI, key1LO, key2HI, key2LO, key3HI, key3LO, desinHI, desinLO, decrypt]
    uint32_t driver_buffer[9] = {0};
    uint32_t *buffer_ptr = (uint32_t*)buffer;

    driver_buffer[0] = key.high1;
    driver_buffer[1] = key.low1;
    driver_buffer[2] = key.high2;
    driver_buffer[3] = key.low2;
    driver_buffer[4] = key.high3;
    driver_buffer[5] = key.low3;
    driver_buffer[6] = *(buffer_ptr++);
    driver_buffer[7] = *(buffer_ptr++);
    driver_buffer[8] = decrypt;
    // write driver_buffer to driver
    FILE *fptr = fopen(DES3MODULE_WRITE, "w");
    fwrite(driver_buffer, sizeof(uint32_t), 9, fptr);
    fclose(fptr);
}

int read_from_driver(char *buffer) {
    uint32_t driver_buffer[13] = {0};
    char *driver_buffer_ptr = (char*)(&driver_buffer[9]);
    // read driver_buffer from driver
    FILE *fptr = fopen(DES3MODULE_READ, "r");
    fread(driver_buffer, sizeof(uint32_t), 13, fptr);
    fclose(fptr);

    buffer[0] = *(driver_buffer_ptr++);
    buffer[1] = *(driver_buffer_ptr++);
    buffer[2] = *(driver_buffer_ptr++);
    buffer[3] = *(driver_buffer_ptr++);
    buffer[4] = *(driver_buffer_ptr++);
    buffer[5] = *(driver_buffer_ptr++);
    buffer[6] = *(driver_buffer_ptr++);
    buffer[7] = *(driver_buffer_ptr++);

    int valid = driver_buffer[12];
    return valid;
}

int validkey(struct key key) {
    int valid = 0;
    char tmp[8] = {0};
    // write key to driver
    write_to_driver(tmp, key, DES3_ENCRYPT);

    // read result from driver
    valid = read_from_driver(tmp);
    return valid;
}

int get_file_length(FILE *file) {
    int length;
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    // move the file pointer back to the start of the file
    fseek(file, 0, SEEK_SET);
    return length;
}

struct key get_key(FILE *keyfile) {
    struct key key;
    char buffer[9] = {0};
    int length;

    // get length of keyfile
    length = get_file_length(keyfile);

    // check that the keyfile is 48 characters long
    if (length != 48) {
        // print error to stderr and exit
        fprintf(stderr, "Error: keyfile must be 48 characters long, got %d\n", length);
        exit(1);
    }


    // read the keyfile into the key struct
    fread(buffer, 1, 8, keyfile);
    key.high1 = strtol(buffer, NULL, 16);
    fread(buffer, 1, 8, keyfile);
    key.low1 = strtol(buffer, NULL, 16);
    fread(buffer, 1, 8, keyfile);
    key.high2 = strtol(buffer, NULL, 16);
    fread(buffer, 1, 8, keyfile);
    key.low2 = strtol(buffer, NULL, 16);
    fread(buffer, 1, 8, keyfile);
    key.high3 = strtol(buffer, NULL, 16);
    fread(buffer, 1, 8, keyfile);
    key.low3 = strtol(buffer, NULL, 16);
    
    return key;
}

void decryptdes3(FILE *datainfile, struct key key, char *dataoutfilename) {
    int length;
    char buffer[9] = {0};
    uint32_t driver_buffer[13] = {0};
    int i;
    FILE *dataoutfile;

    length = get_file_length(datainfile);
    // check that the length of the file is a multiple of 8
    if (length % 8 != 0) {
        // print error to stderr and exit
        fprintf(stderr, "Error: datainfile length is not a multiple of 8\n");
        fclose(datainfile);
        exit(1);
    }

    // open dataoutfile for writing
    if ((dataoutfile = fopen(dataoutfilename, "w")) == NULL) {
        // print error to stderr and exit
        fprintf(stderr, "Error: cannot open dataoutfile\n");
        fclose(datainfile);
        exit(1);
    }
    

    for (i = 0; i < length-8; i+=8) {
        // read 8 bytes from datainfile and store in buffer
        fread(buffer, 1, 8, datainfile);
        // call driver to decrypt buffer
        write_to_driver(buffer, key, DES3_DECRYPT);
        read_from_driver(buffer);
        // write buffer to dataoutfile
        fwrite(buffer, 1, 8, dataoutfile);

    }
    fread(buffer, 1, 8, datainfile);
    // call driver to decrypt buffer
    write_to_driver(buffer, key, DES3_DECRYPT);
    read_from_driver(buffer);

    // check for padding in last block
    // read last byte of buffer
    // check that the last byte is less or equal to 8, call this value ref
    // if it is, check that the last ref bytes of buffer are equal to ref
    // if they are, write buffer to dataoutfile without the last ref bytes
    // if they are not, write buffer to dataoutfile
    uint8_t ref = buffer[7];
    uint8_t found_different = 0;
    if (ref <= 8 && ref > 0) {
        for (i = 1; i < ref; i++) {
            if (buffer[7-i] != ref) {
                found_different = 1;
            }
        }
        if (!found_different) {
            fwrite(buffer, 1, 8-ref, dataoutfile);
            fclose(dataoutfile);
            return;
        }        
    }
    // error format in last block
    // print error to stderr and exit
    fprintf(stderr, "Error: format in last block is incorrect\n");
    fclose(dataoutfile);
    fclose(datainfile);
    exit(1);
}

void encryptdes3(FILE *datainfile, struct key key, char *dataoutfilename) {
    int length;
    int mod_length;
    int padding;
    char buffer[9] = {0};
    uint32_t driver_buffer[13] = {0};
    int i;
    FILE *dataoutfile;

    length = get_file_length(datainfile);
    mod_length = length % 8;
    padding = 8 - mod_length;


    // open dataoutfile for writing
    if ((dataoutfile = fopen(dataoutfilename, "w")) == NULL) {
        // print error to stderr and exit
        fprintf(stderr, "Error: cannot open dataoutfile\n");
        fclose(datainfile);
        exit(1);
    }

    for (i = 0; i < length-mod_length; i+=8) {
        // read 8 bytes from datainfile and store in buffer
        fread(buffer, 1, 8, datainfile);
        // call driver to encrypt buffer
        write_to_driver(buffer, key, DES3_ENCRYPT);
        read_from_driver(buffer);
        // write buffer to dataoutfile
        fwrite(buffer, 1, 8, dataoutfile);
    }
    if (padding != 8) {
        fread(buffer, 1, mod_length, datainfile);
        for (i = 0; i < padding; i++) {
            buffer[mod_length+i] = padding;
        }
        // call driver to encrypt buffer
        write_to_driver(buffer, key, DES3_ENCRYPT);
        read_from_driver(buffer);
        // write buffer to dataoutfile
        fwrite(buffer, 1, 8, dataoutfile);
    }
    else {
        fread(buffer, 1, 8, datainfile);
        // call driver to encrypt buffer
        write_to_driver(buffer, key, DES3_ENCRYPT);
        read_from_driver(buffer);
        // write buffer to dataoutfile
        fwrite(buffer, 1, 8, dataoutfile);

        for(i = 0; i < padding; i++) {
            buffer[i] = padding;
        }
        // call driver to encrypt buffer
        write_to_driver(buffer, key, DES3_ENCRYPT);
        read_from_driver(buffer);
        // write buffer to dataoutfile
        fwrite(buffer, 1, 8, dataoutfile);
    }
    fclose(dataoutfile);    
}

int main(int argc, char *argv[]) {
    // check if the number of arguments is 4
    if (argc != 5) {
        printf("Usage: %s <keyfile> <datainfile> <dataoutfile> <decrypt/encrypt>\n", argv[0]);
        return 1;
    }

    FILE *keyfile;
    FILE *datainfile;
    uint8_t decrypt;
    struct key key;
    
    // check if the last argument is 0 or 1 and assign to encrypt
    if (strcmp(argv[4], "0") != 0 && strcmp(argv[4], "1") != 0) {
        // print error to stderr and exit
        fprintf(stderr, "Error: the last argument must be 0 or 1\n");
        return 1;
    }
    else {
        decrypt = atoi(argv[4]);
    }

    // check if keyfile can be opened
    if ((keyfile = fopen(argv[1], "r")) == NULL) {
        // print error to stderr and exit
        fprintf(stderr, "Error: cannot open keyfile\n");
        return 1;
    }

    // check if datainfile can be opened
    if ((datainfile = fopen(argv[2], "r")) == NULL) {
        // print error to stderr and exit
        fprintf(stderr, "Error: cannot open datainfile\n");
        return 1;
    }

    key = get_key(keyfile);
    fclose(keyfile);

    // check validitiy of key
    // if invalid, print error to stderr and exit
    if (!validkey(key)) {
        fprintf(stderr, "Error: key is invalid\n");
        return 1;
    }

    if (decrypt) {
        decryptdes3(datainfile, key, argv[3]);
    }
    else {
        encryptdes3(datainfile, key, argv[3]);
    }
    fclose(datainfile);
    

    return 0;
}