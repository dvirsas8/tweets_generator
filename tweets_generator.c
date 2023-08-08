#include "linked_list.h"
#include "markov_chain.h"

#include <stdio.h>  // For printf(), sscanf()
#include <stdlib.h> // For exit(), malloc()
#include <stdbool.h> // for bool
#include <string.h>

#define USG_ERR "Usage: argument number must be 3 or 4."
#define ERR_MSG "Error: Given path is corrupted or unreachable."

#define TWEET "Tweet "
#define MAX_WORDS 20
#define MAX_TWEET 1000
#define DELIM "\n "
#define DOT_ASCII 46
#define MAX_WORD_LENGTH 100

#define MIN_ARGS 4
#define MAX_ARGS 5

typedef struct NeededValues
{
    int seed;
    int tweet_num;
    int read_num;
    FILE *fp;
} NeededValues;

/**
 * fills database
 * @param fp File to read tweets from
 * @param markov_chain markov chain
 * @param words_to_read number of words to read from the file
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
fill_database (FILE *fp, int words_to_read, MarkovChain *markov_chain)
{
  char str[MAX_TWEET];
  MarkovNode *temp = NULL;
  char *word = NULL;
  char *k = fgets (str, MAX_TWEET, fp);
  while (k != NULL && words_to_read != 0)
  {
    while (words_to_read != 0)
    {
      word = strtok (word == NULL ? str : NULL, DELIM);
      if (word == NULL)
      {
        break;
      }
      Node *new = add_to_database (markov_chain, word);
      if (new == NULL)
      {
        fclose (fp);
        free_markov_chain (&markov_chain);
        return EXIT_FAILURE;
      }
      if (temp != NULL)
      {
        int i = add_node_to_counter_list (temp, new->data, markov_chain);
        if (i == false)
        {
          fclose (fp);
          free_markov_chain (&markov_chain);
          return EXIT_FAILURE;
        }
      }
      temp = new->data;
      words_to_read--;
    }
    k = fgets (str, MAX_TWEET, fp);
  }
  return EXIT_SUCCESS;
}

/**
 * handles user input
 * @param argc number of arguments
 * @param argv arguments
 * @return struct containing loaded data if successful, empty struct otherwise
 */
static NeededValues handle_input (int argc, char **argv)
{
  if (argc == MIN_ARGS || argc == MAX_ARGS)
  {
    int read_num, seed, tweet_num;
    sscanf (argv[1], "%d", &seed);
    sscanf (argv[2], "%d", &tweet_num);
    char *path = argv[3];
    if (argc == MAX_ARGS)
    {
      sscanf (argv[4], "%d", &read_num);
    }
    else
    {
      read_num = -1;
    }
    FILE *fp = fopen (path, "r");
    if (fp == NULL)
    {
      printf (ERR_MSG);
      NeededValues ret = {0, 0, 0, NULL};
      return ret;
    }
    NeededValues ret = {seed, tweet_num, read_num, fp};
    return ret;
  }
  else
  {
    printf (USG_ERR);
    NeededValues ret = {0, 0, 0, NULL};
    return ret;
  }
}

/**
 * frees the string
 * @param str_p pointer to node containing the string
 */
static void str_free (void *str_p)
{
  MarkovNode *node = (MarkovNode *) str_p;
  free (node->data);
}

/**
 * Checks if there's a dot at the end of a string
 * @param str string
 * @return true if there is a dot, false otherwise
 */
static bool dot_at_end (const void *str_p)
{
  MarkovNode *cur = (MarkovNode *) str_p;
  char *str = cur->data;
  unsigned long t = strlen (str);
  char comp = *(str + t - 1);
  if (comp == DOT_ASCII || cur->is_last)
  {
    return true;
  }
  return false;
}

/**
 * copies a string
 * @param str_data data to copy
 * @return the copied string
 */
static void *str_copy (const void *str_data)
{
  char *str = (char *) str_data;
  void *ret = malloc (MAX_WORD_LENGTH);
  if (ret == NULL)
  {
    return NULL;
  }
  strcpy (ret, str);
  return ret;
}

/**
 * prints the string in given format
 * @param str_p pointer to node containing string
 */
static void str_print (const void *str_p)
{
  MarkovNode *cur = (MarkovNode *) str_p;
  char *str = cur->data;
  if (dot_at_end (cur))
  {
    printf ("%s\n", str);
    return;
  }
  printf ("%s ", str);
}

/**
 * compares two strings
 * @param a first string
 * @param b secong string
 * @return 0 if the same, the difference in their ASCII otherwise
 */
static int str_cmp (const void *a, const void *b)
{
  const char *str1 = (char *) a, *str2 = (char *) b;
  return strcmp (str1, str2);
}

/**
 * fills the chain with needed functions
 * @param markov_chain markov chain to be filled
 */
static void set_chain (MarkovChain *markov_chain)
{
  markov_chain->free_data = str_free;
  markov_chain->copy_func = str_copy;
  markov_chain->print_func = str_print;
  markov_chain->comp_func = str_cmp;
  markov_chain->is_last = dot_at_end;
}

/**
 * initiates linked list
 * @return initiated linked list
 */
LinkedList *init_linked (void)
{
  LinkedList *linked = malloc (sizeof (*linked));
  if (linked == NULL)
  {
    printf (ALLOCATION_ERROR_MASSAGE);
    return NULL;
  }
  linked->first = NULL;
  linked->last = NULL;
  linked->size = 0;
  return linked;
}

int main (int argc, char **argv)
{
  NeededValues input = handle_input (argc, argv);
  if (input.fp == NULL)
  {
    return EXIT_FAILURE;
  }
  int read_num = input.read_num, tweet_num = input.tweet_num,seed = input.seed;
  FILE *fp = input.fp;
  MarkovChain *chain = malloc (sizeof (*chain));
  if (chain == NULL)
  {
    printf (ALLOCATION_ERROR_MASSAGE);
    fclose (fp);
    return EXIT_FAILURE;
  }
  LinkedList *linked = init_linked ();
  if (linked == NULL)
  {
    fclose (fp);
    free (chain);
    return EXIT_FAILURE;
  }
  chain->database = linked;
  set_chain (chain);
  int suc = fill_database (fp, read_num, chain);
  if (suc == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  srand (seed);
  int i = 1;
  while (tweet_num >= i)
  {
    printf (TWEET);
    printf ("%d: ", i);
    generate_random_sequence (chain, NULL, MAX_WORDS);
    i++;
  }
  free_markov_chain (&chain);
  fclose (fp);
  return EXIT_SUCCESS;
}