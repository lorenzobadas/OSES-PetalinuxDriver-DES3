/*
* Copyright (C) 2013-2022  Xilinx, Inc.  All rights reserved.
* Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in this
* Software without prior written authorization from Xilinx.
*
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

void split_key(const char *hex_key, int *var1, int *var2, int *var3, int *var4, int *var5, int *var6);
void split_input(const char *hex_input, int *var1, int *var2);

int main(int argc, char **argv)
{
    FILE *fptr;
    char *endptr;

    
    //                          key1HI      key1LO      key2HI      key2LO      key3HI      key3LO      desinHI     desinLO      decrypt
    uint32_t buffer[13];// = {0x6d6e7364, 0x6e7a6e62, 0x61646173, 0x31323234, 0x34373738, 0x38313838, 0x6369616F, 0x6369616F, 0x00000000};

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <key> <input> <decrypt>\n", argv[0]);
        return 1; // Exit with an error code
    }
    
    split_key(argv[1], &buffer[0], &buffer[1], &buffer[2], &buffer[3], &buffer[4], &buffer[5]);
    split_input(argv[2], &buffer[6], &buffer[7]);
    buffer[8] = strtol(argv[3], &endptr, 16);
    // Check for conversion errors
    if (*endptr != '\0') {
        fprintf(stderr, "Error: Invalid hexadecimal input\n");
        return 1; // Exit with an error code
    }

    // scrivi i dati in ingresso
    fptr = fopen("/dev/des3module", "w");
    fwrite(buffer, sizeof(uint32_t), 9, fptr);
    fclose(fptr);

    // leggi i risultati
    fptr = fopen("/dev/des3module", "r");
    fread(buffer, sizeof(uint32_t), sizeof(buffer)/sizeof(buffer[0]), fptr);
    fclose(fptr);

    /*
    printf("des_in  : 0x%08X%08X\n", buffer[0], buffer[1]);
    printf("decrypt : 0x%08X\n",     buffer[8]);
    printf("key1    : 0x%08X%08X\n", buffer[2], buffer[3]);
    printf("key2    : 0x%08X%08X\n", buffer[4], buffer[5]);
    printf("key3    : 0x%08X%08X\n", buffer[6], buffer[7]);
    printf("des_out : 0x%08X%08X\n", buffer[9], buffer[10]);
    printf("ready   : 0x%08X\n",     buffer[11]);
    printf("validkey: 0x%08X\n\n",   buffer[12]);
    */

    if (!buffer[12]) { // if key not valid
        fprintf(stderr, "Specified key does not conform to TripleDES standard\n");
        return 1;
    }
    printf("DES output: 0x%08X%08X\n", buffer[9], buffer[10]);

    return 0;
}

// Function to split a 192-bit hexadecimal key into six 32-bit chunks
void split_key(const char *hex_key, int *var1, int *var2, int *var3, int *var4, int *var5, int *var6) {
    char padded_key[49];
    int i, j;
    memset(padded_key, 0, sizeof(padded_key));
    // Ensure the input string is of the correct length
    size_t input_length = strlen(hex_key);
    if (input_length > 48) {
        fprintf(stderr, "Error: The hexadecimal key should be at most 192 bits long.\n");
        exit(1);
    }

    for (i = 47, j = input_length-1; j >= 0; i--, j--) {
        padded_key[i] = hex_key[j];
    }

    // Convert each 32-bit chunk from hexadecimal string to integer
    if (sscanf(padded_key, "%8x%8x%8x%8x%8x%8x", var1, var2, var3, var4, var5, var6) != 6) {
        fprintf(stderr, "Error: Failed to parse the hexadecimal key.\n");
        exit(1);
    }
}

// Function to split a 64-bit hexadecimal input into two 32-bit chunks
void split_input(const char *hex_input, int *var1, int *var2) {
    char padded_input[17];
    int i, j;
    memset(padded_input, 0, sizeof(padded_input));
    // Ensure the input string is of the correct length
    size_t input_length = strlen(hex_input);
    if (input_length > 16) {
        fprintf(stderr, "Error: The hexadecimal input should be at most 64 bits long.\n");
        exit(1);
    }

    for (i = 15, j = input_length-1; j >= 0; i--, j--) {
        padded_input[i] = hex_input[j];
    }

    // Convert each 32-bit chunk from hexadecimal string to integer
    if (sscanf(padded_input, "%8x%8x", var1, var2) != 2) {
        fprintf(stderr, "Error: Failed to parse the hexadecimal input.\n");
        exit(1);
    }
}