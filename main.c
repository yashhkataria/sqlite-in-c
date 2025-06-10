#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "inputBuffer.c"

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

MetaCommandResult doMetaCommand(InputBuffer* inputBuffer) {
    if(strcmp(inputBuffer->buffer, ".exit") == 0) {
        printf("BYE\n");
        exit(EXIT_SUCCESS);
    }

    else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
} ExecuteResult;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    int id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
    StatementType type;
    Row rowToInsert;
} Statement;

#define sizeOfAttribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
#define ID_SIZE           sizeOfAttribute(Row, id)
#define USERNAME_SIZE     sizeOfAttribute(Row, username)
#define EMAIL_SIZE        sizeOfAttribute(Row, email)
#define ID_OFFSET         0
#define USERNAME_OFFSET   (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET      (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE          (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)
#define PAGE_SIZE         4096
#define TABLE_MAX_PAGES   100
#define ROWS_PER_PAGE     (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS    (ROWS_PER_PAGE * TABLE_MAX_PAGES)


void serializeRow(Row* source, void* destination) {
    memcpy(destination+ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination+USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy(destination+EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserializeRow(void* source, Row* destination) {
    memcpy(&(destination->id), source+ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source+USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source+EMAIL_OFFSET, EMAIL_SIZE);
}

typedef struct {
    uint32_t numRows;
    void* pages[TABLE_MAX_PAGES];
} Table;

void* rowSlot(Table* table, uint32_t rowNum) {
    uint32_t pageNum = rowNum / ROWS_PER_PAGE;
    void* page = table->pages[pageNum];

    if(page == NULL) {
        page = table->pages[pageNum] = malloc(PAGE_SIZE);
    }

    uint32_t rowOffset = rowNum % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;

    return (page + byteOffset);
}

void printRow(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

PrepareResult prepareStatement(InputBuffer* inputBuffer, Statement* statement) {
    if(strncmp(inputBuffer->buffer, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;
        int argsAssigned = sscanf(inputBuffer->buffer, "insert %d %s %s",
                                  &statement->rowToInsert.id,
                                  statement->rowToInsert.username,
                                  statement->rowToInsert.email);

        if(argsAssigned < 3) {
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }

        return PREPARE_SUCCESS;
    }

    if(strncmp(inputBuffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult executeInsert(Statement* statement, Table* table) {
    if(table->numRows >= TABLE_MAX_ROWS)  {
        return EXECUTE_TABLE_FULL;
    }

    Row* rowToInsert = &(statement->rowToInsert);

    serializeRow(rowToInsert, rowSlot(table, table->numRows));
    table->numRows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult executeSelect(Statement* statement, Table* table) {
    Row row;
    for(uint32_t i = 0; i < table->numRows; i++) {
        deserializeRow(rowSlot(table, i), &row);
        printRow(&row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult executeStatement(Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            return executeInsert(statement, table);

        case STATEMENT_SELECT:
            return executeSelect(statement, table);
    }
}

void printPrompt() {
    printf("db > ");
}

Table* newTable() {
    Table* table = malloc(sizeof(Table));
    table->numRows = 0;

    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }

    return table;
}

void freeTable(Table* table) {
    for(int i=0;i<TABLE_MAX_PAGES;i++) {
        free(table->pages[i]);
    }

    free(table);
}

void start(InputBuffer* inputBuffer) {
    Table* table = newTable();

    while(true) {
        printPrompt();
        readInput(inputBuffer);

        if(inputBuffer->buffer[0] == '.') {
            switch (doMetaCommand(inputBuffer)) {
                case (META_COMMAND_SUCCESS):
                    continue;

                case (META_COMMAND_UNRECOGNIZED_COMMAND):
                    printf("Unrecognized command '%s'\n", inputBuffer->buffer);
                    continue;
            }
        }

        Statement statement;
        switch (prepareStatement(inputBuffer, &statement)) {
            case (PREPARE_SUCCESS):
                break;

            case (PREPARE_SYNTAX_ERROR):
                printf("Unrecognized keyword at start of '%s'\n", inputBuffer->buffer);
                continue;

            case (PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'\n", inputBuffer->buffer);
                continue;
        }

        switch (executeStatement(&statement, table)) {
            case (EXECUTE_SUCCESS):
                printf("Executed.\n");
                break;

            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full.\n");
                break;
        }
    }
}

int main() {

    InputBuffer* inputBuffer = newInputBuffer();
    start(inputBuffer);

    return 0;
}