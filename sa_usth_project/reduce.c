/**
 * @file reduce.c
 * @author Thi-Hong-Hanh TRAN (hanh.usth@gmail.com)
 * @brief This program implements the reduce part of the wordcount application.
 * It simply receives the content of a set of tables on STDIN, aggregates the
 * tables and computes the final count table which is sent on STDOUT. This
 * program is compiled into a binary which is independent from the tools used to
 * execute in the distributed environment.
 *
 * @version 0.1
 * @date 2019-04-01
 *
 * Copyright (c) 2019
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORDLEN 60 /* Max length of a word - not including trailing null */
#define MAX_WORDCOUNT 10000

int searchWord (char* word, char** words)
{
    for (int i = 0; i < MAX_WORDCOUNT; i++)
    {
        if (words[i] == NULL)
        {
            break;
        }
        if (strcmp (word, words[i]) == 0)
        {
            return i;
        }
    }

    return -1;
}

int main (int argc, char const* argv[])
{
    char word[WORDLEN + 1]; /* one word-allow extra space for final null */
    int  wordcount = 0;     /* count of number of words seen */

    char*        word_list[MAX_WORDCOUNT];
    unsigned int word_count[MAX_WORDCOUNT];
    memset ((void*)word_list, 0, sizeof word_list);
    memset ((void*)word_count, 0, sizeof word_list);

    while (1)
    {
        unsigned int count = 0;
        scanf ("%s %d", word, &count);

        if (count == 0)
        {
            break; /* End of file seen */
        }

        int index = searchWord (word, word_list);
        if (index < 0)  // not found in word_list
        {
            if (wordcount >= MAX_WORDCOUNT)
            {
                continue;
            }
            word_list[wordcount] = (char*)malloc (strlen (word + 1));
            strcpy (word_list[wordcount], word);
            word_count[wordcount] += count;
            wordcount++;
        }
        else
        {
            word_count[index] += count;
        }
    }

    for (int i = 0; i < wordcount; i++)
    {
        printf ("%s %d\n", word_list[i], word_count[i]);
    }
    return 0;
}
