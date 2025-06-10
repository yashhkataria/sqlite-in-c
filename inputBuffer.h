#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

#include <stdlib.h>

typedef struct {
    char *buffer;
    size_t bufferLength;
    size_t inputLength;
} InputBuffer;

InputBuffer* newInputBuffer();
void readInput(InputBuffer* inputBuffer);
void closeInputBuffer(InputBuffer* inputBuffer);

#endif