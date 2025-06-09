#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int table[19][128], i, j, k;
int input;
char buffi[2][1024], Lexeme[2048];
int LoadTable();
void BufferSwap();
const char *stages[20] = {"Start", "LT", "GT", "EQUALS", "LTE", "NTE", "GTE", "EQS", "OP", "DELIM", "WS", "KEYWORD", "ALPHA", "NUM", "INT.", "DOUBLE", "E", "(+/-)", "Cdouble", "ERROR"};
const char *KEYWORDS[16] = {"def", "fed", "int", "double", "if", "fi", "while", "od", "print", "return", "or", "and", "not", "then", "else", "do"};
char error[3];
int ErrorInd = sizeof(stages) / sizeof(stages[0]) - 1;

int currbuffi = 0, currPos = 0;
int Currstate = 0, Prevstate = 0;
int bytesRead = 0;
int CurrLine = 1;
int m = 0;

int main(int argc, char *argv[])
{
    if ((input = open("test.txt", O_RDONLY)) == -1)
    {
        perror("Error opening file");
        return 1;
    }
    FILE *f = fopen("Lexeme.txt", "w");
    FILE *x = fopen("error_log.txt", "w");

    LoadTable();
    BufferSwap();
    Process();
    Print();
    Error();
    ErrorPrint();

    close(input);
    return 0;
}

int LoadTable()
{
    int value;
    FILE *Ftable = fopen("table.txt", "r");
    if (!Ftable)
    {
        perror("Error opening table.txt");
        exit(1);
    }

    for (i = 0; i < 19; i++)
    {
        for (j = 0; j < 128; j++)
        {
            fscanf(Ftable, "%d", &value);
            table[i][j] = value;
        }
    }

    fclose(Ftable);
    return 0;
}

void BufferSwap()
{
    currbuffi = 1 - currbuffi; // Swap buffer

    bytesRead = read(input, buffi[currbuffi], sizeof(buffi[currbuffi]));
    currPos = 0;
}

void Process()
{
    j = 0;
    k = 0;
    CurrLine = 0;
    while (bytesRead > 0 && currPos < bytesRead) // Only process valid bytes
    {
        Prevstate = Currstate;

        Currstate = table[Currstate][(int)buffi[currbuffi][currPos]];
        if (buffi[currbuffi][currPos] == '\n' && Prevstate != 0)
        {

            CurrLine++; // Only count if not preceded by '\r'
        }
        if (Currstate == -1 && Prevstate != 0)
        {

            Lexeme[j] = '\0';
            Error();
            j++;

            if (Prevstate == 11)
            {
                Prevstate = CheckKeywords();
            }
        }
        else
        {

            Lexeme[j] = buffi[currbuffi][currPos];

            j++;
        }

        if (Currstate == 0 && Prevstate != 0)
        {

            currPos--;
            Lexeme[j - 1] = '\0'; // Properly null-terminate the string

            if (Prevstate != 10)
            {

                if (Prevstate == 11)
                {
                    Prevstate = CheckKeywords();
                }

                if (Prevstate != -1 && Currstate == 0)
                {

                    Print();

                    printf("%s, %s\n", stages[Prevstate], Lexeme);
                    Lexeme[j] = '\0';
                }

                if (Prevstate == -1)
                {
                    ErrorPrint();
                    printf("Error at line %d: %s, %c\n", CurrLine, stages[ErrorInd], error[0]);

                    error[0] = '\0';
                }
            }
            j = 0;
        }

        currPos++;

        // **BUFFER SWITCH CHECK**
        if (currPos >= bytesRead) // If buffer is exhausted
        {
            BufferSwap();
        }
    }

    if (Prevstate != 10)
    {
        if (Prevstate == 11)
        {
            Prevstate = CheckKeywords();
        }
        Lexeme[j] = '\0';
        printf("%d\n", CurrLine);
        printf("%s,%s\n", stages[Prevstate], Lexeme);
    }
}

int CheckKeywords()
{
    for (i = 0; i < 16; i++)
    {
        if (strcmp(Lexeme, KEYWORDS[i]) == 0)
            return 11;
    }
    return 12;
}

void Print()
{
    FILE *f = fopen("Lexeme.txt", "a");
    if (f)
    {

        fprintf(f, "%s,%s\n", stages[Prevstate], Lexeme);

        fclose(f);
    }
}
void Error()
{

    Currstate = table[Currstate][(int)buffi[currbuffi][currPos]];

    if (Currstate == 0)
    {

        int m = 0;
        Lexeme[j] = '\0';

        error[0] = buffi[currbuffi][currPos];
    }
    else
    {

        Lexeme[j] = buffi[currbuffi][currPos];
        Lexeme[j] = '\0';
    }
}
void ErrorPrint()
{
    FILE *x = fopen("error_log.txt", "a");
    if (x)
    {
        fprintf(x, "Line %d: %s, %c\n", CurrLine, stages[ErrorInd], error[0]);

        fclose(x);
    }
}
